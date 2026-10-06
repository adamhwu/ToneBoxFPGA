# Working in ToneBoxFPGA

- The active Neural Amp Modeler A1 work is in `custom_nam/`. The C++ inference engine is `a1_native.cpp`; `native_a1.py` builds and loads its local shared library; `live_a1.py` is the macOS GUI and PortAudio interface.
- For a fresh Linux or Codex Cloud checkout, run `bash scripts/codex-cloud-setup.sh` from the repository root. It creates `custom_nam/.venv`, builds the C++ engine locally, and runs the headless fixture check. Do not install Python packages globally.
- The fast headless check is `custom_nam/.venv/bin/python custom_nam/verify_headless.py`. It needs Python 3 and a C++17 compiler, but no third-party Python packages or audio device. It uses a synthetic capture in `custom_nam/fixtures/`, so it does not need the user's real model or recordings.
- `custom_nam/verify_native.py` compares against the independent PyTorch implementation, but needs PyTorch and NumPy in the local environment. The live GUI does not use either package.
- Codex Cloud cannot exercise the Focusrite, CoreAudio, or the Tk GUI. Validate model and DSP changes with the headless check there; validate physical audio and latency on a local machine with the interface connected.
- The real `custom_nam/output.nam` and `.wav` recordings are intentionally ignored because the GitHub repository is public. Do not add them to commits without the user's explicit direction.
- Keep generated libraries, virtual environments, and rendered audio out of commits. Preserve existing recordings and unrelated changes in `cAudio/`.
