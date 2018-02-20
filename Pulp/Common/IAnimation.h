#pragma once


#ifndef _X_ASSET_ANIM_H_
#define _X_ASSET_ANIM_H_

#include <IConverterModule.h>
#include <Time\TimeVal.h>
#include <Util\FlagsMacros.h>

X_NAMESPACE_BEGIN(anim)

// The Anim Foramts
//
//  File Ext: .anim
//	Version: 1.0
//  Info:
//  
//  for each tag we have a list of names.
//	The data is in the same order as the data.
//	And there is same data as num tags.
//	for the tag with a name at index 5 it's data is at index 5 of the tag data.
//	
//	Animation data is relative to bone positions.
//	So requires the default pose skelton to compile against.
//	
//	Tag Data:
//		The amount of frames we have data for is diffrent for pos / angle.
//		Meaning a tag can have full frame position data but part frame angles data
//		If we have data for every frame, we don't store frame indexs.
//		Angle data comes first then position data.
//		Both angle and position data are optional.
//
//	Frames:
//		the frame size type is 8bits when numFrames is <=255 16bit otherwise.
//
//	Angles:
//		Angles are compressed Quatanions with the w dropped.
//		So angles become 3 16bit ints.
//		Optionaly a tag can be defined as having only z rotation.
//		Meaning for that tag there is only one 16bit int per angle.
//
//	Positions:
//		if we have only one position it's a vec3f
//		otherwise we have two vec3f start - range
//		followed by translate frames which can be either 8bit or 16bit (flag per tag)
//		which is a value between 0-1 that is used to scale the range.
//		there are 3 scalers pe frame one for each axis x,y,z
//		Example:
//			start + (range * frameScale);
//
//	Notes:
//		notes area at the end of the file if the flag NOTES is present.
//		they have a 16bit frame number and note name.
//		they are sorted in frame order.
//		multiple notes can occur on the same frame.
//
#define X_ANIM_NOTE_LOWER_CASE_VALUE 1


static const uint32_t	 ANIM_VERSION = 2;
static const uint32_t	 ANIM_MAX_BONES = 255;
static const uint32_t	 ANIM_MAX_FRAMES = 4096; // can be increased up to (1 << 16) -1
static const uint32_t	 ANIM_DEFAULT_FPS = 30;
static const uint32_t	 ANIM_MIN_FPS = 1;
static const uint32_t	 ANIM_MAX_FPS = 90;
static const uint32_t	 ANIM_MAX_NOTES = 255;
static const uint32_t	 ANIM_MAX_NOTES_PER_FRAME = 8;
static const uint32_t	 ANIM_MAX_NOTE_NAME_LENGTH = 48; // the max lengt of each notes name
static const uint32_t	 ANIM_MAX_NAME_LENGTH = 60;
static const char*		 ANIM_FILE_EXTENSION = "anim";
static const wchar_t*	 ANIM_FILE_EXTENSION_W = L"anim";

static const uint32_t	 ANIM_MAX_LOADED = 1 << 10;

// Intermidiate format stuff.
// this is used for saving out animation data that is not relative.
// It's then later processed against a skelton to creat a anim.
// This also allows other tools to export anims since the inter format is text based.
static const uint32_t	 ANIM_INTER_VERSION = 2;
static const char*		 ANIM_INTER_FILE_EXTENSION = "anim_inter";
static const wchar_t*	 ANIM_INTER_FILE_EXTENSION_W = L"anim_inter";


#ifdef RELATIVE
#undef RELATIVE
#endif
#ifdef ABSOLUTE
#undef ABSOLUTE
#endif


struct IAnimLib : public IConverter
{

};

X_DECLARE_ENUM8(AnimType)(
	RELATIVE, 
	ABSOLUTE,
	ADDITIVE,
	DELTA
);

X_DECLARE_FLAGS8(AnimFlag)(
	LOOP,
	NOTES // got notes
);


X_DECLARE_FLAGS8(BoneFlag)(
	LargePosScalers,

	// if we have data for every frame, so we did not bother writing frame list.
	PosFullFrame,
	AngFullFrame,
	// if frame numbers are 16bit
	PosLargeFrames,
	AngLargeFrames
);

typedef Flags8<BoneFlag> BoneFlags;


struct AnimState
{
	core::TimeVal elapsed;
	int32_t currentFrame;

};

struct FrameBlend
{
	int32		cylces;
	int32		frame1;
	int32		frame2;
	float		frontlerp;
	float		backlerp;
};

struct NoteTrackHdr
{
	uint16_t num;
};

struct Note
{
	uint16_t value;
	uint16_t frame;
};

struct AnimHeader
{
	// 4
	uint8_t				version;
	Flags8<AnimFlag>	flags;
	AnimType::Enum		type;
	uint8_t				numBones;
	// 4
	uint16_t			numFrames;
	uint16_t			fps;

	uint32_t			dataSize; // size of rest of file.

	X_INLINE bool isValid(void) const;
	X_INLINE bool isLooping(void) const;
	X_INLINE bool hasNotes(void) const;
	X_INLINE size_t numTagHeaderBytes(void) const;
};


X_INLINE bool AnimHeader::isValid(void) const
{
	return version == ANIM_VERSION;
}

X_INLINE bool AnimHeader::isLooping(void) const
{
	return flags.IsSet(AnimFlag::LOOP);
}
X_INLINE bool AnimHeader::hasNotes(void) const
{
	return flags.IsSet(AnimFlag::NOTES);
}

X_INLINE size_t AnimHeader::numTagHeaderBytes(void) const
{
	return core::bitUtil::RoundUpToMultiple<uint32_t>(numBones, 8);
}


X_ENSURE_SIZE(AnimHeader, 12);
X_ENSURE_SIZE(NoteTrackHdr, 2);
X_ENSURE_SIZE(Note, 4);


X_NAMESPACE_END

#endif // !_X_ASSET_ANIM_H_