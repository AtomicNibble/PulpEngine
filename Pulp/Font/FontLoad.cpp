#include "stdafx.h"
#include "Font.h"

#include "Sys\XFontSystem.h"

#include <IFileSys.h>
#include <Threading\JobSystem2.h>

#include <Memory\AllocationPolicies\MallocFreeAllocator.h>

X_NAMESPACE_BEGIN(font)

using namespace core;
using namespace xml;
using namespace rapidxml;

namespace
{
    void* XmlAllocate(std::size_t size)
    {
        return X_NEW_ARRAY(char, size, g_fontArena, "xmlPool");
    }

    void XmlFree(void* pointer)
    {
        char* pChar = (char*)pointer;
        X_DELETE_ARRAY(pChar, g_fontArena);
    }

    bool ParseColValue(const char* attr, uint8_t& col)
    {
        if (strUtil::IsNumeric(attr)) {
            float value = strUtil::StringToFloat<float>(attr);
            if (value >= 0.f && value <= 1.f) {
                col = static_cast<uint8_t>(value * 255.f);
                return true;
            }
        }
        X_ERROR("Font", "color value is not a valid float beetween: 0 - 1");
        return false;
    }

    bool ParsePassCol(xml_node<>* node, Color8u& col)
    {
        X_ASSERT_NOT_NULL(node);
        xml_attribute<>* attr;
        int num = 0;

        for (attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
            if (strUtil::IsEqual("r", attr->name())) {
                if (ParseColValue(attr->value(), col.r))
                    num++;
            }
            else if (strUtil::IsEqual("g", attr->name())) {
                if (ParseColValue(attr->value(), col.g))
                    num++;
            }
            else if (strUtil::IsEqual("b", attr->name())) {
                if (ParseColValue(attr->value(), col.b))
                    num++;
            }
            else if (strUtil::IsEqual("a", attr->name())) {
                if (ParseColValue(attr->value(), col.a))
                    num++;
            }
        }

        if (num == 4) {
            return true;
        }
        X_ERROR("Font", "invalid color node required attr: r,g,b,a");
        return false;
    }

    bool ParsePassPos(xml_node<>* node, Vec2f& pos)
    {
        X_ASSERT_NOT_NULL(node);
        xml_attribute<>* attr;
        int num = 0;

        for (attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
            if (strUtil::IsEqual("x", attr->name())) {
                if (strUtil::IsNumeric(attr->value())) {
                    pos.x = strUtil::StringToFloat<float>(attr->value());
                    num++;
                }
            }

            if (strUtil::IsEqual("y", attr->name())) {
                if (strUtil::IsNumeric(attr->value())) {
                    pos.y = strUtil::StringToFloat<float>(attr->value());
                    num++;
                }
            }
        }
        if (num == 2) {
            return true;
        }
        X_ERROR("Font", "invalid pos node required attr: x and y");
        return false;
    }

    bool ParseEffect(xml_node<>* node, FontEffect& effect)
    {
        X_ASSERT_NOT_NULL(node);
        core::StackString<64> name;
        xml_attribute<>* attr;
        xml_node<>* passNode;
        xml_node<>* pass;

        for (attr = node->first_attribute();
             attr; attr = attr->next_attribute()) {
            if (strUtil::IsEqualCaseInsen("name", attr->name())) {
                if (attr->value_size() <= 64) {
                    name.append(attr->value());
                }
                else {
                    X_ERROR("Font", "effect name is longer then 64: %s", attr->name());
                }
            }
        }

        if (name.isEmpty()) {
            X_ERROR("Font", "effect missing name attribute");
            return false;
        }

        effect.name = name;

        for (pass = node->first_node();
             pass; pass = pass->next_sibling()) {
            if (strUtil::IsEqualCaseInsen("pass", pass->name())) {
                // add a pass, additional info is optional.
                FontPass effectPass;

                if (effect.passes.size() == MAX_FONT_PASS) {
                    X_ERROR("Font", "font exceeds max pass count: " X_STRINGIZE(MAX_FONT_PASS) " ignoring extra passes");
                    break;
                }

                for (passNode = pass->first_node();
                     passNode; passNode = passNode->next_sibling()) {
                    if (strUtil::IsEqualCaseInsen("color", passNode->name())) {
                        // r, g, b, a nodes
                        Color8u col;
                        if (ParsePassCol(passNode, col)) {
                            effectPass.col = col;
                        }
                    }
                    else if (strUtil::IsEqualCaseInsen("pos", passNode->name())) {
                        // x / y nodes.
                        Vec2f pos;
                        if (ParsePassPos(passNode, pos)) {
                            effectPass.offset = pos;
                        }
                    }
                }

                effect.passes.append(effectPass);
            }
        }

        // we support effects with no passes defied.
        // it's the same as a single empty pass.
        if (effect.passes.isEmpty()) {
            effect.passes.emplace_back(FontPass());
        }

        return true;
    }

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

    X_ASSERT(pRequest->getType() == core::IoRequest::OPEN_READ_ALL, "Recived unexpected request type")
    (pRequest->getType());
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
    if (processXmlData(pJobData->pData, pJobData->pData + pJobData->dataSize, sourceName_, effects_, flags_)) {
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

        if (!processXmlData(file->getBufferStart(), file->getBufferEnd(), sourceName_, effects_, flags_)) {
            X_ERROR("Font", "Error processing font def file: \"%s\"", path.c_str());
            return false;
        }

        X_ASSERT(sourceName_.isNotEmpty(), "Source name is empty")
        (sourceName_.isEmpty());
        X_ASSERT(pFontTexture_ == nullptr, "Fonttexture already set")
        (pFontTexture_);

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

bool XFont::processXmlData(const char* pBegin, const char* pEnd, SourceNameStr& sourceNameOut,
    EffetsArr& effectsOut, FontFlags& flags)
{
    ptrdiff_t size = (pEnd - pBegin);
    if (!size) {
        return false;
    }

    core::Array<char> data(g_fontArena, size + 1);
    std::memcpy(data.data(), pBegin, size);
    data.back() = '\0';

    xml_document<> doc; // character type defaults to char
    doc.set_allocator(XmlAllocate, XmlFree);
    doc.parse<0>(data.data()); // 0 means default parse flags

    effectsOut.clear();
    sourceNameOut.clear();
    flags.Clear();

    xml_node<>* node = doc.first_node("font");
    if (node) {
        xml_attribute<>* source = node->first_attribute("source");
        if (source) {
            if (source->value_size() < 128) {
                sourceNameOut.append(source->value());
            }
        }

        xml_attribute<>* proportional = node->first_attribute("proportional");
        if (proportional) {
            // 'true' | '1'
            const char* pPropBegin = proportional->value();
            const char* pPropEnd = proportional->value() + proportional->value_size();

            if (core::strUtil::StringToBool(pPropBegin, pPropEnd) == true) {
                flags.Set(FontFlag::PROPORTIONAL);
            }
        }

        if (!sourceNameOut.isEmpty()) {
            // parse the nodes.
            for (xml_node<>* child = node->first_node();
                 child; child = child->next_sibling()) {
                if (core::strUtil::IsEqualCaseInsen("effect", child->name())) {
                    FontEffect effect;

                    if (ParseEffect(child, effect)) {
                        effectsOut.append(effect);
                    }
                }
                else {
                    // do i care?
                    // just skip them.
                }
            }
        }
        else {
            X_ERROR("Font", "missing source attr from <font> tag");
            return false;
        }
    }
    else {
        return false;
    }

    return true;
}

X_NAMESPACE_END
