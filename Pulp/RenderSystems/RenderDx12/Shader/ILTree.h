#pragma once


X_NAMESPACE_BEGIN(render)

namespace shader
{


	// input layout tree nodes.
	class ILTreeNode
	{
		static const size_t MAX_IL_NODE_CHILDREN = 4;
		typedef core::Array<ILTreeNode> childVec;

	public:
		ILTreeNode(core::MemoryArenaBase* arena);
		ILTreeNode(core::MemoryArenaBase* arena, const char* Sematic);
		ILTreeNode(const ILTreeNode& oth);
		~ILTreeNode();

		void free(void);

		ILTreeNode& AddChild(ILTreeNode& node, InputLayoutFormat::Enum fmt = InputLayoutFormat::Invalid);
		const ILTreeNode* GetChildWithSemtaic(const char* sematic) const;

		X_INLINE void SetFormat(InputLayoutFormat::Enum fmt);
		X_INLINE bool IsEnd(void) const;
		X_INLINE InputLayoutFormat::Enum GetILFmt(void) const;

	private:
		core::StackString<64> sematicName_;
		InputLayoutFormat::Enum fmt_;
		childVec children_; // never gonna be that many children.
		bool ___pad[8];
	};

	// it's so close to 128 might as well pad it.
	// just cus i like power of 2, these are pretty much read only post creation.
	X_ENSURE_SIZE(ILTreeNode, 128);

} // namespace shader

X_NAMESPACE_END

#include "ILTree.inl"