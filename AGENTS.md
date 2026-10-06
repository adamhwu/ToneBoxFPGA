# Working in ToneBoxFPGA

- The active Neural Amp Modeler A1 work is in `custom_nam/`: `engine/` contains the PyTorch reference, C++ inference engine, and native loader; `app/` contains the macOS GUI and PortAudio interface; `tests/` contains verification and synthetic fixtures; `data/` holds ignored local captures and recordings.
- For a fresh Linux or Codex Cloud checkout, run `bash scripts/codex-cloud-setup.sh` from the repository root. It creates `custom_nam/.venv`, builds the C++ engine locally, and runs the headless fixture check. Do not install Python packages globally.
- The fast headless check is `custom_nam/.venv/bin/python -m custom_nam.tests.verify_headless`. It needs Python 3 and a C++17 compiler, but no third-party Python packages or audio device. It uses a synthetic capture in `custom_nam/tests/fixtures/`, so it does not need the user's real model or recordings.
- `custom_nam.tests.verify_native` compares against the independent PyTorch implementation, but needs PyTorch and NumPy in the local environment. The live GUI does not use either package.
- Codex Cloud cannot exercise the Focusrite, CoreAudio, or the Tk GUI. Validate model and DSP changes with the headless check there; validate physical audio and latency on a local machine with the interface connected.
- The real `custom_nam/data/output.nam` and `.wav` recordings are intentionally ignored because the GitHub repository is public. Do not add them to commits without the user's explicit direction.
- Keep generated libraries, virtual environments, and rendered audio out of commits. Preserve existing recordings and unrelated changes in `cAudio/`.
