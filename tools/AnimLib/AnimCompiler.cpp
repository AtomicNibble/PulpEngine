#include "stdafx.h"
#include "AnimCompiler.h"

#include <Time\StopWatch.h>
#include <String\HumanDuration.h>

#include <IFileSys.h>

X_NAMESPACE_BEGIN(anim)

namespace
{
    X_INLINE float removeNoise(float f)
    {
        if (math<float>::abs(f) <= AnimCompiler::NOISE_ELIPSON) {
            return 0.f;
        }

        return f;
    }

    X_INLINE Vec3f removeNoise(Vec3f vec)
    {
        return Vec3f(removeNoise(vec.x), removeNoise(vec.y), removeNoise(vec.z));
    }

} // namespace

const float AnimCompiler::DEFAULT_POS_ERRR = 0.05f;
const float AnimCompiler::DEFAULT_ANGLE_ERRR = 0.005f;
const float AnimCompiler::NOISE_ELIPSON = 0.0001f;

AnimCompiler::Stats::Stats(core::MemoryArenaBase* arena) :
    noneAnimatedBoneName(arena),
    notPresentBoneNames(arena)
{
    clear();
}

void AnimCompiler::Stats::clear(void)
{
    scale = 1.0f;
    numFrames = 0;
    fps = 0;
    totalBones = 0;
    totalBonesPosData = 0;
    totalBonesAngleData = 0;

    compileTime = core::TimeVal(0ll);
    noneAnimatedBoneName.clear();
}

void AnimCompiler::Stats::print(void) const
{
    core::HumanDuration::Str durStr;

    X_LOG0("Anim", "Anim Info:");
    X_LOG0("Anim", "> Scale: ^6%g", scale);
    X_LOG0("Anim", "> Compile Time: ^6%s", core::HumanDuration::toString(durStr, compileTime.GetMilliSeconds()));
    X_LOG0("Anim", "> Num Frames: ^6%" PRIi32, numFrames);
    X_LOG0("Anim", "> Fps: ^6%" PRIi32, fps);
    X_LOG0("Anim", "> Total Bones: ^6%" PRIuS, totalBones);
    X_LOG0("Anim", "> Total Bones(Pos): ^6%" PRIuS, totalBonesPosData);
    X_LOG0("Anim", "> Total Bones(Ang): ^6%" PRIuS, totalBonesAngleData);

    core::StackString<1024> info("Dropped Bones:");

    if (noneAnimatedBoneName.size() > 0) {
        info.append(" -> (");
        for (uint i = 0; i < noneAnimatedBoneName.size(); i++) {
            info.append(noneAnimatedBoneName[i].c_str());

            if (i < (noneAnimatedBoneName.size() - 1)) {
                info.append(", ");
            }

            if (i > 9 && (i % 10) == 0) {
                info.append("");
            }
        }
        info.append(")");
    }
    if (noneAnimatedBoneName.size() > 10) {
        info.append("");
    }

    X_LOG0("Anim", info.c_str());

    info.set("NotInAnim Bones:");

    if (notPresentBoneNames.size() > 0) {
        info.append(" -> (");
        for (auto& name : notPresentBoneNames) {
            info.append(name.c_str());
            info.append(", ");
        }

        info.trimRight(',');
    }

    X_LOG0("Anim", info.c_str());
}

// ------------------------------------------

AnimCompiler::Position::Position(core::MemoryArenaBase* arena) :
    fullPos_(arena),
    relPos_(arena),
    posFrames_(arena),
    scalers_(arena)
{
    fullPos_.setGranularity(128);

    largeScalers_ = false;
}

void AnimCompiler::Position::save(core::ByteStream& stream) const
{
    int16_t numPos = safe_static_cast<uint16_t>(posFrames_.size());

    stream.write(numPos);
    if (numPos == 0) {
        return;
    }

    auto& relativeMin = min();

    if (numPos == 1) {
        stream.write(relativeMin); // just write min pos.
    }
    else {
        // if we not full frames we write frames numbers out.
        if (!isFullFrames() && posFrames_.size() > 1) {
            // frame numbers are 8bit if total anim frames less than 255
            if (!isLargeFrames()) {
                for (const PosFrame& entry : posFrames_) {
                    uint8_t frame = safe_static_cast<uint8_t, uint32_t>(entry.frame);
                    stream.write(frame);
                }
            }
            else {
                for (const PosFrame& entry : posFrames_) {
                    uint16_t frame = safe_static_cast<uint16_t, uint32_t>(entry.frame);
                    stream.write(frame);
                }
            }
        }

        // now we need to write the scalers.
        if (isLargeScalers()) {
            stream.write(scalers_.ptr(), scalers_.size());
        }
        else {
            for (auto s : scalers_) {
                Vec3<uint8_t> s8;

                s8.x = safe_static_cast<uint8_t, uint16_t>(s.x);
                s8.y = safe_static_cast<uint8_t, uint16_t>(s.y);
                s8.z = safe_static_cast<uint8_t, uint16_t>(s.z);

                stream.write(s8);
            }
        }

        stream.write(relativeMin);
        stream.write(range_);
    }
}

void AnimCompiler::Position::clearData(void)
{
    posFrames_.clear();
}

void AnimCompiler::Position::appendFullPos(const Vec3f& pos)
{
    fullPos_.append(pos);
}

void AnimCompiler::Position::setBasePositions(const Vec3f& basePosWorld, const Vec3f& basePosRel)
{
    basePosWorld_ = removeNoise(basePosWorld);
    basePosRel_ = removeNoise(basePosRel);
}

size_t AnimCompiler::Position::numPosFrames(void) const
{
    return posFrames_.size();
}

bool AnimCompiler::Position::hasData(void) const
{
    return posFrames_.isNotEmpty();
}

bool AnimCompiler::Position::isLargeFrames(void) const
{
    return fullPos_.size() > std::numeric_limits<uint8_t>::max();
}

bool AnimCompiler::Position::isFullFrames(void) const
{
    return posFrames_.size() == fullPos_.size();
}

bool AnimCompiler::Position::isLargeScalers(void) const
{
    return largeScalers_;
}

const Vec3f& AnimCompiler::Position::min(void) const
{
    return min_;
}

const Vec3f& AnimCompiler::Position::range(void) const
{
    return range_;
}

const Vec3f& AnimCompiler::Position::basePosWorld(void) const
{
    return basePosWorld_;
}

const Vec3f& AnimCompiler::Position::basePosRel(void) const
{
    return basePosRel_;
}

const Vec3f& AnimCompiler::Position::getWorldPosForFrame(size_t idx) const
{
    return fullPos_[idx];
}

const AnimCompiler::Position::PosFrameArr& AnimCompiler::Position::getPositions(void) const
{
    return posFrames_;
}

void AnimCompiler::Position::calculateRelativeData(const Position& parentPos, const Angle& parentAng)
{
    relPos_.resize(fullPos_.size());

    for (size_t i = 0; i < fullPos_.size(); i++) {
        const Vec3f& parWorldPos = parentPos.getWorldPosForFrame(i);
        const Vec3f& worldPod = fullPos_[i];
        Vec3f rel = worldPod - parWorldPos;

        // remove the rotation?
        auto& angle = parentAng.getAngForFrame(i);
        Vec3f unRotatedRel = angle.inverse() * rel;

        relPos_[i] = removeNoise(unRotatedRel);
    }
}

void AnimCompiler::Position::calculateRelativeDataRoot(void)
{
    relPos_ = fullPos_;
}

void AnimCompiler::Position::calculateFullFrames(void)
{
    posFrames_.resize(relPos_.size());

    min_ = Vec3f::max();
    max_ = Vec3f::min();

    for (size_t i = 0; i < relPos_.size(); i++) {
        const Vec3f& pos = relPos_[i];
        auto delta = pos - basePosRel_;

        delta = removeNoise(delta);

        min_.checkMin(delta);
        max_.checkMax(delta);

        PosFrame& posEntry = posFrames_[i];
        posEntry.delta = delta;
        posEntry.relPos = relPos_[i];
        posEntry.frame = safe_static_cast<int32_t>(i);
    }

    min_ = removeNoise(min_);
    max_ = removeNoise(max_);
}

void AnimCompiler::Position::calculateDeltaFrames(const float posError)
{
    X_ASSERT(relPos_.size() == fullPos_.size(), "Rel pos data size mistmatch")(); 

    posFrames_.clear();
    posFrames_.reserve(relPos_.size());

    min_ = Vec3f::max();
    max_ = Vec3f::min();

    /*
		This logic looks for linera movement and only stores info when the
		the translation deviates more than 'posError'.

		For example if the bone moves in the same direction at the same speed every frame
		we don't need to bother storing.

		if the rate of movement keeps chaning frames will need to be stored.

	*/

    // we don't do anthing until we have moved.
    const int32_t totalFrames = static_cast<int32_t>(relPos_.size());
    int32_t frame = 0;

    auto addFrame = [&](int32_t frameIdx) {
        const Vec3f& pos = relPos_[frameIdx];
        auto delta = pos - basePosRel_;

        min_.checkMin(delta);
        max_.checkMax(delta);

        PosFrame& posEntry = posFrames_.AddOne();
        posEntry.relPos = pos;
        posEntry.delta = delta;
        posEntry.frame = frameIdx;
    };

    // find the first frame we move
    for (frame = 0; frame < totalFrames; frame++) {
        auto delta = relPos_[frame] - basePosRel_;

        if (!delta.compare(Vec3f::zero(), posError)) {
            // store the frame before as that's when movement started
            // if it's the first frame tho, we are 'jumping' the bone to a start location.
            if (frame > 0) {
                --frame;
            }

            addFrame(frame);
            break;
        }
    }

    if (frame < totalFrames) {
        // we stored movement data for current frame
        // now we keep moving forward checking pos error against last frame.
        int32_t lastStoredFrame = frame;

        for (++frame; frame < totalFrames; ++frame) {
            const Vec3f& lastStoredPos = relPos_[lastStoredFrame];
            const Vec3f& singleDelta = relPos_[lastStoredFrame + 1] - lastStoredPos;
            const Vec3f& curFramePos = relPos_[frame];

            int32_t numFrames = frame - lastStoredFrame;

            // if continue at same rate we would have this delta
            Vec3f linDelta = singleDelta * numFrames;
            // the actual delta.
            Vec3f delta = curFramePos - lastStoredPos;

            // have we dirifted?
            if (!delta.compare(linDelta, posError)) {
                // lets store the frame before we drifted,
                lastStoredFrame = frame - 1;

                addFrame(lastStoredFrame);
            }
        }

        // do we always need a end frame?
        if (lastStoredFrame != totalFrames - 1) {
            const Vec3f& lastStoredPos = relPos_[lastStoredFrame];
            const Vec3f& curFramePos = relPos_[totalFrames - 1];

            Vec3f delta = curFramePos - lastStoredPos;

            if (!delta.compare(Vec3f::zero(), posError)) {
                addFrame(totalFrames - 1);
            }
        }
    }

    if (posFrames_.isEmpty()) {
        min_ = Vec3f::zero();
        max_ = Vec3f::zero();
        return;
    }
    else {
        X_ASSERT(min_ != Vec3f::max(), "Infinate range")(); 
        X_ASSERT(max_ != Vec3f::min(), "Infinate range")(); 
    }
}

void AnimCompiler::Position::buildScalers(const float posError)
{
    scalers_.clear();
    scalers_.reserve(posFrames_.size());

    range_ = max_ - min_;

    uint32_t segments = (1 << 8) - 1;

    // work out if we can use 8bit scalers with selected posError.
    Vec3f rangePercision = range_ / std::numeric_limits<uint8_t>::max();
    if (rangePercision.x > posError || rangePercision.y > posError || rangePercision.z > posError) {
        segments = (1 << 16) - 1; // need 16 bit.

        largeScalers_ = true;
    }
    else {
        largeScalers_ = false;
    }

    Vec3f segmentsize(range_ / Vec3f(static_cast<float>(segments)));

    for (auto& posEntry : posFrames_) {
        // we know want to know the scaler needed to get the relative pos.

        const Vec3f& delta = posEntry.delta - min_;
        Vec3f scalerF = (delta / segmentsize);

        Scaler scaler;
        scaler.x = static_cast<Scaler::value_type>(math<float>::round(scalerF.x));
        scaler.y = static_cast<Scaler::value_type>(math<float>::round(scalerF.y));
        scaler.z = static_cast<Scaler::value_type>(math<float>::round(scalerF.z));

        scalers_.append(scaler);
    }

#if X_DEBUG
    if (!largeScalers_) {
        // i know what the max is, i just think numeric_limits makes code intention more clear
        auto max = std::numeric_limits<uint8_t>::max();
        for (auto& scaler : scalers_) {
            X_ASSERT(scaler.x <= max && scaler.x <= max && scaler.x <= max, "Invalid scaler")(scaler.x, scaler.y, scaler.z); 
        }
    }
#endif
}

// ====================================================

AnimCompiler::Angle::Angle(core::MemoryArenaBase* arena) :
    fullAngles_(arena),
    relAngles_(arena),
    angles_(arena)
{
    fullAngles_.setGranularity(128);
}

void AnimCompiler::Angle::save(core::ByteStream& stream) const
{
    int16_t numAngle = safe_static_cast<uint16_t>(angles_.size());

    stream.write(numAngle);

    if (numAngle == 0) {
        return;
    }

    if (!isFullFrames() && angles_.size() > 1) {
        // frame numbers are 8bit if total anim frames less than 255
        if (!isLargeFrames()) {
            for (const auto& a : angles_) {
                uint8_t frame = safe_static_cast<uint8_t, uint32_t>(a.frame);
                stream.write(frame);
            }
        }
        else {
            for (const auto& a : angles_) {
                uint16_t frame = safe_static_cast<uint16_t, uint32_t>(a.frame);
                stream.write(frame);
            }
        }
    }

    // write angles
    for (const auto& a : angles_) {
        // compressed quat.
        XQuatCompressedf quatf(a.angle);
        stream.write(quatf);
    }
}

void AnimCompiler::Angle::clearData(void)
{
    angles_.clear();
}

void AnimCompiler::Angle::appendFullAng(const Matrix33f& ang)
{
    fullAngles_.append(ang);
}

void AnimCompiler::Angle::setBaseOrients(const Quatf& angWorld, const Quatf& andRel)
{
    baseOrientWorld_ = angWorld;
    baseOrient_ = andRel;
}

size_t AnimCompiler::Angle::numAngleFrames(void) const
{
    return angles_.size();
}

bool AnimCompiler::Angle::hasData(void) const
{
    return angles_.isNotEmpty();
}

bool AnimCompiler::Angle::isLargeFrames(void) const
{
    return fullAngles_.size() > std::numeric_limits<uint8_t>::max();
}

bool AnimCompiler::Angle::isFullFrames(void) const
{
    return fullAngles_.size() == angles_.size();
}

const Matrix33f& AnimCompiler::Angle::getAngForFrame(size_t idx) const
{
    return fullAngles_[idx];
}

const AnimCompiler::Angle::AngleFrameArr& AnimCompiler::Angle::getAngles(void) const
{
    return angles_;
}

void AnimCompiler::Angle::calculateRelativeData(const Position& parentPos, const Angle& parentAng)
{
    relAngles_.resize(fullAngles_.size());

    for (size_t i = 0; i < fullAngles_.size(); i++) {
        auto& parWorldAngle = parentAng.getAngForFrame(i);
        auto& worldAngle = fullAngles_[i];
        auto rel = parWorldAngle.inverse() * worldAngle;

        relAngles_[i] = rel;
    }
}

void AnimCompiler::Angle::calculateRelativeDataRoot(void)
{
    relAngles_ = fullAngles_;
}

void AnimCompiler::Angle::calculateFullFrames(void)
{
    angles_.resize(relAngles_.size());

    for (size_t i = 0; i < relAngles_.size(); i++) {
        angles_[i].angle = relAngles_[i];
        angles_[i].frame = safe_static_cast<int32_t>(i);
    }
}

void AnimCompiler::Angle::calculateDeltaFrames(const float angError)
{
    angles_.clear();
    angles_.reserve(relAngles_.size());

    // work out any axis changes
    for (const auto& ang : relAngles_) {
        Vec3f euler = Quatf(ang).getEulerDegrees();

        bool pitchPass = math<float>::abs(euler.x) > angError;
        bool rollPass = math<float>::abs(euler.y) > angError;
        bool yawPass = math<float>::abs(euler.z) > angError;

        axisChanges_[0] |= pitchPass;
        axisChanges_[1] |= rollPass;
        axisChanges_[2] |= yawPass;
    }

    auto addFrame = [&](int32_t frameIdx) {
        const auto& angle = relAngles_[frameIdx];

        AngleFrame& entry = angles_.AddOne();
        entry.angle = angle;
        entry.frame = frameIdx;
    };

    // find first frame we rotate.
    const int32_t totalFrames = static_cast<int32_t>(relAngles_.size());

    int32_t frame = 0;
    for (frame = 0; frame < totalFrames; frame++) {
        Matrix33f delta = relAngles_[frame];
        Quatf deltaQuat(delta);
        Vec3f deltaEuler = deltaQuat.getEulerDegrees();

        if (!deltaEuler.compare(Vec3f::zero(), angError)) {
            if (frame > 0) {
                --frame;
            }

            addFrame(frame);
            break;
        }
    }

    if (frame < totalFrames) {
        int32_t lastStoredFrame = frame;

        for (frame += 1; frame < totalFrames - 1; ++frame) {
            const Matrix33f& lastStoredM = relAngles_[lastStoredFrame];
            const Matrix33f& curFrameM = relAngles_[frame];
            const Matrix33f& curFramePlusOneM = relAngles_[frame + 1];

            // turn them in to quats like they will be stored.
            // so we can check what the runtime error value would actually be.
            Quatf lastStored = Quatf(lastStoredM);
            Quatf curFrame = Quatf(curFrameM);
            Quatf curFramePlusOne = Quatf(curFramePlusOneM);

            // simulate the compression loss?
#if 0
			lastStored = XQuatCompressedf(lastStored).asQuat();
			curFrame = XQuatCompressedf(curFrame).asQuat();
			curFramePlusOne = XQuatCompressedf(curFramePlusOne).asQuat();
#endif

            int32_t numFrames = frame - lastStoredFrame;
            float fraction = static_cast<float>(numFrames) / (numFrames + 1);

            Quatf interpolated = lastStored.slerp(fraction, curFramePlusOne);
            Quatf diff = interpolated.diff(curFrame);

            Vec3f diffEuler = diff.getEulerDegrees();

            if (!diffEuler.compare(Vec3f::zero(), angError)) {
                // lets store the frame before we drifted,
                lastStoredFrame = frame;

                addFrame(lastStoredFrame);
            }
        }

        // I only want a end frame if it's diffrent.
        if (lastStoredFrame != totalFrames - 1) {
            const Matrix33f& lastStoredM = relAngles_[lastStoredFrame];
            const Matrix33f& curFrameM = relAngles_[totalFrames - 1];

            Quatf lastStored = Quatf(lastStoredM);
            Quatf curFrame = Quatf(curFrameM);

            Quatf diff = curFrame.diff(lastStored);
            Vec3f diffEuler = diff.getEulerDegrees();

            if (!diffEuler.compare(Vec3f::zero(), angError)) {
                addFrame(totalFrames - 1);
            }
        }
    }
}

// ====================================================

AnimCompiler::Bone::Bone(core::MemoryArenaBase* arena) :
    pos(arena),
    ang(arena),
    parentIdx(-1)
{
}

bool AnimCompiler::Bone::hasData(void) const
{
    return ang.hasData() || pos.hasData();
}

void AnimCompiler::Bone::clearData(void)
{
    ang.clearData();
    pos.clearData();
}

// ----------------------------------------------------

AnimCompiler::AnimCompiler(core::MemoryArenaBase* arena, const Inter::Anim& inter, const model::ModelSkeleton& skelton) :
    arena_(arena),
    inter_(inter),
    skelton_(skelton),
    scale_(1.f),
    type_(AnimType::RELATIVE),
    bones_(arena),
    notes_(inter.getNotes()),
    stats_(arena)
{
}

AnimCompiler::~AnimCompiler()
{
}

void AnimCompiler::printStats(bool verbose) const
{
    stats_.print();

    if (verbose) {
        X_LOG0("Anim", "Per bone info:");

        for (auto& bone : bones_) {
            if (!bone.hasData()) {
                continue;
            }

            X_LOG0("Anim", "-> \"%s\"", bone.name.c_str());
            X_LOG_BULLET;

            auto basePosRel = bone.pos.basePosRel();
            basePosRel *= 2.54f;

            X_LOG0("Anim", "rel: ^6%g^7,^6%g^7,^6%g^7", basePosRel.x, basePosRel.y, basePosRel.z);
            X_LOG0("Anim", "ang: ^6%2" PRIuS "^7 full: ^6%d^7 large-f: ^6%d",
                bone.ang.numAngleFrames(), bone.ang.isFullFrames(), bone.ang.isLargeFrames());

            if (bone.ang.hasData()) {
                X_LOG_BULLET;

                auto& angles = bone.ang.getAngles();

                for (auto& a : angles) {
                    Quatf q = Quatf(a.angle);
                    auto euler = q.getEulerDegrees();
                    X_LOG0("Anim", "Angle(%" PRIi32 "): ^2(%g,%g,%g) %g^7 e: (%g,%g,%g)",
                        a.frame, q.v.x, q.v.y, q.v.z, q.w, euler.x, euler.y, euler.z);
                }
            }

            X_LOG0("Anim", "pos: ^6%2" PRIuS "^7 full: ^6%d^7 large-f: ^6%d^7 large-s: ^6%d",
                bone.pos.numPosFrames(), bone.pos.isFullFrames(), bone.pos.isLargeFrames(), bone.pos.isLargeScalers());

            if (bone.pos.hasData()) {
                X_LOG_BULLET;

                auto min = bone.pos.min();
                auto& r = bone.pos.range();

                X_LOG0("Anim", "min(%g,%g,%G)", min.x, min.y, min.z);
                X_LOG0("Anim", "range(%g,%g,%g)", r.x, r.y, r.z);

                auto& pos = bone.pos.getPositions();

                for (auto& p : pos) {
                    X_LOG0("Anim", "Pos(%" PRIi32 "): ^2(%g,%g,%g)", p.frame, p.delta.x, p.delta.y, p.delta.z);
                }
            }
        }
    }
}

void AnimCompiler::setScale(float scale)
{
    scale_ = scale;
}

void AnimCompiler::setLooping(bool loop)
{
    if (loop) {
        flags_.Set(CompileFlag::LOOPING);
    }
    else {
        flags_.Remove(CompileFlag::LOOPING);
    }
}

void AnimCompiler::disableOptimizations(bool disable)
{
    if (disable) {
        flags_.Set(CompileFlag::NO_OPTIMISE);
    }
    else {
        flags_.Remove(CompileFlag::NO_OPTIMISE);
    }
}

void AnimCompiler::setAnimType(AnimType::Enum type)
{
    type_ = type;
}

bool AnimCompiler::compile(const float posError, const float angError)
{
    if (type_ != AnimType::RELATIVE) {
        X_ERROR("Anim", "Compiling of none relative animations is not yet supported");
        return false;
    }

    // got any bones in the inter?
    if (inter_.getNumBones() < 1) {
        X_WARNING("Anim", "skipping compile of anim, source inter anim has no bones");
        return false;
    }
    // any anims in the model skeleton?
    if (skelton_.getNumBones() < 1) {
        X_WARNING("Anim", "skipping compile of anim, source model skeleton has no bones");
        return false;
    }

    if (inter_.getFps() > anim::ANIM_MAX_FPS) {
        X_ERROR("Anim", "inter anim fps exceeds max: %" PRIu32, anim::ANIM_MAX_FPS);
        return false;
    }
    if (inter_.getNumFrames() > anim::ANIM_MAX_FRAMES) {
        X_ERROR("Anim", "inter anim exceeds max frames: %" PRIu32, anim::ANIM_MAX_FRAMES);
        return false;
    }

    if (notes_.size() > ANIM_MAX_NOTES) {
        X_ERROR("Anim", "Exceeded max notes (%" PRIu32 ")", ANIM_MAX_NOTES);
        return false;
    }

    if (notes_.isNotEmpty()) {
        // sort them base on frame.
        std::stable_sort(notes_.begin(), notes_.end(), [](NoteArr::ConstReference lhs, NoteArr::ConstReference rhs) {
            return lhs.frame < rhs.frame;
        });

        // check per frame limits.
        int32_t num = 0;
        int32_t curFrame = notes_.front().frame;
        for (size_t i = 0; i < notes_.size(); i++) {
            num++;

            if (num >= ANIM_MAX_NOTES_PER_FRAME) {
                X_ERROR("Anim", "Exceeded max notes per frame (%" PRIu32 ") on frame: %" PRIi32, ANIM_MAX_NOTES_PER_FRAME, curFrame);
                return false;
            }

            if (notes_[i].frame != curFrame) {
                num = 0;
                curFrame = notes_[i].frame;
            }
        }
    }

    core::StopWatch timer;

    loadBones();

    if (bones_.isEmpty()) {
        X_WARNING("Anim", "skipping compile of anim, inter anim and model skelton have no bones in common");
        return true;
    }

    processBones(posError, angError);

    if (type_ == AnimType::RELATIVE) {
        for (auto& bone : bones_) {
            if (bone.parentIdx == -1) {
                bone.clearData();
            }
        }
    }

    // create list of un-animated bones.
    for (const auto& bone : bones_) {
        if (!bone.hasData()) {
            stats_.noneAnimatedBoneName.append(bone.name);
        }
    }

    // build some stats.
    stats_.scale = scale_;
    stats_.numFrames = inter_.getNumFrames();
    stats_.fps = inter_.getFps();
    stats_.totalBones = bones_.size();
    stats_.compileTime = timer.GetTimeVal();

    for (const auto& bone : bones_) {
        stats_.totalBonesAngleData += static_cast<int32_t>(bone.ang.hasData());
        stats_.totalBonesPosData += static_cast<int32_t>(bone.pos.hasData());
    }

    return true;
}

bool AnimCompiler::save(const core::Path<char>& path)
{
    X_ASSERT_NOT_NULL(gEnv->pFileSys);

    core::Path<char> fullPath(path);
    fullPath.setExtension(anim::ANIM_FILE_EXTENSION);

    core::FileFlags mode;
    mode.Set(core::FileFlag::RECREATE);
    mode.Set(core::FileFlag::WRITE);

    core::XFileScoped file;
    if (!file.openFile(fullPath, mode, core::VirtualDirectory::BASE)) {
        X_ERROR("Anim", "Failed to open output file for compiled animation: \"%ls\"", fullPath.c_str());
        return false;
    }

    return save(file.GetFile());
}


bool AnimCompiler::saveOS(const core::Path<wchar_t>& osPath)
{
    X_ASSERT_NOT_NULL(gEnv->pFileSys);

    core::Path<wchar_t> fullPath(osPath);
    fullPath.setExtension(anim::ANIM_FILE_EXTENSION_W);

    core::FileFlags mode;
    mode.Set(core::FileFlag::RECREATE);
    mode.Set(core::FileFlag::WRITE);

    core::XFileScoped file;
    if (!file.openFileOS(fullPath, mode)) {
        X_ERROR("Anim", "Failed to open output file for compiled animation: \"%ls\"", fullPath.c_str());
        return false;
    }

    return save(file.GetFile());
}

bool AnimCompiler::save(core::XFile* pFile)
{
    X_ASSERT_NOT_NULL(gEnv);

    const size_t numBonesWithData = core::accumulate(bones_.begin(), bones_.end(), 0_sz, [](const Bone& b) {
        return static_cast<int32_t>(b.hasData());
    });

    anim::AnimHeader hdr;
    hdr.version = anim::ANIM_VERSION;
    hdr.type = type_;
    hdr.numBones = safe_static_cast<uint8_t, size_t>(numBonesWithData);
    hdr.numFrames = safe_static_cast<uint16_t, uint32_t>(inter_.getNumFrames());
    hdr.fps = safe_static_cast<uint16_t, uint32_t>(inter_.getFps());

    if (flags_.IsSet(CompileFlag::LOOPING)) {
        hdr.flags.Set(AnimFlag::LOOP);
    }

    if (notes_.isNotEmpty()) {
        hdr.flags.Set(AnimFlag::NOTES);
    }

    core::ByteStream stream(arena_);

    // write the bone names.
    for (const auto& bone : bones_) {
        if (!bone.hasData()) {
            continue;
        }

        stream.write(bone.name.c_str(), core::strUtil::StringBytesIncNull(bone.name));
    }

    // now we save the data.
    for (const auto& bone : bones_) {
        if (!bone.hasData()) {
            continue;
        }

        BoneFlags flags;
        if (bone.pos.isLargeScalers()) {
            flags.Set(BoneFlag::LargePosScalers);
        }
        if (bone.pos.isFullFrames()) {
            flags.Set(BoneFlag::PosFullFrame);
        }
        if (bone.pos.isLargeFrames()) {
            flags.Set(BoneFlag::PosLargeFrames);
        }
        if (bone.ang.isFullFrames()) {
            flags.Set(BoneFlag::AngFullFrame);
        }
        if (bone.ang.isLargeFrames()) {
            flags.Set(BoneFlag::AngLargeFrames);
        }

        stream.write(flags);

        bone.ang.save(stream);
        bone.pos.save(stream);
    }

    if (notes_.isNotEmpty()) {
        NoteTrackHdr noteHdr;
        noteHdr.num = safe_static_cast<uint16_t>(notes_.size());

        static_assert(std::numeric_limits<decltype(noteHdr.num)>::max() >= ANIM_MAX_NOTES, "Can't represent max notes");

        stream.write(noteHdr);

        size_t noteValueOffset = 0;
        for (auto& n : notes_) {
            if (n.value.length() > ANIM_MAX_NOTE_NAME_LENGTH) {
                X_ERROR("Anim", "Note value \"%s\" has length %" PRIuS " which exceeds limit of %" PRIu32,
                    n.value.c_str(), n.value.length(), ANIM_MAX_NOTE_NAME_LENGTH);
                return false;
            }

            auto numFrames = inter_.getNumFrames();
            auto fraction = static_cast<float>(n.frame) / numFrames;

            X_ASSERT(fraction >= 0.f && fraction <= 1.f, "Fraction out of range")(fraction); 

            Note note;
            note.value = safe_static_cast<uint16_t>(noteValueOffset);
            note.frame = fraction;

            noteValueOffset += core::strUtil::StringBytesIncNull(n.value);

            stream.write(note);
        }

        for (auto& n : notes_) {
            stream.write(n.value.c_str(), core::strUtil::StringBytesIncNull(n.value));
        }
    }

    hdr.dataSize = safe_static_cast<uint32_t>(stream.size());

    if (pFile->writeObj(hdr) != sizeof(hdr)) {
        X_ERROR("Anim", "Failed to write header");
        return false;
    }

    if (pFile->write(stream.data(), stream.size()) != stream.size()) {
        X_ERROR("Anim", "Failed to write data");
        return false;
    }

    return true;
}

void AnimCompiler::loadBones(void)
{
    bones_.reserve(inter_.getNumBones());

    size_t i, x, numModelBones = skelton_.getNumBones();
    for (i = 0; i < numModelBones; i++) {
        const char* pBoneName = skelton_.getBoneName(i);

        // got anim data for this bone?
        for (x = 0; x < inter_.getNumBones(); x++) {
            const anim::Inter::Bone& interBone = inter_.getBone(x);
            if (interBone.name == pBoneName) {
                size_t parentIdx = skelton_.getBoneParent(i);
                const Quatf& angleWorld = skelton_.getBoneAngle(i);
                const Vec3f& posWorld = skelton_.getBonePos(i);

                const Quatf& angleParWorld = skelton_.getBoneAngle(parentIdx);
                const Vec3f& posParWorld = skelton_.getBonePos(parentIdx);

                // work out parent index for anim.
                const char* pParentName = skelton_.getBoneName(parentIdx);
                auto it = std::find_if(bones_.begin(), bones_.end(), [pParentName](const Bone& b) {
                    return b.name == pParentName;
                });

                int32_t localParentIdx = -1;
                if (it != bones_.end()) {
                    localParentIdx = safe_static_cast<int32_t>(std::distance(bones_.begin(), it));
                }
                else {
                    if (pParentName != interBone.name) {
                        X_ASSERT_UNREACHABLE();
                    }
                }

                Vec3f posRel = posWorld;
                Quatf angleRel = angleWorld;

                if (localParentIdx >= 0) {
                    posRel = (posWorld - posParWorld) * angleParWorld.inverse();
                    angleRel = angleWorld * angleParWorld.inverse();
                }

                Bone bone(arena_);
                bone.name = interBone.name;
                bone.parentIdx = localParentIdx;
                bone.pos.setBasePositions(posWorld, posRel);
                bone.ang.setBaseOrients(angleWorld, angleRel);

                for (size_t num = 0; num < interBone.data.size(); num++) {
                    bone.pos.appendFullPos(interBone.data[num].position * scale_);
                    bone.ang.appendFullAng(interBone.data[num].rotation);
                }

                bones_.push_back(std::move(bone));
                break;
            }
        }

        if (x == inter_.getNumBones()) {
            // we don't have anim data for this model bone.
            stats_.notPresentBoneNames.append(core::string(pBoneName));
        }
    }
}

void AnimCompiler::processBones(const float posError, const float angError)
{
    for (auto& bone : bones_) {
        if (bone.parentIdx >= 0) {
            auto& parBone = bones_[bone.parentIdx];
            bone.pos.calculateRelativeData(parBone.pos, parBone.ang);
            bone.ang.calculateRelativeData(parBone.pos, parBone.ang);
        }
        else {
            bone.pos.calculateRelativeDataRoot();
            bone.ang.calculateRelativeDataRoot();
        }

        if (flags_.IsSet(CompileFlag::NO_OPTIMISE)) {
            bone.ang.calculateFullFrames();
            bone.pos.calculateFullFrames();
        }
        else {
            bone.ang.calculateDeltaFrames(0.05f);
            bone.pos.calculateDeltaFrames(posError);
        }

        bone.pos.buildScalers(posError);
    }
}

X_NAMESPACE_END