#pragma once

X_NAMESPACE_BEGIN(core)

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
    explicit Process(HANDLE process, uint32_t processID);

public:
    Process();
    ~Process();

    uint32_t GetID(void);

    Priority::Enum GetPriorityClass(void) const;
    bool SetPriorityClass(Priority::Enum priority);

public:
    static Process GetCurrent(void);
    static uint32_t GetCurrentID(void);

private:
    HANDLE process_;
    uint32_t processID_;
};

X_NAMESPACE_END