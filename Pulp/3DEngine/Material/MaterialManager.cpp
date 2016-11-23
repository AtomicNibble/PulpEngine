#include "stdafx.h"
#include "IMaterial.h"
#include "Material.h"
#include "MaterialManager.h"

#include <IFileSys.h>
#include <IShader.h>
#include <IRender.h>
#include <IConsole.h>
#include <ITexture.h>


#include <Memory\AllocationPolicies\MallocFreeAllocator.h>

#include <algorithm>


X_NAMESPACE_BEGIN(engine)

using namespace render::shader;

namespace 
{

	// Commands

	void Cmd_ListMaterials(core::IConsoleCmdArgs* Cmd)
	{
		// optional search criteria
		const char* pSearchPatten = nullptr;

		if (Cmd->GetArgCount() >= 2) {
			pSearchPatten = Cmd->GetArg(1);
		}

		(static_cast<XMaterialManager*>(XEngineBase::getMaterialManager()))->ListMaterials(pSearchPatten);
	}

	void sortMatsByName(core::Array<IMaterial*>& mats)
	{
		std::sort(mats.begin(), mats.end(), [](IMaterial* a, IMaterial* b){
				const char* nameA = a->getName();
				const char* nameB = b->getName();
				return strcmp(nameA, nameB) < 0;
			}
		);
	}

} // namespace

XMaterialManager::XMaterialManager(VariableStateManager& vsMan) :
	vsMan_(vsMan),
	materials_(g_3dEngineArena, sizeof(MaterialResource), X_ALIGN_OF(MaterialResource)),
	pListner_(nullptr),
	pDefaultMtl_(nullptr)
{
}

XMaterialManager::~XMaterialManager()
{

}


bool XMaterialManager::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pHotReload);

	InitDefaults();


	ADD_COMMAND("listMaterials", Cmd_ListMaterials, core::VarFlag::SYSTEM, "List all the loaded materials");

	// hotreload support.
	gEnv->pHotReload->addfileType(this, MTL_FILE_EXTENSION);
	gEnv->pHotReload->addfileType(this, MTL_B_FILE_EXTENSION);

	return true;
}

void XMaterialManager::ShutDown(void)
{
	X_LOG0("MtlManager", "Shutting Down");


	// hotreload support.
	gEnv->pHotReload->addfileType(nullptr, MTL_FILE_EXTENSION);
	gEnv->pHotReload->addfileType(nullptr, MTL_B_FILE_EXTENSION);


	releaseMaterial(pDefaultMtl_);

#if 0
	// any left?
	core::XResourceContainer::ResourceItor it = materials_.begin();
	for (; it != materials_.end(); )
	{
		Material* pMat = static_cast<Material*>(it->second);

		++it;

		if (!pMat) {
			continue;
		}

		X_WARNING("Material", "\"%s\" was not deleted", pMat->getName());

		pMat->release();
	}
#endif 
}

void XMaterialManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
{
	X_UNUSED(jobSys);
#if 0
	const char* fileExt;		

	fileExt = core::strUtil::FileExtension(name);
	if (fileExt)
	{
		if (core::strUtil::IsEqual(MTL_FILE_EXTENSION, fileExt))
		{
			X_LOG0("Material", "reload material: \"%s\"", name);


		}
		else if (core::strUtil::IsEqual(MTL_B_FILE_EXTENSION, fileExt))
		{
			//	X_LOG0("Material", "reload material: \"%s\"", name);


		}
	}
	return true;
#else
	X_UNUSED(name);
#endif
}


// IMaterialManager
Material* XMaterialManager::createMaterial(const char* pMtlName)
{
	core::string name(pMtlName);

	MaterialResource* pMtl = findMaterial_Internal(name);
	if (pMtl) {
		// add ref?
		pMtl->addReference();
		X_WARNING("MtlMan", "Create material called with name of exsisting material: \"%s\"", pMtlName);
		return pMtl;
	}

	return createMaterial_Internal(pMtlName);
}

Material* XMaterialManager::findMaterial(const char* pMtlName) const
{
	core::string name(pMtlName);

	Material* pMtl = findMaterial_Internal(name);
	if (pMtl) {
		return pMtl;
	}

	X_WARNING("MatMan", "Failed to find material: \"%s\"", pMtlName);
	return nullptr;
}

Material* XMaterialManager::loadMaterial(const char* pMtlName)
{
	X_ASSERT_NOT_NULL(pMtlName);

	const char* pExt = core::strUtil::FileExtension(pMtlName);
	if(pExt)
	{
		// engine should not make requests for materials with a extensiob
		X_ERROR("MtlMan", "Invalid mtl name extension was included: %s",
			pExt);
		return getDefaultMaterial();

	}

	core::string name(pMtlName);


	// try find it.
	MaterialResource* pMatRes = findMaterial_Internal(name);

	if (pMatRes)
	{
		// inc ref count.
		pMatRes->addReference();
		return pMatRes;
	}

	// lets look for binary first.
	//iMat = loadMaterialCompiled(MtlName);
	//if (iMat) {
	//	return iMat;
	//}


	X_ERROR("MatMan", "Failed to find material: %s", pMtlName);
	return getDefaultMaterial();
}

void XMaterialManager::releaseMaterial(Material* pMat)
{
	MaterialResource* pMatRes = reinterpret_cast<MaterialResource*>(pMat);
	if (pMatRes->removeReference() == 0)
	{
		materials_.releaseAsset(pMatRes);
	}
}


XMaterialManager::MaterialResource* XMaterialManager::createMaterial_Internal(const char* pMtlName)
{
	core::string name(pMtlName);

	// internal create expects you to know no duplicates
	X_ASSERT(findMaterial_Internal(name) == nullptr, "Creating a material that already exsists")();

	auto pMatRes = materials_.createAsset(name);
	pMatRes->setName(name);

	return pMatRes;
}

XMaterialManager::MaterialResource* XMaterialManager::findMaterial_Internal(const core::string& name) const
{
	return materials_.findAsset(name);
}

void XMaterialManager::InitDefaults(void)
{
	if (pDefaultMtl_ == nullptr)
	{
		pDefaultMtl_ = createMaterial_Internal(MTL_DEFAULT_NAME);

		// we want texture info to sent and get back a shader item.
//		XInputShaderResources input;
//		input.material.diffuse.set(1.0, 1.0, 1.0, 1.0);
//		input.textures[ShaderTextureIdx::DIFFUSE].name = texture::TEX_DEFAULT_DIFFUSE;
//		input.textures[ShaderTextureIdx::BUMP].name = texture::TEX_DEFAULT_BUMP;

		// load the shader / textures needed.
	//	XShaderItem item = gEnv->pRender->LoadShaderItem(input);

#if 1
		//	X_ASSERT_NOT_IMPLEMENTED();
#else
		pDefaultMtl_->setShaderItem(item);
#endif
	//	pDefaultMtl_->setCoverage(MaterialCoverage::OPAQUE);
	}
}



Material* XMaterialManager::getDefaultMaterial(void)
{
	return pDefaultMtl_;
}

void XMaterialManager::setListener(IMaterialManagerListener* pListner)
{
	pListner_ = pListner;
}

// ~IMaterialManager


// ICoreEventListener
void XMaterialManager::OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam)
{
	X_UNUSED(event);
	X_UNUSED(wparam);
	X_UNUSED(lparam);



}
// ~ICoreEventListener


void XMaterialManager::ListMaterials(const char* pSearchPatten) const
{
#if 1
	X_UNUSED(pSearchPatten);
	X_ASSERT_NOT_IMPLEMENTED();
#else
	core::Array<IMaterial*> sorted_mats(g_3dEngineArena);
	sorted_mats.setGranularity(materials_.size());

	MaterialCon::ResourceConstItor Matit;

	for (Matit = materials_.begin(); Matit != materials_.end(); ++Matit)
	{
		IMaterial* mat = static_cast<Material*>(Matit->second);

		if (!searchPatten || core::strUtil::WildCompare(pSearchPatten, mat->getName()))
		{
			sorted_mats.push_back(mat);
		}
	}

	sortMatsByName(sorted_mats);

	X_LOG0("Console", "------------ ^8Materials(%" PRIuS ")^7 ------------", sorted_mats.size());

	core::Array<IMaterial*>::ConstIterator it = sorted_mats.begin();
	for (; it != sorted_mats.end(); ++it)
	{
		const IMaterial* mat = *it;
		X_LOG0("Material", "^2\"%s\"^7", 
			mat->getName());
	}

	X_LOG0("Console", "------------ ^8Materials End^7 -----------");
#endif
}


X_NAMESPACE_END
