#pragma once

#include <Containers/Array.h>

struct IDiaDataSource;
struct IDiaSession;

X_NAMESPACE_BEGIN(telemetry)

// Symbol stuff
X_DECLARE_ENUM(SymType)(
    Path,
    Server
);

struct SymPath
{
    core::Path<> path;
    SymType::Enum type;
};


struct SymGuid
{
    using ByteArr = std::array<uint8_t, 16>;
    union {
        ByteArr bytes;
        struct GUID {
            unsigned long  data1;
            unsigned short data2;
            unsigned short data3;
            unsigned char  data4[8];
        } guid;
    };
};

static_assert(sizeof(SymGuid) == 16, "SymGuid has incorrect size");

// TODO: make a seperate lookup array which is more cache friendly.
struct SymModule
{

public:
    SymModule();
    ~SymModule();

    X_INLINE bool containsAddr(uintptr_t addr) const {
        return addr > baseAddr_ && addr < (baseAddr_ + virtualSize_);
    }

public:
    uintptr_t baseAddr_;
    uint32_t virtualSize_;

    SymGuid guid_;
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
    TELEMETRY_SYMLIB_EXPORT bool loadPDB(uintptr_t baseAddr, uint32_t virtualSize, const SymGuid& guid, uint32_t age, core::string_view path);

    TELEMETRY_SYMLIB_EXPORT void addPath(core::string_view path, SymType::Enum type);

    // want some api for getting functions names?
    TELEMETRY_SYMLIB_EXPORT bool resolveForAddr(uintptr_t addr);

private:
    bool haveModuleWithBase(uintptr_t baseAddr);

private:
    SymPathArr searchPaths_;
    SymModuleArr modules_;

    bool comInit_;
};



X_NAMESPACE_END

