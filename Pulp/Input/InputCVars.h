#ifndef _X_INPUT_CVARS_H_
#define _X_INPUT_CVARS_H_

X_NAMESPACE_BEGIN(input)

class XInputCVars
{
public:
    XInputCVars();
    ~XInputCVars();

    void registerVars(void);

public:
    int32_t inputDebug_;
    int32_t inputMousePosDebug_;
    int32_t scrollLines_;
};

X_NAMESPACE_END

#endif // !_X_INPUT_CVARS_H_
