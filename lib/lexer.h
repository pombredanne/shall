#ifndef LEXER_H

# define LEXER_H

# include "types.h"
# include "darray.h"

# ifndef DOXYGEN
#  define SHELLMAGIC "#!"
#  define UTF8_BOM "\xEF\xBB\xBF"
# endif /* !DOXYGEN */

# define YYLEX_ARGS LexerInput *yy, LexerData *data, envent_cb_t cb, void *cb_data
# define YYCTYPE  unsigned char
# define YYTEXT   (yy->yytext)
// # define YYLINENO (yy->lineno)
# define YYLIMIT  (yy->limit)  /*re2c:define:YYLIMIT  = yy->limit;*/
# define YYCURSOR (yy->cursor) /*re2c:define:YYCURSOR = yy->cursor;*/
# define YYMARKER (yy->marker) /*re2c:define:YYMARKER = yy->marker;*/
# define YYLENG   (yy->cursor - yy->yytext)
# define YYFILL(n) \
    do { \
        if ((YYCURSOR/* + n*/) >= YYLIMIT) { \
            return 0; \
        } \
    } while(0);

#define yyless(x) \
    do { \
        YYCURSOR = YYTEXT + x; \
    } while (0);

# define STATE(name)  yyc##name /* re2c:condenumprefix */
# define BEGIN(state) YYSETCONDITION(STATE(state))
# define YYSTATE      YYGETCONDITION()

# define YYGETCONDITION()  data->state
# define YYSETCONDITION(s) data->state = s
# if 0
#  define YYDEBUG(s, c) fprintf(stderr, "state: %d char: %c\n", s, c)
# else
#  define YYDEBUG(s, c)
# endif

# define yymore() goto yymore_restart

#define NEWLINE /* TODO */

# define TOKEN(type) \
    cb(EVENT_TOKEN, cb_data, type); \

# define PUSH_TOKEN(type) \
    cb(EVENT_TOKEN, cb_data, type); \
    continue;

# define PUSH(limp, ldata) \
    do { \
        cb(EVENT_PUSH, cb_data, limp, ldata); \
        continue; \
    } while (0);

# define DONE \
    do { \
        cb(EVENT_DONE, cb_data); \
        return 1; \
    } while (0);

# define REPLAY(cursor, limit, limp, ldata) \
    do { \
        cb(EVENT_REPLAY, cb_data, cursor, limit, limp, ldata); \
        continue; \
    } while (0);

#define PUSH_STATE(new_state) \
    do { \
        darray_push(data->state_stack, &YYSTATE); \
        BEGIN(new_state); \
    } while (0);

#define POP_STATE() \
    do { \
        darray_pop(data->state_stack, &YYSTATE); \
    } while (0);

/*
#   define YYCTYPE        char
#   define YYPEEK()       *cursor
#   define YYSKIP()       ++cursor
#   define YYBACKUP()     marker = cursor
#   define YYBACKUPCTX()  ctxmarker = cursor
#   define YYRESTORE()    cursor = marker
#   define YYRESTORECTX() cursor = ctxmarker
#   define YYLESSTHAN(n)  limit - cursor < n
#   define YYFILL(n)      {}
*/

enum {
    NONE,
    CLASS,
    FUNCTION,
    NAMESPACE
};

/**
 * Different positionning stuffs for tokenization
 */
struct LexerInput {
    /**
     * Current position
     */
    YYCTYPE *cursor;
    /**
     * End of string
     */
    YYCTYPE */* const*/ limit;
    /**
     * For internal use by re2c (its "marker")
     */
    YYCTYPE *marker;
//     size_t yyleng;
    /**
     * Begin of current token
     */
    YYCTYPE *yytext;
//     size_t lineno;
//     int bol;
};

# define LEXER_UNWRAP(optval) \
    (NULL == OPT_LEXUWF(optval) ? (Lexer *) OPT_LEXPTR(optval) : (OPT_LEXUWF(optval)(OPT_LEXPTR(optval))))

typedef enum {
    /**
     * Emitted when parsing is finished (ie YYLIMIT is reached)
     *
     * Parameters expected by the event callback: none
     */
    EVENT_DONE,
    /**
     * Emitted to step down current parsing to a sublexer
     *
     * Parameters expected by the event callback:
     * - const LexerImplementation * : the lexer implementation to use from this point (YYCURSOR)
     * - LexerData * or NULL : the lexer data to use or NULL to create one with default values
     */
    EVENT_PUSH,
    /**
     * Emitted to step down current parsing to a sublexer
     *
     * Parameters expected by the event callback:
     * - int : the type of the encountered token
     */
    EVENT_TOKEN,
    /**
     * Emitted to replay a token with a sublexer.
     *
     * Difference with EVENT_PUSH:
     * - EVENT_PUSH: this is the sublexer which decides when to return the control. It is intended for
     * cases when current/top/parent lexer is not able to determine where the parsing for this sublexer
     * should end.
     * - EVENT_REPLAY: in the oppposite, with a "replay", the current lexer know exactly which part
     * of the actual input it has to handle (from a start point - usually YYTEXT - to a specific end
     * point - which may be not YYCURSOR)
     *
     * Parameters expected by the event callback:
     * - YYCTYPE * : the start point of the token to replay (becomes again YYCURSOR for the sublexer)
     * - YYCTYPE * : the end point of the token to replay (becomes YYLIMIT for the sublexer)
     * - const LexerImplementation * : the lexer implementation to use from this point (YYCURSOR)
     * - LexerData * or NULL : the lexer data to use or NULL to create one with default values
     */
    EVENT_REPLAY,
    /**
     * Emitted when a newline is encountered
     *
     * Parameters expected by the event callback: none
     */
    EVENT_NEWLINE
} event_t;

typedef void envent_cb_t(event_t, void *, ...);

/**
 * Common data of any lexer
 *
 * @note each member have to be ALIGNED(sizeof(OptionValue)),
 * this is a shortcut to union with an OptionValue and let us
 * to have a direct access to the final value.
 */
typedef struct {
    /**
     * Lexer internal flags
     */
    uint16_t flags ALIGNED(sizeof(OptionValue));
    /**
     * Lexer's state/condition
     */
    int state ALIGNED(sizeof(OptionValue));
    /**
     * States stack
     */
    DArray *state_stack ALIGNED(sizeof(OptionValue));
    /**
     * Mark next token kind
     */
    int next_label ALIGNED(sizeof(OptionValue));
} LexerData;

/**
 * Lexer option
 */
typedef struct {
    /**
     * Option's name
     */
    const char *name;
    /**
     * Its type, one of OPT_TYPE_* constants
     */
    OptionType type;
    /**
     * Offset of the member into the struct that override LexerData
     */
    size_t offset;
//     int (*accept)(?);
    /**
     * Its default value
     */
    OptionValue defval;
    /**
     * A string for self documentation
     */
    const char *docstr;
} LexerOption;

# define END_OF_LEXER_OPTIONS \
    { NULL, 0, 0, OPT_DEF_INT(0), NULL }

typedef struct LexerInput LexerInput;

/**
 * Define a lexer (acts as a class)
 */
struct LexerImplementation {
    /**
     * Official name of the lexer.
     * Should start with an uppercase letter and include only ASCII alphanumeric characters plus underscores
     */
    const char *name;
    /**
     * Flags
     */
    uint16_t flags;
    /**
     * A string for self documentation
     */
    const char *docstr;
    /**
     * Optionnal (may be NULL) array of additionnal names 
     */
    const char * const *aliases;
    /**
     * Optionnal (may be NULL) array of glob patterns to associate a lexer
     * to different file extensions.
     * Example: "*.rb", "*.rake", ... for Ruby
     */
    const char * const *patterns;
    /**
     * Optionnal (may be NULL) array of MIME types appliable.
     * Eg: text/html for HTML.
     */
    const char * const *mimetypes;
    /**
     * Optionnal (may be NULL) array of (glob) patterns to recognize interpreter.
     * Not fullpaths, only basenames.
     * Example: "php", "php-cli", "php[45]" for PHP
     */
    const char * const *interpreters;
    /**
     * Optionnal (may be NULL) callback for additionnal initialization that can be
     * done elsewhere
     */
    void (*init)(LexerData *);
    /**
     * Optionnal (may be NULL) callback to find out a suitable lexer
     * for an input string. Higher is the returned value more accurate
     * is the lexer
     */
    int (*analyse)(const char *, size_t);
    /**
     * Callback for lexing an input string into tokens
     */
    int (*yylex)(YYLEX_ARGS);
    /**
     * Size to allocate to create a Lexer
     * default is `sizeof(LexerData)`
     */
    size_t data_size;
    /**
     * Available options
     */
    /*const*/ LexerOption /*const */*options;
    /**
     * Implicit dependencies to other lexers
     */
    const struct LexerImplementation * const *implicit;
};

/**
 * Use a lexer (acts as an instance)
 */
struct Lexer {
    /**
     * Implementation on which depends the lexer (like its "class")
     */
    const LexerImplementation *imp;
    /**
     * (Variable) Space to store current values of options
     */
    OptionValue optvals[];
};

void reset_lexer(LexerData *);
bool lexer_data_init(LexerData *, size_t);
void lexer_data_destroy(LexerData *);

#define YYSTRNCMP(x) \
    strcmp_l(x, STR_LEN(x), (char *) YYTEXT, YYLENG/*, STR_LEN(x)*/)

#define YYSTRNCASECMP(x) \
    ascii_strcasecmp_l(x, STR_LEN(x), (char *) YYTEXT, YYLENG/*, STR_LEN(x)*/)

#define NE(s) { s, STR_LEN(s) }

typedef struct {
    const char *name;
    size_t name_len;
} named_element_t;

typedef struct {
    named_element_t ne;
    int type;
} typed_named_element_t;

int named_elements_cmp(const void *a, const void *b);
int named_elements_casecmp(const void *a, const void *b);

#endif /* !LEXER_H */