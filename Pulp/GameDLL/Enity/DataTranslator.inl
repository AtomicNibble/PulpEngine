

X_NAMESPACE_BEGIN(game)

namespace entity
{
    inline BaseField::BaseField(const char* pName, core::StrHash hash, FieldType::Enum type, int32_t offset) :
        name(pName),
        nameHash(hash),
        type(type),
        offset(offset)
    {

    }

    // -----------------------------------------------------

    template<typename T>
    inline DataTranslator<T>::Field::Field(const char* pName, core::StrHash hash, TypeCallBack<const char*>&& fn) :
        BaseField(pName, hash, FieldType::String, -1),
        initializer(std::move(fn))
    {

    }

    // -----------------------------------------------------


    template<typename T>
    DataTranslator<T>::DataTranslator(core::MemoryArenaBase* arena) :
        hashes_(arena),
        fields_(arena)
    {
    }

    template<typename T>
    void DataTranslator<T>::add(const char* pName, BoolMember member)
    {
        addInternal(pName, FieldType::Bool, static_cast<int32_t>(X_OFFSETOF(T, *member)));
    }

    template<typename T>
    void DataTranslator<T>::add(const char* pName, IntMember member)
    {
        addInternal(pName, FieldType::Int, static_cast<int32_t>(X_OFFSETOF(T, *member)));
    }

    template<typename T>
    void DataTranslator<T>::add(const char* pName, FloatMember member)
    {
        addInternal(pName, FieldType::Float, static_cast<int32_t>(X_OFFSETOF(T, *member)));
    }

    template<typename T>
    void DataTranslator<T>::add(const char* pName, Vec3Member member)
    {
        addInternal(pName, FieldType::Vec3, static_cast<int32_t>(X_OFFSETOF(T, *member)));
    }

    template<typename T>
    void DataTranslator<T>::add(const char* pName, StringMember member)
    {
        addInternal(pName, FieldType::String, static_cast<int32_t>(X_OFFSETOF(T, *member)));
    }

    template<typename T>
    bool DataTranslator<T>::assignBool(T& out, core::StrHash nameHash, bool value) const
    {
        auto* pField = findField(nameHash);

        if (pField->type != FieldType::Bool) {
            return false;
        }

        auto* pValue = pField->getValuePtr<bool>(out);
        *pValue = value;
        return true;
    }

    template<typename T>
    bool DataTranslator<T>::assignInt(T& out, core::StrHash nameHash, int32_t value) const
    {
        auto* pField = findField(nameHash);

        if (pField->type != FieldType::Int) {
            return false;
        }

        auto* pValue = pField->getValuePtr<int32_t>(out);
        *pValue = value;
        return true;
    }

    template<typename T>
    bool DataTranslator<T>::assignFloat(T& out, core::StrHash nameHash, float value) const
    {
        auto* pField = findField(nameHash);

        if (pField->type != FieldType::Float) {
            return false;
        }

        auto* pValue = pField->getValuePtr<float>(out);
        *pValue = value;
        return true;
    }

    template<typename T>
    bool DataTranslator<T>::assignVec3(T& out, core::StrHash nameHash, Vec3f value) const
    {
        auto* pField = findField(nameHash);

        if (pField->type != FieldType::Vec3) {
            return false;
        }

        auto* pValue = pField->getValuePtr<Vec3f>(out);
        *pValue = value;
        return true;
    }

    template<typename T>
    bool DataTranslator<T>::assignString(T& out, core::StrHash nameHash, const char* pString, size_t length) const
    {
        auto* pField = findField(nameHash);

        if (pField->type != FieldType::String) {
            return false;
        }

        auto* pValue = pField->getValuePtr<core::string>(out);
        pValue->assign(pString, length);
        return true;
    }

    template<typename T>
    const typename DataTranslator<T>::Field* DataTranslator<T>::findField(core::StrHash nameHash) const
    {
        for (size_t i = 0; i < hashes_.size(); i++) {
            if (hashes_[i] == nameHash) {
                X_ASSERT(fields_[i].nameHash == nameHash, "Hash of field not match lookup")();
                return &fields_[i];
            }
        }

        return nullptr;
    }

    template<typename T>
    bool DataTranslator<T>::containsHash(core::StrHash hash) const
    {
        return std::find_if(fields_.begin(), fields_.end(), [hash](const FieldArray::value_type& v) {
            return v.nameHash == hash;
        }) != fields_.end();
    }


} // namespace entity

X_NAMESPACE_END