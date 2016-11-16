#pragma once


#include <String\StringHash.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
	X_DECLARE_FLAGS8(ParamFlag)(FLOAT, INT, BOOL, SAMPLER, MATRIX, VEC2, VEC3, VEC4);
	X_DECLARE_ENUM8(ConstbufType)(PER_FRAME, PER_BATCH, PER_INSTANCE);

	typedef Flags8<ParamFlag> ParamFlags;

	// PF = PerFrame
	// PI = Per Instance
	// PB = Per Batch
	struct ParamType
	{
		enum Enum : uint8_t
		{
			Unknown,

			//	PF_ViewProjMatrix,

			PF_worldToScreenMatrix,
			PF_worldToCameraMatrix,
			PF_cameraToWorldMatrix,

			PI_objectToWorldMatrix,

			PI_worldMatrix,
			PI_viewMatrix,
			PI_projectionMatrix,
			PI_worldViewProjectionMatrix,

			PF_Time,
			PF_FrameTime,
			PF_ScreenSize,
			PF_CameraPos,

			PB_CameraFront,
			PB_CameraRight,
			PB_CameraUp,


			ParamCount,
		};
	};


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
		ConstbufType::Enum  slot;
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

		// 8
		core::string name;

		ConstbufType::Enum type;
		uint8_t _pad;
		int16_t size;

		int16_t bindPoint;
		int16_t bindCount;

		ParamArr params;
	};

	// X_ENSURE_SIZE(XShaderParam, 24);
	X_ENSURE_SIZE(ParamType::Enum, 1);

} // namespace shader

X_NAMESPACE_END