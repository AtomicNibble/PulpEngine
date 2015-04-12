#pragma once

#ifndef X_I_SERIALIZE_H_
#define X_I_SERIALIZE_H_

#include <IFileSys.h>

X_NAMESPACE_BEGIN(core)

struct ISerialize
{
	virtual bool SSave(XFile* pFile) X_ABSTRACT;
	virtual bool SLoad(XFile* pFile) X_ABSTRACT;

protected:
	virtual ~ISerialize() {}
};

X_NAMESPACE_END

#endif // !X_I_SERIALIZE_H_