#pragma once


struct IGoatFactory;
struct XRegFactoryNode;

extern XRegFactoryNode* g_pHeadToRegFactories;


struct XRegFactoryNode
{
	XRegFactoryNode()
	{
	}

	XRegFactoryNode(IGoatFactory* pFactory)
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

	IGoatFactory* m_pFactory;
	XRegFactoryNode* m_pNext;
};
