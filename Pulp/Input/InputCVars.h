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
	int32_t	input_debug;
	int32_t	input_mouse_pos_debug;
	int32_t	scrollLines;
};

extern XInputCVars* g_pInputCVars;

X_NAMESPACE_END

#endif // !_X_INPUT_CVARS_H_
