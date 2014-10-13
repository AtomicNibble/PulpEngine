#include "stdafx.h"
#include "XSound.h"




X_NAMESPACE_BEGIN(sound)


XSound::XSound()
{

}

XSound::~XSound()
{
}


bool XSound::Init()
{

	return true;
}

void XSound::ShutDown()
{
	X_LOG0("SoundSys", "Shutting Down");


}

void XSound::release(void)
{
	X_DELETE(this,g_SoundArena);
}

void XSound::Update()
{
	X_PROFILE_BEGIN("SoundUpdate", core::ProfileSubSys::SOUND);

}


// Shut up!
void XSound::Mute(bool mute)
{

}

// Volume
void XSound::SetMasterVolume(float v)
{
	// nope
}

float XSound::GetMasterVolume() const
{

	return 1.f; // MAX VOLUME ALL THE TIME!!!
}


X_NAMESPACE_END