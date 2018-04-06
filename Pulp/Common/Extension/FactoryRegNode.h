#pragma once

struct IEngineFactory;
struct XRegFactoryNode;

extern XRegFactoryNode* g_pHeadToRegFactories;

struct XRegFactoryNode
{
    XRegFactoryNode()
        = default;

    XRegFactoryNode(IEngineFactory* pFactory) :
        pFactory(pFactory),
        pNext(g_pHeadToRegFactories)
    {
        g_pHeadToRegFactories = this;
    }

    static void* operator new(size_t, void* p)
    {
        return p;
    }

    static void operator delete(void*, void*)
    {
    }

    IEngineFactory* pFactory;
    XRegFactoryNode* pNext;
};

#define X_FORCE_LINK_FACTORY(className) \
    X_FORCE_SYMBOL_LINK("?s_factory@"##className "@@0V?$XSingletonFactory@V"##className "@@@@A")
