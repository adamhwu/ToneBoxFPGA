"""Independent PyTorch inference for legacy NAM A1 WaveNet captures.

Loads the architecture and flattened weights from a trainer-exported ``.nam``
file. It deliberately does not import NAM's Python model implementation.

Usage:
    python -m custom_nam.engine.a1_model custom_nam/data/output.nam custom_nam/data/clean_1.wav custom_nam/data/dirty_1.wav

The model runs at the sample rate stored in the .nam file. Audio at another
rate is resampled for inference and resampled back for the output WAV.
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Any

import numpy as np
import torch
from scipy.io import wavfile
from scipy.signal import resample_poly
from torch import nn
from torch.nn import functional as F


class WeightReader:
    """Consume NAM's flattened, PyTorch-order convolution weights."""

    def __init__(self, weights: list[float]):
        self.values = torch.tensor(weights, dtype=torch.float32)
        self.offset = 0

    def take(self, shape: tuple[int, ...]) -> torch.Tensor:
        count = math.prod(shape)
        end = self.offset + count
        if end > self.values.numel():
            raise ValueError(f".nam weights end at {self.offset}; need {count} more")
        result = self.values[self.offset:end].reshape(shape)
        self.offset = end
        return result

    def load_conv(self, conv: nn.Conv1d) -> None:
        with torch.no_grad():
            conv.weight.copy_(self.take(tuple(conv.weight.shape)))
            if conv.bias is not None:
                conv.bias.copy_(self.take(tuple(conv.bias.shape)))

    def finish(self) -> None:
        if self.offset != self.values.numel():
            raise ValueError(
                f".nam has {self.values.numel()} weights, but architecture used {self.offset}"
            )


class CausalConv(nn.Module):
    def __init__(
        self, in_channels: int, out_channels: int, kernel: int,
        dilation: int = 1, bias: bool = True, groups: int = 1,
    ):
        super().__init__()
        self.lookback = (kernel - 1) * dilation
        self.conv = nn.Conv1d(
            in_channels, out_channels, kernel, dilation=dilation,
            padding=0, bias=bias, groups=groups,
        )

    def forward(
        self, x: torch.Tensor, state: dict | None = None
    ) -> torch.Tensor:
        if state is None:
            return self.conv(F.pad(x, (self.lookback, 0)))
        if self.lookback == 0:
            return self.conv(x)
        history = state.get(self)
        if history is None:
            history = x.new_zeros((*x.shape[:-1], self.lookback))
        window = torch.cat((history, x), dim=-1)
        state[self] = window[..., -self.lookback:].detach()
        return self.conv(window)


def _activation(spec: Any) -> nn.Module:
    # Classic A1 exports use "Tanh". Permit the other parameter-free activations
    # used by WaveNet configurations, while rejecting unknown trained parameters.
    if isinstance(spec, dict):
        name = spec.get("name", spec.get("type"))
    else:
        name = spec
    key = str(name).lower()
    if key == "tanh":
        return nn.Tanh()
    if key == "relu":
        return nn.ReLU()
    if key == "sigmoid":
        return nn.Sigmoid()
    if key == "softsign":
        return nn.Softsign()
    if key == "leakyrelu":
        slope = spec.get("negative_slope", 0.01) if isinstance(spec, dict) else 0.01
        return nn.LeakyReLU(float(slope))
    if key in ("identity", "linear"):
        return nn.Identity()
    raise ValueError(f"Unsupported activation in this A1 loader: {spec!r}")


def _per_layer(value: Any, count: int, label: str) -> list[Any]:
    if isinstance(value, list):
        if len(value) != count:
            raise ValueError(f"{label} has {len(value)} entries for {count} layers")
        return value
    return [value] * count


def _enabled_option(config: dict, name: str, default: bool) -> tuple[bool, int]:
    option = config.get(name)
    if option is None:
        return default, 1
    if isinstance(option, bool):
        return option, 1
    return bool(option.get("active", default)), int(option.get("groups", 1))


class A1Layer(nn.Module):
    def __init__(
        self, channels: int, condition_channels: int, bottleneck: int,
        kernel: int, dilation: int, activation: Any, gating: str,
        secondary_activation: Any, conv_groups: int,
        mixin_groups: int, residual_active: bool, residual_groups: int,
        head1x1: dict | None,
    ):
        super().__init__()
        if gating not in ("none", "gated", "blended"):
            raise ValueError(f"Unsupported gating mode: {gating!r}")
        self.gating = gating
        self.bottleneck = bottleneck
        conv_outputs = bottleneck * (2 if gating != "none" else 1)
        self.dilated = CausalConv(
            channels, conv_outputs, kernel, dilation, groups=conv_groups
        )
        self.mixin = nn.Conv1d(
            condition_channels, conv_outputs, 1,
            bias=False, groups=mixin_groups,
        )
        self.activation = _activation(activation)
        self.secondary_activation = (
            _activation(secondary_activation) if gating != "none" else None
        )
        self.residual = (
            nn.Conv1d(bottleneck, channels, 1, groups=residual_groups)
            if residual_active else None
        )
        self.head1x1 = (
            nn.Conv1d(
                bottleneck, int(head1x1["out_channels"]), 1,
                groups=int(head1x1.get("groups", 1)),
            )
            if head1x1 and head1x1.get("active", False) else None
        )

    def load_weights(self, reader: WeightReader) -> None:
        for conv in (self.dilated.conv, self.mixin, self.residual, self.head1x1):
            if conv is not None:
                reader.load_conv(conv)

    def forward(
        self, hidden: torch.Tensor, condition: torch.Tensor,
        state: dict | None = None,
    ) -> tuple[torch.Tensor, torch.Tensor]:
        z = self.dilated(hidden, state) + self.mixin(condition)
        if self.gating == "none":
            activated = self.activation(z)
        else:
            first, second = z.split(self.bottleneck, dim=1)
            a = self.activation(first)
            b = self.secondary_activation(second)
            activated = a * b if self.gating == "gated" else a * b + first * (1 - b)
        next_hidden = hidden + self.residual(activated) if self.residual else hidden
        head = self.head1x1(activated) if self.head1x1 else activated
        return next_hidden, head


class A1Stack(nn.Module):
    def __init__(self, config: dict):
        super().__init__()
        film_names = (
                "conv_pre_film", "conv_post_film", "input_mixin_pre_film",
                "input_mixin_post_film", "activation_pre_film",
                "activation_post_film", "layer1x1_post_film",
                "head1x1_post_film",
        )
        unsupported = [
            name for name in film_names
            if config.get(name) and not (
                isinstance(config[name], dict) and not config[name].get("active", True)
            )
        ]
        if unsupported:
            raise ValueError(f"FiLM options are outside classic A1: {unsupported}")

        self.input_size = int(config["input_size"])
        self.condition_size = int(config["condition_size"])
        channels = int(config["channels"])
        bottleneck = int(config.get("bottleneck", channels))
        dilations = [int(d) for d in config["dilations"]]
        if not dilations or min(dilations) < 1:
            raise ValueError("dilations must be positive and nonempty")
        count = len(dilations)
        kernels = _per_layer(
            config.get("kernel_sizes", config.get("kernel_size")), count, "kernel sizes"
        )
        activations = _per_layer(config["activation"], count, "activations")
        gates = _per_layer(
            config.get("gating_mode", "gated" if config.get("gated", False) else "none"),
            count, "gating modes",
        )
        secondary = _per_layer(
            config.get("secondary_activation", "Sigmoid"), count,
            "secondary activations",
        )
        residual_active, residual_groups = _enabled_option(config, "layer1x1", True)
        if not residual_active and bottleneck != channels:
            raise ValueError("inactive layer1x1 needs bottleneck == channels")
        head1x1 = config.get("head1x1")
        head_channels = (
            int(head1x1["out_channels"])
            if head1x1 and head1x1.get("active", False) else bottleneck
        )
        self.head_channels = head_channels
        head_config = config.get("head")
        if head_config:
            self.head_size = int(head_config["out_channels"])
            head_kernel = int(head_config["kernel_size"])
            head_bias = bool(head_config["bias"])
        else:
            self.head_size = int(config["head_size"])
            head_kernel = 1
            head_bias = bool(config["head_bias"])
        self.receptive_field = 1 + sum(
            (int(k) - 1) * d for k, d in zip(kernels, dilations)
        ) + head_kernel - 1

        self.rechannel = nn.Conv1d(self.input_size, channels, 1, bias=False)
        self.layers = nn.ModuleList([
            A1Layer(
                channels, self.condition_size, bottleneck, int(kernel), dilation,
                activation, str(gate).lower(), secondary_activation,
                int(config.get("groups_input", 1)),
                int(config.get("groups_input_mixin", 1)),
                residual_active, residual_groups, head1x1,
            )
            for kernel, dilation, activation, gate, secondary_activation in zip(
                kernels, dilations, activations, gates, secondary
            )
        ])
        self.head_rechannel = CausalConv(
            head_channels, self.head_size, head_kernel, bias=head_bias
        )
        self.channels = channels

    def load_weights(self, reader: WeightReader) -> None:
        reader.load_conv(self.rechannel)
        for layer in self.layers:
            layer.load_weights(reader)
        reader.load_conv(self.head_rechannel.conv)

    def forward(
        self, x: torch.Tensor, condition: torch.Tensor,
        previous_head: torch.Tensor | None, state: dict | None = None,
    ) -> tuple[torch.Tensor, torch.Tensor]:
        hidden = self.rechannel(x)
        skip_sum = None
        for layer in self.layers:
            hidden, skip = layer(hidden, condition, state)
            skip_sum = skip if skip_sum is None else skip_sum + skip
        if previous_head is not None:
            if previous_head.shape[1] != skip_sum.shape[1]:
                raise ValueError("Adjacent stacks have incompatible head channels")
            skip_sum = skip_sum + previous_head
        return hidden, self.head_rechannel(skip_sum, state)


class A1Model(nn.Module):
    """Causal A1 WaveNet with an arbitrary number of exported stacks."""

    def __init__(self, config: dict):
        super().__init__()
        if config.get("condition_dsp") is not None:
            raise ValueError("condition_dsp models are outside the classic A1 architecture")
        stack_configs = config.get("layers")
        if not stack_configs:
            raise ValueError("Expected exported WaveNet config.layers")
        self.stacks = nn.ModuleList(A1Stack(c) for c in stack_configs)
        self.input_channels = self.stacks[0].input_size
        if self.input_channels != self.stacks[0].condition_size:
            raise ValueError("Classic A1 conditions each stack on its original input")
        for previous, current in zip(self.stacks, self.stacks[1:]):
            if previous.channels != current.input_size:
                raise ValueError("Adjacent stacks have incompatible main channels")
            if previous.head_size != current.head_channels:
                raise ValueError("Adjacent stacks have incompatible head channels")
            if current.condition_size != self.input_channels:
                raise ValueError("Classic A1 uses the original input as condition")
        self.receptive_field = 1 + sum(s.receptive_field - 1 for s in self.stacks)
        head = config.get("head")
        if head is not None:
            widths = [int(k) for k in head["kernel_sizes"]]
            layers = []
            in_channels = self.stacks[-1].head_size
            for i, kernel in enumerate(widths):
                out_channels = (
                    int(head["out_channels"]) if i == len(widths) - 1
                    else int(head["channels"])
                )
                layers.append(CausalConv(in_channels, out_channels, kernel))
                in_channels = out_channels
            self.post_head = nn.ModuleList(layers)
            self.post_activation = _activation(head["activation"])
            self.receptive_field += sum(k - 1 for k in widths)
            self.output_channels = in_channels
        else:
            self.post_head = None
            self.output_channels = self.stacks[-1].head_size
        self.head_scale = float(config.get("head_scale", 1.0))

    def load_weights(self, weights: list[float]) -> None:
        reader = WeightReader(weights)
        for stack in self.stacks:
            stack.load_weights(reader)
        if self.post_head is not None:
            for layer in self.post_head:
                reader.load_conv(layer.conv)
        self.head_scale = float(reader.take((1,))[0])
        reader.finish()

    def forward(self, x: torch.Tensor, state: dict | None = None) -> torch.Tensor:
        if x.ndim == 2:
            x = x[:, None, :]
        if x.ndim != 3 or x.shape[1] != self.input_channels:
            raise ValueError(f"Expected (batch, {self.input_channels}, samples)")
        condition = x
        head = None
        for stack in self.stacks:
            x, head = stack(x, condition, head, state)
        out = head * self.head_scale
        if self.post_head is not None:
            for layer in self.post_head:
                out = layer(self.post_activation(out), state)
        return out


class A1Streamer:
    """Retain each convolution's history across live audio blocks."""

    def __init__(self, model: A1Model):
        self.model = model.eval()
        self.state: dict[CausalConv, torch.Tensor] = {}
        self.reset()

    @torch.inference_mode()
    def reset(self) -> None:
        self.state.clear()
        remaining = self.model.receptive_field - 1
        while remaining:
            frames = min(remaining, 512)
            self.model(
                torch.zeros(1, self.model.input_channels, frames), self.state
            )
            remaining -= frames

    @torch.inference_mode()
    def process(self, mono: np.ndarray) -> np.ndarray:
        x = torch.from_numpy(np.asarray(mono, dtype=np.float32))[None, None, :]
        return self.model(x, self.state)[0, 0].cpu().numpy()


def load_nam(path: str | Path) -> tuple[A1Model, int]:
    with open(path, encoding="utf-8") as file:
        capture = json.load(file)
    if capture.get("architecture") != "WaveNet":
        raise ValueError(f"Expected A1 WaveNet, got {capture.get('architecture')!r}")
    model = A1Model(capture["config"])
    model.load_weights(capture["weights"])
    return model.eval(), int(capture.get("sample_rate", 48000))


@torch.inference_mode()
def process_in_blocks(
    model: A1Model, mono: np.ndarray, block_size: int = 16384
) -> np.ndarray:
    """Use overlap context and silent prewarm to match streaming NAM inference."""
    lookback = model.receptive_field - 1
    # The C++ runtime runs zeros through the network before the first sample.
    # This matters because layer biases make its hidden history nonzero even
    # while the audio input is silent.
    padded = np.pad(mono, (lookback, 0))
    output = np.empty(len(padded), dtype=np.float32)
    for start in range(0, len(padded), block_size):
        end = min(start + block_size, len(padded))
        context_start = max(0, start - lookback)
        segment = padded[context_start:end]
        x = torch.from_numpy(np.asarray(segment, dtype=np.float32))[None, None, :]
        y = model(x)
        output[start:end] = y[0, 0, -(end - start):].cpu().numpy()
    return output[lookback:]


def render_wav(
    model_path: str | Path, input_path: str | Path, output_path: str | Path,
    block_size: int = 16384,
) -> None:
    model, model_rate = load_nam(model_path)
    if model.input_channels != 1 or model.output_channels != 1:
        raise ValueError("WAV renderer currently requires mono input and output")
    input_rate, samples = wavfile.read(input_path)
    if samples.ndim != 1:
        raise ValueError("Expected a mono input WAV")
    if np.issubdtype(samples.dtype, np.integer):
        bits = samples.dtype.itemsize * 8
        mono = samples.astype(np.float32) / float(2 ** (bits - 1))
    else:
        mono = samples.astype(np.float32)
    if input_rate != model_rate:
        divisor = math.gcd(input_rate, model_rate)
        mono = resample_poly(mono, model_rate // divisor, input_rate // divisor).astype(np.float32)
    rendered = process_in_blocks(model, mono, block_size)
    if input_rate != model_rate:
        divisor = math.gcd(input_rate, model_rate)
        rendered = resample_poly(
            rendered, input_rate // divisor, model_rate // divisor
        ).astype(np.float32)
        rendered = rendered[:len(samples)]
        if len(rendered) < len(samples):
            rendered = np.pad(rendered, (0, len(samples) - len(rendered)))
    if not np.isfinite(rendered).all():
        raise ValueError("Model produced non-finite audio")
    wavfile.write(output_path, input_rate, rendered.astype(np.float32))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("model", type=Path, help="A1 WaveNet .nam file")
    parser.add_argument("input", type=Path, help="Mono input WAV")
    parser.add_argument("output", type=Path, help="Output float32 WAV")
    parser.add_argument("--block-size", type=int, default=16384)
    args = parser.parse_args()
    if args.block_size < 1:
        parser.error("--block-size must be positive")
    render_wav(args.model, args.input, args.output, args.block_size)
    print(f"Wrote {args.output}")


if __name__ == "__main__":
    main()
