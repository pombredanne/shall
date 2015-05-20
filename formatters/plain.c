#include <stddef.h>

#include "cpp.h"
#include "tokens.h"
#include "formatter.h"

static const struct {
    const char *name;
    size_t name_len;
} map[] = {
#define TOKEN(constant, description, cssclass) \
    { #constant, STR_LEN(#constant) },
#include "keywords.h"
#undef TOKEN
};

#define STRING_APPEND_STRING(dest, suffix) \
    do { \
        string_append_string_len(out, suffix, STR_LEN(suffix)); \
    } while (0);

#if 0
static int start_document(String *out, FormatterData *data)
{
    STRING_APPEND_STRING(out, "--BOS--");

    return 0;
}

static int end_document(String *out, FormatterData *UNUSED(data))
{
    STRING_APPEND_STRING(out, "--EOS--");

    return 0;
}
#endif

static int start_token(int token, String *out, FormatterData *UNUSED(data))
{
    string_append_string_len(out, map[token].name, map[token].name_len);
    STRING_APPEND_STRING(out, ": ");

    return 0;
}

static int end_token(int UNUSED(token), String *out, FormatterData *UNUSED(data))
{
    STRING_APPEND_STRING(out, "\n");

    return 0;
}

static int write_token(String *out, const char *token, size_t token_len, FormatterData *UNUSED(data))
{
    string_append_string_len_dump(out, token, token_len);

    return 0;
}

const FormatterImplementation _plainfmt = {
    "Plain",
    "Format tokens in plain text, mostly instended for tests. Each token is written on a new line with the form: <token name>: <token value>",
#ifndef WITHOUT_FORMATTER_OPTIONS
    formatter_implementation_default_get_option_ptr,
#endif
    NULL,
    NULL,
    start_token,
    end_token,
    write_token,
    sizeof(FormatterData),
    NULL
};

/*SHALL_API*/ const FormatterImplementation *plainfmt = &_plainfmt;
