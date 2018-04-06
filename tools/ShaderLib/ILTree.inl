
X_NAMESPACE_BEGIN(render)

namespace shader
{
    X_INLINE void ILTreeNode::SetFormat(InputLayoutFormat::Enum fmt)
    {
        this->fmt_ = fmt;
    }

    X_INLINE bool ILTreeNode::IsEnd(void) const
    {
        return this->fmt_ == InputLayoutFormat::Invalid;
    }

    X_INLINE InputLayoutFormat::Enum ILTreeNode::GetILFmt(void) const
    {
        return fmt_;
    }

} // namespace shader

X_NAMESPACE_END