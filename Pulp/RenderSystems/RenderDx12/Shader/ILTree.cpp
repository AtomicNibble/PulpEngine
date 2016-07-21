#include "stdafx.h"
#include "ILTree.h"

X_NAMESPACE_BEGIN(render)

namespace shader
{


	ILTreeNode::ILTreeNode(core::MemoryArenaBase* arena) :
		children_(arena)
	{
		fmt_ = InputLayoutFormat::Invalid;
	}


	ILTreeNode::ILTreeNode(core::MemoryArenaBase* arena, const char* pSematic) :
		children_(arena)
	{
		sematicName_.set(pSematic);
		fmt_ = InputLayoutFormat::Invalid;
	}

	ILTreeNode::ILTreeNode(const ILTreeNode& oth) :
		children_(oth.children_)
	{
		sematicName_ = oth.sematicName_;
		fmt_ = oth.fmt_;
	}

	ILTreeNode::~ILTreeNode() 
	{
		free();
	}

	void ILTreeNode::free(void) 
	{
		children_.free();
	}


	ILTreeNode& ILTreeNode::AddChild(ILTreeNode& node, InputLayoutFormat::Enum fmt)
	{
		if (children_.size() < MAX_IL_NODE_CHILDREN)
		{
			children_.append(node);
			ILTreeNode& retnode = children_[children_.size() - 1];
			retnode.fmt_ = fmt;
			return retnode;
		}
		X_FATAL("Shader", "ILTree exceeded max children. max: %i", MAX_IL_NODE_CHILDREN);
		static ILTreeNode s_node(nullptr);
		return s_node;
	}

	const ILTreeNode* ILTreeNode::GetChildWithSemtaic(const char* pSematic) const
	{
		// we want to skip SEMTA0 SEMTA1 SEMTA2 etc so we only compare the length.
		const size_t sematicStrLen = core::strUtil::strlen(pSematic);

		childVec::ConstIterator it;
		for (it = children_.begin(); it != children_.end(); ++it)
		{
			if (sematicStrLen < it->sematicName_.length()) {
				continue;
			}

			if (core::strUtil::IsEqualCaseInsen(it->sematicName_.begin(),
				it->sematicName_.end(), pSematic, pSematic + it->sematicName_.length())) {
				return it;
			}
		}
		return nullptr;
	}




} // namespace shader

X_NAMESPACE_END