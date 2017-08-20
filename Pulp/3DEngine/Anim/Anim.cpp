#include "stdafx.h"
#include "Anim.h"

#include <Memory\MemCursor.h>

X_NAMESPACE_BEGIN(anim)

Bone::Bone()
{
	pName_ = nullptr;
	pAngleData_ = nullptr;
	pAngleFrames_ = nullptr;
	pPosScalers_ = nullptr;
}

void Bone::setName(const char* pName)
{
	pName_ = pName;
}

void Bone::load(core::MemCursor& cursor)
{
	// angles
	numAngles_ = cursor.getSeek<uint16_t>();

	if (numAngles_ > 0)
	{
		if (numAngles_ > 1)
		{
			// frame numbers.
			pAngleFrames_ = cursor.postSeekPtr<uint8_t>(numAngles_);
		}


		pAngleData_ = cursor.postSeekPtr<XQuatCompressedf>(numAngles_);
	}


	// positions.
	numPos_ = cursor.getSeek<uint16_t>();

	if (numPos_ == 0)
	{
		posMin_ = cursor.getSeek<Vec3f>();
	}
	else
	{
		if (numPos_ > 1)
		{
			// frame numbers.
			pPosFrames_ = cursor.postSeekPtr<uint8_t>(numPos_);
		}

		pPosScalers_ = cursor.postSeekPtr<Vec3<uint8_t>>(numPos_);

		posMin_ = cursor.getSeek<Vec3f>();
		posRange_ = cursor.getSeek<Vec3f>();
	}
}

// ----------------------------------

Anim::Anim(core::string& name, core::MemoryArenaBase* arena) :
	name_(name),
	bones_(arena)
{
	id_ = 0;
	status_ = core::LoadStatus::NotLoaded;
}

Anim::~Anim()
{

}

void Anim::update(core::TimeVal delta, AnimState& state, Mat44Arr& bonesOut) const
{
	// so we want generate data and shove it in the bones out.
	// we have state to know last position.
	// and delta since we last updated.
	X_ASSERT(numBones() == static_cast<int32_t>(bonesOut.size()), "Size mismatch")(numBones(), bonesOut.size());	
	X_UNUSED(delta, state);



}


void Anim::processData(AnimHeader& hdr, core::UniquePointer<uint8_t[]> data)
{
	X_UNUSED(hdr, data);

	bones_.resize(hdr.numBones);

	core::MemCursor cursor(data.ptr(), hdr.dataSize);

	for (auto& bone : bones_)
	{
		bone.setName(cursor.getPtr<const char>());

		while (cursor.get<char>() != '\0') {
			cursor.seekBytes(1);
		}
		cursor.seekBytes(1);
	}

	for (auto& bone : bones_)
	{
		bone.load(cursor);

	}


	hdr_ = hdr;
	data_ = std::move(data);
	status_ = core::LoadStatus::Complete;
}





X_NAMESPACE_END