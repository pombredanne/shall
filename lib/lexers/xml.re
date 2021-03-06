#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"
#include "utils.h"

extern const LexerImplementation css_lexer;
extern const LexerImplementation dtd_lexer;
extern const LexerImplementation js_lexer;

typedef struct {
    LexerData data;
    bool html;
    bool css;
    bool js;
} XMLLexerData;

enum {
    STATE(INITIAL),
    STATE(IN_TAG),
    STATE(IN_PREPROC),
    STATE(IN_COMMENT),
    STATE(IN_CDATA),
    STATE(IN_ATTRIBUTE),
    STATE(IN_SINGLE_QUOTES),
    STATE(IN_DOUBLE_QUOTES),
};

static void htmlinit(const OptionValue *UNUSED(options), LexerData *data, void *UNUSED(ctxt))
{
    XMLLexerData *mydata;

    BEGIN(INITIAL);
    mydata = (XMLLexerData *) data;
    mydata->html = true;
}

static int xmlanalyse(const char *src, size_t src_len)
{
    if (src_len >= STR_LEN("<?xml") && 0 == memcmp(src, "<?xml", STR_LEN("<?xml"))) {
        return 999;
    }
    if (src_len >= STR_LEN("<!DOCTYPE") && 0 == memcmp(src, "<!DOCTYPE", STR_LEN("<!DOCTYPE"))) {
        return 300;
    }

    return 0;
}

static int default_token_type[] = {
    IGNORABLE,         // INITIAL
    IGNORABLE,         // IN_TAG
    IGNORABLE,         // IN_PREPROC
    COMMENT_MULTILINE, // IN_COMMENT
    IGNORABLE,         // IN_CDATA
    IGNORABLE,         // IN_ATTRIBUTE
    STRING_SINGLE,     // IN_SINGLE_QUOTES
    STRING_SINGLE,     // IN_DOUBLE_QUOTES
};

/**
 * NOTE:
 * - ' = case insensitive (ASCII letters only)
 * - " = case sensitive
 * (for re2c, by default, without --case-inverted or --case-insensitive)
 **/
static int xmllex(YYLEX_ARGS) {
    XMLLexerData *mydata;

    (void) options;
    mydata = (XMLLexerData *) data;
    while (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

// as defined by http://www.w3.org/TR/REC-xml/#charsets

// #xD = \r
// #xA = \n

// Productions 33 through 38 have been removed

S = [ \n\r\t]+; // [2]
Char = [\n\r\t] | [ \uD7FF] | [\uE000-\uFFFD] | [\U00010000-\U0010FFFF]; // [3]
NameStartChar = ':' | [A-Z] | '_' | [a-z] | [\xC0-\xD6] | [\xD8-\xF6] | [\u00F8-\u02FF] | [\u0370-\u037D] | [\u037F-\u1FFF] | [\u200C-\u200D] | [\u2070-\u218F] | [\u2C00-\u2FEF] | [\u3001-\uD7FF] | [\uF900-\uFDCF] | [\uFDF0-\uFFFD] | [\U00010000-\U000EFFFF]; // [4]
//PubidChar = [ \r\n] | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%]; // [13]
PubidCharMinusSingleQuote = [ \r\n] | [a-zA-Z0-9] | [-()+,./:=?;!*#@$_%]; // redefinition of [13]
//CharData = [^<&]* - ([^<&]* ']]>' [^<&]*); // [14]

EncName = [A-Za-z] ([A-Za-z0-9._] | '-')*; // [81]
//Ignore = Char* - (Char* ("<![" | "]]>") Char*); // [65]
NameChar = NameStartChar | '-' | '.' | [0-9] | [\xB7] | [\u0300-\u036F] | [\u203F-\u2040]; // [4a]
Name = NameStartChar NameChar*; // [5]
Names = Name ([ ] Name)*; // [6]
Nmtoken = NameChar+; // [7]
Nmtokens = Nmtoken ([ ] Nmtoken)*; // [8]
CharRef = "&#" [0-9]+ ";" | "&#x" [0-9a-fA-F]+ ";"; // [66]
EntityRef = "&" Name ";"; // [68]
Reference = EntityRef | CharRef; // [67]
PEReference = "%" Name ";"; // [69]
EntityValue = '"' ([^%&"] | PEReference | Reference)* '"' | "'" ([^%&'] | PEReference | Reference)* "'"; // [9]
AttValue = '"' ([^<&"] | Reference)* '"' |  "'" ([^<&'] | Reference)* "'"; // [10]
SystemLiteral = ('"' [^"]* '"') | ("'" [^']* "'"); // [11]
//PubidLiteral = '"' PubidChar* '"' | "'" (PubidChar - "'")* "'"; // [12]
PubidLiteral = '"' (PubidCharMinusSingleQuote | ['])* '"' | "'" PubidCharMinusSingleQuote* "'"; // redefinition of [12]
//Comment = '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'; // [15] (a comment cannot end with "--->")
//PITarget = Name - (('X' | 'x') ('M' | 'm') ('L' | 'l')); // [17] (any Name except - case insensitive - 'xml')
//PI = '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'; // [16]
CDStart = "<![CDATA["; // [19]
//CData = (Char* - (Char* ']]>' Char*)); // [20] (any string which is not "]]>")
CDEnd = ']]>'; // [21]
//CDSect = CDStart CData CDEnd; // [18]
Eq = S? '=' S?; // [25]
VersionNum = '1.' [0-9]+; // [26]
VersionInfo = S "version" Eq ("'" VersionNum "'" | '"' VersionNum '"'); // [24]
//Misc = Comment | PI | S; // [27]
SDDecl = S "standalone" Eq (("'" ("yes" | "no") "'") | ('"' ("yes" | "no") '"')); // [32]
EncodingDecl = S "encoding" Eq ('"' EncName '"' | "'" EncName "'" ); // [80]
XMLDecl = "<?xml" VersionInfo EncodingDecl? SDDecl? S? "?>"; // [23]
//prolog = XMLDecl? Misc* (doctypedecl Misc*)?; // [22]
PublicID = "PUBLIC" S PubidLiteral; // [83]
ExternalID = "SYSTEM" S SystemLiteral | "PUBLIC" S PubidLiteral S SystemLiteral; // [75]
NotationDecl = "<!NOTATION" S Name S (ExternalID | PublicID) S? ">"; // [82]
//contentspec = "EMPTY" | "ANY" | Mixed | children; // [46]
//elementdecl = "<!ELEMENT" S Name S contentspec S? ">"; // [45]
//markupdecl = elementdecl | AttlistDecl | EntityDecl | NotationDecl | PI | Comment; // [29]
DeclSep = PEReference | S; // [28a]
//intSubset = (markupdecl | DeclSep)*; // [28b]
//doctypedecl = "<!DOCTYPE" S Name (S ExternalID)? S? ("[" intSubset "]" S?)? ">"; // [28]

//extSubsetDecl = (markupdecl | conditionalSect | DeclSep)*; // [31]
//includeSect = "<![" S? "INCLUDE" S? "[" extSubsetDecl "]]>"; // [62]
//ignoreSectContents = Ignore ("<![" ignoreSectContents "]]>" Ignore)*; // [64] self circular reference
//ignoreSect = "<![" S? "IGNORE" S? "[" ignoreSectContents* "]]>"; // [63]
//conditionalSect = includeSect | ignoreSect; // [61]
TextDecl = "<?xml" VersionInfo? EncodingDecl S? "?>"; // [77]
//extSubset = TextDecl? extSubsetDecl; // [30]
//document = prolog element Misc*; // [1]

Attribute = Name Eq AttValue; // [41]
STag = "<" Name (S Attribute)* S? ">"; // [40]
ETag = "</" Name S? ">"; // [42]
//content = CharData? ((element | Reference | CDSect | PI | Comment) CharData?)*; // [43]
EmptyElemTag = '<' Name (S Attribute)* S? '/>'; // [44]
//element = EmptyElemTag | STag content ETag; // [39]
//cp = (Name | choice | seq) ("?" | "*" | "+")?; // [48] circular reference between cp and (choice | seq)
//choice = "(" S? cp ( S? "|" S? cp )+ S? ")"; // [49]
//seq = "(" S? cp ( S? "," S? cp )* S? ")"; // [50]
//children = (choice | seq) ("?" | "*" | "+")?; // [47]
Mixed = "(" S? "#PCDATA" (S? "|" S? Name)* S? ")*" | "(" S? "#PCDATA" S? ")"; // [51]
StringType = "CDATA"; // [55]
TokenizedType = "ID" | "IDREF" | "IDREFS" | "ENTITY" | "ENTITIES" | "NMTOKEN" | "NMTOKENS"; // [56]
NotationType = "NOTATION" S "(" S? Name (S? "|" S? Name)* S? ")"; // [58]
Enumeration = "(" S? Nmtoken (S? "|" S? Nmtoken)* S? ")"; // [59]
EnumeratedType = NotationType | Enumeration; // [57]
AttType = StringType | TokenizedType | EnumeratedType; // [54]
DefaultDecl = "#REQUIRED" | "#IMPLIED"; // [60]
AttDef = S Name S AttType S DefaultDecl; // [53]
AttlistDecl = "<!ATTLIST" S Name AttDef* S? ">"; // [52]
NDataDecl = S "NDATA" S Name; // [76]
//EntityDef = EntityValue | (ExternalID NDataDecl?); // [73]
//GEDecl = "<!ENTITY" S Name S EntityDef S? ">"; // [71]
//PEDef = EntityValue | ExternalID; // [74]
//PEDecl = "<!ENTITY" S "%" S Name S PEDef S? ">"; // [72]
//EntityDecl = GEDecl | PEDecl; // [70]
//extParsedEnt = TextDecl? content; // [78]

<INITIAL>'<!--' {
    BEGIN(IN_COMMENT);
    TOKEN(COMMENT_MULTILINE);
}

<IN_COMMENT>[^-]'-->' {
    BEGIN(INITIAL);
    TOKEN(COMMENT_MULTILINE);
}

<INITIAL> CDStart {
    BEGIN(IN_CDATA);
    TOKEN(TAG_PREPROC);
}

<IN_CDATA> CDEnd {
    BEGIN(INITIAL);
    TOKEN(TAG_PREPROC);
}

<INITIAL>"<!DOCTYPE" S Name (S ExternalID)? S? "[" {
    yyless(0);
    prepend_lexer_implementation(ctxt, &dtd_lexer);
    DELEGATE_FULL(IGNORABLE);
}

// "<!DOCTYPE" S Name (S ExternalID)? S? ">"
<INITIAL>"<!DOCTYPE" S {
debug("%d >%.*s<", __LINE__, (int) YYLENG, YYTEXT);
    yyless(STR_LEN("<!DOCTYPE")); // cette ligne fait tout foirer ?
debug("%d >%.*s<", __LINE__, (int) YYLENG, YYTEXT);
    PUSH_STATE(IN_TAG);
    TOKEN(NAME_TAG);
}

<INITIAL>"<!" {
    TOKEN(IGNORABLE);
}

<INITIAL>"<?" Name {
    PUSH_STATE(IN_PREPROC);
    TOKEN(TAG_PREPROC);
}

<INITIAL>"<" Name {
    if (mydata->html) {
        if (0 == YYSTRNCASECMP("<style")) {
            mydata->css = true;
            prepend_lexer_implementation(ctxt, &css_lexer);
        } else if (0 == YYSTRNCASECMP("<script")) {
            mydata->js = true;
            prepend_lexer_implementation(ctxt, &js_lexer);
        }
    }
    PUSH_STATE(IN_TAG);
    TOKEN(NAME_TAG);
}

<INITIAL> ETag {
    if (mydata->css && 0 == YYSTRNCASECMP("</style>")) {
        mydata->css = false;
        unprepend_lexer(ctxt, &css_lexer);
    } else if (mydata->js && 0 == YYSTRNCASECMP("</script>")) {
        mydata->js = false;
        unprepend_lexer(ctxt, &js_lexer);
    }
    TOKEN(NAME_TAG);
}

<IN_TAG,IN_PREPROC> S {
    TOKEN(IGNORABLE);
}

<IN_TAG>">" {
    //BEGIN(INITIAL);
    POP_STATE();
    TOKEN(NAME_TAG);
}

<IN_PREPROC> "?>" {
    //BEGIN(INITIAL);
    POP_STATE();
    TOKEN(TAG_PREPROC);
}

<IN_TAG,IN_PREPROC> Name Eq {
    PUSH_STATE(IN_ATTRIBUTE);
    TOKEN(NAME_ATTRIBUTE);
}

<IN_TAG> Name {
    TOKEN(NAME_ATTRIBUTE);
}

<IN_ATTRIBUTE> "'" {
    //PUSH_STATE(IN_SINGLE_QUOTES);
    BEGIN(IN_SINGLE_QUOTES);
    TOKEN(STRING_SINGLE);
}

<IN_SINGLE_QUOTES> "'" {
    POP_STATE();
    //POP_STATE();
    TOKEN(STRING_SINGLE);
}

<IN_ATTRIBUTE> '"' {
    //PUSH_STATE(IN_DOUBLE_QUOTES);
    BEGIN(IN_DOUBLE_QUOTES);
    TOKEN(STRING_SINGLE);
}

<IN_DOUBLE_QUOTES> '"' {
    POP_STATE();
    //POP_STATE();
    TOKEN(STRING_SINGLE);
}

<INITIAL,IN_SINGLE_QUOTES,IN_DOUBLE_QUOTES> EntityRef {
    TOKEN(NAME_ENTITY);
}

<INITIAL> [^] {
    if (mydata->css) {
        YYCTYPE *end;

        // TODO: case insensitive search of "</style>"
        if (NULL == (end = (YYCTYPE *) memstr((const char *) YYCURSOR, "</style>", STR_LEN("</style>"), (const char *) YYLIMIT))) {
            YYCURSOR = YYLIMIT;
        } else {
            YYCURSOR = end;
        }
        DELEGATE_UNTIL(IGNORABLE);
    } else if (mydata->js) {
        YYCTYPE *end;

        // TODO: case insensitive search of "</script>"
        if (NULL == (end = (YYCTYPE *) memstr((const char *) YYCURSOR, "</script>", STR_LEN("</script>"), (const char *) YYLIMIT))) {
            YYCURSOR = YYLIMIT;
        } else {
            YYCURSOR = end;
        }
        DELEGATE_UNTIL(IGNORABLE);
    } else {
        TOKEN(IGNORABLE);
    }
}

<*>[^] {
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
    DONE();
}

LexerImplementation xml_lexer = {
    "XML",
    "Generic lexer for XML (eXtensible Markup Language)",
    NULL,
    (const char * const []) { "*.xml", "*.xsd", NULL },
    (const char * const []) { "text/xml", "application/xml", NULL },
    NULL, // interpreters
    xmlanalyse,
    NULL, // init
    xmllex,
    NULL, // finalize
    sizeof(XMLLexerData),
    NULL, // options
    (const LexerImplementation * const []) { &dtd_lexer, NULL },
    NULL, // yypush_parse
    NULL, // yypstate_new
    NULL, // yypstate_delete
};

LexerImplementation html_lexer = {
    "HTML",
    "XXX",
    NULL,
    (const char * const []) { "*.html", NULL },
    (const char * const []) { "text/html", NULL },
    NULL, // interpreters
    NULL, // analyse
    htmlinit,
    xmllex,
    NULL, // finalize
    sizeof(XMLLexerData),
    NULL, // options
    (const LexerImplementation * const []) { &css_lexer, &js_lexer, NULL },
    NULL, // yypush_parse
    NULL, // yypstate_new
    NULL, // yypstate_delete
};
