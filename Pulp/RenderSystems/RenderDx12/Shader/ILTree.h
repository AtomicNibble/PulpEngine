#pragma once


X_NAMESPACE_BEGIN(render)

namespace shader
{



	// input layout tree nodes.
	struct ILTreeNode
	{
		static const size_t MAX_IL_NODE_CHILDREN = 4;
		typedef core::Array<ILTreeNode> childVec;

		ILTreeNode() : children_(g_rendererArena) {
			this->fmt_ = InputLayoutFormat::Invalid;
		}
		ILTreeNode(const ILTreeNode& oth) : children_(g_rendererArena) {
			this->children_ = oth.children_;
			this->SematicName_ = oth.SematicName_;
			this->fmt_ = oth.fmt_;
		}
		ILTreeNode(const char* Sematic) : children_(g_rendererArena) {
			this->SematicName_.set(Sematic);
			this->fmt_ = InputLayoutFormat::Invalid;
		}

		~ILTreeNode() {
			free();
		}

		void free(void) {
			children_.free();
		}

		ILTreeNode& AddChild(ILTreeNode& node,
			InputLayoutFormat::Enum fmt = InputLayoutFormat::Invalid)
		{
			if (children_.size() < MAX_IL_NODE_CHILDREN)
			{
				children_.append(node);
				ILTreeNode& retnode = children_[children_.size() - 1];
				retnode.fmt_ = fmt;
				return retnode;
			}
			X_ERROR("Shader", "ILTree exceeded max children. max: %i", MAX_IL_NODE_CHILDREN);
			static ILTreeNode s_node;
			return s_node;
		}

		const ILTreeNode* GetChildWithSemtaic(const char* sematic) const {
			// we want to skip SEMTA0 SEMTA1 SEMTA2 etc so we only compare the length.
			childVec::ConstIterator it;
			for (it = children_.begin(); it != children_.end(); ++it)
			{
				if (core::strUtil::IsEqualCaseInsen(it->SematicName_.begin(),
					it->SematicName_.end(), sematic, sematic + it->SematicName_.length()))
				{
					return it;
				}
			}
			return nullptr;
		}

		X_INLINE void SetFormat(InputLayoutFormat::Enum fmt) {
			this->fmt_ = fmt;
		}

		X_INLINE bool IsEnd(void) const {
			return this->fmt_ == InputLayoutFormat::Invalid;
		}

		X_INLINE InputLayoutFormat::Enum GetILFmt(void) const {
			return fmt_;
		}

	private:
		core::StackString<64> SematicName_;
		InputLayoutFormat::Enum fmt_;
		childVec children_; // never gonna be that many children.
		bool ___pad[3];
	};




} // namespace shader

X_NAMESPACE_END