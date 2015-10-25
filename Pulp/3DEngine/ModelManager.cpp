#include <stdafx.h>
#include <ModelManager.h>

#include <IConsole.h>

#include "XModel.h"

#include <algorithm>


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

	static void sortModelsByName(core::Array<XModel*>& models)
	{
		std::sort(models.begin(), models.end(),
			[](XModel* a, XModel* b)
		{
			const char* nameA = a->getName();
			const char* nameB = b->getName();
			return strcmp(nameA, nameB) < 0;
		}
		);
	}
}


XModelManager::XModelManager() : 
	pDefaultModel_(nullptr),
	models_(g_3dEngineArena, 2048)
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

	// should load the default model.

}


void XModelManager::ShutDown(void)
{
	X_LOG0("ModelManager", "Shutting Down");

	gEnv->pHotReload->addfileType(nullptr, MODEL_FILE_EXTENSION);

	// any left?
	core::XResourceContainer::ResourceItor it = models_.begin();
	for (; it != models_.end();)
	{
		IModel* pModel = static_cast<XModel*>(it->second);
		
		++it;

		if (!pModel) {
			continue;
		}

		X_WARNING("XModel", "\"%s\" was not deleted", pModel->getName());
		pModel->forceRelease();
	}

	// default model
	if (pDefaultModel_) {
		pDefaultModel_->forceRelease();
	}
}


IModel* XModelManager::findModel(const char* ModelName) const
{
	X_UNUSED(ModelName);

	IModel* pModel = findModel_Internal(ModelName);

	if (pModel) {
		return pModel;
	}

	X_WARNING("ModelManager", "Failed to find model: \"%s\"", ModelName);
	return nullptr;
}

IModel* XModelManager::findModel_Internal(const char* ModelName) const
{
	X_UNUSED(ModelName);

	XModel* pModel = static_cast<XModel*>(
		models_.findAsset(X_CONST_STRING(ModelName)));

	return pModel;
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
	iModel = findModel_Internal(ModelName);

	if (iModel)
	{
		// inc ref count.
		iModel->addRef();
		return iModel;
	}

	// try load it.
	iModel = LoadCompiledModel(ModelName);
	if (iModel) {
		X_LOG1("ModelManager", "Loaded model: \"%s\"", ModelName);
		return iModel;
	}

	X_WARNING("ModelManager", "Failed to load model: \"%s\"", ModelName);
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

void XModelManager::ListModels(const char* searchPatten) const
{
	core::Array<XModel*> sorted_models(g_3dEngineArena);
	sorted_models.setGranularity(models_.size());

	ModelCon::ResourceConstItor ModelIt;

	for (ModelIt = models_.begin(); ModelIt != models_.end(); ++ModelIt)
	{
		XModel* model = static_cast<XModel*>(ModelIt->second);

		if (!searchPatten || core::strUtil::WildCompare(searchPatten, model->getName()))
		{
			sorted_models.push_back(model);
		}
	}

	sortModelsByName(sorted_models);

	X_LOG0("Console", "------------ ^8Models(%i)^7 ------------", sorted_models.size());
	X_LOG_BULLET;

	core::Array<XModel*>::ConstIterator it = sorted_models.begin();
	for (; it != sorted_models.end(); ++it)
	{
		const XModel* model = *it;
		X_LOG0("Model", "^2\"%s\"^7",
			model->getName());
	}

	X_LOG0("Console", "------------ ^8Materials End^7 ------------");

}

IModel* XModelManager::LoadCompiledModel(const char* ModelName)
{
	// check if the file exsists.
	XModel* pModel = X_NEW(XModel, g_3dEngineArena, "3DModel");
	if (pModel)
	{
		if (pModel->LoadModel(ModelName))
		{
			models_.AddAsset(ModelName, pModel);
		}
		else
		{
			X_DELETE_AND_NULL(pModel, g_3dEngineArena);
		}
	}
	return pModel;
}

X_NAMESPACE_END
