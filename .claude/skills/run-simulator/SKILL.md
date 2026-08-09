---
name: run-simulator
description: Build, launch, and drive the CrossInk/CrossPoint desktop simulator (PlatformIO env "simulator") on macOS — a real SDL2 window standing in for the ESP32-C3 e-ink device. Use when asked to run the simulator, test a change on-device without hardware, or screenshot/verify reader UI (font rendering, Bangla shaping, settings, file browser).
---

Builds, launches, and drives the CrossInk/CrossPoint desktop simulator so a change can be seen and verified visually, instead of only compiled.

**macOS only.** The simulator is a real SDL2 window (`.pio/build/simulator/program`), not a headless framebuffer. Window control, input, and screenshots go through `osascript`/System Events and `screencapture`. On first use, macOS will prompt to grant Accessibility (and possibly Screen Recording) permissions to the terminal app running Claude Code — approve both, then retry.

All commands go through `.claude/skills/run-simulator/driver.sh <command>`. All paths are relative to the repo root.

## Prerequisites

Already present if you can build the firmware at all:

```bash
which pio          # PlatformIO Core (brew install platformio, or pipx)
brew list sdl2      # SDL2 dev libs — platformio.ini links -lSDL2 from /opt/homebrew
```

## Commands

| Command | Effect |
|---|---|
| `build` | `pio run -e simulator` → `.pio/build/simulator/program` |
| `seed` | Copies `test/epubs/test_bangla.epub` and a hand-written Bangla `.txt` sample into `fs_/books/` (shows up in the on-device file browser at `/books/`) |
| `launch` | Kills any running instance, starts the binary in the background, raises its window, waits for it to actually become frontmost before returning |
| `bounds` | Prints `x,y,w,h` of the simulator window |
| `shot <path.png>` | Waits for the simulator to be frontmost, then screenshots just its window region |
| `key <name>` | Sends one keypress mapped to a device button (table below); waits for frontmost first |
| `stop` | Kills the simulator process |

Key mapping (from the simulator library's `HalGPIO.cpp`, not this repo):

| `key` name | physical key | device button |
|---|---|---|
| `back` | Escape | BTN_BACK |
| `confirm` | Return | BTN_CONFIRM |
| `left` / `right` | ← / → | BTN_LEFT / BTN_RIGHT |
| `up` / `down` | ↑ / ↓ | BTN_UP / BTN_DOWN |
| `power` | P | BTN_POWER |

There is no CLI flag to auto-open a file — navigate the on-device file browser with `key` presses (Home → `down` to Browse Files → `confirm` → `confirm` into `books/` → `confirm` on the target file).

## Typical flow

```bash
D=.claude/skills/run-simulator/driver.sh
$D build
$D seed                     # only needed for Bangla text/EPUB testing
$D launch
$D shot /tmp/before.png     # Read the PNG to see it
```

Navigate and re-check:

```bash
$D key down
$D key confirm
$D shot /tmp/after.png
```

When done:

```bash
$D stop
```

## Run (human path)

```bash
pio run -e simulator -t run_simulator   # build + launch, window stays open; close it or Ctrl-C to stop
```

## Test

No automated test suite drives the simulator; "tested" means running it and exercising the changed feature (per the simulator library's own `CLAUDE.md`).

---

## Gotchas

- **`AXRaise` can trigger a macOS Space-switch animation that pollutes a screenshot taken too soon after.** If the simulator window lives on a different virtual desktop (Space) than the one currently active, raising it via Accessibility can kick off a Space-switch animation. A `screencapture` fired mid-transition captures Mission Control's view of whatever *other* desktop/app was showing — not the simulator — which can expose unrelated windows and their content. `driver.sh` guards every `shot`/`key` with `wait_frontmost`, which polls System Events' frontmost-process name and adds a settle delay before proceeding, aborting loudly instead of capturing blind if the simulator never becomes frontmost. Never screenshot the window by raw coordinates without this check — always go through `driver.sh shot`, and if it errors, bring the simulator window to your active Space manually rather than retrying blindly.
- **`fs_/` (the simulated SD card) is gitignored** — it doesn't exist on a fresh clone. Run `driver.sh seed` (or manually `mkdir -p fs_/books` and copy a book in) before the first `launch`, otherwise the file browser is empty.
- **State persists across runs.** `fs_/.crosspoint/` caches parsed EPUB/TXT output and reading progress. After changing storage/cache-layout code, `rm -rf ./fs_/.crosspoint/` before re-testing or a stale cache can mask your fix — a `[ERR] [SCT] Deserialization failed: Parameters do not match` in the log is the cache correctly detecting this and self-healing, not a bug.
- **The window's process name is `program`**, not "simulator" or the app's display name — that's what `osascript`/`pgrep` need to match (`ps aux | grep program` to confirm if `driver.sh` can't find it).
- **Must run from the repo root.** The binary resolves `./fs_/...` relative to its current working directory, not its own location; `driver.sh` `cd`s to the repo root itself, but a manual `./.pio/build/simulator/program` invoked from elsewhere will create/read the wrong `fs_/`.
- **`launch` always runs whatever binary is currently at `.pio/build/simulator/program`** — it does not rebuild. Re-run `build` after any source change.

## Troubleshooting

- **`error: simulator window not found` from `launch`**: the binary crashed on startup, or the window never got Accessibility-granted focus in time. Check `/tmp/crossink_sim.log` first.
- **`error: "program" never became frontmost` from `shot`/`key`**: a Space-switch animation got stuck, or Accessibility permission was revoked mid-session. Bring the simulator window to your active desktop manually, then retry.
- **`seed`'s EPUB copy is a no-op with a warning**: `test/epubs/test_bangla.epub` doesn't exist yet — generate it first with `python3 scripts/generate_bangla_test_epub.py`.
