#pragma once

#ifndef X_PREPROCESSORIF_H_
#define X_PREPROCESSORIF_H_

#define X_PP_IF_0(t, f) f
#define X_PP_IF_1(t, f) t
#define X_PP_IF(cond, t, f) X_PP_JOIN_2(X_PP_IF_, X_PP_TO_BOOL(cond)) \
(t, f)

#endif // X_PREPROCESSORIF_H_
