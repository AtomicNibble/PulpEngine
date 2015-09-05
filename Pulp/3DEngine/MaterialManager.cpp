#include "stdafx.h"
#include "IMaterial.h"
#include "Material.h"
#include "MaterialManager.h"

#include <IFileSys.h>
#include <IShader.h>
#include <IRender.h>
#include <IConsole.h>
#include <ITexture.h>

#include <String\Xml.h>
#include <String\Path.h>

#include <Memory\AllocationPolicies\MallocFreeAllocator.h>

#include <algorithm>


X_NAMESPACE_BEGIN(engine)

using namespace core::xml::rapidxml;
using namespace shader;

namespace {

	X_DECLARE_FLAGS(MtlXmlFlags)(NAME, FLAGS, SURFACETYPE);


	struct TextureType
	{
		const char* name;
		shader::ShaderTextureIdx::Enum type;
	};

	TextureType g_textTypes[] = {
		{ "albedo", shader::ShaderTextureIdx::DIFFUSE },
		{ "diffuse", shader::ShaderTextureIdx::DIFFUSE },
		{ "bump", shader::ShaderTextureIdx::BUMP },
		{ "spec", shader::ShaderTextureIdx::SPEC },
	};

	static const int g_textTypesNum = (sizeof(g_textTypes) / sizeof(g_textTypes[0]));


	bool strToVec4(const char* pStr, Vec4f& vecOut)
	{
		size_t len = core::strUtil::strlen(pStr);
		if (len > 128)
			return false;

		Vec4f temp;

		// it's comma seperated.
		if (sscanf_s(pStr, "%f,%f,%f,%f", &temp.x, &temp.y, &temp.z, &temp.w) == 4) {
			vecOut = temp;
			return true;
		}

		return false;
	}

	bool ProcessMaterialXML(XMaterial* pMaterial, xml_node<>* node)
	{
		X_ASSERT_NOT_NULL(pMaterial);
		X_ASSERT_NOT_NULL(node);
		xml_attribute<>* attr;
		xml_node<>* params;
		xml_node<>* textures;
		xml_node<>* texture;

		int numTextures = 0;
		int i;

		// name="" flags="0" SurfaceType="none"
		Flags<MtlXmlFlags> flags;
		XInputShaderResources input;


		for (attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			const char* begin = attr->name();
			const char* end = begin + attr->name_size();

			if (core::strUtil::IsEqual(begin, end, "name"))
			{
				flags.Set(MtlXmlFlags::NAME);

			}
			else if (core::strUtil::IsEqual(begin, end, "flags"))
			{
				flags.Set(MtlXmlFlags::FLAGS);

				uint32_t Matflags = core::strUtil::StringToInt<uint32_t>(attr->value());
				pMaterial->setFlags(Matflags);
			}
			else if (core::strUtil::IsEqual(begin, end, "SurfaceType"))
			{
				flags.Set(MtlXmlFlags::SURFACETYPE);

				MaterialSurType::Enum type = core::strUtil::StringToInt<MaterialSurType::Enum>(attr->value());
				pMaterial->setSurfaceType(type);
			}
			else
			{
				X_WARNING("Mtl", "unkown attribute: %s on material node", begin);
			}
		}

		if (!flags.IsSet(MtlXmlFlags::NAME))
			X_ERROR("Mtl", "material node missing 'name' attribute");
		if (!flags.IsSet(MtlXmlFlags::FLAGS))
			X_ERROR("Mtl", "material node missing 'flags' attribute");
		if (!flags.IsSet(MtlXmlFlags::SURFACETYPE))
			X_ERROR("Mtl", "material node missing 'SurfaceType' attribute");

		// are all 3 set ?
		if (core::bitUtil::CountBits(flags.ToInt()) != MtlXmlFlags::FLAGS_COUNT)
			return false;
		
		// texture me up !
		textures = node->first_node("textures");

		if (textures)
		{
			// type="albedo" image_name="default.dds"
			for (texture = textures->first_node("texture"); texture;
				texture = texture->next_sibling())
			{
				shader::ShaderTextureIdx::Enum texId;
				core::StackString<256> name;

				attr = texture->first_attribute("type", 4);
				if (attr)
				{
					// check it's a valid type
					const char* typeBegin = attr->value();
					const char* typeEnd = typeBegin + attr->value_size();

					for (i = 0; i < g_textTypesNum; i++)
					{
						if (core::strUtil::IsEqual(typeBegin, typeEnd, g_textTypes[i].name))
						{
							break;
						}
					}

					if (i == g_textTypesNum)
					{
						X_ERROR("Mtl", "material texture 'type' is invalid.");
						continue;
					}

					texId = g_textTypes[i].type;
				}
				else
				{
					X_ERROR("Mtl", "material texture node missing 'type' attribute");
					continue;
				}

				attr = texture->first_attribute("image_name", 10);
				if (attr)
				{
					if (attr->value_size() < 2)
					{
						X_ERROR("Mtl", "material texture 'image_name' is smaller than 2");
						continue;
					}
					if (attr->value_size() > 256)
					{
						X_ERROR("Mtl", "material texture 'image_name' is greater than 256");
						continue;
					}

					name.append(attr->value());
				}
				else
				{
					X_ERROR("Mtl", "material texture node missing 'image_name' attribute");
					continue;
				}

				// we have both type + image_name.
				input.textures[texId].name = name.c_str();

				numTextures++;
			}
		}
		else
		{
			X_ERROR("Mtl", "material missing textures node");
			return false;
		}

		// texture loading done.
		// how many do we have :| ?
		// 1-2-3-4 tickle my back door?
		if (numTextures == 0) {
			X_ERROR("Mtl", "material has zero valid textures");
			return false;
		}

		// do we have a albedo?
		if (input.textures[shader::ShaderTextureIdx::DIFFUSE].name.isEmpty()) {
			X_WARNING("Mtl", "material has no albedo texture defined");
		}

		if (input.textures[shader::ShaderTextureIdx::BUMP].name.isEmpty()) {
			X_WARNING("Mtl", "material has no bump map assigning identity");
			input.textures[shader::ShaderTextureIdx::BUMP].name = texture::TEX_DEFAULT_BUMP;

		}

		// Ok now params!
		params = node->first_node("Params", 6, false);
		if (params)
		{
			for (attr = params->first_attribute(); attr;
				attr = attr->next_attribute())
			{
				const char* begin = attr->name();
				const char* end = begin + attr->name_size();
				const char* value = attr->value();
				Vec4f vec;

				// diffuse="1,1,1"
				// specular="1,1,1"
				// emissive="1,1,1"
				// shineness="1"
				// opacity="1"
				if (core::strUtil::IsEqual(begin, end, "diffuse"))
				{
					if (!strToVec4(value, vec))
						X_ERROR("Mtl", "material param 'diffuse' has a invalid value");
					else
						input.material.diffuse = Color(vec);
				}
				else if (core::strUtil::IsEqual(begin, end, "specular"))
				{
					if (!strToVec4(value, vec))
						X_ERROR("Mtl", "material param 'specular' has a invalid value");
					else
						input.material.specular = Color(vec);
				}
				else if (core::strUtil::IsEqual(begin, end, "emissive"))
				{
					if (!strToVec4(value, vec))
						X_ERROR("Mtl", "material param 'emissive' has a invalid value");
					else
						input.material.emissive = Color(vec);
				}
				else if (core::strUtil::IsEqual(begin, end, "shineness"))
				{
					if (!core::strUtil::IsNumeric(value))
						X_ERROR("Mtl", "material param 'shineness' is not a valid float");
					else
						input.material.specShininess = core::strUtil::StringToFloat<float>(value);
				}
				else if (core::strUtil::IsEqual(begin, end, "opacity"))
				{
					if (!core::strUtil::IsNumeric(value))
						X_ERROR("Mtl", "material param 'opacity' is not a valid float");
					else
						input.opacity = core::strUtil::StringToFloat<float>(value);
				}

			}
		}

		// anything else?
		XShaderItem item = gEnv->pRender->LoadShaderItem(input);

		pMaterial->setShaderItem(item);

		return true;
	}

	// Commands

	void Cmd_ListMaterials(core::IConsoleCmdArgs* Cmd)
	{
		// optional search criteria
		const char* searchPatten = nullptr;

		if (Cmd->GetArgCount() >= 2) {
			searchPatten = Cmd->GetArg(1);
		}

		((XMaterialManager*)XEngineBase::getMaterialManager())->ListMaterials(searchPatten);
	}

	static void sortMatsByName(core::Array<IMaterial*>& mats)
	{
		std::sort(mats.begin(), mats.end(),
			[](IMaterial* a, IMaterial* b)
			{
				const char* nameA = a->getName();
				const char* nameB = b->getName();
				return strcmp(nameA, nameB) < 0;
			}
		);
	}
}

XMaterialManager::XMaterialManager() :
	pListner_(nullptr),
	pDefaultMtl_(nullptr),
	materials_(g_3dEngineArena, 2048)
{
}

XMaterialManager::~XMaterialManager()
{

}


void XMaterialManager::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pHotReload);

	InitDefaults();


	ADD_COMMAND("listMaterials", Cmd_ListMaterials, core::VarFlag::SYSTEM, "List all the loaded materials.");

	// hotreload support.
	gEnv->pHotReload->addfileType(this, MTL_FILE_EXTENSION);
	gEnv->pHotReload->addfileType(this, MTL_B_FILE_EXTENSION);
}


void XMaterialManager::ShutDown(void)
{
	X_LOG0("MtlManager", "Shutting Down");


	// hotreload support.
	gEnv->pHotReload->addfileType(nullptr, MTL_FILE_EXTENSION);
	gEnv->pHotReload->addfileType(nullptr, MTL_B_FILE_EXTENSION);


	core::SafeRelease(pDefaultMtl_);

	// any left?
	core::XResourceContainer::ResourceItor it = materials_.begin();
	for (; it != materials_.end(); )
	{
		XMaterial* pMat = (XMaterial*)it->second;

		++it;

		if (!pMat)
			continue;

		X_WARNING("Material", "\"%s\" was not deleted", pMat->getName());

		pMat->release();
	}
}

bool XMaterialManager::OnFileChange(const char* name)
{
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
}


void XMaterialManager::InitDefaults(void)
{
	if (pDefaultMtl_ == nullptr)
	{
		pDefaultMtl_ = (XMaterial*)createMaterial("default");

		// we want texture info to sent and get back a shader item.
		XInputShaderResources input;
		input.material.diffuse.set(1.0, 1.0, 1.0, 1.0);
		input.textures[ShaderTextureIdx::DIFFUSE].name = texture::TEX_DEFAULT_DIFFUSE;
		input.textures[ShaderTextureIdx::BUMP].name = texture::TEX_DEFAULT_BUMP;

		// load the shader / textures needed.
		XShaderItem item = gEnv->pRender->LoadShaderItem(input);

		pDefaultMtl_->setShaderItem(item);
	}
}

// IMaterialManager
IMaterial* XMaterialManager::createMaterial(const char* MtlName)
{
	XMaterial *pMat = nullptr;

	pMat = (XMaterial*)materials_.findAsset(MtlName);

	if (pMat)
	{
		pMat->addRef(); // add a ref.
	}
	else
	{
		pMat = X_NEW_ALIGNED( XMaterial, g_3dEngineArena, "Material", X_ALIGN_OF(XMaterial));
		pMat->setName(MtlName);

		if (pListner_)
			pListner_->OnCreateMaterial(pMat);

		// add it.
		materials_.AddAsset(MtlName, pMat);
	}

	return pMat;
}

IMaterial* XMaterialManager::findMaterial(const char* MtlName) const
{
	X_ASSERT_NOT_NULL(MtlName);

	XMaterial* pMaterial = (XMaterial*)materials_.findAsset(MtlName);
	if (pMaterial)
		return pMaterial;

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
		((XMaterial*)iMat)->addRef();
		return iMat;
	}

	// lets look for binary first.
	iMat = loadMaterialCompiled(MtlName);
	if (iMat) {
		return iMat;
	}

	iMat = loadMaterialXML(MtlName);
	if (iMat) {
		return iMat;
	}

	X_ERROR("MatMan", "Failed to find material: %s", MtlName);
	return getDefaultMaterial();
}

void XMaterialManager::unregister(IMaterial* pMat)
{
	X_ASSERT_NOT_NULL(pMat);

//	XMaterial* pXMat = (XMaterial*)pMat;

	if (pListner_)
		pListner_->OnDeleteMaterial(pMat);
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

namespace 
{
	void* XmlAllocate(std::size_t size)
	{
		return X_NEW_ARRAY(char, size, g_3dEngineArena, "xmlPool");
	}

	void XmlFree(void* pointer)
	{
		char* pChar = (char*)pointer;
		X_DELETE_ARRAY(pChar, g_3dEngineArena);
	}
}

IMaterial* XMaterialManager::loadMaterialXML(const char* MtlName)
{
	X_ASSERT_NOT_NULL(MtlName);

	core::XFileScoped file;
	core::Path<char> path;
	size_t length;
	XMaterial* pMat = nullptr;

	path /= "materials/";
	path.setFileName(MtlName);
	path.setExtension(MTL_FILE_EXTENSION);

	if (!pFileSys_->fileExists(path.c_str())) {
		return pMat;
	}

	if (file.openFile(path.c_str(), core::fileMode::READ))
	{
		length = file.remainingBytes();


		char* pText = X_NEW_ARRAY_ALIGNED(char, length + 1, g_3dEngineArena, "MaterialXMLBuf", 4);

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
				pMat = (XMaterial*)createMaterial(MtlName);
				if (!ProcessMaterialXML(pMat, node))
				{
					// failed to load it :|
					unregister(pMat);
					pMat = nullptr;
				}
			}
			else
			{
				X_ERROR("Mtl", "material is missing <material> node");
			}
		}

		X_DELETE_ARRAY(pText, g_3dEngineArena);
	}

	return pMat;
}


IMaterial* XMaterialManager::loadMaterialCompiled(const char* MtlName)
{
	X_ASSERT_NOT_NULL(MtlName);
	core::XFileScoped file;
	core::Path<char> path;
	MaterialHeader hdr;
	XMaterial* pMat = nullptr;
	uint32_t i;

	path /= "materials/";
	path.setFileName(MtlName);
	path.setExtension(MTL_B_FILE_EXTENSION);

	if (!pFileSys_->fileExists(path.c_str())) {
		return pMat;
	}

	if (file.openFile(path.c_str(), core::fileMode::READ))
	{
		file.readObj(hdr);

		if (hdr.isValid())
		{
			// read the file entries.
			MaterialTexture texture[shader::ShaderTextureIdx::ENUM_COUNT];

			uint32_t Num = core::Min<uint32_t>(hdr.numTextures, shader::ShaderTextureIdx::ENUM_COUNT);

			if (file.readObjs(texture, Num) == Num)
			{
				pMat = (XMaterial*)createMaterial(MtlName);
				pMat->CullType_ = hdr.cullType;
				pMat->MatSurfaceType_ = hdr.type;
				
				XInputShaderResources input;
				input.material.diffuse = hdr.diffuse;
				input.material.specular = hdr.specular;
				input.material.emissive = hdr.emissive;
				input.material.specShininess = hdr.shineness;
				input.opacity = hdr.opacity;
				
				for (i = 0; i < Num; i++)
				{
					X_ASSERT(texture[i].type < static_cast<int32_t>(shader::ShaderTextureIdx::ENUM_COUNT), "invalid texture type")();
					input.textures[texture[i].type].name = texture[i].name.c_str();
				}

				// load the shader / textures needed.
				XShaderItem item = gEnv->pRender->LoadShaderItem(input);

				pMat->setShaderItem(item);

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
}

bool XMaterialManager::saveMaterialCompiled(IMaterial* pMat_)
{
	X_ASSERT_NOT_NULL(pMat_);
	core::Path<char> path;
	XMaterial* pMat;
	MaterialHeader hdr;
	uint32_t i, numTex;

	pMat = (XMaterial*)pMat_;

	path /= "core_assets/materials/";
	path /= pMat->getName();
	path.setExtension(MTL_B_FILE_EXTENSION);

	core::zero_object(hdr);

	shader::XShaderItem item = pMat->getShaderItem();
	shader::IRenderShaderResources* pRes = item.pResources_;

	hdr.fourCC = MTL_B_FOURCC;
	hdr.version = MTL_B_VERSION;
	hdr.diffuse = pRes->getDiffuseColor();
	hdr.specular = pRes->getSpecularColor();
	hdr.emissive = pRes->getEmissiveColor();
	hdr.opacity = pRes->getOpacity();
	hdr.shineness = pRes->getSpecularShininess();

	XTextureResource* pTex = nullptr;
	MaterialTexture textures[shader::ShaderTextureIdx::ENUM_COUNT];

	core::zero_object(textures);

	for (i = 0, numTex = 0; i < ShaderTextureIdx::ENUM_COUNT; i++)
	{
		pTex = pRes->getTexture((ShaderTextureIdx::Enum)i);

		if (pTex)
		{
			textures[numTex].name.set(pTex->name);
			textures[numTex].type = (ShaderTextureIdx::Enum)i;
			numTex++;
		}
	}

	hdr.numTextures = safe_static_cast<uint8_t, uint32_t>(numTex);

	X_ASSERT(hdr.isValid(), "invalid header")();

	// open the file.
	core::XFileScoped file;

	if (file.openFile(path.c_str(), core::fileMode::WRITE | core::fileMode::RECREATE))
	{
		file.writeObj(hdr);
		file.writeObjs(textures, numTex);
		return true;
	}

	return false;
}



void XMaterialManager::ListMaterials(const char* searchPatten) const
{
	core::Array<IMaterial*> sorted_mats(g_3dEngineArena);
	sorted_mats.setGranularity(materials_.size());

	MaterialCon::ResourceConstItor Matit;

	for (Matit = materials_.begin(); Matit != materials_.end(); ++Matit)
	{
		IMaterial* mat = static_cast<XMaterial*>(Matit->second);

		if (!searchPatten || core::strUtil::WildCompare(searchPatten, mat->getName()))
		{
			sorted_mats.push_back(mat);
		}
	}

	sortMatsByName(sorted_mats);

	X_LOG0("Console", "------------ ^8Materials(%i)^7 ------------", sorted_mats.size());
	X_LOG_BULLET;

	core::Array<IMaterial*>::ConstIterator it = sorted_mats.begin();
	for (; it != sorted_mats.end(); ++it)
	{
		const IMaterial* mat = *it;
		X_LOG0("Material", "^2\"%s\"^7", 
			mat->getName());
	}

	X_LOG0("Console", "------------ ^8Materials End^7 ------------");
}


X_NAMESPACE_END
