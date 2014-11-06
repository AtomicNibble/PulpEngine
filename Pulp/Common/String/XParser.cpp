#include "EngineCommon.h"
#include "XParser.h"

#include "String\StackString.h"

X_NAMESPACE_BEGIN(core)




XParser::XParser() :
pLexer_(nullptr)
{
	core::zero_object(macroCharCache);
}

XParser::~XParser()
{

}

int	XParser::ReadToken(XLexToken& token)
{
	while (1)
	{
		if (!ReadSourceToken(token))
			return false;

		if (token.type == TT_PUNCTUATION && token.length() > 0 && token.begin()[0] == '#')
		{
			if (!ReadDirective()) {
				return false;
			}

			continue;
		}

		return true;
	}
}

void XParser::UnreadToken(const XLexToken& token)
{

}

const int XParser::GetLineNumber(void)
{
	if (pLexer_)
		return pLexer_->GetLineNumber();
	X_WARNING("Parser", "called 'GetLineNumber' on a parser without a valid file loaded");
	return true;
}

bool XParser::isEOF(void) const
{
	if (pLexer_)
		return pLexer_->isEOF();
	X_WARNING("Parser", "called 'EOF' on a parser without a valid file loaded");
	return true;
}

bool XParser::ReadSourceToken(XLexToken& token)
{
	if (!pLexer_) {
		X_ERROR("Parser", "can't read token, no source loaded");
		return false;
	}

	return pLexer_->ReadToken(token);
}

bool XParser::ReadDirective(void)
{
	XLexToken token;

	if (!ReadSourceToken(token))
	{
		X_ERROR("Parser", "'#' without a name");
		return false;
	}

	if (token.type == TT_NAME)
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

	X_ERROR("Parser", "unknown precompiler directive. line(%i)", token.line);
	return false;
}

bool XParser::isInCache(const char ch) const
{
	int idx = ch - 65;
	if (idx < 0 || idx > numLetters)
		return false;

	return macroCharCache[ch - 65];
}

void XParser::addToCache(const char ch)
{
	int idx = ch - 65;
	if (idx < 0 || idx > numLetters)
		return;

	macroCharCache[ch - 65] = 1;
}

bool XParser::Directive_define(void)
{
	XLexToken token;
	MacroDefine* define;

	if (!ReadSourceToken(token))
	{
		X_ERROR("Parser", "#define without a name");
		return false;
	}

	if (token.type != TT_NAME)
	{
		core::StackString512 temp(token.begin(), token.end());
		X_ERROR("Parser", "expected a name after #define got: %s", temp.c_str());
		return false;
	}

	if (token.length() < 1)
	{
		X_ERROR("Parser", "invalid token line %i", token.line);
		return false;
	}

	define = FindDefine(token);


}

bool XParser::Directive_include(void)
{
	X_ASSERT_NOT_IMPLEMENTED();
	return false;
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



X_NAMESPACE_END