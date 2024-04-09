#pragma once

#ifndef X_PREPROCESSORSTRINGIZE_H
#define X_PREPROCESSORSTRINGIZE_H

#define X_PP_STRINGIZE_HELPER(token) #token
#define X_PP_STRINGIZE(str) X_PP_STRINGIZE_HELPER(str)

#endif
