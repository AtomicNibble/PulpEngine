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

#include "CBuffer.h"

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


class XShaderTechniqueHW : public IShaderPermatation
{

public:
	XShaderTechniqueHW(core::MemoryArenaBase* arena);

	bool canDraw(void) const;
	bool tryCompile(bool forceSync = false) X_FINAL;

	const CBufLinksArr& getCbufferLinks(void) const X_FINAL;

private:
	void addCbufs(XHWShader* pShader);

public:
	TechFlags techFlags;
	ILFlags ILFlags;
	InputLayoutFormat::Enum IlFmt;

	// cbuffers.
	CBufLinksArr cbLinks;

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

	bool tryCompile(bool forceSync = false) X_FINAL;

	IShaderPermatation* getPermatation(VertexFormat::Enum vertexFmt) X_FINAL;


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
	IShaderTech* getTech(const char* pName) X_OVERRIDE;

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