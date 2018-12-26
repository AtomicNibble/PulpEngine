#pragma once


#include <Ilocalisation.h>

#include <Containers/FixedHashTable.h>

X_NAMESPACE_BEGIN(locale)


class Localisation : public ILocalisation
{
    struct hash
    {
        size_t operator()(const core::StrHash& s) const
        {
            return static_cast<size_t>(s.hash());
        }
    };

    struct equal_to
    {
        bool operator()(const core::StrHash& lhs, const core::StrHash& rhs) const
        {
            return lhs == rhs;
        }
    };

    using HashTable = core::FixedHashTable<core::StrHash, core::string, hash, equal_to>;

public:
    Localisation(core::MemoryArenaBase* arena);
    ~Localisation() X_FINAL;

    string_view getString(Key k) const X_FINAL;

    bool loadDict(core::Path<char>& name);

private:
    core::MemoryArenaBase* arena_;
    HashTable ht_;
};


X_NAMESPACE_END
