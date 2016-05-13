#ifndef ASSERT_H
#define ASSERT_H


namespace Utils { void writeAssertLocation(const char *msg); }

#define BUG_ASSERT_STRINGIFY_HELPER(x) #x
#define BUG_ASSERT_STRINGIFY(x) BUG_ASSERT_STRINGIFY_HELPER(x)
#define BUG_ASSERT_STRING(cond) ::Utils::writeAssertLocation(\
    "\"" cond"\" in file " __FILE__ ", line " BUG_ASSERT_STRINGIFY(__LINE__))


#define BUG_ASSERT(cond, action) if (cond) {} else { BUG_ASSERT_STRING(#cond); action; } do {} while (0)
#define BUG_CHECK(cond) if (cond) {} else { BUG_ASSERT_STRING(#cond); } do {} while (0)


#endif // ASSERT_H
