#pragma once

#include <Util\Function.h>

X_NAMESPACE_BEGIN(game)

namespace entity
{
    #define ADD_TRANS_MEMBER(dt, m) dt.add(X_STRINGIZE(m), &decltype(dt)::Type::m);

    X_DECLARE_ENUM(FieldType)(
        Bool,
        Int,
        Float,
        String,
        Vec3
    );

    struct BaseField
    {
        BaseField(const char* pName, core::StrHash hash, FieldType::Enum, int32_t offset);

        core::string name;
        core::StrHash nameHash;
        FieldType::Enum type;

        int32_t offset;
    };

    template<class T>
    class DataTranslator
    {
        // single members
        typedef bool T::*BoolMember;
        typedef int T::*IntMember;
        typedef float T::*FloatMember;
        typedef Vec3f T::*Vec3Member;
        typedef core::string T::*StringMember;

        template<typename T>
        using MulArray = core::Array<T, core::ArrayAllocator<T>, core::growStrat::FixedLinear<4>>;

        template<class... Args>
        using TypeCallBack = core::Function<bool(T&, Args...), 32>;

        using StringTypeCallBack = TypeCallBack<const char*, size_t>;

        struct Field : public BaseField
        {
            using BaseField::BaseField;

            Field(const char* pName, core::StrHash hash, StringTypeCallBack&& fn);

            template<typename T2>
            T2* getValuePtr(T& instance) const {
                return reinterpret_cast<T2*>(reinterpret_cast<char*>(&instance) + offset);
            }

            mutable StringTypeCallBack initializer;
        };

        using FieldArray = MulArray<Field>;
        using StrHashArr = MulArray<core::StrHash>;

    public:
        typedef T Type;

    public:
        DataTranslator(core::MemoryArenaBase* arena);

        bool assignBool(T& out, core::StrHash nameHash, bool value) const;
        bool assignInt(T& out, core::StrHash nameHash, int32_t value) const;
        bool assignFloat(T& out, core::StrHash nameHash, float value) const;
        bool assignVec3(T& out, core::StrHash nameHash, Vec3f value) const;
        bool assignString(T& out, core::StrHash nameHash, const char* pString, size_t length) const;

        void add(const char* pName, BoolMember member);
        void add(const char* pName, IntMember member);
        void add(const char* pName, FloatMember member);
        void add(const char* pName, Vec3Member member);
        void add(const char* pName, StringMember member);

        void initializeFromString(const char* pName, StringTypeCallBack fn);

    private:
        void addInternal(const char* pName, FieldType::Enum type, int32_t offset);
        const Field* findField(core::StrHash nameHash) const;

        bool containsHash(core::StrHash hash) const;

    private:
        StrHashArr hashes_; // used as lookup, as 16 hashes fit in single cache line :D
        FieldArray fields_;
    };

    template<typename T>
    void DataTranslator<T>::addInternal(const char* pName, FieldType::Enum type, int32_t offset)
    {
        core::StrHash hash(pName);

        X_ASSERT(!containsHash(hash), "Hash collision")(pName);

        fields_.emplace_back(pName, hash, type, offset);
        hashes_.push_back(hash);
    }


    template<typename T>
    void DataTranslator<T>::initializeFromString(const char* pName, StringTypeCallBack fn)
    {
        // so we will have a function that has diffrent return type :(
        // can i generalize this slut?
        core::StrHash hash(pName);
        
        fields_.emplace_back(pName, hash, std::move(fn));
        hashes_.push_back(hash);
    }

} // namespace entity

X_NAMESPACE_END

#include "DataTranslator.inl"