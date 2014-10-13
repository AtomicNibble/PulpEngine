#pragma once


#ifndef X_3D_ENGINE_BASE_H_
#define X_3D_ENGINE_BASE_H_

struct ICore;

X_NAMESPACE_DECLARE(core,
struct ITimer;
struct IFileSys;
struct IConsole;
)

X_NAMESPACE_DECLARE(render,
struct IRender;
)

X_NAMESPACE_DECLARE(engine,
class XMaterialManager;
)


X_NAMESPACE_BEGIN(engine)

struct XEngineBase
{
	static ICore* pCore;

	static core::ITimer* pTimer;
	static core::IFileSys* pFileSys;
	static core::IConsole* pConsole;
	static render::IRender* pRender;

	// 3d
	static engine::XMaterialManager* pMaterialManager;


	// goats.
	X_INLINE static ICore* getCore(void) { return pCore; }
	X_INLINE static core::ITimer* getTime(void) { return pTimer; }
	X_INLINE static core::IFileSys* getFileSys(void) { return pFileSys; }
	X_INLINE static core::IConsole* getConsole(void) { return pConsole; }
	X_INLINE static render::IRender* getRender(void) { return pRender; }

	X_INLINE static IMaterialManager* getMaterialManager(void) { 
		return reinterpret_cast<IMaterialManager*>(pMaterialManager); 
	}



};

X_NAMESPACE_END

#endif // !X_3D_ENGINE_BASE_H_