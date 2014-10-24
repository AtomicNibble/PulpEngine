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
	static ICore* pCore_;

	static core::ITimer* pTimer_;
	static core::IFileSys* pFileSys_;
	static core::IConsole* pConsole_;
	static render::IRender* pRender_;

	// 3d
	static engine::XMaterialManager* pMaterialManager_;


	// goats.
	X_INLINE static ICore* getCore(void) { return pCore_; }
	X_INLINE static core::ITimer* getTime(void) { return pTimer_; }
	X_INLINE static core::IFileSys* getFileSys(void) { return pFileSys_; }
	X_INLINE static core::IConsole* getConsole(void) { return pConsole_; }
	X_INLINE static render::IRender* getRender(void) { return pRender_; }

	X_INLINE static IMaterialManager* getMaterialManager(void) { 
		return reinterpret_cast<IMaterialManager*>(pMaterialManager_); 
	}



};

X_NAMESPACE_END

#endif // !X_3D_ENGINE_BASE_H_