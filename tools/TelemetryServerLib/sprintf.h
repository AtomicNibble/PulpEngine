#pragma once

X_NAMESPACE_BEGIN(telemetry)


struct ArgData
{
    constexpr static tt_int32 BUF_SIZE = 255;

    tt_uint8 numArgs;
    tt_uint8 data[BUF_SIZE];
};


int sprintf_ArgData(char* buffer, int32_t bufLength, const char* format, const ArgData& data);

X_NAMESPACE_END