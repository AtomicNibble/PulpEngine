#pragma once

X_NAMESPACE_BEGIN(core)

typedef uint32_t ProcessId;

class Process
{
public:
    X_DECLARE_ENUM(Priority)
    (
        IDLE,
        BELOW_NORMAL,
        NORMAL,
        ABOVE_NORMAL,
        HIGH,
        REALTIME);

private:
    explicit Process(HANDLE process, ProcessId  processID);

public:
    Process();
    ~Process();

    ProcessId getID(void);

    Priority::Enum getPriorityClass(void) const;
    bool setPriorityClass(Priority::Enum priority);

public:
    static Process getCurrent(void);
    static ProcessId getCurrentID(void);

private:
    HANDLE process_;
    ProcessId processID_;
};

X_NAMESPACE_END