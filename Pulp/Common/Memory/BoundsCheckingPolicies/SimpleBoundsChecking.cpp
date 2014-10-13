#include "EngineCommon.h"

#include "SimpleBoundsChecking.h"

X_NAMESPACE_BEGIN(core)


const char* const SimpleBoundsChecking::TYPE_NAME = "";


void SimpleBoundsChecking::GuardFront(void* memory)
{	 
	memset( memory, TOKEN_FRONT, 4 );
}

void SimpleBoundsChecking::GuardBack(void* memory)
{
	memset( memory, TOKEN_BACK, 4 );
}

void SimpleBoundsChecking::CheckFront(const void* memory)
{
  union
  {
    const void* as_void;
    BYTE* as_char;
  };
 	
  as_void = memory;

  lopi( 4 )
  {
		X_ASSERT( as_char[i] == TOKEN_FRONT , "Memory has been overwritten at address 0x%08p", memory )( i, TOKEN_FRONT );
  }
}

void SimpleBoundsChecking::CheckBack(const void* memory)
{
  union
  {
    const void* as_void;
    BYTE* as_char;
  };
 	
  as_void = memory;

  lopi( 4 )
  {
		X_ASSERT( as_char[i] == TOKEN_BACK, "Memory has been overwritten at address 0x%08p", memory )( i, TOKEN_FRONT );
  }
}



X_NAMESPACE_END