#pragma once


#ifndef X_MODEL_INFO_H_
#define X_MODEL_INFO_H_

#include <String\Path.h>
#include <Math\XAabb.h>

namespace ModelInfo
{

	bool GetNModelAABB(const core::string& name, AABB& boxOut);


} // namespace ModelInfo


#endif // !X_MODEL_INFO_H_