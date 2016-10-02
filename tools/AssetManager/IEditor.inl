
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


X_NAMESPACE_END