#pragma once

#ifndef X_I_SERIALIZE_H_
#define X_I_SERIALIZE_H_

X_NAMESPACE_BEGIN(core)

struct XFile;

struct ISerialize
{
    virtual bool SSave(XFile* pFile) const X_ABSTRACT;
    virtual bool SLoad(XFile* pFile) X_ABSTRACT;

protected:
    virtual ~ISerialize() = default;
};

X_NAMESPACE_END

#endif // !X_I_SERIALIZE_H_
