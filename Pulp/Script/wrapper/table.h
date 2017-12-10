#pragma once


X_NAMESPACE_BEGIN(script)

namespace lua
{
	class BasicReference
	{
		BasicReference() = default;
		BasicReference(lua_nil_t);
		BasicReference(lua_State* L, int index = -1);
		BasicReference(lua_State* L, ref_index index);
		BasicReference(lua_State* L, lua_nil_t);
		~BasicReference();


		X_INLINE explicit operator bool() const;

		X_INLINE bool valid(void) const;
		X_INLINE Type::Enum getType(void) const;
		X_INLINE lua_State* luaState(void) const;

		X_INLINE int push(void) const;
		X_INLINE int push(lua_State* Ls) const;
		X_INLINE void pop(void) const;
		X_INLINE void pop(lua_State* Ls, int n = 1) const;

		X_INLINE int registryIndex(void) const;



	private:
		lua_State* luastate_;
		int32_t ref_;
	};


	class BasicTable
	{
	public:

#if 0
		template <typename Sig, typename Key, typename... Args>
		void setFunction(Key&& key, Args&&... args) {
			set_fx(types<Sig>(), std::forward<Key>(key), std::forward<Args>(args)...);
		}

		template <typename Key, typename... Args>
		void setFunction(Key&& key, Args&&... args) {
			set_fx(types<>(), std::forward<Key>(key), std::forward<Args>(args)...);
		}

	private:

		template <typename R, typename... Args, typename Fx, typename Key, typename = std::result_of_t<Fx(Args...)>>
		void set_fx(types<R(Args...)>, Key&& key, Fx&& fx) {
			set_resolved_function<R(Args...)>(std::forward<Key>(key), std::forward<Fx>(fx));
		}

		template <typename Fx, typename Key, meta::enable<meta::is_specialization_of<overload_set, meta::unqualified_t<Fx>>> = meta::enabler>
		void set_fx(types<>, Key&& key, Fx&& fx) {
			set(std::forward<Key>(key), std::forward<Fx>(fx));
		}

		template <typename Fx, typename Key, typename... Args, meta::disable<meta::is_specialization_of<overload_set, meta::unqualified_t<Fx>>> = meta::enabler>
		void set_fx(types<>, Key&& key, Fx&& fx, Args&&... args) {
			set(std::forward<Key>(key), as_function_reference(std::forward<Fx>(fx), std::forward<Args>(args)...));
		}

		template <typename... Sig, typename... Args, typename Key>
		void set_resolved_function(Key&& key, Args&&... args) {
			set(std::forward<Key>(key), as_function_reference<function_sig<Sig...>>(std::forward<Args>(args)...));
		}

		template <typename... Args>
		basic_table_core& set(Args&&... args) {
			tuple_set<false>(std::make_index_sequence<sizeof...(Args) / 2>(), std::forward_as_tuple(std::forward<Args>(args)...));
			return *this;
		}
#endif
	};


} // namespace lua

X_NAMESPACE_END