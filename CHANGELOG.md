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
- Fix a crash when opening EPUB chapters that continue with normal text after a buffered table
- Fix a crash when using `Go to %` in EPUBs by serializing the jump calculation with other reader cache access
- Fix OTA update checks after the streaming release parser merge by keeping variant-aware firmware asset matching
