#include "stdafx.h"
#include "AkIoHook.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(sound)

namespace
{
	core::fileModeFlags AKModeToEngineMode(AkOpenMode openMode)
	{
		core::fileModeFlags mode;

		switch (openMode)
		{
		case AK_OpenModeRead:
			mode.Set(core::fileMode::READ);
			mode.Set(core::fileMode::SHARE);
			mode.Set(core::fileMode::RANDOM_ACCESS);
			break;
		case AK_OpenModeWrite:
			mode.Set(core::fileMode::WRITE);
			mode.Set(core::fileMode::SHARE);
			break;
		case AK_OpenModeWriteOvrwr:
			mode.Set(core::fileMode::WRITE);
			mode.Set(core::fileMode::RECREATE);
			break;
		case AK_OpenModeReadWrite:
			mode.Set(core::fileMode::READ);
			mode.Set(core::fileMode::WRITE);
			break;
		default:
			X_ASSERT_UNREACHABLE();
			return core::fileModeFlags();
		}

		return mode;
	}


} // namespace

const wchar_t* IOhook::DEVICE_NAME = L"Engine VFS wrap";

IOhook::IOhook() :
	pFileSys_(nullptr),
	deviceID_(AK_INVALID_DEVICE_ID),
	asyncOpen_(false)
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
	X_UNUSED(pszFileName);
	X_UNUSED(eOpenMode);
	X_UNUSED(pFlags);
	X_UNUSED(SyncOpen);
	X_UNUSED(outFileDesc);

	if (SyncOpen || !asyncOpen_)
	{
		// sync open
		SyncOpen = true;

		core::fileModeFlags mode = AKModeToEngineMode(eOpenMode);
		if (!mode.IsAnySet()) {
			return AK_InvalidParameter;
		}

		core::XFileAsync* pFile = pFileSys_->openFileAsync(pszFileName, mode);
		if (!pFile) {
			return AK_Fail;
		}

		outFileDesc.hFile = pFile;
		outFileDesc.iFileSize = pFile->remainingBytes();
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
	X_UNUSED(fileID);
	X_UNUSED(eOpenMode);
	X_UNUSED(pFlags);
	X_UNUSED(SyncOpen);
	X_UNUSED(outFileDesc);

	if (SyncOpen || !asyncOpen_)
	{
		// sync open
		SyncOpen = true;

		X_ASSERT_NOT_IMPLEMENTED();


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


AKRESULT IOhook::Read(AkFileDesc& fileDesc, const AkIoHeuristics& heuristics,	
	AkAsyncIOTransferInfo& transferInfo)
{
	X_UNUSED(heuristics);
	X_ASSERT_NOT_NULL(fileDesc.hFile);
	X_ASSERT(transferInfo.uRequestedSize > 0, "")(transferInfo.uRequestedSize);
	X_ASSERT(transferInfo.uBufferSize >= transferInfo.uRequestedSize,"")(transferInfo.uBufferSize, transferInfo.uRequestedSize);

	core::XFileAsync* pFile = reinterpret_cast<core::XFileAsync*>(fileDesc.hFile);

	core::XFileAsyncOperation op = pFile->readAsync(transferInfo.pBuffer,
		transferInfo.uRequestedSize,
		transferInfo.uFilePosition);
	
//	transferInfo.pUserData = op;
	X_ASSERT_NOT_IMPLEMENTED();

	return AK_Success;
}

// Writes data to a file (asynchronous).
AKRESULT IOhook::Write(AkFileDesc&fileDesc, const AkIoHeuristics& heuristics, 
	AkAsyncIOTransferInfo& transferInfo)
{
	X_UNUSED(heuristics);
	X_ASSERT_NOT_NULL(fileDesc.hFile);
	X_ASSERT(transferInfo.uRequestedSize > 0, "")(transferInfo.uRequestedSize);
	X_ASSERT(transferInfo.uBufferSize >= transferInfo.uRequestedSize, "")(transferInfo.uBufferSize, transferInfo.uRequestedSize);

	core::XFileAsync* pFile = reinterpret_cast<core::XFileAsync*>(fileDesc.hFile);

	core::XFileAsyncOperation op = pFile->writeAsync(transferInfo.pBuffer,
		transferInfo.uRequestedSize,
		transferInfo.uFilePosition);

	//	transferInfo.pUserData = op;
	X_ASSERT_NOT_IMPLEMENTED();

	return AK_Success;
}

// Notifies that a transfer request is cancelled. It will be flushed by the streaming device when completed.
void IOhook::Cancel(AkFileDesc&	fileDesc, AkAsyncIOTransferInfo& transferInfo,
	bool& bCancelAllTransfersForThisFile)
{
	X_UNUSED(transferInfo);
	X_ASSERT_NOT_NULL(fileDesc.hFile);
//	core::XFileAsync* pFile = reinterpret_cast<core::XFileAsync*>(fileDesc.hFile);

	X_ASSERT_NOT_IMPLEMENTED();

	bCancelAllTransfersForThisFile = false;
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
		return safe_static_cast<AkUInt32,size_t>(pFileSys_->getMinimumSectorSize());
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