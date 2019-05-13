#pragma once

#include <Containers/Array.h>
#include <Util/Guid.h>

struct IDiaDataSource;
struct IDiaSession;

X_NAMESPACE_BEGIN(telemetry)

// Symbol stuff
X_DECLARE_ENUM(SymPathType)(
    Cache,
    Path,
    Server
);


struct SymPath
{
    core::Path<> path;
    SymPathType::Enum type;
};

// TODO: make a seperate lookup array which is more cache friendly.
struct SymModule
{

public:
    SymModule();
    SymModule(const SymModule&) = delete;
    SymModule(SymModule&& oth);
    ~SymModule();

    SymModule& operator=(const SymModule&) = delete;
    SymModule& operator=(SymModule&& oth);

    X_INLINE bool containsAddr(uintptr_t addr) const {
        return addr > baseAddr_ && addr < (baseAddr_ + virtualSize_);
    }

public:
    uintptr_t baseAddr_;
    uint32_t virtualSize_;

    core::Guid guid_;
    uint32_t age_;

    core::string path_;

    // Seams we need one for each PDB
    IDiaDataSource* pSource_;
    IDiaSession* pSession_;
};

class SymResolver
{
public:
    using SymPathArr = core::Array<SymPath>;
    using SymModuleArr = core::Array<SymModule>;

public:
    TELEMETRY_SYMLIB_EXPORT SymResolver(core::MemoryArenaBase* arena);
    TELEMETRY_SYMLIB_EXPORT ~SymResolver();

    TELEMETRY_SYMLIB_EXPORT bool init(void);
    TELEMETRY_SYMLIB_EXPORT bool loadPDB(uintptr_t baseAddr, uint32_t virtualSize, const core::Guid& guid, uint32_t age, core::string_view path);

    TELEMETRY_SYMLIB_EXPORT void setCachePath(core::string_view path);
    TELEMETRY_SYMLIB_EXPORT void addPath(core::string_view path, SymPathType::Enum type);

    // want some api for getting functions names?
    TELEMETRY_SYMLIB_EXPORT bool resolveForAddr(uintptr_t addr);

    TELEMETRY_SYMLIB_EXPORT static void addSymSrvFolderNameForPDB(core::Path<>& path, const core::Guid& guid, uint32_t age);

private:
    bool haveModuleWithBase(uintptr_t baseAddr);

private:
    SymPathArr searchPaths_;
    SymModuleArr modules_;

    bool comInit_;
};



X_NAMESPACE_END

