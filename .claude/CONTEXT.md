# CrossPoint Reader — Durable Context

Keep this file focused on repo-specific gotchas that are worth reusing in future sessions.

## Simulator

- Simulator patches belong in the adjacent `crosspoint-simulator` repo.
- The valid local simulator env in this repo is `simulator`, and `pio run -e simulator` currently builds cleanly.
- The simulator `PNGdec` stub in `crosspoint-simulator/src/PNGdec.h` needs to mirror the real API shape used by app code, including `hasAlpha()` and `getTransparentColor()`, even though decode still fails intentionally.
- Known simulator limits:
  - No image rendering: `platformio.ini` ignores `hal`, `PNGdec`, and `JPEGDEC`, so image decoders are intentionally absent.
  - JPEGDEC stub always fails; `JPEGDEC fallback: open failed (err=-1)` is expected in simulator.
  - `esp_deep_sleep_start()` is a no-op in simulator.
  - `HalStorage` uses POSIX file access under `./fs_` and allows multiple readers, unlike real hardware.

## Real Hardware / Storage

- SdFat on hardware allows only one open reader per file path at a time. If a fallback needs to reopen the same file, close the first handle before reopening.

## Rendering / Reader Pipeline

- `lib/Epub/Epub/Page.cpp`: images must render only in `GfxRenderer::BW`; grayscale passes are text anti-aliasing passes only.
- Kindle EPUBs may contain paired high-res and old-Kindle fallback images. `ChapterHtmlSlimParser` should skip `<img>` nodes with `data-AmznRemoved-M8` to avoid duplicate stacked images.
- After image/layout pipeline changes that affect cached EPUB output, clear the affected `.crosspoint/epub_<hash>/` cache if behavior looks stale.

## Settings System — SettingInfo::Enum

`SettingInfo::Enum` stores the raw `uint8_t` field value as the **index** into the `enumValues` vector. Cycling: `(value + 1) % enumValues.size()`. Display: `enumValues[value]` with `value < size ? value : 0` bounds guard.

**Critical invariant**: the order of entries in `enumValues` must match the corresponding `enum` integer values exactly. Removing a middle entry shifts all following entries' indices down by 1, corrupting stored settings for any enum value that was after the removed entry. Example: removing `CHAREINK` (index 2) from the font picker made value 2 display as "Noto Bengali" and made `NOTOBENGALI` (value 3) unreachable and display as "LexendDeca".

**Rule**: never remove an enum value from an `SettingInfo::Enum` values list mid-sequence, even for compile-time flag exclusions. Keep the entry so indices stay in sync with enum integer values.

## Misc Repo Gotchas

- POSIX TZ signs are inverted from ISO 8601 in `TimeStore::applyTimezone()`: `"UTC-1"` means UTC+1.
- `LyraTheme::drawHeader()` does not call `BaseTheme::drawHeader()`, so header changes in the base theme must be duplicated in Lyra if needed.
