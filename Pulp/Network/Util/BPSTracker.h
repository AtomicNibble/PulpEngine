#pragma once

#include <Containers\Fifo.h>

X_NAMESPACE_BEGIN(net)

class BPSTracker
{
    typedef std::pair<core::TimeVal, uint64_t> TimeVal;
    typedef core::Fifo<TimeVal> TimeValArr;

public:
    typedef typename TimeValArr::Type Type;
    typedef typename TimeValArr::value_type value_type;
    typedef typename TimeValArr::size_type size_type;

public:
    BPSTracker(core::MemoryArenaBase* arena);

    void update(core::TimeVal time);
    void reset(void);
    void free(void);

    X_INLINE void add(core::TimeVal time, uint64_t val);
    X_INLINE uint64_t getTotal(void) const;
    X_INLINE uint64_t getBPS(void) const;

    X_INLINE size_type size(void) const;
    X_INLINE size_type capacity(void) const;

private:
    uint64_t total_;
    uint64_t lastSec_;

    TimeValArr values_;
};

X_NAMESPACE_END

#include "BPSTracker.inl"