

X_NAMESPACE_BEGIN(core)

namespace V2
{
	X_INLINE CountSplitter::CountSplitter(size_t count)
		: count_(count)
	{
	}

	template <typename T>
	X_INLINE bool CountSplitter::Split(size_t count) const
	{
		return (count > count_);
	}


	// =============================================

	X_INLINE DataSizeSplitter:: DataSizeSplitter(size_t size)
		: size_(size)
	{
	}

	template <typename T>
	X_INLINE bool DataSizeSplitter::Split(size_t count) const
	{
		return (count*sizeof(T) > size_);
	}

	// =============================================

	X_INLINE bool JobSystem::IsEmptyJob(Job* pJob) const
	{
		return pJob == nullptr;
	}

	X_INLINE bool JobSystem::HasJobCompleted(Job* pJob) const
	{
		return pJob->unfinishedJobs < 1;
	}

	X_INLINE Job* JobSystem::CreateJob(JobFunction::Pointer function, void* pData)
	{
		Job* job = CreateJob(function);
		job->pArgData = pData;
		return job;
	}

	X_INLINE Job* JobSystem::CreateJobAsChild(Job* pParent, JobFunction::Pointer function, void* pData)
	{
		Job* job = CreateJobAsChild(pParent, function);
		job->pArgData = pData;
		return job;
	}

	template<typename T>
	X_INLINE Job* JobSystem::CreateJob(JobFunction::Pointer function, const T& data)
	{
		static_assert((sizeof(T) <= Job::PAD_SIZE), " does not fit in job data, pass as void");

		Job* job = CreateJob(function);
		job->pArgData = &job->pad;
		::memcpy(job->pad, &data, sizeof(T));
		return job;
	}


	template<typename T>
	X_INLINE Job* JobSystem::CreateJobAsChild(Job* pParent, JobFunction::Pointer function, const T& data)
	{
		static_assert((sizeof(T) <= Job::PAD_SIZE), " does not fit in job data, pass as void");

		Job* job = CreateJobAsChild(pParent, function);
		job->pArgData = &job->pad;
		::memcpy(job->pad, &data, sizeof(T));
		return job;
	}


	template <typename T, typename SplitterT>
	X_INLINE Job* JobSystem::parallel_for(T* data, size_t count,
		typename parallel_for_job_data<T, SplitterT>::DataJobFunctionPtr function, const SplitterT& splitter)
	{
		typedef parallel_for_job_data<T, SplitterT> JobData;
		JobData jobData(data, count, function, splitter);

		Job* job = CreateJob<JobData>(&parallel_for_job<JobData>, jobData);

		return job;
	}

	template<typename ClassType>
	X_INLINE Job* JobSystem::CreateJobMemberFunc(ClassType* pInst, typename member_function_job_data<ClassType>::MemberFunctionPtr pFunction)
	{
		typedef member_function_job_data<ClassType> MemberCallerData;
		MemberCallerData jobData(pInst, pFunction);

		Job* job = CreateJob<MemberCallerData>(&member_function_job<MemberCallerData>, jobData);

		return job;
	}

} // namespace V2

X_NAMESPACE_END
