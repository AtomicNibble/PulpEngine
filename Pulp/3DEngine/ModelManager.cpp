#include <stdafx.h>
#include <ModelManager.h>

#include <IConsole.h>

X_NAMESPACE_BEGIN(model)

namespace
{
	void Cmd_ListModels(core::IConsoleCmdArgs* Cmd)
	{
		// optional search criteria
		const char* searchPatten = nullptr;

		if (Cmd->GetArgCount() >= 2) {
			searchPatten = Cmd->GetArg(1);
		}

		(static_cast<XModelManager*>(engine::XEngineBase::getModelManager()))->ListModels(searchPatten);
	}

}


XModelManager::XModelManager() : models_(g_3dEngineArena, 2048)
{

}

XModelManager::~XModelManager()
{

}

void XModelManager::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pHotReload);


	ADD_COMMAND("listModels", Cmd_ListModels, core::VarFlag::SYSTEM, "List all the loaded models.");

	// hotreload support.
	gEnv->pHotReload->addfileType(this, MODEL_FILE_EXTENSION);
}


void XModelManager::ShutDown(void)
{
	X_LOG0("ModelManager", "Shutting Down");

	gEnv->pHotReload->addfileType(nullptr, MODEL_FILE_EXTENSION);

	// any left?
	core::XResourceContainer::ResourceItor it = models_.begin();
	for (; it != models_.end();)
	{

	}
}


IModel* XModelManager::findModel(const char* ModelName) const
{
	X_UNUSED(ModelName);

	return nullptr;
}

IModel* XModelManager::loadModel(const char* ModelName)
{
	X_ASSERT_NOT_NULL(ModelName);
	const char* pExt;
	IModel* iModel;

	pExt = core::strUtil::FileExtension(ModelName);
	if (pExt)
	{
		// engine should not make requests for models with a extension
		X_ERROR("ModelManager", "Invalid model name extension was included: %s",
			pExt);
		return getDefaultModel();
	}
	
	// try find it.
	iModel = findModel(ModelName);

	if (iModel)
	{
		// inc ref count.
		iModel->addRef();
		return iModel;
	}
	
	return nullptr;
}

IModel* XModelManager::getDefaultModel(void)
{

	return nullptr;
}

bool XModelManager::OnFileChange(const char* name)
{
	const char* fileExt;

	fileExt = core::strUtil::FileExtension(name);
	if (fileExt)
	{
		if (core::strUtil::IsEqual(MODEL_FILE_EXTENSION, fileExt))
		{
			X_LOG0("Model", "reload model: \"%s\"", name);


		}
	}
	return true;
}


X_NAMESPACE_END
