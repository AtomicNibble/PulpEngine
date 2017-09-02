

#ifndef X_MODELLIB_H_
#define X_MODELLIB_H_

#include <Core\Compiler.h>

#ifndef MODELLIB_EXPORT
#ifdef X_LIB
#define MODELLIB_EXPORT
#else
#ifdef MODEL_LIB_EXPORT
#define MODELLIB_EXPORT X_EXPORT
#else
#define MODELLIB_EXPORT X_IMPORT
#endif // !MODEL_LIB_EXPORT
#endif // X_LIB
#endif // !MODELLIB_EXPORT



#include "Util\ModelUtil.h"

#include "Model\XModel.h"

#endif // !X_MODELLIB_H_