#pragma once


#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

X_NAMESPACE_DECLARE(core,
struct XFileAsync;
struct IoRequestBase;
)

X_NAMESPACE_BEGIN(sound)

class IOhook : public AK::StreamMgr::IAkFileLocationResolver,
	public AK::StreamMgr::IAkIOHookDeferred
{
	static const wchar_t* DEVICE_NAME;

public:
	IOhook();
	~IOhook() X_OVERRIDE;

	AKRESULT Init(const AkDeviceSettings& deviceSettings, bool AsyncOpen = false);
	void Term(void);

	// IAkFileLocationAware

	// Returns a file descriptor for a given file name (string).
	AKRESULT Open(const AkOSChar* pszFileName, AkOpenMode eOpenMode,		
		AkFileSystemFlags* pFlags, bool& SyncOpen,	AkFileDesc&	outFileDesc) X_OVERRIDE;

	// Returns a file descriptor for a given file ID.
	AKRESULT Open(AkFileID fileID, AkOpenMode eOpenMode,    
		AkFileSystemFlags* pFlags, bool& SyncOpen, AkFileDesc&	outFileDesc) X_OVERRIDE;

	// ~IAkFileLocationAware


	// IAkIOHookDeferred

	/// Reads data from a file (asynchronous).
	AKRESULT Read(AkFileDesc& fileDesc, const AkIoHeuristics& heuristics,	// Heuristics for this data transfer.
		AkAsyncIOTransferInfo& transferInfo	// Asynchronous data transfer info.
		) X_OVERRIDE;

	// Writes data to a file (asynchronous).
	AKRESULT Write(AkFileDesc&fileDesc, const AkIoHeuristics& heuristics, // Heuristics for this data transfer.
		AkAsyncIOTransferInfo& io_transferInfo	// Platform-specific asynchronous IO operation info.
		) X_OVERRIDE;

	// Notifies that a transfer request is cancelled. It will be flushed by the streaming device when completed.
	void Cancel(AkFileDesc&	fileDesc, AkAsyncIOTransferInfo& transferInfo,	// Transfer info to cancel.
		bool& io_bCancelAllTransfersForThisFile	// Flag indicating whether all transfers should be cancelled for this file (see notes in function description).
		) X_OVERRIDE;

	// Cleans up a file.
	AKRESULT Close(AkFileDesc& fileDesc) X_OVERRIDE;
	// Returns the block size for the file or its storage device. 
	AkUInt32 GetBlockSize(AkFileDesc& fileDesc) X_OVERRIDE;
	// Returns a description for the streaming device above this low-level hook.
	void GetDeviceDesc(AkDeviceDesc& outDeviceDesc) X_OVERRIDE;
	// Returns custom profiling data: 1 if file opens are asynchronous, 0 otherwise.
	AkUInt32 GetDeviceData(void) X_OVERRIDE;

	// ~IAkIOHookDeferred
private:
	void IoRequestCallback(core::IFileSys& pFileSys, const core::IoRequestBase* pRequest,
		core::XFileAsync* pFile, uint32_t bytesTransferred);

private:
	core::IFileSys* pFileSys_;

	AkDeviceID deviceID_;
	bool asyncOpen_;

	AkUInt32 minSectorSize_;
};


X_NAMESPACE_END