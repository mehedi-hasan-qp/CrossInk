# Bangla Flash Overflow Fix — OMIT_CHAREIN_FONT

## Status
**Implemented** — 2026-05-07.

## Background
Fixes for Danda (U+0964), 3-consonant conjuncts, and 5-cp lookahead in BanglaShaper are **already done** (see BANGLA_FONT.md). The resulting build overflows flash:
- Limit: 6,553,600 bytes
- Actual: 7,573,115 bytes (+1,019,515 bytes over)

Root cause: 4,183 new 3-consonant cluster PUA bitmaps added ~1.57 MB to flash.

## Decision
Remove CharEInK font family from `[env:bangla]` using an `OMIT_CHAREIN_FONT` compile flag. Saves ~1.27 MB. Bitter and LexendDeca remain. CHAREINK=2 stays in the enum so stored settings aren't corrupted.

## Files to Modify

### 1. `lib/EpdFont/builtinFonts/all.h` (lines 63–100)
Wrap all `charein_*` includes with outer `#ifndef OMIT_CHAREIN_FONT` guard. Also add to the guard-comment block at top (lines 10–15):
```
//   OMIT_CHAREIN_FONT  - excludes all CharEInK fonts; used by env:bangla to fit Bangla glyphs in flash
```

### 2. `src/main.cpp`
- **Lines 47–97**: Wrap all `EpdFont charein*` + `EpdFontFamily charein*` declarations with `#ifndef OMIT_CHAREIN_FONT` / `#endif`. Preserve inner per-size guards inside.
- **Lines 368–384**: Wrap all `renderer.insertFont(CHAREINK_*` calls with `#ifndef OMIT_CHAREIN_FONT` / `#endif`. Preserve inner per-size guards inside.

### 3. `src/CrossPointSettings.cpp`
- **`getReaderLineCompression()` lines 334–343**: Wrap `case CHAREINK:` block with `#ifndef OMIT_CHAREIN_FONT` / `#endif`
- **`getReaderFontId()` lines 471–498**: Wrap `case CHAREINK:` block with `#ifndef OMIT_CHAREIN_FONT` / `#endif`
- When omitted, fontFamily==CHAREINK falls through to `default:` (LEXENDDECA).

### 4. `src/SettingsList.h` (line 47)
~~Wrap `StrId::STR_CHAREINK` in the font picker~~ — **DO NOT DO THIS.**
The settings system stores fontFamily as the **vector index** into enumValues. Removing CHAREINK (index 2) shifts NOTOBENGALI from index 3 to index 2, causing fontFamily=2 to be interpreted as CHAREINK by getReaderFontId(), which then falls back to LEXENDDECA — breaking all Bangla rendering.
STR_CHAREINK must stay in the list. Selecting CharEInK in the bangla build falls back to LexendDeca (acceptable).

### 5. `platformio.ini` (`[env:bangla]`, line ~160)
Add `-DOMIT_CHAREIN_FONT` to `build_flags`.

### 6. `src/CrossPointSettings.h` — DO NOT CHANGE
`CHAREINK = 2` must stay in `FONT_FAMILY` enum to avoid corrupting stored settings.

## Verification
```bash
# Must fit in flash:
pio run -e bangla 2>&1 | grep -E "RAM:|Flash:"
# Expect: Flash < 6,553,600 bytes

# Default build must still compile (CharEInK present):
pio run -e simulator

# Bangla rendering regression:
# Open simulator → Noto Bengali font → verify ।  ন্ত্র  ফ্লা  ক্ষ all render
```
