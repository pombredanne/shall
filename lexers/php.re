/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2015 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Marcus Boerger <helly@php.net>                              |
   |          Nuno Lopes <nlopess@php.net>                                |
   |          Scott MacVicar <scottmac@php.net>                           |
   | Flex version authors:                                                |
   |          Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

#include <stddef.h> /* offsetof */
#include <stdlib.h>
#include <string.h>

#include "cpp.h"
#include "utils.h"
#include "tokens.h"
#include "lexer.h"
#include "lexer-private.h"
#include "darray.h"

typedef struct {
    LexerData data;
    int short_tags ALIGNED(sizeof(OptionValue));
    int asp_tags ALIGNED(sizeof(OptionValue));
    OptionValue secondary ALIGNED(sizeof(OptionValue));
    int in_namespace;
    char *doclabel; // (?:now|here)doc label // TODO: may leak
    size_t doclabel_len;
    DArray state_stack;
} PHPLexerData;

static void phpinit(LexerData *data)
{
    PHPLexerData *mydata;

    mydata = (PHPLexerData *) data;
    darray_init(&mydata->state_stack, 0, sizeof(data->state));
}

static int phpanalyse(const char *src, size_t src_len)
{
    if (src_len >= STR_LEN("<?XXX") && (0 == memcmp(src, "<?php", STR_LEN("<?php")) || (0 == memcmp(src, "<?", STR_LEN("<?")) && 0 != memcmp(src, "<?xml", STR_LEN("<?xml"))))) {
        return 999;
    }

    return 0;
}

enum {
    STATE(INITIAL),         // should be 1st as &data.state is interfaced as the (boolean) start_inline option
    STATE(ST_IN_SCRIPTING), // should be 2nd for the same reason
    STATE(ST_COMMENT_MULTI),
    STATE(ST_BACKQUOTE),
    STATE(ST_SINGLE_QUOTES),
    STATE(ST_DOUBLE_QUOTES),
    STATE(ST_NOWDOC),
    STATE(ST_HEREDOC),
    STATE(ST_END_NOWDOC),
    STATE(ST_END_HEREDOC),
    STATE(ST_VAR_OFFSET),
    STATE(ST_LOOKING_FOR_VARNAME),
    STATE(ST_LOOKING_FOR_PROPERTY)
};

static int default_token_type[] = {
    IGNORABLE, // INITIAL
    IGNORABLE, // ST_IN_SCRIPTING
    COMMENT_MULTILINE, // ST_COMMENT_MULTI
    STRING_BACKTICK, // ST_BACKQUOTE
    STRING_SINGLE, // ST_SINGLE_QUOTES
    STRING_DOUBLE, // ST_DOUBLE_QUOTES
    STRING_SINGLE, // ST_NOWDOC
    STRING_DOUBLE, // ST_HEREDOC
    STRING_SINGLE, // ST_END_NOWDOC
    STRING_DOUBLE, // ST_END_HEREDOC
    IGNORABLE, // ST_VAR_OFFSET
    IGNORABLE, // ST_LOOKING_FOR_VARNAME
    IGNORABLE  // ST_LOOKING_FOR_PROPERTY
};

// http://lxr.php.net/xref/phpng/Zend/zend_language_scanner.l

#define DECL_OP(s) \
    { OPERATOR, s, STR_LEN(s) },
#define DECL_KW(s) \
    { KEYWORD, s, STR_LEN(s) },
#define DECL_KW_NS(s) \
    { KEYWORD_NAMESPACE, s, STR_LEN(s) },
#define DECL_KW_DECL(s) \
    { KEYWORD_DECLARATION, s, STR_LEN(s) },
#define DECL_KW_CONSTANT(s) \
    { KEYWORD_CONSTANT, s, STR_LEN(s) },
#define DECL_NAME_B(s) \
    { NAME_BUILTIN, s, STR_LEN(s) },
#define DECL_NAME_BP(s) \
    { NAME_BUILTIN_PSEUDO, s, STR_LEN(s) },

// https://gcc.gnu.org/onlinedocs/gcc/Designated-Inits.html#Designated-Inits
static int case_insentive[_LAST_TOKEN] = {
    [ OPERATOR ] = 1,
    [ KEYWORD ] = 1,
    [ KEYWORD_CONSTANT ] = 1,
    [ KEYWORD_DECLARATION ] = 1,
    [ KEYWORD_NAMESPACE ] = 1,
    [ NAME_BUILTIN ] = 1,
    [ NAME_BUILTIN_PSEUDO ] = 1,
};

// stdClass, __PHP_Incomplete_Class

static struct {
    int type;
    const char *name;
    size_t name_len;
} keywords[] = {
    //
    DECL_OP("and")
    DECL_OP("or")
    DECL_OP("xor")
    //
    DECL_KW_CONSTANT("NULL")
    DECL_KW_CONSTANT("TRUE")
    DECL_KW_CONSTANT("FALSE")
    // http://php.net/manual/fr/reserved.constants.php
    // ...
    //
    DECL_NAME_BP("this")
    DECL_NAME_BP("self")
    DECL_NAME_BP("parent")
    DECL_NAME_BP("__CLASS__")
    DECL_NAME_BP("__COMPILER_HALT_OFFSET__")
    DECL_NAME_BP("__DIR__")
    DECL_NAME_BP("__FILE__")
    DECL_NAME_BP("__FUNCTION__")
    DECL_NAME_BP("__LINE__")
    DECL_NAME_BP("__METHOD__")
    DECL_NAME_BP("__NAMESPACE__")
    DECL_NAME_BP("__TRAIT__")
    //
    DECL_NAME_B("__halt_compiler")
    DECL_NAME_B("echo")
    DECL_NAME_B("empty")
    DECL_NAME_B("eval")
    DECL_NAME_B("exit")
    DECL_NAME_B("die")
    DECL_NAME_B("include")
    DECL_NAME_B("include_once")
    DECL_NAME_B("isset")
    DECL_NAME_B("list")
    DECL_NAME_B("print")
    DECL_NAME_B("require")
    DECL_NAME_B("require_once")
    DECL_NAME_B("unset")
    //
//     DECL_KW_NS("namespace")
//     DECL_KW("use") // is used for namespaces and by closures
    //
    DECL_KW_DECL("var")
    DECL_KW("abstract")
    DECL_KW("final")
    DECL_KW("private")
    DECL_KW("protected")
    DECL_KW("public")
    DECL_KW("static")
    DECL_KW("extends")
    DECL_KW("implements")
    //
    DECL_KW("callable")
    DECL_KW("insteadof")
    DECL_KW("new")
    DECL_KW("clone")
    DECL_KW("class")
    DECL_KW("trait")
    DECL_KW("interface")
    DECL_KW("function")
    DECL_KW("global")
    DECL_KW("const")
    DECL_KW("return")
    DECL_KW("yield")
    DECL_KW("try")
    DECL_KW("catch")
    DECL_KW("finally")
    DECL_KW("throw")
    DECL_KW("if")
    DECL_KW("else")
    DECL_KW("elseif")
    DECL_KW("endif")
    DECL_KW("while")
    DECL_KW("endwhile")
    DECL_KW("do")
    DECL_KW("for")
    DECL_KW("endfor")
    DECL_KW("foreach")
    DECL_KW("endforeach")
    DECL_KW("declare")
    DECL_KW("enddeclare")
    DECL_KW("instanceof")
    DECL_KW("as")
    DECL_KW("switch")
    DECL_KW("endswitch")
    DECL_KW("case")
    DECL_KW("default")
    DECL_KW("break")
    DECL_KW("continue")
    DECL_KW("goto")
};

#if 0 /* UNUSED YET */
static named_element_t functions[] = {
    NE("urlencode"),
    NE("array_key_exists"),
};

static named_element_t classes[] = {
    NE("Directory"),
//     NE("stdClass"),
//     NE("__PHP_Incomplete_Class"),
};
#endif

#if 0
static void phpclose(LexerData *data)
{
    PHPLexerData *mydata;

    mydata = (PHPLexerData *) data;
    darray_destroy(&mydata->state_stack);
}
#endif

/**
 * NOTE:
 * - ' = case insensitive (ASCII letters only)
 * - " = case sensitive
 * (for re2c, by default, without --case-inverted or --case-insensitive)
 **/

/**
 * TODO:
 * - :: et ->
 * - array reconnue comme une fonction ?
 * - tout ce qui est interpolé
 * - dimension de tableau (machin[bidule])
 **/

#define IS_LABEL_START(c) \
    (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_' || (c) >= 0x7F)

static int phplex(YYLEX_ARGS) {
    PHPLexerData *mydata;

    mydata = (PHPLexerData *) data;
restart:
    YYTEXT = YYCURSOR;
// yymore_restart:
/*!re2c
re2c:yyfill:check = 0;

LNUM = [0-9]+;
DNUM = ([0-9]*"."[0-9]+)|([0-9]+"."[0-9]*);
EXPONENT_DNUM = ((LNUM|DNUM)[eE][+-]?LNUM);
HNUM = "0" 'x' [0-9a-fA-F]+;
BNUM = "0" 'b' [01]+;
LABEL =  [a-zA-Z_\x7f-\xff][a-zA-Z0-9_\x7f-\xff]*;
NAMESPACED_LABEL = LABEL? ("\\" LABEL)+;
WHITESPACE = [ \n\r\t]+;
TABS_AND_SPACES = [ \t]*;
TOKENS = [;:,.\[\]()|^&+-/*=%!~$<>?@];
ANY_CHAR = [^];
NEWLINE = ("\r"|"\n"|"\r\n");

<INITIAL>'<?php' ([ \t]|NEWLINE) {
    //yy->yyleng = STR_LEN("<?php");
    yyless(STR_LEN("<?php"));
    BEGIN(ST_IN_SCRIPTING);
    return NAME_TAG;
}

<INITIAL>"<?=" {
    // NOTE: <?= does not depend on short_open_tag since PHP 5.4.0
    BEGIN(ST_IN_SCRIPTING);
    return NAME_TAG;
}

<INITIAL>"<?" {
    if (mydata->short_tags) {
        BEGIN(ST_IN_SCRIPTING);
        return NAME_TAG;
    } else {
        goto not_php; // if short_open_tag is off, give it to sublexer (if any)
    }
}

<INITIAL>"<%" {
    if (mydata->asp_tags) {
        BEGIN(ST_IN_SCRIPTING);
        return NAME_TAG;
    } else {
        goto not_php; // if asp_tags is off, give it to sublexer (if any)
    }
}

<INITIAL>'<script'WHITESPACE+'language'WHITESPACE*"="WHITESPACE*('php'|'"php"'|'\'php\'')WHITESPACE*">" {
    BEGIN(ST_IN_SCRIPTING);
    return NAME_TAG;
}

<ST_IN_SCRIPTING> "=>" {
    return OPERATOR;
}

<ST_IN_SCRIPTING>"->" {
    PUSH_STATE(ST_LOOKING_FOR_PROPERTY);
    return OPERATOR;
}

<ST_LOOKING_FOR_PROPERTY>"->" {
    return OPERATOR;
}

<ST_LOOKING_FOR_PROPERTY>LABEL {
    POP_STATE();
    return NAME_VARIABLE_INSTANCE;
}

<ST_LOOKING_FOR_PROPERTY>ANY_CHAR {
    yyless(0);
    POP_STATE();
    goto restart;
}

<ST_IN_SCRIPTING> "++" | "--" | [!=]"==" | "<>" | [-+*/%.<>+&|^!=]"=" | ">>=" | "<<=" | "**=" | "<<" | ">>" | "**" | [-+.*/%=^&|!~<>?:@] {
    return OPERATOR;
}

<ST_IN_SCRIPTING>"#" | "//" {
    while (YYCURSOR < YYLIMIT) {
        switch (*YYCURSOR++) {
            case '\r':
                if ('\n' == *YYCURSOR) {
                    YYCURSOR++;
                }
                /* fall through */
            case '\n':
                break;
            case '%':
                if (!mydata->asp_tags) {
                    continue;
                }
                /* fall through */
            case '?':
                if ('>' == *YYCURSOR) {
                    YYCURSOR--;
                    break;
                }
                /* fall through */
            default:
                continue;
        }
        break;
    }
    return COMMENT_SINGLE;
}

/*
<ST_IN_SCRIPTING>"(" TABS_AND_SPACES ('int' | 'integer' | 'bool' | 'boolean' | 'string' | 'binary' | 'real' | 'float' | 'double' | 'array' | 'object' | 'unset') TABS_AND_SPACES ")" {
    return OPERATOR;
}
*/

<ST_IN_SCRIPTING>"{" {
    PUSH_STATE(ST_IN_SCRIPTING);
    return PUNCTUATION;
}

<ST_IN_SCRIPTING>"}" {
//     if (darray_length(&mydata->state_stack)) {
        POP_STATE();
//     }
    if (STATE(ST_IN_SCRIPTING) == YYSTATE) {
        return PUNCTUATION;
    } else {
        return SEQUENCE_INTERPOLATED;
    }
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC>"${" {
    PUSH_STATE(ST_LOOKING_FOR_VARNAME);
    return SEQUENCE_INTERPOLATED; // TODO
}

<ST_DOUBLE_QUOTES,ST_BACKQUOTE,ST_HEREDOC>"{$" {
    PUSH_STATE(ST_IN_SCRIPTING);
    yyless(1);
    return SEQUENCE_INTERPOLATED; // token is shorten to '{'
}

<ST_LOOKING_FOR_VARNAME>LABEL [[}] {
    yyless(YYLENG - 1);
    POP_STATE();
    PUSH_STATE(ST_IN_SCRIPTING);
    return NAME_VARIABLE;
}

<ST_LOOKING_FOR_VARNAME>ANY_CHAR {
    yyless(0);
    POP_STATE();
    PUSH_STATE(ST_IN_SCRIPTING);
    goto restart;
}

<ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE>"$" LABEL "->" [a-zA-Z_\x7f-\xff] {
    yyless(YYLENG - 3);
    PUSH_STATE(ST_LOOKING_FOR_PROPERTY);
    return NAME_VARIABLE;
}

<ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE>"$" LABEL "[" {
    yyless(YYLENG - 1);
    PUSH_STATE(ST_VAR_OFFSET);
    return NAME_VARIABLE;
}

<ST_IN_SCRIPTING,ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE,ST_VAR_OFFSET>"$" LABEL {
    return NAME_VARIABLE;
}

<ST_VAR_OFFSET>"]" {
    POP_STATE();
    return PUNCTUATION;
}

<ST_VAR_OFFSET>"[" {
    return PUNCTUATION;
}

<ST_VAR_OFFSET>TOKENS | [{}"`] {
debug("[ERR] %d", __LINE__);
    /* Only '[' can be valid, but returning other tokens will allow a more explicit parse error */
    return IGNORABLE;
}

<ST_VAR_OFFSET>[ \n\r\t\\'#] {
debug("[ERR] %d", __LINE__);
    /* Invalid rule to return a more explicit parse error with proper line number */
    yyless(0);
    POP_STATE();
    return IGNORABLE;
}

<ST_VAR_OFFSET>LABEL {
    return NAME_VARIABLE; // TODO: it's a constant
}

<ST_IN_SCRIPTING>[,;()] {
    return PUNCTUATION;
}

<ST_IN_SCRIPTING,ST_LOOKING_FOR_PROPERTY>WHITESPACE+ {
    return IGNORABLE;
}

<ST_IN_SCRIPTING>"/*" | "/**" WHITESPACE {
    BEGIN(ST_COMMENT_MULTI);
    return COMMENT_MULTILINE;
}

<ST_COMMENT_MULTI>"*/" {
    BEGIN(INITIAL);
    return COMMENT_MULTILINE;
}

<ST_IN_SCRIPTING> 'new' {
    data->next_label = CLASS;
    return KEYWORD;
}

/*
<ST_IN_SCRIPTING> 'class' | 'interface' {
    data->next_label = CLASS;
    return KEYWORD;
}

<ST_IN_SCRIPTING> 'function' {
    data->next_label = FUNCTION;
    return KEYWORD;
}
*/

/*
<ST_IN_SCRIPTING> LABEL WHITESPACE* '(' {
    size_t i;

    // TODO: rempiler les ( + espaces
    // TODO: marquer si on a vu -> ou :: (on ne chercherait plus une fonction mais une méthode)
    for (i = 0; i < ARRAY_SIZE(functions); i++) {
        if (0 == ascii_strcasecmp_l(functions[i].name, functions[i].name_len, YYTEXT, YYLENG)) {
            return NAME_FUNCTION;
        }
    }

    return IGNORABLE;
//     return NAME_FUNCTION;
}
*/

// TODO: handle '{' ... '}'
<ST_IN_SCRIPTING> 'namespace' {
    data->next_label = NAMESPACE;
    mydata->in_namespace = 1;
    return KEYWORD_NAMESPACE;
}

// TODO: at BOL "use" is related to namespace else to closure
<ST_IN_SCRIPTING> 'use' {
    data->next_label = NAMESPACE;
    return KEYWORD_NAMESPACE;
}

<ST_IN_SCRIPTING> LABEL | NAMESPACED_LABEL {
    int type;

    type = data->next_label;
    data->next_label = 0;
    switch (type) {
        case NAMESPACE:
        {
            return NAME_NAMESPACE;
        }
        case FUNCTION:
        {
            return NAME_FUNCTION;
        }
        case CLASS:
        {
#if 0
            // TODO: prendre en compte le namespace
            for (i = 0; i < ARRAY_SIZE(classes); i++) {
                if (0 == ascii_strcasecmp_l(classes[i].name, classes[i].name_len, YYTEXT, YYLENG)) {
                    // link it
                }
            }
#endif
            return NAME_CLASS;
        }
        default:
        {
            size_t i;

            for (i = 0; i < ARRAY_SIZE(keywords); i++) {
                if (0 == ascii_strcasecmp_l(keywords[i].name, keywords[i].name_len, (char *) YYTEXT, YYLENG)) {
                    if (!case_insentive[keywords[i].type] && 0 != strcmp_l(keywords[i].name, keywords[i].name_len, (char *) YYTEXT, YYLENG)) {
                        break;
                    } else {
                        return keywords[i].type;
                    }
                }
            }
#if 0
            // il faudrait pouvoir regarder en avant pour distinguer fonction / constante / classe
            // par contre, si on marque le passage d'opérateurs objet (:: et ->) on devrait savoir que c'est un nom de méthode
            for (i = 0; i < ARRAY_SIZE(functions); i++) {
                if (0 == ascii_strcasecmp_l(functions[i].name, functions[i].name_len, YYTEXT, YYLENG)) {
                    return NAME_FUNCTION;
                }
            }
#endif
            return IGNORABLE;
        }
    }
}

<ST_IN_SCRIPTING>LNUM {
    if ('0' == *YYTEXT) {
        return NUMBER_OCTAL;
    } else {
        return NUMBER_DECIMAL;
    }
}

<ST_IN_SCRIPTING>HNUM {
    return NUMBER_HEXADECIMAL;
}

<ST_IN_SCRIPTING>BNUM {
    return NUMBER_BINARY;
}

<ST_IN_SCRIPTING>DNUM | EXPONENT_DNUM {
    return NUMBER_FLOAT;
}

<ST_IN_SCRIPTING>("?>" | '</script' WHITESPACE* ">") NEWLINE? {
    BEGIN(INITIAL);
    return NAME_TAG;
}

<ST_IN_SCRIPTING>"%>"NEWLINE? {
    if (mydata->asp_tags) {
        BEGIN(INITIAL);
        return NAME_TAG;
    } else {
        //yyless(1);
        return IGNORABLE;
    }
}

<ST_IN_SCRIPTING>"`" {
    BEGIN(ST_BACKQUOTE);
    return STRING_BACKTICK;
}

<ST_IN_SCRIPTING>"b"? "'" {
    BEGIN(ST_SINGLE_QUOTES);
    return STRING_SINGLE;
}

<ST_IN_SCRIPTING>"b"? "\"" {
    BEGIN(ST_DOUBLE_QUOTES);
    return STRING_DOUBLE;
}

// TODO: split "b"? "<<<" [ \t]* (["']?) LABEL \1 NEWLINE into separate tokens ?
<ST_IN_SCRIPTING>"b"? "<<<" TABS_AND_SPACES (LABEL | (["] LABEL ["]) | ((['] LABEL [']))) NEWLINE {
    YYCTYPE *p;
    int bprefix, quoted;

    quoted = 0;
    bprefix = '<' != *YYTEXT;
//     if (NULL != mydata->doclabel) {
//         free(mydata->doclabel);
//     }
    for (p = YYTEXT + bprefix + STR_LEN("<<<"); '\t' == *p || ' ' == *p; p++)
        ;
    switch (*p) {
        case '\'':
            ++p;
            quoted = 1;
            BEGIN(ST_NOWDOC);
            break;
        case '"':
            ++p;
            quoted = 1;
            /* no break */
        default:
            BEGIN(ST_HEREDOC);
            break;
    }
    mydata->doclabel_len = YYCURSOR - p - quoted - STR_LEN("\n") - ('\r' == YYCURSOR[-2]);
    mydata->doclabel = strndup((char *) p, mydata->doclabel_len);
    // optimisation for empty string, avoid an useless pass into <ST_HEREDOC,ST_NOWDOC>ANY_CHAR ?
    if (mydata->doclabel_len < YYLIMIT - YYCURSOR && !memcmp(YYCURSOR, p, mydata->doclabel_len)) {
        YYCTYPE *end = YYCURSOR + mydata->doclabel_len;

        if (*end == ';') {
            end++;
        }

        if ('\n' == *end || '\r' == *end) {
            if (STATE(ST_NOWDOC) == YYSTATE) {
                BEGIN(ST_END_NOWDOC);
            } else {
                BEGIN(ST_END_HEREDOC);
            }
        }
    }

    return default_token_type[YYSTATE];
}

<ST_HEREDOC,ST_NOWDOC>ANY_CHAR {
    YYCTYPE c;

    if (YYCURSOR > YYLIMIT) {
        return 0;
    }
    YYCURSOR--;
    while (YYCURSOR < YYLIMIT) {
        switch (c = *YYCURSOR++) {
            case '\r':
                if (*YYCURSOR == '\n') {
                    YYCURSOR++;
                }
                /* fall through */
            case '\n':
                /* Check for ending label on the next line */
                if (IS_LABEL_START(*YYCURSOR) && mydata->doclabel_len < YYLIMIT - YYCURSOR && !memcmp(YYCURSOR, mydata->doclabel, mydata->doclabel_len)) {
                    YYCTYPE *end = YYCURSOR + mydata->doclabel_len;

                    if (*end == ';') {
                        end++;
                    }

                    if ('\n' == *end || '\r' == *end) {
                        if (STATE(ST_NOWDOC) == YYSTATE) {
                            BEGIN(ST_END_NOWDOC);
                        } else {
                            BEGIN(ST_END_HEREDOC);
                        }
//                         goto nowdoc_scan_done;
                        return default_token_type[YYSTATE];
                    }
                }
                if (STATE(ST_HEREDOC) == YYSTATE) {
                    continue;
                }
                /* fall through */
            case '$':
                if (STATE(ST_HEREDOC) == YYSTATE && (IS_LABEL_START(*YYCURSOR) || '{' == *YYCURSOR)) {
                    break;
                }
                continue;
            case '{':
                if (STATE(ST_HEREDOC) == YYSTATE && *YYCURSOR == '$') {
                    break;
                }
                continue;
            case '\\':
                if (STATE(ST_HEREDOC) == YYSTATE && YYCURSOR < YYLIMIT && '\n' != *YYCURSOR && '\r' != *YYCURSOR) {
                    YYCURSOR++;
                }
                /* fall through */
            default:
                continue;
        }
        if (STATE(ST_HEREDOC) == YYSTATE) {
            YYCURSOR--;
            break;
        }
    }

// nowdoc_scan_done:
    return default_token_type[YYSTATE];
}

<ST_END_HEREDOC,ST_END_NOWDOC>ANY_CHAR {
    int old_state;

    old_state = YYSTATE;
    YYCURSOR += mydata->doclabel_len - 1;
    free(mydata->doclabel);
    mydata->doclabel = NULL;
    mydata->doclabel_len = 0;

    BEGIN(ST_IN_SCRIPTING);
    return default_token_type[old_state];
}

<ST_DOUBLE_QUOTES>("\\0"[0-9]{2}) | ("\\" 'x' [0-9A-Fa-f]{2}) | ("\\"[$"efrntv\\]) {
    return ESCAPED_CHAR;
}

<ST_DOUBLE_QUOTES>"\"" {
    BEGIN(ST_IN_SCRIPTING);
    return STRING_DOUBLE;
}

<ST_IN_SCRIPTING> "$" LABEL {
    return NAME_VARIABLE;
}

<ST_BACKQUOTE>"\\" [\\`] {
    return ESCAPED_CHAR;
}

<ST_BACKQUOTE>"`" {
    BEGIN(ST_IN_SCRIPTING);
    return STRING_BACKTICK;
}

/*<ST_BACKQUOTE>ANY_CHAR {
    return STRING_BACKTICK;
}

<ST_DOUBLE_QUOTES>ANY_CHAR {
    return STRING_DOUBLE;
}*/

<ST_SINGLE_QUOTES>"\\" [\\'] {
    return ESCAPED_CHAR;
}

<ST_SINGLE_QUOTES>"'" {
    BEGIN(ST_IN_SCRIPTING);
    return STRING_SINGLE;
}

/*<ST_SINGLE_QUOTES>ANY_CHAR {
    return STRING_SINGLE;
}*/

/*<ST_IN_SCRIPTING>ANY_CHAR {
    return IGNORABLE;
}*/

<INITIAL>ANY_CHAR {
    Lexer *secondary;

not_php:
    secondary = LEXER_UNWRAP(mydata->secondary);
    if (NULL == secondary) {
        return IGNORABLE;
    } else {
        YYCURSOR = YYTEXT;

        return secondary->imp->yylex(yy, (LexerData *) secondary->optvals);
#if 0
    extern const LexerImplementation xml_lexer;
//     extern const LexerImplementation json_lexer;
    static LexerData data = {0};

    YYCURSOR = YYTEXT;
//     memset(&data, 0, sizeof(data));
    return xml_lexer.yylex(yy, &data);
//     return json_lexer.yylex(yy, &data);
#endif
    }
}

<*>ANY_CHAR { // should be the last "rule"
    return default_token_type[YYSTATE];
}
*/
}

LexerImplementation php_lexer = {
    "PHP",
    "For PHP source code",
    (const char * const []) { "php3", "php4", "php5", NULL },
    (const char * const []) { "*.php", "*.php[345]", "*.inc", NULL },
    (const char * const []) { "text/x-php", "application/x-httpd-php", NULL },
    (const char * const []) { "php", "php-cli", "php5*", NULL },
    phpinit,
    phpanalyse,
    phplex,
    sizeof(PHPLexerData),
    (/*const*/ LexerOption /*const*/ []) {
        { "start_inline", OPT_TYPE_BOOL, offsetof(LexerData, state), OPT_DEF_BOOL(0), "if true the lexer starts highlighting with php code (ie no starting `<?php`/`<?`/`<script language=\"php\">` is required at top)" },
        { "asp_tags", OPT_TYPE_BOOL, offsetof(PHPLexerData, asp_tags), OPT_DEF_BOOL(0), "support, or not, `<%`/`%>` tags to begin/end PHP code ([asp_tags](http://php.net/asp_tags))" },
        { "short_tags", OPT_TYPE_BOOL, offsetof(PHPLexerData, short_tags), OPT_DEF_BOOL(1), "support, or not, `<?` tags to begin PHP code ([short_open_tag](http://php.net/short_open_tag))" },
        { "secondary", OPT_TYPE_LEXER, offsetof(PHPLexerData, secondary), OPT_DEF_LEXER, "Lexer to highlight content outside of PHP tags (if none, these parts will not be highlighted)" },
        END_OF_LEXER_OPTIONS
    }
};
