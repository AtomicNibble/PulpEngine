#pragma once


#include "ILTree.h"
#include <String\StringHash.h>

X_NAMESPACE_DECLARE(core,
	namespace V2 {
		struct Job;
		class JobSystem;
	}
)


X_NAMESPACE_BEGIN(render)

namespace shader
{

	X_DECLARE_FLAGS8(ParamFlag)(FLOAT, INT, BOOL, MATRIX, VEC2, VEC3, VEC4);
	X_DECLARE_FLAGS8(ShaderStatus) (NotCompiled, Compiling, AsyncCompileDone, ReadyToRock, FailedToCompile);
	X_DECLARE_ENUM8(ConstbufType)(PER_FRAME, PER_BATCH, PER_INSTANCE);

	typedef Flags8<ParamFlag> ParamFlags;

	class ShaderBin;
	class XShaderManager;
	class SourceFile;

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



	// Per frame
	struct XParamsPF
	{
	public:

		Matrix44f ViewProjMatrix;	//	PF_ViewProjMatrix

		Vec3f CameraFront;			//	PB_CameraFront
		Vec3f CameraRight;			//	PB_CameraRight
		Vec3f CameraUp;				//	PB_CameraUp
		Vec3f CameraPos;			//	PF_CameraPos

		Vec4f ScreenSize;			//	PF_ScreenSize
		Vec4f NearFarDist;			//	PF_NearFarDist

		Vec3f SunColor;				//	PF_SunColor 
		Vec3f SkyColor;				//	PF_SkyColor 
		Vec3f SunDirection;			//	PF_SunDirection
	};


	struct XShaderParam
	{
		XShaderParam();
		XShaderParam(const XShaderParam& sb) = default;
		XShaderParam& operator = (const XShaderParam& sb) = default;

		// 8 / 4
		core::string		name;
		// 4
		core::StrHash		nameHash;
		// 4
		ParamFlags			flags;
		ParamType::Enum		type;
		uint8_t _pad[2];

		// 8
		int16_t				bind;
		int16_t				constBufferSlot;
		int32_t				numParameters;
	};

	// X_ENSURE_SIZE(XShaderParam, 24);
	X_ENSURE_SIZE(ParamType::Enum, 1);



	X_ALIGNED_SYMBOL(class XHWShader, 64)
	{
		typedef core::Array<XShaderParam> ParamArr;
		typedef core::Array<uint8_t> ByteArr;

		friend class ShaderBin;

	public:
		XHWShader(core::MemoryArenaBase* arena, XShaderManager& shaderMan,
			ShaderType::Enum type, const char* pName, const core::string& entry,
			SourceFile* pSourceFile, TechFlags techFlags);

		const int32_t release(void) {
			return 0;
		}


		X_INLINE const core::string& getName(void) const;
		X_INLINE const core::string& getSourceFileName(void) const;
		X_INLINE const core::string& getEntryPoint(void) const;
		X_INLINE TechFlags getTechFlags(void) const;
		X_INLINE ShaderType::Enum getType(void) const;
		X_INLINE InputLayoutFormat::Enum getILFormat(void) const;
		X_INLINE uint32_t getNumRenderTargets(void) const;
		X_INLINE uint32_t getNumSamplers(void) const;
		X_INLINE uint32_t getNumConstantBuffers(void) const;
		X_INLINE uint32_t getNumInputParams(void) const;
		X_INLINE uint32_t getSourceCrc32(void) const;
		X_INLINE uint32_t getD3DCompileFlags(void) const;

		X_INLINE ShaderStatus::Enum getStatus(void) const;
		X_INLINE bool isValid(void) const;
		X_INLINE bool FailedtoCompile(void) const;
		X_INLINE bool isCompiling(void) const;

		X_INLINE const core::Array<XShaderParam> getBindVars(void) const;
		X_INLINE const ByteArr& getShaderByteCode(void) const;

	public:
		bool compile(bool forceSync = false);
		bool invalidateIfChanged(uint32_t newSourceCrc32);

		XShaderParam* getParameter(const core::StrHash& nameHash);

	private:
		void getShaderCompilePaths(core::Path<char>& src, core::Path<char>& dest);
		void getShaderCompileSrc(core::Path<char>& src);
		void getShaderCompileDest(core::Path<char>& dest);

		bool loadFromCache(ShaderBin& shaderBin);
		bool saveToCache(ShaderBin& shaderBin);

		bool loadSourceAndCompile(bool forceSync = false);
		bool compileFromSource(core::string& source);
		void CompileShader_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
		bool reflectShader(void);

	public:
		static const char* getProfileFromType(ShaderType::Enum type);
		static std::pair<uint8_t, uint8_t> getProfileVersionForType(ShaderType::Enum type);

	protected:
		XShaderManager& shaderMan_;

		core::string name_;
		core::string sourceFileName_;
		core::string entryPoint_;
		core::string source_;

		uint32_t sourceCrc32_; // the crc of the source this was compiled from.

		// status
		ShaderStatus::Enum status_;
		// color, textured, skinned, instanced
		TechFlags techFlags_;
		// Vert / Pixel / Hull / Geo
		ShaderType::Enum type_;
		// POS_UV_COL_NOR
		InputLayoutFormat::Enum IlFmt_;

		// save info from shader reflection.
		uint32_t numRenderTargets_;
		uint32_t numSamplers_;
		uint32_t numConstBuffers_;
		uint32_t numInputParams_;
		// the flags it was compiled with: DEBUG | OPT_LEVEL1 etc.
		uint32_t D3DCompileflags_;
		int32_t maxVecs_[3];

		ParamArr bindVars_;
		ByteArr bytecode_;
	};


} // namespace shader

X_NAMESPACE_END

#include "HWShader.inl"