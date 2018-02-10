#pragma once


#include <IConverterModule.h>

X_NAMESPACE_BEGIN(engine)

namespace fx
{

	static const uint32_t	EFFECT_VERSION = 1;
	static const uint32_t	EFFECT_FOURCC = X_TAG('x', 'e', 'f', 'x');
	static const char*		EFFECT_FILE_EXTENSION = "efx";

	static const uint32_t	EFFECT_MAX_LOADED = 1 << 12;
	static const uint32_t	EFFECT_MAX_ELEMENTS = 1 << 8;
	static const uint32_t	EFFECT_MAX_STAGES = 16;

	static const uint32_t	EFFECT_GRAPH_MAX_POINTS = 8;


	// think i will support grahphs for shit like:
	//
	//	color, size, verlocity, rotation?
	//
	//	the graph will just allow a arbitary set of points over the timeline.


	struct IFxLib : public IConverter
	{

	};


	X_DECLARE_ENUM(StageType)(
		BillboardSprite,
		OrientedSprite,
		Tail,
		Line,
		Sound
	);

	X_DECLARE_FLAGS(StageFlag)(
		Looping
	);

	typedef Flags<StageFlag> StageFlags;

	struct GraphHeader
	{
		int32_t numFrames;
	};

	struct Range
	{
		int32_t start;
		int32_t end;
	};

	struct Stage
	{
		StageType::Enum type;
		StageFlags flags;

		int32_t interval;
		int32_t loopCount;

		Range count;
		Range life;
		Range delay;


		Range spawnOrgX;
		Range spawnOrgY;
		Range spawnOrgZ;
	};



	// file stuff 
	struct EffectHdr
	{
		// 4
		uint32_t fourCC;
		// 4
		uint8_t version;
		uint8_t numStages;
		uint8_t _pad[2];

	};


	// X_ENSURE_SIZE(Stage, 60);
	// X_ENSURE_SIZE(EffectHdr, 8);


} // namespace fx

X_NAMESPACE_END