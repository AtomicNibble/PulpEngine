#pragma once


#ifndef X_GAME_I_H_
#define X_GAME_I_H_

X_NAMESPACE_BEGIN(game)

struct IGame
{
	virtual ~IGame() {};


	virtual bool Init(void) X_ABSTRACT;
	virtual bool ShutDown(void) X_ABSTRACT;
	virtual bool Update(void) X_ABSTRACT;

	virtual void release(void) X_ABSTRACT;


};

X_NAMESPACE_END

#endif // !X_GAME_I_H_