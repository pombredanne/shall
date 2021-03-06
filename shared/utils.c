/**
 * @file shared/utils.c
 * @brief string utility (search and comparaisons) functions
 */

#include <string.h>

#include "cpp.h"
#include "utils.h"

/*
static const unsigned char lower[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};
*/

static const unsigned char upper[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

int ascii_toupper(int c) {
    return (int) upper[(unsigned char) c];
}

int ascii_strcasecmp(const char *str1, const char *str2)
{
    int c1, c2;

    if (str1 != str2) {
        do {
            if ('\0' == *str1) {
                return 0;
            }
            c1 = ascii_toupper(*(unsigned char *) str1++);
            c2 = ascii_toupper(*(unsigned char *) str2++);
        } while (c1 == c2);

        return c1 - c2;
    }

    return 0;
}

int ascii_strcasecmp_l(
    const char *str1, size_t str1_len,
    const char *str2, size_t str2_len
) {
    int c1, c2;

    if (str1 != str2) {
        size_t min_len;

        if (str2_len < str1_len) {
            min_len = str2_len;
        } else {
            min_len = str1_len;
        }
        while (0 != min_len--) {
            c1 = ascii_toupper(*(unsigned char *) str1++);
            c2 = ascii_toupper(*(unsigned char *) str2++);
            if (c1 != c2) {
                return c1 - c2;
            }
        }
    }

    return str1_len - str2_len;
}

int ascii_strncasecmp_l(
    const char *str1, size_t str1_len,
    const char *str2, size_t str2_len,
    size_t n
) {
    int c1, c2;

    if (str1 != str2 && 0 != n) {
        size_t min_len;

        min_len =  MIN(n, MIN(str1_len, str2_len));
        do {
            if ('\0' == *str1) {
                return 0;
            }
            c1 = ascii_toupper(*(unsigned char *) str1++);
            c2 = ascii_toupper(*(unsigned char *) str2++);
        } while (c1 == c2 && 0 != --min_len);

        return (unsigned char) c1 - (unsigned char) c2;
    }

    return 0;
}

int strcmp_l(
    const char *str1, size_t str1_len,
    const char *str2, size_t str2_len
) {
    if (str1 != str2) {
        size_t min_len;

        if (str2_len < str1_len) {
            min_len = str2_len;
        } else {
            min_len = str1_len;
        }
        while (0 != min_len--) {
            if (*str1 != *str2) {
                return (unsigned char) *str1 - (unsigned char) *str2;
            }
            ++str1, ++str2;
        }
    }

    return str1_len - str2_len;
}

int strncmp_l(
    const char *str1, size_t str1_len,
    const char *str2, size_t str2_len,
    size_t n
) {
    if (str1 != str2 && 0 != n) {
        size_t min_len;

        min_len =  MIN(n, MIN(str1_len, str2_len));
        while (0 != min_len--) {
            if (*str1 != *str2) {
                return *(const unsigned char *) str1 - *(const unsigned char *) str2;
            }
            ++str1, ++str2;
        }

        return MIN(n, str1_len) - MIN(n, str2_len);
    }

    return 0;
}

char *ascii_memcasechr(const char *str, int c, size_t n)
{
    int uc;

    for (uc = ascii_toupper((unsigned char) c); 0 != n; n--, str++) {
        if (uc == ascii_toupper(*(const unsigned char *) str)) {
            return (char *) str;
        }
    }

    return NULL;
}

int ascii_memcasecmp(const char *str1, const char *str2, size_t n)
{
    if (str1 != str2) {
        for (; 0 != n; n--) {
            int c1, c2;

            c1 = ascii_toupper(*(unsigned char *) str1++);
            c2 = ascii_toupper(*(unsigned char *) str2++);
            if (c1 != c2) {
                return (unsigned char) c1 - (unsigned char) c2;
            }
        }
    }

    return 0;
}

char *memstr(const char *haystack, const char *needle, size_t needle_len, const char * const haystack_end)
{
    if (needle_len <= (size_t) (haystack_end - haystack)) {
        if (0 == needle_len) {
            return (char *) haystack;
        } else if (1 == needle_len) {
            return (char *) memchr(haystack, (int) needle[0], haystack_end - haystack);
        } else {
            const char *p;
            const char * const l = haystack_end - needle_len; // last possible position to find a match

            for (p = haystack; p <= l; p++) {
                if (*p == needle[0] && 0 == memcmp(needle + 1, p + 1, needle_len - 1)) {
                    return (char *) p;
                }
            }
        }
    }

    return NULL;
}

#include <stdlib.h>

#undef TOUPPER
#define TOUPPER(c) \
    (char) ascii_toupper((unsigned char) c)

typedef struct {
    size_t i, q;
    unsigned int flags;
    size_t pattern_len;
    size_t *prefix_table;
    char pattern[0];
    // simulates:
/*
    union {
        char variable_splace[sizeof(*ctxt->prefix_table) * pattern_len + sizeof(*ctxt->pattern) * (pattern_len + 1)];
        struct {
            char pattern[pattern_len + 1];
            size_t prefix_table[pattern_len];
        };
    };
*/
} kmp_ctxt;

void *kmp_init(const char *pattern, size_t pattern_len, unsigned int flags)
{
    size_t k, q;
    kmp_ctxt *ctxt;

    ctxt = malloc(sizeof(*ctxt) + sizeof(*ctxt->prefix_table) * pattern_len + sizeof(*ctxt->pattern) * (pattern_len + 1)); // +1 for \0, even if we don't use or rely it
    ctxt->flags = flags;
    ctxt->i = ctxt->q = 0;
//     ctxt->pattern = HAS_FLAG(ctxt->flags, KMP_PATTERN_DUP) ? strdup(pattern) : pattern;
    memcpy(ctxt->pattern, pattern, pattern_len);
    ctxt->pattern[pattern_len] = '\0';
    ctxt->pattern_len = pattern_len;
    ctxt->prefix_table = (size_t *) (ctxt->pattern + ctxt->pattern_len + 1); // + 1: after the \0
    ctxt->prefix_table[0] = 0;
    k = 0;
    if (HAS_FLAG(ctxt->flags, KMP_INSENSITIVE)) {
        for (q = 0; q < pattern_len; q++) {
            ctxt->pattern[q] = TOUPPER(ctxt->pattern[q]);
        }
    }
    for (q = 1; q < ctxt->pattern_len; q++) {
        while (k > 0 && (ctxt->pattern[k]) != (HAS_FLAG(ctxt->flags, KMP_INSENSITIVE) ? TOUPPER(ctxt->pattern[q]) : ctxt->pattern[q])) {
            k = ctxt->prefix_table[k - 1];
        }
        if ((ctxt->pattern[k]) == (HAS_FLAG(ctxt->flags, KMP_INSENSITIVE) ? TOUPPER(ctxt->pattern[q]) : ctxt->pattern[q])) {
            k++;
        }
        ctxt->prefix_table[q] = k;
    }

    return (void *) ctxt;
}

char *kmp_search_first(const char *string, size_t string_len, void *rawctxt)
{
    kmp_ctxt *ctxt;

    ctxt = (kmp_ctxt *) rawctxt;
    ctxt->i = ctxt->q = 0;

    return kmp_search_next(string, string_len, rawctxt);
}

char *kmp_search_next(const char *string, size_t string_len, void *rawctxt)
{
    char *match;
    kmp_ctxt *ctxt;

    match = NULL;
    ctxt = (kmp_ctxt *) rawctxt;
    if (EXPECTED(string_len > ctxt->pattern_len)) {
        for (/* NOP */; NULL == match && ctxt->i < string_len; ctxt->i++) {
            while (ctxt->q > 0 && ctxt->pattern[ctxt->q] != (HAS_FLAG(ctxt->flags, KMP_INSENSITIVE) ? TOUPPER(string[ctxt->i]) : string[ctxt->i])) {
                ctxt->q = ctxt->prefix_table[ctxt->q - 1];
            }
            if (ctxt->pattern[ctxt->q] == (HAS_FLAG(ctxt->flags, KMP_INSENSITIVE) ? TOUPPER(string[ctxt->i]) : string[ctxt->i])) {
                ctxt->q++;
            }
            if (ctxt->q == ctxt->pattern_len) {
                match = (char *) string + ctxt->i - ctxt->pattern_len + 1;
            }
        }
    }

    return match;
}

void kmp_finalize(void *rawctxt)
{
    kmp_ctxt *ctxt;

    ctxt = (kmp_ctxt *) rawctxt;
//     if (HAS_FLAG(ctxt->flags, KMP_PATTERN_DUP)) {
//         free((void *) ctxt->pattern);
//     }
    free(rawctxt);
}
