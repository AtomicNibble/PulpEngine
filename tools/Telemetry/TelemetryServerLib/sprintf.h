#pragma once

X_NAMESPACE_BEGIN(telemetry)


struct ArgData
{
    constexpr static tt_int32 BUF_SIZE = 255;

    tt_uint8 numArgs;
    tt_uint8 data[BUF_SIZE];
};

using SprintfStrBuf = core::StackString<MAX_STRING_LEN, char>;

void sprintf_ArgData(SprintfStrBuf& buf, const char* pFormat, const ArgData& data);

X_NAMESPACE_END
