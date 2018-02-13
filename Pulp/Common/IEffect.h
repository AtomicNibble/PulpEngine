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

	typedef uint8_t IndexType;
	typedef IndexType IndexOffset;

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
		Looping,
		RandGraphCol,
		RandGraphAlpha,
		RandGraphSize,
		RandGraphVel
	);

	typedef Flags<StageFlag> StageFlags;

	struct GraphHeader
	{
		int32_t numFrames;
	};

	X_PACK_PUSH(4)

	struct Range
	{
		float start;
		float range;
	};


	struct Graph
	{
		uint8_t numPoints;
		IndexOffset timeStart;		// 0-1 times
		IndexOffset valueStart;		// values
		IndexOffset scaleIdx;		// 
	};

	static_assert(std::numeric_limits<decltype(Graph::numPoints)>::max() >= EFFECT_GRAPH_MAX_POINTS, "Can't represent max points");

	// for animated sprites.
	// the number of frames is materials atlas row * col count.
	struct Sequence
	{
		int32_t startFrame; // 0 - index zero, index into atlas, negative is random
		int32_t fps;		// 0 - never change, frame rate, negative is sync to particle lifetime.
		int32_t loop;		// 0 - forever
	};	

	struct Stage
	{
		typedef std::array<Graph, 2> GraphArr;

		StageType::Enum type;
		StageFlags flags;

		Sequence sequence;

		int32_t materialStrOffset;
		int32_t interval;	// how often we run, if loopcount > 1 we wait this time before next.
		int32_t loopCount;	// how many times we spawn before the end.

		Range count;		
		Range life;			// life for each elem.
		Range delay;		// delay is when we start this stage.

		Range spawnOrgX;
		Range spawnOrgY;
		Range spawnOrgZ;

		GraphArr color;
		GraphArr alpha;
		GraphArr size;
		GraphArr scale;
		GraphArr rot;
		GraphArr vel0X;
		GraphArr vel0Y;
		GraphArr vel0Z;
	};

	X_PACK_POP;

	// file stuff 
	struct EffectHdr
	{
		X_INLINE bool isValid(void) const;


		// 4
		uint32_t fourCC;
		// 4
		uint8_t version;
		uint8_t numStages;
		uint8_t numIndex;
		uint8_t numFloats;
	};

	X_INLINE bool EffectHdr::isValid(void) const
	{
		return fourCC == EFFECT_FOURCC && version == EFFECT_VERSION;
	}

	// X_ENSURE_SIZE(Stage, 60);
	// X_ENSURE_SIZE(EffectHdr, 8);


} // namespace fx

X_NAMESPACE_END