#pragma once


X_NAMESPACE_DECLARE(engine,
	class XMaterialManager;
	class TextureManager;
	class CBufferManager;
	class VariableStateManager;

	namespace gui {
		class XGuiManager;
	}

	namespace fx {
		class EffectManager;
	} 
)

X_NAMESPACE_DECLARE(model,
	class XModelManager;
)

X_NAMESPACE_BEGIN(engine)



struct EngineEnv
{
	engine::XMaterialManager* pMaterialMan_;
	engine::TextureManager* pTextureMan_;
	model::XModelManager* pModelMan_;
	gui::XGuiManager* pGuiMan_;

	engine::I3DEngine* p3DEngine_;
	engine::fx::EffectManager* pEffectMan_;
};

extern EngineEnv gEngEnv;



X_NAMESPACE_END
