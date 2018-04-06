#pragma once

#ifndef X_PREPROCESSORWIDEN_H
#define X_PREPROCESSORWIDEN_H

#define X_PP_WIDEN_HELPER(token) L##token

#define X_PP_WIDEN(str) X_PP_WIDEN_HELPER(str)

#endif // X_PREPROCESSORWIDEN_H
