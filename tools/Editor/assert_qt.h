#ifndef QT_ASSERT_H
#define QT_ASSERT_H


namespace Internal { void writeAssertLocation(const char *msg); }

#define BUG_ASSERT_STRINGIFY_HELPER(x) #x
#define BUG_ASSERT_STRINGIFY(x) BUG_ASSERT_STRINGIFY_HELPER(x)
#define BUG_ASSERT_STRING(cond) ::Internal::writeAssertLocation(\
    "\"" cond"\" in file " __FILE__ ", line " BUG_ASSERT_STRINGIFY(__LINE__))

#define BUG_ASSERT_NOT_IMPLEMENTED() ::Internal::writeAssertLocation(\
    "Not implemented! file " __FILE__ ", line " BUG_ASSERT_STRINGIFY(__LINE__))


#define BUG_ASSERT(cond, action) if (cond) {} else { BUG_ASSERT_STRING(#cond); action; } do {} while (0)
#define BUG_CHECK(cond) if (cond) {} else { BUG_ASSERT_STRING(#cond); } do {} while (0)


#endif // QT_ASSERT_H
