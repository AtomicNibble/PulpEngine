#pragma once


#ifndef X_SHADER_H_
#define X_SHADER_H_

#include <IShader.h>

#include <Containers\Array.h>
#include <Util\Flags.h>

#include "CBuffer.h"


X_NAMESPACE_BEGIN(render)

namespace shader
{

class XHWShader;

class ShaderPermatation : public IShaderPermatation
{
public:
	typedef std::array<XHWShader*, ShaderType::ENUM_COUNT - 1> HWShaderStagesArr;

public:
	SHADERLIB_EXPORT ShaderPermatation(const ShaderStagesArr& stages, core::MemoryArenaBase* arena);

	SHADERLIB_EXPORT void generateMeta(void);
	SHADERLIB_EXPORT bool isCompiled(void) const;

	X_INLINE bool isStageSet(ShaderType::Enum type) const;
	X_INLINE XHWShader* getStage(ShaderType::Enum type) const;

	X_INLINE InputLayoutFormat::Enum getILFmt(void) const X_FINAL;
	X_INLINE const CBufLinksArr& getCbufferLinks(void) const X_FINAL;
	SHADERLIB_EXPORT const BufferArr& getBuffers(void) const X_FINAL;
	SHADERLIB_EXPORT const SamplerArr& getSamplers(void) const X_FINAL;
	SHADERLIB_EXPORT const TexutreArr& getTextures(void) const X_FINAL;
	X_INLINE const HWShaderStagesArr& getStages(void) const;

private:
	void createCBLinks(void);
	void addCBufsToLink(XHWShader* pShader);


private:
	InputLayoutFormat::Enum IlFmt_;
	CBufLinksArr cbLinks_;
	HWShaderStagesArr stages_;
};



} // namespace shader

X_NAMESPACE_END


#include "ShaderPermatation.inl"


#endif // !X_SHADER_H_