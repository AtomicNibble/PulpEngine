#pragma once

#ifndef X_TEXT_DRAW_LIST
#define X_TEXT_DRAW_LIST

#include <Containers\ByteStreamFifo.h>

X_NAMESPACE_BEGIN(render)

class XTextDrawList
{
public:
    XTextDrawList(core::MemoryArenaBase* arena);
    ~XTextDrawList();

    void setArena(core::MemoryArenaBase* arena);
    void addEntry(const Vec3f& vPos, const XDrawTextInfo& ti, const char* pStr);

    void setCapacity(size_t numBytes);

    void clear();
    void free();

    X_INLINE bool isEmpty(void) const
    {
        return data_.isEmpty();
    }

    struct TextEntry
    {
        const char* getText() const
        {
            return (char*)(this + 1);
        }

        Vec3f pos;
        Color8u color;
        Flags<render::DrawTextFlags> flags;
        uint32_t strLen;
        // string data follows.
    };

    const TextEntry* getNextTextEntry(void);

private:
    core::ByteStreamFifo data_;
};

X_NAMESPACE_END

#endif // !X_TEXT_DRAW_LIST