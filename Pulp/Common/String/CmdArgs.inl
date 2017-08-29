
X_NAMESPACE_BEGIN(core)


template<size_t BUF_SIZE, typename TChar>
CmdArgs<BUF_SIZE, TChar>::CmdArgs(void)
{
	clear();
}


template<size_t BUF_SIZE>
CmdArgs<BUF_SIZE, char>::CmdArgs(const char* pText)
{
	tokenize(pText);
}


template<size_t BUF_SIZE, typename TChar>
void CmdArgs<BUF_SIZE, TChar>::clear(void)
{
	argc_ = 0;
	core::zero_object(argv_);
	core::zero_object(tokenized_);
}

template<size_t BUF_SIZE, typename TChar>
size_t CmdArgs<BUF_SIZE, TChar>::getArgc(void) const
{
	return argc_;
}

template<size_t BUF_SIZE, typename TChar>
const TChar * CmdArgs<BUF_SIZE, TChar>::getArgv(size_t idx) const
{
	return argv_[idx];
}


template<size_t BUF_SIZE>
void CmdArgs<BUF_SIZE,char>::tokenize(const char* pText)
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

		if ((totalLen + len + 1) > BUF_SIZE) {
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

template<size_t BUF_SIZE>
const char* CmdArgs<BUF_SIZE, char>::getOption(const char* pOptionName) const
{
	if (argc_ < 2) {
		return nullptr;
	}

	for (size_t i = 0; i < argc_ - 1; i++)
	{
		if (core::strUtil::IsEqualCaseInsen(pOptionName, getArgv(i)))
		{
			return getArgv(i + 1);
		}
	}

	return nullptr;
}

template<size_t BUF_SIZE>
bool CmdArgs<BUF_SIZE, char>::hasFlag(const char* pFlag) const
{
	for (size_t i = 0; i < argc_; i++)
	{
		if (core::strUtil::IsEqualCaseInsen(pFlag, getArgv(i)))
		{
			return true;
		}
	}

	return false;
}


template<size_t BUF_SIZE>
void CmdArgs<BUF_SIZE, char>::AppendArg(const char* pArg)
{
	const size_t argLen = strlen(pArg);

	if (argc_ < 1)
	{
		const size_t copySize = core::Min(argLen + 1, BUF_SIZE - 1);

		argc_ = 1;
		argv_[0] = tokenized_;

		::memcpy(
			tokenized_,
			pArg,
			copySize * sizeof(char)
		);
	}
	else
	{
		argv_[argc_] = argv_[argc_ - 1] + (wcslen(argv_[argc_ - 1]) + 1) * sizeof(char);

		const size_t bytesLeft = (sizeof(tokenized_) - (argv_[argc_] - tokenized_));
		const size_t copySize = core::Min(argLen + 1, bytesLeft - 1);

		::memcpy(
			argv_[argc_],
			pArg,
			copySize * sizeof(char) // copy nt.
		);

		argc_++;
	}
}

// ==================================


template<size_t BUF_SIZE>
CmdArgs<BUF_SIZE, wchar_t>::CmdArgs(void)
{
	clear();
}


template<size_t BUF_SIZE>
CmdArgs<BUF_SIZE, wchar_t>::CmdArgs(const wchar_t* pText)
{
	tokenize(pText);
}


template<size_t BUF_SIZE>
void CmdArgs<BUF_SIZE, wchar_t>::clear(void)
{
	argc_ = 0;
	core::zero_object(argv_);
	core::zero_object(tokenized_);
}

template<size_t BUF_SIZE>
size_t CmdArgs<BUF_SIZE, wchar_t>::getArgc(void) const
{
	return argc_;
}

template<size_t BUF_SIZE>
const wchar_t* CmdArgs<BUF_SIZE, wchar_t>::getArgv(size_t idx) const
{
	return argv_[idx];
}

template<size_t BUF_SIZE>
void CmdArgs<BUF_SIZE, wchar_t>::tokenize(const wchar_t* pText)
{
	if (!pText) {
		return;
	}
	// lex don't support wde, so this was born.
	char narrow[BUF_SIZE] = { 0 };
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

		if ((totalLen + len + 1) > BUF_SIZE) {
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


template<size_t BUF_SIZE>
const wchar_t* CmdArgs<BUF_SIZE, wchar_t>::getOption(const wchar_t* pOptionName) const
{
	if (argc_ < 2) {
		return nullptr;
	}

	for (size_t i = 0; i < argc_ - 1; i++)
	{
		if (core::strUtil::IsEqualCaseInsen(pOptionName, getArgv(i)))
		{
			return getArgv(i + 1);
		}
	}

	return nullptr;
}

template<size_t BUF_SIZE>
bool CmdArgs<BUF_SIZE, wchar_t>::hasFlag(const wchar_t* pFlag) const
{
	for (size_t i = 0; i < argc_; i++)
	{
		if (core::strUtil::IsEqualCaseInsen(pFlag, getArgv(i)))
		{
			return true;
		}
	}

	return false;
}

template<size_t BUF_SIZE>
void CmdArgs<BUF_SIZE, wchar_t>::AppendArg(const wchar_t* pArg)
{
	const size_t argLen = wcslen(pArg);

	if (argc_ < 1)
	{
		const size_t copySize = core::Min(argLen + 1, BUF_SIZE - 1);

		argc_ = 1;
		argv_[0] = tokenized_;

		::memcpy(
			tokenized_,
			pArg, 
			copySize * sizeof(wchar_t)
		);
	}
	else
	{
		argv_[argc_] = argv_[argc_ - 1] + (wcslen(argv_[argc_ - 1]) + 1) * sizeof(wchar_t);

		const size_t bytesLeft = (sizeof(tokenized_) - (argv_[argc_] - tokenized_));
		const size_t copySize = core::Min(argLen + 1, bytesLeft - 1);

		::memcpy(
			argv_[argc_],
			pArg,
			copySize * sizeof(wchar_t) // copy nt.
		);

		argc_++;
	}
}


X_NAMESPACE_END