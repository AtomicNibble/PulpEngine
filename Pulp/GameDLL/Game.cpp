#include "stdafx.h"
#include "Game.h"


X_NAMESPACE_BEGIN(game)

XGame::XGame(ICore* pCore) :
pCore_(pCore)
{
	X_ASSERT_NOT_NULL(pCore);

}

XGame::~XGame()
{

}

bool XGame::Init(void)
{



	return true;
}

bool XGame::ShutDown(void)
{



	return true;
}

bool XGame::Update(void)
{
	X_PROFILE_BEGIN("Update", core::ProfileSubSys::GAME);




	return true;
}




X_NAMESPACE_END