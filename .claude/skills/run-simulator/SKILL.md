# Run Simulator Skill

Builds, launches, and drives the CrossInk/CrossPoint desktop simulator so a change can be seen and verified visually, instead of only compiled.

**macOS only.** The simulator is a real SDL2 window (`.pio/build/simulator/program`), not a headless framebuffer. Window control, input, and screenshots go through `osascript`/System Events and `screencapture`. On first use, macOS will prompt to grant Accessibility and Screen Recording permissions to the terminal app running Claude Code — approve both, then retry.

All commands go through `.claude/skills/run-simulator/driver.sh <command>`.

---

## Commands

| Command | Effect |
|---|---|
| `build` | `pio run -e simulator` |
| `seed` | Copies `test/epubs/test_bangla.epub` and a hand-written Bangla `.txt` sample into `fs_/books/` |
| `launch` | Kills any running instance, starts the binary in the background, raises its window |
| `bounds` | Prints `x,y,w,h` of the simulator window |
| `shot <path.png>` | Screenshots just the simulator window to `<path.png>` |
| `key <name>` | Sends one keypress. `name` = `back \| confirm \| left \| right \| up \| down \| power` |
| `stop` | Kills the simulator process |

---

## Typical flow

```bash
.claude/skills/run-simulator/driver.sh build
.claude/skills/run-simulator/driver.sh seed      # only needed for Bangla text/EPUB testing
.claude/skills/run-simulator/driver.sh launch
.claude/skills/run-simulator/driver.sh shot /tmp/before.png
```

View the screenshot, then navigate and re-check:

```bash
.claude/skills/run-simulator/driver.sh key down
.claude/skills/run-simulator/driver.sh key confirm
.claude/skills/run-simulator/driver.sh shot /tmp/after.png
```

When done:

```bash
.claude/skills/run-simulator/driver.sh stop
```

---

## Notes

- `launch` will fail with `error: simulator window not found` if the binary crashed on startup or the window never got focus in time — check `/tmp/crossink_sim.log` first.
- `seed`'s EPUB copy step is a no-op with a warning if `test/epubs/test_bangla.epub` doesn't exist yet; generate it with `scripts/generate_bangla_test_epub.py` first.
- Re-run `build` after any source change — `launch` always runs whatever binary is currently at `.pio/build/simulator/program`, it does not rebuild.
- `key` names map to `MappedInputManager::Button::*` — see `driver.sh`'s `cmd_key` for the exact keycode table.
