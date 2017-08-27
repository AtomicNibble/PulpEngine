#pragma once


X_NAMESPACE_BEGIN(model)

// This takes a index list and generates a new index list that 
// that tries to optermize GPU cache hits.

template <typename IndexType>
class FaceOptimize
{
	static const uint32_t MAX_VERTEX_CACHE_SIZE = 64;
	static const uint32_t MAX_PRECOMPUTED_VERTEX_VALENCE_SCORES = 64;

	static const float32_t FindVertexScore_CacheDecayPower;
	static const float32_t FindVertexScore_LastTriScore;
	static const float32_t FindVertexScore_ValenceBoostScale;
	static const float32_t FindVertexScore_ValenceBoostPower;

	struct OptimizeVertexData
	{
		OptimizeVertexData();

		float32_t	score;
		uint32_t    activeFaceListStart;
		uint32_t    activeFaceListSize;
		IndexType	cachePos0;
		IndexType	cachePos1;
	};

	template <typename T>
	struct IndexSortCompareIndexed
	{
		IndexSortCompareIndexed(const IndexType* pIndexData);

		bool operator()(T a, T b) const;

		const IndexType* pIndexData_;
	};

	template <typename T>
	struct FaceValenceSort
	{
		FaceValenceSort(const OptimizeVertexData* pVertexData);

		bool operator()(T a, T b) const;

		const OptimizeVertexData* pVertexData_;
	};


public:
	FaceOptimize(core::MemoryArenaBase* arena);
	~FaceOptimize() = default;

	bool OptimizeFaces(const IndexType* indexList, size_t indexCount, IndexType* newIndexList, uint16_t lruCacheSize);


private:
	bool ComputeVertexScores(void);

	float32_t FindVertexScore(uint32_t numActiveFaces, uint32_t cachePosition, uint32_t vertexCacheSize);

	X_INLINE float32_t FindVertexCacheScore(uint32_t cachePosition, uint32_t maxSizeVertexCache);
	X_INLINE float32_t FindVertexValenceScore(uint32_t numActiveTris);

	static float32_t ComputeVertexCacheScore(int cachePosition, uint32_t vertexCacheSize);
	static float32_t ComputeVertexValenceScore(uint32_t numActiveFaces);

private:
	float32_t vertexCacheScores_[MAX_VERTEX_CACHE_SIZE + 1][MAX_VERTEX_CACHE_SIZE];
	float32_t vertexValenceScores_[MAX_PRECOMPUTED_VERTEX_VALENCE_SCORES];

	core::MemoryArenaBase* arena_;
};





X_NAMESPACE_END