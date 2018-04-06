#pragma once

#include <Time\TimeVal.h>
#include <Util\Flags.h>

X_NAMESPACE_BEGIN(core)

namespace IPC
{
    class Pipe
    {
    public:
        X_DECLARE_ENUM(CreateMode)
        (
            DUPLEX,  // The pipe is bi-directional; both server and client processes can read from and write to the pipe.
            INBOUND, // The flow of data in the pipe goes from client to server only.
            OUTBOUND // The flow of data in the pipe goes from server to client only.
        );
        X_DECLARE_ENUM(PipeMode)
        (
            BYTE,      // data write / read as byte
            MESSAGE_W, // data sent as msg but read as byte
            MESSAGE_RW // data write / read as msg
        );

        X_DECLARE_FLAGS(OpenMode)
        (READ, WRITE, APPEND, WRITE_FLUSH, RECREATE, SHARE, RANDOM_ACCESS);

        typedef Flags<OpenMode> OpenModeFlags;

    public:
        Pipe();
        ~Pipe();

        bool create(const char* name, CreateMode::Enum openMode, PipeMode::Enum pipeMode,
            size_t maxInstances, size_t outBufferSize,
            size_t inBufferSize, core::TimeVal defaultTimeOut);
        bool create(const wchar_t* name, CreateMode::Enum openMode, PipeMode::Enum pipeMode,
            size_t maxInstances, size_t outBufferSize,
            size_t inBufferSize, core::TimeVal defaultTimeOut);

        bool open(const char* name, OpenModeFlags openflags);
        bool open(const wchar_t* name, OpenModeFlags openflags);

        bool connect(void);
        bool disconnect(void);
        bool flush(void);

        void close(void);
        bool isOpen(void) const;

        bool write(const void* pBuffer, size_t numBytesToWrite, size_t* pNumberBytesWritten = nullptr);
        bool read(void* pBuffer, size_t numBytesToRead, size_t* pNumberBytesRead = nullptr);
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

    X_DECLARE_FLAG_OPERATORS(Pipe::OpenModeFlags);

} // namespace IPC

X_NAMESPACE_END