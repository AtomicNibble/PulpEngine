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

using namespace core::xml::rapidxml;
using namespace render::shader;

namespace {

	struct TextureType
	{
		const char* name;
		ShaderTextureIdx::Enum type;
	};

	TextureType g_textTypes[] = {
		{ "albedo", ShaderTextureIdx::DIFFUSE },
		{ "diffuse", ShaderTextureIdx::DIFFUSE },
		{ "bump", ShaderTextureIdx::BUMP },
		{ "spec", ShaderTextureIdx::SPEC },
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

	// string to material type.
	MaterialType::Enum MatTypeFromStr(const char* str)
	{
		// case sensitive for this one
		const char* pBegin = str;
		const char* pEnd = str + core::strUtil::strlen(str);

		if (core::strUtil::IsEqual(pBegin, pEnd, "world")) {
			return MaterialType::WORLD;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "ui")) {
			return MaterialType::UI;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "model")) {
			return MaterialType::MODEL;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "tool")) {
			return MaterialType::TOOL;
		}

		X_ERROR("Mtl", "Unknown material type: '%s' (case-sen)", str);
		return MaterialType::UNKNOWN;
	}

	MaterialSurType::Enum SurfaceTypeFromStr(const char* str)
	{
		// case sensitive for this one
		const char* pBegin = str;
		const char* pEnd = str + core::strUtil::strlen(str);

		if (core::strUtil::IsEqual(pBegin, pEnd, "none")) {
			return MaterialSurType::NONE;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "brick")) {
			return MaterialSurType::BRICK;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "concrete")) {
			return MaterialSurType::CONCRETE;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "cloth")) {
			return MaterialSurType::CLOTH;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "flesh")) {
			return MaterialSurType::FLESH;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "glass")) {
			return MaterialSurType::GLASS;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "grass")) {
			return MaterialSurType::GRASS;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "gravel")) {
			return MaterialSurType::GRAVEL;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "ice")) {
			return MaterialSurType::ICE;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "metal")) {
			return MaterialSurType::METAL;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "mud")) {
			return MaterialSurType::MUD;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "plastic")) {
			return MaterialSurType::PLASTIC;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "paper")) {
			return MaterialSurType::PAPER;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "rock")) {
			return MaterialSurType::ROCK;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "snow")) {
			return MaterialSurType::SNOW;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "sand")) {
			return MaterialSurType::SAND;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "wood")) {
			return MaterialSurType::WOOD;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "water")) {
			return MaterialSurType::WATER;
		}

		X_ERROR("Mtl", "Unknown material surface type: '%s' (case-sen)", str);
		return MaterialSurType::NONE;
	}

	MaterialCoverage::Enum CoverageFromStr(const char* str)
	{
		const char* pBegin = str;
		const char* pEnd = str + core::strUtil::strlen(str);

		if (core::strUtil::IsEqual(pBegin, pEnd, "opaque")) {
			return MaterialCoverage::OPAQUE;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "perforated")) {
			return MaterialCoverage::PERFORATED;
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "translucent")) {
			return MaterialCoverage::TRANSLUCENT;
		}

		X_ERROR("Mtl", "Unknown material coverage type: '%s' (case-sen)", str);
		return MaterialCoverage::BAD;
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
			else if (core::strUtil::IsEqual(begin, end, "Coverage"))
			{
				flags.Set(MtlXmlFlags::COVERAGE);

				MaterialCoverage::Enum coverage = CoverageFromStr(attr->value());
				pMaterial->setCoverage(coverage);
			}
			else if (core::strUtil::IsEqual(begin, end, "MaterialType"))
			{
				// make sure the name is a valid material type.
				MaterialType::Enum type = MatTypeFromStr(attr->value());
				pMaterial->setType(type);
			}
			else
			{
				X_WARNING("Mtl", "unknown attribute: %s on material node", begin);
			}
		}

		if (!flags.IsSet(MtlXmlFlags::NAME))
			X_ERROR("Mtl", "material node missing 'name' attribute");
		if (!flags.IsSet(MtlXmlFlags::FLAGS))
			X_ERROR("Mtl", "material node missing 'flags' attribute");
		if (!flags.IsSet(MtlXmlFlags::SURFACETYPE))
			X_ERROR("Mtl", "material node missing 'SurfaceType' attribute");

		// default to opaque
		if (!flags.IsSet(MtlXmlFlags::COVERAGE))
		{
			flags.Set(MtlXmlFlags::COVERAGE);
			pMaterial->setCoverage(MaterialCoverage::OPAQUE);
		}

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
				render::shader::ShaderTextureIdx::Enum texId;
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
		if (input.textures[render::shader::ShaderTextureIdx::DIFFUSE].name.isEmpty()) {
			X_WARNING("Mtl", "material has no albedo texture defined");
		}

		if (input.textures[render::shader::ShaderTextureIdx::BUMP].name.isEmpty()) {
			X_WARNING("Mtl", "material has no bump map assigning identity");
			input.textures[render::shader::ShaderTextureIdx::BUMP].name = texture::TEX_DEFAULT_BUMP;

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

#if 1
	//	X_ASSERT_NOT_IMPLEMENTED();
#else
		pMaterial->setShaderItem(item);
#endif

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

		(static_cast<XMaterialManager*>(XEngineBase::getMaterialManager()))->ListMaterials(searchPatten);
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
IMaterial* XMaterialManager::createMaterial(const char* MtlName)
{
	XMaterial *pMat = nullptr;

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

	return pMat;
}

IMaterial* XMaterialManager::findMaterial(const char* MtlName) const
{
	X_ASSERT_NOT_NULL(MtlName);

	XMaterial* pMaterial = static_cast<XMaterial*>(
		materials_.findAsset(X_CONST_STRING(MtlName)));

	if (pMaterial) {
		return pMaterial;
	}

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

namespace 
{
	void* XmlAllocate(std::size_t size)
	{
		return X_NEW_ARRAY(char, size, g_3dEngineArena, "xmlPool");
	}

	void XmlFree(void* pointer)
	{
		char* pChar = static_cast<char*>(pointer);
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

//	if (!pFileSys_->fileExists(path.c_str())) {
//		return pMat;
//	}

	if (file.openFile(path.c_str(), core::fileMode::READ))
	{
		length = safe_static_cast<size_t, uint64_t>(file.remainingBytes());


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
				pMat = static_cast<XMaterial*>(createMaterial(MtlName));
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

//	if (!pFileSys_->fileExists(path.c_str())) {
//		return pMat;
//	}

	if (file.openFile(path.c_str(), core::fileMode::READ))
	{
		file.readObj(hdr);

		if (hdr.isValid())
		{
			// read the file entries.
			MaterialTexture texture[render::shader::ShaderTextureIdx::ENUM_COUNT];

			uint32_t Num = core::Min<uint32_t>(hdr.numTextures, render::shader::ShaderTextureIdx::ENUM_COUNT);

			if (file.readObjs(texture, Num) == Num)
			{
				pMat = static_cast<XMaterial*>(createMaterial(MtlName));
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
					X_ASSERT(texture[i].type < static_cast<int32_t>(render::shader::ShaderTextureIdx::ENUM_COUNT),
						"invalid texture type")();
					input.textures[texture[i].type].name = texture[i].name.c_str();
				}

				// load the shader / textures needed.
				XShaderItem item = gEnv->pRender->LoadShaderItem(input);

#if 1
				X_ASSERT_NOT_IMPLEMENTED();
#else
				pMat->setShaderItem(item);
#endif

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

#if 1
	X_ASSERT_NOT_IMPLEMENTED();
#else
	core::Path<char> path;
	XMaterial* pMat;
	MaterialHeader hdr;
	uint32_t i, numTex;

	pMat = static_cast<XMaterial*>(pMat_);

	path /= "materials/";
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
	MaterialTexture textures[render::shader::ShaderTextureIdx::ENUM_COUNT];

	core::zero_object(textures);

	for (i = 0, numTex = 0; i < ShaderTextureIdx::ENUM_COUNT; i++)
	{
		pTex = pRes->getTexture(static_cast<ShaderTextureIdx::Enum>(i));

		if (pTex)
		{
			textures[numTex].name.set(pTex->name);
			textures[numTex].type = static_cast<ShaderTextureIdx::Enum>(i);
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

#endif
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

	X_LOG0("Console", "------------ ^8Materials(%" PRIuS ")^7 ------------", sorted_mats.size());

	core::Array<IMaterial*>::ConstIterator it = sorted_mats.begin();
	for (; it != sorted_mats.end(); ++it)
	{
		const IMaterial* mat = *it;
		X_LOG0("Material", "^2\"%s\"^7", 
			mat->getName());
	}

	X_LOG0("Console", "------------ ^8Materials End^7 -----------");
}


X_NAMESPACE_END
