#include "EngineCommon.h"
#include "XParser.h"

#include <Hashing\Fnva1Hash.h>
#include <Logging\LoggerBase.h>
#include <Util\UniquePointer.h>

X_NAMESPACE_BEGIN(core)

XParser::XParser(MemoryArenaBase* arena) :
    XParser(LexFlags(), arena)
{
}

XParser::XParser(LexFlags flags, MemoryArenaBase* arena) :
    XParser(nullptr, nullptr, nullptr, flags, arena)
{
    X_ASSERT_NOT_NULL(arena);
    core::zero_object(macroCharCache);
}

XParser::XParser(const char* startInclusive, const char* endExclusive,
    const char* name, LexFlags flags, MemoryArenaBase* arena) :
    arena_(arena),
    pTokens_(nullptr),
    macros_(arena, 4),
    idents_(arena, MAX_IDENTS),
    scriptStack_(arena, MAX_SCRIPT_STACK),
    flags_(flags)
{
    X_ASSERT_NOT_NULL(arena);
    core::zero_object(macroCharCache);

    if (startInclusive) {
        SetMemory(startInclusive, endExclusive, name);
    }
}

XParser::~XParser()
{
    freeSource();
}

void XParser::setIncludeCallback(OpenIncludeDel& del)
{
    openIncDel_ = del;
}

void XParser::freeSource(void)
{
    while (scriptStack_.isNotEmpty()) {
        X_DELETE(scriptStack_.top(), arena_);
        scriptStack_.pop();
    }

    idents_.clear();

    while (pTokens_) {
        auto token = pTokens_;
        pTokens_ = token;
        X_DELETE(token, arena_);
    }

    for (auto d : macros_) {
        X_DELETE(d.second, arena_);
    }

    macros_.clear();

    filename_.clear();
}

bool XParser::SetMemory(const char* startInclusive, const char* endExclusive,
    const char* name)
{
    X_ASSERT_NOT_NULL(startInclusive);
    X_ASSERT_NOT_NULL(endExclusive);
    X_ASSERT_NOT_NULL(name);

    if (scriptStack_.isNotEmpty()) {
        // someting already set.
        return false;
    }

    filename_ = name;

    auto* pLexer = X_NEW(XLexer, arena_, "ParserLex")(startInclusive, endExclusive, filename_);
    pLexer->setFlags(flags_);

    scriptStack_.push(pLexer);

    skip_ = false;

    return true;
}

bool XParser::ReadToken(XLexToken& token)
{
    X_DISABLE_WARNING(4127)
    while (true)
        X_ENABLE_WARNING(4127)
        {
            if (!ReadSourceToken(token)) {
                return false;
            }

            if (token.GetType() == TokenType::PUNCTUATION && token.GetPuncId() == PunctuationId::PRECOMP) {
                if (!ReadDirective()) {
                    return false;
                }
                continue;
            }

            // keep reading skipped tokens.
            if (skip_) {
                continue;
            }

            if (token.GetType() == TokenType::NAME) {
                MacroDefine* define = FindDefine(token);
                if (define) {
                    // expand the defined macro
                    if (!ExpandDefineIntoSource(token, define)) {
                        return false;
                    }
                    continue;
                }
            }
            return true;
        }
}

bool XParser::ExpectTokenString(const char* string)
{
    XLexToken token;

    if (!XParser::ReadToken(token)) {
        X_LOG0("Parser", "couldn't find expected '%s'", string);
        return false;
    }

    if (!token.isEqual(string)) {
        X_LOG0("Parser", "expected '%s' but found '%.*s'",
            string, token.length(), token.begin());
        return false;
    }
    return true;
}

bool XParser::ExpectTokenType(TokenType::Enum type, XLexToken::TokenSubTypeFlags subtype,
    PunctuationId::Enum puncId, XLexToken& token)
{
    if (!XParser::ReadToken(token)) {
        Error("couldn't read expected token");
        return false;
    }

    if (token.GetType() != type) {
        core::StackString<128> str;

        switch (type) {
            case TokenType::STRING:
                str.append("string");
                break;
            case TokenType::LITERAL:
                str.append("literal");
                break;
            case TokenType::NUMBER:
                str.append("number");
                break;
            case TokenType::NAME:
                str.append("name");
                break;
            case TokenType::PUNCTUATION:
                str.append("punctuation");
                break;
            default:
                str.append("unknown type");
                break;
        }
        Error("expected a %s but found '%.*s'", str.c_str(), token.length(), token.begin());
        return false;
    }

    if (token.GetType() == TokenType::NUMBER) {
        if ((token.GetSubType() & subtype) != subtype) {
            core::StackString<128> str;
            if (subtype.IsSet(TokenSubType::DECIMAL))
                str.append("decimal ");
            if (subtype.IsSet(TokenSubType::HEX))
                str.append("hex ");
            if (subtype.IsSet(TokenSubType::OCTAL))
                str.append("octal ");
            if (subtype.IsSet(TokenSubType::BINARY))
                str.append("binary ");
            if (subtype.IsSet(TokenSubType::UNSIGNED))
                str.append("unsigned ");
            if (subtype.IsSet(TokenSubType::LONG))
                str.append("long ");
            if (subtype.IsSet(TokenSubType::FLOAT))
                str.append("float ");
            if (subtype.IsSet(TokenSubType::INTEGER))
                str.append("integer ");
            str.stripTrailing(' ');
            Error("expected %s but found '%.*s'", str.c_str(), token.length(), token.begin());
            return false;
        }
    }
    else if (token.GetType() == TokenType::PUNCTUATION && puncId != PunctuationId::UNUSET) {
        if (token.GetPuncId() != puncId) {
            X_ASSERT(scriptStack_.isNotEmpty(), "Script stack should not be empty")
            ();

            Error("expected '%s' but found '%.*s'",
                scriptStack_.top()->GetPunctuationFromId(puncId), token.length(), token.begin());
            return false;
        }
    }
    return true;
}

int32_t XParser::ParseInt(void)
{
    XLexToken token;

    if (!XParser::ReadToken(token)) {
        Error("couldn't read expected integer");
        return 0;
    }
    if (token.GetType() == TokenType::PUNCTUATION && token.isEqual("-")) {
        ExpectTokenType(TokenType::NUMBER, TokenSubType::INTEGER,
            PunctuationId::UNUSET, token);

        return -(safe_static_cast<int32_t>(token.GetIntValue()));
    }
    else if (token.GetType() != TokenType::NUMBER
             || token.GetSubType() == TokenSubType::FLOAT) {
        Error("expected integer value, found '%.*s'", token.length(), token.begin());
    }
    return token.GetIntValue();
}

bool XParser::ParseBool(void)
{
    XLexToken token;

    if (!XParser::ReadToken(token)) {
        Error("couldn't read expected bool");
        return false;
    }

    if (token.GetType() == TokenType::NUMBER) {
        return (token.GetIntValue() != 0);
    }
    else if (token.GetType() == TokenType::NAME) {
        if (token.isEqual("true")) {
            return true;
        }
        else if (token.isEqual("false")) {
            return false;
        }
    }

    Error("couldn't read expected boolean");
    return false;
}

float XParser::ParseFloat(void)
{
    XLexToken token;

    if (!XParser::ReadToken(token)) {
        Error("couldn't read expected floating point number");
        return 0.0f;
    }
    if (token.GetType() == TokenType::PUNCTUATION && token.isEqual("-")) {
        ExpectTokenType(TokenType::NUMBER, TokenSubType::UNUSET,
            PunctuationId::UNUSET, token);

        return -token.GetFloatValue();
    }
    else if (token.GetType() != TokenType::NUMBER) {
        Error("expected float value, found '%.*s'", token.length(), token.begin());
    }
    return token.GetFloatValue();
}

void XParser::UnreadToken(const XLexToken& token)
{
    UnreadSourceToken(token);
}

bool XParser::isEOF(void) const
{
    if (scriptStack_.isNotEmpty()) {
        return scriptStack_.top()->isEOF();
    }
    X_WARNING("Parser", "called 'EOF' on a parser without a valid file loaded");
    return true;
}

bool XParser::ReadSourceToken(XLexToken& token)
{
    if (scriptStack_.isEmpty()) {
        X_ERROR("Parser", "can't read token, no source loaded");
        return false;
    }

    int32_t changedScript = 0;
    while (!pTokens_) {
        auto pCurScript = scriptStack_.top();

        if (pCurScript->ReadToken(token)) {
            token.linesCrossed_ += changedScript;

            return true;
        }

        // if at end of file.
        if (pCurScript->isEOF()) {
            // remove all unclosed indents.
            while (idents_.isNotEmpty() && idents_.top().pScript == pCurScript) {
                XParser::Warning("missing #endif");
                idents_.pop();
            }

            changedScript = 1;
        }

        if (scriptStack_.size() == 1) {
            return false;
        }

        // this script is fully parsed.
        X_DELETE(scriptStack_.top(), arena_);
        scriptStack_.pop();
    }

    // copy the already available token
    token = *pTokens_;

    // remove the token from the source
    XLexToken* t = pTokens_;
    pTokens_ = t->pNext_;

    X_DELETE(t, arena_);
    return true;
}

bool XParser::UnreadSourceToken(const XLexToken& token)
{
    XLexToken* t;

    t = X_NEW(XLexToken, arena_, "TokenUnread")(token);
    t->pNext_ = pTokens_;
    pTokens_ = t;
    return true;
}

bool XParser::ReadDirective(void)
{
    XLexToken token;

    if (!ReadSourceToken(token)) {
        X_ERROR("Parser", "'#' without a name");
        return false;
    }

    if (token.GetType() == TokenType::NAME) {
        using namespace core::Hash::Literals;

        static_assert(PreProType::ENUM_COUNT == 10, "PreProType count changed? this code needs updating.");
        const auto tokenHash = core::Hash::Fnv1aHash(token.begin(), token.length());

        switch (tokenHash) {
            case "if"_fnv1a:
                return Directive_if();
            case "ifdef"_fnv1a:
                return Directive_ifdef();
            case "ifndef"_fnv1a:
                return Directive_ifndef();
            case "elif"_fnv1a:
                return Directive_elif();
            case "else"_fnv1a:
                return Directive_else();
            case "endif"_fnv1a:
                return Directive_endif();

            default: {
                if (skip_) {
                    while (XParser::ReadLine(token)) {
                        // read rest of line
                    }
                    return true;
                }

                switch (tokenHash) {
                    case "include"_fnv1a:
                        return Directive_include();
                    case "define"_fnv1a:
                        return Directive_define();
                    case "undef"_fnv1a:
                        return Directive_undef();
                    case "line"_fnv1a:
                        return Directive_line();
                    case "error"_fnv1a:
                        return Directive_error();
                    case "warning"_fnv1a:
                        return Directive_warning();

                    default:
                        break;
                }

                break;
            }
        }
    }

    X_ERROR("Parser", "unknown precompiler directive. line(%i)", token.GetLine());
    return false;
}

bool XParser::isInCache(const uint8_t ch) const
{
    return macroCharCache[ch] == 1;
}

void XParser::addToCache(const uint8_t ch)
{
    macroCharCache[ch] = 1;
}

bool XParser::Directive_define(void)
{
    XLexToken token, *t, *last;
    MacroDefine* define;

    if (!ReadSourceToken(token)) {
        X_ERROR("Parser", "#define without a name");
        return false;
    }

    if (token.GetType() != TokenType::NAME) {
        core::StackString512 temp(token.begin(), token.end());
        X_ERROR("Parser", "expected a name after #define got: %s", temp.c_str());
        return false;
    }

    if (token.length() < 1) {
        X_ERROR("Parser", "invalid token line %i", token.GetLine());
        return false;
    }

    define = FindDefine(token);
    if (define) {
        // alreayd defined.
        return true;
    }

    define = X_NEW(MacroDefine, arena_, "Macro")();
    define->name.assign(token.begin(), token.end());
    addDefinetoHash(define);

    // check above makes sure atleast one char
    addToCache(define->name[0]);

    if (!ReadLine(token)) {
        return true;
    }

    if (token.isEqual("(")) {
        // read the define parameters
        last = nullptr;

        if (!CheckTokenString(")")) {
            X_DISABLE_WARNING(4127)
            while (true)
                X_ENABLE_WARNING(4127)
                {
                    if (!ReadLine(token)) {
                        Error("expected define parameter");
                        return false;
                    }
                    // if it isn't a name
                    if (token.GetType() != TokenType::NAME) {
                        Error("invalid define parameter");
                        return false;
                    }

                    if (FindDefineParm(define, token) >= 0) {
                        Error("two the same define parameters");
                        return false;
                    }

                    // add the define parm
                    t = X_NEW(XLexToken, arena_, "MarcoParam")(token);
                    //	t->ClearTokenWhiteSpace();
                    t->pNext_ = nullptr;
                    if (last)
                        last->pNext_ = t;
                    else
                        define->pParms = t;

                    last = t;
                    define->numParams++;

                    // read next token
                    if (!ReadLine(token)) {
                        Error("define parameters not terminated");
                        return false;
                    }

                    // the end?
                    if (token.isEqual(")")) {
                        break;
                    }
                    // then it must be a comma
                    if (!token.isEqual(",")) {
                        Error("define not terminated");
                        return false;
                    }
                }

            if (!ReadLine(token)) {
                return true;
            }
        }
    }

    do {
        XLexToken* pT = X_NEW(XLexToken, arena_, "Macrotoken")(token);

        if (token.GetType() == TokenType::NAME && token.isEqual(define->name)) {
            //			token.flags_;
            //	t->flags |= TOKEN_FL_RECURSIVE_DEFINE;
            XParser::Warning("recursive define (removed recursion)");
        }
        //	t->ClearTokenWhiteSpace();

        pT->pNext_ = define->pTokens;
        define->pTokens = pT;

    } while (ReadLine(token));

    return true;
}

bool XParser::Directive_include(void)
{
    XLexToken token;

    if (!openIncDel_) {
        XParser::Error("#include callback not set");
        return false;
    }

    if (!XParser::ReadSourceToken(token)) {
        XParser::Error("#include without filename");
        return false;
    }

    if (token.linesCrossed_ > 0) {
        XParser::Error("#include without file name");
        return false;
    }

    core::UniquePointer<XLexer> lex = makeUnique<XLexer>(arena_);
    core::string path;

    if (token.GetType() == TokenType::STRING) {
        path = core::string(token.begin(), token.end());

        if (!openIncDel_.Invoke(*lex.ptr(), path, false)) {
            lex.reset();
        }
    }
    else if (token.GetType() == TokenType::PUNCTUATION && token.GetPuncId() == PunctuationId::LOGIC_LESS) // "<"
    {
        while (XParser::ReadSourceToken(token)) {
            if (token.linesCrossed_ > 0) {
                XParser::UnreadSourceToken(token);
                break;
            }
            if (token.GetType() == TokenType::PUNCTUATION && token.GetPuncId() == PunctuationId::LOGIC_GREATER) // ">"
            {
                break;
            }

            path.append(token.begin(), token.end());
        }

        if (token.GetPuncId() != PunctuationId::LOGIC_GREATER) {
            XParser::Warning("#include missing trailing >");
        }
        if (!path.length()) {
            XParser::Error("#include without file name between < >");
            return false;
        }

        // open the fucker!
        if (!openIncDel_.Invoke(*lex.ptr(), path, true)) {
            lex.reset();
        }
    }
    else {
        XParser::Error("#include without file name");
        return false;
    }

    if (!lex) {
        XParser::Error("#include file \"%s\" not found", path.c_str());
        return false;
    }

    scriptStack_.push(lex.release());
    return true;
}

bool XParser::Directive_undef(void)
{
    XLexToken token;

    if (!XParser::ReadLine(token)) {
        XParser::Error("undef without name");
        return false;
    }

    if (token.GetType() != TokenType::NAME) {
        XParser::UnreadSourceToken(token);
        XParser::Error("expected name but found '%.*s'", token.length(), token.begin());
        return false;
    }

    MacroDefine* pDefine = FindDefine(token);
    if (!pDefine) {
        XParser::Warning("can't undef '%.*s' not defined", token.length(), token.begin());
        return true;
    }

    if (pDefine->flags.IsSet(DefineFlag::BUILTIN)) {
        XParser::Warning("can't undef '%.*s'", token.length(), token.begin());
    }
    else {
        removeDefine(pDefine);
    }

    return true;
}

bool XParser::Directive_if_def(PreProType::Enum type)
{
    XLexToken token;

    if (!XParser::ReadLine(token)) {
        XParser::Error("undef without name");
        return false;
    }

    if (token.GetType() != TokenType::NAME) {
        XParser::UnreadSourceToken(token);
        XParser::Error("expected name after #ifdef, found '%.*s'", token.length(), token.begin());
        return false;
    }

    MacroDefine* pDefine = FindDefine(token);

    const bool skip = (type == PreProType::IfNDef) == (pDefine == nullptr);
    XParser::PushIndent(type, skip);
    return true;
}

bool XParser::Directive_ifdef(void)
{
    return XParser::Directive_if_def(PreProType::IfDef);
}

bool XParser::Directive_ifndef(void)
{
    return XParser::Directive_if_def(PreProType::IfNDef);
}

bool XParser::Directive_else(void)
{
    bool skip;
    PreProType::Enum type;

    XParser::PopIndent(&type, &skip);
    if (!type) {
        XParser::Error("misplaced #else");
        return false;
    }
    if (type == PreProType::Else) {
        XParser::Error("#else after #else");
        return false;
    }

    XParser::PushIndent(PreProType::Else, !skip);
    return true;
}

bool XParser::Directive_endif(void)
{
    bool skip;
    PreProType::Enum type;

    XParser::PopIndent(&type, &skip);
    if (type == PreProType::Undefined) {
        XParser::Error("misplaced #endif");
        return false;
    }
    return true;
}

bool XParser::Directive_elif(void)
{
    bool skip;
    PreProType::Enum type;

    XParser::PopIndent(&type, &skip);

    if (type == PreProType::Undefined || type == PreProType::Else) {
        XParser::Error("misplaced #elif");
        return false;
    }

    int32_t value = 0;
    if (!XParser::Evaluate(&value, nullptr, true)) {
        return false;
    }

    skip = (value == 0);
    XParser::PushIndent(PreProType::ElseIf, skip);
    return true;
}

bool XParser::Directive_if(void)
{
    int32_t value = 0;
    if (!XParser::Evaluate(&value, nullptr, true)) {
        return false;
    }

    const bool skip = (value == 0);

    XParser::PushIndent(PreProType::If, skip);
    return true;
}

bool XParser::Directive_line(void)
{
    XLexToken token;

    XParser::Error("#line directive not supported");
    while (XParser::ReadLine(token)) {
        // ...
    }
    return true;
}

bool XParser::Directive_error(void)
{
    XLexToken token;

    if (!XParser::ReadLine(token) || token.GetType() != TokenType::STRING) {
        XParser::Error("#error without string");
        return false;
    }

    XParser::Error("#error: %.*s", token.length(), token.begin());
    return true;
}

bool XParser::Directive_warning(void)
{
    XLexToken token;

    if (!XParser::ReadLine(token) || token.GetType() != TokenType::STRING) {
        XParser::Error("#warning without string");
        return false;
    }

    XParser::Warning("#warning: %.*s", token.length(), token.begin());
    return true;
}

void XParser::PushIndent(PreProType::Enum type, bool skip)
{
    if (idents_.size() == MAX_IDENTS) {
        XParser::Error("Exceeded limit of max idents. Max: %" PRIuS, idents_.size());
        return;
    }

    skip_ += skip;

    idents_.emplace(type, skip, scriptStack_.top());
}

void XParser::PopIndent(PreProType::Enum* pType, bool* pSkip)
{
    X_ASSERT_NOT_NULL(pType);
    X_ASSERT_NOT_NULL(pSkip);

    if (idents_.isEmpty()) {
        *pType = PreProType::Undefined;
        *pSkip = false;
        return;
    }

    const auto ident = idents_.top();
    if (ident.pScript != scriptStack_.top()) {
        XParser::Warning("Tried to pop ident that is not from current script");
        return;
    }

    *pType = ident.type;
    *pSkip = ident.skip;

    skip_ -= ident.skip;

    idents_.pop();
}

bool XParser::Evaluate(int32_t* pIntvalue, double* pFloatvalue, bool isInteger)
{
    XLexToken token, *pFirsttoken, *pLasttoken;
    XLexToken *pToken, *pNexttoken;

    if (pIntvalue) {
        *pIntvalue = 0;
    }
    if (pFloatvalue) {
        *pFloatvalue = 0;
    }

    //
    if (!XParser::ReadLine(token)) {
        XParser::Error("no value after #if/#elif");
        return false;
    }

    pFirsttoken = nullptr;
    pLasttoken = nullptr;

    bool defined = false;
    MacroDefine* pDefine = nullptr;

    do {
        //if the token is a name
        if (token.GetType() == TokenType::NAME) {
            if (defined) {
                defined = false;
                pToken = X_NEW(XLexToken, arena_, "Evaltoken")(token);
                pToken->pNext_ = nullptr;
                if (pLasttoken)
                    pLasttoken->pNext_ = pToken;
                else
                    pFirsttoken = pToken;

                pLasttoken = pToken;
            }
            else if (token.isEqual("defined")) {
                defined = true;
                pToken = X_NEW(XLexToken, arena_, "Evaltoken")(token);
                pToken->pNext_ = nullptr;
                if (pLasttoken)
                    pLasttoken->pNext_ = pToken;
                else
                    pFirsttoken = pToken;

                pLasttoken = pToken;
            }
            else {
                //then it must be a define
                pDefine = FindDefine(token);
                if (!pDefine) {
                    XParser::Error("can't Evaluate '%.*s', not defined", token.length(), token.begin());
                    return false;
                }
                if (!XParser::ExpandDefineIntoSource(token, pDefine)) {
                    return false;
                }
            }
        }
        else if (token.GetType() == TokenType::NUMBER || token.GetType() == TokenType::PUNCTUATION) {
            pToken = X_NEW(XLexToken, arena_, "Evaltoken")(token);
            pToken->pNext_ = nullptr;
            if (pLasttoken)
                pLasttoken->pNext_ = pToken;
            else
                pFirsttoken = pToken;

            pLasttoken = pToken;
        }
        else {
            XParser::Error("can't Evaluate '%.*s'", token.length(), token.begin());
            return false;
        }
    } while (XParser::ReadLine(token));

    if (!XParser::EvaluateTokens(pFirsttoken, pIntvalue, pFloatvalue, isInteger)) {
        return false;
    }

    // #define DEBUG_EVAL 0

#ifdef DEBUG_EVAL
    X_LOG0("Parse", "eval:");
#endif //DEBUG_EVAL
    for (pToken = pFirsttoken; pToken; pToken = pNexttoken) {
#ifdef DEBUG_EVAL
        X_LOG0("Parse", " %.*s", pToken->length(), pToken->begin());
#endif //DEBUG_EVAL
        pNexttoken = pToken->pNext_;
        X_DELETE(pToken, arena_);
    }

#ifdef DEBUG_EVAL
    if (isInteger) {
        X_LOG0("Parse", "eval result: %d", *pIntvalue);
    }
    else {
        X_LOG0("Parse", "eval result: %f", *pFloatvalue);
    }
#endif

    return true;
}

bool XParser::EvaluateTokens(XLexToken* pTokens, int32_t* pIntvalue, double* pFloatvalue, bool isInteger)
{
    if (pIntvalue) {
        *pIntvalue = 0;
    }
    if (pFloatvalue) {
        *pFloatvalue = 0;
    }

    bool brace = false;
    bool parentheses = false;
    bool lastwasvalue = false;
    bool negativevalue = false;
    bool error = false;

    X_UNUSED(isInteger);
    X_UNUSED(brace);

    XLexToken* pToken = nullptr;

    core::FixedArray<Value, 64> values;

    for (pToken = pTokens; pToken; pToken = pToken->pNext_) {
        switch (pToken->GetType()) {
            case TokenType::NAME:
                X_ASSERT_NOT_IMPLEMENTED();
                break;

            case TokenType::NUMBER: {
                if (lastwasvalue) {
                    XParser::Error("syntax error in #if/#elif");
                    error = true;
                    break;
                }

                auto& v = values.AddOne();

                if (negativevalue) {
                    v.intvalue = -pToken->GetIntValue();
                    v.floatvalue = -pToken->GetFloatValue();
                }
                else {
                    v.intvalue = pToken->GetIntValue();
                    v.floatvalue = pToken->GetFloatValue();
                }

                v.parentheses = parentheses;

                //last token was a value
                lastwasvalue = true;
                negativevalue = false;
                break;
            }

            case TokenType::PUNCTUATION:
                X_ASSERT_NOT_IMPLEMENTED();
                break;

            default:
                XParser::Error("unknown '%.*s' in #if/#elif", pToken->length(), pToken->begin());
                error = true;
                break;
        }
    }

    if (!error) {
        if (!lastwasvalue) {
            XParser::Error("trailing operator in #if/#elif");
            error = true;
        }
        else if (parentheses) {
            XParser::Error("too many ( in #if/#elif");
            error = true;
        }
    }

    if (values.isNotEmpty()) {
        auto& v = values.back();
        if (pIntvalue) {
            *pIntvalue = v.intvalue;
        }
        if (pFloatvalue) {
            *pFloatvalue = v.floatvalue;
        }
    }

    return !error;
}

bool XParser::CheckTokenString(const char* string)
{
    XLexToken tok;

    if (!XParser::ReadToken(tok)) {
        return false;
    }
    //if the token is available
    if (tok.isEqual(string)) {
        return true;
    }

    UnreadSourceToken(tok);
    return false;
}

int XParser::FindDefineParm(MacroDefine* define, const XLexToken& token)
{
    XLexToken* p;
    int i;

    i = 0;
    for (p = define->pParms; p; p = p->pNext_) {
        if (p->isEqual(token.begin(), token.end())) {
            return i;
        }
        i++;
    }
    return -1;
}

int XParser::ReadLine(XLexToken& token)
{
    int crossline = 0;

    do {
        if (!ReadSourceToken(token)) {
            return false;
        }

        if (token.linesCrossed_ > crossline) {
            UnreadSourceToken(token);
            return false;
        }

        crossline = 1;
    } while (token.isEqual("\\"));

    return true;
}

int XParser::ReadDefineParms(MacroDefine* pDefine, XLexToken** parms, int maxparms)
{
    MacroDefine* newdefine;
    XLexToken token, *t, *last;
    int i, done, lastcomma, numparms, indent;

    if (!ReadSourceToken(token)) {
        Error("define '%s' missing parameters", pDefine->name.c_str());
        return false;
    }

    if (pDefine->numParams > maxparms) {
        Error("define with more than %d parameters", maxparms);
        return false;
    }

    for (i = 0; i < pDefine->numParams; i++) {
        parms[i] = nullptr;
    }
    // if no leading "("
    if (!token.isEqual("(")) {
        UnreadSourceToken(token);
        Error("define '%s' missing parameters", pDefine->name.c_str());
        return false;
    }
    // read the define parameters
    for (done = 0, numparms = 0, indent = 1; !done;) {
        if (numparms >= maxparms) {
            Error("define '%s' with too many parameters", pDefine->name.c_str());
            return false;
        }
        parms[numparms] = nullptr;
        lastcomma = 1;
        last = nullptr;
        while (!done) {
            if (!ReadSourceToken(token)) {
                Error("define '%s' incomplete", pDefine->name.c_str());
                return false;
            }

            if (token.isEqual(",")) {
                if (indent <= 1) {
                    if (lastcomma) {
                        Warning("too many comma's");
                    }
                    if (numparms >= pDefine->numParams) {
                        Warning("too many define parameters");
                    }
                    lastcomma = 1;
                    break;
                }
            }
            else if (token.isEqual("(")) {
                indent++;
            }
            else if (token.isEqual(")")) {
                indent--;
                if (indent <= 0) {
                    if (!parms[pDefine->numParams - 1]) {
                        Warning("too few define parameters");
                    }
                    done = 1;
                    break;
                }
            }
            else if (token.GetType() == TokenType::NAME) {
                newdefine = FindDefine(token);
                if (newdefine) {
                    if (!ExpandDefineIntoSource(token, newdefine)) {
                        return false;
                    }
                    continue;
                }
            }

            lastcomma = 0;

            if (numparms < pDefine->numParams) {
                t = X_NEW(XLexToken, arena_, "Token")(token);
                t->pNext_ = nullptr;
                if (last) {
                    last->pNext_ = t;
                }
                else {
                    parms[numparms] = t;
                }
                last = t;
            }
        }
        numparms++;
    }
    return true;
}

bool XParser::ExpandDefine(XLexToken& deftoken, MacroDefine* pDefine,
    XLexToken*& firsttoken, XLexToken*& lasttoken)
{
    XLexToken* parms[MAX_DEFINEPARMS];
    XLexToken *dt, *t;
    XLexToken *first, *last, token;
    XLexToken *pt, *nextpt;
    int i, parmnum;

    // empty list at first
    first = nullptr;
    last = nullptr;

    // if the define has parameters
    if (pDefine->numParams) {
        if (!ReadDefineParms(pDefine, parms, MAX_DEFINEPARMS)) {
            return false;
        }
    }

    // create a list with tokens of the expanded define
    for (dt = pDefine->pTokens; dt; dt = dt->pNext_) {
        parmnum = -1;
        // if the token is a name, it could be a define parameter
        if (dt->GetType() == TokenType::NAME) {
            parmnum = FindDefineParm(pDefine, *dt);
        }
        // if it is a define parameter
        if (parmnum >= 0) {
            for (pt = parms[parmnum]; pt; pt = pt->pNext_) {
                t = X_NEW(XLexToken, arena_, "TokenParam")(*pt);
                //add the token to the list
                t->pNext_ = nullptr;
                if (last)
                    last->pNext_ = t;
                else
                    first = t;
                last = t;
            }
        }
        else {
            // if stringizing operator
            if (dt->isEqual("#")) {
                t = nullptr; // warn 4701 fix.
                X_ASSERT_NOT_IMPLEMENTED();
            }
            else {
                t = X_NEW(XLexToken, arena_, "DefineToken")(*dt);
                t->line_ = deftoken.GetLine();
            }

            // add the token to the list
            t->pNext_ = nullptr;
            // the token being read from the define list should use the line number of
            // the original file, not the header file
            t->line_ = deftoken.GetLine();

            if (last) {
                last->pNext_ = t;
            }
            else {
                first = t;
            }

            last = t;
        }
    }

    // store the first and last token of the list
    firsttoken = first;
    lasttoken = last;

    // free all the parameter tokens
    for (i = 0; i < pDefine->numParams; i++) {
        for (pt = parms[i]; pt; pt = nextpt) {
            nextpt = pt->pNext_;
            X_DELETE(pt, arena_);
        }
    }

    return true;
}

bool XParser::ExpandDefineIntoSource(XLexToken& token, MacroDefine* pDefine)
{
    X_ASSERT_NOT_NULL(pDefine);

    XLexToken *firsttoken, *lasttoken;

    if (!ExpandDefine(token, pDefine, firsttoken, lasttoken)) {
        return false;
    }

    // if the define is not empty
    if (firsttoken && lasttoken) {
        firsttoken->linesCrossed_ += token.linesCrossed_;
        lasttoken->pNext_ = pTokens_;
        pTokens_ = firsttoken;
    }

    return true;
}

MacroDefine* XParser::FindDefine(XLexToken& token) const
{
    X_ASSERT(token.length() > 0, "invalid token passed, must have a length")
    (token.length());
    // little optermisation, which will work very well if all macro's
    // are upper case, since anything else in the file not starting with uppercase.
    // will fail this test.
    if (!isInCache(token.begin()[0])) {
        return nullptr;
    }

    // create a null term string.
    core::StackString512 temp(token.begin(), token.end());

    auto it = macros_.find(X_CONST_STRING(temp.c_str()));
    if (it != macros_.end()) {
        return it->second;
    }

    return nullptr;
}

void XParser::addDefinetoHash(MacroDefine* pDefine)
{
    X_ASSERT_NOT_NULL(pDefine);

    macros_.insert(MacroMap::value_type(pDefine->name, pDefine));
}

void XParser::removeDefine(MacroDefine* pDefine)
{
    macros_.erase(pDefine->name);
    X_DELETE(pDefine, arena_);
}

void XParser::Error(const char* str, ...)
{
    core::StackString<sizeof(core::LoggerBase::Line)> temp;

    va_list args;
    va_start(args, str);

    temp.appendFmt(str, args);

    va_end(args);
    X_ERROR("Parse", temp.c_str());
}

void XParser::Warning(const char* str, ...)
{
    core::StackString<sizeof(core::LoggerBase::Line)> temp;

    va_list args;
    va_start(args, str);

    temp.appendFmt(str, args);

    va_end(args);

    X_WARNING("Parse", temp.c_str());
}

X_NAMESPACE_END