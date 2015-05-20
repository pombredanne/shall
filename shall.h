#ifndef SHALL_H

# define SHALL_H

# define SHALL_VERSION_MAJOR 0
# define SHALL_VERSION_MINOR 0
# define SHALL_VERSION_PATCH 0

# define SHALL_API

# ifndef SHALL_LEXER_DEFINED
typedef struct LexerImplementation LexerImplementation;
typedef struct Lexer Lexer;
# endif /* SHALL_LEXER_DEFINED */
# ifndef SHALL_FORMATTER_DEFINED
typedef struct FormatterImplementation FormatterImplementation;
typedef struct Formatter Formatter;
# endif /* SHALL_FORMATTER_DEFINED */

typedef void (*on_lexer_destroy_cb_t)(void *);

SHALL_API const size_t SHALL_LEXER_COUNT;

SHALL_API Lexer *lexer_unwrap(OptionValue);

SHALL_API const char *lexer_implementation_name(const LexerImplementation *);
SHALL_API const char *lexer_implementation_description(const LexerImplementation *);
SHALL_API const LexerImplementation *lexer_implementation_by_name(const char *);
SHALL_API const LexerImplementation *lexer_implementation_for_filename(const char *);
SHALL_API const LexerImplementation *lexer_implementation_for_mimetype(const char *);
SHALL_API const LexerImplementation *lexer_implementation_guess(const char *, size_t);
SHALL_API void lexer_implementation_each(void (*) (const LexerImplementation *, void *), void *);
SHALL_API void lexer_implementation_each_alias(const LexerImplementation *, void (*)(const char *, void *), void *);
SHALL_API void lexer_implementation_each_filename(const LexerImplementation *, void (*)(const char *, void *), void *);
SHALL_API void lexer_implementation_each_mimetype(const LexerImplementation *, void (*)(const char *, void *), void *);
SHALL_API void lexer_implementation_each_option(const LexerImplementation *, void (*)(int, const char *, OptionValue, const char *, void *), void *);

SHALL_API void lexer_each_sublexers(Lexer *, on_lexer_destroy_cb_t);
SHALL_API void lexer_destroy(Lexer *, on_lexer_destroy_cb_t);
SHALL_API Lexer *lexer_create(const LexerImplementation *);
SHALL_API Lexer *lexer_from_string(const char *, const char **);
SHALL_API const LexerImplementation *lexer_implementation(Lexer *);

enum {
    OPT_SUCCESS,
    OPT_ERR_TYPE_MISMATCH,
    OPT_ERR_UNKNOWN_LEXER,
    OPT_ERR_INVALID_VALUE,
    OPT_ERR_INVALID_OPTION
};

SHALL_API int lexer_get_option(Lexer *, const char *, OptionValue **);
SHALL_API int lexer_set_option(Lexer *, const char *, OptionType, OptionValue, void **);
SHALL_API int lexer_set_option_as_string(Lexer *, const char *, const char *, size_t);

# ifndef DOXYGEN
extern SHALL_API const FormatterImplementation *htmlfmt;
extern SHALL_API const FormatterImplementation *termfmt;
extern SHALL_API const FormatterImplementation *plainfmt;
# endif /* !DOXYGEN */

SHALL_API const size_t SHALL_FORMATTER_COUNT;

SHALL_API const FormatterImplementation *formatter_implementation(Formatter *);
SHALL_API const FormatterImplementation *formatter_implementation_by_name(const char *);
SHALL_API const char *formatter_implementation_name(const FormatterImplementation *);
SHALL_API const char *formatter_implementation_description(const FormatterImplementation *);
SHALL_API void formatter_implementation_each(void (*)(const FormatterImplementation *, void *), void *);
SHALL_API void formatter_implementation_each_option(const FormatterImplementation *, void (*)(int, const char *, OptionValue, const char *, void *), void *);

SHALL_API void formatter_destroy(Formatter *);
SHALL_API Formatter *formatter_create(const FormatterImplementation *);

SHALL_API int formatter_get_option(Formatter *, const char *, OptionValue **);
SHALL_API int formatter_set_option(Formatter *, const char *, OptionType, OptionValue);
SHALL_API int formatter_set_option_as_string(Formatter *, const char *, const char *, size_t);

SHALL_API size_t highlight_string(Lexer *, Formatter *, const char *, char **);

#endif /* !SHALL_H  */
