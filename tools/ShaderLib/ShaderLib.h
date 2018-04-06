#pragma once

#ifndef X_SHADERLIB_H_
#define X_SHADERLIB_H_

#include <Core\Compiler.h>

#ifndef SHADERLIB_EXPORT
#ifdef X_LIB
#define SHADERLIB_EXPORT
#else
#ifdef SHADER_LIB_EXPORT
#define SHADERLIB_EXPORT X_EXPORT
#else
#define SHADERLIB_EXPORT X_IMPORT
#endif // !SHADER_LIB_EXPORT
#endif // X_LIB
#endif // !SHADERLIB_EXPORT

#include "ShaderBin.h"
#include "SourceBin.h"
#include "HWShader.h"
#include "ShaderUtil.h"

#include "ShaderManager.h"
#include "ShaderPermatation.h"

#endif // !X_SHADERLIB_H_