#pragma once
#ifndef _X_CALLSTACK_H_
#define _X_CALLSTACK_H_

X_NAMESPACE_BEGIN(core)

class CallStack
{
    static const unsigned int MAX_FRAMES = 32;

public:
    typedef char Description[2048];

public:
    explicit CallStack(unsigned int numFramesToSkip);

    const char* ToDescription(Description& description) const;
    const char* ToDescription(const char* info, Description& description) const;

    inline const void* GetFrame(unsigned int idx) const;

private:
    X_NO_ASSIGN(CallStack);

    void* frames_[MAX_FRAMES];
};

#include "CallStack.inl"

X_NAMESPACE_END

#endif // !_X_CALLSTACK_H_