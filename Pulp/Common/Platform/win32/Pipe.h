#pragma once

#include <Time\TimeVal.h>

X_NAMESPACE_BEGIN(core)

namespace IPC
{

	class Pipe
	{
	public:

		X_DECLARE_ENUM(OpenMode)(DUPLEX, INBOUND, OUTBOUND);
		X_DECLARE_ENUM(PipeMode)(BYTE, MESSAGE);
		X_DECLARE_ENUM(Access)(OPEN);
		X_DECLARE_ENUM(ShareMode)(OPEN);

	public:
		Pipe();
		~Pipe();

		bool create(const char* name, OpenMode::Enum openMode, PipeMode::Enum pipeMode,
			size_t maxInstances, size_t outBufferSize,
			size_t inBufferSize, core::TimeVal defaultTimeOut);
		bool create(const wchar_t* name, OpenMode::Enum openMode, PipeMode::Enum pipeMode,
			size_t maxInstances, size_t outBufferSize,
			size_t inBufferSize, core::TimeVal defaultTimeOut);

		bool open(const char* name, Access::Enum desiredAccess, ShareMode::Enum shareMode);
		bool open(const wchar_t* name, Access::Enum desiredAccess, ShareMode::Enum shareMode);

		void close(void);
		bool isOpen(void) const;
		bool flush(void);

		bool Write(const void* pBuffer, size_t numBytesToWrite, size_t* pNumberBytesWritten = nullptr);
		bool Read(void* pBuffer, size_t numBytesToRead, size_t* pNumberBytesRead = nullptr);
		bool peek(void* pBuffer, size_t bufferSizeBytes, size_t* pNumberBytesRead = nullptr,
			size_t* pTotalBytesAvail = nullptr, size_t* pBytesLeftThisMsg = nullptr);

		bool getClientComputerName(core::StackString512& nameOut);
		bool getClientComputerName(core::StackStringW512& nameOut);
		bool getClientProcessId(PULONG clientProcessId);
		bool getClientSessionId(PULONG clientSessionId);
		bool getServerProcessId(PULONG serverProcessId);
		bool getServerSessionId(PULONG serverSessionId);
		bool impersonateClient(void);

	private:
		HANDLE hPipe_;
	};


} // namespace IPC

X_NAMESPACE_END