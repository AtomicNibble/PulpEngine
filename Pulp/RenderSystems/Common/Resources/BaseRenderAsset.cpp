#include "stdafx.h"
#include "BaseRenderAsset.h"
#include "../ReaderThread.h"
#include "../XRender.h"

X_NAMESPACE_BEGIN(render)


bool XRenderResourceContainer::removeAsset(core::XBaseAsset* pAsset)
{
	X_ASSERT_NOT_NULL(pAsset);

	if (XResourceContainer::removeAsset(pAsset))
	{
		/*
		if (gRenDev && gRenDev->rThread())
			gRenDev->rThread()->RC_ReleaseBaseResource(pAsset);
		else
		{
			X_WARNING("RenderResource", "failed to release render resource: %s",
				pAsset->resourceName());
		}
		*/
		return true;
	}
	return false;
}


X_NAMESPACE_END