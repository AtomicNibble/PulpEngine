#include "EngineCommon.h"
#include "Lexer.h"

#include <String\StackString.h>

X_NAMESPACE_BEGIN(core)


double XLexToken::GetDoubleValue(void) {
	if (type != TT_NUMBER) {
		return 0.0;
	}
	if (!(subtype & TT_VALUESVALID)) {
		NumberValue();
	}
	return floatvalue;
}

float XLexToken::GetFloatValue(void) {
	return (float)GetDoubleValue();
}

unsigned long	XLexToken::GetUnsignedLongValue(void) {
	if (type != TT_NUMBER) {
		return 0;
	}
	if (!(subtype & TT_VALUESVALID)) {
		NumberValue();
	}
	return intvalue;
}

int XLexToken::GetIntValue(void) {
	return (int)GetUnsignedLongValue();
}

void XLexToken::NumberValue(void)
{
	int i, pow, div, c, negative;
	const char* p;
	double m;

	X_ASSERT(type == TT_NUMBER, "token is not a number")(type);

	// make a nullterm string
	// lol wtf this is 10% of the function.
	core::StackString<128> temp(begin(), end());

	p = temp.c_str();
	floatvalue = 0;
	intvalue = 0;
	negative = 0;

	// floating point number
	if (subtype & TT_FLOAT) 
	{
#if 0
		floatvalue = atof(p);
#else
		if (subtype & (TT_INFINITE | TT_INDEFINITE | TT_NAN)) {
			if (subtype & TT_INFINITE) {			// 1.#INF
				unsigned int inf = 0x7f800000;
				floatvalue = (double)*(float*)&inf;
			}
			else if (subtype & TT_INDEFINITE) {	// 1.#IND
				unsigned int ind = 0xffc00000;
				floatvalue = (double)*(float*)&ind;
			}
			else if (subtype & TT_NAN) {			// 1.#QNAN
				unsigned int nan = 0x7fc00000;
				floatvalue = (double)*(float*)&nan;
			}
		}
		else 
		{
			if (*p == '-')
			{
				negative = 1;
				p++;
			}

			while (*p && *p != '.' && *p != 'e') {
				floatvalue = floatvalue * 10.0 + (double)(*p - '0');
				p++;
			}
			if (*p == '.') {
				p++;
				for (m = 0.1; *p && *p != 'e'; p++) {
					floatvalue = floatvalue + (double)(*p - '0') * m;
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
				pow = 0;
				for (pow = 0; *p; p++) {
					pow = pow * 10 + (int)(*p - '0');
				}
				for (m = 1.0, i = 0; i < pow; i++) {
					m *= 10.0;
				}
				if (div) {
					floatvalue /= m;
				}
				else {
					floatvalue *= m;
				}
			}
			
			if (negative) {
				floatvalue -= floatvalue * 2;
			}
		}
#endif
		intvalue = (unsigned long)(floatvalue);
	}
	else if (subtype & TT_DECIMAL) 
	{
		if (*p == '-')
		{
			negative = 1;
			p++;
		}

		while (*p != ' ' && *p) {
			intvalue = intvalue * 10 + (*p - '0');
			p++;
		}

		if (negative) {
			intvalue = -intvalue;
		}

		floatvalue = intvalue;
	}
	else if (subtype & TT_IPADDRESS) {
		c = 0;
		while (*p && *p != ':') {
			if (*p == '.') {
				while (c != 3) {
					intvalue = intvalue * 10;
					c++;
				}
				c = 0;
			}
			else {
				intvalue = intvalue * 10 + (*p - '0');
				c++;
			}
			p++;
		}
		while (c != 3) {
			intvalue = intvalue * 10;
			c++;
		}
		floatvalue = intvalue;
	}
	else if (subtype & TT_OCTAL) {
		// step over the first zero
		p += 1;
		while (*p != ' ' && *p) {
			intvalue = (intvalue << 3) + (*p - '0');
			p++;
		}
		floatvalue = intvalue;
	}
	else if (subtype & TT_HEX) {
		// step over the leading 0x or 0X
		p += 2;
		while (*p != ' ' && *p) {
			intvalue <<= 4;
			if (*p >= 'a' && *p <= 'f')
				intvalue += *p - 'a' + 10;
			else if (*p >= 'A' && *p <= 'F')
				intvalue += *p - 'A' + 10;
			else
				intvalue += *p - '0';
			p++;
		}
		floatvalue = intvalue;
	}
	else if (subtype & TT_BINARY) {
		// step over the leading 0b or 0B
		p += 2;
		while (*p != ' ' && *p) {
			intvalue = (intvalue << 1) + (*p - '0');
			p++;
		}
		floatvalue = intvalue;
	}
	subtype |= TT_VALUESVALID;
}


//longer punctuations first
punctuation_t default_punctuations[] = {
	//binary operators
	{ ">>=", P_RSHIFT_ASSIGN },
	{ "<<=", P_LSHIFT_ASSIGN },
	//
	{ "...", P_PARMS },
	//define merge operator
	{ "##", P_PRECOMPMERGE },				// pre-compiler
	//logic operators
	{ "&&", P_LOGIC_AND },					// pre-compiler
	{ "||", P_LOGIC_OR },					// pre-compiler
	{ ">=", P_LOGIC_GEQ },					// pre-compiler
	{ "<=", P_LOGIC_LEQ },					// pre-compiler
	{ "==", P_LOGIC_EQ },					// pre-compiler
	{ "!=", P_LOGIC_UNEQ },				// pre-compiler
	//arithmatic operators
	{ "*=", P_MUL_ASSIGN },
	{ "/=", P_DIV_ASSIGN },
	{ "%=", P_MOD_ASSIGN },
	{ "+=", P_ADD_ASSIGN },
	{ "-=", P_SUB_ASSIGN },
	{ "++", P_INC },
	{ "--", P_DEC },
	//binary operators
	{ "&=", P_BIN_AND_ASSIGN },
	{ "|=", P_BIN_OR_ASSIGN },
	{ "^=", P_BIN_XOR_ASSIGN },
	{ ">>", P_RSHIFT },					// pre-compiler
	{ "<<", P_LSHIFT },					// pre-compiler
	//reference operators
	{ "->", P_POINTERREF },
	//C++
	{ "::", P_CPP1 },
	{ ".*", P_CPP2 },
	//arithmatic operators
	{ "*", P_MUL },						// pre-compiler
	{ "/", P_DIV },						// pre-compiler
	{ "%", P_MOD },						// pre-compiler
	{ "+", P_ADD },						// pre-compiler
	{ "-", P_SUB },						// pre-compiler
	{ "=", P_ASSIGN },
	//binary operators
	{ "&", P_BIN_AND },					// pre-compiler
	{ "|", P_BIN_OR },						// pre-compiler
	{ "^", P_BIN_XOR },					// pre-compiler
	{ "~", P_BIN_NOT },					// pre-compiler
	//logic operators
	{ "!", P_LOGIC_NOT },					// pre-compiler
	{ ">", P_LOGIC_GREATER },				// pre-compiler
	{ "<", P_LOGIC_LESS },					// pre-compiler
	//reference operator
	{ ".", P_REF },
	//seperators
	{ ",", P_COMMA },						// pre-compiler
	{ ";", P_SEMICOLON },
	//label indication
	{ ":", P_COLON },						// pre-compiler
	//if statement
	{ "?", P_QUESTIONMARK },				// pre-compiler
	//embracements
	{ "(", P_PARENTHESESOPEN },			// pre-compiler
	{ ")", P_PARENTHESESCLOSE },			// pre-compiler
	{ "{", P_BRACEOPEN },					// pre-compiler
	{ "}", P_BRACECLOSE },					// pre-compiler
	{ "[", P_SQBRACKETOPEN },
	{ "]", P_SQBRACKETCLOSE },
	//
	{ "\\", P_BACKSLASH },
	//precompiler operator
	{ "#", P_PRECOMP },					// pre-compiler
	{ "$", P_DOLLAR },
	{ NULL, 0 }
};

int default_punctuationtable[256];
int default_nextpunctuation[sizeof(default_punctuations) / sizeof(punctuation_t)];
bool default_setup = false;;




XLexer::XLexer(const char* startInclusive, const char* endExclusive)
{
	start_ = startInclusive;
	current_ = startInclusive;
	lastp_ = startInclusive;
	end_ = endExclusive;

	curLine_ = 0;
	lastLine_ = 0;

	flags_ = 0;

	SetPunctuations(NULL);

	tokenavailable = 0;
}

const char *XLexer::GetPunctuationFromId(int id) {
	int i;

	for (i = 0; punctuations[i].p; i++) {
		if (punctuations[i].n == id) {
			return punctuations[i].p;
		}
	}
	return "unkown punctuation";
}

void XLexer::CreatePunctuationTable(const punctuation_t *punctuations)
{
	int i, n, lastp;
	const punctuation_t *p, *newp;

	//get memory for the table
	if (punctuations == default_punctuations) {
		punctuationtable = default_punctuationtable;
		nextpunctuation = default_nextpunctuation;
		if (default_setup) {
			return;
		}
		default_setup = true;
		i = sizeof(default_punctuations) / sizeof(punctuation_t);
	}

	memset(punctuationtable, 0xFF, 256 * sizeof(int));
	memset(nextpunctuation, 0xFF, i * sizeof(int));
	//add the punctuations in the list to the punctuation table
	for (i = 0; punctuations[i].p; i++) {
		newp = &punctuations[i];
		lastp = -1;
		//sort the punctuations in this table entry on length (longer punctuations first)
		for (n = punctuationtable[(unsigned int)newp->p[0]]; n >= 0; n = nextpunctuation[n]) {
			p = &punctuations[n];
			if (strlen(p->p) < strlen(newp->p)) {
				nextpunctuation[i] = n;
				if (lastp >= 0) {
					nextpunctuation[lastp] = i;
				}
				else {
					punctuationtable[(unsigned int)newp->p[0]] = i;
				}
				break;
			}
			lastp = n;
		}
		if (n < 0) {
			nextpunctuation[i] = -1;
			if (lastp >= 0) {
				nextpunctuation[lastp] = i;
			}
			else {
				punctuationtable[(unsigned int)newp->p[0]] = i;
			}
		}
	}
}

void XLexer::SetPunctuations(const punctuation_t *p) 
{
#ifdef PUNCTABLE
	if (p) {
		CreatePunctuationTable(p);
	}
	else {
		CreatePunctuationTable(default_punctuations);
	}
#endif //PUNCTABLE
	if (p) {
		punctuations = p;
	}
	else {
		punctuations = default_punctuations;
	}
}

int	XLexer::ReadToken(XLexToken& token)
{
	int c;

	token.Reset();

	if (tokenavailable) {
		tokenavailable = 0;
		token = this->token;
		return 1;
	}

	lastLine_ = curLine_;

	if (!ReadWhiteSpace()) {
		return 0;
	}

	// save script pointer
	lastp_ = current_;

//	token->whiteSpaceEnd_p = script_p;
	// line the token is on
	token.line = curLine_;
	// number of lines crossed before token
	token.linesCrossed = curLine_ - lastLine_;
	// clear token flags
	token.flags = 0;

	c = *current_;

	// if we're keeping everything as whitespace deliminated strings
	if (flags_.IsSet(LexFlag::ONLYSTRINGS)) {
		// if there is a leading quote
		if (c == '\"' || c == '\'') {
			if (!ReadString(token, c)) {
				return 0;
			}
		}
		else if (!ReadName(token)) {
			return 0;
		}
	}
	// if there is a number
	else if ((c >= '0' && c <= '9') ||
		((c == '.' || c == '-') && (*(current_ + 1) >= '0' && *(current_ + 1) <= '9'))) 
	{
		if (!ReadNumber(token)) {
			return 0;
		}
		// if names are allowed to start with a number
		if (flags_.IsSet(LexFlag::ALLOWNUMBERNAMES)) {
			c = *current_;
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
				if (!ReadName(token)) {
					return 0;
				}
			}
		}
	}
	// if there is a leading quote
	else if (c == '\"' || c == '\'') {
		if (!ReadString(token, c)) {
			return 0;
		}
	}
	// if there is a name
	else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
		if (!ReadName(token)) {
			return 0;
		}
	}
	// names may also start with a slash when pathnames are allowed
	else if ((flags_.IsSet(LexFlag::ALLOWPATHNAMES)) && ((c == '/' || c == '\\') || c == '.')) {
		if (!ReadName(token)) {
			return 0;
		}
	}
	// check for punctuations
	else if (!ReadPunctuation(token)) {
		Error("unknown punctuation %c", c);
		return 0;
	}
	// succesfully read a token
	return 1;
}

int XLexer::ExpectTokenString(const char *string) {
	XLexToken token;

	if (!ReadToken(token)) {
		Error("couldn't find expected '%s' EOF", string);
		return 0;
	}
	if (!token.isEqual(string)) {
		Error("expected '%s' but found '%.*s'", string, token.length(), token.begin());
		return 0;
	}
	return 1;
}

/*
================
idLexer::ExpectTokenType
================
*/
int XLexer::ExpectTokenType(int type, int subtype, XLexToken& token) {

	core::StackString<128> str;

	if (!ReadToken(token)) {
		Error("couldn't read expected token");
		return 0;
	}

	if (token.type != type) {
		switch (type) {
			case TT_STRING: str.append("string"); break;
			case TT_LITERAL: str.append("literal"); break;
			case TT_NUMBER: str.append("number"); break;
			case TT_NAME: str.append("name"); break;
			case TT_PUNCTUATION: str.append("punctuation"); break;
			default: str.append("unknown type"); break;
		}
		Error("expected a %s but found '%.*s'", str.c_str(), token.length(), token.begin());
		return 0;
	}
	if (token.type == TT_NUMBER) {
		if ((token.subtype & subtype) != subtype) {
			str.clear();
			if (subtype & TT_DECIMAL) str.append("decimal ");
			if (subtype & TT_HEX) str.append("hex ");
			if (subtype & TT_OCTAL) str.append("octal ");
			if (subtype & TT_BINARY) str.append("binary ");
			if (subtype & TT_UNSIGNED) str.append("unsigned ");
			if (subtype & TT_LONG) str.append("long ");
			if (subtype & TT_FLOAT) str.append("float ");
			if (subtype & TT_INTEGER) str.append("integer ");
			str.stripTrailing(' ');
			Error("expected %s but found '%.*s'", str.c_str(), token.length(), token.begin());
			return 0;
		}
	}
	else if (token.type == TT_PUNCTUATION) {
		if (subtype < 0) {
			Error("BUG: wrong punctuation subtype");
			return 0;
		}
		if (token.subtype != subtype) {
			Error("expected '%s' but found '%.*s'", GetPunctuationFromId(subtype), token.length(), token.begin());
			return 0;
		}
	}
	return 1;
}


int XLexer::ParseInt(void) {
	XLexToken token;

	if (!ReadToken(token)) {
		Error("couldn't read expected integer");
		return 0;
	}
	if (token.type == TT_PUNCTUATION && token.isEqual("-")) {
		ExpectTokenType(TT_NUMBER, TT_INTEGER, token);
		return -((signed int)token.GetIntValue());
	}
	else if (token.type != TT_NUMBER || token.subtype == TT_FLOAT) {
		Error("expected integer value, found '%.*s'", token.length(), token.begin());
	}
	return token.GetIntValue();
}

/*
================
idLexer::ParseBool
================
*/
bool XLexer::ParseBool(void) {
	XLexToken token;

	if (!ExpectTokenType(TT_NUMBER, 0, token)) {
		Error("couldn't read expected boolean");
		return false;
	}
	return (token.GetIntValue() != 0);
}

/*
================
idLexer::ParseFloat
================
*/
float XLexer::ParseFloat() {
	XLexToken token;

	if (!ReadToken(token)) {
		Error("couldn't read expected floating point number");
		return 0;
	}
	if (token.type == TT_PUNCTUATION && token.isEqual("-")) {
		ExpectTokenType(TT_NUMBER, 0, token);
		return -token.GetFloatValue();
	}
	else if (token.type != TT_NUMBER) {
		Error("expected float value, found '%.*s'", token.length(), token.begin());
	}
	return token.GetFloatValue();
}

/*
================
idLexer::Parse1DMatrix
================
*/
int XLexer::Parse1DMatrix(int x, float *m) {
	int i;

	if (!ExpectTokenString("(")) {
		return false;
	}

	for (i = 0; i < x; i++) {
		m[i] = ParseFloat();
	}

	if (!ExpectTokenString(")")) {
		return false;
	}
	return true;
}

/*
================
idLexer::Parse2DMatrix
================
*/
int XLexer::Parse2DMatrix(int y, int x, float *m) {
	int i;

	if (!ExpectTokenString("(")) {
		return false;
	}

	for (i = 0; i < y; i++) {
		if (!Parse1DMatrix(x, m + i * x)) {
			return false;
		}
	}

	if (!ExpectTokenString(")")) {
		return false;
	}
	return true;
}


int XLexer::Parse3DMatrix(int z, int y, int x, float *m) {
	int i;

	if (!ExpectTokenString("(")) {
		return false;
	}

	for (i = 0; i < z; i++) {
		if (!Parse2DMatrix(y, x, m + i * x*y)) {
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
	this->token = token;
	tokenavailable = 1;
}

int XLexer::ReadTokenOnLine(XLexToken& token)
{
	XLexToken tok;

	if (!ReadToken(tok)) {
		current_ = lastp_;
		curLine_ = lastLine_;
		return false;
	}
	// if no lines were crossed before this token
	if (!tok.linesCrossed) {
		token = tok;
		return true;
	}
	// restore our position
	current_ = lastp_;
	curLine_ = lastLine_;
//	token.Reset();
	return false;
}

int XLexer::ReadWhiteSpace(void)
{
	while (1) {
		// skip white space
		while (*current_ <= ' ') {
			if (!*current_) {
				return 0;
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
					if (!*current_) {
						return 0;
					}
				} while (*current_ != '\n');
				curLine_++;
				current_++;
				if (!*current_) {
					return 0;
				}
				continue;
			}
			// comments /* */
			else if (*(current_ + 1) == '*') {
				current_++;
				while (1) {
					current_++;
					if (!*current_) {
						return 0;
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
				if (!*current_) {
					return 0;
				}
				current_++;
				if (!*current_) {
					return 0;
				}
				continue;
			}
		}
		break;
	}
	return 1;
}

int XLexer::ReadEscapeCharacter(char *ch) {
	int c, val, i;

	// step over the leading '\\'
	current_++;
	// determine the escape character
	switch (*current_) {
		case '\\': c = '\\'; break;
		case 'n': c = '\n'; break;
		case 'r': c = '\r'; break;
		case 't': c = '\t'; break;
		case 'v': c = '\v'; break;
		case 'b': c = '\b'; break;
		case 'f': c = '\f'; break;
		case 'a': c = '\a'; break;
		case '\'': c = '\''; break;
		case '\"': c = '\"'; break;
		case '\?': c = '\?'; break;
		case 'x':
		{
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
	*ch = c;
	// succesfully read escape character
	return 1;
}

/*
================
idLexer::ReadString

Escape characters are interpretted.
Reads two strings with only a white space between them as one string.
================
*/
int XLexer::ReadString(XLexToken& token, int quote) {
	int tmpline;
	const char *tmpscript_p;
	char ch;

	if (quote == '\"') {
		token.type = TT_STRING;
	}
	else {
		token.type = TT_LITERAL;
	}

	// leading quote
	current_++;

	token.start_ = current_;

	while (1) {
		// if there is an escape character and escape characters are allowed
		if (*current_ == '\\' && !(flags_.IsSet(LexFlag::NOSTRINGESCAPECHARS))) {
			if (!ReadEscapeCharacter(&ch)) {
				return 0;
			}
			//			token->AppendDirty(ch);
		}
		// if a trailing quote
		else if (*current_ == quote) {
			// step over the quote
			current_++;
			// if consecutive strings should not be concatenated
			if (flags_.IsSet(LexFlag::NOSTRINGCONCAT) &&
				(!flags_.IsSet(LexFlag::ALLOWBACKSLASHSTRINGCONCAT) || (quote != '\"'))) {
				break;
			}

			tmpscript_p = current_;
			tmpline = this->curLine_;
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
					return 0;
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
				return 0;
			}
			if (*current_ == '\n') {
				Error("newline inside string");
				return 0;
			}

			current_++;
			//			token->AppendDirty(*current_++);
		}
	}
	//	token->data[token->len] = '\0';

	token.end_ = (current_-1);

	if (token.type == TT_LITERAL) {
		if (!(flags_.IsSet(LexFlag::ALLOWMULTICHARLITERALS))) {
			if (token.length() != 1) {
				Warning("literal is not one character long");
			}
		}
		token.subtype = token.begin()[0];
	}
	else {
		// the sub type is the length of the string
		token.subtype = (int)token.length();
	}

	return 1;
}


int XLexer::ReadName(XLexToken& token) {
	char c;

	token.type = TT_NAME;

	token.start_ = current_;

	do {
		//		token->AppendDirty(*current_++);
		current_++;
		c = *current_;
	} while ((c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') ||
		c == '_' ||
		// if treating all tokens as strings, don't parse '-' as a seperate token
		(flags_.IsSet(LexFlag::ONLYSTRINGS) && (c == '-')) ||
		// if special path name characters are allowed
		(flags_.IsSet(LexFlag::ALLOWPATHNAMES) && (c == '/' || c == '\\' || c == ':' || c == '.')));
	//	token->data[token->len] = '\0';
	//the sub type is the length of the name
	//	token.subtype = token->Length();
	token.end_ = current_;

	return 1;
}


X_INLINE int XLexer::CheckString(const char *str) const {
	int i;

	for (i = 0; str[i]; i++) {
		if (current_[i] != str[i]) {
			return false;
		}
	}
	return true;
}

int XLexer::ReadNumber(XLexToken& token) {
	int i;
	int dot, negative;
	char c, c2;

	token.type = TT_NUMBER;
	token.subtype = 0;
	token.intvalue = 0;
	token.floatvalue = 0;

	c = *current_;
	c2 = *(current_ + 1);

	token.start_ = current_;

	if (c == '0' && c2 != '.' && c2 != ' ') {
		// check for a hexadecimal number
		if (c2 == 'x' || c2 == 'X') {
			//	token->AppendDirty(*current_++);
			//	token->AppendDirty(*current_++);
			current_ += 2;
			c = *current_;
			while ((c >= '0' && c <= '9') ||
				(c >= 'a' && c <= 'f') ||
				(c >= 'A' && c <= 'F')) {
				//		token->AppendDirty(c);
				c = *(++current_);
			}
			token.subtype = TT_HEX | TT_INTEGER;
		}
		// check for a binary number
		else if (c2 == 'b' || c2 == 'B') {
			//	token->AppendDirty(*current_++);
			//	token->AppendDirty(*current_++);
			current_ += 2;
			c = *current_;
			while (c == '0' || c == '1') {
				//	token->AppendDirty(c);
				c = *(++current_);
			}
			token.subtype = TT_BINARY | TT_INTEGER;
		}
		// its an octal number
		else {
			//		token->AppendDirty(*current_++);
			current_++;
			c = *current_;
			while (c >= '0' && c <= '7') {
				//			token->AppendDirty(c);
				c = *(++current_);
			}
			token.subtype = TT_OCTAL | TT_INTEGER;
		}
	}
	else {
		// decimal integer or floating point number or ip address
		dot = 0;
		negative = 0;
		while (1) {
			if (c >= '0' && c <= '9') {
			}
			else if (c == '.' ) {
				dot++;
			}
			else if (c == '-') {
				negative = 1;
			}
			else {
				break;
			}
			//		token->AppendDirty(c);
			c = *(++current_);
		}
		if (c == 'e' && dot == 0) {
			//We have scientific notation without a decimal point
			dot++;
		}
		// if a floating point number
		if (dot == 1) {
			token.subtype = TT_DECIMAL | TT_FLOAT;
			// check for floating point exponent
			if (c == 'e') {
				//Append the e so that GetFloatValue code works
				//			token->AppendDirty(c);
				c = *(++current_);
				if (c == '-') {
					//				token->AppendDirty(c);
					c = *(++current_);
				}
				else if (c == '+') {
					//				token->AppendDirty(c);
					c = *(++current_);
				}
				while (c >= '0' && c <= '9') {
					//				token->AppendDirty(c);
					c = *(++current_);
				}
			}
			// check for floating point exception infinite 1.#INF or indefinite 1.#IND or NaN
			else if (c == '#') {
				c2 = 4;
				if (CheckString("INF")) {
					token.subtype |= TT_INFINITE;
				}
				else if (CheckString("IND")) {
					token.subtype |= TT_INDEFINITE;
				}
				else if (CheckString("NAN")) {
					token.subtype |= TT_NAN;
				}
				else if (CheckString("QNAN")) {
					token.subtype |= TT_NAN;
					c2++;
				}
				else if (CheckString("SNAN")) {
					token.subtype |= TT_NAN;
					c2++;
				}
				for (i = 0; i < c2; i++) {
					//				token->AppendDirty(c);
					c = *(++current_);
				}
				while (c >= '0' && c <= '9') {
					//				token->AppendDirty(c);
					c = *(++current_);
				}
				if (!flags_.IsSet(LexFlag::ALLOWFLOATEXCEPTIONS)) {
					//				token->AppendDirty(0);	// zero terminate for c_str
					//				Error("parsed %s", token->c_str());
				}
			}
		}
		else if (dot > 1) {
			if (!flags_.IsSet(LexFlag::ALLOWIPADDRESSES)) {
				Error("more than one dot in number");
				return 0;
			}
			if (dot != 3) {
				Error("ip address should have three dots");
				return 0;
			}
			token.subtype = TT_IPADDRESS;
		}
		else {
			token.subtype = TT_DECIMAL | TT_INTEGER;
		}
	}

	if (token.subtype & TT_FLOAT) {
		if (c > ' ') {
			// single-precision: float
			if (c == 'f' || c == 'F') {
				token.subtype |= TT_SINGLE_PRECISION;
				current_++;
			}
			// extended-precision: long double
			else if (c == 'l' || c == 'L') {
				token.subtype |= TT_EXTENDED_PRECISION;
				current_++;
			}
			// default is double-precision: double
			else {
				token.subtype |= TT_DOUBLE_PRECISION;
			}
		}
		else {
			token.subtype |= TT_DOUBLE_PRECISION;
		}
	}
	else if (token.subtype & TT_INTEGER) {
		if (c > ' ') {
			// default: signed long
			for (i = 0; i < 2; i++) {
				// long integer
				if (c == 'l' || c == 'L') {
					token.subtype |= TT_LONG;
				}
				// unsigned integer
				else if (c == 'u' || c == 'U') {
					token.subtype |= TT_UNSIGNED;
				}
				else {
					break;
				}
				c = *(++current_);
			}
		}
	}
	else if (token.subtype & TT_IPADDRESS) {
		if (c == ':') {
			//		token->AppendDirty(c);
			c = *(++current_);
			while (c >= '0' && c <= '9') {
				//			token->AppendDirty(c);
				c = *(++current_);
			}
			token.subtype |= TT_IPPORT;
		}
	}
	//	token->data[token->len] = '\0';

	token.end_ = current_;

	return 1;
}

/*
================
idLexer::ReadPunctuation
================
*/
int XLexer::ReadPunctuation(XLexToken& token) {
	int l, n, i;
	char *p;
	const punctuation_t *punc;

#ifdef PUNCTABLE
	for (n = punctuationtable[(unsigned int)*(current_)]; n >= 0; n = nextpunctuation[n])
	{
		punc = &(punctuations[n]);
#else
	int i;

	for (i = 0; punctuations[i].p; i++) {
		punc = &punctuations[i];
#endif
		p = punc->p;
		// check for this punctuation in the script
		for (l = 0; p[l] && current_[l]; l++) {
			if (current_[l] != p[l]) {
				break;
			}
		}
		if (!p[l]) {
			//
			//		token->EnsureAlloced(l + 1, false);
			for (i = 0; i <= l; i++) {
				//			token->data[i] = p[i];
			}
			//		token->len = l;
			//
			token.start_ = current_;
			token.end_ = current_ + 1;

			current_ += l;
			token.type = TT_PUNCTUATION;
			// sub type is the punctuation id
			token.subtype = punc->n;
			return 1;
		}
	}
	return 0;
}


int XLexer::PeekTokenString(const char *string) {
	XLexToken tok;

	if (!ReadToken(tok)) {
		return 0;
	}

	// unread token
	current_ = lastp_;
	curLine_ = lastLine_;

	// if the given string is available
	if (tok.isEqual(string)) {
		return 1;
	}
	return 0;
}

int XLexer::PeekTokenType(int type, int subtype, XLexToken& token) {
	XLexToken tok;

	if (!ReadToken(tok)) {
		return 0;
	}

	// unread token
	current_ = lastp_;
	curLine_ = lastLine_;

	// if the type matches
	if (tok.type == type && (tok.subtype & subtype) == subtype) {
		token = tok;
		return 1;
	}
	return 0;
}

int XLexer::SkipUntilString(const char *string) {
	XLexToken token;

	while (ReadToken(token)) {
		if (token.isEqual(string)) {
			return 1;
		}
	}
	return 0;
}

int XLexer::SkipRestOfLine(void) {
	XLexToken token;

	while (ReadToken(token)) {
		if (token.linesCrossed) {
			current_ = lastp_;
			curLine_ = lastLine_;
			return 1;
		}
	}
	return 0;
}



void XLexer::Error(const char *str, ...)
{
	core::StackString<1024> temp;

	va_list args;
	va_start(args, str);

	temp.appendFmt(str, args);
	temp.appendFmt(" Line: %i", this->curLine_);

	va_end(args);
	X_ERROR("Lex", temp.c_str());
}

void XLexer::Warning(const char *str, ...)
{
	core::StackString<1024> temp;

	va_list args;
	va_start(args, str);
	vsprintf(&temp[0], str, args);
	va_end(args);

	X_WARNING("Lex", temp.c_str());
}



X_NAMESPACE_END