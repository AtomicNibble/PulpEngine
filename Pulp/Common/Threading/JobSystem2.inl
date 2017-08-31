

X_NAMESPACE_BEGIN(core)

namespace V2
{



	template <typename JobData>
	static inline void parallel_for_job(JobSystem& jobSys, size_t threadIdx, Job* job, void* jobData)
	{
		const JobData* data = static_cast<const JobData*>(jobData);
		const typename JobData::SplitterType& splitter = data->splitter_;

		if (splitter.Split<JobData::DataType>(data->count_))
		{
			// split in two
			const size_t leftCount = data->count_ / 2u;
			const JobData leftData(data->data_, leftCount, data->pFunction_, splitter);
			Job* left = jobSys.CreateJobAsChild(job, &parallel_for_job<JobData>, leftData JOB_SYS_SUB_PASS(job->subSystem));
			jobSys.Run(left);

			const size_t rightCount = data->count_ - leftCount;
			const JobData rightData(data->data_ + leftCount, rightCount, data->pFunction_, splitter);
			Job* right = jobSys.CreateJobAsChild(job, &parallel_for_job<JobData>, rightData JOB_SYS_SUB_PASS(job->subSystem));
			jobSys.Run(right);
		}
		else
		{
			(data->pFunction_)(data->data_, data->count_);
		}
	}

	template <typename JobData>
	static inline void parallel_for_member_job(JobSystem& jobSys, size_t threadIdx, Job* job, void* jobData)
	{
		X_UNUSED(threadIdx);

		const JobData* data = static_cast<const JobData*>(jobData);
		const typename JobData::SplitterType& splitter = data->splitter_;

		if (splitter.template Split<typename JobData::DataType>(data->count_))
		{
			// split in two
			const uint32_t leftCount = data->count_ / 2u;
			const JobData leftData(data->delagte_, data->data_, leftCount, splitter);
			Job* left = jobSys.CreateJobAsChild(job, &parallel_for_member_job<JobData>, leftData JOB_SYS_SUB_PASS(job->subSystem));
			jobSys.Run(left);

			const uint32_t rightCount = data->count_ - leftCount;
			const JobData rightData(data->delagte_, data->data_ + leftCount, rightCount, splitter);
			Job* right = jobSys.CreateJobAsChild(job, &parallel_for_member_job<JobData>, rightData JOB_SYS_SUB_PASS(job->subSystem));
			jobSys.Run(right);
		}
		else
		{
			data->delagte_.Invoke(data->data_, data->count_);
		}
	}


	// ----------------------------------------------

	X_INLINE CountSplitter::CountSplitter(size_t count) :
		count_(count)
	{
	}

	template <typename T>
	X_INLINE bool CountSplitter::Split(size_t count) const
	{
		return (count > count_);
	}

	// =============================================

	X_INLINE CountSplitter32::CountSplitter32(uint32_t count) :
		count_(count)
	{
	}

	template <typename T>
	X_INLINE bool CountSplitter32::Split(uint32_t count) const
	{
		return (count > count_);
	}


	// =============================================

	X_INLINE DataSizeSplitter::DataSizeSplitter(size_t size) :
		size_(size)
	{
	}

	template <typename T>
	X_INLINE bool DataSizeSplitter::Split(size_t count) const
	{
		return (count * sizeof(T) > size_);
	}

	// =============================================

	X_INLINE bool JobSystem::IsEmptyJob(Job* pJob) const
	{
		return pJob == nullptr;
	}

	X_INLINE bool JobSystem::HasJobCompleted(Job* pJob) const
	{
		return pJob->unfinishedJobs == 0;
	}

	X_INLINE Job* JobSystem::CreateJob(JobFunction::Pointer function, void* pData JOB_SYS_SUB_PARAM)
	{
		Job* job = CreateJob(function JOB_SYS_SUB_PASS(subSystem));
		job->pArgData = pData;
		return job;
	}

	X_INLINE Job* JobSystem::CreateJobAsChild(Job* pParent, JobFunction::Pointer function, void* pData JOB_SYS_SUB_PARAM)
	{
		Job* job = CreateJobAsChild(pParent, function JOB_SYS_SUB_PASS(subSystem));
		job->pArgData = pData;
		return job;
	}

	template<typename DataT, typename>
	X_INLINE Job* JobSystem::CreateJob(JobFunction::Pointer function, const DataT& data JOB_SYS_SUB_PARAM)
	{
		static_assert((sizeof(DataT) <= Job::PAD_SIZE), " does not fit in job data, pass as void");
		static_assert(core::compileTime::IsTrivialDestruct<DataT>::Value, " type is not trivially destructible");

		Job* job = CreateJob(function JOB_SYS_SUB_PASS(subSystem));
		job->pArgData = &job->pad;
		::memcpy(job->pad, &data, sizeof(DataT));
		return job;
	}


	template<typename DataT, typename>
	X_INLINE Job* JobSystem::CreateJobAsChild(Job* pParent, JobFunction::Pointer function, const DataT& data JOB_SYS_SUB_PARAM)
	{
		static_assert((sizeof(DataT) <= Job::PAD_SIZE), " does not fit in job data, pass as void");
		static_assert(core::compileTime::IsTrivialDestruct<DataT>::Value, " type is not trivially destructible");

		Job* job = CreateJobAsChild(pParent, function JOB_SYS_SUB_PASS(subSystem));
		job->pArgData = &job->pad;
		::memcpy(job->pad, &data, sizeof(DataT));
		return job;
	}


	template <typename T, typename SplitterT>
	X_INLINE Job* JobSystem::parallel_for(T* data, size_t count,
		typename parallel_for_job_data<T, SplitterT>::DataJobFunctionPtr function, const SplitterT& splitter JOB_SYS_SUB_PARAM)
	{
		typedef parallel_for_job_data<T, SplitterT> JobData;
		JobData jobData(data, count, function, splitter);

		Job* job = CreateJob<JobData>(&parallel_for_job<JobData>, jobData JOB_SYS_SUB_PASS(subSystem));

		return job;
	}

	template <typename T, typename SplitterT>
	X_INLINE Job* JobSystem::parallel_for_child(Job* pParent, T* data, size_t count,
		typename parallel_for_job_data<T, SplitterT>::DataJobFunctionPtr function, const SplitterT& splitter JOB_SYS_SUB_PARAM)
	{
		typedef parallel_for_job_data<T, SplitterT> JobData;
		JobData jobData(data, count, function, splitter);

		Job* job = CreateJobAsChild<JobData>(pParent, &parallel_for_job<JobData>, jobData JOB_SYS_SUB_PASS(subSystem));

		return job;
	}

	template <typename ClassType, typename T, typename SplitterT>
	X_INLINE Job* JobSystem::parallel_for_member(typename parallel_for_member_job_data<ClassType, T, SplitterT>::FunctionDelagte del,
		T* data, uint32_t count, const SplitterT& splitter JOB_SYS_SUB_PARAM)
	{
		typedef parallel_for_member_job_data<ClassType, T, SplitterT> JobData;
		JobData jobData(del, data, count, splitter);

		Job* job = CreateJob<JobData>(&parallel_for_member_job<JobData>, jobData JOB_SYS_SUB_PASS(subSystem));

		return job;
	}

	template <typename ClassType, typename T, typename SplitterT>
	X_INLINE Job* JobSystem::parallel_for_member_child(Job* pParent, typename parallel_for_member_job_data<ClassType, T, SplitterT>::FunctionDelagte del,
		T* data, uint32_t count, const SplitterT& splitter JOB_SYS_SUB_PARAM)
	{
		typedef parallel_for_member_job_data<ClassType, T, SplitterT> JobData;
		JobData jobData(del, data, count, splitter);

		Job* job = CreateJobAsChild<JobData>(pParent, &parallel_for_member_job<JobData>, jobData JOB_SYS_SUB_PASS(subSystem));

		return job;
	}

	template<typename ClassType>
	X_INLINE Job* JobSystem::CreateMemberJob(ClassType* pInst, typename member_function_job_data<ClassType>::MemberFunctionPtr pFunction,
		void* pJobData JOB_SYS_SUB_PARAM)
	{
		typedef member_function_job_data<ClassType> MemberCallerData;
		MemberCallerData jobData(pInst, pFunction, pJobData);

		Job* job = CreateJob<MemberCallerData>(&member_function_job<MemberCallerData>, jobData JOB_SYS_SUB_PASS(subSystem));

		return job;
	}


	template<typename ClassType, typename DataT, typename>
	X_INLINE Job* JobSystem::CreateMemberJob(ClassType* pInst,
		typename member_function_job_copy_data<ClassType, DataT>::MemberFunctionPtr pFunction,
		DataT& data JOB_SYS_SUB_PARAM)
	{
		typedef member_function_job_copy_data<ClassType, DataT> MemberCallerData;

		static_assert((sizeof(DataT) <= MemberCallerData::PAD_SIZE), " does not fit in job data, pass as void");
		static_assert(core::compileTime::IsTrivialDestruct<DataT>::Value, " type is not trivially destructible");

		MemberCallerData jobData(pInst, pFunction, data);

		Job* job = CreateJob<MemberCallerData>(&member_function_job_copy<MemberCallerData>, jobData JOB_SYS_SUB_PASS(subSystem));

		return job;
	}

	template<typename ClassType>
	X_INLINE Job* JobSystem::CreateMemberJobAndRun(ClassType* pInst, typename member_function_job_data<ClassType>::MemberFunctionPtr pFunction,
		void* pJobData JOB_SYS_SUB_PARAM)
	{
		Job* pJob = CreateMemberJob(pInst, pFunction, pJobData JOB_SYS_SUB_PASS(subSystem));
		Run(pJob);
		return pJob;
	}


	template<typename ClassType, typename DataT, typename>
	X_INLINE Job* JobSystem::CreateMemberJobAndRun(ClassType* pInst,
		typename member_function_job_copy_data<ClassType, DataT>::MemberFunctionPtr pFunction,
		DataT& data JOB_SYS_SUB_PARAM)
	{
		Job* pJob = CreateMemberJob(pInst, pFunction, data JOB_SYS_SUB_PASS(subSystem));
		Run(pJob);
		return pJob;
	}


	template<typename ClassType>
	X_INLINE Job* JobSystem::CreateMemberJobAsChild(Job* pParent, ClassType* pInst,
		typename member_function_job_data<ClassType>::MemberFunctionPtr pFunction, void* pJobData JOB_SYS_SUB_PARAM)
	{
		typedef member_function_job_data<ClassType> MemberCallerData;
		MemberCallerData jobData(pInst, pFunction, pJobData);

		Job* job = CreateJobAsChild<MemberCallerData>(pParent, &member_function_job<MemberCallerData>, jobData JOB_SYS_SUB_PASS(subSystem));

		return job;
	}

	template<typename ClassType, typename DataT, typename>
	X_INLINE Job* JobSystem::CreateMemberJobAsChild(Job* pParent, ClassType* pInst,
		typename member_function_job_copy_data<ClassType, DataT>::MemberFunctionPtr pFunction,
		DataT& data JOB_SYS_SUB_PARAM)
	{
		typedef member_function_job_copy_data<ClassType, DataT> MemberCallerData;

		static_assert((sizeof(DataT) <= MemberCallerData::PAD_SIZE), " does not fit in job data, pass as void");
		static_assert(core::compileTime::IsTrivialDestruct<DataT>::Value, " type is not trivially destructible");

		MemberCallerData jobData(pInst, pFunction, data);

		Job* job = CreateJobAsChild<MemberCallerData>(pParent, &member_function_job_copy<MemberCallerData>, jobData JOB_SYS_SUB_PASS(subSystem));

		return job;
	}


	// =============================================

	X_INLINE void JobSystem::AddContinuation(Job* ancestor, Job* continuation, bool runInline)
	{
		const int32_t count = (atomic::Increment(&ancestor->continuationCount) - 1);
		X_ASSERT(count < Job::MAX_CONTINUATIONS, "Can't add conitnation, list is full")(Job::MAX_CONTINUATIONS, count);

		X_ASSERT(continuation->pParent != ancestor, "Contination can't have ancestor as parent")();

#if X_ENABLE_JOBSYS_PARENT_CHECK
		// lets check all the way up to see if continuation is a child.
		{
			auto* pParent = continuation->pParent;
			while (pParent) {
				X_ASSERT(pParent  != ancestor, "Contination can't have ancestor as parent")();
				pParent = pParent->pParent;
			}
		}
#endif // !X_ENABLE_JOBSYS_PARENT_CHECK

		size_t threadIdx = GetThreadIndex();
		ThreadJobAllocator* pThreadAlloc = GetWorkerThreadAllocator(threadIdx);

		ptrdiff_t jobIdx = continuation - pThreadAlloc->jobs;

		X_ASSERT(jobIdx >= 0 && jobIdx < ThreadQue::MAX_NUMBER_OF_JOBS, "Job index is invalid")(jobIdx, ThreadQue::MAX_NUMBER_OF_JOBS);

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

	X_INLINE Job* JobSystem::CreateEmtpyJob(JOB_SYS_SUB_PARAM_SINGLE)
	{
		return CreateJob(EmptyJob, nullptr JOB_SYS_SUB_PASS(subSystem));
	}

	X_INLINE Job* JobSystem::CreateEmtpyJobAsChild(Job* pPaerent JOB_SYS_SUB_PARAM)
	{
		return CreateJobAsChild(pPaerent, EmptyJob, nullptr JOB_SYS_SUB_PASS(subSystem));
	}



#if X_ENABLE_JOBSYS_PROFILER


	X_INLINE const int32_t JobQueueHistory::FrameHistory::getMaxreadIdx(void) const
	{
		return top_;
	}

	X_INLINE const JobQueueHistory::FrameHistoryArr& JobQueueHistory::getHistory(void) const
	{
		return frameHistory_;
	}

	X_INLINE int32_t JobSystem::getCurrentProfilerIdx(void) const
	{
		return currentHistoryIdx_;
	}

	X_INLINE const JobSystem::ProfilerThreadTimelinesArr& JobSystem::GetTimeLines(void) const
	{
		return pTimeLines_;
	}

	X_INLINE const JobSystem::ProfilerStatsArr& JobSystem::GetStats(void) const
	{
		return stats_;
	}

#endif // !X_ENABLE_JOBSYS_PROFILER


} // namespace V2

X_NAMESPACE_END
