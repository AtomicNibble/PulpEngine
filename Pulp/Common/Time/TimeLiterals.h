#pragma once

#include "TimeVal.h"



inline X_NAMESPACE(core)::TimeVal operator "" _tv(unsigned long long int x)
{
	return X_NAMESPACE(core)::TimeVal(static_cast<X_NAMESPACE(core)::TimeVal::TimeType>(x));
}


inline X_NAMESPACE(core)::TimeVal operator "" _s(unsigned long long int x)
{
	X_NAMESPACE(core)::TimeVal time;
	time.SetSeconds(static_cast<X_NAMESPACE(core)::TimeVal::TimeType>(x));
	return time;
}

inline X_NAMESPACE(core)::TimeVal operator "" _ms(unsigned long long int x)
{
	X_NAMESPACE(core)::TimeVal time;
	time.SetMilliSeconds(static_cast<X_NAMESPACE(core)::TimeVal::TimeType>(x));
	return time;
}

inline X_NAMESPACE(core)::TimeVal operator "" _ns(unsigned long long int x)
{
	X_NAMESPACE(core)::TimeVal time;
	time.SetNanoSeconds(static_cast<X_NAMESPACE(core)::TimeVal::TimeType>(x));
	return time;
}


