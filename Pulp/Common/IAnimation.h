#pragma once


#ifndef _X_ASSET_ANIM_H_
#define _X_ASSET_ANIM_H_

#include <Util\FlagsMacros.h>

X_NAMESPACE_BEGIN(anim)

// The Model Foramts
//
//  File Ext: .anim
//	Version: 1.0
//  Info:
//  
//  This format contains the animation data
//	
//
//
//
//

static const uint32_t	 ANIM_VERSION = 1;
static const uint32_t	 ANIM_MAX_BONES = 255;
static const uint32_t	 ANIM_MAX_FPS = 90;


#ifdef RELATIVE
#undef RELATIVE
#endif
#ifdef ABSOLUTE
#undef ABSOLUTE
#endif

struct AnimType
{
	enum Enum : uint8_t
	{
		RELATIVE,
		ABSOLUTE
	};
};



struct tagData
{
	uint16_t		numAngles;
	uint16_t		numPositions;
	Vec3f			positions[2];

};


struct AnimHeader
{
	uint8_t			version;
	uint8_t			flags;
	AnimType::Enum	type;
	uint8_t			numJoints;
	uint16_t		numFrames;
	uint16_t		fps;

};



X_NAMESPACE_END

#endif // !_X_ASSET_ANIM_H_