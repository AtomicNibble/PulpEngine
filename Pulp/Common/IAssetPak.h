#pragma once


#ifndef _X_ASSET_PAK_I_H_
#define _X_ASSET_PAK_I_H_

#include <Util\FlagsMacros.h>
#include <Time\CompressedStamps.h>

#include <IAssetDb.h>
#include <ICompression.h>

#include <array>

X_NAMESPACE_BEGIN(AssetPak)

// AssetPak: 
//	Description:	Asset Package
//	File Ext:		.ap
//	Version:		1.0
//	Info:
//	
//	The asset package is like a packed directory.
//	it can contain many diffrent assets.
//  there is a table at the top gving information about all the assets.
//  so that we only have to read the start to get all the names of the assets for caching.
//
//  each table entry will provide a offset for seeking to the asset.
//  so that a asset can be loaded.
//  
//  asset specific headers are stored at the start of the asset.
//
//  the asset names are relative and can contain folders.
//
//  Example Names:
//		images/wall.tx
//		sounds/guns/reload_mp5.wav
//		sounds/guns/reload_ak47.wav
//
//	we store a string pool at the top, which contains strings from assets.
//  since many assets will share bone bones.
//  the pool is 16byte padded null terminated strings.
//  meaning the full tables can be loaded as a single block.
//
//  assests have there strings names replaced with uin16_t which can be used for lookup.
//  example:
//		int16_t idx = 536;
//		const char* str = StrPoolGet(idx);
//
//	Pool Example:
//  j_gun...........j_potato.........
//	j_chicken.....
//
//	FastUpdate:
//		the pak supports fast upates. we leave room for additional entryies and strings.
//		so that new files can be added on the end without a full rebuild.
//	
//		Update of exsiting asset:
//			If the asset is smaller than the current, it's updated in place
//			otherwise it is appended on the end.
//			the table entry is updated to point to the new appended data.
//		
//		Adding of new asset:
//			Adding a new asset in fast mode appends the data to the end, 
//			adding a new entry into the table, as well as adding any new 
//			strings into the string pool.
//
//			The following precoditions must be met:
//			* Any new strings must fit in the string pool
//			* we have a free asset slot.
//			
//		the amout of space left in the string pool is not fixed.
//      the following constants are used when writing a fastUpdate enabled pak.
//		
//		*Note: the actual reverse written may be smaller to fit inside the formats limits.
//		
//		PAK_FU_STRING_POOL_RESERVER	// number of bytes added to end.
//		PAK_FU_SPARE_ASSET_SLOTS	// number of blank entries added.
//	
//  Limits:
//		max_str_pool_size:  1Mb (default padding scheme)
//		asset_name_len:		64
//		max_assets:			1 << 32
//		max_total_size:		(1 << 32 * 64) 256gb
//		max_asset_size:		1 << 32
//
//
//
//	How do i want looks to work?
//  I want to be able to ask for a file and open it.
//  so need a lookup based on name, the index of the name will be the index in the table.
//  basically want a hash index i think.


static const uint32_t PAK_MAGIC = core::X_FOURCC<uint32_t>('a', 'p', 'a', 'k');
static const uint8_t  PAK_VERSION = 1;
static const char* PAK_FILE_EXTENSION = "apak";


static const size_t PAK_BLOCK_PADDING = 16; // each section of the pak file is aligned to this, aka string / entry data.
static const uint32_t PAK_ASSET_PADDING = 64;
static const uint64_t PAK_MAX_SIZE = static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) * PAK_ASSET_PADDING;
static const uint32_t PAK_MAX_ASSETS = std::numeric_limits<uint32_t>::max();
static const uint32_t PAK_MAX_ASSET_SIZE = 1024 * 1024 * 1024; // allow from for some compression algo's that use 32bit signed types.



// Fast Update defines
static const uint32_t PAK_FU_STRING_POOL_RESERVER = 2048;
static const uint32_t PAK_FU_SPARE_ASSET_SLOTS = 128;

// string pool
static const uint32_t PAK_STR_POOL_BLOCK_SIZE = 16; // smaller = less waste / larger = more capaciy.


X_ENSURE_GE(PAK_STR_POOL_BLOCK_SIZE, 8, "padding must be greater or equal to 8");


struct AssetOffset
{
	static const uint64_t SHIFT_VALUE = 6ull;
	static_assert((1 << SHIFT_VALUE) == PAK_ASSET_PADDING, "Incorrect shift value");

	operator uint64_t() const {
		return ((uint64_t)(offset_)) << 6ull;
	}

	void operator=(uint64_t val)  {
		offset_ = (uint32_t)(val >> 6ull);
	}
private:
	uint32_t offset_;
};

X_DECLARE_FLAGS8(APakFlag)(
	DIRTY, 
	FAST_UPDATE, 
	FULL_LOAD,
	SHARED_DICTS		// Pack contains shared dictonaries.
);

typedef Flags8<APakFlag> APakFlags;

X_DECLARE_FLAGS8(APakEntryFlag)(
	FORCE_REBUILD
);

typedef Flags8<APakEntryFlag> APakEntryFlags;

typedef assetDb::AssetType AssetType;
typedef assetDb::AssetId AssetId;

X_PACK_PUSH(4)

struct APakSharedDicHdr
{
	uint32_t offset;
	uint32_t size;
};

typedef std::array<APakSharedDicHdr, AssetType::ENUM_COUNT> APakSharedDicArr;
typedef std::array<core::Compression::Algo::Enum, AssetType::ENUM_COUNT> CompressionAlgoArr;

struct APakDictInfo
{
	APakSharedDicArr sharedHdrs;
};

struct APakHeader
{
	X_INLINE bool isValid(void) const {
		return magic == PAK_MAGIC;
	}
	X_INLINE bool isCompressed(void) const {
		return size != inflatedSize;
	}

	uint32_t	magic;
	uint8_t		version;
	APakFlags	flags;
	uint16_t	unused;
	uint64_t	size;			
	uint64_t	inflatedSize;
	uint32_t	numAssets;
	core::DateTimeStampSmall modified; // 8

	uint32_t stringDataOffset;
	uint32_t entryTableOffset;
	uint32_t dictOffset;
	uint32_t dataOffset;

	// not needed to unpack, just meta data.
	CompressionAlgoArr algos;

	uint8_t pad[17];
};

struct APakStrPool
{
	uint32_t poolSize;
	uint32_t numStrings;
	uint32_t freeSpace; // bytes.
	uint16_t lastIndex; // used for appending.
	uint16_t maxIndex;	// poolsize / padscheme
};


struct APakEntry
{
	X_INLINE bool isCompressed(void) const {
		return size != inflatedSize;
	}

	AssetId							id;
	AssetType::Enum					type; 
	APakEntryFlags					flags;
	uint16_t						_pad;
	AssetOffset						offset;
	uint32_t						size;
	uint32_t						inflatedSize;
};

X_PACK_POP


// check sizes.
X_ENSURE_SIZE(AssetOffset, 4);
X_ENSURE_SIZE(APakSharedDicHdr, 8);

X_ENSURE_SIZE(APakHeader, 80);
X_ENSURE_SIZE(APakStrPool, 16);
X_ENSURE_SIZE(APakEntry, 20);




X_NAMESPACE_END

#endif // !_X_ASSET_PAK_I_H_
