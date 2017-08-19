#pragma once

X_NAMESPACE_BEGIN(anim)


struct IAnim;

struct IAnimManager
{
	virtual ~IAnimManager() {}

	// returns null if not found, ref count unaffected
	virtual IAnim* findAnim(const char* pAnimName) const X_ABSTRACT;
	virtual IAnim* loadAnim(const char* pAnimName) X_ABSTRACT;


};




X_NAMESPACE_END
