#include "stdafx.h"
#include "Font.h"

#include "Sys\XFontSystem.h"

#include <IFileSys.h>
#include <Threading\JobSystem2.h>

#include <Memory\AllocationPolicies\MallocFreeAllocator.h>

X_NAMESPACE_BEGIN(font)

using namespace core;

namespace
{

    struct JobData
    {
        char* pData;
        uint32_t dataSize;
    };

} // namespace

void XFont::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
    core::XFileAsync* pFile, uint32_t bytesTransferred)
{
    X_UNUSED(fileSys);
    X_UNUSED(bytesTransferred);

    X_ASSERT(pRequest->getType() == core::IoRequest::OPEN_READ_ALL, "Recived unexpected request type")(pRequest->getType());
    const core::IoRequestOpenRead* pOpenRead = static_cast<const core::IoRequestOpenRead*>(pRequest);

    if (!pFile) {
        loadStatus_ = LoadStatus::Error;
        X_ERROR("Font", "Failed to load font def file");
        signal_.raise();
        return;
    }

    JobData data;
    data.pData = reinterpret_cast<char*>(pOpenRead->pBuf);
    data.dataSize = pOpenRead->dataSize;

    // dispatch a job to parse it?
    gEnv->pJobSys->CreateMemberJobAndRun<XFont>(this, &XFont::ProcessFontFile_job, data JOB_SYS_SUB_ARG(core::profiler::SubSys::FONT));
}

void XFont::ProcessFontFile_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
    X_UNUSED(jobSys);
    X_UNUSED(threadIdx);
    X_UNUSED(pJob);

    JobData* pJobData = static_cast<JobData*>(pData);

    // so we have the file data just need to process it.
    if (processData(pJobData->pData, pJobData->pData + pJobData->dataSize, sourceName_, effects_, flags_)) {
        // now create a fontTexture and async load it's glyph file.
        // as soon as we assign 'pFontTexture_' other threads might start accessing it.
        // other threads will not access any logic other than IsReady untill after IsReady returns true.
        pFontTexture_ = fontSys_.getFontTexture(sourceName_, true);
        if (!pFontTexture_) {
            X_ERROR("Font", "Failed to get font texture for: \"%s\"", sourceName_.c_str());
            loadStatus_ = LoadStatus::Error;
        }
        else {
            loadStatus_ = LoadStatus::Complete;
        }
    }
    else {
        X_ERROR("Font", "Error parsing font def file");
        loadStatus_ = LoadStatus::Error;
    }

    X_DELETE_ARRAY(pJobData->pData, g_fontArena);

    signal_.raise();
}

void XFont::Reload(void)
{
    loadFont(true);
}

bool XFont::loadFont(bool async)
{
    return loadFontDef(async);
}

bool XFont::loadFontDef(bool async)
{
    // are we loading already?
    if (loadStatus_ == LoadStatus::Loading) {
        return true;
    }

    core::Path<char> path;
    path = "Fonts/";
    path.setFileName(name_.c_str());
    path.setExtension(".font");

    core::fileModeFlags mode;
    mode.Set(fileMode::READ);
    mode.Set(fileMode::SHARE);

    if (async) {
        signal_.clear();
        loadStatus_ = LoadStatus::Loading;

        // load the file async
        core::IoRequestOpenRead open;
        open.callback.Bind<XFont, &XFont::IoRequestCallback>(this);
        open.mode = mode;
        open.path = path;
        open.arena = g_fontArena;

        gEnv->pFileSys->AddIoRequestToQue(open);
    }
    else {
        core::XFileMemScoped file;
        if (!file.openFile(path.c_str(), mode)) {
            return false;
        }

        if (!processData(file->getBufferStart(), file->getBufferEnd(), sourceName_, effects_, flags_)) {
            X_ERROR("Font", "Error processing font def file: \"%s\"", path.c_str());
            return false;
        }

        X_ASSERT(sourceName_.isNotEmpty(), "Source name is empty")(sourceName_.isEmpty());
        X_ASSERT(pFontTexture_ == nullptr, "Fonttexture already set")(pFontTexture_);

        // now we need a glyph cache.
        pFontTexture_ = fontSys_.getFontTexture(sourceName_, async);
        if (!pFontTexture_) {
            X_ERROR("Font", "Error loaded font file: \"%s\"", sourceName_.c_str());
            return false;
        }

        loadStatus_ = LoadStatus::Complete;
    }

    return true;
}

bool XFont::processData(const char* pBegin, const char* pEnd, SourceNameStr& sourceNameOut,
    EffetsArr& effectsOut, FontFlags& flags)
{
    ptrdiff_t size = (pEnd - pBegin);
    if (!size) {
        return false;
    }

    effectsOut.clear();
    sourceNameOut.clear();
    flags.Clear();

    core::json::MemoryStream ms(pBegin, size);
    core::json::EncodedInputStream<core::json::UTF8<>, core::json::MemoryStream> is(ms);

    core::json::Document d;
    if (d.ParseStream<core::json::kParseCommentsFlag>(is).HasParseError()) {
        auto err = d.GetParseError();
        const char* pErrStr = core::json::GetParseError_En(err);
        size_t offset = d.GetErrorOffset();
        size_t line = core::strUtil::LineNumberForOffset(pBegin, pEnd, offset);

        X_ERROR("Font", "Failed to parse font desc(%" PRIi32 "): Offset: %" PRIuS " Line: %" PRIuS " Err: %s",
            err, offset, line, pErrStr);
        return false;
    }

    if (d.GetType() != core::json::Type::kObjectType) {
        X_ERROR("Font", "Unexpected type");
        return false;
    }

    if (!d.HasMember("font")) {
        X_ERROR("Font", "Missing font object");
        return false;
    }

    auto& font = d["font"];
    if (!font.HasMember("source")) {
        X_ERROR("Font", "Missing source field");
        return false;
    }

    auto& source = font["source"];
    sourceNameOut.set(source.GetString(), source.GetString() + source.GetStringLength());

    if (font.HasMember("proportional")) {
        if (font["proportional"].GetBool()) {
            flags.Set(FontFlag::PROPORTIONAL);
        }
    }
    if (font.HasMember("sdf")) {
        if (font["sdf"].GetBool()) {
            flags.Set(FontFlag::SDF);
        }
    }

    if (font.HasMember("effects"))
    {
        auto& effects = font["effects"];
        if (effects.GetType() != core::json::Type::kArrayType) {
            X_ERROR("Font", "Effects field should be a array");
            return false;
        }

        for (auto& effectDesc : effects.GetArray())
        {
            FontEffect effect;

            if (effectDesc.HasMember("name")) {
                auto& name = effectDesc["name"];
                effect.name.set(name.GetString(), name.GetString() + name.GetStringLength());
            }

            if (!effectDesc.HasMember("passes")) {
                X_ERROR("Font", "Missing passes field");
                return false;
            }
            
            auto& passes = effectDesc["passes"];
            if (passes.GetType() != core::json::Type::kArrayType) {
                X_ERROR("Font", "Passes field should be a array");
                return false;
            }

            for (auto& passDesc : passes.GetArray())
            {
                FontPass pass;

                if (passDesc.HasMember("color")) {
                    X_ASSERT_NOT_IMPLEMENTED();
                }
                if (passDesc.HasMember("pos")) {
                    X_ASSERT_NOT_IMPLEMENTED();
                }

                effect.passes.append(pass);
            }

            if (effect.passes.isEmpty())
            {
                X_ERROR("Font", "Effect has no passes");
                return false;
            }

            effectsOut.append(effect);
        }
    }

    if (effectsOut.isEmpty())
    {
        X_WARNING("Font", "No effects provided, adding default");

        FontEffect effect;
        effect.passes.resize(1);
        effectsOut.append(effect);
    }

    return true;
}

X_NAMESPACE_END
