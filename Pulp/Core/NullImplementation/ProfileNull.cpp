#include "stdafx.h"
#include "ProfileNull.h"


X_NAMESPACE_BEGIN(core)

void ProfileNull::registerVars(void)
{

}

void ProfileNull::registerCmds(void)
{

}

bool ProfileNull::init(ICore* pCore)
{
	X_UNUSED(pCore);

	return true;
}

void ProfileNull::shutDown(void)
{

}

void ProfileNull::AddProfileData(XProfileData* pData)
{
	X_UNUSED(pData);
}

void ProfileNull::OnFrameBegin(void)
{

}

void ProfileNull::OnFrameEnd(void)
{

}

void ProfileNull::Render(void)
{


}

X_NAMESPACE_END


