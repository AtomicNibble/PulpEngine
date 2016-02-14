#pragma once


#include <ICore.h>
#include <Platform\Console.h>

class EngineApp
{
public:
	EngineApp();
	~EngineApp();

	bool Init(void);

private:
	HMODULE hSystemHandle_;
	ICore* pICore_;
};

