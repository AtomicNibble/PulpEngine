#pragma once

#include <Containers/Array.h>

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



// TODO: make a seperate lookup array which is more cache friendly.
struct SymModule
{
    using GuidByteArr = std::array<uint8_t, 16>;

    SymModule() :
        baseAddr_(0),
        virtualSize_(0),
        age_(0)
    {
    }

    uintptr_t baseAddr_;
    uint32_t virtualSize_;
    uint32_t age_; // TODO: use..

    GuidByteArr guid_;

    core::string path_;

    // TODO: do i need one of these for each PDB..?
    IDiaDataSource* pSource_;
    IDiaSession* pSession_;
};

class SymResolver
{
public:
    using SymPathArr = core::Array<SymPath>;
    using SymModuleArr = core::Array<SymModule>;

    using GuidByteArr = SymModule::GuidByteArr;

public:
    TELEMETRY_SYMLIB_EXPORT SymResolver(core::MemoryArenaBase* arena) :
        paths_(arena),
        modules_(arena),
        pSource_(nullptr),
        pSession_(nullptr)
    {
    }

    TELEMETRY_SYMLIB_EXPORT ~SymResolver();

    TELEMETRY_SYMLIB_EXPORT bool init(void);

    TELEMETRY_SYMLIB_EXPORT bool loadPDB(uintptr_t baseAddr, uint32_t virtualSize, const GuidByteArr& guid, core::string_view path);

    void addPath(const core::Path<>& path, SymType::Enum type);
    void setCachePath(const core::Path<>& path);

private:
    bool haveModuleWithBase(uintptr_t baseAddr);

private:
    core::Path<> cachePath_;
    SymPathArr paths_;
    SymModuleArr modules_;

    // DIA
    IDiaDataSource* pSource_;
    IDiaSession* pSession_;
};



X_NAMESPACE_END

