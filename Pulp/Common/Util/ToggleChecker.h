#pragma once

#ifndef X_UTIL_TOGGLE_CHECKER_H_
#define X_UTIL_TOGGLE_CHECKER_H_

X_NAMESPACE_BEGIN(core)

X_PACK_PUSH(1)

// used for asserting if some logic is correctly toggling between states
class ToggleChecker
{
public:
    ToggleChecker(bool inital_state) :
        current_(inital_state)
    {
    }

    X_INLINE bool operator=(bool value)
    {
        X_ASSERT(value != current_, "toggle check failed")
        (value, current_);
        current_ = value;
        return current_;
    }

    X_INLINE operator bool(void) const
    {
        return current_;
    }

private:
    bool operator+=(bool value);
    bool operator++(void);
    bool operator--(void);

private:
    bool current_;
};
X_PACK_POP

X_NAMESPACE_END

#endif // X_UTIL_TOGGLE_CHECKER_H_