#include <EngineCommon.h>
#include "Process.h"

X_NAMESPACE_BEGIN(core)

Process::Process(HANDLE process, ProcessId  processID) :
    process_(process),
    processID_(processID)
{
}

Process::Process() :
    process_(INVALID_HANDLE_VALUE),
    processID_(0)
{
}

Process::~Process()
{
}

Process::Priority::Enum Process::getPriorityClass(void) const
{
    DWORD priorityClass = ::GetPriorityClass(process_);
    if (priorityClass == 0) {
        lastError::Description Dsc;
        X_ERROR("Process", "Failed to get process priority for id: % " PRIu32 ". Error: %s", processID_, lastError::ToString(Dsc));
        return Priority::NORMAL;
    }

    switch (priorityClass) {
        case IDLE_PRIORITY_CLASS:
            return Priority::IDLE;
        case BELOW_NORMAL_PRIORITY_CLASS:
            return Priority::BELOW_NORMAL;
        case NORMAL_PRIORITY_CLASS:
            return Priority::NORMAL;
        case ABOVE_NORMAL_PRIORITY_CLASS:
            return Priority::ABOVE_NORMAL;
        case HIGH_PRIORITY_CLASS:
            return Priority::HIGH;
        case REALTIME_PRIORITY_CLASS:
            return Priority::REALTIME;

        default:
            X_ASSERT_UNREACHABLE();
            return Priority::NORMAL;
    }
}

bool Process::setPriorityClass(Priority::Enum priority)
{
    int32_t pri = NORMAL_PRIORITY_CLASS;

    switch (priority) {
        case Priority::IDLE:
            pri = IDLE_PRIORITY_CLASS;
            break;
        case Priority::BELOW_NORMAL:
            pri = BELOW_NORMAL_PRIORITY_CLASS;
            break;
        case Priority::NORMAL:
            pri = NORMAL_PRIORITY_CLASS;
            break;
        case Priority::ABOVE_NORMAL:
            pri = ABOVE_NORMAL_PRIORITY_CLASS;
            break;
        case Priority::HIGH:
            pri = HIGH_PRIORITY_CLASS;
            break;
        case Priority::REALTIME:
            pri = REALTIME_PRIORITY_CLASS;
            break;

        default:
            X_ASSERT_UNREACHABLE();
            break;
    }

    if (!::SetPriorityClass(process_, pri)) {
        lastError::Description Dsc;
        X_ERROR("Process", "Failed to set process priority for id: % " PRIu32 ". Error: %s", processID_, lastError::ToString(Dsc));
        return false;
    }

    return true;
}

Process Process::getCurrent(void)
{
    auto currentHandle = ::GetCurrentProcess();
    auto currentID = ::GetCurrentProcessId();

    return Process(currentHandle, currentID);
}

ProcessId Process::getCurrentID(void)
{
    return safe_static_cast<ProcessId >(::GetCurrentProcessId());
}

X_NAMESPACE_END