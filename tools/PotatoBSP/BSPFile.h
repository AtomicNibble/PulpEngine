#pragma once


#ifndef X_BSP_FILE_TYPES_H_
#define X_BSP_FILE_TYPES_H_

#include "BSPData.h"


X_NAMESPACE_BEGIN(bsp)


class BSPFile
{
public:
	BSPFile();

	bool save(const BSPData& Data, const char* name);

private:

};




X_NAMESPACE_END

#endif // !X_BSP_FILE_TYPES_H_