# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),

## [Unreleased]
### Added
- Add Bangla (Bengali) script support for reading EPUB and TXT content using Noto Sans Bengali font (sizes 10–20px). Conjuncts and pre-base matras are pre-shaped at build time via HarfBuzz and stored as PUA glyphs; a lightweight runtime shaper maps Bengali codepoint sequences to these glyphs with no RAM overhead.
- Prevent a crash when entering sleep with Page Overlay enabled if the cached EPUB page data is invalid
- Clean up EPUB table rendering by removing synthetic row/cell labels and defaulting table cells to readable left alignment
- Improve simple EPUB tables by buffering them into multi-column grid fragments instead of rendering each cell as an unrelated paragraph

### Fixed
- Fix ো (U+09CB) and ৌ (U+09CC) matras not wrapping their base consonant: the runtime BanglaShaper now decomposes each into its pre-base ে prefix (rendered left of the consonant) and post-base suffix (rendered right), using the canonical Unicode decompositions ো→ে+া and ৌ→ে+ৌ-right (U+09D7)
- Fix অ্যা not rendering correctly: added অ (U+0985) to the build-time consonant list so HarfBuzz generates a PUA glyph for অ্য and similar conjuncts; the runtime shaper also handles the alternate আ (U+0986)+্+C encoding by normalising আ→অ and appending the implicit আকার (া) vowel sign
- Fix an `abort()` crash when opening Bangla EPUB content: the font decompressor now checks available contiguous heap before resizing the hot-group buffer, and the PUA conjunct group (770 glyphs, 135 KB) is split into ≤200-glyph sub-groups (~33 KB each) during font generation so each decompression fits comfortably in ESP32-C3 heap
- Fix a crash when opening EPUB chapters that continue with normal text after a buffered table
- Fix a crash when using `Go to %` in EPUBs by serializing the jump calculation with other reader cache access
- Fix OTA update checks after the streaming release parser merge by keeping variant-aware firmware asset matching
