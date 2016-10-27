#include "stdafx.h"
#include "MaterialManager.h"

#include "../3DEngine/Material/Material.h"




X_NAMESPACE_BEGIN(lvl)

using namespace core::xml::rapidxml;

namespace
{
	void* XmlAllocate(std::size_t size)
	{
		return X_NEW_ARRAY(char, size, g_arena, "xmlPool");
	}

	void XmlFree(void* pointer)
	{
		char* pChar = (char*)pointer;
		X_DELETE_ARRAY(pChar, g_arena);
	}

} // namespace


MatManager::MatManager() :
materials_(g_arena, 2048),
pDefaultMtl_(nullptr),
pFileSys_(nullptr)
{
}

MatManager::~MatManager()
{

}

bool MatManager::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pFileSys);

	pFileSys_ = gEnv->pFileSys;

	return loadDefaultMaterial();
}

void MatManager::ShutDown(void)
{
	// clear everything up.

	core::XResourceContainer::ResourceItor it = materials_.begin();
	for (; it != materials_.end();)
	{
		engine::XMaterial * pMat = (engine::XMaterial *)it->second;

		++it;

		if (!pMat)
			continue;

		X_DELETE(pMat, g_arena);
	//	while (pMat->release() > 0) {
	//	}
	}
}

bool MatManager::loadDefaultMaterial(void)
{
	// even tho loadMaterial returns default when not found, default is set as null
	// so we are able to tell if it loaded.
	engine::IMaterial* pDefault = loadMaterial(engine::MTL_DEFAULT_NAME);
	if (!pDefault) {
		X_ERROR("MatMan", "Failed to load default material");
		return false;
	}

	this->pDefaultMtl_ = pDefault;
	return true;
}

engine::IMaterial* MatManager::getDefaultMaterial(void) const
{
	return pDefaultMtl_;
}

engine::IMaterial* MatManager::createMaterial(const char* MtlName)
{
	engine::XMaterial *pMat = nullptr;

	pMat = reinterpret_cast<engine::XMaterial*>(materials_.findAsset(MtlName));

	if (pMat)
	{
		pMat->addRef(); // add a ref.
	}
	else
	{
#if 1
		X_ASSERT_NOT_IMPLEMENTED();
		return nullptr;
#else

		pMat = X_NEW(engine::XMaterial, g_arena, "Material");
		pMat->setName(MtlName);

		// add it.
		materials_.AddAsset(MtlName, pMat);
#endif
	}

	return pMat;
}


engine::IMaterial* MatManager::findMaterial(const char* MtlName) const
{
	X_ASSERT_NOT_NULL(MtlName);

	engine::XMaterial* pMaterial = (engine::XMaterial*)materials_.findAsset(MtlName);
	if (pMaterial)
		return pMaterial;

	return nullptr;
}

engine::IMaterial* MatManager::loadMaterial(const char* MtlName)
{
	X_ASSERT_NOT_NULL(MtlName);
	const char* pExt;
	engine::IMaterial* iMat;

	if ((pExt = core::strUtil::FileExtension(MtlName)))
	{
		// engine should not make requests for materials with a extensiob
		X_ERROR("MtlMan", "Invalid mtl name extension was included: %s",
			pExt);
		return nullptr;
	}

	// try find it.
	if (iMat = findMaterial(MtlName)){
		// inc ref count.
		((engine::XMaterial*)iMat)->addRef();
		return iMat;
	}

	// lets look for binary first.
	if ((iMat = loadMaterialCompiled(MtlName))) {
		return iMat;
	}

	if ((iMat = loadMaterialXML(MtlName))) {
		return iMat;
	}

	X_ERROR("MatMan", "Failed to find material: %s", MtlName);

	// return default?
	return getDefaultMaterial();
}

// loaders.

engine::IMaterial* MatManager::loadMaterialXML(const char* MtlName)
{
#if 1
	X_ASSERT_NOT_IMPLEMENTED();
	return nullptr;
#else

	X_ASSERT_NOT_NULL(MtlName);

	core::XFileScoped file;
	core::Path<char> path;
	size_t length;
	engine::XMaterial* pMat = nullptr;

	path /= "materials/";
	path.setFileName(MtlName);
	path.setExtension(engine::MTL_FILE_EXTENSION);

//	if (!pFileSys_->fileExists(path.c_str())) {
//		return pMat;
//	}

	if (file.openFile(path.c_str(), core::fileMode::READ))
	{
		length = safe_static_cast<size_t, uint64_t>(file.remainingBytes());

		char* pText = X_NEW_ARRAY_ALIGNED(char, length + 1, g_arena, "MaterialXMLBuf", 4);

		// add a null term baby!
		pText[length] = '\0';

		if (file.read(pText, length) == length)
		{
			xml_document<> doc;    // character type defaults to char
			doc.set_allocator(XmlAllocate, XmlFree);
			doc.parse<0>(pText);    // 0 means default parse flags

			xml_node<>* node = doc.first_node("material");
			if (node)
			{
				pMat = (engine::XMaterial*)createMaterial(MtlName);
				if (!engine::XMaterial::ProcessMaterialXML(pMat, node))
				{
					pMat = nullptr;
				}
			}
			else
			{
				X_ERROR("Mtl", "material is missing <material> node");
			}
		}

		X_DELETE_ARRAY(pText, g_arena);
	}

	return pMat;
#endif
}

engine::IMaterial* MatManager::loadMaterialCompiled(const char* MtlName)
{
#if 1
	X_ASSERT_NOT_IMPLEMENTED();
	return nullptr;
#else

	X_ASSERT_NOT_NULL(MtlName);
	core::XFileScoped file;
	core::Path<char> path;
	engine::MaterialHeader hdr;
	engine::XMaterial* pMat = nullptr;

	path /= "materials/";
	path.setFileName(MtlName);
	path.setExtension(engine::MTL_B_FILE_EXTENSION);

	if (!pFileSys_->fileExists(path.c_str())) {
		return pMat;
	}

	if (file.openFile(path.c_str(), core::fileMode::READ))
	{
		// this needs updating to use new logic.
		X_ASSERT_NOT_IMPLEMENTED();

		file.readObj(hdr);

		if (hdr.isValid())
		{
			// read the file entries.
			engine::MaterialTexture texture[render::shader::ShaderTextureIdx::ENUM_COUNT];

			uint32_t Num = core::Min<uint32_t>(hdr.numTextures, render::shader::ShaderTextureIdx::ENUM_COUNT);

			if (file.readObjs(texture, Num) == Num)
			{
				pMat = (engine::XMaterial*)createMaterial(MtlName);
				pMat->setCullType(hdr.cullType);
			//	pMat->setSurfaceType(hdr.type);
			}
			else
			{
				X_ERROR("Mtlb", "failed to read texture info");
			}
		}
		else
		{
			X_ERROR("Mtlb", "material header is invalid.");
		}
	}

	return pMat;
#endif
}

X_NAMESPACE_END