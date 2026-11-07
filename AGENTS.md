# Repository Guidelines

## Project Structure & Module Organization

This repository is an ESP-IDF BSP and hardware demonstration for the ESP32-C3-based FoloToy AI Passport.
It also supports the ESP32-2432S028R CYD through a separate 4 MB profile; see `docs/CYD.md` for wiring, gestures, display variants and hardware acceptance.

- `components/bsp/include/`: public BSP APIs and the hardware pin/configuration source of truth (`bsp_pins.h`).
- `components/bsp/src/`: display, button, audio, battery, and shared-I2C implementations.
- `main/`: the LVGL menu and independent `demo_*.c` hardware validation pages. New demos should implement the `enter`, `exit`, and `key` interface declared in `demo.h`.
- `sdkconfig.defaults`: reproducible target, console, LVGL, and memory defaults.
- `sdkconfig.defaults.cyd` / `partitions.cyd.csv`: ESP32 CYD defaults and 4 MB layout. `bsp_pins.h` selects `bsp_pins_cyd.h` for this board.
- `README.md`: wiring, known hardware traps, and the required on-device acceptance checklist.

Keep reusable hardware logic in `components/bsp`; keep board demonstration and UI behavior in `main`.

## Build, Test, and Development Commands

Use ESP-IDF 5.5.x:

```bash
get_idf553                    # Enter the repository's ESP-IDF 5.5.3 environment
idf.py set-target esp32c3     # Configure a fresh checkout
idf.py build                  # Compile firmware and validate dependencies
idf.py flash monitor          # Flash the connected board and open logs
idf.py fullclean              # Remove generated build state when configuration is stale
```

For CYD, activate ESP-IDF 5.5.3 and run `./tools/build-cyd.ps1` on Windows, or use the equivalent `idf.py` command in `docs/CYD.md`. Keep its `build-cyd` and `sdkconfig.cyd` separate from the original ESP32-C3 build. LVGL is pinned to 9.5.0, whose Kconfig memory pool is expressed in KiB.

Run `python -m unittest discover -s tools -p 'test_*.py' -v` for host protocol and camera regressions. The firmware-function regressions require a native C compiler (`CC=cc` or `CC="zig cc"`); a skipped compiler-dependent suite does not count as a pass. See `tools/tests/README.md` for scope. Treat a clean `idf.py build` as the minimum firmware check, then run every applicable item in the README acceptance checklist on real hardware.

## Coding Style & Naming Conventions

Write C using four-space indentation and K&R-style braces, following nearby files. Use `snake_case` for functions and locals, `BSP_*` for public hardware constants, and `s_` for file-local state. Keep BSP APIs prefixed with `bsp_`; name demo entry points `demo_<feature>_<action>`. Prefer `static` for internal symbols. Hardware-demo UI text stays English. Printer-monitor pages, including the camera page, retain the existing Chinese labels for a consistent interface; camera connection/configuration prompts may stay English. This monitor-specific language choice was explicitly confirmed by the user. Explanatory comments may be Chinese. Preserve comments documenting hardware-specific register values and memory constraints.

## Testing Guidelines

Before submitting, build from the repository root and inspect warnings. On hardware, verify menu navigation and the affected Display, Button, Audio, or Battery page. For pin, display-rotation, codec-clock, ADC, or DMA changes, explicitly record the observed hardware result in the PR. Do not increase LVGL buffers or audio allocations without checking ESP32-C3 internal RAM usage; the board has no PSRAM.

## Commit & Pull Request Guidelines

History follows Conventional Commit-style subjects such as `feat(bsp): ...`, `feat(demo): ...`, `fix(bsp): ...`, and `docs: ...`. Keep commits focused by subsystem. Pull requests should explain the hardware/revision tested, summarize behavior changes, list build and on-device results, and include photos or screenshots for display changes. Link related issues and call out wiring, pin-map, or compatibility impacts.
