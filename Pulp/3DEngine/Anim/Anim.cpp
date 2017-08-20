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


void Anim::processData(AnimHeader& hdr, core::UniquePointer<uint8_t[]> data)
{
	X_UNUSED(hdr, data);


	hdr_ = hdr;
	data_ = std::move(data);
	status_ = core::LoadStatus::Complete;
}





X_NAMESPACE_END