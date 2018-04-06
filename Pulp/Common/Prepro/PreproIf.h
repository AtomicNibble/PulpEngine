#pragma once

#ifndef X_PREPROCESSORIF_H_
#define X_PREPROCESSORIF_H_

/// \def X_PP_IF_0
/// \brief Internal macro used by \ref X_PP_IF.
/// \sa X_PP_IF
#define X_PP_IF_0(t, f) f

/// \def X_PP_IF_1
/// \brief Internal macro used by \ref X_PP_IF.
/// \sa X_PP_IF
#define X_PP_IF_1(t, f) t

/// \def X_PP_IF
/// \ingroup Preprocessor
/// \brief Chooses one of two given values based on the value of a condition.
/// \details The condition can either be a boolean value, or an integer.
/// \code
///   // boolean values
///   X_PP_IF(true, 10, 20);			// outputs 10
///   X_PP_IF(false, 10, 20);			// outputs 20
///
///   // integer values
///   X_PP_IF(0, 10, 20);				// outputs 20
///   X_PP_IF(1, 10, 20);				// outputs 10
///   X_PP_IF(10, 10, 20);				// outputs 10
/// \endcode
#define X_PP_IF(cond, t, f) X_PP_JOIN_2(X_PP_IF_, X_PP_TO_BOOL(cond)) \
(t, f)

#endif // X_PREPROCESSORIF_H_
