#include "EngineCommon.h"
#include "Pipe.h"

X_NAMESPACE_BEGIN(core)

namespace IPC
{
    namespace
    {
        DWORD GetCreateMode(Pipe::CreateMode::Enum mode)
        {
            if (mode == Pipe::CreateMode::DUPLEX) {
                return PIPE_ACCESS_DUPLEX;
            }
            if (mode == Pipe::CreateMode::INBOUND) {
                return PIPE_ACCESS_INBOUND;
            }
            if (mode == Pipe::Pipe::CreateMode::OUTBOUND) {
                return PIPE_ACCESS_OUTBOUND;
            }

            X_ASSERT_UNREACHABLE();
            return 0;
        }

        DWORD GetPipeMode(Pipe::PipeMode::Enum mode)
        {
            if (mode == Pipe::PipeMode::BYTE) {
                return PIPE_WAIT;
            }
            if (mode == Pipe::PipeMode::MESSAGE_W) {
                return PIPE_TYPE_MESSAGE | PIPE_WAIT;
            }
            if (mode == Pipe::PipeMode::MESSAGE_RW) {
                return PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT;
            }

            X_ASSERT_UNREACHABLE();
            return 0;
        }

        DWORD GetAccess(Pipe::OpenModeFlags mode)
        {
            DWORD val = 0;

            if (mode.IsSet(Pipe::OpenMode::WRITE)) {
                val |= FILE_WRITE_DATA;
            }
            if (mode.IsSet(Pipe::OpenMode::READ)) {
                val |= FILE_READ_DATA;
            }
            if (mode.IsSet(Pipe::OpenMode::APPEND)) {
                val |= FILE_APPEND_DATA;
            }

            X_ASSERT(val != 0, "Pipe must have one of the following modes, READ, WRITE, APPEND")
            (mode.ToInt());
            return val;
        }

        DWORD GetShareMode(Pipe::OpenModeFlags mode)
        {
            DWORD val = 0;

            if (mode.IsSet(Pipe::OpenMode::SHARE)) {
                if (mode.IsSet(Pipe::OpenMode::READ)) {
                    val |= FILE_SHARE_READ;
                }
                if (mode.IsSet(Pipe::OpenMode::WRITE)) {
                    val |= FILE_SHARE_WRITE;
                }

                // would i even want delete, or should i always included?
                val |= FILE_SHARE_DELETE;
            }
            return val;
        }

    } // namespace

    Pipe::Pipe()
    {
        hPipe_ = INVALID_HANDLE_VALUE;
    }

    Pipe::~Pipe()
    {
        close();
    }

    bool Pipe::create(const char* name, CreateMode::Enum createMode, PipeMode::Enum pipeMode,
        size_t maxInstances, size_t outBufferSize, size_t inBufferSize, core::TimeVal defaultTimeOut)
    {
        wchar_t buf[256];
        return create(core::strUtil::Convert(name, buf), createMode, pipeMode,
            maxInstances, outBufferSize, inBufferSize, defaultTimeOut);
    }

    bool Pipe::create(const wchar_t* name, CreateMode::Enum createMode, PipeMode::Enum pipeMode,
        size_t maxInstances, size_t outBufferSize, size_t inBufferSize, core::TimeVal defaultTimeOut)
    {
        if (isOpen()) {
            X_ERROR("Pipe", "Failed to create pipe: \"%ls\" a pipe is already open", name);
            return false;
        }

        DWORD timeoutMs = safe_static_cast<DWORD, int64_t>(defaultTimeOut.GetMilliSecondsAsInt64());

        hPipe_ = CreateNamedPipeW(name,
            GetCreateMode(createMode),
            GetPipeMode(pipeMode),
            safe_static_cast<DWORD, size_t>(maxInstances),
            safe_static_cast<DWORD, size_t>(outBufferSize),
            safe_static_cast<DWORD, size_t>(inBufferSize),
            timeoutMs,
            nullptr);

        if (hPipe_ == INVALID_HANDLE_VALUE) {
            core::lastError::Description Dsc;
            X_ERROR("Pipe", "Failed to create pipe: \"%ls\" Err: %s", name, core::lastError::ToString(Dsc));
            return false;
        }

        return true;
    }

    bool Pipe::open(const char* name, OpenModeFlags openflags)
    {
        wchar_t buf[256];
        return open(core::strUtil::Convert(name, buf), openflags);
    }

    bool Pipe::open(const wchar_t* name, OpenModeFlags openflags)
    {
        if (isOpen()) {
            X_ERROR("Pipe", "Failed to open pipe: \"%ls\" a pipe is already open", name);
            return false;
        }

        hPipe_ = CreateFileW(name,
            GetAccess(openflags),
            GetShareMode(openflags),
            nullptr,
            OPEN_EXISTING,
            0, // flags
            nullptr);

        if (hPipe_ == INVALID_HANDLE_VALUE) {
            core::lastError::Description Dsc;
            X_ERROR("Pipe", "Failed to open pipe: \"%ls\" Err: %s", name, core::lastError::ToString(Dsc));
            return false;
        }

        return true;
    }

    bool Pipe::connect(void)
    {
        X_ASSERT(isOpen(), "Pipe must be open to cionnect")
        (isOpen());

        if (!ConnectNamedPipe(hPipe_, nullptr)) {
            core::lastError::Description Dsc;
            X_ERROR("Pipe", "Failed to connect to pipe. Err: %s", core::lastError::ToString(Dsc));
            return false;
        }

        return true;
    }

    bool Pipe::disconnect(void)
    {
        X_ASSERT(isOpen(), "Pipe must be open to disconnect")
        (isOpen());

        if (!DisconnectNamedPipe(hPipe_)) {
            core::lastError::Description Dsc;
            X_ERROR("Pipe", "Failed to disconnect from pipe. Err: %s", core::lastError::ToString(Dsc));
            return false;
        }

        return true;
    }

    bool Pipe::flush(void)
    {
        X_ASSERT(isOpen(), "Pipe must be open to flush")
        (isOpen());

        if (!FlushFileBuffers(hPipe_)) {
            core::lastError::Description Dsc;
            X_ERROR("Pipe", "Failed to flush pipe. Err: %s", core::lastError::ToString(Dsc));
            return false;
        }

        return true;
    }

    void Pipe::close(void)
    {
        if (isOpen()) {
            if (!CloseHandle(hPipe_)) {
                core::lastError::Description Dsc;
                X_ERROR("Pipe", "Failed to close pipe. Err: %s", core::lastError::ToString(Dsc));
            }

            hPipe_ = INVALID_HANDLE_VALUE;
        }
    }

    bool Pipe::isOpen(void) const
    {
        return hPipe_ != INVALID_HANDLE_VALUE;
    }

    bool Pipe::write(const void* pBuffer, size_t numBytesToWrite, size_t* pNumberBytesWritten)
    {
        X_ASSERT(isOpen(), "Pipe must be open to write")
        (isOpen());
        X_ASSERT_NOT_NULL(pBuffer);

        DWORD bytesWritten = 0;

        if (pNumberBytesWritten) {
            *pNumberBytesWritten = 0;
        }

        if (!::WriteFile(hPipe_,
                pBuffer,
                safe_static_cast<DWORD, size_t>(numBytesToWrite),
                &bytesWritten,
                nullptr)) {
            core::lastError::Description Dsc;
            uint32_t lastErr = core::lastError::Get();
            X_ERROR("Pipe", "Failed to write to pipe. Err(%" PRIu32 "): %s", lastErr, core::lastError::ToString(lastErr, Dsc));
            return false;
        }

        if (bytesWritten == 0) {
            core::lastError::Description Dsc;
            uint32_t lastErr = core::lastError::Get();
            if (lastErr == ERROR_BROKEN_PIPE) {
                X_ERROR("Pipe", "Failed to write to a broken pipe. Err(%" PRIu32 "): %s", lastErr, core::lastError::ToString(lastErr, Dsc));
            }
            else {
                X_ERROR("Pipe", "Failed to write to pipe. Err(%" PRIu32 "): %s", lastErr, core::lastError::ToString(lastErr, Dsc));
            }
            return false;
        }

        if (pNumberBytesWritten) {
            *pNumberBytesWritten = safe_static_cast<DWORD, size_t>(bytesWritten);
        }

        return true;
    }

    bool Pipe::read(void* pBuffer, size_t numBytesToRead, size_t* pNumberBytesRead)
    {
        X_ASSERT(isOpen(), "Pipe must be open to read")
        (isOpen());
        X_ASSERT_NOT_NULL(pBuffer);

        DWORD bytesRead = 0;

        if (pNumberBytesRead) {
            *pNumberBytesRead = 0;
        }

        if (!::ReadFile(hPipe_,
                pBuffer,
                safe_static_cast<DWORD, size_t>(numBytesToRead),
                &bytesRead,
                nullptr)) {
            core::lastError::Description Dsc;
            uint32_t lastErr = core::lastError::Get();
            X_ERROR("Pipe", "Failed to read from pipe. Err(%" PRIu32 "): %s", lastErr, core::lastError::ToString(Dsc));
            return false;
        }

        if (bytesRead == 0) {
            core::lastError::Description Dsc;
            uint32_t lastErr = core::lastError::Get();
            if (lastErr == ERROR_BROKEN_PIPE) {
                X_ERROR("Pipe", "Failed to read from a broken pipe. Err(%" PRIu32 "): %s", lastErr, core::lastError::ToString(lastErr, Dsc));
            }
            else {
                X_ERROR("Pipe", "Failed to read from pipe. Err(%" PRIu32 "): %s", lastErr, core::lastError::ToString(lastErr, Dsc));
            }
            return false;
        }

        if (pNumberBytesRead) {
            *pNumberBytesRead = safe_static_cast<DWORD, size_t>(bytesRead);
        }

        return true;
    }

    bool Pipe::peek(void* pBuffer, size_t bufferSizeBytes, size_t* pNumberBytesRead,
        size_t* pTotalBytesAvail, size_t* pBytesLeftThisMsg)
    {
        X_ASSERT(isOpen(), "Pipe must be open to peek")
        (isOpen());
        X_ASSERT_NOT_NULL(pBuffer);

        DWORD bytesRead = 0;
        DWORD bytesAvail = 0;
        DWORD btesLeftMsg = 0;

        if (!::PeekNamedPipe(hPipe_,
                pBuffer,
                safe_static_cast<DWORD, size_t>(bufferSizeBytes),
                &bytesRead,
                &bytesAvail,
                &btesLeftMsg)) {
            core::lastError::Description Dsc;
            uint32_t lastErr = core::lastError::Get();
            X_ERROR("Pipe", "Failed to peek pipe. Err(%" PRIu32 "): %s", lastErr, core::lastError::ToString(Dsc));
            return false;
        }

        if (pNumberBytesRead) {
            *pNumberBytesRead = safe_static_cast<DWORD, size_t>(bytesRead);
        }
        if (pTotalBytesAvail) {
            *pTotalBytesAvail = safe_static_cast<DWORD, size_t>(bytesAvail);
        }
        if (pBytesLeftThisMsg) {
            *pBytesLeftThisMsg = safe_static_cast<DWORD, size_t>(btesLeftMsg);
        }

        return true;
    }

    bool Pipe::getClientComputerName(core::StackString512& nameOut)
    {
        X_ASSERT(isOpen(), "Pipe must be open to get pipe client computer name")
        (isOpen());

        nameOut.clear();

        char buf[512] = {0};
        if (!GetNamedPipeClientComputerNameA(hPipe_, buf, 512)) {
            core::lastError::Description Dsc;
            X_ERROR("Pipe", "Failed to get pipe client computer name. Err: %s", core::lastError::ToString(Dsc));
            return false;
        }

        nameOut = core::StackString512(buf);
        return true;
    }

    bool Pipe::getClientComputerName(core::StackStringW512& nameOut)
    {
        X_ASSERT(isOpen(), "Pipe must be open to get pipe client computer name")
        (isOpen());

        nameOut.clear();

        wchar_t buf[512] = {0};
        if (!GetNamedPipeClientComputerNameW(hPipe_, buf, 512 * sizeof(wchar_t))) {
            core::lastError::Description Dsc;
            X_ERROR("Pipe", "Failed to get pipe client computer name. Err: %s", core::lastError::ToString(Dsc));
            return false;
        }

        nameOut = core::StackStringW512(buf);
        return true;
    }

    bool Pipe::getClientProcessId(PULONG clientProcessId)
    {
        X_ASSERT(isOpen(), "Pipe must be open to get pipe client process id")
        (isOpen());
        X_ASSERT_NOT_NULL(clientProcessId);

        if (!GetNamedPipeClientProcessId(hPipe_, clientProcessId)) {
            core::lastError::Description Dsc;
            X_ERROR("Pipe", "Failed to get pipe pipe client process id. Err: %s", core::lastError::ToString(Dsc));
            return false;
        }

        return true;
    }

    bool Pipe::getClientSessionId(PULONG clientSessionId)
    {
        X_ASSERT(isOpen(), "Pipe must be open to get pipe client session id")
        (isOpen());
        X_ASSERT_NOT_NULL(clientSessionId);

        if (!GetNamedPipeClientSessionId(hPipe_, clientSessionId)) {
            core::lastError::Description Dsc;
            X_ERROR("Pipe", "Failed to get pipe pipe client session id. Err: %s", core::lastError::ToString(Dsc));
            return false;
        }

        return true;
    }

    bool Pipe::getServerProcessId(PULONG serverProcessId)
    {
        X_ASSERT(isOpen(), "Pipe must be open to get pipe server process id")
        (isOpen());
        X_ASSERT_NOT_NULL(serverProcessId);

        if (!GetNamedPipeServerProcessId(hPipe_, serverProcessId)) {
            core::lastError::Description Dsc;
            X_ERROR("Pipe", "Failed to get pipe pipe server process id. Err: %s", core::lastError::ToString(Dsc));
            return false;
        }

        return true;
    }

    bool Pipe::getServerSessionId(PULONG serverSessionId)
    {
        X_ASSERT(isOpen(), "Pipe must be open to get pipe server session id")
        (isOpen());
        X_ASSERT_NOT_NULL(serverSessionId);

        if (!GetNamedPipeServerSessionId(hPipe_, serverSessionId)) {
            core::lastError::Description Dsc;
            X_ERROR("Pipe", "Failed to get pipe pipe server session id. Err: %s", core::lastError::ToString(Dsc));
            return false;
        }

        return true;
    }

    bool Pipe::impersonateClient(void)
    {
        X_ASSERT(isOpen(), "Pipe must be open to impersonate")
        (isOpen());

        if (!ImpersonateNamedPipeClient(hPipe_)) {
            core::lastError::Description Dsc;
            X_ERROR("Pipe", "Failed to impersonate pipe client. Err: %s", core::lastError::ToString(Dsc));
            return false;
        }

        return true;
    }

} // namespace IPC

X_NAMESPACE_END