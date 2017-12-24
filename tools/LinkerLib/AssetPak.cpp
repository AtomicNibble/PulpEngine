#include "stdafx.h"
#include "AssetPak.h"

#include <IAssetPak.h>
#include <IFileSys.h>

X_NAMESPACE_BEGIN(AssetPak)

Asset::Asset(const core::string& name, AssetType::Enum type, DataVec&& data, core::MemoryArenaBase* arena) :
	name(name),
	type(type),
	data(std::move(data))
{
	X_UNUSED(arena);
}


// -----------------------------------------------------

AssetPakBuilder::AssetPakBuilder(core::MemoryArenaBase* arena) :
	arena_(arena),
	assets_(arena)
{
	assets_.reserve(1024);
}

bool AssetPakBuilder::save(core::Path<char>& path)
{
	if (assets_.size() > PAK_MAX_ASSETS) {
		X_ERROR("AssetPak", "Pak contains too many assets %" PRIuS " max: " PRIu32, assets_.size(), PAK_MAX_ASSETS);
		return false;
	}

	if (assets_.isEmpty()) {
		X_ERROR("AssetPak", "No assets in pak");
		return false;
	}

	core::XFileScoped file;
	core::fileModeFlags mode;
	mode.Set(core::fileMode::RECREATE);
	mode.Set(core::fileMode::WRITE);

	// ensure correct extension.
	core::Path<char> pathExt(path);
	pathExt.setExtension(PAK_FILE_EXTENSION);

	if (!file.openFile(pathExt.c_str(), mode)) {
		X_ERROR("AssetPak", "Failed to open file for saving");
		return false;
	}

	APakHeader hdr;
	core::zero_object(hdr._pad);
	hdr.magic = PAK_MAGIC;
	hdr.version = PAK_VERSION;
	hdr.flags.Clear();
	hdr.unused = 0;
	hdr.size = 0;
	hdr.inflatedSize = 0;
	hdr.numAssets = safe_static_cast<uint32_t>(assets_.size());
	hdr.modified = core::DateTimeStampSmall::systemDateTime();

	// not sure if want this in header or not.
	APakDictInfo dictInfo;
	core::zero_object(dictInfo.sharedHdrs);

	core::ByteStream strings(arena_);
	core::ByteStream entries(arena_);
	core::ByteStream data(arena_);


	// write all the strings.
	uint64_t stringDataSize = 0;
	uint64_t dataSize = 0;

	{
		for (const auto& a : assets_)
		{
			stringDataSize += core::strUtil::StringBytesIncNull(a.name);
		}

		stringDataSize = core::bitUtil::RoundUpToMultiple(stringDataSize, PAK_BLOCK_PADDING);

		strings.reserve(stringDataSize);

		for (const auto& a : assets_)
		{
			strings.write(a.name.data(), core::strUtil::StringBytesIncNull(a.name));
		}

		strings.alignWrite(PAK_BLOCK_PADDING);
	}

	// write all the asset entries.
	const uint64_t entryTablesize = sizeof(APakEntry) * assets_.size();

	{
		entries.reserve(entryTablesize);

		uint64_t assetOffset = 0;

		for (const auto& a : assets_)
		{
			X_ASSERT_ALIGNMENT(assetOffset, PAK_ASSET_PADDING, 0);
			
			APakEntry entry;
			entry.id = -1;
			entry.type = a.type;
			entry.flags.Clear();
			entry.offset = assetOffset;
			entry.size = safe_static_cast<uint32_t>(a.data.size());
			entry.inflatedSize = 0;

			entries.write(entry);

			assetOffset += core::bitUtil::RoundUpToMultiple<uint64_t>(a.data.size(), PAK_ASSET_PADDING);
		}

		entries.alignWrite(PAK_BLOCK_PADDING);
	}

	// write all the asset data.
	{
		for (const auto& a : assets_)
		{
			if (a.data.size() > PAK_MAX_ASSET_SIZE) {
				X_ERROR("AssetPak", "Asset in pack is too large %" PRIuS " max: " PRIu32, a.data.size(), PAK_MAX_ASSET_SIZE);
				return false;
			}

			dataSize += core::bitUtil::RoundUpToMultiple<uint64_t>(a.data.size(), PAK_ASSET_PADDING);
		}

		data.reserve(dataSize);

		for (const auto& a : assets_)
		{
			X_ASSERT_ALIGNMENT(data.size(), PAK_ASSET_PADDING, 0);

			data.write(a.data.data(), a.data.size());
			data.alignWrite(PAK_ASSET_PADDING);
		}
	}

	// calculate the offsets of the data.
	const uint64_t entryTableOffset = core::bitUtil::RoundUpToMultiple(sizeof(hdr) + strings.size(), PAK_BLOCK_PADDING);
	const uint64_t dataOffset = core::bitUtil::RoundUpToMultiple(entryTableOffset + entryTablesize, PAK_BLOCK_PADDING);

	uint64_t totalFileSize = 0;
	totalFileSize += sizeof(hdr);
	totalFileSize += stringDataSize;
	totalFileSize += entryTablesize;
	totalFileSize += dataSize;

	if (totalFileSize > PAK_MAX_SIZE) {
		X_ERROR("AssetPak", "Asset exceeds max size %" PRIuS " max: " PRIu32, totalFileSize, PAK_MAX_SIZE);
		return false;
	}

	if (entryTableOffset > std::numeric_limits<decltype(hdr.entryTableOffset)>::max()) {
		X_ERROR("AssetPak", "Invalid entry table offset");
		return false;
	}
	if (dataOffset > std::numeric_limits<decltype(hdr.dataOffset)>::max()) {
		X_ERROR("AssetPak", "Invalid data offset");
		return false;
	}

	file.setSize(static_cast<int64_t>(totalFileSize));

	hdr.size = totalFileSize;
	hdr.stringDataOffset = sizeof(hdr);
	hdr.entryTableOffset = safe_static_cast<uint32_t>(entryTableOffset);
	hdr.dataOffset = safe_static_cast<uint32_t>(dataOffset);

	if (file.writeObj(hdr) != sizeof(hdr)) {
		X_ERROR("AssetPak", "Failed to write header");
		return false;
	}

	X_ASSERT_ALIGNMENT(file.tell(), PAK_BLOCK_PADDING, 0);

	if (file.write(strings.data(), strings.size()) != strings.size()) {
		X_ERROR("AssetPak", "Failed to write string data");
		return false;
	}

	X_ASSERT_ALIGNMENT(file.tell(), PAK_BLOCK_PADDING, 0);

	if (file.write(entries.data(), entries.size()) != entries.size()) {
		X_ERROR("AssetPak", "Failed to write entry data");
		return false;
	}

	X_ASSERT_ALIGNMENT(file.tell(), PAK_BLOCK_PADDING, 0);

	if (file.write(data.data(), data.size()) != data.size()) {
		X_ERROR("AssetPak", "Failed to write data");
		return false;
	}

	if (file.tell() != totalFileSize) {
		X_ERROR("AssetPak", "File size mismatch");
		return false;
	}

	return true;
}


void AssetPakBuilder::addAsset(const core::string& name, AssetType::Enum type, DataVec&& data)
{
	assets_.emplace_back(name, type, std::move(data), arena_);
}

X_NAMESPACE_END
