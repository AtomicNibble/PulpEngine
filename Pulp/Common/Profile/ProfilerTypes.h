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

#define X_PROFILE_ENABLE_THREADS 1
#define X_PROFILE_HISTORY_SIZE 64

X_DECLARE_ENUM8(ProfileSubSys)(
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
	TOOL,
	UNITTEST
);


template<typename T, size_t N>
class ProfilerHistory : public core::FixedRingBuffer<T, N>
{
public: 

	T getAvg(void) const 
	{
		T avg = (T)0;
		const_iterator it = begin();
		for (; it != end(); ++it) 
			avg += (*it);
			
		return avg / (T)size();
	}

	T getMin(void) const 
	{
		T min = (T)0;
		const_iterator it = begin();
		for (min = (*it); it != end(); ++it)
			if (min > (*it))
				min = (*it);
		return min;
	}

	T getMax(void) const 
	{
		T max = (T)0;
		const_iterator it = begin();
		for (max = (*it); it != end(); ++it)
			if (max < (*it))
				max = (*it);
		return max;
	}
};


class XProfileData
{
public:

	XProfileData(ICore*	pCore, const char* function, const char* nick, ProfileSubSys::Enum sys) :
		time_(0),
		timeSelf_(0),
		sumTime_(0),
		sumTimeSelf_(0),
		pParent_(nullptr),
		nickName_(nick),
		functionName_(function),
		pCore_(pCore),
		callCount_(0)
#ifdef X_PROFILE_ENABLE_THREADS
		, threadID_(0)
#endif
		, hasChildren_(0)
		, subSystem_(sys)
	{
		X_ASSERT_NOT_NULL(pCore);
		X_ASSERT_NOT_NULL(pCore->GetIProfileSys());

		// add it.
		pCore->GetIProfileSys()->AddProfileData(this);
	}

	friend class XProfileSys;
	friend class XProfileScope;

protected:
	const char*		nickName_;			// the name baybe
	const char*		functionName_;		// the name baybe
	ICore*			pCore_;
										// sum of all calls in a frame.
	uint64_t		time_;				// time spent in function including children
	uint64_t		timeSelf_;			// time spent in function not including children.

	uint64_t		sumTime_;			// sum of each frame value
	uint64_t		sumTimeSelf_;		// ''

	uint64_t		lastFrame_;

	XProfileData*	pParent_;			// parent node

	int				callCount_;			// reset each frame
#ifdef X_PROFILE_ENABLE_THREADS
	int				threadID_;			
#endif // X_PROFILE_ENABLE_THREADS

	uint8			hasChildren_;
	ProfileSubSys::Enum subSystem_;

	uint16_t pad;

	typedef ProfilerHistory<float, X_PROFILE_HISTORY_SIZE> TotalTimes;
	typedef ProfilerHistory<float, X_PROFILE_HISTORY_SIZE> SelfTimes;
	typedef ProfilerHistory<int, X_PROFILE_HISTORY_SIZE> CallCounts;

	TotalTimes totalTimeHistory_;
	SelfTimes selfTimeHistory_;
	CallCounts callCountHistory_;
};

X_ENSURE_SIZE(ProfileSubSys::Enum,1);

class XProfileScope
{
public:
	XProfileScope(XProfileData* pData)
	{
		// we need start updating the stats
		// we are best sending this to the profile system since we need more
		// info than we have in current scope.
		// like stack :D	
		if (gEnv->IsProfilerEnabled())
		{
			pData_ = pData;
			gEnv->profileScopeStart(this);
		}
		else {
			pData_ = nullptr;
		}
	}

	~XProfileScope()
	{
		if (pData_)
			gEnv->profileScopeEnd(this);
	}

	friend class XProfileSys;
private:
	X_NO_ASSIGN(XProfileScope);

	uint64_t	start_;
	uint64_t	excludeTime_;

	XProfileScope* pParent_;
	XProfileData* pData_;
};


#if X_ENABLE_PROFILER 

#define X_PROFILE_BEGIN(nickname, sys) \
	static core::XProfileData __Profiledata(gEnv->pCore, __FUNCTION__, nickname, sys); \
	core::XProfileScope       __ProfileLogCall(&__Profiledata);

#else

#define X_PROFILE_BEGIN(nickname, sys) X_UNUSED(nickname), X_UNUSED(sys)


#endif // !X_ENABLE_PROFILER

X_NAMESPACE_END


#endif // !X_PROFILER_TYPES_H_
