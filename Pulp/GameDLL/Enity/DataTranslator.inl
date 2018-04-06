

X_NAMESPACE_BEGIN(game)

namespace entity
{
    template<typename T>
    DataTranslator<T>::DataTranslator(core::MemoryArenaBase* arena) :
        bools_(arena),
        ints_(arena),
        floats_(arena),
        vecs_(arena),
        strings_(arena)
    {
    }

    template<typename T>
    bool DataTranslator<T>::AssignBool(T& out, core::StrHash nameHash, bool value) const
    {
        return Assign(bools_, out, nameHash, value);
    }

    template<typename T>
    bool DataTranslator<T>::AssignInt(T& out, core::StrHash nameHash, int32_t value) const
    {
        return Assign(ints_, out, nameHash, value);
    }

    template<typename T>
    bool DataTranslator<T>::AssignFloat(T& out, core::StrHash nameHash, float value) const
    {
        return Assign(floats_, out, nameHash, value);
    }

    template<typename T>
    bool DataTranslator<T>::AssignVec3(T& out, core::StrHash nameHash, Vec3f value) const
    {
        return Assign(vecs_, out, nameHash, value);
    }

    template<typename T>
    bool DataTranslator<T>::AssignString(T& out, core::StrHash nameHash, core::string value) const
    {
        return Assign(strings_, out, nameHash, value);
    }

    template<typename T>
    bool DataTranslator<T>::AssignString(T& out, core::StrHash nameHash, const char* pValue) const
    {
        return Assign(strings_, out, nameHash, pValue);
    }

    // -----------------------------------------------------

    template<typename T>
    DataTranslator<T>& DataTranslator<T>::Add(core::StrHash nameHash, BoolMember member)
    {
        X_ASSERT(!ContainsHash(bools_, nameHash), "Hash collision")(nameHash);

        bools_.emplace_back(nameHash, member);
        return *this;
    }

    template<typename T>
    DataTranslator<T>& DataTranslator<T>::Add(core::StrHash nameHash, IntMember member)
    {
        X_ASSERT(!ContainsHash(ints_, nameHash), "Hash collision")(nameHash);

        ints_.emplace_back(nameHash, member);
        return *this;
    }

    template<typename T>
    DataTranslator<T>& DataTranslator<T>::Add(core::StrHash nameHash, FloatMember member)
    {
        X_ASSERT(!ContainsHash(floats_, nameHash), "Hash collision")(nameHash);

        floats_.emplace_back(nameHash, member);
        return *this;
    }

    template<typename T>
    DataTranslator<T>& DataTranslator<T>::Add(core::StrHash nameHash, Vec3Member member)
    {
        X_ASSERT(!ContainsHash(floats_, nameHash), "Hash collision")(nameHash);

        vecs_.emplace_back(nameHash, member);
        return *this;
    }

    template<typename T>
    DataTranslator<T>& DataTranslator<T>::Add(core::StrHash nameHash, StringMember member)
    {
        X_ASSERT(!ContainsHash(strings_, nameHash), "Hash collision")(nameHash);

        strings_.emplace_back(nameHash, member);
        return *this;
    }

    // -----------------------------------------------------

    template<typename T>
    template<typename MemberArrT, typename ValueT>
    inline bool DataTranslator<T>::Assign(MemberArrT& members, T& out, core::StrHash nameHash, ValueT value) const
    {
        for (auto& b : members) {
            if (b.first == nameHash) {
                (out.*b.second) = value;
                return true;
            }
        }
        return false;
    }

    template<typename T>
    template<typename MemberArrT>
    bool DataTranslator<T>::ContainsHash(MemberArrT& members, typename MemberArrT::value_type::first_type hash)
    {
        return std::find_if(members.begin(), members.end(), [hash](const MemberArrT::value_type& v) {
            return v.first == hash;
        }) != members.end();
    }

} // namespace entity

X_NAMESPACE_END