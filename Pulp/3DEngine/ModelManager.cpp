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

	void Cmd_ReloadModel(core::IConsoleCmdArgs* pCmd)
	{
		if (pCmd->GetArgCount() < 2) {
			X_WARNING("Model", "reloadModel <name>");
			return;
		}
		
		XModelManager* pModelManager = (static_cast<XModelManager*>(engine::XEngineBase::getModelManager()));
		const char* pName = pCmd->GetArg(1);

		pModelManager->ReloadModel(pName);
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

} // namespace


/*

I need to redesign this so that models are returned instantly and loaded later.

Allowing for the IO reads to be que'd and performend by one thread.
Creating process jobs at the end, that once complete switch out the data.

Also this will allow for proper hot reload as the handels given back can have thier data updated in background.

This does pose the issue of where to store the actuall data buffer.
Well we are only storing one instance of XModel no matter how many times it's used.
So if that owned the data would be ok.

WE have special case for default model which we want to load / setup in a synchronous manner.

*/

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
	X_ASSERT_NOT_NULL(gEnv->pConsole);


	ADD_COMMAND("listModels", Cmd_ListModels, core::VarFlag::SYSTEM, "List all the loaded models");
	ADD_COMMAND("modelReload", Cmd_ReloadModel, core::VarFlag::SYSTEM, "Reload a model <name>");

	XModel::RegisterVars();

	// hotreload support.
	gEnv->pHotReload->addfileType(this, MODEL_FILE_EXTENSION);

	// should load the default model.

	pDefaultModel_ = loadModelSync("default");
	if (!pDefaultModel_) {
		X_ERROR("ModelManager", "Failed to load default model");
	}
}


void XModelManager::ShutDown(void)
{
	X_LOG0("ModelManager", "Shutting Down");

	gEnv->pHotReload->addfileType(nullptr, MODEL_FILE_EXTENSION);


	// default model
	if (pDefaultModel_) {
		pDefaultModel_->forceRelease();
	}

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

	// we create a model and give it back
	XModel* pModel = X_NEW(XModel, g_3dEngineArena, "3DModel");

	pModel->LoadModelAsync(ModelName);

	models_.AddAsset(ModelName, pModel);

	return pModel;
}

IModel* XModelManager::createModel(const char* ModelName)
{
	XModel* pModel = X_NEW(XModel, g_3dEngineArena, "3DModel");
	if (pModel)
	{
		pModel->AssignDefault();

		models_.AddAsset(ModelName, pModel);
	}

	return pModel;
}

IModel* XModelManager::loadModelSync(const char* ModelName)
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
	return getDefaultModel();
}


IModel* XModelManager::getDefaultModel(void)
{
	X_ASSERT_NOT_NULL(pDefaultModel_);
	return pDefaultModel_;
}

void XModelManager::OnFileChange(const core::Path<char>& name)
{
#if 0
	const char* fileExt;

	fileExt = core::strUtil::FileExtension(name);
	if (fileExt)
	{
		if (core::strUtil::IsEqual(MODEL_FILE_EXTENSION, fileExt))
		{
			// all asset names need forward slashes, for the hash.
			core::Path<char> path(name);
			path.replaceAll('\\', '/');
			path.removeExtension();

			ReloadModel(path.fileName());
			return true;
		}
	}
	return false;
#else
	X_UNUSED(name);
#endif
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

	X_LOG0("Console", "------------ ^8Models(%" PRIuS ")^7 ---------------", sorted_models.size());

	core::Array<XModel*>::ConstIterator it = sorted_models.begin();
	for (; it != sorted_models.end(); ++it)
	{
		const XModel* model = *it;
		X_LOG0("Model", "^2%-32s^7 Lods:^2%i^7 Bones:^2%i^7 BlankBones:^2%i^7 TotalMesh:^2%i^7",
			model->getName(), model->numLods(), model->numBones(), model->numBlankBones(),
			model->numMeshTotal());
	}

	X_LOG0("Console", "------------ ^8Models End^7 --------------");

}

void XModelManager::ReloadModel(const char* pName)
{
	X_ASSERT_NOT_NULL(pName);

	XModel* pModel = static_cast<XModel*>(findModel_Internal(pName));
	if (pModel)
	{
		X_LOG0("Model", "Reload model: \"%s\"", pName);
		pModel->ReloadAsync();
	}
	else
	{
		X_WARNING("Model", "%s is not loaded skipping reload", pName);
	}
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
