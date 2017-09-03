#pragma once

X_NAMESPACE_BEGIN(anim)


class Anim;

struct IAnimManager
{
	virtual ~IAnimManager() {}

	// returns null if not found, ref count unaffected
	virtual Anim* findAnim(const char* pAnimName) const X_ABSTRACT;
	virtual Anim* loadAnim(const char* pAnimName) X_ABSTRACT;


};




X_NAMESPACE_END
