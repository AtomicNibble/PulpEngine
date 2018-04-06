#pragma once

#ifndef X_PREPRO_STRING_H
#define X_PREPRO_STRING_H

#include "PreproStringize.h"
#include "PreproWiden.h"

#define X_STRINGIZE(str) X_PP_STRINGIZE_HELPER(str)
#define X_WIDEN(str) X_PP_WIDEN(str)

#endif // X_PREPRO_STRING_H
