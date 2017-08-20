#include "stdafx.h"
#include "Anim.h"

X_NAMESPACE_BEGIN(anim)


Anim::Anim(core::string& name) :
	name_(name)
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
	X_ASSERT(numBones() == bonesOut.size(), "Size mismatch")(numBones(), bonesOut.size());
	
	X_UNUSED(delta, state);




}

void Anim::processData(AnimHeader& hdr, core::UniquePointer<uint8_t[]> data)
{
	X_UNUSED(hdr, data);


	hdr_ = hdr;
	data_ = std::move(data);
	status_ = core::LoadStatus::Complete;
}





X_NAMESPACE_END