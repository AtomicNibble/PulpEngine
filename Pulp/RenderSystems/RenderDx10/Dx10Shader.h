#pragma once

#ifndef X_DX10_SHADER_H_
#define X_DX10_SHADER_H_

#include "../Common/Shader/XShader.h"

#include "Dx10Render.h"
#include "XShaderBin.h"

X_NAMESPACE_DECLARE(core,
	namespace V2 {
		struct Job;
		class JobSystem;
	}
)

X_NAMESPACE_BEGIN(shader)

// No float flag, it's float by default.
X_DECLARE_FLAGS(ParamFlags)(FLOAT, INT, BOOL, MATRIX, VEC2, VEC3, VEC4);

/*
struct ShaderStatus
{
	enum Enum
	{
		NotCompiled,
		Compiling,		// async compile in progress.
		UploadedToHW, // reflection is required now, this is when uploaded to gpu
		ReadyToRock,	// this is set after successful reflection.
		FailedToCompile
	};
};
*/

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
	core::string		name;
	core::StrHash		nameHash;
	Flags<ParamFlags>	flags;
	ParamType::Enum		type;
	short				bind;
	short				constBufferSlot;
	int					numParameters;

	XShaderParam()
	{
		type = ParamType::Unknown;
		bind = -2;
		constBufferSlot = 0;
		numParameters = 1;
	}
	XShaderParam(const XShaderParam& sb) = default;
	XShaderParam& operator = (const XShaderParam& sb) = default;
};

X_ENSURE_SIZE(ParamType::Enum, 1);

// input layout tree nodes.
struct ILTreeNode
{
	static const size_t MAX_IL_NODE_CHILDREN = 4;
	typedef core::Array<ILTreeNode> childVec;

	ILTreeNode() : children_(g_rendererArena) {
		this->fmt_ = InputLayoutFormat::Invalid;
	}
	ILTreeNode(const ILTreeNode& oth) : children_(g_rendererArena) {
		this->children_ = oth.children_;
		this->SematicName_ = oth.SematicName_;
		this->fmt_ = oth.fmt_;
	}
	ILTreeNode(const char* Sematic) : children_(g_rendererArena) {
		this->SematicName_.set(Sematic);
		this->fmt_ = InputLayoutFormat::Invalid;
	}

	~ILTreeNode() {
		free();
	}

	void free(void) {
		children_.free();
	}

	ILTreeNode& AddChild(ILTreeNode& node, 
		InputLayoutFormat::Enum fmt = InputLayoutFormat::Invalid) 
	{
		if (children_.size() < MAX_IL_NODE_CHILDREN)
		{
			children_.append(node);
			ILTreeNode& retnode = children_[children_.size() - 1];
			retnode.fmt_ = fmt;
			return retnode;
		}
		X_ERROR("Shader", "ILTree exceeded max children. max: %i", MAX_IL_NODE_CHILDREN);
		static ILTreeNode s_node;
		return s_node;
	}

	const ILTreeNode* GetChildWithSemtaic(const char* sematic) const {
		// we want to skip SEMTA0 SEMTA1 SEMTA2 etc so we only compare the length.
		childVec::ConstIterator it;
		for (it = children_.begin(); it != children_.end(); ++it)
		{
			if (core::strUtil::IsEqualCaseInsen(it->SematicName_.begin(),
				it->SematicName_.end(), sematic, sematic + it->SematicName_.length()))
			{
				return it;
			}
		}
		return nullptr;
	}

	X_INLINE void SetFormat(InputLayoutFormat::Enum fmt) {
		this->fmt_ = fmt;
	}

	X_INLINE bool IsEnd(void) const {
		return this->fmt_ == InputLayoutFormat::Invalid;
	}

	X_INLINE InputLayoutFormat::Enum GetILFmt(void) const {
		return fmt_;
	}

private:
	core::StackString<64> SematicName_;
	InputLayoutFormat::Enum fmt_;
	childVec children_; // never gonna be that many children.
	bool ___pad[3];
};


class XHWShader_Dx10 : public XHWShader
{
	friend class XHWShader;
	friend class XShaderBin;
public:
	XHWShader_Dx10();

	bool activate(void);
	const int release(void) X_OVERRIDE;

	// release just the HW device.
	const int releaseHW(void);

	// binds the shader to gpu ()
	X_INLINE bool bind(void);
	X_INLINE ShaderStatus::Enum getStatus(void) const;
	X_INLINE bool isValid(void) const;
	X_INLINE bool FailedtoCompile(void) const;
	X_INLINE bool isCompiling(void) const;

	static void Init(void);
	static void shutDown(void);

	static void comitParamsGlobal(void);
	static void comitParams(void);
	static void setParamsGlobal(void);
	static void setParams(void);

	// o baby your so dirty.
	static void SetCameraDirty(void) {}
	static void SetGlobalDirty(void) {}

	static void addGlobalParams(core::Array<XShaderParam>& BindVars, ShaderType::Enum type);

	static void bindGS(XHWShader_Dx10* pShader);

	static class XHWShader_Dx10* pCurVS_;
	static class XHWShader_Dx10* pCurPS_;
	static class XHWShader_Dx10* pCurGS_;

private:
	bool loadFromSource(void);
	bool loadFromCache(void);
	void getShaderCompilePaths(core::Path<char>& src, core::Path<char>& dest);
	void getShaderCompileSrc(core::Path<char>& src);
	void getShaderCompileDest(core::Path<char>& dest);
	bool compileFromSource(core::string& source);

	void CompileShader_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

	bool uploadtoHW(void);

	bool reflectShader(void);
	bool saveToCache(void);

private:
	bool bindVS(void);
	bool bindPS(void);
	bool bindGS(void);

	static void commitConstBuffer(ConstbufType::Enum buf_type, ShaderType::Enum shader_type);
	
	static void setParamValues(XShaderParam* pPrams, uint32_t numPrams,
		ShaderType::Enum shaderType, uint32_t maxVecs);

	X_INLINE void setShader(void);
	X_INLINE static void setConstBuffer(ShaderType::Enum type, uint slot, ID3D11Buffer* pBuf);
	static _inline void unMapConstbuffer(ShaderType::Enum shaderType,
		ConstbufType::Enum bufType);

	X_INLINE static bool mapConstBuffer(ShaderType::Enum shaderType,
		ConstbufType::Enum bufType, int maxVectors);

	X_INLINE static bool createConstBuffer(ShaderType::Enum shaderType,
		ConstbufType::Enum bufType, int maxVectors);

	X_INLINE static void setConstBuffer(int nReg, int nCBufSlot, ShaderType::Enum shaderType,
		const Vec4f* pData, const int nVecs, int nMaxVecs);

public:

	X_INLINE static void setParameterRegA(int nReg, int nCBufSlot, ShaderType::Enum shaderType,
		const Vec4f* pData, int nComps, int nMaxVecs);

	X_INLINE static void setParameteri(XShaderParam* pParam, const Vec4f* pData,
		ShaderType::Enum shaderType, uint32_t maxVecs);

	X_INLINE static void setParameterf(XShaderParam* pParam, const Vec4f* pData,
		ShaderType::Enum shaderType, uint32_t maxVecs);


public:
	XShaderParam* getParameter(const core::StrHash& NameParam);

	X_INLINE int getMaxVecs(XShaderParam* pParam) const;
	X_INLINE ID3DBlob* getshaderBlob(void) const;

private:
//	ShaderStatus::Enum status_;
	ID3DBlob* pBlob_;
	void* pHWHandle_;

	core::Array<XShaderParam> bindVars_;
	int maxVecs_[3];

	// the flags it was compiled with: DEBUG | OPT_LEVEL1 etc.
	uint32_t D3DCompileflags_;

	// valid during compile.
	core::string source_;

protected:
	X_INLINE void setMaxVecs(int maxVecs[3]);

	X_INLINE const core::Array<XShaderParam>& getBindVars(void) const;
	X_INLINE void setBindVars(core::Array<XShaderParam>& vars);

	X_INLINE uint32_t getD3DCompileFlags(void) const;

	X_INLINE void setBlob(ID3DBlob* pBlob);

	X_INLINE void setStatus(ShaderStatus::Enum status);

private:
	static void InitBufferPointers(void);
	static void FreeBufferPointers(void);
	static void FreeHWShaders(void);
	static void FreeParams(void);

	static void CreateInputLayoutTree(void);
	static void FreeInputLayoutTree(void);
	static void RegisterDvars(void);

	static ILTreeNode ILTree_;
	static XShaderBin bin_;

	// we have a set of const buffers for each shader type.
	static ID3D11Buffer** pConstBuffers_[ShaderType::ENUM_COUNT][ConstbufType::Num];
	// points to one of the buffers above ^ for the currently mapped buffer
	static ID3D11Buffer*  pCurRequestCB_[ShaderType::ENUM_COUNT][ConstbufType::Num];
	// pointers to mapped memory for the CurrentRequest.
	static Vec4f* pConstBuffData_[ShaderType::ENUM_COUNT][ConstbufType::Num];
	// current max vectors a given buffer can hold.
	// used to check if resize needed :D !
	static int curMaxVecs_[ShaderType::ENUM_COUNT][ConstbufType::Num];
	static int perFrameMaxVecs_[ShaderType::ENUM_COUNT];
	static int perInstantMaxVecs_[ShaderType::ENUM_COUNT];


	// list of params set per frame.
	static core::Array<XShaderParam> perFrameParams_[ShaderType::ENUM_COUNT];
	static core::Array<XShaderParam> perInstantParams_[ShaderType::ENUM_COUNT];

	// vars
	static int writeMergedSource_;
	static int asyncShaderCompile_;
};


X_NAMESPACE_END;

#include "Dx10Shader.inl"

#endif // X_DX10_SHADER_H_