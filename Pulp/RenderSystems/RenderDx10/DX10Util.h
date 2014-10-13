#pragma once


#ifndef X_DX10_UTIL_H_
#define X_DX10_UTIL_H_

#include "String\StrRef.h"

X_NAMESPACE_BEGIN(render)

namespace D3DDebug
{

	X_INLINE void SetDebugObjectName(ID3D11DeviceChild* resource, const core::string& name)
	{
#if X_DEBUG
		resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<uint32>(name.length()), name);
#else
		UNREFERENCED_PARAMETER(resource);
		UNREFERENCED_PARAMETER(name);
#endif
	}

	X_INLINE void SetDebugObjectName(ID3D11DeviceChild* resource, const char *name)
	{
#if X_DEBUG
		resource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<uint32>(strlen(name)), name);
#else
		UNREFERENCED_PARAMETER(resource);
		UNREFERENCED_PARAMETER(name);
#endif
	}

	template<size_t TNameLength>
	X_INLINE void SetDebugObjectName(ID3D11DeviceChild* resource, const char(&name)[TNameLength])
	{
#if X_DEBUG
		resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
#else
		UNREFERENCED_PARAMETER(resource);
		UNREFERENCED_PARAMETER(name);
#endif
	}

}

X_NAMESPACE_END

#endif // !X_DX10_UTIL_H_