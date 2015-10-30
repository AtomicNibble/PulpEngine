#pragma once

#ifndef X_DX10_SHADER_H_
#define X_DX10_SHADER_H_

#include "../Common/Shader/XShader.h"

#include "Dx10Render.h"
#include "XShaderBin.h"

X_NAMESPACE_BEGIN(shader)

// No float flag, it's float by default.
X_DECLARE_FLAGS(ParamFlags)(FLOAT, INT, BOOL, MATRIX, VEC2, VEC3, VEC4);

struct ShaderStatus
{
	enum Enum
	{
		NotCompiled,
		UploadedToHW, // reflection is required now, this is when uploaded to gpu
		ReadyToRock,	// this is set after successful reflection.
		FailedToCompile
	};
};

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
		bind = -2;
		constBufferSlot = 0;
		numParameters = 1;
	}
	XShaderParam(const XShaderParam& sb)
	{
		name = sb.name;
		nameHash = sb.nameHash;
		bind = sb.bind;
		constBufferSlot = sb.constBufferSlot;
		numParameters = sb.numParameters;
		flags = sb.flags;
		type = sb.type;
	}
	XShaderParam& operator = (const XShaderParam& sb)
	{
		this->~XShaderParam();
		// new(this) XShaderParam(sb);
		core::Mem::Construct(this, sb);
		return *this;
	}
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

	bool activate();
	const int release() X_OVERRIDE;

	// release just the HW device.
	const int releaseHW(void);

	// binds the shader to gpu ()
	X_INLINE bool bind()
	{
		if (this->type_ == ShaderType::Vertex)
			return bindVS();
		if (this->type_ == ShaderType::Pixel)
			return bindPS();
		if (this->type_ == ShaderType::Geometry)
			return bindGS();
		X_ASSERT_UNREACHABLE();
		return false;
	}


	X_INLINE ShaderStatus::Enum getStatus(void) const {
		return status_;
	}

	X_INLINE bool isValid(void) const {
		return status_ == ShaderStatus::ReadyToRock;
	}
	X_INLINE bool FailedtoCompile(void) const {
		return status_ == ShaderStatus::FailedToCompile;
	}

	static void Init(void);
	static void shutDown(void);

	static void comitParamsGlobal();
	static void comitParams();
	static void setParamsGlobal();
	static void setParams();

	// o baby your so dirty.
	static void SetCameraDirty(void) {}
	static void SetGlobalDirty(void) {}

	static void addGlobalParams(core::Array<XShaderParam>& BindVars, ShaderType::Enum type);

	static void bindGS(XHWShader_Dx10* pShader);

	static class XHWShader_Dx10* pCurVS_;
	static class XHWShader_Dx10* pCurPS_;
	static class XHWShader_Dx10* pCurGS_;

private:
	bool loadFromSource();
	bool loadFromCache();
	void getShaderCompilePaths(core::Path<char>& src, core::Path<char>& dest);
	void getShaderCompileSrc(core::Path<char>& src);
	void getShaderCompileDest(core::Path<char>& dest);
	bool compileFromSource(core::string& source);

	bool uploadtoHW(void);

	bool reflectShader(void);
	bool saveToCache(void);

private:
	bool bindVS();
	bool bindPS();
	bool bindGS();

	static void commitConstBuffer(ConstbufType::Enum buf_type, ShaderType::Enum shader_type);
	
	static void setParamValues(XShaderParam* pPrams, uint32_t numPrams,
		ShaderType::Enum shaderType, uint32_t maxVecs);

	X_INLINE void setShader()
	{
		ID3D11DeviceContext* pDevice = render::g_Dx11D3D.DxDeviceContext();
		if (isValid())
		{
			if (this->type_ == ShaderType::Vertex)
				pDevice->VSSetShader((ID3D11VertexShader*)pHWHandle_, NULL, 0);
			else if (this->type_ == ShaderType::Pixel)
				pDevice->PSSetShader((ID3D11PixelShader*)pHWHandle_, NULL, 0);
			else if(this->type_ == ShaderType::Geometry)
				pDevice->GSSetShader((ID3D11GeometryShader*)pHWHandle_, NULL, 0);
			else
			{
				// O'Deer
				X_ASSERT_UNREACHABLE();
			}
		}
	}

	X_INLINE static void setConstBuffer(ShaderType::Enum type, uint slot, ID3D11Buffer* pBuf)
	{
		ID3D11DeviceContext* pDevice = render::g_Dx11D3D.DxDeviceContext();

		switch (type)
		{
			case ShaderType::Vertex:
			pDevice->VSSetConstantBuffers(slot, 1, &pBuf);
			break;

			case ShaderType::Pixel:
			pDevice->PSSetConstantBuffers(slot, 1, &pBuf);
			break;

			case ShaderType::Geometry:
			pDevice->GSSetConstantBuffers(slot, 1, &pBuf);
			break;

#if X_DEBUG
			default:
				X_ASSERT_UNREACHABLE();
			break;
#else
			X_NO_SWITCH_DEFAULT;
#endif
		}
	}



	static _inline void unMapConstbuffer(ShaderType::Enum shaderType,
		ConstbufType::Enum bufType)
	{
		ID3D11DeviceContext* pDeviceContext;

		pDeviceContext = render::g_Dx11D3D.DxDeviceContext();

		// mapped?
		// if no values in the const buffer where set.
		// this will be null.
		// preventing a pointles update.
		if (!pConstBuffData_[shaderType][bufType])
			return;

		// unmapp it.
		pDeviceContext->Unmap(pCurRequestCB_[shaderType][bufType], 0);

		// clear data pointer, to flag remapping.
		pConstBuffData_[shaderType][bufType] = nullptr;

		// Update device!
		setConstBuffer(shaderType, 0, pCurRequestCB_[shaderType][bufType]);
	}


	X_INLINE static bool mapConstBuffer(ShaderType::Enum shaderType,
		ConstbufType::Enum bufType, int maxVectors)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		ID3D11DeviceContext* pDeviceContext;
		pDeviceContext = render::g_Dx11D3D.DxDeviceContext();

		if (pConstBuffData_[shaderType][bufType])
		{
			// mapping a buffer that's still mapped O.o?
			// send it to the gpu.
			unMapConstbuffer(shaderType, bufType);
		}

		if (!pConstBuffers_[shaderType][bufType])
		{
			X_ERROR("Shader", "Buffer pointers have not yet been init");
			return false;
		}

		// if buffer not created, make it :D !
		if (!createConstBuffer(shaderType, bufType, maxVectors)) 
		{
			X_ERROR("Shader", "failed to create device buffer");
			return false;
		}


		pCurRequestCB_[shaderType][bufType] = pConstBuffers_[shaderType][bufType][maxVectors];

		pDeviceContext->Map(
			pCurRequestCB_[shaderType][bufType],
			0, 
			D3D11_MAP_WRITE_DISCARD, 
			0, 
			&mappedResource
			);

		pConstBuffData_[shaderType][bufType] = (Vec4f*)mappedResource.pData;

		return true;
	}

	X_INLINE static bool createConstBuffer(ShaderType::Enum shaderType, 
			ConstbufType::Enum bufType, int maxVectors)
	{
		ID3D11Device* pDevice;
		D3D11_BUFFER_DESC bd;
		HRESULT hr = S_OK;

		if (maxVectors == 0)
			return false;

		// buffer already created?
		if (pConstBuffData_[shaderType][bufType])
			return true;

		core::zero_object(bd);
		pDevice = render::g_Dx11D3D.DxDevice();

		// set size.
		curMaxVecs_[shaderType][bufType] = maxVectors;

		if (!pConstBuffers_[shaderType][bufType][maxVectors] && maxVectors)
		{
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bd.MiscFlags = 0;
			bd.ByteWidth = maxVectors * sizeof(Vec4f);
			hr = pDevice->CreateBuffer(&bd, NULL, 
				&pConstBuffers_[shaderType][bufType][maxVectors]);
		}

		return SUCCEEDED(hr);
	}


	X_INLINE static void setConstBuffer(int nReg, int nCBufSlot, ShaderType::Enum shaderType,
		const Vec4f* pData, const int nVecs, int nMaxVecs)
	{

		if (!pConstBuffData_[shaderType][nCBufSlot])
			mapConstBuffer(shaderType, (ConstbufType::Enum)nCBufSlot, nMaxVecs);

		memcpy(&pConstBuffData_[shaderType][nCBufSlot][nReg], pData, nVecs << 4);
	}

public:

	X_INLINE static void setParameterRegA(int nReg, int nCBufSlot, ShaderType::Enum shaderType,
		const Vec4f* pData, int nComps, int nMaxVecs)
	{
		setConstBuffer(nReg, nCBufSlot, shaderType, pData, nComps, nMaxVecs);
	}

	X_INLINE static void setParameteri(XShaderParam* pParam, const Vec4f* pData,
		ShaderType::Enum shaderType, uint32_t maxVecs)
	{
		if (!pParam || pParam->bind < 0)
			return;

		int nReg = pParam->bind;
		setParameterRegA(nReg, pParam->constBufferSlot, shaderType, pData, pParam->numParameters, maxVecs);
	}

	X_INLINE static void setParameterf(XShaderParam* pParam, const Vec4f* pData,
		ShaderType::Enum shaderType, uint32_t maxVecs)
	{
		if (!pParam || pParam->bind < 0)
			return;

		int nReg = pParam->bind;
		setParameterRegA(nReg, pParam->constBufferSlot, shaderType, pData, pParam->numParameters, maxVecs);
	}


public:
	XShaderParam* getParameter(const core::StrHash& NameParam);

	int getMaxVecs(XShaderParam* pParam) const {
		return maxVecs_[pParam->constBufferSlot];
	}

	ID3DBlob* getshaderBlob(void) const {
		X_ASSERT_NOT_NULL(pBlob_);
		return pBlob_;
	}

private:
	ShaderStatus::Enum status_;
	ID3DBlob* pBlob_;
	void* pHWHandle_;

	core::Array<XShaderParam> bindVars_;
	int maxVecs_[3];

	// the flags it was compiled with: DEBUG | OPT_LEVEL1 etc.
	uint32_t D3DCompileflags_;

protected:
	void setMaxVecs(int maxVecs[3]) {
		memcpy(maxVecs_, maxVecs, sizeof(maxVecs_));
	}

	const core::Array<XShaderParam>& getBindVars(void) const {
		return bindVars_;
	}
	void setBindVars(core::Array<XShaderParam>& vars) {
		bindVars_ = vars;
	}

	uint32_t getD3DCompileFlags(void) const {
		return D3DCompileflags_;
	}

	void setBlob(ID3DBlob* pBlob) {
		pBlob_ = pBlob;
	}

	void setStatus(ShaderStatus::Enum status) {
		status_ = status;
	}

private:
	static void InitBufferPointers();
	static void FreeBufferPointers();
	static void FreeHWShaders();
	static void FreeParams();

	static void CreateInputLayoutTree(void);
	static void FreeInputLayoutTree(void);

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

};


X_NAMESPACE_END;

#endif // X_DX10_SHADER_H_