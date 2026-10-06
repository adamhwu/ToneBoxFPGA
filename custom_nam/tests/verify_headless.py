"""Check C++ A1 inference without audio hardware or third-party Python packages."""

from __future__ import annotations

import ctypes as C
import json
from pathlib import Path

from custom_nam.engine.native_a1 import NativeA1


HERE = Path(__file__).resolve().parent


def main() -> None:
    fixture = json.loads((HERE / "fixtures" / "a1_stream.json").read_text())
    assert sum(fixture["chunks"]) == len(fixture["input"])
    assert len(fixture["expected"]) == len(fixture["input"])

    model = NativeA1(HERE / fixture["model"])
    try:
        assert model.sample_rate == fixture["sample_rate"]
        output = []
        offset = 0
        for count in fixture["chunks"]:
            source = (C.c_float * count)(*fixture["input"][offset:offset + count])
            result = (C.c_float * count)()
            status = model.lib.a1_process_mono(model.handle, source, result, count)
            if status:
                raise RuntimeError(model.error_message())
            output.extend(result)
            offset += count

        error = max(abs(actual - expected) for actual, expected in zip(
            output, fixture["expected"]
        ))
        if error > fixture["absolute_tolerance"]:
            raise AssertionError(f"C++ A1 output differs from reference by {error:g}")

        model.reset()
        source = (C.c_float * 2)(0.75, 0.125)
        outgoing = (C.c_float * 2)()
        status = model.process_audio(
            C.addressof(source), C.addressof(outgoing), 1, 2, 1, 2, False
        )
        if status:
            raise RuntimeError(model.error_message())
        if abs(outgoing[0] - 0.125) > 1e-7 or abs(outgoing[1] - 0.125) > 1e-7:
            raise AssertionError("live bypass or channel routing failed")

        print(f"C++ A1 headless check passed: {len(output)} samples, "
              f"maximum reference error {error:.2g}")
    finally:
        model.close()


if __name__ == "__main__":
    main()
