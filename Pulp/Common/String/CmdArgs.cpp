#include "EngineCommon.h"
#include "CmdArgs.h"

#include "Lexer.h"
#include <String\StackString.h>

X_NAMESPACE_BEGIN(core)


template<typename TChar>
CmdArgs<TChar>::CmdArgs(void)
{
	clear();
}

template<typename TChar>
CmdArgs<TChar>::CmdArgs(const TChar* pText)
{
	tokenize(pText);
}

template<typename TChar>
void CmdArgs<TChar>::clear(void)
{
	argc_ = 0;
	core::zero_object(argv_);
	core::zero_object(tokenized_);
}

template<typename TChar>
size_t CmdArgs<TChar>::getArgc(void) const
{
	return argc_;
}

template<typename TChar>
const TChar * CmdArgs<TChar>::getArgv(size_t idx) const
{
	return argv_[idx];
}

template<>
void CmdArgs<char>::tokenize(const char* pText)
{
	if (!pText) {
		return;
	}

	clear();

	size_t txtLen = strUtil::strlen(pText);
	size_t totalLen = 0;

	XLexer lex(pText, pText + txtLen);
	lex.setFlags(
		LexFlag::ALLOWPATHNAMES |
		LexFlag::NOERRORS |
		LexFlag::NOWARNINGS |
		LexFlag::NOSTRINGESCAPECHARS |
		LexFlag::NOSTRINGCONCAT |
		LexFlag::ONLYSTRINGS
	);

	XLexToken token;
	while (lex.ReadToken(token))
	{
		if (argc_ >= MAX_COMMAND_ARGS) {
			return;
		}

		size_t len = token.length();

		if ((totalLen + len + 1) > MAX_COMMAND_STRING) {
			return;
		}


		argv_[argc_] = tokenized_ + totalLen;
		argc_++;
		
		::memcpy_s(tokenized_ + totalLen, 
			sizeof(tokenized_) - totalLen,
			token.begin(), len);

		totalLen += (len + 1);
	}
}

template<>
void CmdArgs<wchar_t>::tokenize(const wchar_t* pText)
{
	if (!pText) {
		return;
	}
	// lex don't support wde, so this was born.
	char narrow[MAX_COMMAND_STRING] = { 0 };
	strUtil::Convert(pText, narrow);

	clear();

	size_t txtLen = strUtil::strlen(narrow);
	size_t totalLen = 0;

	XLexer lex(narrow, narrow + txtLen);
	lex.setFlags(
		LexFlag::ALLOWPATHNAMES |
		LexFlag::NOERRORS |
		LexFlag::NOWARNINGS |
		LexFlag::NOSTRINGESCAPECHARS |
		LexFlag::NOSTRINGCONCAT |
		LexFlag::ONLYSTRINGS
	);

	XLexToken token;
	while (lex.ReadToken(token))
	{
		if (argc_ >= MAX_COMMAND_ARGS) {
			return;
		}

		size_t len = token.length();

		if ((totalLen + len + 1) > MAX_COMMAND_STRING) {
			return;
		}


		argv_[argc_] = tokenized_ + totalLen;
		argc_++;

		core::StackString512 temp(token.begin(), token.begin() + len);

		// convert back to wide.
		strUtil::Convert(
			temp.begin(),
			tokenized_ + totalLen,
			sizeof(tokenized_) - (totalLen * 2) // bytes left in buffer.
		);

		totalLen += (len + 1);
	}
}

template class CmdArgs<char>;
template class CmdArgs<wchar_t>;


X_NAMESPACE_END
