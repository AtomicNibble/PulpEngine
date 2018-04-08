#pragma once

X_NAMESPACE_BEGIN(core)

template<size_t NumDim, typename CoordAccessor>
BoundingSphereGen<NumDim, CoordAccessor>::BoundingSphereGen(core::MemoryArenaBase* arena, Pit begin, Pit end, CoordAccessor ca) :
    pointsBegin_(begin),
    pointsEnd_(end),
    coordAccessor_(ca),
    nt0(NT(0)),
    L(),
    supportEnd_(L.begin()),
    forcedSize_(0),
    supportSize_(0),
    current_c(nullptr),
    currentsqrR_(NT(-1)),
    pSqr_(nullptr),
    pQ0_(nullptr),
    pZ_(nullptr),
    pF_(nullptr),
    buffer_(arena)
{
    create_arrays();

    // set initial center
    for (int32_t j = 0; j < NumDim; ++j) {
        pC_[0][j] = nt0;
    }

    current_c = pC_[0];

    // compute miniball
    pivot_mb(pointsEnd_);
}

template<size_t NumDim, typename CoordAccessor>
BoundingSphereGen<NumDim, CoordAccessor>::~BoundingSphereGen()
{
    delete_arrays();
}

template<size_t NumDim, typename CoordAccessor>
void BoundingSphereGen<NumDim, CoordAccessor>::create_arrays(void)
{
    const size_t dimplusOne = NumDim + 1;
    const size_t num = (dimplusOne * 4) + ((NumDim * 3) * dimplusOne);

    buffer_.resize(num);

    pSqr_ = &buffer_[0];
    pQ0_ = &buffer_[dimplusOne];
    pZ_ = &buffer_[dimplusOne * 2];
    pF_ = &buffer_[dimplusOne * 3];

    NT* pStart = &buffer_[dimplusOne * 4];
    for (int32_t i = 0; i < NumDim + 1; ++i) {
        const size_t offset = (3 * NumDim) * i;
        pC_[i] = &pStart[offset];
        pV_[i] = &pStart[offset + NumDim];
        pA_[i] = &pStart[offset + (NumDim * 2)];

        X_ASSERT(pA_[i] + NumDim <= buffer_.end(), "Out of bounds")(pA_[i] + NumDim, buffer_.end());
    }
}

template<size_t NumDim, typename CoordAccessor>
void BoundingSphereGen<NumDim, CoordAccessor>::delete_arrays(void)
{
    pF_ = nullptr;
    pZ_ = nullptr;
    pQ0_ = nullptr;
    pSqr_ = nullptr;

    for (int32_t i = 0; i < NumDim + 1; ++i) {
        pA_[i] = nullptr;
        pV_[i] = nullptr;
        pC_[i] = nullptr;
    }
    //	delete[] pA_;
    //	delete[] pV_;
    //	delete[] pC_;
}

template<size_t NumDim, typename CoordAccessor>
const typename BoundingSphereGen<NumDim, CoordAccessor>::NT* BoundingSphereGen<NumDim, CoordAccessor>::center(void) const
{
    return current_c;
}

template<size_t NumDim, typename CoordAccessor>
typename BoundingSphereGen<NumDim, CoordAccessor>::NT BoundingSphereGen<NumDim, CoordAccessor>::squaredRadius(void) const
{
    return currentsqrR_;
}

template<size_t NumDim, typename CoordAccessor>
int32_t BoundingSphereGen<NumDim, CoordAccessor>::nrSupportPoints(void) const
{
    return supportSize_;
}

template<size_t NumDim, typename CoordAccessor>
typename BoundingSphereGen<NumDim, CoordAccessor>::SupportPointIterator BoundingSphereGen<NumDim, CoordAccessor>::supportpointsBegin(void) const
{
    return L.begin();
}

template<size_t NumDim, typename CoordAccessor>
typename BoundingSphereGen<NumDim, CoordAccessor>::SupportPointIterator BoundingSphereGen<NumDim, CoordAccessor>::supportpointsEnd(void) const
{
    return supportEnd_;
}

template<size_t NumDim, typename CoordAccessor>
typename BoundingSphereGen<NumDim, CoordAccessor>::NT BoundingSphereGen<NumDim, CoordAccessor>::relativeError(NT& subopt) const
{
    NT e, max_e = nt0;
    // compute maximum absolute excess of support points
    for (SupportPointIterator it = supportpointsBegin(); it != supportpointsEnd(); ++it) {
        e = excess(*it);
        if (e < nt0) {
            e = -e;
        }

        max_e = core::Max(e, max_e);
    }
    // compute maximum excess of any point
    for (Pit i = pointsBegin_; i != pointsEnd_; ++i) {
        if ((e = excess(i)) > max_e) {
            max_e = e;
        }
    }

    subopt = suboptimality();
    return (currentsqrR_ == nt0 ? nt0 : max_e / currentsqrR_);
}

template<size_t NumDim, typename CoordAccessor>
bool BoundingSphereGen<NumDim, CoordAccessor>::isValid(NT tol) const
{
    NT suboptimality;
    return ((relativeError(suboptimality) <= tol) && (suboptimality == 0));
}

template<size_t NumDim, typename CoordAccessor>
void BoundingSphereGen<NumDim, CoordAccessor>::mtf_mb(Sit n)
{
    supportEnd_ = L.begin();

    if ((forcedSize_) == NumDim + 1) {
        return;
    }

    // incremental construction
    for (Sit i = L.begin(); i != n;) {
        Sit j = i++;
        if (excess(*j) > nt0) {
            if (push(*j)) {
                mtf_mb(j);
                pop();
                mtf_move_to_front(j);
            }
        }
    }
}

template<size_t NumDim, typename CoordAccessor>
void BoundingSphereGen<NumDim, CoordAccessor>::mtf_move_to_front(Sit j)
{
    if (supportEnd_ == j) {
        supportEnd_++;
    }

    L.splice(L.begin(), L, j);
}

template<size_t NumDim, typename CoordAccessor>
void BoundingSphereGen<NumDim, CoordAccessor>::pivot_mb(Pit n)
{
    NT old_sqr_r;
    const NT* cLocal;
    Pit pivot, k;
    NT e, max_e, sqr_r_local;
    Cit p;

    do {
        old_sqr_r = currentsqrR_;
        sqr_r_local = currentsqrR_;

        pivot = pointsBegin_;
        max_e = nt0;

        for (k = pointsBegin_; k != n; ++k) {
            p = coordAccessor_(k);
            e = -sqr_r_local;
            cLocal = current_c;
            for (int32_t j = 0; j < NumDim; ++j) {
                e += math<NT>::square(*p++ - *cLocal++);
            }
            if (e > max_e) {
                max_e = e;
                pivot = k;
            }
        }

        if (max_e > nt0) {
            // check if the pivot is already contained in the support set
            if (std::find(L.begin(), supportEnd_, pivot) == supportEnd_) {
                if (push(pivot)) {
                    mtf_mb(supportEnd_);
                    pop();
                    pivot_move_to_front(pivot);
                }
            }
        }

    } while (old_sqr_r < currentsqrR_);
}

template<size_t NumDim, typename CoordAccessor>
void BoundingSphereGen<NumDim, CoordAccessor>::pivot_move_to_front(Pit j)
{
    L.push_front(j);
    if (std::distance(L.begin(), supportEnd_) == NumDim + 2) {
        supportEnd_--;
    }
}

template<size_t NumDim, typename CoordAccessor>
inline typename BoundingSphereGen<NumDim, CoordAccessor>::NT
    BoundingSphereGen<NumDim, CoordAccessor>::excess(Pit pit) const
{
    Cit p = coordAccessor_(pit);
    NT e = -currentsqrR_;
    NT* cLocal = current_c;
    for (int32_t k = 0; k < NumDim; ++k) {
        e += math<NT>::square(*p++ - *cLocal++);
    }
    return e;
}

template<size_t NumDim, typename CoordAccessor>
void BoundingSphereGen<NumDim, CoordAccessor>::pop(void)
{
    --forcedSize_;
}

template<size_t NumDim, typename CoordAccessor>
bool BoundingSphereGen<NumDim, CoordAccessor>::push(Pit pit)
{
    int32_t i, j;
    NT eps = math<NT>::square(std::numeric_limits<NT>::epsilon());

    Cit cit = coordAccessor_(pit);
    Cit p = cit;

    if (forcedSize_ == 0) {
        for (i = 0; i < NumDim; ++i) {
            pQ0_[i] = *p++;
        }
        for (i = 0; i < NumDim; ++i) {
            pC_[0][i] = pQ0_[i];
        }
        pSqr_[0] = nt0;
    }
    else {
        // set v_fsize to Q_fsize
        for (i = 0; i < NumDim; ++i) {
            pV_[forcedSize_][i] = *p++ - pQ0_[i];
        }

        for (i = 1; i < forcedSize_; ++i) {
            pA_[forcedSize_][i] = nt0;
            for (j = 0; j < NumDim; ++j) {
                pA_[forcedSize_][i] += pV_[i][j] * pV_[forcedSize_][j];
            }
            pA_[forcedSize_][i] *= (2 / pZ_[i]);
        }

        for (i = 1; i < forcedSize_; ++i) {
            for (j = 0; j < NumDim; ++j) {
                pV_[forcedSize_][j] -= pA_[forcedSize_][i] * pV_[i][j];
            }
        }

        // compute z_fsize
        pZ_[forcedSize_] = nt0;
        for (j = 0; j < NumDim; ++j) {
            pZ_[forcedSize_] += math<NT>::square(pV_[forcedSize_][j]);
        }
        pZ_[forcedSize_] *= 2;

        // reject push if z_fsize too small
        if (pZ_[forcedSize_] < eps * currentsqrR_) {
            return false;
        }

        // update pC_, pSqr_
        p = cit;
        NT e = -pSqr_[forcedSize_ - 1];
        for (i = 0; i < NumDim; ++i) {
            e += math<NT>::square(*p++ - pC_[forcedSize_ - 1][i]);
        }

        pF_[forcedSize_] = e / pZ_[forcedSize_];

        for (i = 0; i < NumDim; ++i) {
            pC_[forcedSize_][i] = pC_[forcedSize_ - 1][i] + pF_[forcedSize_] * pV_[forcedSize_][i];
        }

        pSqr_[forcedSize_] = pSqr_[forcedSize_ - 1] + e * pF_[forcedSize_] / 2;
    }

    current_c = pC_[forcedSize_];
    currentsqrR_ = pSqr_[forcedSize_];
    supportSize_ = ++forcedSize_;
    return true;
}

template<size_t NumDim, typename CoordAccessor>
typename BoundingSphereGen<NumDim, CoordAccessor>::NT BoundingSphereGen<NumDim, CoordAccessor>::suboptimality(void) const
{
    NT* l = new NT[NumDim + 1];
    NT min_l = nt0;
    l[0] = NT(1);
    for (int32_t i = supportSize_ - 1; i > 0; --i) {
        l[i] = pF_[i];
        for (int32_t k = supportSize_ - 1; k > i; --k) {
            l[i] -= pA_[k][i] * l[k];
        }

        min_l = core::Min(min_l, l[i]);

        l[0] -= l[i];
    }

    min_l = core::Min(min_l, l[0]);

    delete[] l;

    if (min_l < nt0) {
        return -min_l;
    }
    return nt0;
}

X_NAMESPACE_END
