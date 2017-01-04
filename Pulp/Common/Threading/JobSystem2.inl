

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

	X_INLINE CountSplitter32::CountSplitter32(uint32_t count)
		: count_(count)
	{
	}

	template <typename T>
	X_INLINE bool CountSplitter32::Split(uint32_t count) const
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

	template<typename DataT, typename>
	X_INLINE Job* JobSystem::CreateJob(JobFunction::Pointer function, const DataT& data)
	{
		static_assert((sizeof(DataT) <= Job::PAD_SIZE), " does not fit in job data, pass as void");
		static_assert(std::is_trivially_destructible<DataT>::value, " type is not trivially destructible");

		Job* job = CreateJob(function);
		job->pArgData = &job->pad;
		::memcpy(job->pad, &data, sizeof(DataT));
		return job;
	}


	template<typename DataT, typename>
	X_INLINE Job* JobSystem::CreateJobAsChild(Job* pParent, JobFunction::Pointer function, const DataT& data)
	{
		static_assert((sizeof(DataT) <= Job::PAD_SIZE), " does not fit in job data, pass as void");
		static_assert(std::is_trivially_destructible<DataT>::value, " type is not trivially destructible");

		Job* job = CreateJobAsChild(pParent, function);
		job->pArgData = &job->pad;
		::memcpy(job->pad, &data, sizeof(DataT));
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

	template <typename ClassType, typename T, typename SplitterT>
	X_INLINE Job* JobSystem::parallel_for_member(typename parallel_for_member_job_data<ClassType, T, SplitterT>::FunctionDelagte del,
		T* data, uint32_t count, const SplitterT& splitter)
	{
		typedef parallel_for_member_job_data<ClassType, T, SplitterT> JobData;
		JobData jobData(del, data, count, splitter);

		Job* job = CreateJob<JobData>(&parallel_for_member_job<JobData>, jobData);

		return job;
	}

	template <typename ClassType, typename T, typename SplitterT>
	X_INLINE Job* JobSystem::parallel_for_member_child(Job* pParent, typename parallel_for_member_job_data<ClassType, T, SplitterT>::FunctionDelagte del,
		T* data, uint32_t count, const SplitterT& splitter)
	{
		typedef parallel_for_member_job_data<ClassType, T, SplitterT> JobData;
		JobData jobData(del, data, count, splitter);

		Job* job = CreateJobAsChild<JobData>(pParent, &parallel_for_member_job<JobData>, jobData);

		return job;
	}

	template<typename ClassType>
	X_INLINE Job* JobSystem::CreateMemberJob(ClassType* pInst, typename member_function_job_data<ClassType>::MemberFunctionPtr pFunction,
		void* pJobData)
	{
		typedef member_function_job_data<ClassType> MemberCallerData;
		MemberCallerData jobData(pInst, pFunction, pJobData);

		Job* job = CreateJob<MemberCallerData>(&member_function_job<MemberCallerData>, jobData);

		return job;
	}

	template<typename ClassType>
	X_INLINE Job* JobSystem::CreateMemberJobAsChild(Job* pParent, ClassType* pInst,
		typename member_function_job_data<ClassType>::MemberFunctionPtr pFunction, void* pJobData)
	{
		typedef member_function_job_data<ClassType> MemberCallerData;
		MemberCallerData jobData(pInst, pFunction, pJobData);

		Job* job = CreateJobAsChild<MemberCallerData>(pParent, &member_function_job<MemberCallerData>, jobData);

		return job;
	}

	template<typename ClassType, typename DataT, typename>
	X_INLINE Job* JobSystem::CreateMemberJobAsChild(Job* pParent, ClassType* pInst,
		typename member_function_job_copy_data<ClassType, DataT>::MemberFunctionPtr pFunction, 
		typename DataT& data)
	{
		typedef member_function_job_copy_data<ClassType, DataT> MemberCallerData;

		static_assert((sizeof(DataT) <= MemberCallerData::PAD_SIZE), " does not fit in job data, pass as void");
		static_assert(std::is_trivially_destructible<DataT>::value, " type is not trivially destructible");

		MemberCallerData jobData(pInst, pFunction, data);

		Job* job = CreateJobAsChild<MemberCallerData>(pParent, &member_function_job_copy<MemberCallerData>, jobData);

		return job;
	}


	// =============================================

	X_INLINE void JobSystem::AddContinuation(Job* ancestor, Job* continuation, bool runInline)
	{
		const int32_t count = (atomic::Increment(&ancestor->continuationCount) - 1);
		X_ASSERT(count < Job::MAX_CONTINUATIONS, "Can't add conitnation, list is full")(Job::MAX_CONTINUATIONS, count);


		size_t threadIdx = GetThreadIndex();
		ThreadJobAllocator* pThreadAlloc = GetWorkerThreadAllocator(threadIdx);

		size_t jobIdx = continuation - pThreadAlloc->jobs;

		X_ASSERT(jobIdx < ThreadQue::MAX_NUMBER_OF_JOBS, "Job index is invalid")(jobIdx, ThreadQue::MAX_NUMBER_OF_JOBS);

		Job::JobId id;
		id.threadIdx = threadIdx;
		id.jobIdx = jobIdx;
		ancestor->continuations[count] = id;

		if (runInline) {
			ancestor->runFlags = core::bitUtil::SetBit(ancestor->runFlags, count);
		}
	}

	X_INLINE void JobSystem::EmptyJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
	{
		X_UNUSED(jobSys);
		X_UNUSED(threadIdx);
		X_UNUSED(pJob);
		X_UNUSED(pData);

	}

} // namespace V2

X_NAMESPACE_END
