#include <EngineCommon.h>
#include "CBuffer.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
	namespace
	{
		UpdateFreq::Enum updateFreqMax(UpdateFreq::Enum lhs, UpdateFreq::Enum rhs)
		{
			static_assert(UpdateFreq::BATCH > UpdateFreq::FRAME, "enum order not increasing");
			static_assert(UpdateFreq::MATERIAL > UpdateFreq::BATCH, "enum order not increasing");
			static_assert(UpdateFreq::INSTANCE> UpdateFreq::MATERIAL, "enum order not increasing");
			static_assert(UpdateFreq::SKINDATA > UpdateFreq::INSTANCE, "enum order not increasing");
			static_assert(UpdateFreq::UNKNOWN > UpdateFreq::SKINDATA, "enum order not increasing");

			// add more static asserts yo.
			static_assert(UpdateFreq::FLAGS_COUNT == 6, "Enum count changed this code need updating?");

			return core::Max(lhs, rhs);
		}


	} // namespace


	CBufferLink::CBufferLink(ShaderStage::Enum stage, const XCBuffer* pCBufer_) :
		stages(stage),
		pCBufer(pCBufer_)
	{
	}
	
	// -------------------------------------------------------------------

	XShaderParam::XShaderParam() :
		type_(ParamType::Unknown),
		bindPoint_(-1),
		numParameters_(0)
	{

	}

	XShaderParam::XShaderParam(XShaderParam&& sb)
	{
		// implementing move not really worth it lol
		// other than the name copy.
		name_ = sb.name_;
		nameHash_ = sb.nameHash_;

		flags_ = sb.flags_;
		updateRate_ = sb.updateRate_;
		type_ = sb.type_;
		bindPoint_ = sb.bindPoint_;
		numParameters_ = sb.numParameters_;
	}

	void XShaderParam::print(void) const
	{
		X_LOG0("Param", "Name: \"%s\"", name_.c_str());

	}

	bool XShaderParam::isEqual(const XShaderParam& oth) const
	{
		if (nameHash_ != oth.nameHash_) {
			return false;
		}

		if (name_ != oth.name_) {
			return false;
		}

		if (flags_.ToInt() != oth.flags_.ToInt() ||
			type_ != oth.type_ ||
			updateRate_ != oth.updateRate_ ||
			bindPoint_ != oth.bindPoint_ ||
			numParameters_ != oth.numParameters_) {
			return false;
		}

		return true;
	}

	void XShaderParam::addToHash(Hasher& hasher) const
	{
		hasher.update<uint32_t>(nameHash_);
		hasher.update(flags_.ToInt());
		hasher.update(type_);
		hasher.update(updateRate_);
		hasher.update(bindPoint_);
		hasher.update(numParameters_);
	}

	bool XShaderParam::SSave(core::XFile* pFile) const
	{
		pFile->writeString(name_);
		pFile->writeObj(nameHash_);
		pFile->writeObj(flags_);
		pFile->writeObj(updateRate_);
		pFile->writeObj(type_);
		pFile->writeObj(bindPoint_);
		pFile->writeObj(numParameters_);
		return true;
	}


	bool XShaderParam::SLoad(core::XFile* pFile)
	{
		pFile->readString(name_);
		pFile->readObj(nameHash_);
		pFile->readObj(flags_);
		pFile->readObj(updateRate_);
		pFile->readObj(type_);
		pFile->readObj(bindPoint_);
		pFile->readObj(numParameters_);
		return true;
	}


	// -------------------------------------------------------------------


	XCBuffer::XCBuffer(core::MemoryArenaBase* arena) :
		hash_(0),
		updateRate_(UpdateFreq::UNKNOWN),
		size_(0),
		bindPoint_(-1),
		bindCount_(-1),
		params_(arena),
		cpuDataVersion_(-1),
		cpuData_(arena)
	{
		// lets make it so all cbuf cpu data that we send to the render system 
		// is 16byte aligned.
		cpuData_.getAllocator().setBaseAlignment(16);
	}

	void XCBuffer::print(void) const
	{
		X_LOG0("CBuffer", "Name: \"%s\"", name_.c_str());
		for (const auto& p : params_)
		{
			p.print();
		}
	}

	bool XCBuffer::isEqual(const XCBuffer& oth) const
	{
		if (size_ != oth.size_) {
			return false;
		}
		if (name_ != oth.name_) {
			return false;
		}

		if (bindPoint_ != oth.bindPoint_) {
			X_WARNING("CBuffer", "buffer has same name but diffrent bind point");

			return false;
		}
		if (bindCount_ != oth.bindCount_) {
			X_WARNING("CBuffer", "buffer has same name but diffrent bind count");
			return false;
		}

		// now check all the params are the smae.
		if (params_.size() != oth.params_.size()) {
			X_WARNING("CBuffer", "buffer has same name but diffrent params");
			return false;
		}

		for (size_t i = 0; i < params_.size(); i++)
		{
			if (!params_[i].isEqual(oth.params_[i])) {
				// i put this hear as I want to see the scenarios this happens.
				// as i potentially need to handle the fact some params may be marked as unused in one stage and not in others.
				X_WARNING("CBuffer", "buffer has same name but diffrent params");
				return false;
			}
		}

		return true;
	}


	void XCBuffer::addParam(const XShaderParam& param)
	{
		updateRate_ = updateFreqMax(updateRate_, param.getUpdateRate());

		params_.emplace_back(param);
	}

	void XCBuffer::addParam(XShaderParam&& param)
	{
		updateRate_ = updateFreqMax(updateRate_, param.getUpdateRate());

		params_.emplace_back(std::forward<XShaderParam>(param));
	}

	bool XCBuffer::SSave(core::XFile* pFile) const
	{
		pFile->writeString(name_);
		pFile->writeObj(hash_);
		pFile->writeObj(updateFeqFlags_);
		pFile->writeObj(updateRate_);
		pFile->writeObj(size_);
		pFile->writeObj(bindPoint_);
		pFile->writeObj(bindCount_);
		pFile->writeObj(paramFlags_);
		pFile->writeObj(safe_static_cast<uint8_t>(params_.size()));

		for (const auto& p : params_)
		{
			if (!p.SSave(pFile)) {
				return false;
			}
		}
		return true;
	}

	bool XCBuffer::SLoad(core::XFile* pFile)
	{
		pFile->readString(name_);
		pFile->readObj(hash_);
		pFile->readObj(updateFeqFlags_);
		pFile->readObj(updateRate_);
		pFile->readObj(size_);
		pFile->readObj(bindPoint_);
		pFile->readObj(bindCount_);
		pFile->readObj(paramFlags_);

		uint8_t numParams;
		pFile->readObj(numParams);

		cpuData_.resize(size_);
		params_.resize(numParams);
		for (auto& p : params_)
		{
			if (!p.SLoad(pFile)) {
				return false;
			}
		}
		return true;
	}

	void XCBuffer::postParamModify(void)
	{
		postPopulate();
		recalculateUpdateRate();
	}

	void XCBuffer::postPopulate(void)
	{
		computeFlags();
		computeHash();

		// work out if we can shrink?
		uint32_t requiredVecs = 0;
		for (const auto& p : params_)
		{
			requiredVecs = core::Max<uint32_t>(requiredVecs, p.getBindPoint() + p.getNumVecs());
		}

		const uint32_t trailingSpace = size_ - (requiredVecs * sizeof(Vec4f));
		X_UNUSED(trailingSpace);
	}

	void XCBuffer::recalculateUpdateRate(void)
	{
		updateRate_ = UpdateFreq::FRAME;

		for (const auto& p : params_)
		{
			updateRate_ = updateFreqMax(updateRate_, p.getUpdateRate());
		}
	}

	void XCBuffer::computeFlags(void)
	{
		updateFeqFlags_.Clear();
		paramFlags_.Clear();

		for (const auto& p : params_)
		{
			paramFlags_.Set(p.getType());
			updateFeqFlags_.Set(p.getUpdateRate());
		}
	}

	void XCBuffer::computeHash(void)
	{
		Hasher hasher;

		hasher.reset(0);
		hasher.update(name_.c_str(), name_.length());
		hasher.update(updateFeqFlags_.ToInt());
		hasher.update(updateRate_);
		hasher.update(size_);
		hasher.update(bindPoint_);
		hasher.update(bindCount_);
		hasher.update(paramFlags_.ToInt());

		for (const auto& p : params_)
		{
			p.addToHash(hasher);
		}

		hash_ = hasher.finalize();
	}

} // namespace shader

X_NAMESPACE_END