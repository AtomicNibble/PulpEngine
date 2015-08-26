#include "stdafx.h"
#include "TextDrawList.h"


X_NAMESPACE_BEGIN(render)



XTextDrawList::XTextDrawList() :
	data_(nullptr)
{

}

XTextDrawList::~XTextDrawList()
{
	free();
}


void XTextDrawList::setArena(core::MemoryArenaBase* arena)
{
	X_ASSERT_NOT_NULL(arena);

	data_.setArena(arena);
	data_.resize((sizeof(TextEntry)+200) * 512);
}


void XTextDrawList::addEntry(const Vec3f& vPos, const XDrawTextInfo& ti, const char* pStr)
{
	X_ASSERT_NOT_NULL(pStr);

	size_t strLen = core::strUtil::strlen(pStr);

	TextEntry entry;
	entry.pos = vPos;
	entry.color = ti.col;
	entry.flags = ti.flags;
	entry.strLen = safe_static_cast<uint32_t,size_t>(strLen);

	// round up required bytes to multiple of pointer size.
	size_t requiredBytes = core::bitUtil::RoundUpToMultiple(sizeof(TextEntry) + strLen + 1,
		sizeof(TextEntry*));

	data_.write(entry);
	data_.write(pStr, strLen + 1);
}


void XTextDrawList::clear()
{
	data_.reset();
}


void XTextDrawList::free()
{
	data_.free();
}


const XTextDrawList::TextEntry* XTextDrawList::getNextTextEntry(void)
{
	if (data_.isEmpty())
		return nullptr;

	TextEntry* pEntry = &data_.peek<TextEntry>();

	data_.skip<uint8_t>(sizeof(TextEntry) + pEntry->strLen + 1);

	return pEntry;
}



X_NAMESPACE_END