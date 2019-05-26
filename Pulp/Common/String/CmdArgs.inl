
X_NAMESPACE_BEGIN(core)

template<size_t BUF_SIZE>
CmdArgs<BUF_SIZE>::CmdArgs(void) :
    CmdArgs<BUF_SIZE>(nullptr, 0)
{
}

template<size_t BUF_SIZE>
CmdArgs<BUF_SIZE>::CmdArgs(const TChar* pText) :
    CmdArgs<BUF_SIZE>(pText, strUtil::strlen(pText))
{
}

template<size_t BUF_SIZE>
CmdArgs<BUF_SIZE>::CmdArgs(const TChar* pText, size_t length)
{
    argc_ = 0;
    core::zero_object(argv_);
    core::zero_object(tokenized_);

    tokenize(pText, length);
}

template<size_t BUF_SIZE>
void CmdArgs<BUF_SIZE>::clear(void)
{
    argc_ = 0;
    core::zero_object(argv_);
    core::zero_object(tokenized_);
}

template<size_t BUF_SIZE>
size_t CmdArgs<BUF_SIZE>::getArgc(void) const
{
    return argc_;
}

template<size_t BUF_SIZE>
const typename CmdArgs<BUF_SIZE>::TChar* CmdArgs<BUF_SIZE>::getArgv(size_t idx) const
{
    return argv_[idx];
}

template<size_t BUF_SIZE>
void CmdArgs<BUF_SIZE>::tokenize(const TChar* pText, size_t length)
{
    if (!length) {
        return;
    }

    clear();

    size_t txtLen = length;
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
    while (lex.ReadToken(token)) {
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
            token.begin(), len
        );

        totalLen += (len + 1);
    }
}

template<size_t BUF_SIZE>
const typename CmdArgs<BUF_SIZE>::TChar* CmdArgs<BUF_SIZE>::getOption(const TChar* pOptionName) const
{
    if (argc_ < 2) {
        return nullptr;
    }

    for (size_t i = 0; i < argc_ - 1; i++) {
        if (core::strUtil::IsEqualCaseInsen(pOptionName, getArgv(i))) {
            return getArgv(i + 1);
        }
    }

    return nullptr;
}

template<size_t BUF_SIZE>
bool CmdArgs<BUF_SIZE>::hasFlag(const TChar* pFlag) const
{
    for (size_t i = 0; i < argc_; i++) {
        if (core::strUtil::IsEqualCaseInsen(pFlag, getArgv(i))) {
            return true;
        }
    }

    return false;
}

template<size_t BUF_SIZE>
void CmdArgs<BUF_SIZE>::appendArg(const TChar* pArg)
{
    const size_t argLen = strlen(pArg);

    if (argc_ < 1) {
        const size_t copySize = core::Min(argLen + 1, BUF_SIZE - 1);

        argc_ = 1;
        argv_[0] = tokenized_;

        ::memcpy(
            tokenized_,
            pArg,
            copySize * sizeof(TChar));
    }
    else {
        argv_[argc_] = argv_[argc_ - 1] + (strUtil::strlen(argv_[argc_ - 1]) + 1) * sizeof(TChar);

        const size_t bytesLeft = (sizeof(tokenized_) - (argv_[argc_] - tokenized_));
        const size_t copySize = core::Min(argLen + 1, bytesLeft - 1);

        ::memcpy(
            argv_[argc_],
            pArg,
            copySize * sizeof(TChar) // copy nt.
        );

        argc_++;
    }
}


X_NAMESPACE_END
