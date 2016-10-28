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

XMaterialManager::XMaterialManager() :
	pListner_(nullptr),
	pDefaultMtl_(nullptr),
	materials_(g_3dEngineArena, 2048)
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


	core::SafeRelease(pDefaultMtl_);

#if 0
	// any left?
	core::XResourceContainer::ResourceItor it = materials_.begin();
	for (; it != materials_.end(); )
	{
		XMaterial* pMat = static_cast<XMaterial*>(it->second);

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


void XMaterialManager::InitDefaults(void)
{
	if (pDefaultMtl_ == nullptr)
	{
		pDefaultMtl_ = static_cast<XMaterial*>(createMaterial("default"));

		// we want texture info to sent and get back a shader item.
		XInputShaderResources input;
		input.material.diffuse.set(1.0, 1.0, 1.0, 1.0);
		input.textures[ShaderTextureIdx::DIFFUSE].name = texture::TEX_DEFAULT_DIFFUSE;
		input.textures[ShaderTextureIdx::BUMP].name = texture::TEX_DEFAULT_BUMP;

		// load the shader / textures needed.
		XShaderItem item = gEnv->pRender->LoadShaderItem(input);

#if 1
	//	X_ASSERT_NOT_IMPLEMENTED();
#else
		pDefaultMtl_->setShaderItem(item);
#endif
		pDefaultMtl_->setCoverage(MaterialCoverage::OPAQUE);
	}
}

// IMaterialManager
IMaterial* XMaterialManager::createMaterial(const char* pMtlName)
{
	XMaterial *pMat = nullptr;

#if 1
	X_UNUSED(pMtlName);
	X_ASSERT_NOT_IMPLEMENTED();
#else

	pMat = static_cast<XMaterial*>(materials_.findAsset(X_CONST_STRING(MtlName)));

	if (pMat)
	{
		pMat->addRef(); // add a ref.
	}
	else
	{
		pMat = X_NEW_ALIGNED( XMaterial, g_3dEngineArena, "Material", X_ALIGN_OF(XMaterial));
		pMat->setName(MtlName);

		if (pListner_) {
			pListner_->OnCreateMaterial(pMat);
		}

		// add it.
		materials_.AddAsset(MtlName, pMat);
	}
#endif

	return pMat;
}

IMaterial* XMaterialManager::findMaterial(const char* pMtlName) const
{
	X_ASSERT_NOT_NULL(pMtlName);

#if 1
	X_ASSERT_NOT_IMPLEMENTED();
#else

	XMaterial* pMaterial = static_cast<XMaterial*>(
		materials_.findAsset(X_CONST_STRING(MtlName)));

	if (pMaterial) {
		return pMaterial;
	}

#endif

	return nullptr;
}

IMaterial* XMaterialManager::loadMaterial(const char* MtlName)
{
	X_ASSERT_NOT_NULL(MtlName);
	const char* pExt;
	IMaterial* iMat;

	pExt = core::strUtil::FileExtension(MtlName);
	if(pExt)
	{
		// engine should not make requests for materials with a extensiob
		X_ERROR("MtlMan", "Invalid mtl name extension was included: %s",
			pExt);
		return getDefaultMaterial();

	}

	// try find it.
	iMat = findMaterial(MtlName);

	if (iMat)
	{
		// inc ref count.
		static_cast<XMaterial*>(iMat)->addRef();
		return iMat;
	}

	// lets look for binary first.
	//iMat = loadMaterialCompiled(MtlName);
	//if (iMat) {
	//	return iMat;
	//}


	X_ERROR("MatMan", "Failed to find material: %s", MtlName);
	return getDefaultMaterial();
}

void XMaterialManager::unregister(IMaterial* pMat)
{
	X_ASSERT_NOT_NULL(pMat);

//	XMaterial* pXMat = (XMaterial*)pMat;

	if (pListner_) {
		pListner_->OnDeleteMaterial(pMat);
	}
}

IMaterial* XMaterialManager::getDefaultMaterial()
{
	return pDefaultMtl_;
}

void XMaterialManager::setListener(IMaterialManagerListener* pListner)
{
//	X_ASSERT_NOT_NULL(pListner); // allow null? (to disable)
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
		IMaterial* mat = static_cast<XMaterial*>(Matit->second);

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
