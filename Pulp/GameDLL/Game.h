#pragma once

#ifndef X_GAME_H_
#define X_GAME_H_

#include <IGame.h>
#include <IInput.h>

X_NAMESPACE_BEGIN(game)

class XGame :
	public IGame,
	public input::IInputEventListner
{
public:
	XGame(ICore *pCore);
	~XGame() X_FINAL;

	// IGame
	bool Init(void) X_FINAL;
	bool ShutDown(void) X_FINAL;
	bool Update(void) X_FINAL;
	// ~IGame

	// IInputEventListner
	bool OnInputEvent(const input::InputEvent& event) X_OVERRIDE;
	bool OnInputEventChar(const input::InputEvent& event) X_OVERRIDE;
	// ~IInputEventListner
		

private:
	ICore* pCore_;


	Vec3f cameraPos_;
	Vec3f cameraAngle_; // radins
};

X_NAMESPACE_END

#endif // !X_GAME_H_