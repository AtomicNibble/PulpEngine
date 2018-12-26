#include "stdafx.h"
#include "Localisation.h"

#include <String/Json.h>

#include <IFileSys.h>

X_NAMESPACE_BEGIN(locale)


Localisation::Localisation(core::MemoryArenaBase* arena) :
    arena_(arena),
    ht_(arena, core::bitUtil::NextPowerOfTwo(MAX_STRINGS))
{

}

Localisation::~Localisation()
{

}


Localisation::string_view Localisation::getString(Key k) const
{
    auto it = ht_.find(k);
    if (it != ht_.end()) {
        return string_view(it->second.begin(), it->second.length());
    }

    return {};
}

bool Localisation::loadDict(core::Path<char>& path)
{
    core::XFileScoped file;
    core::FileFlags mode;
    mode.Set(core::FileFlag::READ);
    mode.Set(core::FileFlag::SHARE);

    // TODO: async init it up!
    if (!file.openFile(path, mode)) {
        X_ERROR("Lang", "Failed to open lang file");
        return false;
    }

    core::Array<char> data(arena_);
    data.resize(safe_static_cast<size_t>(file.remainingBytes()));

    if (file.read(data.data(), data.size()) != data.size()) {
        X_ERROR("Lang", "Failed to read lang file");
        return false;
    }

    // it's just json yo.
    core::json::MemoryStream ms(data.begin(), data.size());
    core::json::EncodedInputStream<core::json::UTF8<>, core::json::MemoryStream> is(ms);

    core::json::Document d;
    if (d.ParseStream<core::json::kParseCommentsFlag>(is).HasParseError()) {
        auto err = d.GetParseError();
        const char* pErrStr = core::json::GetParseError_En(err);
        size_t offset = d.GetErrorOffset();
        size_t line = core::strUtil::LineNumberForOffset(data.begin(), data.end(), offset);

        X_ERROR("Lang", "Failed to parse land file (%" PRIi32 "): Offset: %" PRIuS " Line: %" PRIuS " Err: %s", err, offset, line, pErrStr);
        return false;
    }

    // should just be key value for days!
    if (d.GetType() != core::json::Type::kObjectType) {
        X_ERROR("Lang", "Unexpected type");
        return false;
    }

    for (auto& p : d.GetObject())
    {
        if (p.value.GetType() != core::json::Type::kStringType) {
            X_ERROR("Lang", "All values must be strings");
            return false;
        }

        std::string_view name(p.name.GetString(), p.name.GetStringLength());
        core::StrHash hash(name.data(), name.length());
        core::string val(p.value.GetString(), p.value.GetStringLength());

        X_MAYBE_UNUSED auto [_, inserted] = ht_.emplace(hash, std::move(val));
        if (!inserted) {
            X_ERROR("Lang", "Hash collision for: \"%.*s\"", name.length(), name.data());
            return false;
        }
    }

    return true;
}

X_NAMESPACE_END
