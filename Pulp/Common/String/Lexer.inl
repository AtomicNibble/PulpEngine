

X_NAMESPACE_BEGIN(core)

XLexToken::XLexToken() :
	XLexToken(nullptr, nullptr)
{ 
}

XLexToken::XLexToken(const char* start, const char* end) :
start_(start), 
end_(end), 
pNext_(nullptr) 
{ 
	Init(); 
}

size_t XLexToken::length(void) const
{ 
	return end_ - start_; 
}

TokenType::Enum XLexToken::GetType(void) const
{
	return type_;
}

XLexToken::TokenSubTypeFlags XLexToken::GetSubType(void) const
{
	return subtype_;
}

PunctuationId::Enum XLexToken::GetPuncId(void) const
{
	return puncId_;
}

int32_t XLexToken::GetLine(void) const
{
	return line_;
}

void XLexToken::SetType(TokenType::Enum type)
{
	type_ = type;
}

void XLexToken::SetSubType(TokenSubTypeFlags subType)
{
	subtype_ = subType;
}

const char* XLexToken::begin(void) const
{ 
	return start_; 
}

const char* XLexToken::end(void) const
{ 
	return end_;
}

bool XLexToken::isEqual(const char* str) const
{
	return strUtil::IsEqual(start_, end_, str);
}

bool XLexToken::isEqual(const char* pBegin, const char* pEnd) const
{
	return strUtil::IsEqual(start_, end_, pBegin, pEnd);
}


double XLexToken::GetDoubleValue(void)
{
	if (type_ != TokenType::NUMBER) {
		return 0.0;
	}
	if (!subtype_.IsSet(TokenSubTypeFlags::VALUESVALID)) {
		NumberValue();
	}
	return floatvalue_;
}

float XLexToken::GetFloatValue(void)
{
	return static_cast<float>(GetDoubleValue());
}

uint32_t XLexToken::GetUIntValue(void)
{
	if (type_ != TokenType::NUMBER) {
		return 0u;
	}
	if (!subtype_.IsSet(TokenSubTypeFlags::VALUESVALID)) {
		NumberValue();
	}

	return static_cast<uint32_t>(floatvalue_);
}

int32_t	XLexToken::GetIntValue(void)
{
	if (type_ != TokenType::NUMBER) {
		return 0;
	}
	if (!subtype_.IsSet(TokenSubTypeFlags::VALUESVALID)) {
		NumberValue();
	}

	return intvalue_;
}

void XLexToken::Reset(void)
{
	start_ = nullptr;
	end_ = nullptr;
	Init();
}
 

void XLexToken::Init(void)
{
	line_ = -1;
	linesCrossed_ = -1;
//	flags_ = 0;

	intvalue_ = 0;
	floatvalue_ = 0.f;

	type_ = TokenType::INVALID;
	subtype_.Clear();
}


void XLexToken::SetStart(const char* start)
{
	start_ = start;
}

void XLexToken::SetEnd(const char* end)
{
	end_ = end;
}

XLexToken* XLexToken::GetNext(void)
{
	return pNext_;
}

const XLexToken* XLexToken::GetNext(void) const
{
	return pNext_;
}

// ------------------------------------------

bool XLexer::isEOF(void) const
{
	// check if we have gone past the end for some strange reason.
	// still returns true for EOF if past tho.
	X_ASSERT(current_ <= end_, "current is past the end of the file")(current_, end_);
	return current_ >= end_;
}

bool XLexer::isEOF(bool skipWhiteSpace)
{
	// check if we have gone past the end for some strange reason.
	// still returns true for EOF if past tho.
	X_ASSERT(current_ <= end_, "current is past the end of the file")(current_, end_);

	if (skipWhiteSpace) {
		ReadWhiteSpace();
	}

	return current_ >= end_;
}

size_t XLexer::BytesLeft(void) const
{
	return static_cast<size_t>(end_ - current_);
}

void XLexer::setFlags(LexFlags flags)
{
	flags_ = flags;
}

XLexer::ErrorState::Enum XLexer::GetErrorState(void) const
{
	return errState_;
}


int32_t XLexer::CheckString(const char *str) const 
{
	int i;

	for (i = 0; str[i]; i++) {
		if (current_[i] != str[i]) {
			return false;
		}
	}
	return true;
}

const char* XLexer::GetFileName(void) const
{
	return filename_.c_str();
}


int32_t XLexer::GetLineNumber(void) const
{
	return curLine_;
}


X_NAMESPACE_END
