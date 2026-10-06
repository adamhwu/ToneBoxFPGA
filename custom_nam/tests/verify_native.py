"""Check the C++ live processor against the independent PyTorch A1 reference."""

from __future__ import annotations

import ctypes as C
from pathlib import Path

import numpy as np
import torch

from custom_nam.engine.a1_model import A1Streamer, load_nam
from custom_nam.engine.native_a1 import NativeA1


def main() -> None:
    path = Path(__file__).resolve().parents[1] / "data" / "output.nam"
    torch.set_num_threads(1)
    reference_model, sample_rate = load_nam(path)
    reference = A1Streamer(reference_model)
    native = NativeA1(path)
    try:
        assert native.sample_rate == sample_rate
        assert native.receptive_field == reference_model.receptive_field
        rng = np.random.default_rng(2026)
        differences = []
        for frames in (1, 17, 128, 256, 64, 512, 1024):
            source = (rng.standard_normal(frames) * 0.1).astype(np.float32)
            result = np.empty_like(source)
            status = native.lib.a1_process_mono(
                native.handle,
                source.ctypes.data_as(C.POINTER(C.c_float)),
                result.ctypes.data_as(C.POINTER(C.c_float)),
                frames,
            )
            if status:
                raise RuntimeError(native.error_message())
            differences.append(float(np.max(np.abs(result - reference.process(source)))))
        error = max(differences)
        assert error < 2e-5, f"C++ and PyTorch differ by {error}"
        print(f"A1 streaming parity: maximum error {error:.2g} at {sample_rate} Hz")

        native.reset()
        source = (rng.standard_normal(128) * 0.1).astype(np.float32)
        incoming = np.zeros((128, 2), dtype=np.float32)
        incoming[:, 1] = source
        outgoing = np.empty_like(incoming)
        status = native.process_audio(
            incoming.ctypes.data, outgoing.ctypes.data, 128, 2, 1, 2, False
        )
        if status:
            raise RuntimeError(native.error_message())
        np.testing.assert_allclose(outgoing[:, 0], source, atol=1e-7)
        np.testing.assert_allclose(outgoing[:, 1], source, atol=1e-7)
        print("Live channel selection and bypass: OK")
    finally:
        native.close()


if __name__ == "__main__":
    main()
