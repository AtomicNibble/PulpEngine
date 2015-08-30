#pragma once


#ifndef _X_SOUNG_I_H_
#define _X_SOUNG_I_H_


X_NAMESPACE_BEGIN(sound)

class XSound : public ISound
{
public:
	XSound();
	virtual ~XSound();

	virtual bool Init(void) X_OVERRIDE;
	virtual void ShutDown(void) X_OVERRIDE;
	virtual void release(void) X_OVERRIDE;

	virtual void Update(void) X_OVERRIDE;



	// Shut up!
	virtual void Mute(bool mute) X_OVERRIDE;

	// Volume
	virtual void SetMasterVolume(float v) X_OVERRIDE;
	virtual float GetMasterVolume(void) const X_OVERRIDE;

private:


};

X_NAMESPACE_END

#endif // !_X_SOUNG_I_H_
