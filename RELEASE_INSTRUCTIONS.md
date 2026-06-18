AURA Release instructions

- Open the `AURA` folder and double-click `AURA.exe` to launch.
- Folders included:
  - `assets/` (images, sounds, videos, fonts, animations)
  - `data/` (accounts.txt, scores.txt, reports.txt, questions.txt)
  - `config/` (aura.cfg and aura_config.cfg)
  - `cache/` (runtime temporary files)
  - `logs/` (application logs)

To build and package on Windows with MSYS2 or a GCC toolchain:

1. Open MSYS2 MinGW64 shell or a terminal with `gcc` and `make` available.
2. Run `make` to compile the binary (outputs to `bin/AURA.exe`).
3. Run `make release` to assemble the `AURA/` distribution folder.

Optional: place `rsc/aura.ico` to set a custom icon before packaging. If you have `windres`, you can add resource compilation to the Makefile manually.
