#pragma once

#ifndef _X_MATH_SPHERE_GEN_H_
#define _X_MATH_SPHERE_GEN_H_

#include <array>
#include <list>

#include <Containers\Array.h>

X_NAMESPACE_BEGIN(core)

template<typename Pit_, typename Cit_>
struct CoordAccessor
{
    typedef Pit_ Pit;
    typedef Cit_ Cit;
    inline Cit operator()(Pit it) const
    {
        return (*it).begin();
    }
};

template<typename Pit_, typename Cit_>
struct CoordAccessor<Pit_, Cit_*>
{
    typedef Pit_ Pit;
    typedef Cit_* Cit;
    inline Cit operator()(Pit it) const
    {
        return *it;
    }
};

template<size_t NumDim, typename CoordAccessor>
class BoundingSphereGen
{
private:
    typedef typename CoordAccessor::Pit Pit;
    typedef typename CoordAccessor::Cit Cit;
    typedef typename std::iterator_traits<Cit>::value_type NT;
    typedef typename std::list<Pit>::iterator Sit;

    // The iterator type to go through the support points
    typedef typename std::list<Pit>::const_iterator SupportPointIterator;

    typedef std::array<NT*, NumDim + 1> NtPtrArr;

public:
    BoundingSphereGen(core::MemoryArenaBase* arena, Pit begin, Pit end, CoordAccessor ca = CoordAccessor());
    ~BoundingSphereGen();

    const NT* center(void) const;
    NT squaredRadius(void) const;

    int32_t nrSupportPoints(void) const;

    SupportPointIterator supportpointsBegin(void) const;
    SupportPointIterator supportpointsEnd(void) const;

    NT relativeError(NT& subopt) const;

    bool is_valid(NT tol = NT(10) * std::numeric_limits<NT>::epsilon()) const;

private:
    void mtf_mb(Sit n);
    void mtf_move_to_front(Sit j);
    void pivot_mb(Pit n);
    void pivot_move_to_front(Pit j);
    NT excess(Pit pit) const;
    void pop(void);
    bool push(Pit pit);
    NT suboptimality(void) const;
    void create_arrays(void);
    void delete_arrays(void);

private:
    const NT nt0; // NT(0)
    Pit pointsBegin_;
    Pit pointsEnd_;
    CoordAccessor coordAccessor_;

    //...for the algorithms
    std::list<Pit> L;
    Sit supportEnd_;
    int32_t forcedSize_;  // number of forced points
    int32_t supportSize_; // number of support points

    NT* current_c;
    NT currentsqrR_;
    NtPtrArr pC_;
    NT* pSqr_;

    // helper arrays
    NT* pQ0_;
    NT* pZ_;
    NT* pF_;
    NtPtrArr pV_;
    NtPtrArr pA_;

    core::Array<NT> buffer_;
};

X_NAMESPACE_END

#include "XSphereGen.inl"

#endif // !_X_MATH_SPHERE_GEN_H_
