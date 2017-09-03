#pragma once

#include "TimeVal.h"



inline X_NAMESPACE(core)::TimeVal operator "" _tv(unsigned long long int x)
{
	return X_NAMESPACE(core)::TimeVal(static_cast<int64_t>(x));
}


