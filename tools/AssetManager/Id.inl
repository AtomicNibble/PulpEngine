
X_NAMESPACE_BEGIN(assman)


Id::Id() : 
	id_(0)
{
}

Id::Id(const Id& oth) :
	id_(oth.id_)
{

}

Id::Id(int32_t uid) : 
	id_(uid)
{
}

X_INLINE bool Id::isValid(void) const 
{ 
	return id_; 
}

X_INLINE bool Id::operator==(Id id) const
{ 
	return id_ == id.id_; 
}

X_INLINE bool Id::operator!=(Id id) const
{ 
	return id_ != id.id_; 
}

X_INLINE bool Id::operator!=(const char* pName) const
{ 
	return !operator==(pName);
}

X_INLINE bool Id::operator<(Id id) const 
{ 
	return id_ < id.id_;
}

X_INLINE bool Id::operator>(Id id) const 
{ 
	return id_ > id.id_; 
}

X_INLINE int32_t Id::uniqueIdentifier(void) const 
{ 
	return id_; 
}

static X_INLINE Id fromUniqueIdentifier(int32_t uid) 
{ 
	return Id(uid);
}


X_NAMESPACE_END
