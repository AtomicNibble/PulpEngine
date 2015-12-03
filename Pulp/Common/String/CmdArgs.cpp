#include "EngineCommon.h"
#include "CmdArgs.h"

#include "Lexer.h"

X_NAMESPACE_BEGIN(core)


template<typename TChar>
CmdArgs<TChar>::CmdArgs(void)
{
	clear();
}

template<typename TChar>
CmdArgs<TChar>::CmdArgs(const TChar* text)
{

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

template<typename TChar>
void CmdArgs<TChar>::tokenize(const TChar* pText)
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



X_NAMESPACE_END
