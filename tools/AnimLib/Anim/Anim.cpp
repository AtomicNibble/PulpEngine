#include "stdafx.h"
#include "Anim.h"

#include <Math\XQuatCompressed.h>
#include <Memory\MemCursor.h>

#include <Time\TimeLiterals.h>

X_NAMESPACE_BEGIN(anim)

Bone::Bone()
{
	pName_ = nullptr;
	numAngles_ = 0;
	numPos_ = 0;
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

void Bone::decodeFrame(Transformf& trans, int32_t frame) const
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


				int32_t offset = frame - first;
				int32_t range = last - first;

				// if range is 10 and offset 5 i want o.5 yo

				// calculate Fraction
				float fraction = static_cast<float>(offset) / range;

				auto firstPos = GetPostion(firstIdx);
				auto lastPos = GetPostion(lastIdx);

				pos = firstPos.lerp(fraction, lastPos);
			}
		}
	}


	trans.pos = pos;

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

void Anim::timeToFrame(core::TimeVal time, FrameBlend& frame) const
{
	int32_t numFrames = getNumFrames();
	int32_t fps = getFps();

	// this turns a elapsed time into frame info.
	if (numFrames <= 1) {
		frame.cylces = 0;
		frame.frame1 = 0;
		frame.frame2 = 0;
		frame.backlerp = 0.0f;
		frame.frontlerp = 1.0f;
		return;
	}

	if (time <= 0_tv) {
		frame.cylces = 0;
		frame.frame1 = 0;
		frame.frame2 = 1;
		frame.backlerp = 0.0f;
		frame.frontlerp = 1.0f;
		return;
	}

	// multiple time by fps then device by 1 second.
	const core::TimeVal oneSecond = 1000_ms;

	core::TimeVal frameTime = core::TimeVal(time.GetValue() * fps);
	int32_t frameNum = safe_static_cast<int32_t>((frameTime / oneSecond).GetValue());

	// frames are 0-numFrames
	// so frame 29 is last.
	frame.cylces = frameNum / (numFrames);
	frame.frame1 = frameNum % (numFrames);
	frame.frame2 = frame.frame1 + 1;
	if (frame.frame2 >= numFrames) {
		frame.frame2 = 0;
	}

	// work out lerps.
	// we turn range of 1 second.
	auto offset = (frameTime % oneSecond);

	frame.backlerp = offset.GetMilliSeconds() / oneSecond.GetMilliSeconds();
	frame.frontlerp = 1.0f - frame.backlerp;

	X_ASSERT(frame.backlerp >= 0 && frame.backlerp <= 1.f && ((frame.backlerp + frame.frontlerp) == 1.f), "Invalid lerp values")(frame.backlerp, frame.frontlerp, (frame.backlerp + frame.frontlerp));
}


void Anim::getFrame(const FrameBlend& frame, TransformArr& boneTransOut, const IndexArr& indexes) const
{
	X_UNUSED(frame, boneTransOut, indexes);

	// we need to animate each bone in the model that we affect
	// the index map is same size as bones, and the index is the index of the bone in animation.
	X_ASSERT(boneTransOut.size() == indexes.size(), "Size mismatch")();

	TransformArr blendTrans(boneTransOut.getArena(), boneTransOut.size());
	IndexArr lerpIndex(boneTransOut.getArena());

	// decode all the joints.
	for (int32_t i =0; i<safe_static_cast<int32_t>(indexes.size()); i++)
	{
		if (indexes[i] < 0) {
			continue;
		}

		int32_t animBoneIdx = indexes[i];

		auto& bone = bones_[animBoneIdx];

		bone.decodeFrame(boneTransOut[i], frame.frame1);
		bone.decodeFrame(blendTrans[i], frame.frame2);

		lerpIndex.append(i);
	}

	// blend them
	Util::blendBones(boneTransOut, blendTrans, lerpIndex, frame.backlerp);

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