#pragma once


#ifndef _X_HELPER_MACROS_H_
#define _X_HELPER_MACROS_H_



// Kinky quick loops
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
	#define BIT( num )				( 1u << ( num ) )
#endif


#endif // _X_HELPER_MACROS_H_