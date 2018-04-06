#pragma once

#define X_SOURCE_INFO X_NAMESPACE(core)::SourceInfo("core", __FILE__, __LINE__, __FUNCTION__, __FUNCSIG__)

#if X_ENABLE_MEMORY_SOURCE_INFO

#define X_SOURCE_INFO_MEM_CB(x) , x
#else

#define X_SOURCE_INFO_MEM_CB(x)

#endif // !X_ENABLE_MEMORY_SOURCE_INFO
