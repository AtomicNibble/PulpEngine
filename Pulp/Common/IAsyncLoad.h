#pragma once

#include <Util\EnumMacros.h>
#include <Util\NamespaceMacros.h>

X_NAMESPACE_BEGIN(core)


X_DECLARE_ENUM8(LoadStatus) (
	NotLoaded,
	Loading,
	Complete,
	Error
);



X_NAMESPACE_END