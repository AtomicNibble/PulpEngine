#pragma once

#include <IShader.h>

#include <Containers\Array.h>

#include <String\StringHash.h>
#include <Hashing\xxHash.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
	class XCBuffer;

	class CBufferLink
	{
	public:
		CBufferLink(ShaderStage::Enum stage, const XCBuffer* pCBufer);

		ShaderStageFlags stages;
		const XCBuffer* pCBufer;
	};

	class XShaderParam
	{
		typedef core::Hash::xxHash64 Hasher;

	public:
		XShaderParam();
		XShaderParam(const XShaderParam& sb) = default;
		XShaderParam(XShaderParam&& sb);
		XShaderParam& operator = (const XShaderParam& sb) = default;

	public:
		void print(void) const;

		bool isEqual(const XShaderParam& oth) const;
		void addToHash(Hasher& hasher) const;

		X_INLINE void setName(const core::string& name);
		X_INLINE void setName(const char* pName);
		X_INLINE void setUpdateRate(UpdateFreq::Enum updateRate);
		X_INLINE void setType(ParamType::Enum type);
		X_INLINE void setFlags(ParamFlags flags);
		X_INLINE void setBindPoint(int32_t bindPoint);
		X_INLINE void setSize(int32_t size);

		X_INLINE const core::string& getName(void) const;
		X_INLINE const core::StrHash& getNameHash(void) const;
		X_INLINE UpdateFreq::Enum getUpdateRate(void) const;
		X_INLINE ParamType::Enum getType(void) const;
		X_INLINE ParamFlags getFlags(void) const;
		X_INLINE int16_t getBindPoint(void) const;
		X_INLINE int16_t getBindOffset(void) const;
		X_INLINE int16_t getNumVecs(void) const;


		bool SSave(core::XFile* pFile) const;
		bool SLoad(core::XFile* pFile);

	private:
		// 8 / 4
		core::string		name_;
		// 4
		core::StrHash		nameHash_;
		// 4
		ParamFlags			flags_;
		UpdateFreq::Enum	updateRate_;
		uint8_t				_pad[2];

		// 4
		ParamType::Enum		type_;

		// 8
		int16_t				bindPoint_;
		int16_t				numParameters_;
	};


	class XCBuffer 
	{
		typedef core::Array<XShaderParam> ParamArr;
		typedef core::Array<uint8_t, core::ArrayAlignedAllocator<uint8_t>> DataArr;

		typedef core::Hash::xxHash64 Hasher;

	public:
		XCBuffer(core::MemoryArenaBase* arena);
		XCBuffer(const XCBuffer& sb) = default;
		XCBuffer(XCBuffer&& sb) = default;

		XCBuffer& operator = (const XCBuffer& sb) = default;
		XCBuffer& operator = (XCBuffer&& sb) = default;

	public:

		void print(void) const;
		bool isEqual(const XCBuffer& oth) const;

		void addParam(const XShaderParam& param);
		void addParam(XShaderParam&& param);

		X_INLINE void setName(const core::string& name);
		X_INLINE void setName(const char* pName);
		X_INLINE void setUpdateRate(UpdateFreq::Enum updateRate);
		X_INLINE void setSize(int16_t size);
		X_INLINE void setBindPointAndCount(int16_t bindPoint, int16_t bindCount);
		X_INLINE void setParamGranularitys(size_t varGran);
		X_INLINE void setCpuDataVersion(int32_t version);

		X_INLINE const core::string& getName(void) const;
		X_INLINE bool requireManualUpdate(void) const;
		X_INLINE bool containsOnlyFreq(UpdateFreq::Enum freq) const; // must contains some of 'freq' && not contain others.
		X_INLINE bool containsUpdateFreqs(UpdateFreqFlags freqFlags) const; // contains any of the provided 'freqFlags'
		X_INLINE bool containsKnownParams(void) const; // contains some known params.
		X_INLINE UpdateFreq::Enum getUpdateFreg(void) const;
		X_INLINE int16_t getBindSize(void) const; // this is size in bytes cb should be.
		X_INLINE int16_t getBindPoint(void) const;
		X_INLINE int16_t getBindCount(void) const;
		X_INLINE int32_t getParamCount(void) const;
		X_INLINE Hasher::HashVal getHash(void) const;
		X_INLINE ParamTypeFlags getParamFlags(void) const;
		X_INLINE int32_t getCpuDataVersion(void) const;

		// int32_t index's to match getParamCount() return.
		X_INLINE const XShaderParam& operator[](int32_t idx) const;
		X_INLINE XShaderParam& operator[](int32_t idx);

		X_INLINE const DataArr& getCpuData(void) const;
		X_INLINE DataArr& getCpuData(void);

		bool SSave(core::XFile* pFile) const;
		bool SLoad(core::XFile* pFile);

		// create hash / flags.
		void postParamModify(void);
		void postPopulate(void);
		void recalculateUpdateRate(void);

	private:
		void computeFlags(void);
		void computeHash(void);

	private:
		// 8
		core::string name_;

		Hasher::HashVal hash_;

		// 4
		UpdateFreq::Enum updateRate_;
		UpdateFreqFlags updateFeqFlags_; // tells me what type fo flags this cbuffer is made up of.
		int16_t size_;

		// 4
		int16_t bindPoint_;
		int16_t bindCount_;

		ParamTypeFlags paramFlags_; // the flags that are set ParamType::unknow is set if 1 or more are unknow.
		ParamArr params_;

		// the version of the data in cpu.
		int32_t cpuDataVersion_;
		DataArr cpuData_;
	};

	// X_ENSURE_SIZE(XCBuffer, 128);

	// these are used as flags and enum so 32bit meaning we can have 32max. instead of 255.
	static_assert(ParamType::FLAGS_COUNT, "ParamType must be flags not enum");
	X_ENSURE_SIZE(ParamType::Enum, 4);

} // namespace shader

X_NAMESPACE_END

#include "CBuffer.inl"