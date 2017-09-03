#pragma once


#ifndef X_ANIM_LIB_H_
#define X_ANIM_LIB_H_

#include <Core\Compiler.h>

#ifndef ANIMLIB_EXPORT
#ifdef X_LIB
#define ANIMLIB_EXPORT
#else
#ifdef ANIM_LIB_EXPORT
#define ANIMLIB_EXPORT X_EXPORT
#else
#define ANIMLIB_EXPORT X_IMPORT
#endif // !ANIM_LIB_EXPORT
#endif // X_LIB
#endif // !ANIMLIB_EXPORT


#include "Anim\Anim.h"


#endif // !X_ANIM_LIB_H_