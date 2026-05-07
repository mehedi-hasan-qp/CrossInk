# Bangla Font Support — Master Tracking Document

## Goal
Add Bangla (Bengali, U+0980–U+09FF) script support so users can read Bangla EPUB/TXT content.
UI stays in English. Font: Noto Sans Bengali (OFL). Hardware: ESP32-C3, no PSRAM, ~380 KB RAM.

## Key Constraint
Bangla needs **text shaping** — conjuncts (e.g. ক + ্ + ষ → ক্ষ) are 3-codepoint sequences that map
to a single glyph, and the pre-base matra ি must render before its base consonant even though it is
typed after it. The current pipeline is glyph-per-codepoint only. Solution: pre-shape at **build time**
using `uharfbuzz` in `fontconvert.py`, store shaped glyphs as PUA (U+E000+) codepoints, and add a
tiny runtime `BanglaShaper` class that maps Bengali sequences → PUA before they reach the renderer.

---

## Implementation Phases

### Phase A — Standard Font Pipeline  ← START HERE
Register Noto Sans Bengali through the existing custom-font workflow first (using standard
fontconvert.py with no shaping). This validates the pipeline and lets us see basic Bangla
(vowels, consonants, digits) before adding the shaping layer.

### Phase B — Build-Time Shaping
Extend `fontconvert.py` with `--bangla` flag + `uharfbuzz` to pre-shape conjuncts/matras into
PUA bitmaps. Regenerate font headers with shaped glyphs. Also generates `bangla_clusters.h`
(sequence → PUA lookup table for the runtime shaper).

### Phase C — Runtime BanglaShaper
New `lib/BanglaShaper/` library. Converts raw Bengali UTF-8 → PUA-substituted UTF-8 before
the text enters the render pipeline. Integrates in `ParsedText::addWord()` and `Txt.cpp`.

---

## Prerequisites ✅ DONE
- [x] `uharfbuzz>=0.36.0` added to `lib/EpdFont/scripts/requirements.txt`
- [x] venv at `.font-tools-venv/` (project root) — moved out of `lib/` to avoid PlatformIO scanning
- [x] `NotoSansBengali-Regular.ttf` (195 KB) at `lib/EpdFont/builtinFonts/source/NotoSansBengali/`
- [ ] Add `.venv` to `.gitignore` (do when committing)

---

## Phase A Checklist — Standard Font Pipeline

| # | Step | File(s) | Status |
|---|------|---------|--------|
| A1 | Update `convert-builtin-fonts.sh` to generate Noto Bengali headers | `lib/EpdFont/scripts/convert-builtin-fonts.sh` | ✅ |
| A2 | Run the script to generate `.h` files | `lib/EpdFont/builtinFonts/notobengali_*.h` | ✅ 6 headers, 32–60 KB each |
| A3 | Add includes in `all.h` | `lib/EpdFont/builtinFonts/all.h` | ✅ |
| A4 | Add font objects + `insertFont()` in `main.cpp` | `src/main.cpp` | ✅ |
| A5 | Update `build-font-ids.sh` and regenerate `fontIds.h` | `lib/EpdFont/scripts/build-font-ids.sh`, `src/fontIds.h` | ✅ |
| A6 | Add `NOTOBENGALI` to `FONT_FAMILY` enum | `src/CrossPointSettings.h` line 105 | ✅ |
| A7 | Add font ID mapping in `CrossPointSettings.cpp` | `src/CrossPointSettings.cpp` | ✅ |
| A8 | Add `STR_NOTO_BENGALI` to `SettingsList.h` | `src/SettingsList.h` | ✅ |
| A9 | Add translation string to `english.yaml` | `lib/I18n/translations/english.yaml` | ✅ |
| A10 | Regenerate i18n | `lib/I18n/I18nKeys.h` updated | ✅ |
| A11 | Add `OMIT_BANGLA_FONT` guard to `platformio.ini` tiny env | `platformio.ini` | ✅ |
| A12 | Verify build | `pio run -e simulator` → **SUCCESS** (20.52s) | ✅ |

---

## Phase B Checklist — Build-Time Shaping

| # | Step | File(s) | Status |
|---|------|---------|--------|
| B1 | Add `--bangla` flag to `fontconvert.py` arg parser | `lib/EpdFont/scripts/fontconvert.py` | ✅ |
| B2 | Add Bangla interval list + PUA assignment logic | `fontconvert.py` | ✅ |
| B3 | Add `uharfbuzz` shaping loop for conjuncts + pre-base matra | `fontconvert.py` | ✅ 199 conjuncts |
| B4 | Emit `bangla_clusters.h` companion file alongside font header | `fontconvert.py` | ✅ |
| B5 | Add Bengali script group to `SCRIPT_GROUP_RANGES` in compressor | `fontconvert.py` | ✅ |
| B6 | Update `convert-builtin-fonts.sh` to pass `--bangla` flag | `convert-builtin-fonts.sh` | ✅ |
| B7 | Regenerate font headers with shaped glyphs | `lib/EpdFont/builtinFonts/notobengali_*.h` | ✅ 6 headers, PUA U+E000–U+E0C6 |
| B8 | Regenerate `fontIds.h` (hashes change with new glyphs) | `src/fontIds.h` | ✅ |
| B9 | Add 3-consonant shaping loop + 5-cp key format to `fontconvert.py` | `lib/EpdFont/scripts/fontconvert.py` | ✅ 4183 new 3-cp entries, PUA U+E000–U+F35E |
| B10 | Add U+0964–U+0965 (Danda/Double Danda) to `convert-builtin-fonts.sh` | `lib/EpdFont/scripts/convert-builtin-fonts.sh` | ✅ |
| B11 | Regenerate headers with Danda + 3-consonant glyphs | `lib/EpdFont/builtinFonts/notobengali_*.h` | ✅ 4967 conjuncts total |
| B12 | Regenerate `fontIds.h` | `src/fontIds.h` | ✅ |

---

## Phase C Checklist — Runtime BanglaShaper

| # | Step | File(s) | Status |
|---|------|---------|--------|
| C1 | Create `lib/BanglaShaper/BanglaShaper.h` | new file | ✅ |
| C2 | Create `lib/BanglaShaper/BanglaShaper.cpp` | new file | ✅ binary search on BANGLA_CLUSTERS |
| C3 | Integrate shaper in `ParsedText::addWord()` | `lib/Epub/Epub/ParsedText.cpp` | ✅ |
| C4 | Integrate shaper in plain-text pipeline | `src/activities/reader/TxtReaderActivity.cpp` | ✅ in parseAndWrapLines |
| C5 | Final end-to-end test with a Bangla EPUB in simulator | — | ✅ ya-phala, conjuncts, digits all correct |
| C6 | Add `lookupCluster5()` + 5-cp lookahead in `BanglaShaper::shape()` | `lib/BanglaShaper/BanglaShaper.cpp`, `.h` | ✅ |

---

---

## Phase D — Flash Optimization (env:bangla)

After adding 4,183 three-consonant cluster glyphs (B9), the bangla build overflowed flash by ~1 MB.

**Solution**: Added `OMIT_CHAREIN_FONT` and `OMIT_TINY_FONT` compile flags to `[env:bangla]` in `platformio.ini`. Together they save ~1.3 MB. Flash now at **95.9%** (6,284,241 / 6,553,600 bytes).

### Files changed
| File | Change |
|------|--------|
| `lib/EpdFont/builtinFonts/all.h` | Outer `#ifndef OMIT_CHAREIN_FONT` guard wrapping all charein_* includes (lines 63–101) |
| `src/main.cpp` | Outer `OMIT_CHAREIN_FONT` guard around charein EpdFont/EpdFontFamily declarations and `renderer.insertFont` calls |
| `src/CrossPointSettings.cpp` | `case CHAREINK:` in `getReaderFontId()` and `getReaderLineCompression()` wrapped in `#ifndef OMIT_CHAREIN_FONT`; falls to `default:` (LEXENDDECA) when omitted |
| `platformio.ini` | `-DOMIT_CHAREIN_FONT -DOMIT_TINY_FONT` added to `[env:bangla]` |

### Critical lesson: do NOT change SettingsList.h
The original plan included wrapping `STR_CHAREINK` in `SettingsList.h` with `#ifndef OMIT_CHAREIN_FONT`. **This breaks Bangla rendering completely.** `SettingInfo::Enum` uses the vector index as the stored `uint8_t` value. Removing CHAREINK (index 2) shifts NOTOBENGALI from index 3 to index 2 — selecting "Noto Bengali" stores value 2 (= CHAREINK), which then falls to LexendDeca in `getReaderFontId()`. **Keep `STR_CHAREINK` in the list at index 2 always.**

---

## Known Issues

### ~~BUG-1: Ya-phala composite glyph renders over base consonant~~ ✅ FIXED

### ~~BUG-2: U+0964 (।, Devanagari Danda) invisible~~ ✅ FIXED

**Root cause**: `--additional-intervals 0x0980,0x09FF` in `convert-builtin-fonts.sh` covers only the Bengali block. U+0964 (।) is in the Devanagari block (U+0900–U+097F) and was never included.

**Fix**: Added `--additional-intervals 0x0964,0x0965` to `convert-builtin-fonts.sh` (line ~245). Regenerated all 6 font headers.

### ~~BUG-3: 3-consonant conjuncts (ন্ত্র, স্ত্র, etc.) render broken~~ ✅ FIXED

**Root cause (two layers)**:
1. **Build-time** (`fontconvert.py`): Only a 2-consonant loop existed. No bitmaps were ever generated for C1+্+C2+্+C3 sequences.
2. **Runtime** (`BanglaShaper.cpp`): After emitting PUA(C1C2), the remaining ্+C3 was processed as a standalone virama followed by a loose consonant → visible ্ + floating C3.

**Fix**:
- `fontconvert.py`: Added 3-consonant loop (O(n³) over ~41 consonants). Key encoding: `(1ULL<<63) | (cp1<<42) | (cp3<<21) | cp5` (bit-63 flag distinguishes 5-cp keys from existing 3-cp keys, which are always < 2⁵⁴).
- `BanglaShaper.cpp/h`: Added `lookupCluster5()` + 5-cp lookahead in `shape()`. 5-cp path is tried first; falls back to 3-cp path if no match.

**Result**: 4,183 new 3-consonant cluster entries, total 4,967 conjuncts, PUA U+E000–U+F35E.

**Root cause**: `fontconvert.py` line 497 set `_hb_font.scale = (args.size * 64, args.size * 64)`
— treating size as pixels at 72 DPI. FreeType renders at 150 DPI, so actual pixel size is
`size * 150/72 ≈ 2.08×` larger. This made HarfBuzz advances too small relative to FreeType
bitmap widths, placing ya-phala 9–17 px inside the base consonant bitmap.

**Fix applied** (`fontconvert.py` line 497–498):
```python
_hb_px = round(args.size * 150 / 72 * 64)
_hb_font.scale = (_hb_px, _hb_px)
```

**Verified**: All 6 Noto Bengali headers regenerated. Simulator test confirmed ব্য, ক্য, ত্য,
সাহিত্য, বাক্য, সংখ্যা all render correctly (ya-phala clearly to the right of its base).

---

## File Reference Map

| File | Role | Notes |
|------|------|-------|
| `lib/EpdFont/scripts/fontconvert.py` | Font → C header converter | Extend with `--bangla` in Phase B |
| `lib/EpdFont/scripts/convert-builtin-fonts.sh` | Orchestrates font generation | Add Noto Bengali block in A1 |
| `lib/EpdFont/scripts/build-font-ids.sh` | SHA256 font ID generator | Add Bengali entries in A5 |
| `lib/EpdFont/scripts/requirements.txt` | Python deps for font tools | ✅ uharfbuzz added |
| `lib/EpdFont/scripts/.venv/` | Isolated Python env | ✅ all deps installed |
| `lib/EpdFont/builtinFonts/source/NotoSansBengali/NotoSansBengali-Regular.ttf` | Source font | ✅ downloaded |
| `lib/EpdFont/builtinFonts/notobengali_*.h` | Generated font headers | Created in A2 / regenerated in B7 |
| `lib/EpdFont/builtinFonts/all.h` | Master include header | Add Bengali includes in A3 |
| `src/main.cpp` | App init — font registration | Add font objects + insertFont in A4 |
| `src/fontIds.h` | Generated font ID constants | Regenerate in A5 |
| `src/CrossPointSettings.h` | FONT_FAMILY enum | Add NOTOBENGALI in A6 |
| `src/CrossPointSettings.cpp` | Font ID switch + line settings | Add Bengali case in A7 |
| `src/SettingsList.h` | Font picker UI enum entries | Add STR_NOTO_BENGALI in A8 |
| `lib/I18n/translations/english.yaml` | UI display strings | Add "Noto Bengali" string in A9 |
| `lib/I18n/` (generated) | Compiled i18n tables | Regenerate in A10 |
| `platformio.ini` | Build environments | Add OMIT_BANGLA_FONT to tiny in A11 |
| `lib/BanglaShaper/BanglaShaper.h` | Shaper interface | Created in C1 |
| `lib/BanglaShaper/BanglaShaper.cpp` | Shaper implementation | Created in C2 |
| `lib/BanglaShaper/bangla_clusters.h` | Generated PUA lookup table | Emitted by fontconvert.py in B4 |
| `lib/Epub/Epub/ParsedText.cpp` | EPUB word layout | ✅ BanglaShaper called in addWord() |
| `src/activities/reader/TxtReaderActivity.cpp` | Plain text reader | ✅ BanglaShaper called in parseAndWrapLines() |

---

## Architecture Notes

### Why PUA codepoints?
The existing renderer processes one codepoint → one glyph. Bangla conjuncts require N codepoints
→ 1 glyph. By pre-shaping into PUA codepoints at build time, the conjunct bitmaps live in the font
as regular glyphs. The runtime shaper only needs to map sequences → PUA, with no bitmap logic.

### PUA range used
U+E000–U+F8FF (6400 slots). Bangla needs at most ~600 (conjuncts + matra variants). No conflict
with existing fonts (current fonts use no PUA glyphs).

### Pre-base matra reordering
ি (U+09BF) is typed after its base consonant but drawn to the left. BanglaShaper emits:
  `[PUA_pre_i_of_C]` + `[C]`  instead of  `[C]` + `[U+09BF]`
The PUA glyph for "pre_i_of_C" is the ি shape; cursor is advanced normally left-to-right.

### Key encoding in bangla_clusters.h
  2-cp sequence: `key = (uint64_t)cp1 << 21 | cp2`
  3-cp sequence: `key = (uint64_t)cp1 << 42 | (uint64_t)cp2 << 21 | cp3`
  (21 bits per codepoint — sufficient for all Unicode up to U+10FFFF)

### Ligature system (NOT extended)
The existing 2-codepoint `EpdLigaturePair` system is NOT changed. BanglaShaper pre-processes the
string before it reaches `applyLigatures()`, so there is no conflict.

### venv usage
The venv lives at `.font-tools-venv/` (project root, NOT inside lib/ to avoid PlatformIO scanning it).
Always use it when running fontconvert.py for Bangla fonts:
  `cd lib/EpdFont/scripts && ../../../.font-tools-venv/bin/python3 fontconvert.py ...`

---

## Bangla Unicode Reference

| Range | Contents |
|-------|----------|
| U+0981–U+0983 | Anusvara, Anusvara, Visarga (diacritics) |
| U+0985–U+0994 | Independent vowels: অ আ ই ঈ উ ঊ ঋ ঌ এ ঐ ও ঔ |
| U+0995–U+09B9 | Consonants: ক খ গ ঘ ঙ চ ছ জ ঝ ঞ ট ঠ ড ঢ ণ ত থ দ ধ ন প ফ ব ভ ম য র ল শ ষ স হ |
| U+09BC | Nukta (modifies consonants: ড় ঢ় য়) |
| U+09BE–U+09CC | Dependent vowel signs (matras): া ি ী ু ূ ৃ ৄ এ ে ৈ ো ৌ |
| U+09CD | Hasant / Virama (্) — creates conjuncts |
| U+09CE | Khanda Ta (ৎ) |
| U+09D7 | AU Length Mark |
| U+09DC–U+09DF | Ra, Rra, Yya with nukta: ড় ঢ় য় |
| U+09E6–U+09EF | Digits: ০ ১ ২ ৩ ৪ ৫ ৬ ৭ ৮ ৯ |
| U+09F0–U+09FB | Additional signs |

---

## Verification Plan

```bash
# After Phase A:
pio run -e simulator           # must build cleanly
# Open simulator, switch font to "Noto Bengali", open a Bangla EPUB
# Expect: vowels + consonants render; conjuncts show component letters (not yet shaped)

# After Phase B+C:
pio run -e simulator           # must build cleanly
# Expect: ক্ষ renders as single shaped glyph, ি appears left of base, digits correct

# Binary size check (must stay under 6,553,600 bytes):
pio run -e default 2>&1 | grep -E "RAM:|Flash:"
```
