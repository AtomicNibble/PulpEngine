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
class CBufferManager;
class VariableStateManager;

	namespace gui {
		class XGuiManager;
	}
)

X_NAMESPACE_DECLARE(model,
class XModelManager;
)

X_NAMESPACE_DECLARE(physics,
struct IPhysics;
struct IModelManager;
)


X_NAMESPACE_BEGIN(engine)


struct XEngineBase
{
	static ICore* pCore_;

	static core::ITimer* pTimer_;
	static core::IFileSys* pFileSys_;
	static core::IConsole* pConsole_;
	static render::IRender* pRender_;
	static physics::IPhysics* pPhysics_;

	// 3d
	static engine::XMaterialManager* pMaterialManager_;
	static model::XModelManager* pModelManager_;

	// gui
	static engine::gui::XGuiManager* pGuiManger_;

	static CBufferManager* pCBufMan_;
	static VariableStateManager* pVariableStateMan_;

	// goats.
	X_INLINE static ICore* getCore(void) { return pCore_; }
	X_INLINE static core::ITimer* getTime(void) { return pTimer_; }
	X_INLINE static core::IFileSys* getFileSys(void) { return pFileSys_; }
	X_INLINE static core::IConsole* getConsole(void) { return pConsole_; }
	X_INLINE static render::IRender* getRender(void) { return pRender_; }

	X_INLINE static IMaterialManager* getMaterialManager(void) { 
		return reinterpret_cast<IMaterialManager*>(pMaterialManager_); 
	}
	X_INLINE static physics::IPhysics* getPhysics(void);

	X_INLINE static model::IModelManager* getModelManager(void) {
		return reinterpret_cast<model::IModelManager*>(pModelManager_);
	}

	X_INLINE static engine::gui::XGuiManager* getGuiManager(void) {
		return pGuiManger_;
	}

X_INLINE physics::IPhysics* XEngineBase::getPhysics(void)
{ 
	return pPhysics_;
}

};

X_NAMESPACE_END

#endif // !X_3D_ENGINE_BASE_H_