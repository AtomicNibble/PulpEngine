#pragma once

#include <Containers\Array.h>

X_NAMESPACE_BEGIN(game)

namespace ecs
{
    template<typename, typename, typename...>
    class ComponentPool;

    template<typename Entity, typename Component>
    class ComponentPool<Entity, Component>
    {
    public:
        using component_type = Component;
        using entity_type = Entity;
        using pos_type = entity_type;
        using size_type = typename core::Array<component_type>::size_type;

        template<typename T>
        using AlignedArray = core::Array<T, core::ArrayAlignedAllocatorFixed<T, 64>, core::growStrat::Multiply>;

        typedef AlignedArray<component_type> CompDataArr;
        typedef AlignedArray<pos_type> PosTypeArr;
        typedef AlignedArray<entity_type> EntityTypeArr;

    public:
        explicit ComponentPool(core::MemoryArenaBase* arena, size_type dim = 4098) :
            data_(arena),
            reverse_(arena),
            direct_(arena)
        {
            data_.reserve(dim);
            reverse_.reserve(dim);
            direct_.reserve(dim);
        }

        ComponentPool(ComponentPool&&) = default;
        ~ComponentPool()
        {
        }

        ComponentPool& operator=(ComponentPool&&) = default;

        void reserve(size_type size)
        {
            data_.reserve(size);
            reverse_.reserve(size);
            direct_.reserve(size);
        }

        X_INLINE bool empty(void) const
        {
            return data_.isEmpty();
        }

        X_INLINE size_type capacity(void) const
        {
            return data_.capacity();
        }

        X_INLINE size_type size(void) const
        {
            return data_.size();
        }

        X_INLINE const entity_type* entities(void) const
        {
            return direct_.data();
        }

        X_INLINE bool has(entity_type entity) const
        {
            return valid(entity);
        }

        X_INLINE const component_type& get(entity_type entity) const
        {
            X_ASSERT(valid(entity), "Entity not valid")
            ();
            return data_[reverse_[entity]];
        }

        X_INLINE component_type& get(entity_type entity)
        {
            return const_cast<component_type&>(const_cast<const ComponentPool*>(this)->get(entity));
        }

        template<typename... Args>
        component_type& construct(entity_type entity, Args... args)
        {
            X_ASSERT(!valid(entity), "Entity already valid")
            ();

            if (!(entity < reverse_.size())) {
                reverse_.resize(entity + 1);
            }

            reverse_[entity] = pos_type(direct_.size());
            direct_.emplace_back(entity);
            data_.push_back({std::forward<Args>(args)...});

            return data_.back();
        }

        void destroy(entity_type entity)
        {
            X_ASSERT(valid(entity), "Entity not valid")
            ();

            auto last = direct_.size() - 1;

            reverse_[direct_[last]] = reverse_[entity];
            direct_[reverse_[entity]] = direct_[last];
            data_[reverse_[entity]] = std::move(data_[last]);

            direct_.pop_back();
            data_.pop_back();
        }

        void reset(void)
        {
            data_.clear();
            reverse_.clear();
            direct_.clear();
        }

    private:
        X_INLINE bool valid(entity_type entity) const
        {
            return entity < reverse_.size() && reverse_[entity] < direct_.size() && direct_[reverse_[entity]] == entity;
        }

    private:
        CompDataArr data_;
        PosTypeArr reverse_;
        EntityTypeArr direct_;
    };

    template<typename Entity, typename Component, typename... Components>
    class ComponentPool : ComponentPool<Entity, Component>
        , ComponentPool<Entity, Components>...
    {
        template<typename Comp>
        using Pool = ComponentPool<Entity, Comp>;

    public:
        using entity_type = typename Pool<Component>::entity_type;
        using pos_type = typename Pool<Component>::pos_type;
        using size_type = typename Pool<Component>::size_type;

    public:
        explicit ComponentPool(core::MemoryArenaBase* arena, size_type dim = 4098) :
            ComponentPool<Entity, Component>{arena, dim},
            ComponentPool<Entity, Components>{arena, dim}...
        {
        }

        ComponentPool(const ComponentPool&) = delete;
        ComponentPool(ComponentPool&&) = delete;

        ComponentPool& operator=(const ComponentPool&) = delete;
        ComponentPool& operator=(ComponentPool&&) = delete;

        template<typename Comp>
        X_INLINE bool empty(void) const
        {
            return Pool<Comp>::empty();
        }

        template<typename Comp>
        X_INLINE size_type capacity(void) const
        {
            return Pool<Comp>::capacity();
        }

        template<typename Comp>
        X_INLINE size_type size(void) const
        {
            return Pool<Comp>::size();
        }

        template<typename Comp>
        X_INLINE const entity_type* entities(void) const
        {
            return Pool<Comp>::entities();
        }

        template<typename Comp>
        X_INLINE bool has(entity_type entity) const
        {
            return Pool<Comp>::has(entity);
        }

        template<typename Comp>
        X_INLINE const Comp& get(entity_type entity) const
        {
            return Pool<Comp>::get(entity);
        }

        template<typename Comp>
        X_INLINE Comp& get(entity_type entity)
        {
            return const_cast<Comp&>(const_cast<const ComponentPool*>(this)->get<Comp>(entity));
        }

        template<typename Comp, typename... Args>
        X_INLINE Comp& construct(entity_type entity, Args... args)
        {
            return Pool<Comp>::construct(entity, std::forward<Args>(args)...);
        }

        template<typename Comp>
        X_INLINE void destroy(entity_type entity)
        {
            Pool<Comp>::destroy(entity);
        }

        template<typename Comp>
        X_INLINE void reset(void)
        {
            Pool<Comp>::reset();
        }

        void reset(void)
        {
            using accumulator_type = int[];
            Pool<Component>::reset();
            accumulator_type accumulator = {(Pool<Components>::reset(), 0)...};
            X_UNUSED(accumulator);
        }

        void reserve(size_t size)
        {
            using accumulator_type = int[];
            Pool<Component>::reserve(size);
            accumulator_type accumulator = {(Pool<Components>::reserve(size), 0)...};
            X_UNUSED(accumulator);
        }

        void setGranularity(size_t gran)
        {
            using accumulator_type = int[];
            Pool<Component>::setGranularity(gran);
            accumulator_type accumulator = {(Pool<Components>::setGranularity(gran), 0)...};
            X_UNUSED(accumulator);
        }
    };

} // namespace ecs

X_NAMESPACE_END