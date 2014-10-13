#include <EngineCommon.h>

#include "AssertContainer.h"

X_NAMESPACE_BEGIN(core)

const int XBaseAsset::release()
{
	--RefCount_;

	int ref = RefCount_;

	if (ref <= 0)
	{
		X_ASSERT_NOT_NULL(pContainer_);


		pContainer_->removeAsset(this);
		return 0;
	}

	return ref;
}


bool XResourceContainer::removeAsset(XBaseAsset* pAsset)
{
	X_ASSERT_NOT_NULL(pAsset);

	if (list[pAsset->ID_] = pAsset)
	{
		list[pAsset->ID_] = nullptr;
		hash.erase(pAsset->resourceName_);
		return true;
	}
	return false;
}




X_NAMESPACE_END