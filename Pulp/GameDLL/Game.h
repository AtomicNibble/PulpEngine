#pragma once

#ifndef X_GAME_H_
#define X_GAME_H_

#include <IGame.h>

X_NAMESPACE_BEGIN(game)

class XGame : public IGame
{
public:
	XGame(ICore *pCore);
	~XGame() X_FINAL;

	bool Init(void) X_FINAL;
	bool ShutDown(void) X_FINAL;
	bool Update(void) X_FINAL;


private:
	ICore* pCore_;

};

X_NAMESPACE_END

#endif // !X_GAME_H_