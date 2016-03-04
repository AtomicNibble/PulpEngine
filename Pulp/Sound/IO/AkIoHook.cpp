#include "stdafx.h"
#include "AkIoHook.h"


X_NAMESPACE_BEGIN(sound)

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


	return AK_Success;
}

// ~IAkFileLocationAware


// IAkIOHookDeferred


AKRESULT IOhook::Read(AkFileDesc& fileDesc, const AkIoHeuristics& heuristics,	
	AkAsyncIOTransferInfo& transferInfo)
{
	X_UNUSED(fileDesc);
	X_UNUSED(heuristics);
	X_UNUSED(transferInfo);

	return AK_Success;
}

// Writes data to a file (asynchronous).
AKRESULT IOhook::Write(AkFileDesc&fileDesc, const AkIoHeuristics& heuristics, 
	AkAsyncIOTransferInfo& io_transferInfo)
{
	X_UNUSED(fileDesc);
	X_UNUSED(heuristics);
	X_UNUSED(io_transferInfo);

	return AK_Success;
}

// Notifies that a transfer request is cancelled. It will be flushed by the streaming device when completed.
void IOhook::Cancel(AkFileDesc&	fileDesc, AkAsyncIOTransferInfo& transferInfo,
	bool& bCancelAllTransfersForThisFile)
{
	X_UNUSED(fileDesc);
	X_UNUSED(transferInfo);
	X_UNUSED(bCancelAllTransfersForThisFile);
}

// Cleans up a file.
AKRESULT IOhook::Close(AkFileDesc& fileDesc)
{
	X_UNUSED(fileDesc);

	return AK_Success;
}

// Returns the block size for the file or its storage device. 
AkUInt32 IOhook::GetBlockSize(AkFileDesc& fileDesc)
{
	X_UNUSED(fileDesc);

	return AK_Success;
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