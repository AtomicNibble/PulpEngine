#pragma once

#include <Util\Span.h>


X_NAMESPACE_BEGIN(net)

static int32_t MAX_ARRAY_ELEMENTS = 128;

X_DECLARE_ENUM(CompPropType)
(
    Int,
    Int64,
    Float,
    Vector,
    VectorXY,
    String,
    Array,
    DataTable,
    Quaternion);

X_DECLARE_FLAGS(CompPropFlag)(
    NormalVec,
    Unsigned);

typedef Flags<CompPropFlag> CompPropFlags;

class CompProp
{
public:
    CompProp(const char* pName, CompPropType::Enum type, int32_t fieldOffset, int32_t sizeOfVar, int32_t numBits,
        CompPropFlags flags);

    CompProp(const char* pName, CompPropType::Enum type, int32_t fieldOffset, int32_t sizeOfVar, int32_t numBits, int32_t numElemets);

    X_INLINE CompPropType::Enum getType(void) const;
    X_INLINE CompPropFlags getFlags(void) const;
    X_INLINE int32_t getFieldOffset(void) const;
    X_INLINE int32_t getNumBits(void) const;
    X_INLINE int32_t getSizeOfVar(void) const;
    X_INLINE int32_t getNumElements(void) const;

private:
    const char* pName_;
    CompPropType::Enum type_;
    CompPropFlags flags_;
    int32_t fieldOffset_;
    int32_t numBits_;
    int32_t sizeOfVar_;
    int32_t numElements_; // Array
    float fltLowValue_;
    float fltHighValue_;
};

class CompTable
{
public:
    CompTable();
    CompTable(core::span<CompProp> props, const char* pTableName);

    void set(core::span<CompProp> props, const char* pTableName);

    X_INLINE size_t numProps(void) const;
    X_INLINE const CompProp& getProp(size_t idx) const;

    X_INLINE const char* getTableName(void) const;

private:
    core::span<CompProp> props_;
    const char* pTableName_;
};


CompProp CompPropInt(const char* pName, int32_t offset, int32_t sizeOfVar, int32_t numBits = -1, CompPropFlags flags = CompPropFlags());
CompProp CompPropQuat(const char* pName, int32_t offset, int32_t sizeOfVar, int32_t numBits = 32, CompPropFlags flag = CompPropFlags());
CompProp CompPropVec(const char* pName, int32_t offset, int32_t sizeOfVar, int32_t numBits = 32, CompPropFlags flags = CompPropFlags());

CompProp CompPropArray(const char* pName, int32_t offset, int32_t sizeOfVar, int32_t numElemets, int32_t elementSize);


X_NAMESPACE_END

#include "MetaTable.inl"