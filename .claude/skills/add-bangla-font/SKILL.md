# Add Bangla Font Skill

Adds support for a new Bangla (Bengali script) font alongside the existing Noto Sans Bengali font. Works for both CrossInk and CrossPoint forks — the file structure is identical.

## What this skill does NOT do

- It does NOT replace the existing Noto Sans Bengali font.
- It does NOT modify `BanglaShaper` or `bangla_clusters.h`. The PUA cluster table is generated deterministically by `fontconvert.py` based on the Bengali Unicode block; a second font processed with `--bangla` will produce identical PUA assignments, so the existing `bangla_clusters.h` remains authoritative for all Bangla fonts.
- It does NOT modify `getReaderLineCompression()`. New Bangla fonts fall to the `default:` case (same compression ratios as LexendDeca), which is correct for Bengali script.

---

## Step 0 — Gather inputs from the user

Ask the user for:
1. **TTF file path** — full path to the `.ttf` source file (e.g. `/Users/.../MuktiBangla-Regular.ttf`)
2. **Display name** — how it appears in the font picker UI (e.g. `"Mukti Bangla"`)

Derive everything else from the display name:
- **file prefix** — lowercase, underscores, no spaces: `mukti_bangla`
- **enum name** — SCREAMING_SNAKE_CASE: `MUKTIBANGLA`
- **CamelCase name** — for C++ variables: `muktiBangla`
- **I18n key** — `STR_MUKTI_BANGLA`
- **Source directory** — `lib/EpdFont/builtinFonts/source/MuktiBangla/`

Confirm all derived names with the user before proceeding.

---

## Step 1 — Prerequisite check

Verify the venv and Python toolchain exist:

```bash
ls .font-tools-venv/bin/python3
.font-tools-venv/bin/python3 -c "import uharfbuzz; print('uharfbuzz ok')"
```

If either fails, stop and tell the user to create the venv:
```bash
python3 -m venv .font-tools-venv
.font-tools-venv/bin/pip install -r lib/EpdFont/scripts/requirements.txt
```

---

## Step 2 — Copy the TTF

```bash
mkdir -p lib/EpdFont/builtinFonts/source/<SourceDir>/
cp <user_ttf_path> lib/EpdFont/builtinFonts/source/<SourceDir>/<SourceDir>-Regular.ttf
```

---

## Step 3 — Add font generation to `convert-builtin-fonts.sh`

Read `lib/EpdFont/scripts/convert-builtin-fonts.sh`. Find the closing `echo "Noto Bengali fonts complete."` line and append immediately after it:

```bash

# <DisplayName> — Regular only; sizes 10–20.
# Uses --bangla for HarfBuzz pre-shaping of conjuncts into PUA glyphs.
# No --bangla-clusters-out: PUA assignments are deterministic; existing bangla_clusters.h is authoritative.
<ENUM_UC>_FONT="../builtinFonts/source/<SourceDir>/<SourceDir>-Regular.ttf"
<ENUM_UC>_SIZES=(10 12 14 16 18 20)
<ENUM_UC>_RENDER_ARGS=(--2bit --compress --darken-aa)

echo "Generating <DisplayName> fonts..."
for size in ${<ENUM_UC>_SIZES[@]}; do
  font_name="<prefix>_${size}_regular"
  output_path="../builtinFonts/${font_name}.h"
  ../../../.font-tools-venv/bin/python3 fontconvert.py "$font_name" "$size" "$<ENUM_UC>_FONT" \
    --additional-intervals 0x0980,0x09FF \
    --additional-intervals 0x0964,0x0965 \
    --bangla \
    "${<ENUM_UC>_RENDER_ARGS[@]}" > "$output_path"
  echo "Generated $output_path"
done
echo "<DisplayName> fonts complete."
echo ""
```

Replace `<ENUM_UC>`, `<SourceDir>`, `<prefix>`, `<DisplayName>` with the actual values.

---

## Step 4 — Run font generation

```bash
cd lib/EpdFont/scripts && bash convert-builtin-fonts.sh
```

This generates `lib/EpdFont/builtinFonts/<prefix>_10_regular.h` through `<prefix>_20_regular.h` (6 files). Confirm all 6 files exist before continuing.

---

## Step 5 — Update `lib/EpdFont/builtinFonts/all.h`

Read the file. Find the closing `#endif` of the existing `OMIT_BANGLA_FONT` block (currently ends around line 160) and insert immediately after it:

```c
#ifndef OMIT_BANGLA_FONT
#include <builtinFonts/<prefix>_10_regular.h>
#include <builtinFonts/<prefix>_12_regular.h>
#include <builtinFonts/<prefix>_14_regular.h>
#include <builtinFonts/<prefix>_16_regular.h>
#ifndef OMIT_XLARGE_FONT
#include <builtinFonts/<prefix>_18_regular.h>
#endif
#ifndef OMIT_HUGE_FONT
#include <builtinFonts/<prefix>_20_regular.h>
#endif
#endif
```

Note: reuses `OMIT_BANGLA_FONT` — both Bangla fonts are excluded together in size-constrained builds.

---

## Step 6 — Update `src/main.cpp`

### 6a — Font declarations

Read `src/main.cpp`. Find the closing `#endif  // OMIT_BANGLA_FONT` of the notobengali declarations block (currently ends around line 210). Insert immediately after it:

```cpp
#ifndef OMIT_BANGLA_FONT
EpdFont <camel>10RegularFont(&<prefix>_10_regular);
EpdFontFamily <camel>10FontFamily(&<camel>10RegularFont, nullptr, nullptr, nullptr);
EpdFont <camel>12RegularFont(&<prefix>_12_regular);
EpdFontFamily <camel>12FontFamily(&<camel>12RegularFont, nullptr, nullptr, nullptr);
EpdFont <camel>14RegularFont(&<prefix>_14_regular);
EpdFontFamily <camel>14FontFamily(&<camel>14RegularFont, nullptr, nullptr, nullptr);
EpdFont <camel>16RegularFont(&<prefix>_16_regular);
EpdFontFamily <camel>16FontFamily(&<camel>16RegularFont, nullptr, nullptr, nullptr);
#ifndef OMIT_XLARGE_FONT
EpdFont <camel>18RegularFont(&<prefix>_18_regular);
EpdFontFamily <camel>18FontFamily(&<camel>18RegularFont, nullptr, nullptr, nullptr);
#endif
#ifndef OMIT_HUGE_FONT
EpdFont <camel>20RegularFont(&<prefix>_20_regular);
EpdFontFamily <camel>20FontFamily(&<camel>20RegularFont, nullptr, nullptr, nullptr);
#endif
#endif
```

`nullptr` for bold/italic/bolditalic — Bangla fonts are regular-only.

### 6b — insertFont calls

Find the closing `#endif  // OMIT_BANGLA_FONT` of the notobengali insertFont block (currently ends around line 436). Insert immediately after it:

```cpp
#ifndef OMIT_BANGLA_FONT
  renderer.insertFont(<ENUM>_10_FONT_ID, <camel>10FontFamily);
  renderer.insertFont(<ENUM>_12_FONT_ID, <camel>12FontFamily);
  renderer.insertFont(<ENUM>_14_FONT_ID, <camel>14FontFamily);
  renderer.insertFont(<ENUM>_16_FONT_ID, <camel>16FontFamily);
#ifndef OMIT_XLARGE_FONT
  renderer.insertFont(<ENUM>_18_FONT_ID, <camel>18FontFamily);
#endif
#ifndef OMIT_HUGE_FONT
  renderer.insertFont(<ENUM>_20_FONT_ID, <camel>20FontFamily);
#endif
#endif
```

Where `<ENUM>` is the SCREAMING_SNAKE_CASE name (e.g. `MUKTIBANGLA`).

---

## Step 7 — Update `lib/EpdFont/scripts/build-font-ids.sh` and regenerate `fontIds.h`

Read `build-font-ids.sh`. Find the closing notobengali entry and append:

```bash
# <DisplayName>
echo "#define <ENUM>_10_FONT_ID ($(hash_files ./<prefix>_10_regular.h))"
echo "#define <ENUM>_12_FONT_ID ($(hash_files ./<prefix>_12_regular.h))"
echo "#define <ENUM>_14_FONT_ID ($(hash_files ./<prefix>_14_regular.h))"
echo "#define <ENUM>_16_FONT_ID ($(hash_files ./<prefix>_16_regular.h))"
echo "#define <ENUM>_18_FONT_ID ($(hash_files ./<prefix>_18_regular.h))"
echo "#define <ENUM>_20_FONT_ID ($(hash_files ./<prefix>_20_regular.h))"
```

Then regenerate:

```bash
cd lib/EpdFont/builtinFonts && bash ../scripts/build-font-ids.sh > ../../../src/fontIds.h
```

---

## Step 8 — Update `src/CrossPointSettings.h`

⚠️ **Critical ordering rule**: The `FONT_FAMILY` enum value is stored as the raw `uint8_t` on device. The `SettingInfo::Enum` picker uses the stored value as a direct index into its `enumValues` vector. Append the new value at the END, before `FONT_FAMILY_COUNT`. NEVER insert mid-sequence.

Read `src/CrossPointSettings.h`. Find the `FONT_FAMILY` enum (currently line 105):

```cpp
// Current:
enum FONT_FAMILY { LEXENDDECA = 0, BITTER = 1, CHAREINK = 2, NOTOBENGALI = 3, FONT_FAMILY_COUNT };
// After:
enum FONT_FAMILY { LEXENDDECA = 0, BITTER = 1, CHAREINK = 2, NOTOBENGALI = 3, <ENUM> = 4, FONT_FAMILY_COUNT };
```

If a previous font was already added (FONT_FAMILY_COUNT > 4), append at the next available integer value — read the current enum first.

---

## Step 9 — Update `src/CrossPointSettings.cpp`

Read `src/CrossPointSettings.cpp`. Find the `getReaderFontId()` function. Locate the closing `#endif` of the NOTOBENGALI case (currently around line 557). Insert immediately after it:

```cpp
#ifndef OMIT_BANGLA_FONT
    case <ENUM>:
      switch (effectiveSize) {
#ifndef OMIT_TINY_FONT
        case TEENSY:
        case TINY:
          return <ENUM>_10_FONT_ID;
#endif
#ifndef OMIT_SMALL_FONT
        case SMALL:
          return <ENUM>_12_FONT_ID;
#endif
        case MEDIUM:
        default:
          return <ENUM>_14_FONT_ID;
        case LARGE:
          return <ENUM>_16_FONT_ID;
#ifndef OMIT_XLARGE_FONT
        case EXTRA_LARGE:
          return <ENUM>_18_FONT_ID;
#endif
#ifndef OMIT_HUGE_FONT
        case HUGE_SIZE:
          return <ENUM>_20_FONT_ID;
#endif
      }
#endif
```

Do NOT touch `getReaderLineCompression()` — the new font falls to `default:` (LexendDeca compression ratios), which is correct.

---

## Step 10 — Update `src/SettingsList.h`

⚠️ **Critical**: The vector index = stored fontFamily integer value. The new font is at index 4 (= enum value 4). It must be appended AFTER `STR_NOTO_BENGALI`, still inside the `#ifndef OMIT_BANGLA_FONT` guard.

Read `src/SettingsList.h`. Find the font family picker block (currently around line 43–52). Change:

```cpp
// Before:
#ifndef OMIT_BANGLA_FONT
                              StrId::STR_NOTO_BENGALI,
#endif
// After:
#ifndef OMIT_BANGLA_FONT
                              StrId::STR_NOTO_BENGALI,
                              StrId::STR_<I18N_KEY_SUFFIX>,
#endif
```

Where `STR_<I18N_KEY_SUFFIX>` matches the key you'll add in Step 11 (e.g. `STR_MUKTI_BANGLA`).

---

## Step 11 — Add i18n string and regenerate

Read `lib/I18n/translations/english.yaml`. Find the `STR_NOTO_BENGALI` line and add immediately after it:

```yaml
STR_<I18N_KEY>: "<DisplayName>"
```

Then regenerate:

```bash
cd lib/I18n && python3 ../../scripts/gen_i18n.py translations/ .
```

Confirm `lib/I18n/I18nKeys.h` now contains `STR_<I18N_KEY>`.

---

## Step 12 — Verify

```bash
pio run -e simulator
```

Must link cleanly. Then check the bangla env flash budget:

```bash
pio run -e bangla 2>&1 | grep -E "RAM:|Flash:"
```

**If Flash > 97%**: warn the user — the new font adds ~1–2 MB and the bangla env has limited headroom. Suggest adding `-DOMIT_BANGLA_FONT` as a last resort (hides both Bangla fonts), or propose reducing sizes by adding `-DOMIT_TINY_FONT` / `-DOMIT_SMALL_FONT` to `[env:bangla]` in `platformio.ini` if not already set.

**If Flash > 100%**: the build will fail. The user MUST trim flash. Offer to add the minimum OMIT flags needed to fit.

---

## Verification checklist

After a successful build, instruct the user to:
1. Open the simulator: `pio run -e simulator && .pio/build/simulator/program`
2. Go to Settings → Font Family → confirm the new font appears in the picker
3. Select the new font and open a Bangla EPUB
4. Verify conjuncts (ক্ষ, ন্ত্র), pre-base matras (ি), and Danda (।) all render correctly
