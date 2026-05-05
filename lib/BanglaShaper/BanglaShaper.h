#pragma once
#include <stddef.h>
#include <stdint.h>

// Pre-base matras that must be reordered: typed after base consonant, drawn before it.
// U+09BF ি and U+09C7 ে are swapped (C + matra → matra + C) at runtime.
// Conjuncts (C + U+09CD + C) are replaced by PUA codepoints pre-shaped at build time.

class BanglaShaper {
public:
    // Returns true if utf8 contains any codepoint in the Bangla block U+0980–U+09FF.
    static bool containsBangla(const char* utf8);

    // Transforms Bangla sequences in utf8 to shaped equivalents and writes to out.
    // Conjuncts → PUA codepoint; pre-base matras → reordered in output.
    // Non-Bangla codepoints pass through unchanged.
    // Returns number of bytes written (not including null terminator).
    // out must have capacity for at least maxOut bytes.
    static size_t shape(const char* in, size_t inLen, char* out, size_t maxOut);

private:
    static uint32_t lookupCluster(uint32_t cp1, uint32_t cp2, uint32_t cp3);
    static int encodeUtf8(uint32_t cp, char* out);
};
