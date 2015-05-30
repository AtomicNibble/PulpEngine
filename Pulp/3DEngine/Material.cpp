#include "stdafx.h"
#include "Material.h"


#include <String\Xml.h>


X_NAMESPACE_BEGIN(engine)

using namespace core::xml::rapidxml;


namespace
{

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
		uint32_t len = core::strUtil::strlen(pStr);
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
			return MaterialType::UI
		}
		if (core::strUtil::IsEqual(pBegin, pEnd, "model")) {
			return MaterialType::MODEL;
		}

		X_ERROR("Mtl", "Unkown material type: '%s' (case-sen)", str);
		return MaterialType::UNKNOWN;
	}

} // namespace


bool MaterialHeader::isValid(void) const
{
	if (version != MTL_B_VERSION) {
		X_ERROR("Mtl", "material version is invalid. FileVer: %i RequiredVer: %i",
			version, MTL_B_VERSION);
	}

	return version == MTL_B_VERSION &&
		fourCC == MTL_B_FOURCC;
}



XMaterial::XMaterial() 
{
	MatSurfaceType_ = MaterialSurType::NONE;
	CullType_ = MaterialCullType::BACK_SIDED;

	texRepeat_ = MaterialTexRepeat::TILE_BOTH;
	polyOffsetType_ = MaterialPolygonOffset::NONE;
	filterType_ = MaterialFilterType::LINEAR;
	MatType_ = MaterialType::UNKNOWN;
}

XMaterial::~XMaterial()
{
	ShutDown();
}

void XMaterial::ShutDown(void)
{
	core::SafeRelease(shaderItem_.pShader_);
	core::SafeRelease(shaderItem_.pResources_);

	// free my nipples!
	((XMaterialManager*)getMaterialManager())->unregister(this);

}

// XBaseAsset
const int XMaterial::release()
{
	const int ref = XBaseAsset::release();
	if (ref == 0)
		X_DELETE(this, g_3dEngineArena);
	return ref;
}
// ~XBaseAsset


void XMaterial::setName(const char* pName)
{
	MatName_ = pName;
}



const MaterialFlags XMaterial::getFlags() const
{
	return flags_;
}

void XMaterial::setFlags(const MaterialFlags flags)
{
	flags_ = flags;
}



MaterialSurType::Enum XMaterial::getSurfaceType() const
{
	return MatSurfaceType_;
}

void XMaterial::setSurfaceType(MaterialSurType::Enum type)
{
	MatSurfaceType_ = type;
}

MaterialCullType::Enum XMaterial::getCullType() const
{
	return CullType_;
}

void XMaterial::setCullType(MaterialCullType::Enum type)
{
	CullType_ = type;
}


MaterialTexRepeat::Enum XMaterial::getTexRepeat(void) const
{
	return texRepeat_;
}

void XMaterial::setTexRepeat(MaterialTexRepeat::Enum texRepeat)
{
	texRepeat_ = texRepeat;
}

MaterialPolygonOffset::Enum XMaterial::getPolyOffsetType(void) const
{
	return polyOffsetType_;
}

void XMaterial::setPolyOffsetType(MaterialPolygonOffset::Enum polyOffsetType)
{
	polyOffsetType_ = polyOffsetType;
}

MaterialFilterType::Enum XMaterial::getFilterType(void) const
{
	return filterType_;
}

void XMaterial::setFilterType(MaterialFilterType::Enum filterType)
{
	filterType_ = filterType;
}

MaterialType::Enum XMaterial::getType(void) const
{
	return MatType_;
}

void XMaterial::setType(MaterialType::Enum type)
{
	MatType_ = type;
}


void XMaterial::setShaderItem(shader::XShaderItem& item)
{
	core::SafeRelease(shaderItem_.pShader_);
	core::SafeRelease(shaderItem_.pResources_);

	shaderItem_ = item;
}


bool XMaterial::isDefault() const
{
	return getMaterialManager()->getDefaultMaterial() == this;
}


bool XMaterial::ProcessMaterialXML(XMaterial* pMaterial, 
	core::xml::rapidxml::xml_node<char>* node)
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

			uint32_t flags = core::strUtil::StringToInt<uint32_t>(attr->value());
			pMaterial->setFlags(flags);
		}
		else if (core::strUtil::IsEqual(begin, end, "SurfaceType"))
		{
			flags.Set(MtlXmlFlags::SURFACETYPE);
			// this needs fixing since we get strings not numbers here.
			MaterialSurType::Enum type = core::strUtil::StringToInt<MaterialSurType::Enum>(attr->value());
			pMaterial->setSurfaceType(type);
		}
		else if (core::strUtil::IsEqual(begin, end, "MaterialType"))
		{
			// make sure the name is a valid material type.
			MaterialType::UNKNOWN;

		}
		else
		{
			const char* pName = pMaterial->getName();
			X_WARNING("Mtl", "%s -> unkown attribute: '%s' on material node.", pName, begin);
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

			if (attr = texture->first_attribute("type", 4))
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

			if (attr = texture->first_attribute("image_name", 10))
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
			// input.textures[texId].name = name.c_str();

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
				else {
				//	input.material.diffuse = Color(vec);
				}
			}
			else if (core::strUtil::IsEqual(begin, end, "specular"))
			{
				if (!strToVec4(value, vec))
					X_ERROR("Mtl", "material param 'specular' has a invalid value");
				else {
				//	input.material.specular = Color(vec);
				}
			}
			else if (core::strUtil::IsEqual(begin, end, "emissive"))
			{
				if (!strToVec4(value, vec))
					X_ERROR("Mtl", "material param 'emissive' has a invalid value");
				else {
				//	input.material.emissive = Color(vec);
				}
			}
			else if (core::strUtil::IsEqual(begin, end, "shineness"))
			{
				if (!core::strUtil::IsNumeric(value))
					X_ERROR("Mtl", "material param 'shineness' is not a valid float");
				else {
				//	input.material.specShininess = core::strUtil::StringToFloat<float>(value);
				}
			}
			else if (core::strUtil::IsEqual(begin, end, "opacity"))
			{
				if (!core::strUtil::IsNumeric(value))
					X_ERROR("Mtl", "material param 'opacity' is not a valid float");
				else {
				//	input.opacity = core::strUtil::StringToFloat<float>(value);
				}
			}

		}
	}


	return true;
}


X_NAMESPACE_END
