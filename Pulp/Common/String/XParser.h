#pragma once

#ifndef X_STR_PARSER_H_
#define X_STR_PARSER_H_

#include "Lexer.h"

#include "Containers\HashMap.h"
#include <Containers\Stack.h>

#include <Util\Delegate.h>

X_NAMESPACE_BEGIN(core)

//
//	This is a Lexer that supports macros
//
X_DECLARE_ENUM(PreProType)
(
    Include,
    Define,
    Undef,
    If,
    IfDef,
    IfNDef,
    Else,
    ElseIf,
    EndIF,
    Undefined);

X_DECLARE_FLAGS(DefineFlag)
(
    RECURSIVE,
    BUILTIN);

typedef Flags<DefineFlag> DefineFlags;

struct MacroDefine
{
    MacroDefine() :
        numParams(0),
        pTokens(nullptr),
        pParms(nullptr)
    {
    }

    core::string name;
    int32_t numParams;
    DefineFlags flags;
    XLexToken* pTokens;
    XLexToken* pParms;
};

struct Ident
{
    Ident() = default;
    Ident(PreProType::Enum type, bool skip, XLexer* pScript) :
        type(type),
        skip(skip),
        pScript(pScript)
    {
    }

    PreProType::Enum type;
    bool skip;
    bool _pad[3];
    XLexer* pScript;
};

class XParser
{
    typedef XLexer::LexFlags LexFlags;
    typedef core::HashMap<core::string, MacroDefine*> MacroMap;
    typedef core::Stack<Ident> IdentStack;
    typedef core::Stack<XLexer*> ScriptStack;

    struct Value
    {
        int32_t intvalue;
        double floatvalue;
        int32_t parentheses;
    };

public:
    typedef core::Delegate<bool(XLexer& lex, core::string&, bool useIncludePath)> OpenIncludeDel;

    static const int32_t MAX_DEFINEPARMS = 32;
    static const int32_t MAX_IDENTS = 16; // this is stack so you can have unlimited groups this is more of a depth limit.
    static const int32_t MAX_SCRIPT_STACK = 8;

public:
    explicit XParser(MemoryArenaBase* arena);
    XParser(LexFlags flags, MemoryArenaBase* arena);
    XParser(const char* startInclusive, const char* endExclusive, const char* name, LexFlags flags, MemoryArenaBase* arena);
    ~XParser();

    void setIncludeCallback(OpenIncludeDel& del);

    // load me up!
    bool SetMemory(const char* startInclusive, const char* endExclusive, const char* name);

    bool ReadToken(XLexToken& token);
    bool ExpectTokenString(const char* string);
    bool ExpectTokenType(TokenType::Enum type, XLexToken::TokenSubTypeFlags subtype,
        PunctuationId::Enum puncId, XLexToken& token);

    int ParseInt(void);
    bool ParseBool(void);
    float ParseFloat(void);

    void UnreadToken(const XLexToken& token);
    bool isEOF(void) const;

    X_INLINE const char* GetFileName(void) const;
    X_INLINE int32_t GetLineNumber(void) const;
    X_INLINE void setFlags(LexFlags flags);
    X_INLINE LexFlags getFlags(void);

private:
    void Error(const char* str, ...);
    void Warning(const char* str, ...);

    void freeSource(void);

    bool ReadSourceToken(XLexToken& token);
    bool UnreadSourceToken(const XLexToken& token);
    bool ReadDirective(void);

    bool isInCache(const uint8_t ch) const;
    void addToCache(const uint8_t ch);

    bool Directive_define(void);
    bool Directive_include(void);

    bool Directive_undef(void);
    bool Directive_if_def(PreProType::Enum type);
    bool Directive_ifdef(void);
    bool Directive_ifndef(void);
    bool Directive_else(void);
    bool Directive_endif(void);
    bool Directive_elif(void);
    bool Directive_if(void);
    bool Directive_line(void);
    bool Directive_error(void);
    bool Directive_warning(void);

    void PushIndent(PreProType::Enum type, bool skip);
    void PopIndent(PreProType::Enum* pType, bool* skip);

    bool Evaluate(int32_t* pIntvalue, double* pFloatvalue, bool isInteger);
    bool EvaluateTokens(XLexToken* pTokens, int32_t* pIntvalue, double* pFloatvalue, bool isInteger);

    bool CheckTokenString(const char* string);
    int FindDefineParm(MacroDefine* define, const XLexToken& token);
    int ReadDefineParms(MacroDefine* pDefine, XLexToken** parms, int maxparms);

    int ReadLine(XLexToken& token);

    bool ExpandDefine(XLexToken& deftoken, MacroDefine* pDefine,
        XLexToken*& firsttoken, XLexToken*& lasttoken);

    bool ExpandDefineIntoSource(XLexToken& token, MacroDefine* pDefine);

    MacroDefine* FindDefine(XLexToken& token) const;
    void addDefinetoHash(MacroDefine* pDefine);
    void removeDefine(MacroDefine* pDefine);

private:
    MemoryArenaBase* arena_;

    core::string filename_;

    MacroMap macros_;
    IdentStack idents_;
    LexFlags flags_;

    ScriptStack scriptStack_;
    XLexToken* pTokens_;

    OpenIncludeDel openIncDel_;

    int32_t skip_; // skip(count) current content inside a #if 0 .. #endif etc..

    uint8_t macroCharCache[255];
};

X_NAMESPACE_END

#include "XParser.inl"

#endif // !X_STR_PARSER_H_