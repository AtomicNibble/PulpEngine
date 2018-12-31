#pragma once

#include <tuple>
#include <bitset>

X_NAMESPACE_BEGIN(game)

namespace ecs
{
    namespace details
    {
        template<typename Type>
        struct Wrapper
        {
            using type = Type;

            constexpr Wrapper(size_t index) :
                index{index}
            {
            }

            const size_t index;
        };

        template<typename... Types>
        struct Identifier final : Wrapper<Types>...
        {
            template<size_t... Indexes>
            constexpr Identifier(std::index_sequence<Indexes...>) :
                Wrapper<Types>(Indexes)...
            {
            }

            template<typename Type>
            constexpr size_t get(void) const
            {
                return Wrapper<std::decay_t<Type>>::index;
            }
        };

    } // namespace details

    template<typename... Types>
    constexpr auto ident = details::Identifier<std::decay_t<Types>...>{std::make_index_sequence<sizeof...(Types)>{}};

    template<typename...>
    class View;

    template<template<typename...> class Pool, typename Entity, typename... Components, typename Type, typename... Types, typename... Filters>
    class View<Pool<Entity, Components...>, std::tuple<Type, Types...>, std::tuple<Filters...>> final
    {
        using pool_type = Pool<Entity, Components...>;
        using entity_type = typename pool_type::entity_type;
        using mask_type = std::bitset<sizeof...(Components) + 1>;

        class ViewIterator
        {
        public:
            using value_type = entity_type;
            using difference_type = std::ptrdiff_t;
            using reference = entity_type&;
            using pointer = entity_type*;
            using iterator_category = std::input_iterator_tag;

            ViewIterator(pool_type& pool, const entity_type* pEntities, typename pool_type::size_type pos, mask_type* pMask) :
                pool_{pool},
                pEntities_{pEntities},
                pos_{pos},
                pMask_{pMask}
            {
                if (this->pos_) {
                    while (!isValid() && --this->pos_)
                        ;
                }
            }

            X_INLINE ViewIterator& operator++()
            {
                if (pos_) {
                    while (--pos_ && !isValid())
                        ;
                }
                return *this;
            }

            X_INLINE ViewIterator operator++(int)
            {
                ViewIterator orig = *this;
                return this->operator++(), orig;
            }

            X_INLINE bool operator==(const ViewIterator& other) const
            {
                return other.pos_ == pos_ && other.pMask_ == pMask_ && other.pEntities_ == pEntities_;
            }

            X_INLINE bool operator!=(const ViewIterator& other) const
            {
                return !(*this == other);
            }

            X_INLINE value_type operator*() const
            {
                return *(pEntities_ + pos_ - 1);
            }

        private:
            bool isValid(void) const
            {
                using accumulator_type = bool[];
                auto& bitmask = pMask_[pEntities_[pos_ - 1]];
                bool all = bitmask.test(ident<Components...>.template get<Type>());
                accumulator_type types = {true, (all = all && bitmask.test(ident<Components...>.template get<Types>()))...};
                accumulator_type filters = {true, (all = all && !bitmask.test(ident<Components...>.template get<Filters>()))...};
                return void(types), void(filters), all;
            }

        private:
            pool_type& pool_;
            const entity_type* pEntities_;
            typename pool_type::size_type pos_;
            mask_type* pMask_;
        };

        template<typename Comp>
        void prefer(void)
        {
            size_type sz = pool_.template size<Comp>();

            if (sz < size_) {
                pEntities_ = pool_.template entities<Comp>();
                size_ = sz;
            }
        }

    public:
        using iterator_type = ViewIterator;
        using size_type = typename pool_type::size_type;

        template<typename... Comp>
        using view_type = View<pool_type, std::tuple<Type, Types...>, std::tuple<Comp...>>;

        explicit View(pool_type& pool, mask_type* pMask) :
            pEntities_{pool.template entities<Type>()},
            size_{pool.template size<Type>()},
            pool_{pool},
            pMask_{pMask}
        {
            using accumulator_type = int[];
            accumulator_type accumulator = {0, (prefer<Types>(), 0)...};
            (void)accumulator;
        }

        template<typename... Comp>
        X_INLINE view_type<Comp...> exclude(void)
        {
            return view_type<Comp...>{pool_, pMask_};
        }

        X_INLINE iterator_type begin(void) const
        {
            return ViewIterator{pool_, pEntities_, size_, pMask_};
        }

        X_INLINE iterator_type end(void) const
        {
            return ViewIterator{pool_, pEntities_, 0, pMask_};
        }

        void reset(void)
        {
            using accumulator_type = int[];
            pEntities_ = pool_.template entities<Type>();
            size_ = pool_.template size<Type>();
            accumulator_type accumulator = {0, (prefer<Types>(), 0)...};
            (void)accumulator;
        }

    private:
        const entity_type* pEntities_;
        size_type size_;
        pool_type& pool_;
        mask_type* pMask_;
    };

    template<template<typename...> class Pool, typename Entity, typename... Components, typename Type>
    class View<Pool<Entity, Components...>, std::tuple<Type>, std::tuple<>> final
    {
        using pool_type = Pool<Entity, Components...>;
        using entity_type = typename pool_type::entity_type;
        using mask_type = std::bitset<sizeof...(Components) + 1>;

        struct ViewIterator
        {
            using value_type = entity_type;
            using difference_type = std::ptrdiff_t;
            using reference = entity_type&;
            using pointer = entity_type*;
            using iterator_category = std::input_iterator_tag;

            ViewIterator(const entity_type* pEntities, typename pool_type::size_type pos) :
                pEntities_{pEntities},
                pos_{pos}
            {
            }

            X_INLINE ViewIterator& operator++()
            {
                --pos_;
                return *this;
            }

            X_INLINE ViewIterator operator++(int)
            {
                ViewIterator orig = *this;
                return this->operator++(), orig;
            }

            X_INLINE bool operator==(const ViewIterator& other) const
            {
                return other.pos_ == pos_ && other.pEntities_ == pEntities_;
            }

            X_INLINE bool operator!=(const ViewIterator& other) const
            {
                return !(*this == other);
            }

            X_INLINE value_type operator*() const
            {
                return *(pEntities_ + pos_ - 1);
            }

        private:
            const entity_type* pEntities_;
            typename pool_type::size_type pos_;
        };

    public:
        using iterator_type = ViewIterator;
        using size_type = typename pool_type::size_type;

        template<typename... Comp>
        using view_type = View<pool_type, std::tuple<Type>, std::tuple<Comp...>>;

    public:
        explicit View(pool_type& pool, mask_type* pMask) :
            pool_{pool},
            pMask_{pMask}
        {
        }

        template<typename... Comp>
        X_INLINE view_type<Comp...> exclude(void)
        {
            return view_type<Comp...>{pool_, pMask_};
        }

        X_INLINE iterator_type begin(void) const
        {
            return ViewIterator{pool_.template entities<Type>(), pool_.template size<Type>()};
        }

        X_INLINE iterator_type end(void) const
        {
            return ViewIterator{pool_.template entities<Type>(), 0};
        }

        X_INLINE size_type size(void) const
        {
            return pool_.template size<Type>();
        }

    private:
        pool_type& pool_;
        mask_type* pMask_;
    };

    template<typename>
    class Registry;

    template<template<typename...> class Pool, typename Entity, typename... Components>
    class Registry<Pool<Entity, Components...>>
    {
        static_assert(sizeof...(Components) > 1, "!");

        static constexpr auto validity_bit = sizeof...(Components);

    public:
        template<typename T>
        using AlignedArray = core::Array<T, core::ArrayAlignedAllocatorFixed<T, 64>, core::growStrat::Multiply>;

        using pool_type = Pool<Entity, Components...>;
        using mask_type = std::bitset<sizeof...(Components) + 1>;
        using entity_type = typename pool_type::entity_type;
        using size_type = typename AlignedArray<mask_type>::size_type;

    public:
        template<typename... Comp>
        using view_type = View<pool_type, std::tuple<Comp...>, std::tuple<>>;

        typedef AlignedArray<mask_type> MaskArr;
        typedef AlignedArray<entity_type> EntityArr;

        static const entity_type INVALID_ID = std::numeric_limits<entity_type>::max();

    public:
        template<typename... Args>
        Registry(core::MemoryArenaBase* arena, Args&&... args) :
            pool_{arena, std::forward<Args>(args)...},
            entities_(arena),
            freelist_(arena)
        {
        }

        Registry(const Registry&) = delete;
        Registry(Registry&&) = delete;

        Registry& operator=(const Registry&) = delete;
        Registry& operator=(Registry&&) = delete;

        void entIdReserve(size_t size)
        {
            entities_.reserve(size);
        }

        void compReserve(size_t size)
        {
            pool_.reserve(size);
        }

        template<typename Comp>
        void compReserve(size_t size)
        {
            pool_.template reserve<Comp>(size);
        }

        void freelistReserve(size_t size)
        {
            freelist_.reserve(size);
        }

        X_INLINE size_type size(void) const
        {
            return entities_.size() - freelist_.size();
        }

        X_INLINE size_type capacity(void) const
        {
            return entities_.size();
        }

        template<typename Comp>
        X_INLINE bool empty(void) const
        {
            return pool_.template empty<Comp>();
        }

        X_INLINE bool empty(void) const
        {
            return entities_.isEmpty();
        }

        X_INLINE bool isValid(entity_type entity) const
        {
            return (entity < entities_.size() && entities_[entity].test(validity_bit));
        }

        template<typename... Comp>
        entity_type create(void)
        {
            using accumulator_type = int[];
            entity_type entity = create();
            accumulator_type accumulator = {0, (assign<Comp>(entity), 0)...};
            (void)accumulator;
            return entity;
        }

        entity_type create(void)
        {
            entity_type entity;

            if (freelist_.isEmpty()) {
                entity = safe_static_cast<entity_type>(entities_.size());
                entities_.emplace_back();
            }
            else {
                entity = freelist_.back();
                freelist_.pop_back();
            }

            entities_[entity].set(validity_bit);

            return entity;
        }

        void destroy(entity_type entity)
        {
            X_ASSERT(isValid(entity), "Not valid entity")();

            using accumulator_type = int[];
            accumulator_type accumulator = {0, (reset<Components>(entity), 0)...};
            freelist_.push_back(entity);
            entities_[entity].reset();
            (void)accumulator;
        }

        mask_type getComponentMask(entity_type entity) const
        {
            X_ASSERT(isValid(entity), "Not valid entity")();
            return entities_[entity];
        }

        template<typename Comp, typename... Args>
        Comp& assign(entity_type entity, Args... args)
        {
            X_ASSERT(isValid(entity), "Not valid entity")();

            entities_[entity].set(ident<Components...>.template get<Comp>());
            return pool_.template construct<Comp>(entity, args...);
        }

        template<typename Comp>
        void remove(entity_type entity)
        {
            X_ASSERT(isValid(entity), "Not valid entity")();

            entities_[entity].reset(ident<Components...>.template get<Comp>());
            pool_.template destroy<Comp>(entity);
        }

        // make this a overload of has() if I can be bothered.
        template<typename... Comp>
        bool hasAll(entity_type entity) const
        {
            X_ASSERT(isValid(entity), "Not valid entity")();

            using accumulator_type = bool[];
            bool all = true;
            auto& mask = entities_[entity];
            accumulator_type accumulator = {true, (all = all && mask.test(ident<Components...>.template get<Comp>()))...};
            (void)accumulator;
            return all;
        }

        template<typename Comp>
        bool has(entity_type entity) const
        {
            X_ASSERT(isValid(entity), "Not valid entity")();

            auto& mask = entities_[entity];
            return mask.test(ident<Components...>.template get<Comp>());
        }

        template<typename Comp>
        bool has(mask_type mask) const
        {
            return mask.test(ident<Components...>.template get<Comp>());
        }

        template<typename Comp>
        X_INLINE const Comp& get(entity_type entity) const
        {
            return pool_.template get<Comp>(entity);
        }

        template<typename Comp>
        X_INLINE Comp& get(entity_type entity)
        {
            return pool_.template get<Comp>(entity);
        }

        template<typename Comp, typename... Args>
        X_INLINE Comp& replace(entity_type entity, Args... args)
        {
            return (pool_.template get<Comp>(entity) = Comp{args...});
        }

        template<typename Comp, typename... Args>
        Comp& accomodate(entity_type entity, Args... args)
        {
            X_ASSERT(isValid(entity), "Not valid entity")();

            return (entities_[entity].test(ident<Components...>.template get<Comp>())
                        ? this->template replace<Comp>(entity, std::forward<Args>(args)...)
                        : this->template assign<Comp>(entity, std::forward<Args>(args)...));
        }

        entity_type clone(entity_type from)
        {
            X_ASSERT(isValid(from), "Not valid entity")();

            using accumulator_type = int[];
            auto to = create();
            accumulator_type accumulator = {0, (clone<Components>(to, from), 0)...};
            (void)accumulator;
            return to;
        }

        template<typename Comp>
        Comp& copy(entity_type to, entity_type from)
        {
            return (pool_.template get<Comp>(to) = pool_.template get<Comp>(from));
        }

        void copy(entity_type to, entity_type from)
        {
            using accumulator_type = int[];
            accumulator_type accumulator = {0, (sync<Components>(to, from), 0)...};
            (void)accumulator;
        }

        template<typename Comp>
        void reset(entity_type entity)
        {
            X_ASSERT(isValid(entity), "Not valid entity")();

            if (entities_[entity].test(ident<Components...>.template get<Comp>())) {
                remove<Comp>(entity);
            }
        }

        void reset(entity_type entity)
        {
            X_ASSERT(isValid(entity), "Not valid entity")();

            using accumulator_type = int[];
            accumulator_type accumulator = { 0, (reset<Components>(entity), 0)... };
            (void)accumulator;

            X_ASSERT(isValid(entity), "Not valid entity")();
        }

        template<typename Comp>
        void reset(void)
        {
            for (entity_type entity = 0, last = safe_static_cast<entity_type>(entities_.size()); entity < last; ++entity) {
                if (entities_[entity].test(ident<Components...>.template get<Comp>())) {
                    remove<Comp>(entity);
                }
            }
        }

        void reset(void)
        {
            entities_.clear();
            freelist_.clear();
            pool_.reset();
        }

        template<typename... Comp>
        view_type<Comp...> view(void)
        {
            return view_type<Comp...>{pool_, entities_.data()};
        }

    private:
        template<typename Comp>
        void clone(entity_type to, entity_type from)
        {
            if (entities_[from].test(ident<Components...>.template get<Comp>())) {
                assign<Comp>(to, pool_.template get<Comp>(from));
            }
        }

        template<typename Comp>
        void sync(entity_type to, entity_type from)
        {
            bool src = entities_[from].test(ident<Components...>.template get<Comp>());
            bool dst = entities_[to].test(ident<Components...>.template get<Comp>());

            if (src && dst) {
                copy<Comp>(to, from);
            }
            else if (src) {
                clone<Comp>(to, from);
            }
            else if (dst) {
                remove<Comp>(to);
            }
        }

    private:
        MaskArr entities_;
        EntityArr freelist_;
        pool_type pool_;
    };

    template<typename Entity, typename... Components>
    using StandardRegistry = Registry<ComponentPool<Entity, Components...>>;

    template<typename... Components>
    using DefaultRegistry = Registry<ComponentPool<uint16_t, Components...>>;

} // namespace ecs

X_NAMESPACE_END