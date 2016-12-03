#pragma once


struct IPotatoFactory;
struct XRegFactoryNode;

extern XRegFactoryNode* g_pHeadToRegFactories;


struct XRegFactoryNode
{
	XRegFactoryNode()
	{
	}

	XRegFactoryNode(IPotatoFactory* pFactory) :
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

	IPotatoFactory* pFactory;
	XRegFactoryNode* pNext;
};
