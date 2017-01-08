#include <stdafx.h>
#include <Model\ModelManager.h>

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
	models_(g_3dEngineArena, sizeof(ModelResource), X_ALIGN_OF(ModelResource))
{

}

XModelManager::~XModelManager()
{

}

bool XModelManager::Init(void)
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

	pDefaultModel_ = loadModelSync(MODEL_DEFAULT_NAME);
	if (!pDefaultModel_) {
		X_ERROR("ModelManager", "Failed to load default model");
		return false;
	}

	return true;
}


void XModelManager::ShutDown(void)
{
	X_LOG0("ModelManager", "Shutting Down");

	gEnv->pHotReload->addfileType(nullptr, MODEL_FILE_EXTENSION);


	// default model
	if (pDefaultModel_) {
		releaseModel(pDefaultModel_);
	}

	// any left?
	for (const auto& m : models_)
	{
		ModelResource* pModelRes = m.second;
		const auto& name = pModelRes->getName();
		X_WARNING("XModel", "\"%s\" was not deleted. refs: %" PRIi32, name.c_str(), pModelRes->getRefCount());
	}

	models_.free();
}


IModel* XModelManager::findModel(const char* pModelName) const
{
	core::string name(pModelName);

	IModel* pModel = findModel_Internal(name);
	if (pModel) {
		return pModel;
	}

	X_WARNING("ModelManager", "Failed to find model: \"%s\"", pModelName);
	return nullptr;
}

XModelManager::ModelResource* XModelManager::findModel_Internal(const core::string& name) const
{
	return models_.findAsset(name);
}


IModel* XModelManager::loadModel(const char* pModelName)
{
	X_ASSERT_NOT_NULL(pModelName);
	const char* pExt;

	pExt = core::strUtil::FileExtension(pModelName);
	if (pExt)
	{
		// engine should not make requests for models with a extension
		X_ERROR("ModelManager", "Invalid model name extension was included: %s",
			pExt);
		return getDefaultModel();
	}

	core::string name(pModelName);

	// try find it.
	ModelResource* pModelRes = findModel_Internal(name);
	if (pModelRes)
	{
		// inc ref count.
		pModelRes->addReference();
		return pModelRes;
	}

	// we create a model and give it back
	pModelRes = models_.createAsset(name);
	pModelRes->LoadModelAsync(pModelName);

	return pModelRes;
}


IModel* XModelManager::loadModelSync(const char* pModelName)
{
	X_ASSERT_NOT_NULL(pModelName);
	const char* pExt;

	pExt = core::strUtil::FileExtension(pModelName);
	if (pExt)
	{
		// engine should not make requests for models with a extension
		X_ERROR("ModelManager", "Invalid model name extension was included: %s",
			pExt);
		return getDefaultModel();
	}

	core::string name(pModelName);

	// try find it.
	ModelResource* pModelRes = findModel_Internal(name);
	if (pModelRes)
	{
		// inc ref count.
		pModelRes->addReference();
		return pModelRes;
	}

	pModelRes = models_.createAsset(name);

	if (!pModelRes->LoadModel(pModelName))
	{
		if (!pDefaultModel_) { // this could be the default not failed to load.
			releaseModel(pModelRes);
			return nullptr;
		}
		pModelRes->AssignDefault();
		X_WARNING("ModelManager", "Failed to load model: \"%s\"", pModelName);
	}
	else
	{
		X_LOG1("ModelManager", "Loaded model: \"%s\"", pModelName);
	}

	return pModelRes;
}

void XModelManager::releaseModel(IModel* pModel)
{
	ModelResource* pModelRes = static_cast<ModelResource*>(pModel);
	if (pModelRes->removeReference() == 0)
	{
		models_.releaseAsset(pModelRes);
	}
}


XModelManager::ModelResource* XModelManager::createModel(const char* pModelName)
{
	core::string name(pModelName);

	ModelResource* pModelRes = models_.createAsset(name);
	pModelRes->AssignDefault();
	return pModelRes;
}


IModel* XModelManager::getDefaultModel(void)
{
	return pDefaultModel_;
}

void XModelManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
{
	X_UNUSED(jobSys);
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

	for (const auto& model : models_)
	{
		auto* pModelRes = model.second;

		if (!searchPatten || core::strUtil::WildCompare(searchPatten, pModelRes->getName()))
		{
			sorted_models.push_back(pModelRes);
		}
	}

	sortModelsByName(sorted_models);

	X_LOG0("Console", "------------ ^8Models(%" PRIuS ")^7 ---------------", sorted_models.size());

	for (const auto& model : sorted_models)
	{
		X_LOG0("Model", "^2%-32s^7 Lods:^2%i^7 Bones:^2%i^7 BlankBones:^2%i^7 TotalMesh:^2%i^7",
			model->getName(), model->numLods(), model->numBones(), model->numBlankBones(),
			model->numMeshTotal());
	}

	X_LOG0("Console", "------------ ^8Models End^7 --------------");

}

void XModelManager::ReloadModel(const char* pModelName)
{
	core::string name(pModelName);

	ModelResource* pModelRes = findModel_Internal(name);
	if (pModelRes)
	{
		X_LOG0("Model", "Reload model: \"%s\"", name.c_str());
		pModelRes->ReloadAsync();
	}
	else
	{
		X_WARNING("Model", "\"%s\" is not loaded skipping reload", name.c_str());
	}
}

X_NAMESPACE_END
