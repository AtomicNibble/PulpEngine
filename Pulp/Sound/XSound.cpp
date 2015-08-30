#include "stdafx.h"
#include "XSound.h"




X_NAMESPACE_BEGIN(sound)


XSound::XSound()
{

}

XSound::~XSound()
{
}


bool XSound::Init(void)
{

	return true;
}

void XSound::ShutDown(void)
{
	X_LOG0("SoundSys", "Shutting Down");


}

void XSound::release(void)
{
	X_DELETE(this,g_SoundArena);
}

void XSound::Update(void)
{
	X_PROFILE_BEGIN("SoundUpdate", core::ProfileSubSys::SOUND);

}


// Shut up!
void XSound::Mute(bool mute)
{
	X_UNUSED(mute);
}

// Volume
void XSound::SetMasterVolume(float v)
{
	// nope
	X_UNUSED(f);
}

float XSound::GetMasterVolume(void) const
{
	return 1.f; // MAX VOLUME ALL THE TIME!!!
}


X_NAMESPACE_END