#pragma once

#include <Containers\Array.h>
#include <Math\XQuatCompressed.h>
#include <String\Path.h>

#include <IModel.h>

X_NAMESPACE_DECLARE(core,
                    class XLexer);

X_NAMESPACE_BEGIN(model)

class ModelSkeleton
{
    typedef core::Array<core::StackString<model::MODEL_MAX_BONE_NAME_LENGTH>> TagNames;
    typedef core::Array<uint16_t> TagNameIdx;
    typedef core::Array<uint8_t> TagTree;
    typedef core::Array<Quatf> TagAngles;
    typedef core::Array<Vec3f> TagPos;

public:
    ModelSkeleton(core::MemoryArenaBase* arena);
    ~ModelSkeleton();

    void dumpToLog(void) const;

    bool LoadCompiledSkelton(const core::Path<char>& path);

    bool LoadRawModelSkelton(const core::Path<char>& path);
    bool LoadRawModelSkelton(const core::Array<uint8_t>& data);

    void scale(float scale);

    size_t getNumBones(void) const;

    const char* getBoneName(size_t idx) const;
    const Quatf getBoneAngle(size_t idx) const;
    const Vec3f getBonePos(size_t idx) const;
    const size_t getBoneParent(size_t idx) const;

private:
    bool LoadRawModelSkelton_int(core::XLexer& lex);

    bool ReadBones(core::XLexer& lex, int32_t numBones);
    bool ReadheaderToken(core::XLexer& lex, const char* pName, int32_t& valOut);

private:
    core::MemoryArenaBase* arena_;

    size_t numBones_;

    // built.
    TagNames tagNames_;
    // from file.
    TagNameIdx nameIdx_;
    TagTree tree_;
    TagAngles angles_;
    TagPos positions_;
};

X_NAMESPACE_END