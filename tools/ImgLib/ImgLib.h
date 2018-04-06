#pragma once

#ifndef X_IMGLIB_H_
#define X_IMGLIB_H_

#include <Core\Compiler.h>

#ifndef IMGLIB_EXPORT
#ifdef X_LIB
#define IMGLIB_EXPORT
#else
#ifdef IMG_LIB_EXPORT
#define IMGLIB_EXPORT X_EXPORT
#else
#define IMGLIB_EXPORT X_IMPORT
#endif // !IMG_LIB_EXPORT
#endif // X_LIB
#endif // !IMGLIB_EXPORT

#include "Fmts\TextureLoaderCI.h"
#include "Fmts\TextureLoaderDDS.h"
#include "Fmts\TextureLoaderJPG.h"
#include "Fmts\TextureLoaderPNG.h"
#include "Fmts\TextureLoaderPSD.h"
#include "Fmts\TextureLoaderTGA.h"

#include "Util\TextureUtil.h"
#include "TextureFile.h"

#endif // !X_IMGLIB_H_