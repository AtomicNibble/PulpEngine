#pragma once

X_NAMESPACE_BEGIN(net)

class CompTable;

#define IMPLEMENT_META_INTERNAL(className, dataTable, serverClassName)         \
    namespace dataTable                                               \
    {                                                                 \
        extern X_NAMESPACE(net)::CompTable g_RecvTable;                                 \
    }                                                                 \
    X_NAMESPACE(net)::CompTable* className::pMetaTable_ = &dataTable::g_RecvTable; \
    namespace dataTable { \
		struct tag; \
    } \
    template <> int netclassInit<dataTable::tag>(void); \
    namespace dataTable                                               \
    {                                                                 \
        X_NAMESPACE(net)::CompTable g_RecvTable;                      \
        int g_TableInit = netclassInit<dataTable::tag>();                                     \
    }                                                               \
    template <> int netclassInit<dataTable::tag>()                                                    \
    {                                                             \
        typedef className currentClass;                           \
        const char* pTableName = #serverClassName;                \
                                                                  \
        auto& table = dataTable::g_RecvTable;                                \
                                                                  \
        static X_NAMESPACE(net)::CompProp CompProps[] = {                           \
            X_NAMESPACE(net)::CompPropInt("__empty__", 0, sizeof(int)),


#define IMPLEMENT_META(className)         \
        IMPLEMENT_META_INTERNAL(className, className##Meta , className##Table)         

#define END_META()                                                                                  \
            };                                                                                      \
                                                                                                    \
            table.set(core::make_span(CompProps + 1, X_ARRAY_SIZE(CompProps) - 1), pTableName);     \
            return 1;                                                                               \
        }                                                                                           
   


#define FIELD_SIZE(classname, field) sizeof(((classname*)0)->field)
#define ADD_FIELD(field) #field, X_OFFSETOF(currentClass, field), FIELD_SIZE(currentClass, field)
#define ADD_FIELD_ARRAY(field) ADD_FIELD(field), FIELD_SIZE(currentClass, field) / FIELD_SIZE(currentClass, field[0]), FIELD_SIZE(currentClass, field[0])


#define ADD_META() static X_NAMESPACE(net)::CompTable* pMetaTable_; \
                    template <typename T> friend int netclassInit(void);


X_NAMESPACE_END