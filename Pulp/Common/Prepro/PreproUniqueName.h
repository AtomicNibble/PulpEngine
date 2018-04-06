#pragma once

#ifndef X_PREPROCESSORUNIQUENAX_H
#define X_PREPROCESSORUNIQUENAX_H

/// \def X_PP_UNIQUE_NAME
/// \ingroup Preprocessor
/// \brief Turns a given name into a unique name inside a file.
/// \code
///   // declares two variables having distinct names
///   int X_PP_UNIQUE_NAME(var);
///   int X_PP_UNIQUE_NAME(var);
/// \endcode
/// \remark Only works as long as \ref X_PP_UNIQUE_NAME is not used more than once on the same line.
#define X_PP_UNIQUE_NAME(name) X_PP_JOIN_2(name, __LINE__)

#endif
