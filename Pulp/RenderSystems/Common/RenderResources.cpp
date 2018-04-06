#include "stdafx.h"
#include "XRender.h"

#include <ITexture.h>

#include <../Common/Textures/XTexture.h>

#include <IFileSys.h>

#include <Memory\AllocationPolicies\MallocFreeAllocator.h>

X_NAMESPACE_BEGIN(render)

using namespace texture;

namespace
{
    void* XmlAllocate(std::size_t size)
    {
        return X_NEW_ARRAY(char, size, g_rendererArena, "xmlPool");
    }

    void XmlFree(void* pointer)
    {
        char* pChar = (char*)pointer;
        X_DELETE_ARRAY(pChar, g_rendererArena);
    }
} // namespace

bool ParseScaleValue(const char* attr, float& scale)
{
    if (core::strUtil::IsNumeric(attr)) {
        float value = core::strUtil::StringToFloat<float>(attr);
        if (value >= 0.f && value <= 1.f) {
            scale = value;
            return true;
        }
    }
    X_ERROR("Render", "scale value is not a valid float beetween: 0 - 1");
    return false;
}

bool XRender::LoadResourceDeffintion(void)
{
    using namespace core;
    using namespace xml::rapidxml;

    // load the resources shiz.
    core::XFileScoped file;
    size_t length;

    core::Path<char> path("shaders/render.setup");

    RenderResource res;

    if (file.openFile(path.c_str(), core::fileMode::READ)) {
        length = safe_static_cast<size_t, uint64_t>(file.remainingBytes());

        char* pText = X_NEW_ARRAY_ALIGNED(char, length + 1, g_rendererArena, "RenderXMLBuf", 4);

        pText[length] = '\0';

        if (file.read(pText, length) == length) {
            xml_document<> doc; // character type defaults to char
            doc.set_allocator(XmlAllocate, XmlFree);
            doc.parse<0>(pText); // 0 means default parse flags

            xml_node<>* node = doc.first_node("resources");
            if (node) {
                // parse the nodes.
                for (xml_node<>* child = node->first_node();
                     child; child = child->next_sibling()) {
                    // name, foramt, scale_x, scale_Y
                    int num = 0;

                    xml_attribute<>* attr;
                    for (attr = child->first_attribute();
                         attr; attr = attr->next_attribute()) {
                        if (strUtil::IsEqualCaseInsen("name", attr->name())) {
                            if (attr->value_size() <= 28) {
                                res.name.append(attr->value());
                                num++;
                            }
                            else
                                X_ERROR("Render", "render resource name is longer than 28: %s", attr->name());
                        }
                        else if (strUtil::IsEqualCaseInsen("format", attr->name())) {
                            res.fmt = texture::Util::TexFmtFromStr(attr->value());
                            if (res.fmt == texture::Texturefmt::UNKNOWN) {
                                X_ERROR("Render", "render resource format is not valid: %s", attr->value());
                            }
                            else {
                                num++;
                            }
                        }
                        else if (strUtil::IsEqualCaseInsen("scale_x", attr->name())) {
                            if (ParseScaleValue(attr->value(), res.scale.x))
                                num++;
                        }
                        else if (strUtil::IsEqualCaseInsen("scale_y", attr->name())) {
                            if (ParseScaleValue(attr->value(), res.scale.y))
                                num++;
                        }
                    }

                    if (num == 4)
                        RenderResources_.append(res);
                    else {
                        X_ERROR("Render", "render resource deffitnition is incomplete");
                    }
                }
            }
        }

        X_DELETE_ARRAY(pText, g_rendererArena);

        return true;
    }

    return false;
}

X_NAMESPACE_END
