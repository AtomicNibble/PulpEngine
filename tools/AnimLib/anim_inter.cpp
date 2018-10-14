#include "stdafx.h"
#include "anim_inter.h"

#include <IFileSys.h>
#include <IModel.h>
#include <String\Lexer.h>

X_NAMESPACE_BEGIN(anim)

namespace Inter
{
    Bone::Bone(core::MemoryArenaBase* arena) :
        data(arena)
    {
    }

    Bone::~Bone()
    {
    }

    // =================================

    Anim::Anim(core::MemoryArenaBase* arena) :
        arena_(arena),
        numFrames_(0),
        fps_(ANIM_DEFAULT_FPS),
        bones_(arena),
        notes_(arena)
    {
    }

    bool Anim::load(core::Path<wchar_t>& filePath)
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pFileSys);

        // swap a woggle watch it toggle!
        if (filePath.isEmpty()) {
            X_ERROR("Anim", "invalid path");
            return false;
        }

        filePath.setExtension(anim::ANIM_INTER_FILE_EXTENSION_W);

        core::FileFlags mode;
        mode.Set(core::FileFlag::READ);

        core::XFileScoped file;
        if (file.openFile(filePath, mode)) {
            X_ERROR("Anim", "failed to open file: %ls", filePath.c_str());
            return false;
        }

        const size_t fileSize = safe_static_cast<size_t, uint64_t>(file.remainingBytes());
        if (fileSize < 1) {
            X_ERROR("Anim", "file size invalid");
            return false;
        }

        core::Array<char> fileData(arena_);
        fileData.resize(fileSize);

        const size_t bytesRead = file.read(fileData.ptr(), fileSize);
        if (bytesRead != fileSize) {
            X_ERROR("Anim", "failed to read file data. got: %" PRIuS " requested: %" PRIuS, bytesRead, fileSize);
            return false;
        }

        core::XLexer lex(fileData.begin(), fileData.end());

        return ParseData(lex);
    }

    bool Anim::load(const core::Array<uint8_t>& fileData)
    {
        core::XLexer lex(reinterpret_cast<const char*>(fileData.begin()), reinterpret_cast<const char*>(fileData.end()));

        return ParseData(lex);
    }

    bool Anim::load(const core::ByteStream& fileData)
    {
        core::XLexer lex(reinterpret_cast<const char*>(fileData.begin()), reinterpret_cast<const char*>(fileData.end()));

        return ParseData(lex);
    }

    bool Anim::save(core::Path<wchar_t>& path) const
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pFileSys);

        path.setExtension(ANIM_INTER_FILE_EXTENSION_W);

        {
            core::XFileScoped file;
            core::FileFlags mode;
            mode.Set(core::FileFlag::RECREATE);
            mode.Set(core::FileFlag::WRITE);
            mode.Set(core::FileFlag::SHARE);

            if (!gEnv->pFileSys->createDirectoryTree(path)) {
                X_ERROR("Anim", "Failed to create export directory");
            }

            if (!file.openFile(path, mode)) {
                X_ERROR("Anim", "Failed to open file for inter anim");
                return false;
            }

            core::ByteStream fileData(arena_);
            if (!save(fileData)) {
                return false;
            }

            if (file.write(fileData.data(), fileData.size()) != fileData.size()) {
                return false;
            }
        }

        return true;
    }

    bool Anim::save(core::ByteStream& stream) const
    {
        // I will store each tags data.
        // for each frame there will be a position and a quat.
        const int32_t numBones = safe_static_cast<int32_t>(bones_.size());
        const int32_t numNotes = safe_static_cast<int32_t>(notes_.size());
        const int32_t numFrames = getNumFrames();
        const int32_t fps = static_cast<int32_t>(fps_);

        // work out max possible size bufffer needed.
        const size_t maxFloatBytes = 24; // -6.4
        const size_t numFloatsPerEntry = 3 + 3 + 4;
        const size_t tagSizes = sizeof("POS ") + sizeof("SCALE ") + sizeof("ANG ");
        const size_t paddingSize = 10 * 6; // more than needed
        const size_t maxSizePerEntry = (maxFloatBytes * numFloatsPerEntry) + tagSizes + paddingSize;

        const size_t headerSize = 8096; // plenty for shiz
        const size_t requiredSize = ((maxSizePerEntry * numFrames) * numBones) + headerSize;

        stream.reset();
        stream.reserve(requiredSize);

        core::StackString<4096> buf;
        buf.clear();
        buf.appendFmt("// " X_ENGINE_NAME " intermidiate animation format\n");
        buf.appendFmt("// Source: \"%s\"\n", srcInfo_.sourceFile.c_str());
        buf.appendFmt("// TimeLine range: %" PRIi32 " <-> %" PRIi32 "\n", srcInfo_.startFrame, srcInfo_.endFrame);
        buf.appendFmt("\n");
        buf.appendFmt("VERSION %" PRIu32 "\n", anim::ANIM_INTER_VERSION);
        buf.appendFmt("BONES %" PRIi32 "\n", numBones);
        buf.appendFmt("FRAMES %" PRIi32 "\n", numFrames);
        buf.appendFmt("FPS %" PRIi32 "\n", fps);
        buf.appendFmt("NOTES %" PRIi32 "\n", numNotes);
        buf.appendFmt("\n");

        // notes.
        if (notes_.isNotEmpty()) {
            for (const auto& note : notes_) {
                if (note.frame < 0 || note.frame >= numFrames) {
                    X_ERROR("Anim", "Note has invalid frame: %" PRIi32, note.frame);
                    return false;
                }

                buf.appendFmt("NOTE %" PRIi32 " \"%s\"\n", note.frame, note.value.c_str());
            }

            buf.appendFmt("\n");
        }

        // list the bones.
        for (const auto& bone : bones_) {
            buf.appendFmt("BONE \"%s\"\n", bone.name.c_str());
        }

        buf.append("\n");

        stream.write(buf.c_str(), buf.length());

        buf.clear();

        // write each bones data.
        for (const auto& bone : bones_) {
            buf.appendFmt("BONE_DATA // \"%s\"\n", bone.name.c_str());
            stream.write(buf.c_str(), buf.length());

            X_ASSERT(static_cast<int32_t>(bone.data.size()) == numFrames, "Don't have bone data for all frames")(bone.data.size(), numFrames); 

            for (int32_t i = 0; i < numFrames; i++) {
                const anim::Inter::FrameData& data = bone.data[i];

                buf.clear();
                buf.appendFmt("POS ( %.8g %.8g %.8g )\n",
                    data.position.x,
                    data.position.y,
                    data.position.z);

                buf.appendFmt("SCALE ( %.4g %.4g %.4g )\n",
                    data.scale.x,
                    data.scale.y,
                    data.scale.z);

                const auto& ang = data.rotation;
                buf.appendFmt("ANG ((%f %f %f) (%f %f %f) (%f %f %f))\n",
                    ang.m00, ang.m01, ang.m02,
                    ang.m10, ang.m11, ang.m12,
                    ang.m20, ang.m21, ang.m22);

                buf.append("\n");

                stream.write(buf.c_str(), buf.length());
            }
        }

        if (stream.size() > requiredSize) {
            X_WARNING("Anim", "Inter file size exceeded calculated size");
        }

        return true;
    }

    void Anim::setSourceInfo(const core::string& sourceFile, int32_t startFrame, int32_t endFrame)
    {
        srcInfo_.sourceFile = sourceFile;
        srcInfo_.startFrame = startFrame;
        srcInfo_.endFrame = endFrame;
    }

    int32_t Anim::getNumFrames(void) const
    {
        return numFrames_;
    }

    int32_t Anim::getFps(void) const
    {
        return fps_;
    }

    size_t Anim::getNumBones(void) const
    {
        return bones_.size();
    }

    const Bone& Anim::getBone(size_t idx) const
    {
        return bones_[idx];
    }

    const Anim::NoteArr& Anim::getNotes(void) const
    {
        return notes_;
    }

    bool Anim::ParseData(core::XLexer& lex)
    {
        int32_t version, numBones, numNotes;

        /*
	Example header:
		VERSION 1
		BONES 46
		FRAMES 99
		FPS 30
	*/

        if (!ReadheaderToken(lex, "VERSION", version, false)) {
            return false;
        }
        if (!ReadheaderToken(lex, "BONES", numBones, false)) {
            return false;
        }
        if (!ReadheaderToken(lex, "FRAMES", numFrames_, false)) {
            return false;
        }
        if (!ReadheaderToken(lex, "FPS", fps_, false)) {
            return false;
        }
        if (!ReadheaderToken(lex, "NOTES", numNotes, true)) {
            return false;
        }

        // check dat version number slut.
        if (version < anim::ANIM_INTER_VERSION) {
            X_ERROR("Anim", "Anim file version is too old: %" PRIi32 " required: %" PRIu32,
                version, anim::ANIM_INTER_VERSION);
            return false;
        }
        // limit checks
        if (fps_ > anim::ANIM_MAX_FPS) {
            X_ERROR("Anim", "Anim file fps is too high: %" PRIi32 " max: %" PRIu32,
                fps_, anim::ANIM_MAX_FPS);
            return false;
        }
        if (fps_ < anim::ANIM_MIN_FPS) {
            X_ERROR("Anim", "Anim file fps is too low: %" PRIi32 " min: %" PRIu32,
                fps_, anim::ANIM_MIN_FPS);
            return false;
        }
        if (numBones > anim::ANIM_MAX_BONES) {
            X_ERROR("Anim", "Anim file has too many bones: %" PRIi32 " max: %" PRIu32,
                numBones, anim::ANIM_MAX_BONES);
            return false;
        }
        if (numNotes > anim::ANIM_MAX_NOTES) {
            X_ERROR("Anim", "Anim file has too many notes: %" PRIi32 " max: %" PRIu32,
                numNotes, anim::ANIM_MAX_NOTES);
            return false;
        }

        // how many bones!
        if (numBones < 1) {
            X_ERROR("Anim", "animation has zero bones");
            return false;
        }
        // we need some frames to play some games.
        if (numFrames_ < 1) {
            X_ERROR("Anim", "animation has zero frames");
            return false;
        }
        // fluffy pig sausages
        if (fps_ < 1) {
            X_ERROR("Anim", "animation has fps lower than 1");
            return false;
        }

        if (!ReadNotes(lex, numNotes)) {
            X_ERROR("Anim", "failed to parse note data");
            return false;
        }

        // fill my cofin.
        if (!ReadBones(lex, numBones)) {
            X_ERROR("Anim", "failed to parse bone data");
            return false;
        }

        // data time.
        if (!ReadFrameData(lex, numBones)) {
            X_ERROR("Anim", "failed to parse frame data");
            return false;
        }

        if (!lex.isEOF(true)) {
            X_ERROR("Anim", "trailing data in file");
            return false;
        }

        return true;
    }

    bool Anim::ReadheaderToken(core::XLexer& lex, const char* pName, int32_t& valOut, bool optional)
    {
        core::XLexToken token(nullptr, nullptr);

        valOut = 0;

        if (!lex.ReadToken(token)) {
            X_ERROR("Anim", "Failed to read token");
            return false;
        }

        if (token.GetType() != core::TokenType::NAME || !token.isEqual(pName)) {
            if (optional) {
                lex.UnreadToken(token);
                return true;
            }

            X_ERROR("Anim", "Failed to read '%s'", pName);
            return false;
        }

        // get value
        if (!lex.ReadToken(token)) {
            X_ERROR("Anim", "Failed to read '%s' value", pName);
            return false;
        }

        if (token.GetType() != core::TokenType::NUMBER) {
            X_ERROR("Anim", "Failed to read '%s' value, it's not of interger type", pName);
            return false;
        }

        valOut = token.GetIntValue();
        return true;
    }

    bool Anim::ReadNotes(core::XLexer& lex, int32_t numNotes)
    {
        notes_.reserve(numNotes);

        core::XLexToken token(nullptr, nullptr);

        // NOTE idx "value"
        for (int32_t i = 0; i < numNotes; i++) {
            if (!lex.ReadToken(token)) {
                X_ERROR("Anim", "Failed to read 'NOTE' token");
                return false;
            }

            if (!token.isEqual("NOTE")) {
                X_ERROR("Anim", "Failed to read 'NOTE' token");
                return false;
            }

            if (!lex.ReadTokenOnLine(token)) {
                X_ERROR("Anim", "Failed to read 'NOTE' frame");
                return false;
            }

            if (token.GetType() != core::TokenType::NUMBER) {
                X_ERROR("Anim", "Failed to read 'NOTE' frame expected number");
                return false;
            }

            int32_t frame = token.GetIntValue();
            if (frame < 0) {
                X_ERROR("Anim", "Invalid 'NOTE' frame index: %" PRIi32, frame);
                return false;
            }

            if (!lex.ReadTokenOnLine(token)) {
                X_ERROR("Anim", "Failed to read 'NOTE' value");
                return false;
            }

            if (token.GetType() != core::TokenType::STRING) {
                X_ERROR("Anim", "Failed to read 'NOTE' value");
                return false;
            }

            Note note;
            note.frame = frame;
            note.value = core::string(token.begin(), token.end());

            if (note.value.length() < 1) {
                X_ERROR("Anim", "note value too short: \"%s\"", note.value.c_str());
                return false;
            }
            if (note.value.length() > anim::ANIM_MAX_NOTE_NAME_LENGTH) {
                X_ERROR("Anim", "note valuetoo long: \"%s\" max: %" PRIu32,
                    note.value.c_str(), anim::ANIM_MAX_NOTE_NAME_LENGTH);
                return false;
            }

#if X_ANIM_NOTE_LOWER_CASE_VALUE
            note.value.toLower();
#endif // !X_ANIM_NOTE_LOWER_CASE_VALUE

            notes_.append(note);
        }

        return true;
    }

    bool Anim::ReadBones(core::XLexer& lex, int32_t numBones)
    {
        bones_.reserve(numBones);

        core::XLexToken token(nullptr, nullptr);

        // BONE "name"
        for (int32_t i = 0; i < numBones; i++) {
            if (!lex.ReadToken(token)) {
                X_ERROR("Anim", "Failed to read 'BONE' token");
                return false;
            }

            if (!token.isEqual("BONE")) {
                X_ERROR("Anim", "Failed to read 'BONE' token");
                return false;
            }

            // read the string
            if (!lex.ReadTokenOnLine(token)) {
                X_ERROR("Anim", "Failed to read 'BONE' token");
                return false;
            }

            if (token.GetType() != core::TokenType::STRING) {
                X_ERROR("Anim", "Failed to read 'BONE' name");
                return false;
            }

            Bone bone(arena_);
            bone.name = core::string(token.begin(), token.end());

            // validate the names
            if (bone.name.length() < 1) {
                X_ERROR("Anim", "bone name too short: \"%s\"", bone.name.c_str());
                return false;
            }
            if (bone.name.length() > model::MODEL_MAX_BONE_NAME_LENGTH) {
                X_ERROR("Anim", "bone name too long: \"%s\" max: %" PRIu32,
                    bone.name.c_str(), model::MODEL_MAX_BONE_NAME_LENGTH);
                return false;
            }

#if X_MODEL_BONES_LOWER_CASE_NAMES
            bone.name.toLower();
#endif // !X_MODEL_BONES_LOWER_CASE_NAMES

            bones_.append(bone);
        }

        return true;
    }

    bool Anim::ReadFrameData(core::XLexer& lex, int32_t numBones)
    {
        X_ASSERT(numBones == bones_.size(), "bones size should alread equal numbones")(numBones, bones_.size()); 

        // for each bone there is numFrames worth of data.
        // in bone order.
        for (auto& bone : bones_) {
            // data has a start tag.
            if (!lex.SkipUntilString("BONE_DATA")) {
                X_ERROR("Anim", "missing BONE_DATA tag");
                return false;
            }

            /*
		Example:
		POS 0 0 0
		SCALE 1 1 1
		ANG 0 -0.21869613 0 0.97579301
		*/

            Bone::BoneData& data = bone.data;

            data.reserve(numFrames_);
            data.clear();

            core::XLexToken token(nullptr, nullptr);

            for (int32_t i = 0; i < numFrames_; i++) {
                FrameData& fd = data.AddOne();

                // pos
                if (!lex.ExpectTokenString("POS")) {
                    return false;
                }
                if (!lex.Parse1DMatrix(3, &fd.position[0])) {
                    return false;
                }

                // scale
                if (!lex.ExpectTokenString("SCALE")) {
                    return false;
                }
                if (!lex.Parse1DMatrix(3, &fd.scale[0])) {
                    return false;
                }

                // angles
                if (!lex.ExpectTokenString("ANG")) {
                    return false;
                }
                if (!lex.Parse2DMatrix(3, 3, &fd.rotation[0])) {
                    return false;
                }
            }
        }

        return true;
    }

} // namespace Inter

X_NAMESPACE_END