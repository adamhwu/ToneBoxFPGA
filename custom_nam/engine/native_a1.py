"""Build and load the local C++ A1 processor for the Tkinter audio GUI."""

from __future__ import annotations

import ctypes as C
import os
from pathlib import Path
import shutil
import subprocess
import sys


HERE = Path(__file__).resolve().parent
SOURCE = HERE / "a1_native.cpp"
JSON_HEADER = HERE / "vendor" / "json.hpp"
LIBRARY = HERE.parent / "build" / (
    "liba1_native.dylib" if sys.platform == "darwin" else "liba1_native.so"
)


def build_library() -> Path:
    sources = (SOURCE, JSON_HEADER)
    if LIBRARY.exists() and LIBRARY.stat().st_mtime_ns >= max(
        source.stat().st_mtime_ns for source in sources
    ):
        return LIBRARY
    compiler = shutil.which("clang++") or shutil.which("g++")
    if compiler is None:
        raise RuntimeError("A C++17 compiler is needed to build the local A1 library")
    LIBRARY.parent.mkdir(exist_ok=True)
    temporary = LIBRARY.with_name(LIBRARY.name + ".tmp")
    command = [
        compiler, "-std=c++17", "-O3", "-fPIC",
        "-dynamiclib" if sys.platform == "darwin" else "-shared",
        str(SOURCE), "-o", str(temporary),
    ]
    try:
        subprocess.run(command, check=True, capture_output=True, text=True)
        os.replace(temporary, LIBRARY)
    except subprocess.CalledProcessError as error:
        temporary.unlink(missing_ok=True)
        raise RuntimeError(f"C++ build failed:\n{error.stderr}") from error
    return LIBRARY


class NativeA1:
    """Own one prewarmed A1 model; all audio processing happens in C++."""

    def __init__(self, model_path: str | Path):
        self.lib = C.CDLL(str(build_library()))
        self.lib.a1_last_error.restype = C.c_char_p
        self.lib.a1_create.argtypes = [C.c_char_p]
        self.lib.a1_create.restype = C.c_void_p
        self.lib.a1_destroy.argtypes = [C.c_void_p]
        self.lib.a1_sample_rate.argtypes = [C.c_void_p]
        self.lib.a1_sample_rate.restype = C.c_int
        self.lib.a1_receptive_field.argtypes = [C.c_void_p]
        self.lib.a1_receptive_field.restype = C.c_int
        self.lib.a1_reset.argtypes = [C.c_void_p]
        self.lib.a1_reset.restype = C.c_int
        self.lib.a1_process_audio.argtypes = [
            C.c_void_p, C.c_void_p, C.c_void_p,
            C.c_int, C.c_int, C.c_int, C.c_int, C.c_int,
        ]
        self.lib.a1_process_audio.restype = C.c_int
        self.lib.a1_process_mono.argtypes = [
            C.c_void_p, C.POINTER(C.c_float), C.POINTER(C.c_float), C.c_int,
        ]
        self.lib.a1_process_mono.restype = C.c_int
        handle = self.lib.a1_create(os.fsencode(model_path))
        if not handle:
            raise RuntimeError(self.error_message())
        self.handle = C.c_void_p(handle)
        self.sample_rate = self.lib.a1_sample_rate(self.handle)
        self.receptive_field = self.lib.a1_receptive_field(self.handle)

    def error_message(self) -> str:
        return self.lib.a1_last_error().decode(errors="replace")

    def reset(self) -> None:
        if self.lib.a1_reset(self.handle):
            raise RuntimeError(self.error_message())

    def process_audio(
        self, input_ptr: int | None, output_ptr: int, frames: int,
        input_channels: int, selected_channel: int, output_channels: int,
        enabled: bool,
    ) -> int:
        return self.lib.a1_process_audio(
            self.handle, input_ptr, output_ptr, frames, input_channels,
            selected_channel, output_channels, int(enabled),
        )

    def close(self) -> None:
        handle = getattr(self, "handle", None)
        if handle:
            self.lib.a1_destroy(handle)
            self.handle = None

    def __del__(self) -> None:
        self.close()
