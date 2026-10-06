"""Live Focusrite monitor for an A1 .nam capture.

Run from the repository root with ``python3 -m custom_nam.app.live_a1``.
The app uses ``custom_nam/data/output.nam`` by default and the locally built C++ A1 processor for
all audio computation, and the installed PortAudio library for audio I/O.
No Python audio package or global package installation is required.

Plug headphones or monitors into the selected output interface to avoid a
feedback loop through the computer's microphone.
"""

from __future__ import annotations

import argparse
import ctypes as C
import ctypes.util
from dataclasses import dataclass
from pathlib import Path
import tkinter as tk
from tkinter import messagebox, ttk

from custom_nam.engine.native_a1 import NativeA1


SAMPLE_FORMAT_FLOAT32 = 0x00000001
PA_CONTINUE = 0
FRAMES_PER_BUFFER = 128
# Ask PortAudio for the device's minimum viable latency. Its advertised
# defaultLow*Latency can add substantial extra buffering on CoreAudio.
SUGGESTED_LATENCY = 0.0
XRUN_FLAGS = 0x0F
FOCUSRITE_NAMES = ("focusrite", "scarlett", "clarett", "vocaster", "saffire")


class PaDeviceInfo(C.Structure):
    _fields_ = [
        ("structVersion", C.c_int),
        ("name", C.c_char_p),
        ("hostApi", C.c_int),
        ("maxInputChannels", C.c_int),
        ("maxOutputChannels", C.c_int),
        ("defaultLowInputLatency", C.c_double),
        ("defaultLowOutputLatency", C.c_double),
        ("defaultHighInputLatency", C.c_double),
        ("defaultHighOutputLatency", C.c_double),
        ("defaultSampleRate", C.c_double),
    ]


class PaStreamParameters(C.Structure):
    _fields_ = [
        ("device", C.c_int),
        ("channelCount", C.c_int),
        ("sampleFormat", C.c_ulong),
        ("suggestedLatency", C.c_double),
        ("hostApiSpecificStreamInfo", C.c_void_p),
    ]


class PaCallbackTimeInfo(C.Structure):
    _fields_ = [
        ("inputBufferAdcTime", C.c_double),
        ("currentTime", C.c_double),
        ("outputBufferDacTime", C.c_double),
    ]


CALLBACK = C.CFUNCTYPE(
    C.c_int, C.c_void_p, C.c_void_p, C.c_ulong,
    C.POINTER(PaCallbackTimeInfo), C.c_ulong, C.c_void_p,
)


@dataclass(frozen=True)
class AudioDevice:
    index: int
    name: str
    inputs: int
    outputs: int
    low_input_latency: float
    low_output_latency: float

    @property
    def label(self) -> str:
        return f"{self.index}: {self.name}"


class PortAudio:
    def __init__(self) -> None:
        candidates = [
            ctypes.util.find_library("portaudio"),
            "/opt/homebrew/lib/libportaudio.dylib",
            "/usr/local/lib/libportaudio.dylib",
        ]
        library = None
        for path in filter(None, candidates):
            try:
                library = C.CDLL(path)
                break
            except OSError:
                continue
        if library is None:
            raise RuntimeError("PortAudio is unavailable on this machine")
        self.lib = library
        self.lib.Pa_Initialize.restype = C.c_int
        self.lib.Pa_Terminate.restype = C.c_int
        self.lib.Pa_GetDeviceCount.restype = C.c_int
        self.lib.Pa_GetDeviceInfo.argtypes = [C.c_int]
        self.lib.Pa_GetDeviceInfo.restype = C.POINTER(PaDeviceInfo)
        self.lib.Pa_GetDefaultInputDevice.restype = C.c_int
        self.lib.Pa_GetDefaultOutputDevice.restype = C.c_int
        self.lib.Pa_GetErrorText.argtypes = [C.c_int]
        self.lib.Pa_GetErrorText.restype = C.c_char_p
        self.lib.Pa_OpenStream.argtypes = [
            C.POINTER(C.c_void_p), C.POINTER(PaStreamParameters),
            C.POINTER(PaStreamParameters), C.c_double, C.c_ulong,
            C.c_ulong, CALLBACK, C.c_void_p,
        ]
        self.lib.Pa_OpenStream.restype = C.c_int
        self.lib.Pa_StartStream.argtypes = [C.c_void_p]
        self.lib.Pa_StartStream.restype = C.c_int
        self.lib.Pa_StopStream.argtypes = [C.c_void_p]
        self.lib.Pa_StopStream.restype = C.c_int
        self.lib.Pa_CloseStream.argtypes = [C.c_void_p]
        self.lib.Pa_CloseStream.restype = C.c_int
        self.check(self.lib.Pa_Initialize())

    def check(self, code: int) -> None:
        if code < 0:
            raise RuntimeError(self.lib.Pa_GetErrorText(code).decode(errors="replace"))

    def devices(self) -> list[AudioDevice]:
        count = self.lib.Pa_GetDeviceCount()
        self.check(count)
        devices = []
        for index in range(count):
            info = self.lib.Pa_GetDeviceInfo(index)
            if info:
                item = info.contents
                devices.append(AudioDevice(
                    index, item.name.decode(errors="replace"),
                    item.maxInputChannels, item.maxOutputChannels,
                    item.defaultLowInputLatency,
                    item.defaultLowOutputLatency,
                ))
        return devices

    def close(self) -> None:
        self.lib.Pa_Terminate()


class LiveApp:
    def __init__(self, root: tk.Tk, model_path: Path):
        self.root = root
        self.root.title("NAM A1 Live")
        self.root.resizable(False, False)
        self.model = NativeA1(model_path)
        self.sample_rate = self.model.sample_rate
        self.audio = PortAudio()
        self.stream = C.c_void_p()
        self.callback = None
        self.effect_enabled = False
        self.error: str | None = None
        self.xruns = 0
        self.input_channels = 1
        self.output_channels = 2
        self.selected_input_channel = 0
        self.devices: list[AudioDevice] = []

        main = ttk.Frame(root, padding=16)
        main.grid(sticky="nsew")
        self.input_choice = tk.StringVar()
        self.output_choice = tk.StringVar()
        self.channel_choice = tk.StringVar(value="1")
        self.status = tk.StringVar(value="Select your audio interface")

        ttk.Label(main, text="Input device").grid(row=0, column=0, sticky="w", pady=(0, 4))
        self.input_menu = ttk.Combobox(
            main, textvariable=self.input_choice, state="readonly", width=42
        )
        self.input_menu.grid(row=1, column=0, columnspan=2, sticky="ew", pady=(0, 10))
        self.input_menu.bind("<<ComboboxSelected>>", self._input_changed)

        ttk.Label(main, text="Output device").grid(row=2, column=0, sticky="w", pady=(0, 4))
        self.output_menu = ttk.Combobox(
            main, textvariable=self.output_choice, state="readonly", width=42
        )
        self.output_menu.grid(row=3, column=0, columnspan=2, sticky="ew", pady=(0, 10))
        self.output_menu.bind("<<ComboboxSelected>>", self._selection_changed)

        ttk.Label(main, text="Guitar input channel").grid(row=4, column=0, sticky="w")
        self.channel_menu = ttk.Combobox(
            main, textvariable=self.channel_choice, state="readonly", width=6
        )
        self.channel_menu.grid(row=4, column=1, sticky="e")

        self.effect_button = ttk.Button(
            main, text="Turn effect ON", command=self.toggle_effect
        )
        self.effect_button.grid(row=5, column=0, columnspan=2, sticky="ew", pady=(18, 8))
        self.stop_button = ttk.Button(main, text="Stop audio", command=self.stop)
        self.stop_button.grid(row=6, column=0, sticky="w")
        ttk.Button(main, text="Refresh devices", command=self.refresh).grid(
            row=6, column=1, sticky="e"
        )
        ttk.Label(main, textvariable=self.status).grid(
            row=7, column=0, columnspan=2, sticky="w", pady=(12, 0)
        )
        self.refresh()
        root.protocol("WM_DELETE_WINDOW", self.close)
        root.after(200, self._poll_status)

    def refresh(self) -> None:
        if self.stream.value:
            return
        previous_input = self.input_choice.get()
        previous_output = self.output_choice.get()
        self.devices = self.audio.devices()
        inputs = [d for d in self.devices if d.inputs]
        outputs = [d for d in self.devices if d.outputs]
        self.input_menu["values"] = [d.label for d in inputs]
        self.output_menu["values"] = [d.label for d in outputs]

        def choose(devices: list[AudioDevice], previous: str) -> str:
            if previous in (d.label for d in devices):
                return previous
            focusrite = next(
                (d for d in devices if any(name in d.name.lower() for name in FOCUSRITE_NAMES)),
                None,
            )
            return focusrite.label if focusrite else ""

        self.input_choice.set(choose(inputs, previous_input))
        self.output_choice.set(choose(outputs, previous_output))
        self._input_changed()

    def _selection_changed(self, _event=None) -> None:
        ready = bool(self.input_choice.get() and self.output_choice.get())
        self.effect_button.configure(state="normal" if ready else "disabled")
        if ready:
            self.status.set(f"Ready · {self.sample_rate} Hz")
        elif not self.devices:
            self.status.set("No audio devices detected. Connect the Focusrite and refresh.")
        else:
            self.status.set("Connect the Focusrite or select input and output devices.")

    def _device(self, label: str) -> AudioDevice:
        index = int(label.split(":", 1)[0])
        return next(d for d in self.devices if d.index == index)

    def _input_changed(self, _event=None) -> None:
        if not self.input_choice.get():
            self.channel_menu["values"] = []
            self._selection_changed()
            return
        count = self._device(self.input_choice.get()).inputs
        self.channel_menu["values"] = [str(i) for i in range(1, count + 1)]
        if self.channel_choice.get() not in self.channel_menu["values"]:
            self.channel_choice.set("1")
        self._selection_changed()

    def _audio_callback(
        self, input_ptr, output_ptr, frame_count, _time_info, flags, _user_data
    ) -> int:
        frames = int(frame_count)
        try:
            if flags & XRUN_FLAGS:
                self.xruns += 1
            if self.model.process_audio(
                input_ptr, output_ptr, frames, self.input_channels,
                self.selected_input_channel, self.output_channels,
                self.effect_enabled,
            ):
                raise RuntimeError(self.model.error_message())
        except Exception as exc:
            C.memset(output_ptr, 0, frames * self.output_channels * C.sizeof(C.c_float))
            self.error = str(exc)
        return PA_CONTINUE

    def _start(self) -> None:
        input_device = self._device(self.input_choice.get())
        output_device = self._device(self.output_choice.get())
        self.xruns = 0
        self.error = None
        self.selected_input_channel = int(self.channel_choice.get()) - 1
        self.input_channels = self.selected_input_channel + 1
        self.output_channels = min(2, output_device.outputs)
        self.model.reset()
        input_params = PaStreamParameters(
            input_device.index, self.input_channels, SAMPLE_FORMAT_FLOAT32,
            SUGGESTED_LATENCY, None,
        )
        output_params = PaStreamParameters(
            output_device.index, self.output_channels, SAMPLE_FORMAT_FLOAT32,
            SUGGESTED_LATENCY, None,
        )
        self.callback = CALLBACK(self._audio_callback)
        self.audio.check(self.audio.lib.Pa_OpenStream(
            C.byref(self.stream), C.byref(input_params), C.byref(output_params),
            float(self.sample_rate), FRAMES_PER_BUFFER, 0, self.callback, None,
        ))
        try:
            self.audio.check(self.audio.lib.Pa_StartStream(self.stream))
        except Exception:
            self.audio.lib.Pa_CloseStream(self.stream)
            self.stream = C.c_void_p()
            raise
        self.input_menu.configure(state="disabled")
        self.output_menu.configure(state="disabled")
        self.channel_menu.configure(state="disabled")

    def toggle_effect(self) -> None:
        try:
            if not self.stream.value:
                self._start()
            self.effect_enabled = not self.effect_enabled
            self.effect_button.configure(
                text="Turn effect OFF" if self.effect_enabled else "Turn effect ON"
            )
            self.status.set("Effect on" if self.effect_enabled else "Bypass on")
        except Exception as exc:
            self.status.set(f"Audio error: {exc}")
            messagebox.showerror("Could not start audio", str(exc))

    def stop(self) -> None:
        self.effect_enabled = False
        if self.stream.value:
            self.audio.lib.Pa_StopStream(self.stream)
            self.audio.lib.Pa_CloseStream(self.stream)
            self.stream = C.c_void_p()
        self.callback = None
        self.input_menu.configure(state="readonly")
        self.output_menu.configure(state="readonly")
        self.channel_menu.configure(state="readonly")
        self.effect_button.configure(text="Turn effect ON")
        self.status.set("Audio stopped")

    def _poll_status(self) -> None:
        if self.error:
            error, self.error = self.error, None
            self.stop()
            self.status.set(f"Audio error: {error}")
        elif self.stream.value and self.xruns:
            mode = "Effect on" if self.effect_enabled else "Bypass on"
            self.status.set(f"{mode} · audio underruns/overruns: {self.xruns}")
        self.root.after(200, self._poll_status)

    def close(self) -> None:
        self.stop()
        self.audio.close()
        self.model.close()
        self.root.destroy()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "model", nargs="?", type=Path,
        default=Path(__file__).resolve().parents[1] / "data" / "output.nam",
        help="A1 .nam model (default: custom_nam/data/output.nam)",
    )
    parser.add_argument("--list-devices", action="store_true")
    args = parser.parse_args()
    if args.list_devices:
        audio = PortAudio()
        try:
            for device in audio.devices():
                print(f"{device.label} | inputs={device.inputs} outputs={device.outputs}")
        finally:
            audio.close()
        return
    root = tk.Tk()
    try:
        LiveApp(root, args.model)
    except Exception:
        root.destroy()
        raise
    root.mainloop()


if __name__ == "__main__":
    main()
