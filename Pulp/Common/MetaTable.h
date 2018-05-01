#pragma once

#include <Util\Span.h>


X_NAMESPACE_BEGIN(net)


X_DECLARE_ENUM(CompPropType)
(
    Int,
    Int64,
    Float,
    Vector,
    VectorXY,
    String,
    DataTable,
    Quaternion);

X_DECLARE_FLAGS(CompPropFlag)(
    NormalVec,
    Unsigned);

typedef Flags<CompPropFlag> CompPropFlags;

class CompProp
{
public:
    CompProp(const char* pName, CompPropType::Enum type, int32_t fieldOffset, int32_t numBits,
        CompPropFlags flags);

private:
    const char* pName_;
    CompPropType::Enum type_;
    CompPropFlags flags_;
    int32_t fieldOffset_;
    int32_t numBits_;
    float fltLowValue_;
    float fltHighValue_;
};

class CompTable
{
public:
    CompTable();
    CompTable(core::span<CompProp> props, const char* pTableName);

    void set(core::span<CompProp> props, const char* pTableName);

private:
    core::span<CompProp> props_;
    const char* pTableName_;
};


CompProp CompPropInt(const char* pName, int32_t offset, int32_t sizeOfVar, int32_t numBits = -1, CompPropFlags flags = CompPropFlags());
CompProp CompPropQuat(const char* pName, int32_t offset, int32_t sizeOfVar, int32_t numBits = 32, CompPropFlags flag = CompPropFlags());
CompProp CompPropVec(const char* pName, int32_t offset, int32_t sizeOfVar, int32_t numBits = 32, CompPropFlags flags = CompPropFlags());



X_NAMESPACE_END