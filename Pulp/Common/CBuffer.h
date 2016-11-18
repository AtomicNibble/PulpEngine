#pragma once

#include <IShader.h>

#include <String\StringHash.h>

#include <Containers\Array.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{


	struct XShaderParam
	{
		XShaderParam();
		XShaderParam(const XShaderParam& sb) = default;
		XShaderParam& operator = (const XShaderParam& sb) = default;

		void print(void) const;

		bool isEqual(const XShaderParam& oth) const;


		// 8 / 4
		core::string		name;
		// 4
		core::StrHash		nameHash;
		// 4
		ParamFlags			flags;
		ParamType::Enum		type;
		UpdateFreq::Enum	updateRate;
		uint8_t _pad[1];

		// 8
		int16_t				bind;
		int16_t				numParameters;
	};


	struct XCBuffer
	{
		typedef core::Array<XShaderParam> ParamArr;

	public:
		XCBuffer(core::MemoryArenaBase* arena);
		XCBuffer(const XCBuffer& sb) = default;
		XCBuffer& operator = (const XCBuffer& sb) = default;

		void print(void) const;

		bool isEqual(const XCBuffer& oth) const;
		bool fullyPreDefinedParams(void) const;

		int16_t getBindPoint(void) const;
		int16_t getBindCount(void) const;

	public:
		// 8
		core::string name;

		UpdateFreq::Enum updateRate;
		int16_t size;
		bool allParamsPreDefined;

		int16_t bindPoint;
		int16_t bindCount;

		ParamArr params;
	};

	// X_ENSURE_SIZE(XShaderParam, 24);
	X_ENSURE_SIZE(ParamType::Enum, 1);

} // namespace shader

X_NAMESPACE_END

#include "CBuffer.inl"