#pragma once


X_NAMESPACE_BEGIN(game)

namespace entity
{
	#define ADD_TRANS_MEMBER(dt, m) dt.Add(core::StrHash(X_STRINGIZE(m)), &decltype(dt)::Type::m);


	template <class T>
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

		template<typename T>
		using HashPairArray = MulArray<std::pair<core::StrHash, T>>;
		
		typedef HashPairArray<BoolMember> BoolMemberArr;
		typedef HashPairArray<IntMember> IntMemberArr;
		typedef HashPairArray<FloatMember> FloatMemberArr;
		typedef HashPairArray<Vec3Member> Vec3MemberArr;
		typedef HashPairArray<StringMember> StringMemberArr;

	public:
		typedef T Type;

	public:
		DataTranslator(core::MemoryArenaBase* arena);

		bool AssignBool(T& out, core::StrHash nameHash, bool value) const;
		bool AssignInt(T& out, core::StrHash nameHash, int32_t value) const;
		bool AssignFloat(T& out, core::StrHash nameHash, float value) const;
		bool AssignVec3(T& out, core::StrHash nameHash, Vec3f value) const;
		bool AssignString(T& out, core::StrHash nameHash, core::string value) const;
		bool AssignString(T& out, core::StrHash nameHash, const char* pString) const;

		DataTranslator& Add(core::StrHash nameHash, BoolMember member);
		DataTranslator& Add(core::StrHash nameHash, IntMember member);
		DataTranslator& Add(core::StrHash nameHash, FloatMember member);
		DataTranslator& Add(core::StrHash nameHash, Vec3Member member);
		DataTranslator& Add(core::StrHash nameHash, StringMember member);

	private:
		template<typename MemberArrT, typename ValueT>
		inline bool Assign(MemberArrT& members, T& out, core::StrHash nameHash, ValueT value) const;

		template<typename MemberArrT>
		bool ContainsHash(MemberArrT& members, typename MemberArrT::value_type::first_type hash);

	private:
		BoolMemberArr bools_;
		IntMemberArr ints_;
		FloatMemberArr floats_;
		Vec3MemberArr vecs_;
		StringMemberArr strings_;
	};


} // namespace entity

X_NAMESPACE_END

#include "DataTranslator.inl"