# A1 NAM model and live monitor

The directory is organized by purpose:

| Folder | Contents |
| --- | --- |
| `engine/` | Independent PyTorch A1 implementation, C++ inference engine, native loader, and bundled JSON parser |
| `app/` | Live Focusrite GUI and PortAudio interface |
| `tests/` | Headless and PyTorch parity checks, plus synthetic fixtures |
| `data/` | Your local `.nam` capture and WAV recordings; ignored by Git |

The live app loads `data/output.nam` by default and runs the independent C++ A1 model in `engine/a1_native.cpp`. It accepts one Focusrite input channel and sends the result to the selected output device. The effect button switches between processed audio and a direct software bypass; **Stop audio** closes the stream. The C++ code performs inference, the bypass crossfade, clipping, and channel routing. Python handles only the GUI and PortAudio setup.

Run it from the repository root:

```sh
custom_nam/.venv/bin/python -m custom_nam.app.live_a1
```

The local `.venv` was created with `python3 -m venv --system-site-packages custom_nam/.venv`. Nothing was installed globally. On first launch, `engine/native_a1.py` compiles the C++ source with the machine's C++17 compiler into `custom_nam/build/`, then loads it locally. The GUI uses only Python's standard library, the local C++ library, and the machine's PortAudio library. The PyTorch implementation in `engine/a1_model.py` remains available for offline rendering and comparison, but the GUI does not import or run it. The bundled JSON parser in `engine/vendor/json.hpp` is nlohmann/json 3.12.0, licensed under MIT.

To compare the C++ output against the PyTorch reference, run `custom_nam/.venv/bin/python -m custom_nam.tests.verify_native`. To render the local recording with PyTorch, run:

```sh
custom_nam/.venv/bin/python -m custom_nam.engine.a1_model custom_nam/data/output.nam custom_nam/data/clean_1.wav custom_nam/data/dirty_1.wav
```

For a fresh headless Linux or Codex Cloud checkout, run `bash scripts/codex-cloud-setup.sh` from the repository root. It creates a project-local virtual environment, builds the C++ engine, and verifies it against a synthetic checked-in A1 capture and signal fixture. The headless check uses only Python's standard library and a C++17 compiler. Live Focusrite and GUI testing still requires a local computer with audio hardware. The real `data/output.nam` and `.wav` recordings are kept local because this GitHub repository is public.

In the app, select the Focusrite input and output, choose the guitar's input channel, and press **Turn effect ON**. Connect headphones or monitors to the selected output. If the Focusrite is connected after launch, press **Refresh devices**. Disable hardware direct monitoring on the Focusrite if you want to hear only the processed signal. The stream runs at the model's 48 kHz sample rate with 128 sample blocks and requests the device's minimum viable buffering. The status line reports audio underruns and overruns if the computer cannot keep up.

To use another compatible A1 capture, pass its path as the final argument. To see the devices detected by PortAudio, run:

```sh
custom_nam/.venv/bin/python -m custom_nam.app.live_a1 --list-devices
```
