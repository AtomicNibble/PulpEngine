#pragma once


#ifndef X_SHADER_H_
#define X_SHADER_H_

#include <IShader.h>
#include <IDirectoryWatcher.h>

#include <String\StrRef.h>
#include <String\Lexer.h>
#include <String\StringHash.h>

#include <Containers\HashMap.h>
#include <Containers\Array.h>
#include <Util\Flags.h>

#include <../Common/Resources/BaseRenderAsset.h>

#include <unordered_set>

X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(render)

namespace shader
{

class SourceFile;
class ShaderSourceFile;
class ShaderSourceFileTechnique;
class XShaderTechniqueHW;
class XHWShader;
class XShaderManager;


#if 0
// #define SHADER_BIND_SAMPLER 0x4000


class XShaderResources : public IRenderShaderResources
{
public:
	XShaderResources();

	// IRenderShaderResources
	virtual ~XShaderResources() X_OVERRIDE;

	virtual void release(void) X_OVERRIDE;

	virtual Color& getDiffuseColor() X_OVERRIDE{ return diffuse; }
	virtual Color& getSpecularColor() X_OVERRIDE{ return spec; }
	virtual Color& getEmissiveColor() X_OVERRIDE{ return emissive; }

	virtual float& getSpecularShininess() X_OVERRIDE{ return specShine; }
	virtual float& getGlow() X_OVERRIDE{ return glow; }
	virtual float& getOpacity() X_OVERRIDE{ return opacity; }

	virtual XTextureResource* getTexture(ShaderTextureIdx::Enum idx) const X_OVERRIDE{
		return pTextures[idx];
	}
	// ~IRenderShaderResources

	X_INLINE bool hasTexture(ShaderTextureIdx::Enum idx) const {
		return pTextures[idx] != nullptr;
	}

	// called from render thread.
	void RT_Release(void);
private:

	void freeAssets(void);

protected:
	X_NO_COPY(XShaderResources);
	X_NO_ASSIGN(XShaderResources);

	friend class XShaderManager;

	// the textures
	shader::XTextureResource* pTextures[ShaderTextureIdx::ENUM_COUNT]; // 8 x 3 = 24

	// 12 * 3 = 36
	Color diffuse;
	Color spec;
	Color emissive;

	// 12
	float specShine;
	float glow;
	float opacity;
};


#endif


class XShaderTechniqueHW
{
public:
	XShaderTechniqueHW() {
		static_assert(InputLayoutFormat::Invalid == 0, "Memset won't make enum invalid");
		core::zero_this(this);
	}

	void release(void);
	bool canDraw(void) const;
	bool tryCompile(bool forceSync = false);

public:
	TechFlags techFlags;
	ILFlags ILFlags;
	InputLayoutFormat::Enum IlFmt;

	XHWShader* pVertexShader;
	XHWShader* pPixelShader;
	XHWShader* pGeoShader;
	XHWShader* pHullShader;
	XHWShader* pDomainShader;
};

class XShaderTechnique : public IShaderTech
{
	typedef core::Array<XShaderTechniqueHW> HWTechArr;

public:
	XShaderTechnique(core::MemoryArenaBase* arena);

	void assignSourceTech(const ShaderSourceFileTechnique& srcTech);
	void append(const XShaderTechniqueHW& hwTech);



public:
	core::string name;
	core::StrHash nameHash;
	TechFlags techFlags;

	HWTechArr hwTechs;
};



X_ALIGNED_SYMBOL(class XShader, 16) : public IShader
{
	friend class XShaderManager;

	typedef core::Array<XShaderTechnique> TechArr;

public:
	XShader(core::MemoryArenaBase* arena);
	~XShader();

	X_INLINE const core::string& getName(void) const X_OVERRIDE;
	const IShaderTech* getTech(const char* pName) const X_OVERRIDE;

	X_INLINE size_t numTechs(void) const;

	X_INLINE const int32_t getID(void) const;
	X_INLINE void setID(int32_t id);


private:
	core::string name_;
	int32_t id_;
	uint32_t sourceCrc32_;
	uint32_t hlslSourceCrc32_;
	SourceFile* pHlslFile_;
	TechArr techs_;
};


} // namespace shader

X_NAMESPACE_END


#include "Shader.inl"


#endif // !X_SHADER_H_