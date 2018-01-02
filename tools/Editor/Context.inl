
X_NAMESPACE_BEGIN(assman)


X_INLINE Context::Context(Id c1)
{
	add(c1);
}

X_INLINE Context::Context(Id c1, Id c2)
{
	add(c1); add(c2);
}

X_INLINE Context::Context(Id c1, Id c2, Id c3)
{
	add(c1); add(c2); add(c3);
}

X_INLINE bool Context::contains(Id c) const
{
	return ids_.contains(c);
}

X_INLINE int32_t Context::size(void) const
{
	return ids_.size();
}

X_INLINE bool Context::isEmpty(void) const
{
	return ids_.isEmpty();
}

X_INLINE Id Context::at(int32_t idx) const
{
	return ids_.at(idx);
}

X_INLINE int32_t Context::indexOf(Id c) const
{
	return ids_.indexOf(c);
}

X_INLINE void Context::removeAt(int32_t idx)
{
	ids_.removeAt(idx);
}

X_INLINE void Context::prepend(Id c)
{
	ids_.prepend(c);
}

X_INLINE void Context::add(const Context& c)
{
	ids_ += c.ids_;
}

X_INLINE void Context::add(Id c)
{
	ids_.append(c);
}

X_INLINE bool Context::operator==(const Context& c) const
{
	return ids_ == c.ids_;
}

X_INLINE Context::const_iterator Context::begin(void) const
{
	return ids_.begin();
}

X_INLINE Context::const_iterator Context::end(void) const
{
	return ids_.end();
}


// -------------------------------------------------------

X_INLINE IContext::IContext(QObject* pParent) :
	QObject(pParent)
{

}


X_INLINE Context IContext::context(void) const 
{ 
	return context_; 
}

X_INLINE QWidget* IContext::widget(void) const 
{ 
	return widget_; 
}

X_INLINE void IContext::setContext(const Context& context) 
{ 
	context_ = context; 
}

X_INLINE void IContext::setWidget(QWidget* pWidget) 
{ 
	widget_ = pWidget; 
}

X_NAMESPACE_END
