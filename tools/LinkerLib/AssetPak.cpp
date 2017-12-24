#include "stdafx.h"
#include "AssetPak.h"

#include <IAssetPak.h>
#include <IFileSys.h>

X_NAMESPACE_BEGIN(AssetPak)

Asset::Asset(core::string& name, AssetType::Enum type, core::MemoryArenaBase* arena) :
	name(name),
	type(type),
	data(arena)
{

}


// -----------------------------------------------------

AssetPakBuilder::AssetPakBuilder(core::MemoryArenaBase* arena) :
	arena_(arena),
	assets_(arena)
{

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
	{
		size_t stringDataSize = 0;
		for (const auto& a : assets_)
		{
			stringDataSize += core::strUtil::StringBytesIncNull(a.name);
		}

		strings.reserve(stringDataSize);

		for (const auto& a : assets_)
		{
			strings.write(a.name.data(), core::strUtil::StringBytesIncNull(a.name));
		}
	}

	// write all the asset entries.
	{
		entries.reserve(sizeof(APakEntry) * assets_.size());

		for (const auto& a : assets_)
		{
			APakEntry entry;
			entry.id = 0;
			entry.type = a.type;
			entry.flags.Clear();
			entry.offset = 0;
			entry.unused = 0xffff;
			entry.size = safe_static_cast<uint32_t>(a.data.size());
			entry.inflatedSize = 0;

			entries.write(entry);
		}
	}

	// write all the asset data.
	{
		size_t dataSize = 0;
		for (const auto& a : assets_)
		{
			if (a.data.size() > PAK_MAX_ASSET_SIZE) {
				X_ERROR("AssetPak", "Asset in pack is too large %" PRIuS " max: " PRIu32, a.data.size(), PAK_MAX_ASSET_SIZE);
				return false;
			}

			dataSize += a.data.size();
		}

		data.reserve(dataSize);

		for (const auto& a : assets_)
		{
			data.write(a.data.data(), a.data.size());
		}

	}

	uint64_t totalFileSize = 0;
	totalFileSize += sizeof(hdr);
	totalFileSize += strings.size();
	totalFileSize += entries.size();
	totalFileSize += data.size();

	if (totalFileSize > PAK_MAX_SIZE) {
		X_ERROR("AssetPak", "Asset exceeds max size %" PRIuS " max: " PRIu32, totalFileSize, PAK_MAX_SIZE);
		return false;
	}

	file.setSize(static_cast<int64_t>(totalFileSize));

	hdr.size = totalFileSize;

	if (file.writeObj(hdr) != sizeof(hdr)) {
		X_ERROR("AssetPak", "Failed to write header");
		return false;
	}

	if (file.write(strings.data(), strings.size()) != strings.size()) {
		X_ERROR("AssetPak", "Failed to write string data");
		return false;
	}

	if (file.write(entries.data(), entries.size()) != entries.size()) {
		X_ERROR("AssetPak", "Failed to write entry data");
		return false;
	}

	if (file.write(data.data(), data.size()) != data.size()) {
		X_ERROR("AssetPak", "Failed to write data");
		return false;
	}

	return true;
}


void AssetPakBuilder::addAsset(core::string& name, AssetType::Enum type)
{
	Asset asset(name, type, arena_);

	assets_.emplace_back(std::move(asset));
}

X_NAMESPACE_END
