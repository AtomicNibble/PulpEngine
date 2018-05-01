#include "EngineCommon.h"
#include "MetaTable.h"

X_NAMESPACE_BEGIN(net)

CompProp::CompProp(const char* pName, CompPropType::Enum type, int32_t fieldOffset, int32_t numBits, CompPropFlags flags) :
    pName_(pName),
    type_(type),
    fieldOffset_(fieldOffset),
    numBits_(numBits),
    flags_(flags),
    fltLowValue_(0.f),
    fltHighValue_(0.f)
{
}

// ---------------------------------------------------


CompTable::CompTable() :
    pTableName_(nullptr)
{
}

CompTable::CompTable(core::span<CompProp> props, const char* pTableName) :
    props_(props),
    pTableName_(pTableName)
{
}

void CompTable::set(core::span<CompProp> props, const char* pTableName)
{
    props_ = props;
    pTableName_ = pTableName;
}

// ---------------------------------------------------

CompProp CompPropInt(const char* pName, int32_t offset, int32_t sizeOfVar, int32_t numBits, CompPropFlags flags)
{
    CompPropType::Enum type = CompPropType::Int;

    switch (sizeOfVar) {
        case 1:
        case 2:
        case 4:
            type = CompPropType::Int;
            break;
        case 8:
            type = CompPropType::Int64;
            break;
        default:
            X_ASSERT_NOT_IMPLEMENTED();
            break;
    }

    if (numBits <= 0) {
        numBits = sizeOfVar * 8;
    }

    return{ pName, type, offset, numBits, flags };
}

CompProp CompPropQuat(const char* pName, int32_t offset, int32_t sizeOfVar, int32_t numBits, CompPropFlags flags)
{
    X_ASSERT(sizeOfVar == sizeof(Quatf), "Size not match")
        (sizeOfVar, sizeof(Quatf));

    return{ pName, CompPropType::Quaternion, offset, numBits, flags };
}

CompProp CompPropVec(const char* pName, int32_t offset, int32_t sizeOfVar, int32_t numBits, CompPropFlags flags)
{
    X_ASSERT(sizeOfVar == sizeof(Vec3f), "Size not match")
        (sizeOfVar, sizeof(Vec3f));

    return{ pName, CompPropType::Vector, offset, numBits, flags };
}


X_NAMESPACE_END