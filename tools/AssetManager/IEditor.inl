
X_NAMESPACE_BEGIN(assman)


X_INLINE bool IEditor::duplicateSupported(void) const
{
	return false;
}

X_INLINE IEditor* IEditor::duplicate(void)
{
	return nullptr;
}

X_INLINE QByteArray IEditor::saveState(void) const
{
	return QByteArray();
}

X_INLINE bool IEditor::restoreState(const QByteArray&)
{
	return true;
}

X_INLINE int32_t IEditor::currentLine(void) const
{
	return 0;
}
X_INLINE int32_t IEditor::currentColumn(void) const
{
	return 0;
}

X_INLINE void IEditor::gotoLine(int32_t line, int32_t column)
{
	Q_UNUSED(line);
	Q_UNUSED(column);
}



X_NAMESPACE_END