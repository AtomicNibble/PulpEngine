#include "EngineCommon.h"
#include "RadixSort.h"


X_NAMESPACE_BEGIN(core)

namespace Sorting
{

	RadixSort::RadixSort(core::MemoryArenaBase* arena) :
		histogram_(arena, 256 * 4),
		buckets_(arena, 256),
		ranks_(arena),
		ranks2_(arena),
		pRanks_(nullptr),
		pRanks2_(nullptr),
		currentSize_(0),
		allocatedSize_(0),
		ranksValid_(false)
	{

	}

	RadixSort::~RadixSort()
	{

	}

	RadixSort& RadixSort::sort(const uint32* pInput, size_t num)
	{
		if (!num) {
			return *this;
		}

		checkSize(num);

		std::memset(histogram_.data(), 0, histogram_.size() * sizeof(uint32_t));

		uint8* pCur = union_cast<uint8*, const uint32_t*>(pInput);
		uint8* pEnd = &pCur[num * 4];
		uint32* pH0 = &histogram_[0];     // LSB         
		uint32* pH1 = &histogram_[256];                       
		uint32* pH2 = &histogram_[512];                       
		uint32* pH3 = &histogram_[768];   // MSB         

		bool alreadySorted = true;

		if (!ranksValid_)
		{
			uint32* pRunning = (uint32*)pInput;
			uint32 prevVal = *pRunning;

			// calculate the histogram while checking if already in order.
			while (pCur != pEnd)
			{
				const uint32 val = *pRunning;

				if (val < prevVal) {
					alreadySorted = false;
					break;
				}

				prevVal = val;

				// fill in the histogrma
				pH0[*pCur++]++; 
				pH1[*pCur++]++; 
				pH2[*pCur++]++; 
				pH3[*pCur++]++;
			}

			if (alreadySorted)
			{                                                                                  
				for (uint32_t i = 0; i < num; i++) {
					pRanks_[i] = i;
				}

				return *this;                                                                    
			}
		}
		else
		{
			// if the ranks are valid, see if anything has changes since last time.
			// if everythings still sorted no need to sort.
			// for example when sorting the scene if the camera don't move
			// the list we be the same and eveything sorted from last frames indexes already.
			uint32* pIndices = pRanks_;
			uint32 prevVal = pInput[*pIndices];

			while (pCur != pEnd)
			{
				const uint32 val = pInput[*pIndices++];

				if (val < prevVal) {
					alreadySorted = false;
					break;
				}

				prevVal = val;

				// fill in the histogrma
				pH0[*pCur++]++;
				pH1[*pCur++]++;
				pH2[*pCur++]++;
				pH3[*pCur++]++;
			}

			if (alreadySorted) {
				return *this;
			}
		}

		// finish of if we early out.
		while (pCur != pEnd)
		{                                                                                                              
			pH0[*pCur++]++; 
			pH1[*pCur++]++;
			pH2[*pCur++]++; 
			pH3[*pCur++]++;                                    
		}

		// do the passes.
		for (uint32 pass = 0; pass < 4; pass++)
		{
			bool performPass = true;

			const uint32* pCurCount = &histogram_[pass << 8];

			// check if the block of 8 bits are all zero for the input values.
			// if so can skip this pass
			{
				uint8 uniqueVal = *(reinterpret_cast<const uint8*>(pInput) + pass);

				if (pCurCount[uniqueVal] == num) {
					performPass = false;
				}
			}

			if (performPass)
			{
				// setup bucket pointers.
				{
					uint32_t* pBucketDest = pRanks2_;
					for (uint32 i = 0; i < 256; pBucketDest += pCurCount[i++]) {
						buckets_[i] = pBucketDest;
					}
				}

				// create index.
				const uint8_t* pInputBytes = reinterpret_cast<const uint8_t*>(pInput);
				pInputBytes += pass; // offset, then we move 4 bytes each time.

				if (!ranksValid_)
				{
					for (uint32 i = 0; i < num; i++) {
						*buckets_[pInputBytes[i << 2]]++ = i;
					}

					ranksValid_ = true;
				}
				else
				{
					const uint32* pIndices = pRanks_;
					const uint32* pIndicesEnd = &ranks_[num];

					while (pIndices != pIndicesEnd) {
						const uint32 id = *pIndices++;
						*buckets_[pInputBytes[id << 2]]++ = id;
					}

				}

				core::Swap(pRanks_, pRanks2_);
			}
		}

		return *this;
	}

	std::pair<const uint32_t*, size_t> RadixSort::getRanks(void) const
	{
		X_ASSERT_NOT_NULL(pRanks_);

		return std::make_pair(pRanks_, currentSize_);
	}

	void RadixSort::checkSize(size_t num)
	{
		if (num != currentSize_)
		{
			// bigger?
			if (num > currentSize_)
			{
				if (num > allocatedSize_)
				{
					ranks_.resize(num);
					ranks2_.resize(num);

					// set pointers
					pRanks_ = ranks_.data();
					pRanks2_ = ranks2_.data();
				}

				std::memset(ranks_.data(), 0, ranks_.size() * sizeof(uint32_t));
				std::memset(ranks2_.data(), 0, ranks2_.size() * sizeof(uint32_t));
			}

			// invalid after size change.
			ranksValid_ = false; 
		}

	}


} // namespace Sorting

X_NAMESPACE_END