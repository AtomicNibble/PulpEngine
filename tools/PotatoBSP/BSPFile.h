#pragma once


#ifndef X_BSP_FILE_TYPES_H_
#define X_BSP_FILE_TYPES_H_

#include "BSPData.h"


X_NAMESPACE_BEGIN(bsp)


class BSPFile
{
public:
	BSPFile(const BSPData& Data);

	bool save(const char* name);

private:

	const BSPData& data_;
};




X_NAMESPACE_END

#endif // !X_BSP_FILE_TYPES_H_