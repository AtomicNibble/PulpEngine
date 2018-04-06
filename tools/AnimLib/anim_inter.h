#pragma once

#include <Containers\Array.h>
#include <String\Path.h>

#include <IAnimation.h>

X_NAMESPACE_DECLARE(core,
    class XLexer);

X_NAMESPACE_BEGIN(anim)

namespace Inter
{
    struct FrameData
    {
        Vec3f position;
        Vec3f scale;
        Matrix33f rotation;
    };

    struct Bone
    {
        typedef core::Array<FrameData> BoneData;

    public:
        Bone(core::MemoryArenaBase* arena);
        ~Bone();

    public:
        core::string name; // using this requires gEnv->pArena
        BoneData data;
    };

    struct Note
    {
        core::string value;
        int32_t frame;
    };

    struct SourceInfo
    {
        SourceInfo()
        {
            startFrame = 0;
            endFrame = 0;
        }

        core::string sourceFile;
        int32_t startFrame;
        int32_t endFrame;
    };

    class Anim
    {
    public:
        typedef core::Array<Note> NoteArr;
        typedef core::Array<Bone> BoneArr;

    public:
        Anim(core::MemoryArenaBase* arena);
        ~Anim() = default;

        bool load(core::Path<wchar_t>& path);
        bool load(const core::Array<uint8_t>& fileData);
        bool load(const core::ByteStream& fileData);

        bool save(core::Path<wchar_t>& path) const;
        bool save(core::ByteStream& stream) const;

        void setSourceInfo(const core::string& sourceFile, int32_t startFrame, int32_t endFrame);

        int32_t getNumFrames(void) const;
        int32_t getFps(void) const;
        size_t getNumBones(void) const;
        const Bone& getBone(size_t idx) const;
        const NoteArr& getNotes(void) const;

    private:
        bool ParseData(core::XLexer& lex);
        static bool ReadheaderToken(core::XLexer& lex, const char* pName, int32_t& valOut, bool optional);
        bool ReadNotes(core::XLexer& lex, int32_t numNotes);
        bool ReadBones(core::XLexer& lex, int32_t numBones);
        bool ReadFrameData(core::XLexer& lex, int32_t numBones);

    protected:
        core::MemoryArenaBase* arena_;
        SourceInfo srcInfo_;

        int32_t numFrames_;
        int32_t fps_;

        BoneArr bones_;
        NoteArr notes_;
    };

} // namespace Inter

X_NAMESPACE_END