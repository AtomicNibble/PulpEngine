#include "stdafx.h"
#include "Anim.h"

#include <Math\XQuatCompressed.h>
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

void Bone::setMatrixForFrame(Matrix44f& mat, int32_t frame) const
{
	Vec3f pos;


	// so this gives us the position for the frame relative to the bones base position.
	// so each frame we need to reset all the matrix to pose positions
	// then apply animations to them.
	// we need some kinda of index map that we calculate for anim to model


	// position
	if (numPos_)
	{
		if (numPos_ == 0)
		{
			pos = posMin_;
		}
		else
		{
			// do we have any data for you yet?
			if (pPosFrames_[0] > frame) {
				return;
			}

			// we have multiple pos.
			// find a frame for us.
			int32_t posFrameIdx = 0;
			while (pPosFrames_[posFrameIdx] < frame) {
				++posFrameIdx;
			}

			// we have data
			if (pPosFrames_[posFrameIdx] == frame)
			{
				pos = GetPostion(posFrameIdx);
			}
			else // if(posFrameIdx != 0)
			{
				// interpolate.
				int32_t firstIdx = posFrameIdx - 1;
				int32_t lastIdx = posFrameIdx;

				X_ASSERT(firstIdx >= 0, "invalid index")(firstIdx);

				int32_t first = pPosFrames_[firstIdx];
				int32_t last = pPosFrames_[lastIdx];

				// calculate Fraction
				float fraction = 1.0f / (last - first);

				auto firstPos = GetPostion(first);
				auto lastPos = GetPostion(last);

				pos = firstPos.lerp(fraction, lastPos);
			}
		}
	}


	mat.setTranslate(pos);

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

	// int32_t frame = 10;
#if 0
	for (int32_t frame = 0; frame < numFrames(); ++frame)
	{
		X_LOG0("Anim", "Frame: %i", frame);

		for (size_t i = 0; i<bones_.size(); i++)
		{
			auto& bone = bones_[i];
			bone.setMatrixForFrame(bonesOut[i], frame);

			auto pos = bonesOut[i].getTranslate();
			X_LOG0("Anim", "Bone \"%s\" pos(%g,%g,%g)", bone.getName(), pos.x, pos.y, pos.z);
		}
	}

#endif
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

	// test.
	Mat44Arr bones(g_AnimLibArena);
	bones.resize(hdr.numBones);

	AnimState state;

	update(core::TimeVal(0ll), state, bones);

}





X_NAMESPACE_END