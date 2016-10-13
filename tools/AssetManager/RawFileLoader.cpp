#include "RawFileLoader.h"

#include <String\HumanSize.h>
#include <Compression\LZ4.h>
#include <Time\StopWatch.h>

X_NAMESPACE_BEGIN(assman)



RawFileLoader::RawFileLoader(core::MemoryArenaBase* arena, assetDb::AssetType::Enum type) :
	type_(type),
	compressed_(arena),
	thumbData_(arena)
{

}

RawFileLoader::~RawFileLoader()
{
	stopProcess();

	wait();
}

void RawFileLoader::loadFile(const QString& path)
{
	path_ = path;

	abort_ = false;
	start();
}

Vec2i RawFileLoader::getSrcDim(void) const
{
	return srcDim_;
}

const RawFileLoader::DataArr& RawFileLoader::getCompressedSrc(void) const
{
	return compressed_;
}

const RawFileLoader::DataArr& RawFileLoader::getThumbData(void) const
{
	return thumbData_;
}

void RawFileLoader::run()
{
	emit setProgress(0);

	QFile f(path_);
	if (!f.open(QFile::ReadOnly)) {
		X_ERROR("RawFile", "Failed to load, error opening file. Err: %s", qPrintable(f.errorString()));
		return;
	}

	const auto fileSize = f.size();

	core::Array<uint8_t> imgData(g_arena);
	imgData.resize(fileSize);

	// reading from disk is not that slow
	// just ofr very big images it's a little too slow to do in ui thread.
	const int32_t numChunks = 10;
	const int64_t minChunkSize = 1024 * 64;
	const int64_t chunkSize = core::bitUtil::RoundUpToMultiple(fileSize / numChunks, minChunkSize);

	int64_t bytesLeft = fileSize;
	char* pDst = reinterpret_cast<char*>(imgData.data());

	for (int32_t i = 0; i < numChunks; i++)
	{
		const auto bytesToRead = core::Min(bytesLeft, chunkSize);
		if (bytesToRead <= 0) {
			break;
		}

		if (f.read(pDst, bytesToRead) != bytesToRead) {
			X_ERROR("RawFile", "Error reading file data");
			return;
		}

		pDst += bytesToRead;
		bytesLeft -= bytesToRead;

		emit setProgress(i);
	}

	if (bytesLeft > 0) {
		if (f.read(pDst, bytesLeft) != bytesLeft) {
			X_ERROR("RawFile", "Error reading file data");
			return;
		}
	}

	emit setProgressLabel("Deflating src", numChunks);

	// defalte it.
	core::Compression::Compressor<core::Compression::LZ4> comp;

	{
		core::StopWatch timer;

		if (!comp.deflate(g_arena, imgData, compressed_, core::Compression::CompressLevel::HIGH))
		{
			X_ERROR("RawFile", "Failed to defalte src");
			return;
		}
		else
		{
			const auto elapsed = timer.GetMilliSeconds();
			const float percentageSize = (static_cast<float>(compressed_.size()) / static_cast<float>(imgData.size())) * 100;

			core::HumanSize::Str sizeStr, sizeStr2;
			X_LOG0("RawFile", "Defalated src %s -> %s(%.2g%%) %gms",
				core::HumanSize::toString(sizeStr, imgData.size()),
				core::HumanSize::toString(sizeStr2, compressed_.size()),
				percentageSize,
				elapsed);
		}
	}

	emit setProgressLabel("Processing data", numChunks + 10);

	if (type_ == assetDb::AssetType::IMG)
	{
		// load a image.
		QImage img;

		if (!img.loadFromData(imgData.data(), static_cast<uint32_t>(imgData.size()))) {
			X_ERROR("Img", "Error creating pixmap for preview");
			return;
		}

		srcDim_ = Vec2i(img.width(), img.height());

		emit setProgressLabel("Generating thumb", numChunks + 15);


		auto thumbImg = img.scaled(64, 64);

		QByteArray bytes;
		QBuffer buffer(&bytes);
		buffer.open(QIODevice::WriteOnly);
		if (!thumbImg.save(&buffer, "BMP")) {
			X_ERROR("Texture", "Failed to save thumb");
		}
		else
		{
			thumbData_.resize(bytes.count());
			std::memcpy(thumbData_.data(), bytes.constData(), thumbData_.size());
		}
	}
	else
	{
		// don't do anything with other asset types currently.

	}

	emit setProgress(numChunks + 20);
}

void RawFileLoader::stopProcess()
{
	mutex_.lock();
	abort_ = true;
	mutex_.unlock();
}



X_NAMESPACE_END