#pragma once

// THis has no includes as used by resource file also.

#define X_VERSION_PP_STRINGIZE_HELPER(token) #token
#define X_VERSION_PP_STRINGIZE(str) X_VERSION_PP_STRINGIZE_HELPER(str)


#define X_ENGINE_NAME "Blade"
#define X_ENGINE_OUTPUT_PREFIX "engine_"
#define X_ENGINE_VERSION_MAJOR 0
#define X_ENGINE_VERSION_MINOR 1
#define X_ENGINE_VERSION_PATCH 0
#define X_ENGINE_VERSION_STR \
    X_VERSION_PP_STRINGIZE(X_ENGINE_VERSION_MAJOR) "." \
    X_VERSION_PP_STRINGIZE(X_ENGINE_VERSION_MINOR)  "." \
    X_VERSION_PP_STRINGIZE(X_ENGINE_VERSION_PATCH) 



#define X_VER_COPYRIGHT_STR       "Copyright Tom Crowley (C) 2018 - 2019"
#define X_VER_TRADEMARK_STR       ""
#define X_VER_COMPANY_STR         ""
