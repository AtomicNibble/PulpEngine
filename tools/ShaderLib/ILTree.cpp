#include "stdafx.h"
#include "ILTree.h"

X_NAMESPACE_BEGIN(render)

namespace shader
{

	namespace
	{
		core::CriticalSection cs;
	}


	ILTreeNode::ILTreeNode() 
	{
		fmt_ = InputLayoutFormat::Invalid;
	}


	ILTreeNode::ILTreeNode(const char* pSematic)
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

	const ILTreeNode* ILTreeNode::getILTree(void)
	{
		core::CriticalSection::ScopedLock lock(cs);

		// all the posible node types.
		static ILTreeNode blank;
		static ILTreeNode pos("POSITION");
		static ILTreeNode uv("TEXCOORD");
		static ILTreeNode col("COLOR");
		static ILTreeNode nor("NORMAL");
		static ILTreeNode tan("TANGENT");
		static ILTreeNode bin("BINORMAL");

		static ILTreeNode uv2("TEXCOORD");
		static ILTreeNode col2("COLOR");
		static ILTreeNode nor2("NORMAL");

		// for shader input layouts the format is not given since the shader
		// don't care what the format comes in as.
		// so how can i work out what the formats are since i support identical sematic layouts
		// with diffrent foramts :(
		//
		// maybe i should just have a sematic format, which can be used to tell if the current input
		// layout will work with the shader :)
		//
		//        .
		//        |
		//       P3F_____
		//       / \     \
		//     T2S  T4F  T3F
		//      |    |__
		//     C4B	    |
		//	  __|	   C4B 
		//	 /  |       |
		// N3F N10	   N3F
		//  |    \
		// TB3F  TB10
		//

		static bool isInit = false;

		if (isInit)
		{
			return &blank;
		}
		else
		{
			isInit = true;

			blank.SetFormat(InputLayoutFormat::NONE);

			ILTreeNode& uvBase = blank.AddChild(pos).AddChild(uv, InputLayoutFormat::POS_UV);
			uvBase.AddChild(col, InputLayoutFormat::POS_UV_COL).
				AddChild(nor, InputLayoutFormat::POS_UV_COL_NORM).
				AddChild(tan, InputLayoutFormat::POS_UV_COL_NORM_TAN).
				AddChild(bin, InputLayoutFormat::POS_UV_COL_NORM_TAN_BI);

			// double text coords.
			uvBase.AddChild(uv2).
				AddChild(col2).
				AddChild(nor2, InputLayoutFormat::POS_UV2_COL_NORM);

			return &blank;
		}
	}

	void ILTreeNode::free(void)
	{
		children_.clear();
	}


	ILTreeNode& ILTreeNode::AddChild(ILTreeNode& node, InputLayoutFormat::Enum fmt)
	{
		if (children_.size() < MAX_IL_NODE_CHILDREN)
		{
			children_.append(&node);
			ILTreeNode& retnode = *children_[children_.size() - 1];
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

		for (const auto& c : children_)
		{
			if (sematicStrLen < c->sematicName_.length()) {
				continue;
			}

			if (core::strUtil::IsEqualCaseInsen(c->sematicName_.begin(),
				c->sematicName_.end(), pSematic, pSematic + c->sematicName_.length())) {
				return c;
			}
		}

		return nullptr;
	}




} // namespace shader

X_NAMESPACE_END