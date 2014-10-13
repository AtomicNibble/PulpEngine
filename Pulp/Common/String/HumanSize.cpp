#include "EngineCommon.h"
#include "HumanSize.h"


X_NAMESPACE_BEGIN(core)

namespace HumanSize
{

	const char* toString(Str& str, size_t numBytes)
	{
		str.clear();

		// Kibi not Kilo

		// i use each type untill there is 10,240 of them.
		if (numBytes <= 10240)
			str.appendFmt("%ibytes", numBytes);
		else if (numBytes <= 10485760)
		{
			str.appendFmt("%.2fKB", static_cast<float>(numBytes) / 1024);
		}
		else if (numBytes <= 10737418240)
		{
			str.appendFmt("%.2fMB", (static_cast<float>(numBytes) / 1024) / 1024);
		}
		else
		{
			str.appendFmt("%.2fMB", (static_cast<float>(numBytes) / 1024) / 1024);
		}


		return str.c_str();
	}

}




X_NAMESPACE_END;