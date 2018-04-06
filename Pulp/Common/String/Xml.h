#pragma once

#ifndef X_STRING_XML_H_
#define X_STRING_XML_H_

X_NAMESPACE_BEGIN(core)

namespace xml
{
#define RAPIDXML_NO_EXCEPTIONS

#include <../../3rdparty/source/rapidxml/rapidxml.hpp>

    X_DISABLE_WARNING(4100)
#include <../../3rdparty/source/rapidxml/rapidxml_print.hpp>
    X_ENABLE_WARNING(4100)
} // namespace xml

X_NAMESPACE_END

#endif // !X_STRING_XML_H_