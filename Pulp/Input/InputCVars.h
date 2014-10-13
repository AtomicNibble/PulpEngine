#ifndef _X_INPUT_CVARS_H_
#define _X_INPUT_CVARS_H_

X_NAMESPACE_BEGIN(input)



class XInputCVars
{
public:
	int		input_debug;
	int		input_mouse_pos_debug;
	int		scrollLines;
	

	XInputCVars();
	~XInputCVars();
};

extern XInputCVars* g_pInputCVars;

X_NAMESPACE_END

#endif // !_X_INPUT_CVARS_H_
