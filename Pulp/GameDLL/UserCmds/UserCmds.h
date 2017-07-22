#pragma once


#include <IInput.h>
#include "UserCmd.h"


X_NAMESPACE_BEGIN(game)


class UserCmdGen :
	public input::IInputEventListner
{
public:
	UserCmdGen();

	bool init(void);
	void shutdown(void);
	void clear(void);


	void buildUserCmd(void);

	UserCmd& getCurrentUsercmd(void);
	const UserCmd& getCurrentUsercmd(void) const;


private:
	void resetCmd(void);
	void mouseMove(void);
	void processInput(void);

	// IInputEventListner
	bool OnInputEvent(const input::InputEvent& event) X_FINAL;
	bool OnInputEventChar(const input::InputEvent& event) X_FINAL;
	// ~IInputEventListner

private:
	UserCmd cmd_;

	Vec2f mouseDelta_;
	Vec3f viewAngles_;
	
	int16_t moveForward_;
	int16_t moveRight_;

	input::InputEventBuffer inputEvents_;

};


X_NAMESPACE_END