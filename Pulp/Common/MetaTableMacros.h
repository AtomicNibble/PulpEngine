#pragma once

X_NAMESPACE_BEGIN(net)

class CompTable;

#define IMPLEMENT_META(className, dataTable, serverClassName)         \
    namespace dataTable                                               \
    {                                                                 \
        extern X_NAMESPACE(net)::CompTable g_RecvTable;                                 \
    }                                                                 \
    X_NAMESPACE(net)::CompTable* className::pMetaTable_ = &dataTable::g_RecvTable; \
    namespace dataTable                                               \
    {                                                                 \
        X_NAMESPACE(net)::CompTable g_RecvTable;                                        \
                                                                      \
        int init()                                                    \
        {                                                             \
            typedef className currentClass;                           \
            const char* pTableName = #serverClassName;                \
                                                                      \
            auto& table = g_RecvTable;                                \
                                                                      \
            static X_NAMESPACE(net)::CompProp CompProps[] = {                           \
                X_NAMESPACE(net)::CompPropInt("__empty__", 0, sizeof(int)),

#define ADD_FIELD(field) #field, X_OFFSETOF(currentClass, field), sizeof(((currentClass*)0)->field)

#define END_META()                                                                                  \
            };                                                                                      \
                                                                                                    \
            table.set(core::make_span(CompProps + 1, X_ARRAY_SIZE(CompProps) - 1), pTableName);     \
            return 1;                                                                               \
        }                                                                                           \
        int g_TableInit = init();                                                                   \
    }


#define ADD_META() static X_NAMESPACE(net)::CompTable* pMetaTable_; 


X_NAMESPACE_END