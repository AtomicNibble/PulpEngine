#include "stdafx.h"
#include "AkIoHook.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(sound)

namespace
{
    core::FileFlags AKModeToEngineMode(AkOpenMode openMode)
    {
        core::FileFlags mode;

        switch (openMode) {
            case AK_OpenModeRead:
                mode.Set(core::FileFlag::READ);
                mode.Set(core::FileFlag::SHARE);
                mode.Set(core::FileFlag::RANDOM_ACCESS);
                break;
            case AK_OpenModeWrite:
                mode.Set(core::FileFlag::WRITE);
                mode.Set(core::FileFlag::SHARE);
                break;
            case AK_OpenModeWriteOvrwr:
                mode.Set(core::FileFlag::WRITE);
                mode.Set(core::FileFlag::RECREATE);
                break;
            case AK_OpenModeReadWrite:
                mode.Set(core::FileFlag::READ);
                mode.Set(core::FileFlag::WRITE);
                break;
            default:
                X_ASSERT_UNREACHABLE();
                return core::FileFlags();
        }

        return mode;
    }

} // namespace

const wchar_t* IOhook::DEVICE_NAME = L"Engine VFS wrap";

IOhook::IOhook() :
    pFileSys_(nullptr),
    deviceID_(AK_INVALID_DEVICE_ID),
    asyncOpen_(false),
    minSectorSize_(4096)
{
}

IOhook::~IOhook()
{
}

AKRESULT IOhook::Init(const AkDeviceSettings& deviceSettings, bool AsyncOpen)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pFileSys);

    pFileSys_ = gEnv->pFileSys;

    if (deviceSettings.uSchedulerTypeFlags != AK_SCHEDULER_DEFERRED_LINED_UP) {
        X_ERROR("SoundIO", "I/O hook only works with AK_SCHEDULER_DEFERRED_LINED_UP devices");
        return AK_Fail;
    }

    asyncOpen_ = AsyncOpen;

    if (!AK::StreamMgr::GetFileLocationResolver()) {
        AK::StreamMgr::SetFileLocationResolver(this);
    }

    // Create a device in the Stream Manager, specifying this as the hook.
    deviceID_ = AK::StreamMgr::CreateDevice(deviceSettings, this);
    if (deviceID_ == AK_INVALID_DEVICE_ID) {
        X_ERROR("SoundIO", "Failed to create device");
        return AK_Fail;
    }

    minSectorSize_ = safe_static_cast<AkUInt32>(pFileSys_->getMinimumSectorSize());

    return AK_Success;
}

void IOhook::Term(void)
{
    X_LOG2("SoundIO", "Terminating IO hook");

    if (AK::StreamMgr::GetFileLocationResolver() == this) {
        AK::StreamMgr::SetFileLocationResolver(nullptr);
    }

    if (deviceID_ != AK_INVALID_DEVICE_ID) {
        AK::StreamMgr::DestroyDevice(deviceID_);
    }

    pFileSys_ = nullptr;
}

// IAkFileLocationAware

// Returns a file descriptor for a given file name (string).
AKRESULT IOhook::Open(const AkOSChar* pszFileName, AkOpenMode eOpenMode,
    AkFileSystemFlags* pFlags, bool& SyncOpen, AkFileDesc& outFileDesc)
{
    X_UNUSED(pFlags);

    if (SyncOpen || !asyncOpen_) {
        // sync open
        SyncOpen = true;

        core::FileFlags mode = AKModeToEngineMode(eOpenMode);
        if (!mode.IsAnySet()) {
            return AK_InvalidParameter;
        }

        core::Path<AkOSChar> path;
        path.append(L"sound/");
        path.append(pszFileName);

        core::XFileAsync* pFile = pFileSys_->openFileAsync(path.c_str(), mode);
        if (!pFile) {
            return AK_Fail;
        }

        outFileDesc.hFile = pFile;
        outFileDesc.iFileSize = pFile->fileSize();
        outFileDesc.uSector = 0;
        outFileDesc.deviceID = deviceID_;
        outFileDesc.pCustomParam = (eOpenMode == AK_OpenModeRead) ? nullptr : (void*)1;
        outFileDesc.uCustomParamSize = 0;
        return AK_Success;
    }

    // async open.
    // We only need to specify the deviceID, and leave the boolean to false.
    outFileDesc.iFileSize = 0;
    outFileDesc.uSector = 0;
    outFileDesc.deviceID = deviceID_;
    outFileDesc.pCustomParam = nullptr;
    outFileDesc.uCustomParamSize = 0;
    return AK_Success;
}

// Returns a file descriptor for a given file ID.
AKRESULT IOhook::Open(AkFileID fileID, AkOpenMode eOpenMode,
    AkFileSystemFlags* pFlags, bool& SyncOpen, AkFileDesc& outFileDesc)
{
    X_UNUSED(pFlags);

    if (SyncOpen || !asyncOpen_) {
        // sync open
        SyncOpen = true;
        core::FileFlags mode = AKModeToEngineMode(eOpenMode);
        if (!mode.IsAnySet()) {
            return AK_InvalidParameter;
        }

        core::Path<AkOSChar> path;
        path.appendFmt(L"sound/%" PRIu32 L".wem", fileID);

        core::XFileAsync* pFile = pFileSys_->openFileAsync(path.c_str(), mode);
        if (!pFile) {
            return AK_Fail;
        }

        outFileDesc.hFile = pFile;
        outFileDesc.iFileSize = pFile->fileSize();
        outFileDesc.uSector = 0;
        outFileDesc.deviceID = deviceID_;
        outFileDesc.pCustomParam = (eOpenMode == AK_OpenModeRead) ? nullptr : (void*)1;
        outFileDesc.uCustomParamSize = 0;
        return AK_Success;
    }

    // async open.
    // We only need to specify the deviceID, and leave the boolean to false.
    outFileDesc.iFileSize = 0;
    outFileDesc.uSector = 0;
    outFileDesc.deviceID = deviceID_;
    outFileDesc.pCustomParam = nullptr;
    outFileDesc.uCustomParamSize = 0;
    return AK_Success;
}

// ~IAkFileLocationAware

// IAkIOHookDeferred

void IOhook::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
    core::XFileAsync* pFile, uint32_t bytesTransferred)
{
    X_UNUSED(fileSys);
    X_UNUSED(pFile);

    X_ASSERT(pRequest->getType() == core::IoRequest::READ || pRequest->getType() == core::IoRequest::WRITE, "Invalid request type")(pRequest->getType());

    const core::IoRequestRead* pReq = static_cast<const core::IoRequestRead*>(pRequest);
    AkAsyncIOTransferInfo* transferInfo = reinterpret_cast<AkAsyncIOTransferInfo*>(pReq->pUserData);

    AKRESULT eResult = AK_Fail;

    if (pReq->dataSize == bytesTransferred) {
        eResult = AK_Success;
    }

    transferInfo->pCallback(transferInfo, eResult);
}

AKRESULT IOhook::Read(AkFileDesc& fileDesc, const AkIoHeuristics& heuristics,
    AkAsyncIOTransferInfo& transferInfo)
{
    X_UNUSED(heuristics);
    X_ASSERT_NOT_NULL(fileDesc.hFile);
    X_ASSERT(transferInfo.uRequestedSize > 0, "")(transferInfo.uRequestedSize);
    X_ASSERT(transferInfo.uBufferSize >= transferInfo.uRequestedSize, "")(transferInfo.uBufferSize, transferInfo.uRequestedSize);

    core::XFileAsync* pFile = reinterpret_cast<core::XFileAsync*>(fileDesc.hFile);

    core::IoRequestRead read;
    read.callback.Bind<IOhook, &IOhook::IoRequestCallback>(this);
    read.dataSize = transferInfo.uRequestedSize;
    read.offset = transferInfo.uFilePosition;
    read.pBuf = transferInfo.pBuffer;
    read.pFile = pFile;
    read.pUserData = &transferInfo;

    pFileSys_->AddIoRequestToQue(read);

    return AK_Success;
}

// Writes data to a file (asynchronous).
AKRESULT IOhook::Write(AkFileDesc& fileDesc, const AkIoHeuristics& heuristics,
    AkAsyncIOTransferInfo& transferInfo)
{
    X_UNUSED(heuristics);
    X_ASSERT_NOT_NULL(fileDesc.hFile);
    X_ASSERT(transferInfo.uRequestedSize > 0, "")(transferInfo.uRequestedSize);
    X_ASSERT(transferInfo.uBufferSize >= transferInfo.uRequestedSize, "")(transferInfo.uBufferSize, transferInfo.uRequestedSize);

    core::XFileAsync* pFile = reinterpret_cast<core::XFileAsync*>(fileDesc.hFile);

    core::IoRequestWrite write;
    write.callback.Bind<IOhook, &IOhook::IoRequestCallback>(this);
    write.dataSize = transferInfo.uRequestedSize;
    write.offset = transferInfo.uFilePosition;
    write.pBuf = transferInfo.pBuffer;
    write.pFile = pFile;
    write.pUserData = &transferInfo;

    auto reqHandle = pFileSys_->AddIoRequestToQue(write);

    static_assert(sizeof(reqHandle) <= sizeof(void*), "Can't fit handle in user data");
    transferInfo.pUserData = reinterpret_cast<void*>(safe_static_cast<uintptr_t>(reqHandle));
    return AK_Success;
}

// Notifies that a transfer request is cancelled. It will be flushed by the streaming device when completed.
void IOhook::Cancel(AkFileDesc& fileDesc, AkAsyncIOTransferInfo& transferInfo,
    bool& bCancelAllTransfersForThisFile)
{
    X_UNUSED(transferInfo);
    X_ASSERT_NOT_NULL(fileDesc.hFile);
    core::XFileAsync* pFile = reinterpret_cast<core::XFileAsync*>(fileDesc.hFile);

    if (bCancelAllTransfersForThisFile) {
        pFile->cancelAll();
    }
    else {
        X_ASSERT_NOT_IMPLEMENTED();

        // cancel just a single request.
    }
}

// Cleans up a file.
AKRESULT IOhook::Close(AkFileDesc& fileDesc)
{
    X_ASSERT_NOT_NULL(fileDesc.hFile);
    core::XFileAsync* pFile = reinterpret_cast<core::XFileAsync*>(fileDesc.hFile);

    pFileSys_->closeFileAsync(pFile);
    return AK_Success;
}

// Returns the block size for the file or its storage device.
AkUInt32 IOhook::GetBlockSize(AkFileDesc& fileDesc)
{
    if (fileDesc.pCustomParam == 0) {
        return minSectorSize_;
    }
    return 1;
}

// Returns a description for the streaming device above this low-level hook.
void IOhook::GetDeviceDesc(AkDeviceDesc& outDeviceDesc)
{
    X_ASSERT(deviceID_ != AK_INVALID_DEVICE_ID, "Low-Level device was not initialized")(deviceID_);

    // Deferred scheduler.
    outDeviceDesc.deviceID = deviceID_;
    outDeviceDesc.bCanRead = true;
    outDeviceDesc.bCanWrite = true;
    AKPLATFORM::SafeStrCpy(outDeviceDesc.szDeviceName, DEVICE_NAME, AK_MONITOR_DEVICENAME_MAXLENGTH);
    outDeviceDesc.uStringSize = (AkUInt32)wcslen(outDeviceDesc.szDeviceName) + 1;
}

// Returns custom profiling data: 1 if file opens are asynchronous, 0 otherwise.
AkUInt32 IOhook::GetDeviceData(void)
{
    return asyncOpen_ ? 1 : 0;
}

X_NAMESPACE_END