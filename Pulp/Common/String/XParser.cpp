#include "EngineCommon.h"
#include "XParser.h"


X_NAMESPACE_BEGIN(core)




XParser::XParser(MemoryArenaBase* arena) :
arena_(arena),
pLexer_(nullptr),
pTokens_(nullptr),
macros_(arena,4)
{
	X_ASSERT_NOT_NULL(arena);
	core::zero_object(macroCharCache);
}

XParser::XParser(LexFlags flags, MemoryArenaBase* arena) :
arena_(arena),
pLexer_(nullptr),
pTokens_(nullptr),
macros_(arena, 4),
flags_(flags)
{
	X_ASSERT_NOT_NULL(arena);
	core::zero_object(macroCharCache);
}

XParser::XParser(const char* startInclusive, const char* endExclusive,
	const char* name, LexFlags flags, MemoryArenaBase* arena) :
arena_(arena),
pLexer_(nullptr),
pTokens_(nullptr),
macros_(arena, 4),
flags_(flags)
{
	X_ASSERT_NOT_NULL(arena);
	core::zero_object(macroCharCache);

	LoadMemory(startInclusive, endExclusive, name);
}


XParser::~XParser()
{
	freeSource();
}
	
void XParser::freeSource(void)
{
	if (pLexer_) {
		X_DELETE(pLexer_, arena_);
		pLexer_ = nullptr;
	}
}

bool XParser::LoadMemory(const char* startInclusive, const char* endExclusive, 
	const char* name)
{
	X_ASSERT_NOT_NULL(startInclusive);
	X_ASSERT_NOT_NULL(endExclusive);
	X_ASSERT_NOT_NULL(name);

	filename_ = name;

	pLexer_ = X_NEW(XLexer, arena_, "ParserLex")(startInclusive, endExclusive);
	pLexer_->setFlags(flags_);

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

		if (token.GetType() == TokenType::PUNCTUATION && token.length() > 0 && token.begin()[0] == '#')
		{
			if (!ReadDirective()) {
				return false;
			}
			continue;
		}

		if (token.GetType() == TokenType::NAME)
		{
			MacroDefine* define = FindDefine(token);
			if (define)
			{
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

	if (!ReadToken(token)) {
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
	core::StackString<128> str;

	if (!ReadToken(token)) {
		Error("couldn't read expected token");
		return false;
	}

	if (token.GetType() != type) 
	{
		switch (type) {
			case TokenType::STRING: str.append("string"); break;
			case TokenType::LITERAL: str.append("literal"); break;
			case TokenType::NUMBER: str.append("number"); break;
			case TokenType::NAME: str.append("name"); break;
			case TokenType::PUNCTUATION: str.append("punctuation"); break;
			default: str.append("unknown type"); break;
		}
		Error("expected a %s but found '%.*s'", str.c_str(), token.length(), token.begin());
		return false;
	}

	if (token.GetType() == TokenType::NUMBER)
	{
		if ((token.GetSubType() & subtype) != subtype) 
		{
			str.clear();
			if (subtype.IsSet(TokenSubType::DECIMAL)) str.append("decimal ");
			if (subtype.IsSet(TokenSubType::HEX)) str.append("hex ");
			if (subtype.IsSet(TokenSubType::OCTAL)) str.append("octal ");
			if (subtype.IsSet(TokenSubType::BINARY)) str.append("binary ");
			if (subtype.IsSet(TokenSubType::UNSIGNED)) str.append("unsigned ");
			if (subtype.IsSet(TokenSubType::LONG)) str.append("long ");
			if (subtype.IsSet(TokenSubType::FLOAT)) str.append("float ");
			if (subtype.IsSet(TokenSubType::INTEGER)) str.append("integer ");
			str.stripTrailing(' ');
			Error("expected %s but found '%.*s'", str.c_str(), token.length(), token.begin());
			return false;
		}
	}
	else if (token.GetType() == TokenType::PUNCTUATION) 
	{
		if (token.GetPuncId() != puncId)
		{
			Error("expected '%s' but found '%.*s'", 
				pLexer_->GetPunctuationFromId(puncId), token.length(), token.begin());
			return false;
		}
	}
	return true;
}

int XParser::ParseInt()
{
	XLexToken token;

	if (!ReadToken(token)) {
		Error("couldn't read expected integer");
		return 0;
	}
	if (token.GetType() == TokenType::PUNCTUATION && token.isEqual("-")) 
	{
		ExpectTokenType(TokenType::NUMBER, TokenSubType::INTEGER,
			PunctuationId::UNUSET, token);

		return -(safe_static_cast<signed int, int32_t>(token.GetIntValue()));
	}
	else if (token.GetType() != TokenType::NUMBER 
		|| token.GetSubType() == TokenSubType::FLOAT) {
		Error("expected integer value, found '%.*s'", token.length(), token.begin());
	}
	return token.GetIntValue();
}


bool XParser::ParseBool()
{
	XLexToken token;

	if (!ReadToken(token)) {
		Error("couldn't read expected bool");
		return false;
	}

	if (token.GetType() == TokenType::NUMBER)
	{
		return (token.GetIntValue() != 0);
	}
	else if (token.GetType() == TokenType::NAME)
	{
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


float XParser::ParseFloat()
{
	XLexToken token;

	if (!ReadToken(token)) {
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

const int XParser::GetLineNumber(void)
{
	if (pLexer_) {
		return pLexer_->GetLineNumber();
	}
	X_WARNING("Parser", "called 'GetLineNumber' on a parser without a valid file loaded");
	return true;
}

bool XParser::isEOF(void) const
{
	if (pLexer_) {
		return pLexer_->isEOF();
	}
	X_WARNING("Parser", "called 'EOF' on a parser without a valid file loaded");
	return true;
}

bool XParser::ReadSourceToken(XLexToken& token)
{
	if (!pLexer_) {
		X_ERROR("Parser", "can't read token, no source loaded");
		return false;
	}

	if (pTokens_)
	{
		// copy the already available token
		token = *pTokens_;

		// remove the token from the source
		XLexToken* t = pTokens_;
		pTokens_ = t->pNext_;
		
		X_DELETE(t,arena_);
		return true;
	}

	return pLexer_->ReadToken(token) == 1;
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

	if (!ReadSourceToken(token))
	{
		X_ERROR("Parser", "'#' without a name");
		return false;
	}

	if (token.GetType() == TokenType::NAME)
	{
		if (token.isEqual("define"))
		{
			return Directive_define();
		}
		if (token.isEqual("include"))
		{
			return Directive_include();
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

	if (!ReadSourceToken(token))
	{
		X_ERROR("Parser", "#define without a name");
		return false;
	}

	if (token.GetType() != TokenType::NAME)
	{
		core::StackString512 temp(token.begin(), token.end());
		X_ERROR("Parser", "expected a name after #define got: %s", temp.c_str());
		return false;
	}

	if (token.length() < 1)
	{
		X_ERROR("Parser", "invalid token line %i", token.GetLine());
		return false;
	}

	define = FindDefine(token);
	if (define)
	{
		// alreayd defined.
		return true;
	}

	define = X_NEW(MacroDefine,arena_,"Macro")();
	define->name.assign(token.begin(), token.end());
	addDefinetoHash(define);

	// check above makes sure atleast one char
	addToCache(define->name[0]);

	if (!ReadLine(token)) {
		return true;
	}

	if (token.isEqual("("))
	{
		// read the define parameters
		last = nullptr;

		if (!CheckTokenString(")")) 
		{
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
				t = X_NEW(XLexToken,arena_,"MarcoParam")(token);
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

	do
	{
		XLexToken* pT = X_NEW(XLexToken, arena_, "Macrotoken")(token);

		if (token.GetType() == TokenType::NAME && token.isEqual(define->name)) {
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
	X_ASSERT_NOT_IMPLEMENTED();
	return false;
}

bool XParser::CheckTokenString(const char* string)
{
	XLexToken tok;

	if (!ReadToken(tok)) {
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
	core::StackString512 name(token.begin(), token.end());
	XLexToken* p;
	int i;

	i = 0;
	for (p = define->pParms; p; p = p->pNext_) {
		if (p->isEqual(name.c_str())) {
			return i;
		}
		i++;
	}
	return -1;
}


int	XParser::ReadLine(XLexToken& token)
{
	int crossline;

	crossline = 0;
	do 
	{
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


static const int MAX_DEFINEPARMS = 32;

int XParser::ReadDefineParms(MacroDefine* pDefine, XLexToken** parms, int maxparms)
{
	MacroDefine* newdefine;
	XLexToken token, *t, *last;
	int i, done, lastcomma, numparms, indent;

	if (!ReadSourceToken(token)) {
		Error("define '%s' missing parameters", pDefine->name);
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
		Error("define '%s' missing parameters", pDefine->name);
		return false;
	}
	// read the define parameters
	for (done = 0, numparms = 0, indent = 1; !done;) {
		if (numparms >= maxparms) {
			Error("define '%s' with too many parameters", pDefine->name);
			return false;
		}
		parms[numparms] = nullptr;
		lastcomma = 1;
		last = nullptr;
		while (!done) {

			if (!ReadSourceToken(token)) {
				Error("define '%s' incomplete", pDefine->name);
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
				if (last) 
					last->pNext_ = t;
				else 
					parms[numparms] = t;
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
	for (dt = pDefine->pTokens; dt; dt = dt->pNext_)
	{
		parmnum = -1;
		// if the token is a name, it could be a define parameter
		if (dt->GetType() == TokenType::NAME) {
			parmnum = FindDefineParm(pDefine, *dt);
		}
		// if it is a define parameter
		if (parmnum >= 0) 
		{
			for (pt = parms[parmnum]; pt; pt = pt->pNext_) 
			{
				t = X_NEW(XLexToken,arena_,"TokenParam")(*pt);
				//add the token to the list
				t->pNext_ = nullptr;
				if (last) 
					last->pNext_ = t;
				else 
					first = t;
				last = t;
			}
		}
		else 
		{
			// if stringizing operator
			if (dt->isEqual("#"))
			{
				t = nullptr; // warn 4701 fix.
				X_ASSERT_NOT_IMPLEMENTED();
			}
			else
			{
				t = X_NEW(XLexToken,arena_,"DefineToken")(*dt);
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
	X_ASSERT(token.length() > 0, "invalid token passed, must have a length")(token.length());
	// little optermisation, which will work very well if all macro's
	// are upper case, since anything else in the file not starting with uppercase.
	// will fail this test.
	if (!isInCache(token.begin()[0]))
		return nullptr;

	// create a null term string.
	core::StackString512 temp(token.begin(), token.end());

	MacroMap::const_iterator it = macros_.find(X_CONST_STRING(temp.c_str()));

	if (it != macros_.end())
		return it->second;

	return nullptr;
}

void XParser::addDefinetoHash(MacroDefine* define)
{
	X_ASSERT_NOT_NULL(define);

	macros_.insert(MacroMap::value_type(core::string(define->name.c_str()), define));
}


void XParser::Error(const char *str, ...)
{
	core::StackString<1024> temp;

	va_list args;
	va_start(args, str);

	temp.appendFmt(str, args);
//	temp.appendFmt(" Line: %i", this->curLine_);

	va_end(args);
	X_ERROR("Parse", temp.c_str());
}

void XParser::Warning(const char *str, ...)
{
	core::StackString<1024> temp;

	va_list args;
	va_start(args, str);
	vsprintf(&temp[0], str, args);
	va_end(args);

	X_WARNING("Parse", temp.c_str());
}



X_NAMESPACE_END