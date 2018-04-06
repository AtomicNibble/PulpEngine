#include "stdafx.h"
#include "FaceOptermize.h"

#include <Containers\Array.h>

X_NAMESPACE_BEGIN(model)

template<typename IndexType>
const float32_t FaceOptimize<IndexType>::FindVertexScore_CacheDecayPower = 1.5f;
template<typename IndexType>
const float32_t FaceOptimize<IndexType>::FindVertexScore_LastTriScore = 0.75f;
template<typename IndexType>
const float32_t FaceOptimize<IndexType>::FindVertexScore_ValenceBoostScale = 2.0f;
template<typename IndexType>
const float32_t FaceOptimize<IndexType>::FindVertexScore_ValenceBoostPower = 0.5f;

template<typename IndexType>
FaceOptimize<IndexType>::OptimizeVertexData::OptimizeVertexData() :
    score(0.f),
    activeFaceListStart(0),
    activeFaceListSize(0),
    cachePos0(0),
    cachePos1(0)
{
}

// ------------------------------------------
template<typename IndexType>
template<typename T>
FaceOptimize<IndexType>::IndexSortCompareIndexed<T>::IndexSortCompareIndexed(const IndexType* pIndexData) :
    pIndexData_(pIndexData_)
{
}

template<typename IndexType>
template<typename T>
bool FaceOptimize<IndexType>::IndexSortCompareIndexed<T>::operator()(T a, T b) const
{
    IndexType indexA = pIndexData_[a];
    IndexType indexB = pIndexData_[b];

    if (indexA < indexB) {
        return true;
    }
    return false;
}

// ------------------------------------------

template<typename IndexType>
template<typename T>
FaceOptimize<IndexType>::FaceValenceSort<T>::FaceValenceSort(const OptimizeVertexData* pVertexData) :
    pVertexData_(pVertexData)
{
}

template<typename IndexType>
template<typename T>
bool FaceOptimize<IndexType>::FaceValenceSort<T>::operator()(T a, T b) const
{
    const OptimizeVertexData* vA0 = pVertexData_ + a * 3 + 0;
    const OptimizeVertexData* vA1 = pVertexData_ + a * 3 + 1;
    const OptimizeVertexData* vA2 = pVertexData_ + a * 3 + 2;
    const OptimizeVertexData* vB0 = pVertexData_ + b * 3 + 0;
    const OptimizeVertexData* vB1 = pVertexData_ + b * 3 + 1;
    const OptimizeVertexData* vB2 = pVertexData_ + b * 3 + 2;

    int32_t aValence = vA0->activeFaceListSize + vA1->activeFaceListSize + vA2->activeFaceListSize;
    int32_t bValence = vB0->activeFaceListSize + vB1->activeFaceListSize + vB2->activeFaceListSize;

    // higher scoring faces are those with lower valence totals
    // reverse sort (reverse of reverse)
    if (aValence < bValence) {
        return true;
    }
    return false;
}

// ------------------------------------------

template<typename IndexType>
FaceOptimize<IndexType>::FaceOptimize(core::MemoryArenaBase* arena) :
    arena_(arena)
{
}

template<typename IndexType>
bool FaceOptimize<IndexType>::OptimizeFaces(const IndexType* indexList, size_t indexCount_,
    IndexType* newIndexList, uint16_t lruCacheSize)
{
    if ((indexCount_ % 3) != 0) {
        X_ERROR("FaceOptimize", "index count must be a multiple of 3");
        return false;
    }
    if (lruCacheSize > MAX_VERTEX_CACHE_SIZE) {
        X_ERROR("FaceOptimize", "LRU cache size %i is bigger than max: %i", lruCacheSize, MAX_VERTEX_CACHE_SIZE);
        return false;
    }

    const uint32_t indexCount = safe_static_cast<uint32_t, size_t>(indexCount_);
    const uint32_t faceCount = (indexCount / 3);

    core::Array<OptimizeVertexData> vertexDataList(arena_, indexCount);
    core::Array<IndexType> vertexRemap(arena_, indexCount);
    core::Array<uint32_t> activeFaceList(arena_, indexCount);

    core::Array<uint8_t> processedFaceList(arena_, faceCount);
    core::Array<uint32_t> faceSorted(arena_, faceCount);
    core::Array<uint32_t> faceReverseLookup(arena_, faceCount);

    uint32_t uniqueVertexCount = 0;

    // build the vertex remap table
    {
        typedef IndexSortCompareIndexed<uint32_t> indexSorter;

        core::Array<uint32_t> indexSorted(arena_, indexCount);

        for (uint32_t i = 0; i < indexCount; i++) {
            indexSorted[i] = i;
        }

        indexSorter sortFunc(indexList);
        std::sort(indexSorted.begin(), indexSorted.end(), sortFunc);

        for (uint32_t i = 0; i < indexCount; i++) {
            if (i == 0 || sortFunc(indexSorted[i - 1], indexSorted[i])) {
                // it's not a duplicate
                vertexRemap[indexSorted[i]] = uniqueVertexCount;
                uniqueVertexCount++;
            }
            else {
                vertexRemap[indexSorted[i]] = vertexRemap[indexSorted[i - 1]];
            }
        }
    }

    // compute face count per vertex
    for (uint32_t i = 0; i < indexCount; ++i) {
        OptimizeVertexData& vertexData = vertexDataList[vertexRemap[i]];
        vertexData.activeFaceListSize++;
    }

    const IndexType kEvictedCacheIndex = std::numeric_limits<IndexType>::max();
    {
        // allocate face list per vertex
        uint32_t curActiveFaceListPos = 0;
        for (uint32_t i = 0; i < uniqueVertexCount; ++i) {
            OptimizeVertexData& vertexData = vertexDataList[i];
            vertexData.cachePos0 = kEvictedCacheIndex;
            vertexData.cachePos1 = kEvictedCacheIndex;
            vertexData.activeFaceListStart = curActiveFaceListPos;
            curActiveFaceListPos += vertexData.activeFaceListSize;
            vertexData.score = FindVertexScore(vertexData.activeFaceListSize, vertexData.cachePos0, lruCacheSize);
            vertexData.activeFaceListSize = 0;
        }

        X_ASSERT(curActiveFaceListPos == indexCount, "face list pos error")(curActiveFaceListPos, indexCount); 
    }

    // sort unprocessed faces by highest score
    for (uint32_t f = 0; f < faceCount; f++) {
        faceSorted[f] = f;
    }

    FaceValenceSort<uint32_t> faceValenceSort(vertexDataList.data());
    std::sort(faceSorted.begin(), faceSorted.end(), faceValenceSort);
    for (uint32_t f = 0; f < faceCount; f++) {
        faceReverseLookup[faceSorted[f]] = f;
    }

    // fill out face list per vertex
    for (uint32_t i = 0; i < indexCount; i += 3) {
        for (uint32_t j = 0; j < 3; ++j) {
            OptimizeVertexData& vertexData = vertexDataList[vertexRemap[i + j]];
            activeFaceList[vertexData.activeFaceListStart + vertexData.activeFaceListSize] = i;
            vertexData.activeFaceListSize++;
        }
    }

    IndexType vertexCacheBuffer[(MAX_VERTEX_CACHE_SIZE + 3) * 2];
    IndexType* cache0 = vertexCacheBuffer;
    IndexType* cache1 = vertexCacheBuffer + (MAX_VERTEX_CACHE_SIZE + 3);
    IndexType entriesInCache0 = 0;

    uint32_t bestFace = 0;
    float32_t bestScore = -1.f;

    const float32_t maxValenceScore = FindVertexScore(1, kEvictedCacheIndex, lruCacheSize) * 3.f;

    uint32_t nextBestFace = 0;
    for (uint32_t i = 0; i < indexCount; i += 3) {
        if (bestScore < 0.f) {
            // no verts in the cache are used by any unprocessed faces so
            // search all unprocessed faces for a new starting point
            for (; nextBestFace < faceCount; nextBestFace++) {
                uint32_t faceIndex = faceSorted[nextBestFace];
                if (processedFaceList[faceIndex] == 0) {
                    uint32_t face = faceIndex * 3;
                    float faceScore = 0.f;
                    for (uint32_t k = 0; k < 3; ++k) {
                        float32_t vertexScore = vertexDataList[vertexRemap[face + k]].score;
                        faceScore += vertexScore;
                    }

                    bestScore = faceScore;
                    bestFace = face;

                    nextBestFace++;
                    break; // we're searching a pre-sorted list, first one we find will be the best
                }
            }
            X_ASSERT(bestScore >= 0.f, "Best score must be greater than zero")(bestScore); 
        }

        processedFaceList[bestFace / 3] = 1;
        uint16_t entriesInCache1 = 0;

        // add bestFace to LRU cache and to newIndexList
        for (uint32_t v = 0; v < 3; ++v) {
            IndexType index = indexList[bestFace + v];
            newIndexList[i + v] = index;

            OptimizeVertexData& vertexData = vertexDataList[vertexRemap[bestFace + v]];

            if (vertexData.cachePos1 >= entriesInCache1) {
                vertexData.cachePos1 = entriesInCache1;
                cache1[entriesInCache1++] = vertexRemap[bestFace + v];

                if (vertexData.activeFaceListSize == 1) {
                    --vertexData.activeFaceListSize;
                    continue;
                }
            }

            X_ASSERT(vertexData.activeFaceListSize > 0, "vertexData.activeFaceListSize must be greater than zero")(vertexData.activeFaceListSize); 
            uint32_t* begin = &activeFaceList[vertexData.activeFaceListStart];
            uint32_t* end = &activeFaceList[vertexData.activeFaceListStart + vertexData.activeFaceListSize];
            uint32_t* it = std::find(begin, end, bestFace);
            X_ASSERT(it != end, "")(it, end); 

            core::Swap(*it, *(end - 1));
            --vertexData.activeFaceListSize;
            vertexData.score = FindVertexScore(vertexData.activeFaceListSize, vertexData.cachePos1, lruCacheSize);

            // need to re-sort the faces that use this vertex, as their score will change due to activeFaceListSize shrinking
            for (uint32_t* fi = begin; fi != end - 1; ++fi) {
                uint32_t faceIndex = *fi / 3;
                uint32_t n = faceReverseLookup[faceIndex];

                X_ASSERT(faceSorted[n] == faceIndex, "")(faceSorted[n], faceIndex); 

                // found it, now move it up
                while (n > 0) {
                    if (faceValenceSort(n, n - 1)) {
                        faceReverseLookup[faceSorted[n]] = n - 1;
                        faceReverseLookup[faceSorted[n - 1]] = n;
                        core::Swap(faceSorted[n], faceSorted[n - 1]);
                        n--;
                    }
                    else {
                        break;
                    }
                }
            }
        }

        // move the rest of the old verts in the cache down and compute their new scores
        for (uint32_t c0 = 0; c0 < entriesInCache0; ++c0) {
            OptimizeVertexData& vertexData = vertexDataList[cache0[c0]];

            if (vertexData.cachePos1 >= entriesInCache1) {
                vertexData.cachePos1 = entriesInCache1;
                cache1[entriesInCache1++] = cache0[c0];
                vertexData.score = FindVertexScore(vertexData.activeFaceListSize, vertexData.cachePos1, lruCacheSize);
                // don't need to re-sort this vertex... once it gets out of the cache, it'll have its original score
            }
        }

        // find the best scoring triangle in the current cache (including up to 3 that were just evicted)
        bestScore = -1.f;
        for (uint32_t c1 = 0; c1 < entriesInCache1; ++c1) {
            OptimizeVertexData& vertexData = vertexDataList[cache1[c1]];
            vertexData.cachePos0 = vertexData.cachePos1;
            vertexData.cachePos1 = kEvictedCacheIndex;
            for (uint32_t j = 0; j < vertexData.activeFaceListSize; ++j) {
                uint32_t face = activeFaceList[vertexData.activeFaceListStart + j];
                float faceScore = 0.f;
                for (uint32_t v = 0; v < 3; v++) {
                    OptimizeVertexData& faceVertexData = vertexDataList[vertexRemap[face + v]];
                    faceScore += faceVertexData.score;
                }
                if (faceScore > bestScore) {
                    bestScore = faceScore;
                    bestFace = face;
                }
            }
        }

        core::Swap(cache0, cache1);
        entriesInCache0 = core::Min(entriesInCache1, lruCacheSize);
    }

    return true;
}

template<typename IndexType>
bool FaceOptimize<IndexType>::ComputeVertexScores(void)
{
    for (uint32_t cacheSize = 0; cacheSize <= MAX_VERTEX_CACHE_SIZE; ++cacheSize) {
        for (uint32_t cachePos = 0; cachePos < cacheSize; ++cachePos) {
            vertexCacheScores_[cacheSize][cachePos] = ComputeVertexCacheScore(cachePos, cacheSize);
        }
    }

    for (uint32_t valence = 0; valence < MAX_PRECOMPUTED_VERTEX_VALENCE_SCORES; ++valence) {
        vertexValenceScores_[valence] = ComputeVertexValenceScore(valence);
    }

    return true;
}

template<typename IndexType>
float32_t FaceOptimize<IndexType>::FindVertexScore(uint32_t numActiveFaces, uint32_t cachePosition, uint32_t vertexCacheSize)
{
    if (numActiveFaces == 0) {
        // No tri needs this vertex!
        return -1.0f;
    }

    float score = 0.f;
    if (cachePosition < vertexCacheSize) {
        score += vertexCacheScores_[vertexCacheSize][cachePosition];
    }

    if (numActiveFaces < MAX_PRECOMPUTED_VERTEX_VALENCE_SCORES) {
        score += vertexValenceScores_[numActiveFaces];
    }
    else {
        score += ComputeVertexValenceScore(numActiveFaces);
    }

    return score;
}

template<typename IndexType>
X_INLINE float32_t FaceOptimize<IndexType>::FindVertexCacheScore(uint32_t cachePosition, uint32_t maxSizeVertexCache)
{
    return vertexCacheScores_[maxSizeVertexCache][cachePosition];
}

template<typename IndexType>
X_INLINE float32_t FaceOptimize<IndexType>::FindVertexValenceScore(uint32_t numActiveTris)
{
    return vertexValenceScores_[numActiveTris];
}

template<typename IndexType>
float32_t FaceOptimize<IndexType>::ComputeVertexCacheScore(int cachePosition, uint32_t vertexCacheSize)
{
    float32_t score = 0.0f;
    if (cachePosition < 0) {
        // Vertex is not in FIFO cache - no score.
    }
    else {
        if (cachePosition < 3) {
            // This vertex was used in the last triangle,
            // so it has a fixed score, whichever of the three
            // it's in. Otherwise, you can get very different
            // answers depending on whether you add
            // the triangle 1,2,3 or 3,1,2 - which is silly.
            score = FindVertexScore_LastTriScore;
        }
        else {
            X_ASSERT(cachePosition < static_cast<int32_t>(vertexCacheSize), "")(cachePosition, vertexCacheSize); 
            // Points for being high in the cache.
            const float32_t scaler = 1.0f / (vertexCacheSize - 3);
            score = 1.0f - (cachePosition - 3) * scaler;
            score = math<float>::pow(score, FindVertexScore_CacheDecayPower);
        }
    }

    return score;
}

template<typename IndexType>
float32_t FaceOptimize<IndexType>::ComputeVertexValenceScore(uint32_t numActiveFaces)
{
    float32_t score = 0.f;

    // Bonus points for having a low number of tris still to
    // use the vert, so we get rid of lone verts quickly.
    float32_t valenceBoost = math<float>::pow(static_cast<float32_t>(numActiveFaces), -FindVertexScore_ValenceBoostPower);
    score += FindVertexScore_ValenceBoostScale * valenceBoost;

    return score;
}

template class FaceOptimize<uint16_t>;
template class FaceOptimize<uint32_t>;

X_NAMESPACE_END