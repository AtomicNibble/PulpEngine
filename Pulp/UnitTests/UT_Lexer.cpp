#include "stdafx.h"

#include <String\Lexer.h>
#include <String\StrRef.h>

#include <Util\UniquePointer.h>

#include <IFileSys.h>

X_USING_NAMESPACE;

using namespace core;

class XMapBrush;

class XMapPrimitive
{
public:
    enum
    {
        TYPE_INVALID = -1,
        TYPE_BRUSH,
        TYPE_PATCH
    };

public:
    XMapPrimitive(void)
    {
        type = TYPE_INVALID;
    }
    virtual ~XMapPrimitive(void)
    {
    }

    int GetType(void) const
    {
        return type;
    }

protected:
    int type;
};

class XMapBrushSide
{
    friend class XMapBrush;

public:
    XMapBrushSide(void)
    {
    }
    ~XMapBrushSide(void)
    {
    }
    const char* GetMaterial(void) const
    {
        return material.name.c_str();
    }
    const Planef& GetPlane(void) const
    {
        return plane;
    }

protected:
    struct MaterialInfo
    {
        core::StackString<64> name;
        Vec2f matRepeate;
        Vec2f shift;
        float rotate;
    };

    Planef plane;
    MaterialInfo material;
    MaterialInfo lightMap;

protected:
    static bool ParseMatInfo(XLexer& src, MaterialInfo& mat);
};

class XMapBrush : public XMapPrimitive
{
public:
    XMapBrush(void)
    {
        type = TYPE_BRUSH;
        sides.reserve(6);
    }
    ~XMapBrush(void);

    int GetNumSides(void) const
    {
        return (int)sides.size();
    }
    void AddSide(XMapBrushSide* side)
    {
        sides.push_back(side);
    }
    XMapBrushSide* GetSide(int i) const
    {
        return sides[i];
    }
    unsigned int GetGeometryCRC(void) const;

public:
    static XMapBrush* Parse(XLexer& src, const Vec3f& origin);

protected:
    std::vector<XMapBrushSide*> sides;
};

XMapBrush::~XMapBrush(void)
{
    for (auto& side : sides) {
        X_DELETE(side, g_arena);
    }
}

class XMapPatch : public XMapPrimitive
{
public:
    XMapPatch(void)
    {
        type = TYPE_PATCH;
    }
    ~XMapPatch(void)
    {
    }

public:
    static XMapPatch* Parse(XLexer& src, const Vec3f& origin);

protected:
};

XMapPatch* XMapPatch::Parse(XLexer& src, const Vec3f& origin)
{
    // goaty meshes!
    XLexToken token;
    core::StackString<64> mat_name, light_map;

    int groups, entries, dunno1, dunno2;
    int x, y;
    float v[3], t[4];
    int c[4];

    if (!src.ExpectTokenString("{")) {
        return nullptr;
    }

    // while we have pairs get naked and skip them.
    while (1) {
        if (!src.ReadToken(token)) {
            src.Error("XMapPatch::Parse: unexpected EOF");
            return false;
        }

        if (src.ReadTokenOnLine(token)) {
            src.SkipRestOfLine();
        }
        else {
            src.UnreadToken(token);
            break;
        }
    }

    // read the material name
    if (!src.ReadToken(token)) {
        src.Error("XMapPatch::Parse: unable to read material.");
        return false;
    }

    mat_name = core::StackString<64>(token.begin(), token.end());

    // read the light map name
    if (!src.ReadToken(token)) {
        src.Error("XMapPatch::Parse: unable to read light map material.");
        return false;
    }

    light_map = core::StackString<64>(token.begin(), token.end());

    // sometimes we have smmothing bullshit.
    if (src.PeekTokenString("smoothing")) {
        src.SkipRestOfLine();
    }

    // we now have goaty info.
    groups = src.ParseInt();
    entries = src.ParseInt();

    // dunno yet
    dunno1 = src.ParseInt();
    dunno2 = src.ParseInt();

    // we now how x groups each with y entryies.
    for (x = 0; x < groups; x++) {
        if (!src.ExpectTokenString("(")) {
            return nullptr;
        }

        for (y = 0; y < entries; y++) {
            // each line has a -v and a -t
            if (!src.ExpectTokenString("v"))
                return nullptr;

            v[0] = src.ParseFloat();
            v[1] = src.ParseFloat();
            v[2] = src.ParseFloat();

            // we can have a color here.
            if (!src.ReadToken(token)) {
                src.Error("XMapPatch::Parse: unexpected EOF");
                return false;
            }

            if (token.isEqual("c")) {
                c[0] = src.ParseInt();
                c[1] = src.ParseInt();
                c[2] = src.ParseInt();
                c[3] = src.ParseInt();

                if (!src.ExpectTokenString("t"))
                    return nullptr;
            }
            else if (!token.isEqual("t")) {
                src.Error("XMapPatch::Parse: expected t");
                return false;
            }

            t[0] = src.ParseFloat();
            t[1] = src.ParseFloat();
            t[2] = src.ParseFloat();
            t[3] = src.ParseFloat();

            // some lines have "f 1"
            // get rekt.
            src.SkipRestOfLine();
        }

        if (!src.ExpectTokenString(")")) {
            return nullptr;
        }
    }

    // read the last 2 } }
    if (src.ExpectTokenString("}")) {
        if (src.ExpectTokenString("}")) {
            // valid
            XMapPatch* patch = X_NEW(XMapPatch, g_arena, "MapPatch");
            return patch;
        }
    }

    return nullptr;
}

class XMapEntity
{
public:
    XMapEntity(void);
    ~XMapEntity(void);

    size_t GetNumPrimitives(void) const
    {
        return primitives.size();
    }
    XMapPrimitive* GetPrimitive(size_t i) const
    {
        return primitives[i];
    }
    void AddPrimitive(XMapPrimitive* p)
    {
        primitives.push_back(p);
    }
    unsigned int GetGeometryCRC(void) const;
    void RemovePrimitiveData();

public:
    static XMapEntity* Parse(XLexer& src, bool worldSpawn = false);

private:
    std::vector<XMapPrimitive*> primitives;
};

XMapEntity::XMapEntity()
{
}

XMapEntity::~XMapEntity(void)
{
    for (auto& prim : primitives) {
        X_DELETE(prim, g_arena);
    }
}

bool XMapBrushSide::ParseMatInfo(XLexer& src, XMapBrushSide::MaterialInfo& info)
{
    XLexToken token;

    // read the material name
    if (!src.ReadTokenOnLine(token)) {
        src.Error("MapBrushMat::Parse: unable to read brush material.");
        return false;
    }

    info.name = core::StackString<64>(token.begin(), token.end());

    info.matRepeate[0] = src.ParseFloat();
    info.matRepeate[1] = src.ParseFloat();

    info.shift[0] = src.ParseFloat();
    info.shift[1] = src.ParseFloat();

    info.rotate = src.ParseFloat();

    // dunno what this value is.
    src.ParseFloat();
    return true;
}

XMapBrush* XMapBrush::Parse(XLexer& src, const Vec3f& origin)
{
    Vec3f planepts[3];
    XLexToken token;
    XMapBrushSide* side;
    XMapBrush* brush;

    brush = X_NEW(XMapBrush, g_arena, "MapBrush");

    do {
        if (!src.ReadToken(token)) {
            src.Error("MapBrush::Parse: unexpected EOF");
            X_DELETE(brush, g_arena);
            return nullptr;
        }
        if (token.isEqual("}")) {
            break;
        }

        // here we may have to jump over brush epairs ( only used in editor )
        do {
            // if token is a brace
            if (token.isEqual("(")) {
                break;
            }
            // the token should be a key string for a key/value pair
            if (token.GetType() != TokenType::NAME) {
                src.Error("MapBrush::Parse: unexpected %.*s, expected '(' or pair key string.",
                    token.length(), token.begin());
                X_DELETE(brush, g_arena);
                return nullptr;
            }

            if (!src.ReadTokenOnLine(token) || (token.GetType() != TokenType::STRING && token.GetType() != TokenType::NAME)) {
                src.Error("MapBrush::Parse: expected pair value string not found.");
                X_DELETE(brush, g_arena);
                return nullptr;
            }

            // try to read the next key
            if (!src.ReadToken(token)) {
                src.Error("MapBrush::Parse: unexpected EOF");
                X_DELETE(brush, g_arena);
                return nullptr;
            }

            if (token.isEqual(";")) {
                if (!src.ReadToken(token)) {
                    src.Error("MapBrush::Parse: unexpected EOF");
                    X_DELETE(brush, g_arena);
                    return nullptr;
                }
            }

        } while (1);

        src.UnreadToken(token);

        side = X_NEW(XMapBrushSide, g_arena, "MapBrushSide");
        brush->sides.push_back(side);

        // read the three point plane definition
        if (!src.Parse1DMatrix(3, &planepts[0][0]) || !src.Parse1DMatrix(3, &planepts[1][0]) || !src.Parse1DMatrix(3, &planepts[2][0])) {
            src.Error("MapBrush::Parse: unable to read brush plane definition.");
            X_DELETE(brush, g_arena);
            return nullptr;
        }

        planepts[0] -= origin;
        planepts[1] -= origin;
        planepts[2] -= origin;

        side->plane.set(planepts[0], planepts[1], planepts[2]);

        XMapBrushSide::ParseMatInfo(src, side->material);
        XMapBrushSide::ParseMatInfo(src, side->lightMap);

    } while (1);

    return brush;
}

XMapEntity* XMapEntity::Parse(XLexer& src, bool worldSpawn)
{
    XLexToken token;
    XMapEntity* mapEnt;
    XMapBrush* mapBrush;
    XMapPatch* mapPatch;
    Vec3f origin;
    float v1, v2, v3;
    bool worldent;

    if (!src.ReadToken(token)) {
        return nullptr;
    }

    if (!token.isEqual("{")) {
        src.Error("MapEntity::Parse: { not found.");
        return nullptr;
    }

    mapEnt = X_NEW(XMapEntity, g_arena, "MapEnt");

    if (worldSpawn) {
        mapEnt->primitives.reserve(1024);
    }

    worldent = false;
    origin = Vec3f::zero();

    do {
        if (!src.ReadToken(token)) {
            src.Error("MapEntity::Parse: EOF without closing brace");
            return nullptr;
        }
        if (token.isEqual("}")) {
            break;
        }

        if (token.isEqual("{")) {
            // we need to check for 'mesh'
            if (!src.ReadToken(token)) {
                src.Error("MapEntity::Parse: EOF without closing brace");
                return nullptr;
            }

            if (worldent) {
                origin = Vec3f::zero();
            }

            if (token.isEqual("mesh") || token.isEqual("curve")) {
                mapPatch = XMapPatch::Parse(src, origin);
                if (!mapPatch) {
                    return nullptr;
                }

                mapEnt->AddPrimitive(mapPatch);
            }
            else {
                src.UnreadToken(token);
                mapBrush = XMapBrush::Parse(src, origin);
                if (!mapBrush) {
                    return nullptr;
                }

                mapEnt->AddPrimitive(mapBrush);
            }
        }
        else {
            core::StackString512 key, value;

            // parse a key / value pair
            key = StackString512(token.begin(), token.end());
            src.ReadTokenOnLine(token);
            value = StackString512(token.begin(), token.end());

            // strip trailing spaces
            value.trimWhitespace();
            key.trimWhitespace();

            if (key.isEqual("origin")) {
                v1 = v2 = v3 = 0;
                sscanf_s(value.c_str(), "%f %f %f", &v1, &v2, &v3);
                origin.x = v1;
                origin.y = v2;
                origin.z = v3;
            }
            else if (key.isEqual("classname") && value.isEqual("worldspawn")) {
                worldent = true;
            }
        }

    } while (1);

    return mapEnt;
}

class XMapFile
{
public:
    XMapFile();
    ~XMapFile();

    bool Parse(core::XFile* file);

    size_t GetNumEntities(void) const
    {
        return entities.size();
    }

    XMapEntity* GetEntity(size_t i) const
    {
        return entities[i];
    }

private:
    std::vector<XMapEntity*> entities;
};

XMapFile::XMapFile()
{
}

XMapFile::~XMapFile()
{
    for (auto& ent : entities) {
        X_DELETE(ent, g_arena);
    }
}

bool XMapFile::Parse(core::XFile* file)
{
    size_t size = safe_static_cast<size_t, uint64_t>(file->remainingBytes());

    core::UniquePointer<char[]> pData(g_arena, X_NEW_ARRAY(char, size, g_arena, "LexTextBuf"));

    entities.reserve(2048);

    if (file->read(pData.get(), (uint32_t)size) == size) {
        XLexer lexer(pData.get(), pData.get() + size);
        XLexToken token;
        XMapEntity* mapEnt;

        lexer.setFlags(LexFlag::NOSTRINGCONCAT | LexFlag::NOSTRINGESCAPECHARS | LexFlag::ALLOWPATHNAMES);

        // we need to parse up untill the first brace.
        while (lexer.ReadToken(token)) {
            if (token.isEqual("{")) {
                lexer.UnreadToken(token);
                break;
            }
        }

        while (1) {
            mapEnt = XMapEntity::Parse(lexer, (entities.size() == 0));
            if (!mapEnt) {
                break;
            }
            entities.push_back(mapEnt);
        }
    }

    return true;
}

TEST(Lexer, Mapfile)
{
    // load a map file Drool

    core::XFileScoped map_file;
    core::IFileSys::FileFlags mode;
    mode.Set(FileFlag::READ);

    if (map_file.openFile("alcatraz.map", mode)) {
        XMapFile map;

        EXPECT_TRUE(map.Parse(map_file.GetFile()));

        // work out some info.
        size_t num = map.GetNumEntities();
        size_t i, x;

        size_t num_patch = 0;
        size_t num_brush = 0;

        for (i = 0; i < num; i++) {
            XMapEntity* ent = map.GetEntity(i);
            XMapPrimitive* prim;

            for (x = 0; x < ent->GetNumPrimitives(); x++) {
                prim = ent->GetPrimitive(x);

                if (prim->GetType() == XMapPrimitive::TYPE_BRUSH)
                    num_brush++;
                else if (prim->GetType() == XMapPrimitive::TYPE_PATCH)
                    num_patch++;
            }
        }

        X_LOG0("MapInfo", "num entites: %" PRIuS, num);
        X_LOG0("MapInfo", "num brushes: %" PRIuS, num_brush);
        X_LOG0("MapInfo", "num patches: %" PRIuS, num_patch);
    }
    else {
    }
}

TEST(Lexer, WhiteSpaceAtEnd)
{
    // I had a bug where if there was white space at the end of the file it would
    // read the white space then continue to read past the end of the buffer
    // as the EOF check was performed before whitespae skipping :|

    const char textText[] = "hellow this text has trailing whitespace  ";

    core::XLexer lex(textText, textText + sizeof(textText));
    core::XLexToken token;

    while (lex.ReadToken(token)) {
    }

    // isEOF will trigger asset if past end.
    EXPECT_TRUE(lex.isEOF());
    // this will fail if past end since it performs end - cur;
    EXPECT_EQ(0, lex.BytesLeft());
}