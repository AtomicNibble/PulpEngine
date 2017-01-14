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
	static engine::I3DEngine* p3DEngine_;
	static engine::XMaterialManager* pMaterialManager_;
	static model::XModelManager* pModelManager_;

	// gui
	static engine::gui::XGuiManager* pGuiManger_;

	static CBufferManager* pCBufMan_;
	static VariableStateManager* pVariableStateMan_;

	// goats.
	X_INLINE static ICore* getCore(void);
	X_INLINE static core::ITimer* getTime(void);
	X_INLINE static core::IFileSys* getFileSys(void);
	X_INLINE static core::IConsole* getConsole(void);
	X_INLINE static render::IRender* getRender(void);
	X_INLINE static physics::IPhysics* getPhysics(void);

	X_INLINE static engine::I3DEngine* get3DEngine(void);
	X_INLINE static XMaterialManager* getMaterialManager(void);
	X_INLINE static model::XModelManager* getModelManager(void);
	X_INLINE static engine::gui::XGuiManager* getGuiManager(void);
};


X_INLINE ICore* XEngineBase::getCore(void)
{ 
	return pCore_; 
}

X_INLINE core::ITimer* XEngineBase::getTime(void) 
{ 
	return pTimer_; 
}

X_INLINE core::IFileSys* XEngineBase::getFileSys(void) 
{ 
	return pFileSys_;
}

X_INLINE core::IConsole* XEngineBase::getConsole(void) 
{ 
	return pConsole_; 
}

X_INLINE render::IRender* XEngineBase::getRender(void) 
{ 
	return pRender_; 
}

X_INLINE physics::IPhysics* XEngineBase::getPhysics(void)
{ 
	return pPhysics_;
}

X_INLINE engine::I3DEngine* XEngineBase::get3DEngine(void)
{
	return p3DEngine_;
}

X_INLINE XMaterialManager* XEngineBase::getMaterialManager(void)
{
	return pMaterialManager_;
}

X_INLINE model::XModelManager* XEngineBase::getModelManager(void) 
{
	return pModelManager_;
}

X_INLINE engine::gui::XGuiManager* XEngineBase::getGuiManager(void) 
{
	return pGuiManger_;
}


X_NAMESPACE_END

#endif // !X_3D_ENGINE_BASE_H_