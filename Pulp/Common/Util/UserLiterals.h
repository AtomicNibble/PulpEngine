#pragma once

// place to dump user defined literals.


inline size_t operator "" _sz(unsigned long long int x)
{
	return static_cast<size_t>(x);
}