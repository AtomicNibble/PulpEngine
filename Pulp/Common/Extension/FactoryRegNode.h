#pragma once


struct IPotatoFactory;
struct XRegFactoryNode;

extern XRegFactoryNode* g_pHeadToRegFactories;


struct XRegFactoryNode
{
	XRegFactoryNode()
	{
	}

	XRegFactoryNode(IPotatoFactory* pFactory)
		: m_pFactory(pFactory)
		, m_pNext(g_pHeadToRegFactories)
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

	IPotatoFactory* m_pFactory;
	XRegFactoryNode* m_pNext;
};
