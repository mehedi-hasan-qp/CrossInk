#include "BanglaShaper.h"
#include "bangla_clusters.h"
#include <string.h>

// Bangla block: U+0980–U+09FF
static constexpr uint32_t BANGLA_START = 0x0980;
static constexpr uint32_t BANGLA_END   = 0x09FF;

// Pre-base matras: appear after base consonant in logical order but before it visually.
static constexpr uint32_t MATRA_PRE_I   = 0x09BF;  // ি
static constexpr uint32_t MATRA_PRE_E   = 0x09C7;  // ে

// Hasanta / Virama — joins two consonants into a conjunct.
static constexpr uint32_t VIRAMA = 0x09CD;

// Two-part wrapping matras — split around base consonant: ে-prefix left, right-part right.
static constexpr uint32_t MATRA_O      = 0x09CB;  // ো = ে (09C7) + া (09BE)
static constexpr uint32_t MATRA_OI     = 0x09CC;  // ৌ = ে (09C7) + ৌ-right (09D7)
static constexpr uint32_t MATRA_AA     = 0x09BE;  // া  (right part of ো)
static constexpr uint32_t MATRA_AU_LEN = 0x09D7;  // ৌ-right (right part of ৌ)

// Independent vowels used for আ→অ normalization before conjunct lookup.
static constexpr uint32_t VOWEL_A  = 0x0985;  // অ
static constexpr uint32_t VOWEL_AA = 0x0986;  // আ (= অ + া conceptually)

static inline bool isBangla(uint32_t cp) {
    return cp >= BANGLA_START && cp <= BANGLA_END;
}

static inline bool isPreBaseMatra(uint32_t cp) {
    return cp == MATRA_PRE_I || cp == MATRA_PRE_E;
}

// Decode one UTF-8 codepoint from src, advance *src past it, return codepoint.
// Returns 0xFFFFFFFF on error or end of string.
static uint32_t decodeOne(const char** src, const char* end) {
    if (*src >= end) return 0xFFFFFFFF;
    const uint8_t* p = (const uint8_t*)*src;
    uint32_t cp;
    int bytes;
    if (p[0] < 0x80) {
        cp = p[0]; bytes = 1;
    } else if ((p[0] & 0xE0) == 0xC0 && *src + 2 <= end) {
        cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F); bytes = 2;
    } else if ((p[0] & 0xF0) == 0xE0 && *src + 3 <= end) {
        cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); bytes = 3;
    } else if ((p[0] & 0xF8) == 0xF0 && *src + 4 <= end) {
        cp = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F); bytes = 4;
    } else {
        cp = p[0]; bytes = 1;
    }
    *src += bytes;
    return cp;
}

int BanglaShaper::encodeUtf8(uint32_t cp, char* out) {
    if (cp < 0x80) {
        out[0] = (char)cp;
        return 1;
    } else if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    } else if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    } else {
        out[0] = (char)(0xF0 | (cp >> 18));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
}

uint32_t BanglaShaper::lookupCluster(uint32_t cp1, uint32_t cp2, uint32_t cp3) {
    uint64_t key;
    if (cp3 != 0) {
        key = ((uint64_t)cp1 << 42) | ((uint64_t)cp2 << 21) | (uint64_t)cp3;
    } else {
        key = ((uint64_t)cp1 << 21) | (uint64_t)cp2;
    }

    // Binary search on sorted BANGLA_CLUSTERS
    int lo = 0, hi = (int)BANGLA_CLUSTER_COUNT - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (BANGLA_CLUSTERS[mid].key == key) return BANGLA_CLUSTERS[mid].puaCp;
        if (BANGLA_CLUSTERS[mid].key < key) lo = mid + 1;
        else hi = mid - 1;
    }
    return 0;
}

uint32_t BanglaShaper::lookupCluster5(uint32_t cp1, uint32_t cp3, uint32_t cp5) {
    uint64_t key = (1ULL << 63) | ((uint64_t)cp1 << 42) | ((uint64_t)cp3 << 21) | cp5;
    int lo = 0, hi = (int)BANGLA_CLUSTER_COUNT - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (BANGLA_CLUSTERS[mid].key == key) return BANGLA_CLUSTERS[mid].puaCp;
        if (BANGLA_CLUSTERS[mid].key < key) lo = mid + 1;
        else hi = mid - 1;
    }
    return 0;
}

bool BanglaShaper::containsBangla(const char* utf8) {
    if (!utf8) return false;
    const char* p = utf8;
    const char* end = utf8 + strlen(utf8);
    while (p < end) {
        uint32_t cp = decodeOne(&p, end);
        if (isBangla(cp)) return true;
    }
    return false;
}

size_t BanglaShaper::shape(const char* in, size_t inLen, char* out, size_t maxOut) {
    const char* src = in;
    const char* srcEnd = in + inLen;
    char* dst = out;
    char* dstEnd = out + maxOut - 4;  // reserve space for longest UTF-8 sequence

    // Lookahead buffer: peek up to 2 codepoints ahead without consuming them.
    // We store decoded codepoints and their byte lengths for re-encoding.
    while (src < srcEnd && dst < dstEnd) {
        const char* cpStart = src;
        uint32_t cp1 = decodeOne(&src, srcEnd);
        if (cp1 == 0xFFFFFFFF) break;

        if (!isBangla(cp1)) {
            // Pass through non-Bangla codepoints unchanged.
            size_t len = (size_t)(src - cpStart);
            if (dst + len > dstEnd) break;
            memcpy(dst, cpStart, len);
            dst += len;
            continue;
        }

        // Peek cp2
        const char* cp2Start = src;
        uint32_t cp2 = decodeOne(&src, srcEnd);

        if (cp2 == 0xFFFFFFFF) {
            // cp1 is last character — emit as-is
            int n = encodeUtf8(cp1, dst);
            dst += n;
            break;
        }

        // Two-part wrapping matras: C + ো/ৌ → ে + C + right-part
        // ো (09CB) = ে (pre-base, left) + া (09BE, right)
        // ৌ (09CC) = ে (pre-base, left) + ৌ-right (09D7, right)
        if (cp2 == MATRA_O || cp2 == MATRA_OI) {
            uint32_t rightPart = (cp2 == MATRA_O) ? MATRA_AA : MATRA_AU_LEN;
            if (dst + 9 <= dstEnd) {
                dst += encodeUtf8(MATRA_PRE_E, dst);  // ে before consonant
                dst += encodeUtf8(cp1, dst);           // base consonant
                dst += encodeUtf8(rightPart, dst);     // া or ৌ-right after
            }
            continue;
        }

        // Check for pre-base matra: C + ি/ে → reorder to ি/ে + C
        if (isPreBaseMatra(cp2)) {
            // Peek cp3 to make sure cp1 is a consonant-like codepoint, then reorder.
            if (dst + 6 <= dstEnd) {
                dst += encodeUtf8(cp2, dst);  // matra first
                dst += encodeUtf8(cp1, dst);  // base consonant second
            }
            continue;
        }

        // Check for virama (conjunct): cp1 + U+09CD + cp3
        if (cp2 == VIRAMA) {
            const char* cp3Start = src;
            uint32_t cp3 = decodeOne(&src, srcEnd);

            if (cp3 != 0xFFFFFFFF && isBangla(cp3)) {
                // Normalize আ (0x0986) → অ (0x0985) for conjunct lookup.
                // আ decomposes conceptually to অ + া; if a PUA exists for অ্x, use it
                // and append MATRA_AA as the implicit post-base vowel.
                bool aaVowelNorm = (cp1 == VOWEL_AA);
                uint32_t lookupCp1 = aaVowelNorm ? VOWEL_A : cp1;

                // Try 5-cp match first: C1 + VIRAMA + C2 + VIRAMA + C3
                {
                    const char* peek4 = src;
                    uint32_t cp4peek = decodeOne(&src, srcEnd);
                    if (cp4peek == VIRAMA) {
                        const char* peek5 = src;
                        uint32_t cp5peek = decodeOne(&src, srcEnd);
                        if (cp5peek != 0xFFFFFFFF && isBangla(cp5peek)) {
                            uint32_t pua5 = lookupCluster5(lookupCp1, cp3, cp5peek);
                            if (pua5 != 0) {
                                const char* cp6Start = src;
                                uint32_t cp6 = decodeOne(&src, srcEnd);
                                bool cp6IsPreBase = (cp6 != 0xFFFFFFFF && isPreBaseMatra(cp6));
                                bool cp6IsWrap    = (cp6 != 0xFFFFFFFF && (cp6 == MATRA_O || cp6 == MATRA_OI));
                                if (cp6IsPreBase) {
                                    if (dst + 9 <= dstEnd) {
                                        dst += encodeUtf8(cp6, dst);
                                        dst += encodeUtf8(pua5, dst);
                                        if (aaVowelNorm) dst += encodeUtf8(MATRA_AA, dst);
                                    }
                                } else if (cp6IsWrap) {
                                    uint32_t rightPart = (cp6 == MATRA_O) ? MATRA_AA : MATRA_AU_LEN;
                                    int needed = aaVowelNorm ? 12 : 9;
                                    if (dst + needed <= dstEnd) {
                                        dst += encodeUtf8(MATRA_PRE_E, dst);
                                        dst += encodeUtf8(pua5, dst);
                                        dst += encodeUtf8(rightPart, dst);
                                        if (aaVowelNorm) dst += encodeUtf8(MATRA_AA, dst);
                                    }
                                } else {
                                    int needed = aaVowelNorm ? 6 : 3;
                                    if (dst + needed <= dstEnd) {
                                        dst += encodeUtf8(pua5, dst);
                                        if (aaVowelNorm) dst += encodeUtf8(MATRA_AA, dst);
                                    }
                                    if (cp6 != 0xFFFFFFFF) src = cp6Start;
                                }
                                continue;
                            }
                        }
                    }
                    src = peek4;  // no 5-cp match — restore for 3-cp path
                }

                uint32_t pua = lookupCluster(lookupCp1, VIRAMA, cp3);
                if (pua != 0) {
                    const char* cp4Start = src;
                    uint32_t cp4 = decodeOne(&src, srcEnd);

                    bool cp4IsPreBase = (cp4 != 0xFFFFFFFF && isPreBaseMatra(cp4));
                    bool cp4IsWrap    = (cp4 != 0xFFFFFFFF && (cp4 == MATRA_O || cp4 == MATRA_OI));

                    if (cp4IsPreBase) {
                        if (dst + 9 <= dstEnd) {
                            dst += encodeUtf8(cp4, dst);
                            dst += encodeUtf8(pua, dst);
                            if (aaVowelNorm) dst += encodeUtf8(MATRA_AA, dst);
                        }
                    } else if (cp4IsWrap) {
                        uint32_t rightPart = (cp4 == MATRA_O) ? MATRA_AA : MATRA_AU_LEN;
                        int needed = aaVowelNorm ? 12 : 9;
                        if (dst + needed <= dstEnd) {
                            dst += encodeUtf8(MATRA_PRE_E, dst);
                            dst += encodeUtf8(pua, dst);
                            dst += encodeUtf8(rightPart, dst);
                            if (aaVowelNorm) dst += encodeUtf8(MATRA_AA, dst);
                        }
                    } else {
                        int needed = aaVowelNorm ? 6 : 3;
                        if (dst + needed <= dstEnd) {
                            dst += encodeUtf8(pua, dst);
                            if (aaVowelNorm) dst += encodeUtf8(MATRA_AA, dst);
                        }
                        if (cp4 != 0xFFFFFFFF) src = cp4Start;
                    }
                    continue;
                }
                // No shaped form found — emit all three literally
                if (dst + 9 <= dstEnd) {
                    dst += encodeUtf8(cp1, dst);
                    dst += encodeUtf8(VIRAMA, dst);
                    dst += encodeUtf8(cp3, dst);
                }
                continue;
            }

            // Virama at end of string or followed by non-Bangla: emit cp1 + virama literally
            if (dst + 6 <= dstEnd) {
                dst += encodeUtf8(cp1, dst);
                dst += encodeUtf8(VIRAMA, dst);
            }
            // Put cp3 back if we consumed it (non-Bangla: back up src)
            if (cp3 != 0xFFFFFFFF) src = cp3Start;
            continue;
        }

        // No special sequence — emit cp1 and put cp2 back by rewinding src
        if (dst + 4 <= dstEnd) {
            dst += encodeUtf8(cp1, dst);
        }
        src = cp2Start;
    }

    // Flush any remaining bytes from src if we broke out early (non-Bangla tail)
    // Only needed if we exit the loop without consuming all of src due to dstEnd limit.
    // In practice, maxOut should be >= 3 * inLen for worst case.
    *dst = '\0';
    return (size_t)(dst - out);
}
