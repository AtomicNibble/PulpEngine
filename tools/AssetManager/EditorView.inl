
X_NAMESPACE_BEGIN(assman)


X_INLINE bool SplitterOrView::isView(void) const
{
	return pView_ != nullptr;
}

X_INLINE bool SplitterOrView::isSplitter(void) const
{
	return pSplitter_ != nullptr;
}

X_INLINE bool SplitterOrView::isFloated(void) const
{
	return floated_;
}

X_INLINE bool SplitterOrView::isDrag(void) const
{
	return dragging_;
}

X_INLINE IEditor* SplitterOrView::editor(void) const
{
	return pView_ ? pView_->currentEditor() : nullptr;
}

X_INLINE QList<IEditor*> SplitterOrView::editors(void) const
{
	return pView_ ? pView_->editors() : QList<IEditor*>();
}

X_INLINE bool SplitterOrView::hasEditor(IEditor *editor) const
{
	return pView_ && pView_->hasEditor(editor);
}

X_INLINE bool SplitterOrView::hasEditors(void) const
{
	return pView_ && pView_->editorCount() != 0;
}

X_INLINE EditorView* SplitterOrView::view(void) const
{
	return pView_;
}

X_INLINE QSplitter* SplitterOrView::splitter(void) const
{
	return pSplitter_;
}

X_INLINE QSize SplitterOrView::sizeHint(void) const
{
	return minimumSizeHint();
}



X_NAMESPACE_END