#pragma once


X_NAMESPACE_BEGIN(telemetry)


#if !defined(TELEM_SRV_EXPORT)

#if !defined(X_LIB)
#define TELEM_SRV_EXPORT X_IMPORT
#else
#define TELEM_SRV_EXPORT
#endif

#endif


class TELEM_SRV_EXPORT Server
{
public:
    Server(core::MemoryArenaBase* arena);
    ~Server();

    bool run();

private:
    bool listen(void);


private:
    core::MemoryArenaBase* arena_;
};

X_NAMESPACE_END