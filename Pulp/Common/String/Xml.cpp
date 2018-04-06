#include "EngineCommon.h"
#include "Xml.h"

X_NAMESPACE_BEGIN(core)

namespace xml
{
    namespace rapidxml
    {
        void parse_error_handler(const char* what, void* where)
        {
            X_UNUSED(where); // Pointer to character data where error was detected.
            X_ERROR("Xml", "Parse Error: %s", what);
        }
    } // namespace rapidxml
} // namespace xml

X_NAMESPACE_END