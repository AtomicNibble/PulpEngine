#pragma once

#include <IConverterModule.h>
#include <IAsyncLoad.h>

X_NAMESPACE_BEGIN(engine)

namespace fx
{
    static const uint32_t EFFECT_VERSION = 6;
    static const uint32_t EFFECT_FOURCC = X_TAG('x', 'e', 'f', 'x');
    static const char* EFFECT_FILE_EXTENSION = "efx";

    static const uint32_t EFFECT_MAX_LOADED = 1 << 12;
    static const uint32_t EFFECT_MAX_ELEMENTS = 1 << 8;
    static const uint32_t EFFECT_MAX_STAGES = 16;

    static const uint32_t EFFECT_MAX_EMITTERS = 1 << 10;
    static const uint32_t EFFECT_MAX_ACTIVE_ELEMS = 1 << 11;

    static const uint32_t EFFECT_GRAPH_MAX_POINTS = 16;

    // InitialRotation:
    //		The starting rotation, rotation is added to this.
    //
    // Rotation:
    //		This is a persecond rotation ammount, so it's addative to current rotation.
    //		it has no effect on position of quad.
    //		this rotation must be appylied to the quad before any other.
    //
    //
    //

    typedef uint16_t IndexType;
    typedef IndexType IndexOffset;

    struct IFxLib : public IConverter
    {
    };

    X_DECLARE_ENUM8(StageType)
    (
        BillboardSprite, // the elems are rotated to face the view.
        OrientedSprite,  // the elems are always rotated direction of emmiter.
        RotatedSprite,   // like Oriented, but 3axis elem rotation.
        Tail,            // like Billboard but rotates on longest axis. (Eg a long vertical quad will stay vertical)
        Line,
        Sound, // Emmits a sound event.
        Model, // Spawns a model.
        PlayFX // Plays a Effect.
    );

    X_DECLARE_ENUM8(RelativeTo)
    (
        Spawn, // the elems position is: relative to emitters translation at elem spawn time.
        Now    // the elems position is: relative to emitters current translation.
    );

    X_DECLARE_FLAGS(StageFlag)
    (
        Looping,         // The stage is looping. either unlimited or between a range.
        RandGraphCol,    // Blend between two graphs with a per elem random fraction.
        RandGraphAlpha,  // Blend between two graphs with a per elem random fraction.
        RandGraphSize0,  // Blend between two graphs with a per elem random fraction.
        RandGraphSize1,  // Blend between two graphs with a per elem random fraction.
        RandGraphVel0,   // Blend between two graphs with a per elem random fraction.
        RandGraphVel1,   // Blend between two graphs with a per elem random fraction.
        RandGraphRot,    // Blend between two graphs with a per elem random fraction.
        RelativeVel0,    // The verloticy is relative to the effect axis, instead of world.
        RelativeVel1,    // The verloticy is relative to the effect axis, instead of world.
        RelativeOrigin,  // The origin is relative to the effect axis, instead of world.
        NonUniformScale, // The height of the quad is sampled from a second size graph.
        SpawnSphere,
        SpawnCylindrical);

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
        uint16_t numPoints;
        IndexOffset timeStart;  // 0-1 times
        IndexOffset valueStart; // values
        IndexOffset scaleIdx;   //
    };

    X_ENSURE_SIZE(Graph, 4 * 2);

    static_assert(std::numeric_limits<decltype(Graph::numPoints)>::max() >= EFFECT_GRAPH_MAX_POINTS, "Can't represent max points");

    // for animated sprites.
    // the number of frames is materials atlas row * col count.
    struct Sequence
    {
        int32_t startFrame; // 0 - index zero, index into atlas, negative is random
        int32_t fps;        // 0 - never change, frame rate, negative is sync to particle lifetime.
        int32_t loop;       // 0 - forever
    };

    struct StageDsc
    {
        typedef std::array<Graph, 2> GraphArr;

        StageType::Enum type;
        RelativeTo::Enum postionType;
        uint8_t _pad[2];
        StageFlags flags;

        Sequence sequence;

        int32_t materialStrOffset;
        // two below used when StageFlag::Looping set
        int32_t interval;  // how often we run, if loopcount > 1 we wait this time before next.
        int32_t loopCount; // how many times we spawn before the end.

        Range count; //
        Range life;  // life for each elem.
        Range delay; // delay is when we start this stage.

        Range spawnOrgX;
        Range spawnOrgY;
        Range spawnOrgZ;

        Range spawnRadius; // used for: Sphere/Cylindrical
        Range spawnHeight; // used for Cylindrical

        Range initialRotation;

        Range anglePitch;
        Range angleYaw;
        Range angleRoll;

        GraphArr color;
        GraphArr alpha;
        GraphArr size0;
        GraphArr size1;
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
        uint8_t _pad;
        uint16_t numStages;
        uint16_t numIndex;
        uint16_t numFloats;
    };

    X_INLINE bool EffectHdr::isValid(void) const
    {
        return fourCC == EFFECT_FOURCC && version == EFFECT_VERSION;
    }

    // X_ENSURE_SIZE(Stage, 60);
    X_ENSURE_SIZE(EffectHdr, 12);

    class Effect;

    struct IEffectManager : public core::IAssetLoader
    {
        virtual ~IEffectManager() = default;

        virtual Effect* findEffect(const char* pName) const X_ABSTRACT;
        virtual Effect* loadEffect(const char* pName) X_ABSTRACT;

        virtual bool waitForLoad(Effect* pMaterial) X_ABSTRACT;
    };

    struct IEmitter
    {
        virtual void play(const Effect* pEffect, bool looping, bool clear) X_ABSTRACT;

        virtual void setTrans(const Transformf& trans) X_ABSTRACT;
        virtual void setTrans(const Transformf& trans, const Vec3f& offset) X_ABSTRACT;
    };

} // namespace fx

X_NAMESPACE_END
