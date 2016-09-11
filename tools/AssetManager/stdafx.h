#ifndef PCH_H
#define PCH_H


#ifdef NDEBUG 
#define X_DEBUG 0
#define X_RELEASE 1
#define X_SUPER 0
#else
#define X_DEBUG 1
#define X_RELEASE 0
#define X_SUPER 0
#endif // _DEBUG 

#include <Pulp/Common/EngineCommon.h>
#include <Pulp/Common/IAssetDb.h>


#pragma comment(lib, "Dwrite")

#endif // PCH_H

