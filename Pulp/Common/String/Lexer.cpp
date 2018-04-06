#include "EngineCommon.h"
#include "Lexer.h"

X_NAMESPACE_BEGIN(core)

namespace
{
    //longer punctuations_ first
    PunctuationPair s_default_punctuations_[] = {
        //binary operators
        {">>=", PunctuationId::RSHIFT_ASSIGN},
        {"<<=", PunctuationId::LSHIFT_ASSIGN},
        //
        {"...", PunctuationId::PARMS},
        //define merge operator
        {"##", PunctuationId::PRECOMPMERGE}, // pre-compiler
                                             //logic operators
        {"&&", PunctuationId::LOGIC_AND},    // pre-compiler
        {"||", PunctuationId::LOGIC_OR},     // pre-compiler
        {">=", PunctuationId::LOGIC_GEQ},    // pre-compiler
        {"<=", PunctuationId::LOGIC_LEQ},    // pre-compiler
        {"==", PunctuationId::LOGIC_EQ},     // pre-compiler
        {"!=", PunctuationId::LOGIC_UNEQ},   // pre-compiler
                                             //arithmatic operators
        {"*=", PunctuationId::MUL_ASSIGN},
        {"/=", PunctuationId::DIV_ASSIGN},
        {"%=", PunctuationId::MOD_ASSIGN},
        {"+=", PunctuationId::ADD_ASSIGN},
        {"-=", PunctuationId::SUB_ASSIGN},
        {"++", PunctuationId::INC},
        {"--", PunctuationId::DEC},
        //binary operators
        {"&=", PunctuationId::BIN_AND_ASSIGN},
        {"|=", PunctuationId::BIN_OR_ASSIGN},
        {"^=", PunctuationId::BIN_XOR_ASSIGN},
        {">>", PunctuationId::RSHIFT}, // pre-compiler
        {"<<", PunctuationId::LSHIFT}, // pre-compiler
                                       //reference operators
        {"->", PunctuationId::POINTERREF},
        //C++
        {"::", PunctuationId::CPP1},
        {".*", PunctuationId::CPP2},
        //arithmatic operators
        {"*", PunctuationId::MUL}, // pre-compiler
        {"/", PunctuationId::DIV}, // pre-compiler
        {"%", PunctuationId::MOD}, // pre-compiler
        {"+", PunctuationId::ADD}, // pre-compiler
        {"-", PunctuationId::SUB}, // pre-compiler
        {"=", PunctuationId::ASSIGN},
        //binary operators
        {"&", PunctuationId::BIN_AND},       // pre-compiler
        {"|", PunctuationId::BIN_OR},        // pre-compiler
        {"^", PunctuationId::BIN_XOR},       // pre-compiler
        {"~", PunctuationId::BIN_NOT},       // pre-compiler
                                             //logic operators
        {"!", PunctuationId::LOGIC_NOT},     // pre-compiler
        {">", PunctuationId::LOGIC_GREATER}, // pre-compiler
        {"<", PunctuationId::LOGIC_LESS},    // pre-compiler
                                             //reference operator
        {".", PunctuationId::REF},
        //seperators
        {",", PunctuationId::COMMA}, // pre-compiler
        {";", PunctuationId::SEMICOLON},
        //label indication
        {":", PunctuationId::COLON},            // pre-compiler
                                                //if statement
        {"?", PunctuationId::QUESTIONMARK},     // pre-compiler
                                                //embracements
        {"(", PunctuationId::PARENTHESESOPEN},  // pre-compiler
        {")", PunctuationId::PARENTHESESCLOSE}, // pre-compiler
        {"{", PunctuationId::BRACEOPEN},        // pre-compiler
        {"}", PunctuationId::BRACECLOSE},       // pre-compiler
        {"[", PunctuationId::SQBRACKETOPEN},
        {"]", PunctuationId::SQBRACKETCLOSE},
        //
        {"\\", PunctuationId::BACKSLASH},
        //precompiler operator
        {"#", PunctuationId::PRECOMP}, // pre-compiler
        {"$", PunctuationId::DOLLAR},
        {nullptr, PunctuationId::UNUSET}};

    int32_t s_default_punctuationtable[256];
    int32_t s_default_nextpunctuation[sizeof(s_default_punctuations_) / sizeof(PunctuationPair)];

} // namespace

void XLexToken::NumberValue(void)
{
    X_ASSERT(type_ == TokenType::NUMBER, "token is not a number")
    (type_);

    // make a nullterm string
    // lol wtf this is 10% of the function.
    core::StackString<128> temp(begin(), end());

    const char* p = temp.c_str();

    floatvalue_ = 0;
    intvalue_ = 0;

    // floating point number
    if (subtype_.IsSet(TokenSubType::FLOAT)) {
#if 0
		floatvalue_ = atof(p);
#else
        if (subtype_.IsSet(TokenSubType::INFINITE) && subtype_.IsSet(TokenSubType::INDEFINITE) && subtype_.IsSet(TokenSubType::NAN)) {
            if (subtype_.IsSet(TokenSubType::INFINITE)) {
                // 1.#INF
                uint32_t inf = 0x7f800000;
                floatvalue_ = static_cast<double>(*reinterpret_cast<float*>(&inf));
            }
            else if (subtype_.IsSet(TokenSubType::INDEFINITE)) {
                // 1.#IND
                uint32_t ind = 0xffc00000;
                floatvalue_ = static_cast<double>(*reinterpret_cast<float*>(&ind));
            }
            else if (subtype_.IsSet(TokenSubType::NAN)) {
                // 1.#QNAN
                uint32_t nan = 0x7fc00000;
                floatvalue_ = static_cast<double>(*reinterpret_cast<float*>(&nan));
            }
        }
        else {
            double m;
            bool div;
            bool negative = false;

            if (*p == '-') {
                negative = true;
                p++;
            }

            while (*p && *p != '.' && *p != 'e') {
                floatvalue_ = floatvalue_ * 10.0 + static_cast<double>(*p - '0');
                p++;
            }
            if (*p == '.') {
                p++;
                for (m = 0.1; *p && *p != 'e'; p++) {
                    floatvalue_ = floatvalue_ + static_cast<double>(*p - '0') * m;
                    m *= 0.1;
                }
            }
            if (*p == 'e') {
                p++;
                if (*p == '-') {
                    div = true;
                    p++;
                }
                else if (*p == '+') {
                    div = false;
                    p++;
                }
                else {
                    div = false;
                }

                int pow = 0;
                int i;
                for (pow = 0; *p; p++) {
                    pow = pow * 10 + static_cast<int>(*p - '0');
                }
                for (m = 1.0, i = 0; i < pow; i++) {
                    m *= 10.0;
                }
                if (div) {
                    floatvalue_ /= m;
                }
                else {
                    floatvalue_ *= m;
                }
            }

            if (negative) {
                floatvalue_ -= floatvalue_ * 2;
            }
        }
#endif
        intvalue_ = static_cast<long>(floatvalue_);
    }
    else if (subtype_.IsSet(TokenSubType::DECIMAL)) {
        bool negative = false;

        if (*p == '-') {
            negative = true;
            p++;
        }

        while (*p != ' ' && *p) {
            intvalue_ = intvalue_ * 10 + (*p - '0');
            p++;
        }

        if (negative) {
            intvalue_ = -intvalue_;
        }

        floatvalue_ = intvalue_;
    }
    else if (subtype_.IsSet(TokenSubType::IPADDRESS)) {
        int c = 0;
        while (*p && *p != ':') {
            if (*p == '.') {
                while (c != 3) {
                    intvalue_ = intvalue_ * 10;
                    c++;
                }
                c = 0;
            }
            else {
                intvalue_ = intvalue_ * 10 + (*p - '0');
                c++;
            }
            p++;
        }
        while (c != 3) {
            intvalue_ = intvalue_ * 10;
            c++;
        }
        floatvalue_ = intvalue_;
    }
    else if (subtype_.IsSet(TokenSubType::OCTAL)) {
        // step over the first zero
        p += 1;
        while (*p != ' ' && *p) {
            intvalue_ = (intvalue_ << 3) + (*p - '0');
            p++;
        }
        floatvalue_ = intvalue_;
    }
    else if (subtype_.IsSet(TokenSubType::HEX)) {
        // step over the leading 0x or 0X
        p += 2;
        while (*p != ' ' && *p) {
            intvalue_ <<= 4;
            if (*p >= 'a' && *p <= 'f')
                intvalue_ += *p - 'a' + 10;
            else if (*p >= 'A' && *p <= 'F')
                intvalue_ += *p - 'A' + 10;
            else
                intvalue_ += *p - '0';
            p++;
        }
        floatvalue_ = intvalue_;
    }
    else if (subtype_.IsSet(TokenSubType::BINARY)) {
        // step over the leading 0b or 0B
        p += 2;
        while (*p != ' ' && *p) {
            intvalue_ = (intvalue_ << 1) + (*p - '0');
            p++;
        }
        floatvalue_ = intvalue_;
    }

    subtype_ |= TokenSubType::VALUESVALID;
}

XLexer::XLexer() :
    XLexer(nullptr, nullptr, core::string())
{
}

XLexer::XLexer(const char* startInclusive, const char* endExclusive) :
    XLexer(startInclusive, endExclusive, core::string())
{
}

XLexer::XLexer(const char* startInclusive, const char* endExclusive, core::string name) :
    filename_(name),
    punctuations_(nullptr)
{
    start_ = startInclusive;
    current_ = startInclusive;
    lastp_ = startInclusive;
    end_ = endExclusive;

    curLine_ = 0;
    lastLine_ = 0;

    // default inits itself.
    // flags_ = 0;

    SetPunctuations(nullptr);

    tokenavailable_ = 0;

    errState_ = ErrorState::OK;
}

bool XLexer::SetMemory(const char* startInclusive, const char* endExclusive, core::string name)
{
    X_ASSERT(start_ == nullptr, "Can't set memory on a Lex that is already init.")
    (start_, end_, current_);

    if (start_) {
        return false;
    }

    filename_ = name;
    start_ = startInclusive;
    current_ = startInclusive;
    lastp_ = startInclusive;
    end_ = endExclusive;
    return true;
}

const char* XLexer::GetPunctuationFromId(PunctuationId::Enum id)
{
    for (int32_t i = 0; punctuations_[i].pCharacter; i++) {
        if (punctuations_[i].id == id) {
            return punctuations_[i].pCharacter;
        }
    }
    return "unknown punctuation";
}

void XLexer::CreatePunctuationTable(const PunctuationPair* punctuations)
{
    punctuationtable_ = s_default_punctuationtable;
    nextpunctuation_ = s_default_nextpunctuation;

    if (punctuations == s_default_punctuations_) {
        static core::CriticalSection setupcs;
        static bool setup = false;

        core::CriticalSection::ScopedLock lock(setupcs);

        if (setup) {
            return;
        }

        setup = true;
        std::memset(s_default_punctuationtable, 0xFF, sizeof(s_default_punctuationtable));
        std::memset(s_default_nextpunctuation, 0xFF, sizeof(s_default_nextpunctuation));

        int32_t n, lastp;
        const PunctuationPair *p, *newp;

        //add the punctuations_ in the list to the punctuation table
        for (int32_t i = 0; punctuations[i].pCharacter; i++) {
            newp = &punctuations[i];
            lastp = -1;

            //sort the punctuations_ in this table entry on length (longer punctuations_ first)
            for (n = punctuationtable_[static_cast<uint32_t>(newp->pCharacter[0])]; n >= 0; n = nextpunctuation_[n]) {
                p = &punctuations[n];
                if (strlen(p->pCharacter) < strlen(newp->pCharacter)) {
                    nextpunctuation_[i] = n;
                    if (lastp >= 0) {
                        nextpunctuation_[lastp] = i;
                    }
                    else {
                        punctuationtable_[static_cast<uint32_t>(newp->pCharacter[0])] = i;
                    }
                    break;
                }
                lastp = n;
            }
            if (n < 0) {
                nextpunctuation_[i] = -1;
                if (lastp >= 0) {
                    nextpunctuation_[lastp] = i;
                }
                else {
                    punctuationtable_[static_cast<uint32_t>(newp->pCharacter[0])] = i;
                }
            }
        }
    }
    else {
        X_ASSERT_NOT_IMPLEMENTED();
    }
}

void XLexer::SetPunctuations(const PunctuationPair* p)
{
    if (p) {
        CreatePunctuationTable(p);
        punctuations_ = p;
    }
    else {
        CreatePunctuationTable(s_default_punctuations_);
        punctuations_ = s_default_punctuations_;
    }
}

bool XLexer::ReadToken(XLexToken& token)
{
    token.Reset();

    if (tokenavailable_) {
        tokenavailable_ = 0;
        token = this->token_;
        return true;
    }

    lastLine_ = curLine_;

    if (!ReadWhiteSpace()) {
        return false;
    }

    if (isEOF()) {
        return false;
    }

    X_ASSERT(current_ < end_, "Trying to read from end of buffer")
    (current_, end_);

    // save script pointer
    lastp_ = current_;

    //	token->whiteSpaceEnd_p = script_p;
    // line the token is on
    token.line_ = curLine_;
    // number of lines crossed before token
    token.linesCrossed_ = curLine_ - lastLine_;
    // clear token flags
    //	token.flags_ = 0;

    int32_t c = *current_;

    // if we're keeping everything as whitespace deliminated strings
    if (flags_.IsSet(LexFlag::ONLYSTRINGS)) {
        // if there is a leading quote
        if (c == '\"' || c == '\'') {
            if (!ReadString(token, c)) {
                return false;
            }
        }
        else if (!ReadName(token)) {
            return false;
        }
    }
    // if there is a number
    else if ((c >= '0' && c <= '9') || ((c == '.' || c == '-') && (*(current_ + 1) >= '0' && *(current_ + 1) <= '9'))) {
        if (!ReadNumber(token)) {
            return false;
        }
        // if names are allowed to start with a number
        if (flags_.IsSet(LexFlag::ALLOWNUMBERNAMES)) {
            c = *current_;
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
                if (!ReadName(token)) {
                    return false;
                }
            }
        }
    }
    // if there is a leading quote
    else if (c == '\"' || c == '\'') {
        if (!ReadString(token, c)) {
            return false;
        }
    }
    // if there is a name
    else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
        if (!ReadName(token)) {
            return false;
        }
    }
    // names may also start with a slash when pathnames are allowed
    else if ((flags_.IsSet(LexFlag::ALLOWPATHNAMES)) && ((c == '/' || c == '\\') || c == '.')) {
        if (!ReadName(token)) {
            return false;
        }
    }
    else if (flags_.IsSet(LexFlag::ALLOWDOLLARNAMES) && c == '$' && ReadName(token)) {
        return true;
    }
    // check for punctuations_
    else if (!ReadPunctuation(token)) {
        Error("unknown punctuation %c", c);
        return false;
    }
    // succesfully read a token
    return true;
}

bool XLexer::ExpectTokenString(const char* string)
{
    XLexToken token;

    if (!ReadToken(token)) {
        Error("couldn't find expected '%s' EOF", string);
        return false;
    }
    if (!token.isEqual(string)) {
        Error("expected '%s' but found '%.*s:%" PRId32 "'", string, token.length(), token.begin(), GetLineNumber());
        return false;
    }
    return true;
}

bool XLexer::ExpectTokenType(TokenType::Enum type, XLexToken::TokenSubTypeFlags subtype,
    PunctuationId::Enum puncId, XLexToken& token)
{
    core::StackString<128> str;

    if (!ReadToken(token)) {
        Error("couldn't read expected token");
        return false;
    }

    if (token.GetType() != type) {
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
        Error("expected a %s but found '%.*s'", str.c_str(),
            token.length(), token.begin());
        return false;
    }

    if (token.GetType() == TokenType::NUMBER) {
        if ((token.GetSubType() & subtype) != subtype) {
            str.clear();
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
    else if (token.GetType() == TokenType::PUNCTUATION) {
        if (token.GetPuncId() != puncId) {
            Error("expected '%s' but found '%.*s'", GetPunctuationFromId(puncId),
                token.length(), token.begin());
            return false;
        }
    }
    return true;
}

int XLexer::ParseInt(void)
{
    XLexToken token;

    if (!ReadToken(token)) {
        Error("couldn't read expected integer");
        return 0;
    }
    if (token.GetType() == TokenType::PUNCTUATION && token.isEqual("-")) {
        ExpectTokenType(TokenType::NUMBER, TokenSubType::INTEGER,
            PunctuationId::UNUSET, token);

        return -(safe_static_cast<signed int>(token.GetIntValue()));
    }
    else if (token.GetType() != TokenType::NUMBER || token.GetSubType() == TokenSubType::FLOAT) {
        Error("expected integer value, found '%.*s'", token.length(), token.begin());
    }
    return token.GetIntValue();
}

bool XLexer::ParseBool(void)
{
    XLexToken token;

    if (!ReadToken(token)) {
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

float XLexer::ParseFloat()
{
    XLexToken token;

    if (!ReadToken(token)) {
        Error("couldn't read expected floating point number");
        return 0.f;
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

bool XLexer::Parse1DMatrix(int32_t x, float* m)
{
    X_ASSERT_NOT_NULL(m);

    if (!ExpectTokenString("(")) {
        return false;
    }

    for (int32_t i = 0; i < x; i++) {
        m[i] = ParseFloat();
    }

    if (!ExpectTokenString(")")) {
        return false;
    }
    return true;
}

bool XLexer::Parse2DMatrix(int32_t y, int32_t x, float* m)
{
    X_ASSERT_NOT_NULL(m);

    if (!ExpectTokenString("(")) {
        return false;
    }

    for (int32_t i = 0; i < y; i++) {
        if (!Parse1DMatrix(x, m + i * x)) {
            return false;
        }
    }

    if (!ExpectTokenString(")")) {
        return false;
    }
    return true;
}

bool XLexer::Parse3DMatrix(int32_t z, int32_t y, int32_t x, float* m)
{
    X_ASSERT_NOT_NULL(m);

    if (!ExpectTokenString("(")) {
        return false;
    }

    for (int32_t i = 0; i < z; i++) {
        if (!Parse2DMatrix(y, x, m + i * x * y)) {
            return false;
        }
    }

    if (!ExpectTokenString(")")) {
        return false;
    }
    return true;
}

void XLexer::UnreadToken(const XLexToken& token)
{
    this->token_ = token;
    tokenavailable_ = 1;
}

bool XLexer::ReadTokenOnLine(XLexToken& token)
{
    XLexToken tok;

    if (!ReadToken(tok)) {
        current_ = lastp_;
        curLine_ = lastLine_;
        return false;
    }
    // if no lines were crossed before this token
    if (!tok.linesCrossed_) {
        token = tok;
        return true;
    }
    // restore our position
    current_ = lastp_;
    curLine_ = lastLine_;
    return false;
}

bool XLexer::ReadWhiteSpace(void)
{
    if (isEOF()) {
        return true;
    }

    X_DISABLE_WARNING(4127)
    while (1)
        X_ENABLE_WARNING(4127)
        {
            // skip white space
            while (*current_ <= ' ') {
                if (isEOF()) {
                    return false;
                }
                if (*current_ == '\n') {
                    curLine_++;
                }
                current_++;
            }

            // skip comments
            if (*current_ == '/') {
                // comments //
                if (*(current_ + 1) == '/') {
                    current_++;
                    do {
                        current_++;
                        if (isEOF()) {
                            return false;
                        }
                    } while (*current_ != '\n');
                    curLine_++;
                    current_++;
                    if (isEOF()) {
                        return false;
                    }
                    continue;
                }
                // comments /* */
                else if (*(current_ + 1) == '*') {
                    current_++;
                    X_DISABLE_WARNING(4127)
                    while (1)
                        X_ENABLE_WARNING(4127)
                        {
                            current_++;
                            if (isEOF()) {
                                return false;
                            }
                            if (*current_ == '\n') {
                                curLine_++;
                            }
                            else if (*current_ == '/') {
                                if (*(current_ - 1) == '*') {
                                    break;
                                }
                                if (*(current_ + 1) == '*') {
                                    Warning("nested comment");
                                }
                            }
                        }
                    current_++;

                    if (isEOF()) {
                        return false;
                    }

                    current_++;

                    if (isEOF()) {
                        return false;
                    }

                    continue;
                }
            }
            break;
        }
    return true;
}

bool XLexer::ReadEscapeCharacter(char* ch)
{
    int c, val, i;

    // step over the leading '\\'
    current_++;
    // determine the escape character
    switch (*current_) {
        case '\\':
            c = '\\';
            break;
        case 'n':
            c = '\n';
            break;
        case 'r':
            c = '\r';
            break;
        case 't':
            c = '\t';
            break;
        case 'v':
            c = '\v';
            break;
        case 'b':
            c = '\b';
            break;
        case 'f':
            c = '\f';
            break;
        case 'a':
            c = '\a';
            break;
        case '\'':
            c = '\'';
            break;
        case '\"':
            c = '\"';
            break;
        case '\?':
            c = '\?';
            break;
        case 'x': {
            current_++;
            for (i = 0, val = 0;; i++, current_++) {
                c = *current_;
                if (c >= '0' && c <= '9')
                    c = c - '0';
                else if (c >= 'A' && c <= 'Z')
                    c = c - 'A' + 10;
                else if (c >= 'a' && c <= 'z')
                    c = c - 'a' + 10;
                else
                    break;
                val = (val << 4) + c;
            }
            current_--;
            if (val > 0xFF) {
                Warning("too large value in escape character");
                val = 0xFF;
            }
            c = val;
            break;
        }
        default: //NOTE: decimal ASCII code, NOT octal
        {
            if (*current_ < '0' || *current_ > '9') {
                Error("unknown escape char");
            }
            for (i = 0, val = 0;; i++, current_++) {
                c = *current_;
                if (c >= '0' && c <= '9')
                    c = c - '0';
                else
                    break;
                val = val * 10 + c;
            }
            current_--;
            if (val > 0xFF) {
                Warning("too large value in escape character");
                val = 0xFF;
            }
            c = val;
            break;
        }
    }
    // step over the escape character or the last digit of the number
    current_++;
    // store the escape character
    *ch = safe_static_cast<char>(c);
    // succesfully read escape character
    return true;
}

/*
================
Escape characters are interpretted.
Reads two strings with only a white space between them as one string.
================
*/
bool XLexer::ReadString(XLexToken& token, int32_t quote)
{
    if (quote == '\"') {
        token.SetType(TokenType::STRING);
    }
    else {
        token.SetType(TokenType::LITERAL);
    }

    // leading quote
    current_++;

    token.start_ = current_;

    X_DISABLE_WARNING(4127)
    while (1)
        X_ENABLE_WARNING(4127)
        {
            // if there is an escape character and escape characters are allowed
            if (*current_ == '\\' && !(flags_.IsSet(LexFlag::NOSTRINGESCAPECHARS))) {
                char ch;
                if (!ReadEscapeCharacter(&ch)) {
                    return 0;
                }
            }
            // if a trailing quote
            else if (*current_ == quote) {
                // step over the quote
                current_++;

                // if consecutive strings should not be concatenated
                if (flags_.IsSet(LexFlag::NOSTRINGCONCAT) && (!flags_.IsSet(LexFlag::ALLOWBACKSLASHSTRINGCONCAT) || (quote != '\"'))) {
                    break;
                }

                const char* tmpscript_p = current_;
                int32_t tmpline = curLine_;

                // read white space between possible two consecutive strings
                if (!ReadWhiteSpace()) {
                    current_ = tmpscript_p;
                    curLine_ = tmpline;
                    break;
                }

                if (flags_.IsSet(LexFlag::NOSTRINGCONCAT)) {
                    if (*current_ != '\\') {
                        current_ = tmpscript_p;
                        curLine_ = tmpline;
                        break;
                    }
                    // step over the '\\'
                    current_++;
                    if (!ReadWhiteSpace() || (*current_ != quote)) {
                        Error("expecting string after '\' terminated line");
                        return false;
                    }
                }

                // if there's no leading qoute
                if (*current_ != quote) {
                    current_ = tmpscript_p;
                    curLine_ = tmpline;
                    break;
                }
                // step over the new leading quote
                current_++;
            }
            else {
                if (*current_ == '\0') {
                    Error("missing trailing quote");
                    return false;
                }
                if (*current_ == '\n') {
                    Error("newline inside string");
                    return false;
                }

                current_++;
            }
        }

    token.end_ = (current_ - 1);

    if (token.GetType() == TokenType::LITERAL) {
        if (!(flags_.IsSet(LexFlag::ALLOWMULTICHARLITERALS))) {
            if (token.length() != 1) {
                Warning("literal is not one character long");
            }
        }
        token.subtype_ = TokenSubTypeFlags(token.begin()[0]);
    }
    else {
        // the sub type is the length of the string
        token.subtype_ = TokenSubTypeFlags(safe_static_cast<int32_t>(token.length()));
    }

    return true;
}

bool XLexer::ReadName(XLexToken& token)
{
    char c;

    token.SetType(TokenType::NAME);

    token.SetStart(current_);

    do {
        current_++;
        c = *current_;
    } while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' ||
             // if treating all tokens as strings, don't parse '-' as a seperate token
             (flags_.IsSet(LexFlag::ONLYSTRINGS) && (c == '-')) ||
             // if special path name characters are allowed
             (flags_.IsSet(LexFlag::ALLOWPATHNAMES) && (c == '/' || c == '\\' || c == ':' || c == '.'))); // while

    token.SetEnd(current_);

    return true;
}

bool XLexer::ReadNumber(XLexToken& token)
{
    int i;
    int dot, negative;
    char c, c2;

    token.SetType(TokenType::NUMBER);
    token.subtype_.Clear();
    token.intvalue_ = 0;
    token.floatvalue_ = 0;

    c = *current_;
    c2 = *(current_ + 1);

    token.start_ = current_;

    if (c == '0' && c2 != '.' && c2 != ' ') {
        // check for a hexadecimal number
        if (c2 == 'x' || c2 == 'X') {
            current_ += 2;
            c = *current_;
            while ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                c = *(++current_);
            }
            token.subtype_ = TokenSubType::HEX | TokenSubType::INTEGER;
        }
        // check for a binary number
        else if (c2 == 'b' || c2 == 'B') {
            current_ += 2;
            c = *current_;
            while (c == '0' || c == '1') {
                c = *(++current_);
            }
            token.subtype_ = TokenSubType::BINARY | TokenSubType::INTEGER;
        }
        // its an octal number
        else {
            current_++;
            c = *current_;
            while (c >= '0' && c <= '7') {
                c = *(++current_);
            }
            token.subtype_ = TokenSubType::OCTAL | TokenSubType::INTEGER;
        }
    }
    else {
        // decimal integer or floating point number or ip address
        dot = 0;
        negative = 0;
        X_DISABLE_WARNING(4127)
        while (true)
            X_ENABLE_WARNING(4127)
            {
                if (c >= '0' && c <= '9') {
                }
                else if (c == '.') {
                    dot++;
                }
                else if (c == '-') {
                    negative = 1;
                }
                else {
                    break;
                }
                c = *(++current_);
            }
        if (c == 'e' && dot == 0) {
            //We have scientific notation without a decimal point
            dot++;
        }
        // if a floating point number
        if (dot == 1) {
            token.SetSubType(TokenSubType::DECIMAL | TokenSubType::FLOAT);
            // check for floating point exponent
            if (c == 'e') {
                //Append the e so that GetFloatValue code works
                c = *(++current_);
                if (c == '-') {
                    c = *(++current_);
                }
                else if (c == '+') {
                    c = *(++current_);
                }
                while (c >= '0' && c <= '9') {
                    c = *(++current_);
                }
            }
            // check for floating point exception infinite 1.#INF or indefinite 1.#IND or NaN
            else if (c == '#') {
                c2 = 4;
                if (CheckString("INF")) {
                    token.subtype_ |= TokenSubType::INFINITE;
                }
                else if (CheckString("IND")) {
                    token.subtype_ |= TokenSubType::INDEFINITE;
                }
                else if (CheckString("NAN")) {
                    token.subtype_ |= TokenSubType::NAN;
                }
                else if (CheckString("QNAN")) {
                    token.subtype_ |= TokenSubType::NAN;
                    c2++;
                }
                else if (CheckString("SNAN")) {
                    token.subtype_ |= TokenSubType::NAN;
                    c2++;
                }
                for (i = 0; i < c2; i++) {
                    c = *(++current_);
                }
                while (c >= '0' && c <= '9') {
                    c = *(++current_);
                }
                if (!flags_.IsSet(LexFlag::ALLOWFLOATEXCEPTIONS)) {
                    Error("Float exception detected '%.*s'",
                        token.length(), token.begin());
                }
            }
        }
        else if (dot > 1) {
            if (!flags_.IsSet(LexFlag::ALLOWIPADDRESSES)) {
                Error("More than one dot in number");
                return false;
            }
            if (dot != 3) {
                Error("Ip address should have three dots");
                return false;
            }
            token.subtype_ = TokenSubType::IPADDRESS;
        }
        else {
            token.subtype_ = TokenSubType::DECIMAL | TokenSubType::INTEGER;
        }
    }

    if (token.subtype_.IsSet(TokenSubType::FLOAT)) {
        if (c > ' ') {
            // single-precision: float
            if (c == 'f' || c == 'F') {
                token.subtype_ |= TokenSubType::SINGLE_PRECISION;
                current_++;
            }
            // extended-precision: long double
            else if (c == 'l' || c == 'L') {
                token.subtype_ |= TokenSubType::EXTENDED_PRECISION;
                current_++;
            }
            // default is double-precision: double
            else {
                token.subtype_ |= TokenSubType::DOUBLE_PRECISION;
            }
        }
        else {
            token.subtype_ |= TokenSubType::DOUBLE_PRECISION;
        }
    }
    else if (token.subtype_.IsSet(TokenSubType::INTEGER)) {
        if (c > ' ') {
            // default: signed long
            for (i = 0; i < 2; i++) {
                // long integer
                if (c == 'l' || c == 'L') {
                    token.subtype_ |= TokenSubType::LONG;
                }
                // unsigned integer
                else if (c == 'u' || c == 'U') {
                    token.subtype_ |= TokenSubType::UNSIGNED;
                }
                else {
                    break;
                }
                c = *(++current_);
            }
        }
    }
    else if (token.subtype_.IsSet(TokenSubType::IPADDRESS)) {
        if (c == ':') {
            c = *(++current_);
            while (c >= '0' && c <= '9') {
                c = *(++current_);
            }
            token.subtype_ |= TokenSubType::IPPORT;
        }
    }

    token.end_ = current_;
    return true;
}

bool XLexer::ReadPunctuation(XLexToken& token)
{
    int l, n;
    const char* p;
    const PunctuationPair* punc;

    for (n = punctuationtable_[static_cast<int>(*current_)]; n >= 0; n = nextpunctuation_[n]) {
        punc = &(punctuations_[n]);
        p = punc->pCharacter;

        // check for this punctuation in the script
        for (l = 0; p[l] && current_[l]; l++) {
            if (current_[l] != p[l]) {
                break;
            }
        }
        if (!p[l]) {
            token.start_ = current_;
            token.end_ = current_ + 1;

            current_ += l;
            token.SetType(TokenType::PUNCTUATION);
            token.puncId_ = punc->id;
            return true;
        }
    }
    return false;
}

bool XLexer::PeekTokenString(const char* string)
{
    XLexToken tok;

    if (!ReadToken(tok)) {
        return false;
    }

    // unread token
    current_ = lastp_;
    curLine_ = lastLine_;

    // if the given string is available
    if (tok.isEqual(string)) {
        return true;
    }
    return false;
}

bool XLexer::PeekTokenType(TokenType::Enum type, XLexToken::TokenSubTypeFlags subtype,
    PunctuationId::Enum puncId, XLexToken& token)
{
    XLexToken tok;

    if (!ReadToken(tok)) {
        return false;
    }

    // unread token
    current_ = lastp_;
    curLine_ = lastLine_;

    if (tok.GetType() == type) {
        if (type == TokenType::PUNCTUATION && tok.GetPuncId() == puncId) {
            token = tok;
            return true;
        }
        if (tok.GetSubType() == subtype) {
            token = tok;
            return true;
        }
    }

    return false;
}

bool XLexer::SkipUntilString(const char* string)
{
    XLexToken token;

    while (ReadToken(token)) {
        if (token.isEqual(string)) {
            return true;
        }
    }
    return false;
}

bool XLexer::SkipRestOfLine(void)
{
    XLexToken token;

    while (ReadToken(token)) {
        if (token.linesCrossed_) {
            current_ = lastp_;
            curLine_ = lastLine_;
            return true;
        }
    }
    return false;
}

void XLexer::Error(const char* str, ...)
{
    core::StackString<1024> temp;

    errState_ = ErrorState::ERRORS;

    va_list args;
    va_start(args, str);

    temp.appendFmt(str, args);
    temp.appendFmt(" Line: %i", this->curLine_);

    va_end(args);
    X_ERROR("Lex", temp.c_str());
}

void XLexer::Warning(const char* str, ...)
{
    core::StackString<1024> temp;

    errState_ = ErrorState::WARNINGS;

    va_list args;
    va_start(args, str);
    temp.appendFmt(str, args);
    va_end(args);

    X_WARNING("Lex", temp.c_str());
}

X_NAMESPACE_END