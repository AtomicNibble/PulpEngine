#pragma once


#ifndef _X_HELPER_MACROS_H_
#define _X_HELPER_MACROS_H_



// Kinky quick loops
#define lopv(v)    for(int i=0;i<(v).Num();i++)
#define lopvj(v)   for(int j=0;j<(v).Num();j++)
#define lopvk(v)   for(int k=0;k<(v).Num();k++)
#define lopvx(v)   for(int x=0;x<(v).Num();x++)
#define lopvrev(v) for(int i=(v).Num()-1;i>=0;i--)

#define loop(v,m) for(int v=0;v<m;v++)


#define lopi(m) loop(i,m)
#define lopj(m) loop(j,m)
#define lopk(m) loop(k,m)
#define lopl(m) loop(l,m)
#define lopx(m) loop(x,m)



#define X_VALIST_START( fmt ) \
	va_list args; \
	va_start(args, fmt);

#define X_VALIST_END \
	va_end(args);


#ifndef BIT
	#define BIT( num )				( 1 << ( num ) )
#endif


#endif // _X_HELPER_MACROS_H_