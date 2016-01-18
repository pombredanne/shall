#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cpp.h"
#include "tokens.h"
#include "themes.h"
#include "formatter.h"

#define BOLD      "01"
#define FAINT     "02"
#define STANDOUT  "03"
#define UNDERLINE "04"
#define BLINK     "05"
#define OVERLINE  "06"

typedef struct {
    const Theme *theme ALIGNED(sizeof(OptionValue));
    struct {
        size_t value_len;
        const char *value;
    } sequences[_TOKEN_COUNT];
} TerminalFormatterData;

// bold (1) + italic (3) + underline (4) (unused for now) + fg (38;2;R;G;B) + bg (48;2;R;G;B)
#define LONGEST_ANSI_ESCAPE_SEQUENCE \
    "\e[1;3;4;38;2;RRR;GGG;BBB;48;2;RRR;GGG;BBBm"

static int terminal_start_document(String *UNUSED(out), FormatterData *data)
{
    size_t i;
    const Theme *theme;
    TerminalFormatterData *mydata;

    mydata = (TerminalFormatterData *) data;
    // TODO: define a default theme in shall itself
    if (NULL == (theme = mydata->theme)) {
        theme = theme_by_name("molokai");
    }
    for (i = 0; i < _TOKEN_COUNT; i++) {
        mydata->sequences[i].value_len = 0;
        mydata->sequences[i].value = NULL;
        if (theme->styles[i].flags) {
            char *w, buffer[STR_SIZE(LONGEST_ANSI_ESCAPE_SEQUENCE)];

            *buffer = '\0';
            w = stpcpy(buffer, "\e[");
            if (theme->styles[i].bold) {
                w = stpcpy(w, "1;");
            }
            if (theme->styles[i].italic) {
                w = stpcpy(w, "3;");
            }
            if (theme->styles[i].fg_set) {
                w += sprintf(w, "38;2;%" PRIu8 ";%" PRIu8 ";%" PRIu8 ";", theme->styles[i].fg.r, theme->styles[i].fg.g, theme->styles[i].fg.b);
            }
            if (theme->styles[i].bg_set) {
                w += sprintf(w, "48;2;%" PRIu8 ";%" PRIu8 ";%" PRIu8 ";", theme->styles[i].bg.r, theme->styles[i].bg.g, theme->styles[i].bg.b);
            }
            w[-1] = 'm'; // overwrite last ';'
            mydata->sequences[i].value = strdup(buffer);
            mydata->sequences[i].value_len = w - buffer;
        }
    }

    return 0;
}

static int terminal_end_document(String *UNUSED(out), FormatterData *data)
{
    size_t i;
    TerminalFormatterData *mydata;

    mydata = (TerminalFormatterData *) data;
    for (i = 0; i < _TOKEN_COUNT; i++) {
        if (mydata->sequences[i].value_len > 0) {
            free((void *) mydata->sequences[i].value);
        }
    }

    return 0;
}

static int terminal_start_token(int token, String *out, FormatterData *data)
{
    TerminalFormatterData *mydata;

    mydata = (TerminalFormatterData *) data;
    if (mydata->sequences[token].value_len > 0) {
        string_append_string_len(out, mydata->sequences[token].value, mydata->sequences[token].value_len);
    }

    return 0;
}

static int terminal_end_token(int token, String *out, FormatterData *data)
{
    TerminalFormatterData *mydata;

    mydata = (TerminalFormatterData *) data;
    if (mydata->sequences[token].value_len > 0) {
        STRING_APPEND_STRING(out, "\e[39;49;00m");
    }

    return 0;
}

static int terminal_write_token(String *out, const char *token, size_t token_len, FormatterData *UNUSED(data))
{
    string_append_string_len(out, token, token_len);

    return 0;
}

const FormatterImplementation _termfmt = {
    "Terminal",
    "Format tokens with ANSI color sequences, for output in a text console",
#ifndef WITHOUT_FORMATTER_OPTIONS
    formatter_implementation_default_get_option_ptr,
#endif
    terminal_start_document,
    terminal_end_document,
    terminal_start_token,
    terminal_end_token,
    terminal_write_token,
    NULL,
    NULL,
    sizeof(TerminalFormatterData),
        (/*const*/ FormatterOption /*const*/ []) {
        { S("theme"), OPT_TYPE_THEME, offsetof(TerminalFormatterData, theme), OPT_DEF_THEME, "the theme to use" },
        END_OF_FORMATTER_OPTIONS
    }
};

/*SHALL_API*/ const FormatterImplementation *termfmt = &_termfmt;
