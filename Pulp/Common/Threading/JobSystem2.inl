

X_NAMESPACE_BEGIN(core)

namespace V2
{


	bool JobSystem::IsEmptyJob(Job* pJob) const
	{
		return pJob == nullptr;
	}

	bool JobSystem::HasJobCompleted(Job* pJob) const
	{
		return pJob->unfinishedJobs < 1;
	}



} // namespace V2

X_NAMESPACE_END
