#pragma once


X_NAMESPACE_BEGIN(core)


#if X_ENABLE_FILE_STATS

struct XFileStats
{
	typedef core::StackString512 Str;

	X_INLINE XFileStats() {
		core::zero_this(this);
	}

	X_INLINE XFileStats& operator+=(const XFileStats& oth) {
		NumBytesRead += oth.NumBytesRead;
		NumBytesWrite += oth.NumBytesWrite;
		NumFilesOpened += oth.NumFilesOpened;
		NumSeeks += oth.NumSeeks;
		NumTells += oth.NumTells;
		NumByteLeftChecks += oth.NumByteLeftChecks;
		return *this;
	}

	X_INLINE XFileStats operator+(const XFileStats& oth) {
		XFileStats s = *this;
		s += oth;
		return s;
	}

	X_INLINE const Str::char_type* toString(Str& buf) const {

		core::HumanSize::Str str;

		buf.clear();
		buf.appendFmt("Read: ^6%s^~\n", core::HumanSize::toString(str, NumBytesRead));
		buf.appendFmt("Write: ^6%s^~\n", core::HumanSize::toString(str, NumBytesWrite));
		buf.appendFmt("FilesOpened: ^6%" PRIuS "^~\n", NumFilesOpened);
		buf.appendFmt("Seeks: ^6%" PRIuS "^~\n", NumSeeks);
		buf.appendFmt("TellRequests: ^6%" PRIuS "^~\n", NumTells);
		buf.appendFmt("RemainChecks: ^6%" PRIuS "^~\n", NumByteLeftChecks);
		return buf.c_str();
	}

	// 64bit as we can read more than 4gb in the 32bit build.
	uint64_t NumBytesRead;
	uint64_t NumBytesWrite;
	size_t NumFilesOpened;
	size_t NumSeeks;
	size_t NumTells;
	size_t NumByteLeftChecks;
};

struct IOQueueStats
{
	typedef core::StackString512 Str;

	X_INLINE IOQueueStats() {
		core::zero_this(this);
	}

	X_INLINE const Str::char_type* toString(Str& buf) const {

		core::HumanSize::Str str;

		buf.clear();
		buf.appendFmt("Read: ^6%s^~\n", core::HumanSize::toString(str, NumBytesRead));
		buf.appendFmt("Write: ^6%s^~\n", core::HumanSize::toString(str, NumBytesWrite));
		buf.appendFmt("FilesOpened: ^6%" PRIuS "^~\n", NumFilesOpened);
		
		for (uint32_t i = 0; i < IoRequest::ENUM_COUNT; i++)
		{
			buf.appendFmt("Req[%s]: ^6%" PRIuS "^~\n", IoRequest::ToString(i), RequestCounts[i]);
		}

		return buf.c_str();
	}


	uint64_t NumBytesRead;
	uint64_t NumBytesWrite;
	size_t NumFilesOpened;
	size_t RequestCounts[IoRequest::ENUM_COUNT];
};

#endif // !X_ENABLE_FILE_STATS


X_NAMESPACE_END