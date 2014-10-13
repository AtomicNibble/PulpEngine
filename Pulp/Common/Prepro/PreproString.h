#pragma once
#ifndef X_PREPRO_STRING_H
#define X_PREPRO_STRING_H


#define X_STRINGIZE_HELPER(token)					#token
#define X_STRINGIZE(str)							X_STRINGIZE_HELPER(str)

#endif // X_PREPRO_STRING_H
