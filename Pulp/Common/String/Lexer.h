#pragma once

#ifndef X_STRING_LEXER_H_
#define X_STRING_LEXER_H_

#include <String\StringUtil.h>
#include <Util\FlagsMacros.h>

X_NAMESPACE_BEGIN(core)

X_DECLARE_FLAGS(LexFlag)
(
    NOERRORS,                   // don't print any errors
    NOWARNINGS,                 // don't print any warnings
    NOFATALERRORS,              // errors aren't fatal
    NOSTRINGCONCAT,             // multiple strings seperated by whitespaces are not concatenated
    NOSTRINGESCAPECHARS,        // no escape characters inside strings
    NODOLLARPRECOMPILE,         // don't use the $ sign for precompilation
    NOBASEINCLUDES,             // don't include files embraced with < >
    ALLOWPATHNAMES,             // allow path seperators in names
    ALLOWNUMBERNAMES,           // allow names to start with a number
    ALLOWDOLLARNAMES,           // allow names to start with a $
    ALLOWIPADDRESSES,           // allow ip addresses to be parsed as numbers
    ALLOWFLOATEXCEPTIONS,       // allow float exceptions like 1.#INF or 1.#IND to be parsed
    ALLOWMULTICHARLITERALS,     // allow multi character literals
    ALLOWBACKSLASHSTRINGCONCAT, // allow multiple strings seperated by '\' to be concatenated
    ONLYSTRINGS                 // parse as whitespace deliminated strings (quoted strings keep quotes)
);

typedef Flags<LexFlag> LexFlags;

X_DECLARE_FLAG_OPERATORS(LexFlags);

X_DECLARE_ENUM(TokenType)
(
    STRING,
    LITERAL,
    NUMBER,
    NAME,
    PUNCTUATION,
    INVALID);

#ifdef NAN
#undef NAN
#endif
#ifdef INFINITE
#undef INFINITE
#endif

X_DECLARE_FLAGS(TokenSubType)
(
    UNUSET,
    INTEGER,
    DECIMAL,
    HEX,
    OCTAL,
    BINARY,
    LONG,
    UNSIGNED,
    FLOAT,
    SINGLE_PRECISION,
    DOUBLE_PRECISION,
    EXTENDED_PRECISION, // long double
    INFINITE,
    INDEFINITE,
    NAN,
    IPADDRESS,
    IPPORT,
    VALUESVALID);

typedef Flags<TokenSubType> TokenSubTypeFlags;

X_DECLARE_FLAG_OPERATORS(TokenSubTypeFlags);

struct PunctuationId
{
    enum Enum
    {
        UNUSET,

        RSHIFT_ASSIGN,
        LSHIFT_ASSIGN,
        PARMS,
        PRECOMPMERGE,

        LOGIC_AND,
        LOGIC_OR,
        LOGIC_GEQ,
        LOGIC_LEQ,
        LOGIC_EQ,
        LOGIC_UNEQ,

        MUL_ASSIGN,
        DIV_ASSIGN,
        MOD_ASSIGN,
        ADD_ASSIGN,
        SUB_ASSIGN,
        INC,
        DEC,

        BIN_AND_ASSIGN,
        BIN_OR_ASSIGN,
        BIN_XOR_ASSIGN,
        RSHIFT,
        LSHIFT,

        POINTERREF,
        CPP1,
        CPP2,
        MUL,
        DIV,
        MOD,
        ADD,
        SUB,
        ASSIGN,

        BIN_AND,
        BIN_OR,
        BIN_XOR,
        BIN_NOT,

        LOGIC_NOT,
        LOGIC_GREATER,
        LOGIC_LESS,

        REF,
        COMMA,
        SEMICOLON,
        COLON,
        QUESTIONMARK,

        PARENTHESESOPEN,
        PARENTHESESCLOSE,
        BRACEOPEN,
        BRACECLOSE,
        SQBRACKETOPEN,
        SQBRACKETCLOSE,
        BACKSLASH,

        PRECOMP,
        DOLLAR
    };
};

// punctuation
struct PunctuationPair
{
    const char* pCharacter; // punctuation character(s)
    PunctuationId::Enum id; // punctuation id
};

class XLexer;

class XLexToken
{
public:
    typedef TokenSubTypeFlags TokenSubTypeFlags;

public:
    X_INLINE XLexToken();
    X_INLINE XLexToken(const char* start, const char* end);

    X_INLINE size_t length(void) const;
    X_INLINE TokenType::Enum GetType(void) const;
    X_INLINE TokenSubTypeFlags GetSubType(void) const;
    X_INLINE PunctuationId::Enum GetPuncId(void) const;
    X_INLINE int32_t GetLine(void) const;
    X_INLINE void SetType(TokenType::Enum type);
    X_INLINE void SetSubType(TokenSubTypeFlags subType);

    X_INLINE const char* begin(void) const;
    X_INLINE const char* end(void) const;

    X_INLINE bool isEqual(const char* str) const;
    X_INLINE bool isEqual(const char* pBegin, const char* pEnd) const;

    X_INLINE double GetDoubleValue(void); // double value of TokenType::NUMBER
    X_INLINE float GetFloatValue(void);   // float value of TokenType::NUMBER
    X_INLINE uint32_t GetUIntValue(void); // unsigned int value of TokenType::NUMBER
    X_INLINE int32_t GetIntValue(void);

    X_INLINE void Reset(void);

private:
    X_INLINE void Init(void);

    void NumberValue(void);

protected:
    friend class XLexer;
    friend class XParser;

    X_INLINE void SetStart(const char* start);
    X_INLINE void SetEnd(const char* end);

    X_INLINE XLexToken* GetNext(void);
    X_INLINE const XLexToken* GetNext(void) const;

private:
    TokenType::Enum type_;       // token type
    TokenSubTypeFlags subtype_;  // token sub type
    PunctuationId::Enum puncId_; // punctiation id

    int32_t line_;         // line in script the token was on
    int32_t linesCrossed_; // number of lines crossed in white space before token
                           //	Flags<TokenFlag>	flags_;		// token flags, used for recursive defines

    int32_t intvalue_; // integer value
    double floatvalue_;

    const char* start_;
    const char* end_;

    // shit used by parser.
    XLexToken* pNext_;
};

class XLexer
{
    friend class XParser;

public:
    X_DECLARE_ENUM(ErrorState)
    (
        OK,
        WARNINGS,
        ERRORS);

public:
    typedef LexFlags LexFlags;

    XLexer();
    XLexer(const char* startInclusive, const char* endExclusive);
    XLexer(const char* startInclusive, const char* endExclusive, core::string name);

    bool SetMemory(const char* startInclusive, const char* endExclusive, core::string name);

    bool ReadToken(XLexToken& token);

    bool ExpectTokenString(const char* string);
    bool ExpectTokenType(TokenType::Enum type, XLexToken::TokenSubTypeFlags subtype,
        PunctuationId::Enum puncId, XLexToken& token);

    bool PeekTokenString(const char* string);
    bool PeekTokenType(TokenType::Enum type, XLexToken::TokenSubTypeFlags subtype,
        PunctuationId::Enum puncId, XLexToken& token);

    bool SkipUntilString(const char* string);
    bool SkipRestOfLine(void);

    int32_t ParseInt(void);
    bool ParseBool(void);
    float ParseFloat(void);

    // parse matrices with floats
    bool Parse1DMatrix(int32_t x, float* m);
    bool Parse2DMatrix(int32_t y, int32_t x, float* m);
    bool Parse3DMatrix(int32_t z, int32_t y, int32_t x, float* m);

    void UnreadToken(const XLexToken& token);
    // read a token only if on the same line
    bool ReadTokenOnLine(XLexToken& token);

    X_INLINE const char* GetFileName(void) const;
    X_INLINE int32_t GetLineNumber(void) const;
    X_INLINE bool isEOF(void) const;
    X_INLINE bool isEOF(bool skipWhiteSpace);
    X_INLINE size_t BytesLeft(void) const;
    X_INLINE void setFlags(LexFlags flags);
    X_INLINE ErrorState::Enum GetErrorState(void) const;

    void Error(const char* str, ...);
    void Warning(const char* str, ...);

private:
    void SetPunctuations(const PunctuationPair* p);
    void CreatePunctuationTable(const PunctuationPair* punctuations);
    const char* GetPunctuationFromId(PunctuationId::Enum id);

    bool ReadWhiteSpace(void);

    bool ReadEscapeCharacter(char* ch);
    bool ReadString(XLexToken& token, int quote);
    bool ReadName(XLexToken& token);
    bool ReadNumber(XLexToken& token);
    bool ReadPunctuation(XLexToken& token);
    bool ReadPrimitive(XLexToken& token);

    X_INLINE int32_t CheckString(const char* str) const;

private:
    core::string filename_;

    const char* start_;
    const char* end_;
    const char* current_;
    const char* lastp_;
    int32_t curLine_;
    int32_t lastLine_; // line before reading token
    LexFlags flags_;

    XLexToken token_;
    int32_t tokenavailable_;

    const PunctuationPair* punctuations_; // the punctuations used in the script
    int32_t* punctuationtable_;           // ASCII table with punctuations
    int32_t* nextpunctuation_;

    ErrorState::Enum errState_;
};

X_NAMESPACE_END

#include "Lexer.inl"

#endif // X_STRING_LEXER_H_