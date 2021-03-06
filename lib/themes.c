/**
 * @file lib/themes.c
 * @brief interface with themes
 */

#include <string.h>

#include "cpp.h"
#include "utils.h"
#include "themes.h"
#include "xtring.h"
#include "hashtable.h"

extern SHALL_API const Theme monokai;
extern SHALL_API const Theme molokai;

static const Theme *available_themes[] = {
    &monokai,
    &molokai,
    NULL
};

/**
 * Exposes the number of available builtin themes
 *
 * @note for external use only
 */
SHALL_API const size_t SHALL_THEME_COUNT = ARRAY_SIZE(available_themes) - 1;

/**
 * Executes the given callback for each builtin lexer implementation
 *
 * @param cb the callback
 * @param data an additionnal user data to pass on callback invocation
 */
SHALL_API void theme_each(void (*cb)(const Theme *, void *), void *data)
{
    const Theme **theme;

    for (theme = available_themes; NULL != *theme; theme++) {
        cb(*theme, data);
    }
}

/**
 * Initialize an iterator to iterate on available themes
 *
 * @param it the iterator to set properly
 */
SHALL_API void themes_to_iterator(Iterator *it)
{
    null_terminated_ptr_array_to_iterator(it, (void *) available_themes);
}

/**
 * Gets name of a theme
 *
 * @param theme the theme "instance"
 *
 * @return its name
 */
SHALL_API const char *theme_name(const Theme *theme)
{
    return theme->name;
}

/**
 * Gets a theme from its name.
 *
 * @param name the theme's name
 *
 * @return NULL or the Theme of the given name
 */
SHALL_API const Theme *theme_by_name(const char *name)
{
    const Theme **theme;

    for (theme = available_themes; NULL != *theme; theme++) {
        if (0 == ascii_strcasecmp(name, (*theme)->name)) {
            return *theme;
        }
    }

    return NULL;
}

static bool parse_hexchar(char c, uint8_t *v)
{
    if (c >= '0' && c <= '9') {
        *v = c - '0';
        return true;
    }
    if (c >= 'A' && c <= 'F') {
        *v = 10 + c - 'A';
        return true;
    }
    if (c >= 'a' && c <= 'f') {
        *v = 10 + c - 'a';
        return true;
    }

    return false;
}

/**
 * Parse a color in hexadecimal format (#RGB or #RRGGBB)
 *
 * @param string the string to parse
 * @param string_len its length
 * @param color the color to set
 *
 * @return true if parsing was successfull
 */
SHALL_API bool color_parse_hexstring(const char *string, size_t string_len, Color *color)
{
    if (NULL != string && '#' == *string && (STR_LEN("#RGB") == string_len || STR_LEN("#RRGGBB") == string_len)) {
        size_t i;
        uint8_t *comp;
        bool on3digits;

        comp = (uint8_t *) color;
        bzero(color, sizeof(*color));
        on3digits = STR_LEN("#RGB") == string_len;
        for (i = 1; i < string_len; i++) {
            if (!parse_hexchar(string[i], comp)) {
                return false;
            }
            if (on3digits) {
                *comp |= *comp << 4;
            } else {
                uint8_t v;

                if (!parse_hexchar(string[++i], &v)) {
                    return false;
                }
                *comp <<= 4;
                *comp |= v;
            }
            comp++;
        }
    }

    return true;
}

#define IDENT_STRING "  "

#define STRING_APPEND_IDENT(string) \
    do { \
        if (pretty_print) { \
            string_append_string_len(string, IDENT_STRING, STR_LEN(IDENT_STRING)); \
        } \
    } while (0);

static ht_hash_t hash_style(const Style *style)
{
    struct hashed_style {
        union {
            Style style;
            ht_hash_t h;
        };
    } h;

    memcpy(&h.style, style, sizeof(*style));

    return h.h;
}

/**
 * Generates CSS for a theme
 *
 * @param theme
 * @param scope
 * 
 * @return a string describing the theme in CSS format
 * @todo boolean option to skip background colors
 */
SHALL_API char *theme_export_as_css(const Theme *theme, const char *scope, bool pretty_print)
{
    size_t i, j;
    String *buffer;
    HashTable groups;
    int *last[_TOKEN_COUNT], grouped[_TOKEN_COUNT][_TOKEN_COUNT];

    buffer = string_new();
    hashtable_init(&groups, _TOKEN_COUNT, value_hash, NULL, NULL, NULL, NULL);
    for (i = 0; i < _TOKEN_COUNT; i++) {
        last[i] = &grouped[i][0];
        for (j = 0; j < _TOKEN_COUNT; j++) {
            grouped[i][j] = -1;
        }
        if (' ' != *tokens[i].cssclass && 0 != theme->styles[i].flags) {
            int **ptr;

            ptr = NULL;
            if (hashtable_direct_put(&groups, HT_PUT_ON_DUP_KEY_PRESERVE, hash_style(&theme->styles[i]), &last[i], &ptr)) {
                *last[i] = i;
                ++last[i];
            } else {
                **ptr = i;
                ++*ptr;
            }
        }
    }
    hashtable_destroy(&groups);
    for (i = 0; i < _TOKEN_COUNT; i++) {
        if (-1 != grouped[i][0]) {
            for (j = 0; j < _TOKEN_COUNT && -1 != grouped[i][j]; j++) {
                if (0 != j) {
                    STRING_APPEND_STRING(buffer, ", ");
                }
                if (NULL != scope) {
                    string_append_string(buffer, scope);
                    string_append_char(buffer, ' ');
                }
                string_append_char(buffer, '.');
                string_append_string(buffer, tokens[grouped[i][j]].cssclass);
            }
            STRING_APPEND_STRING(buffer, " {\n"); // TODO: add a description in comment
            if (theme->styles[i].bg_set) {
                STRING_APPEND_IDENT(buffer);
                STRING_APPEND_COLOR(buffer, "background-color: ", theme->styles[i].bg, ";\n");
            }
            if (theme->styles[i].fg_set) {
                STRING_APPEND_IDENT(buffer);
                STRING_APPEND_COLOR(buffer, "color: ", theme->styles[i].fg, ";\n");
            }
            if (theme->styles[i].bold) {
                STRING_APPEND_IDENT(buffer);
                STRING_APPEND_STRING(buffer, "font-weight: bold;\n");
            }
            if (theme->styles[i].italic) {
                STRING_APPEND_IDENT(buffer);
                STRING_APPEND_STRING(buffer, "font-style: italic;\n");
            }
            if (theme->styles[i].underline) {
                STRING_APPEND_IDENT(buffer);
                STRING_APPEND_STRING(buffer, "text-decoration: underline;\n");
            }
            STRING_APPEND_STRING(buffer, "}\n");
        }
    }

    return string_orphan(buffer);
}
