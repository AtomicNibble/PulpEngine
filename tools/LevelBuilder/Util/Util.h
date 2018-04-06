#pragma once

#define Alloca16(numBytes) ((void*)((((uintptr_t)_alloca((numBytes) + 15)) + 15) & ~15))
