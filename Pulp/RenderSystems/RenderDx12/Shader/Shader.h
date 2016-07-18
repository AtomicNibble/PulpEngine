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
#include "ShaderBin.h"

#include <unordered_set>

X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(render)

namespace shader
{

class XShaderManager;
struct ShaderSourceFile;
struct SourceFile;

#define SHADER_BIND_SAMPLER 0x4000


struct ConstbufType
{
	enum Enum
	{
		PER_FRAME,
		PER_BATCH,
		PER_INSTANCE,
		Num
	};
};



X_DECLARE_FLAGS(ShaderStatus) (NotCompiled, Compiling, AsyncCompileDone, UploadedToHW, ReadyToRock, FailedToCompile);


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


class XShader;
class XHWShader;
class XShaderTechniqueHW;






struct XShaderTechnique
{
	static const size_t TECH_MAX_SUB = 6;

	typedef core::FixedArray<XShaderTechniqueHW*, TECH_MAX_SUB> TechHWList;

	XShaderTechnique() :
		pCurHwTech(nullptr)
	{}

	XShaderTechnique& operator=(const ShaderSourceFile::Technique& srcTech);

	void release(void) {
		TechHWList::iterator it;
		for (it = hwTechs.begin(); it != hwTechs.end(); ++it) {
//			it->release();
		}
	}

	void append(const XShaderTechniqueHW& hwTech) {
//		hwTechs.append(&hwTech);
//		pCurHwTech = hwTechs.end() - 1;
	}

	void resetCurHWTech(void) {
//		pCurHwTech = hwTechs.begin();
	}

public:
	core::string name;
	core::StrHash nameHash;
	render::StateFlag state;
	render::CullMode::Enum cullMode;

	BlendInfo src;
	BlendInfo dst;

	XShaderTechniqueHW* pCurHwTech;
	TechHWList hwTechs;

	Flags<TechFlag> techFlags;
};

class XShader : public IShader, public core::XBaseAsset
{
	friend class XShaderManager;

public:
	XShader();
	~XShader();

	virtual ShaderID getID() X_OVERRIDE{ return XBaseAsset::getID(); }
	virtual const int addRef() X_OVERRIDE{ return XBaseAsset::addRef(); }
	virtual const int release() X_OVERRIDE;

	virtual const char* getName() const X_OVERRIDE{ return name_.c_str(); }
	virtual VertexFormat::Enum getVertexFmt() X_OVERRIDE{ return vertexFmt_; }

	// D3D Effects interface
	bool FXSetTechnique(const char* name, const TechFlags flag = TechFlags());
	bool FXSetTechnique(const core::StrHash& name, const TechFlags flag = TechFlags());
	bool FXBegin(uint32 *uiPassCount, uint32 nFlags);
	bool FXBeginPass(uint32 uiPass);
	bool FXCommit(const uint32 nFlags);
	bool FXEndPass();
	bool FXEnd();

	bool FXSetVSFloat(const core::StrHash& NameParam, const Vec4f* pVecs, uint32_t numVecs);

private:

	X_INLINE size_t numTechs(void) const { return techs_.size(); }

private:

	core::string name_;
	uint32_t sourceCrc32_;
	uint32_t hlslSourceCrc32_;

	SourceFile* pHlslFile_;

	VertexFormat::Enum vertexFmt_;

	core::Array<XShaderTechnique> techs_;
};


} // namespace shader

X_NAMESPACE_END


#endif // !X_SHADER_H_