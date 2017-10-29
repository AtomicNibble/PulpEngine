#pragma once

#ifndef X_PROFILER_TYPES_H_
#define X_PROFILER_TYPES_H_

#include <Containers\FixedRingBuffer.h>

X_NAMESPACE_BEGIN(core)

//
// Profile me baby
// we want to have sexy profiling.
//
// we will have a macro that we just place at the top of the function
// that will be static so that we can collect data over time.
// but also it will create a non static local
// that will record the current time on scope enter and log elapsed time on scope leave.
//
// we want to be able to tell the time of the function not including sub functions
// 
// w
// 
// 
// 

#define X_PROFILE_HISTORY_SIZE 64

namespace profiler
{

	X_DECLARE_ENUM8(SubSys)(
		UNCLASSIFIED,
		CORE,
		ENGINE3D,
		FONT,
		INPUT,
		RENDER,
		SCRIPT,
		SOUND,
		GAME,
		PHYSICS,
		NETWORK,
		VIDEO,
		TOOL,
		UNITTEST
	);


#if X_ENABLE_PROFILER 

	template<typename T, size_t N>
	class ProfilerHistory : public core::FixedRingBuffer<T, N>
	{
		typedef core::FixedRingBuffer<T, N> BaseT;

	public:

		X_INLINE T getAvg(void) const
		{
			return core::accumulate(BaseT::begin(), BaseT::end(), 0) / BaseT::size();
		}

		X_INLINE T getMin(void) const
		{
			T min = (T)0;
			auto it = BaseT::begin();
			for (min = (*it); it != BaseT::end(); ++it) {
				if (min > (*it)) {
					min = (*it);
				}
			}
			return min;
		}

		X_INLINE T getMax(void) const
		{
			T max = (T)0;
			auto it = BaseT::begin();
			for (max = (*it); it != BaseT::end(); ++it) {
				if (max < (*it)) {
					max = (*it);
				}
			}
			return max;
		}
	};


	class XProfileData
	{
	public:
		X_DECLARE_ENUM8(Type) (
			SingleShot,
			History
		);

	protected:
		XProfileData(ICore* pCore, const core::SourceInfo& sourceInfo, const char* pNickName, SubSys::Enum sys, Type::Enum type) :
			sourceInfo_(sourceInfo),
			pNickName_(pNickName),
			pCore_(pCore),
			time_(0),
			timeSelf_(0),
			pParent_(nullptr),
			callCount_(0),
			threadID_(core::Thread::GetCurrentID()),
			hasChildren_(0),
			subSystem_(sys),
			type_(type)
		{
		}

	public:
		XProfileData(ICore* pCore, const core::SourceInfo& sourceInfo, const char* pNickName, SubSys::Enum sys) :
			XProfileData(pCore, sourceInfo, pNickName, sys, Type::SingleShot)
		{
			if (gEnv->pProfiler) {
				gEnv->pProfiler->AddProfileData(this);
			}
		}

		X_INLINE Type::Enum getType(void);

	public:
		core::SourceInfo sourceInfo_;
		const char*		 pNickName_;

		ICore*			pCore_;

		uint64_t		time_;				// time spent in function including children
		uint64_t		timeSelf_;			// time spent in function not including children.

		XProfileData*	pParent_;			// parent node

		int32_t			callCount_;
		int32_t			threadID_;

		uint8			hasChildren_;
		SubSys::Enum	subSystem_;
	protected:
		Type::Enum		type_;
	};

	X_ENSURE_SIZE(SubSys::Enum, 1);
	
	X_INLINE XProfileData::Type::Enum XProfileData::getType(void)
	{
		return type_;
	}


	class XProfileDataHistory : public XProfileData
	{
		typedef ProfilerHistory<uint64_t, X_PROFILE_HISTORY_SIZE> TotalTimes;
		typedef ProfilerHistory<uint64_t, X_PROFILE_HISTORY_SIZE> SelfTimes;
		typedef ProfilerHistory<int32_t, X_PROFILE_HISTORY_SIZE> CallCounts;

	public:
		XProfileDataHistory(ICore* pCore, const core::SourceInfo& sourceInfo, const char* pNickName, SubSys::Enum sys) :
			XProfileData(pCore, sourceInfo, pNickName, sys, Type::History)
		{
			if (gEnv->pProfiler) {
				gEnv->pProfiler->AddProfileData(this);
			}
		}

		X_INLINE void onFrameBegin(void);

	protected:
		uint64_t		sumTime_;			// sum of each frame value
		uint64_t		sumTimeSelf_;		// ''

		TotalTimes totalTimeHistory_;
		SelfTimes selfTimeHistory_;
		CallCounts callCountHistory_;
	};


	X_INLINE void XProfileDataHistory::onFrameBegin(void)
	{
		// add frame values
		sumTime_ += time_;
		sumTimeSelf_ += timeSelf_;

		// log values.
		totalTimeHistory_.append(time_);
		selfTimeHistory_.append(timeSelf_);
		callCountHistory_.append(callCount_);

		// clear frame values
		time_ = 0llu;
		timeSelf_ = 0llu;
		callCount_ = 0;
	}

	class XProfileScope
	{
		X_NO_ASSIGN(XProfileScope);
		X_NO_COPY(XProfileScope);

	public:
		XProfileScope(XProfileData* pData)
		{
			if (gEnv->pProfiler)
			{
				pData_ = pData;
				gEnv->pProfiler->ScopeBegin(this);
			}
			else {
				pData_ = nullptr;
			}
		}

		~XProfileScope()
		{
			if (pData_) {
				gEnv->pProfiler->ScopeEnd(this);
			}
		}

	public:
		uint64_t	start_;
		uint64_t	excludeTime_;

	//	XProfileScope*		pParent_;
		XProfileData*		pData_;
	};

#endif // !#X_ENABLE_PROFILER 

} // namespace profiler


#if X_ENABLE_PROFILER 

#define X_PROFILE_BEGIN(pNickName, sys) \
	static core::profiler::XProfileDataHistory		__Profiledata(gEnv->pCore, X_SOURCE_INFO, pNickName, sys); \
	core::profiler::XProfileScope					__ProfileLogCall(&__Profiledata);

#define X_PROFILE_NO_HISTORY_BEGIN(pNickName, sys) \
	static core::profiler::XProfileData				__Profiledata(gEnv->pCore, X_SOURCE_INFO, pNickName, sys); \
	core::profiler::XProfileScope					__ProfileLogCall(&__Profiledata);

#else

#define X_PROFILE_BEGIN(nickname, sys) X_UNUSED(nickname), X_UNUSED(sys)
#define X_PROFILE_NO_HISTORY_BEGIN(nickname, sys) X_UNUSED(nickname), X_UNUSED(sys)

#endif // !X_ENABLE_PROFILER


X_NAMESPACE_END


#endif // !X_PROFILER_TYPES_H_
