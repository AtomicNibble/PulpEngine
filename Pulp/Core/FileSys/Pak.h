#pragma once


X_NAMESPACE_BEGIN(core)

class OsFileAsync;

X_DECLARE_ENUM(PakMode)(
	MEMORY,	// pak is kept in memory
	STREAM  // all reads are async disk OP's
);

struct Pak
{
	Pak(core::MemoryArenaBase* arena);

	int32_t find(core::StrHash hash, const char* pName) const;


	StackString<64> name;

	// have you seen this pak format?
	// oh my!, yes it's very sexy I must say.
	// thanks..

	// OsFileAsync* pFile; // handle to the file.
	XFileAsync* pFile; // handle to the file.

						// when i open a pack i want to load all the strings and the entry table.
						// then i parse the null terms like a hot mess.
						// 
	uint32_t numAssets;
	uint32_t dataOffset;

	PakMode::Enum mode;

	core::Array<uint8_t> data; // from 0-data (or whole file if memory mode)
	core::Array<const char*> strings;

	const AssetPak::APakEntry* pEntires;

	core::XHashIndex hash;

	core::AtomicInt openHandles;
};


inline Pak::Pak(core::MemoryArenaBase* arena) :
	pFile(nullptr),
	numAssets(0),
	dataOffset(0),
	mode(PakMode::STREAM),
	data(arena),
	strings(arena),
	pEntires(nullptr),
	hash(arena)
{

}

inline int32_t Pak::find(core::StrHash nameHash, const char* pName) const
{
	auto idx = hash.first(nameHash);
	while (idx != -1 && strUtil::IsEqual(strings[idx], pName) == false)
	{
		idx = hash.next(idx);
	}

	return idx;
}


X_NAMESPACE_END