

X_NAMESPACE_BEGIN(script)



X_INLINE ScriptValue::ScriptValue(bool value) : 
	type_(Type::BOOLEAN) 
{ 
	bool_ = value; 
}

X_INLINE ScriptValue::ScriptValue(int32_t value) :
	type_(Type::NUMBER) 
{ 
	number_ = static_cast<double>(value); 
}

X_INLINE ScriptValue::ScriptValue(uint32_t value) : 
	type_(Type::NUMBER) 
{ 
	number_ = static_cast<double>(value);
}

X_INLINE ScriptValue::ScriptValue(float value) : 
	type_(Type::NUMBER) 
{
	number_ = value; 
}

X_INLINE ScriptValue::ScriptValue(double value) :
	type_(Type::NUMBER)
{
	number_ = value;
}

X_INLINE ScriptValue::ScriptValue(const char* pValue) :
	type_(Type::STRING) 
{ 
	str_.pStr = pValue;
	str_.len = safe_static_cast<int32_t>(::strlen(pValue));
}

X_INLINE ScriptValue::ScriptValue(IScriptTable* table) :
	type_(Type::TABLE)
{
	pTable_ = table;
	if (pTable_) {
		pTable_->addRef();
	}
}

X_INLINE ScriptValue::ScriptValue(ScriptFunctionHandle function) :
	type_(Type::FUNCTION) 
{ 
	pFunction_ = function; 
}

X_INLINE ScriptValue::ScriptValue(ScriptHandle value) : 
	type_(Type::HANDLE) 
{ 
	pPtr_ = value.pPtr;
}

X_INLINE ScriptValue::ScriptValue(const Vec3f& vec) : 
	type_(Type::VECTOR) 
{ 
	vec3_.x = vec.x; 
	vec3_.y = vec.y; 
	vec3_.z = vec.z; 
}

X_INLINE ScriptValue::ScriptValue(const SmartScriptTable& value) :
	type_(Type::TABLE)
{
	pTable_ = value;
	if (pTable_) {
		pTable_->addRef();
	}
}


X_INLINE ScriptValue::ScriptValue(Type::Enum type) :
	type_(type)
{

}

X_INLINE ScriptValue::ScriptValue() :
	type_(Type::NONE)
{
}


X_INLINE ScriptValue::~ScriptValue()
{
}


X_INLINE void ScriptValue::clear(void)
{
	type_ = Type::NONE;
}

X_INLINE Type::Enum ScriptValue::getType(void) const
{
	return type_;
}


X_INLINE ScriptValue& ScriptValue::operator=(const ScriptValue& rhs)
{
	memcpy(this, &rhs, sizeof(ScriptValue));
	return *this;
}

X_INLINE bool ScriptValue::operator==(const ScriptValue& rhs) const
{
	bool result = type_ == rhs.type_;
	if (result)
	{
		switch (type_)
		{
			case Type::BOOLEAN:
				result = bool_ == rhs.bool_;
				break;
			case Type::NUMBER:
				result = number_ == rhs.number_; 
				break;
			case Type::STRING:
				result = (str_.len == rhs.str_.len && str_.pStr == rhs.str_.pStr);
				break;
			case Type::VECTOR:
				result = vec3_.x == rhs.vec3_.x && vec3_.y == rhs.vec3_.y && vec3_.z == rhs.vec3_.z;
				break;
			case Type::TABLE:
				result = pTable_ == rhs.pTable_; 
				break;
			case Type::HANDLE:
				result = pPtr_ == rhs.pPtr_;
				break;
			case Type::FUNCTION:
				result = gEnv->pScriptSys->compareFuncRef(pFunction_, rhs.pFunction_); 
				break;
				//		case Type::USERDATA: result = ud.nRef == rhs.ud.nRef && ud.ptr == rhs.ud.ptr; break;

			default:
				X_ASSERT_NOT_IMPLEMENTED();
				break;
		}
	}
	return result;
}

X_INLINE bool ScriptValue::operator!=(const ScriptValue& rhs) const
{
	return !(*this == rhs);
}

X_INLINE void ScriptValue::swap(ScriptValue& value)
{
	core::Swap(*this, value);
}


X_INLINE bool ScriptValue::CopyTo(bool& value) const
{
	if (type_ == Type::BOOLEAN) {
		value = bool_;
		return true;
	}
	return false;
}

X_INLINE bool ScriptValue::CopyTo(int32_t& value) const
{
	if (type_ == Type::NUMBER) {
		value = static_cast<int32_t>(number_);
		return true;
	}
	return false;
}

X_INLINE bool ScriptValue::CopyTo(uint32_t& value) const
{
	if (type_ == Type::NUMBER) {
		value = static_cast<uint32_t>(number_);
		return true;
	}
	return false;
}

X_INLINE bool ScriptValue::CopyTo(float& value) const
{
	if (type_ == Type::NUMBER) {
		value = static_cast<float>(number_);
		return true;
	}
	return false;
}

X_INLINE bool ScriptValue::CopyTo(const char* &value) const
{
	if (type_ == Type::STRING) {
		value = str_.pStr;
		return true;
	}
	return false;
}

X_INLINE bool ScriptValue::CopyTo(char* &value) const
{ 
	if (type_ == Type::STRING) { 
		value = (char*)str_.pStr; 
		return true; 
	} 
	return false; 
}

X_INLINE bool ScriptValue::CopyTo(ScriptHandle &value) const 
{ 
	if (type_ == Type::HANDLE) { 
		value.pPtr = const_cast<void*>(pPtr_); 
		return true;
	}
	return false; 
}

X_INLINE bool ScriptValue::CopyTo(ScriptFunctionHandle &value) const
{
	X_UNUSED(value);
	X_ASSERT_NOT_IMPLEMENTED();
}

X_INLINE bool ScriptValue::CopyTo(Vec3f& value) const 
{ 
	if (type_ == Type::VECTOR) { 
		value.x = vec3_.x; 
		value.y = vec3_.y; 
		value.z = vec3_.z; 
		return true; 
	}
	return false; 
}

X_INLINE bool ScriptValue::CopyTo(IScriptTable*& value) const
{
	if (type_ == Type::TABLE) {
		value = pTable_;
		return true;
	}
	return false;
}

X_INLINE bool ScriptValue::CopyTo(SmartScriptTable& value) const
{
	if (type_ == Type::TABLE) {
		value = pTable_;
		return true;
	}
	return false;
}


// ------------------------------------------------------------------------


template <class T>
X_INLINE void IScriptSys::setGlobalValue(const char* pKey, const T& value)
{
	setGlobalValue(pKey, ScriptValue(value));
}

X_INLINE void IScriptSys::setGlobalToNull(const char* pKey)
{
	setGlobalValue(pKey, ScriptValue(Type::NIL));
}

template <class T>
X_INLINE bool IScriptSys::getGlobalValue(const char* pKey, T& value)
{
	ScriptValue any(ValueType<T>::Type);
	return getGlobalValue(pKey, any) && any.CopyTo(value);
}



X_NAMESPACE_END
