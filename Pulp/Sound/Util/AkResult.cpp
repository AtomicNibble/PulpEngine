#include "stdafx.h"
#include "AkResult.h"


X_NAMESPACE_BEGIN(sound)


namespace AkResult
{
	const char* ToString(AKRESULT res, Description& desc)
	{
		const char* pResDesc = "Err(%i) <ukn>";

		switch (res)
		{
			case AK_NotImplemented: pResDesc = "This feature is not implemented.";
			case AK_Success: pResDesc = "The operation was successful.";
			case AK_Fail: pResDesc = "The operation failed.";
			case AK_PartialSuccess: pResDesc = "The operation succeeded partially.";
			case AK_NotCompatible: pResDesc = "Incompatible formats";
			case AK_AlreadyConnected: pResDesc = "The stream is already connected to another node.";
			case AK_NameNotSet: pResDesc = "Trying to open a file when its name was not set";
			case AK_InvalidFile: pResDesc = "An unexpected value causes the file to be invalid.";
			case AK_AudioFileHeaderTooLarge: pResDesc = "The file header is too large.";
			case AK_MaxReached: pResDesc = "The maximum was reached.";
			case AK_InputsInUsed: pResDesc = "Inputs are currently used.";
			case AK_OutputsInUsed: pResDesc = "Outputs are currently used.";
			case AK_InvalidName: pResDesc = "The name is invalid.";
			case AK_NameAlreadyInUse: pResDesc = "The name is already in use.";
			case AK_InvalidID: pResDesc = "The ID is invalid.";
			case AK_IDNotFound: pResDesc = "The ID was not found.";
			case AK_InvalidInstanceID: pResDesc = "The InstanceID is invalid.";
			case AK_NoMoreData: pResDesc = "No more data is available from the source.";
			case AK_NoSourceAvailable: pResDesc = "There is no child (source) associated with the node.";
			case AK_StateGroupAlreadyExists: pResDesc = "The StateGroup already exists.";
			case AK_InvalidStateGroup: pResDesc = "The StateGroup is not a valid channel.";
			case AK_ChildAlreadyHasAParent: pResDesc = "The child already has a parent.";
			case AK_InvalidLanguage: pResDesc = "The language is invalid (applies to the Low-Level I/O).";
			case AK_CannotAddItseflAsAChild: pResDesc = "It is not possible to add itself as its own child.";
			case AK_UserNotInList: pResDesc = "This user is not there.";
			case AK_NoTransitionPoint: pResDesc = "Not in use.";
			case AK_InvalidParameter: pResDesc = "Something is not within bounds.";
			case AK_ParameterAdjusted: pResDesc = "Something was not within bounds and was relocated to the nearest OK value.";
			case AK_IsA3DSound: pResDesc = "The sound has 3D parameters.";
			case AK_NotA3DSound: pResDesc = "The sound does not have 3D parameters.";
			case AK_ElementAlreadyInList: pResDesc = "The item could not be added because it was already in the list.";
			case AK_PathNotFound: pResDesc = "This path is not known.";
			case AK_PathNoVertices: pResDesc = "Stuff in vertices before trying to start it";
			case AK_PathNotRunning: pResDesc = "Only a running path can be paused.";
			case AK_PathNotPaused: pResDesc = "Only a paused path can be resumed.";
			case AK_PathNodeAlreadyInList: pResDesc = "This path is already there.";
			case AK_PathNodeNotInList: pResDesc = "This path is not there.";
			case AK_VoiceNotFound: pResDesc = "Unknown in our voices list";
			case AK_DataNeeded: pResDesc = "The consumer needs more.";
			case AK_NoDataNeeded: pResDesc = "The consumer does not need more.";
			case AK_DataReady: pResDesc = "The provider has available data.";
			case AK_NoDataReady: pResDesc = "The provider does not have available data.";
			case AK_NoMoreSlotAvailable: pResDesc = "Not enough space to load bank.";
			case AK_SlotNotFound: pResDesc = "Bank error.";
			case AK_ProcessingOnly: pResDesc = "No need to fetch new data.";
			case AK_MemoryLeak: pResDesc = "Debug mode only.";
			case AK_CorruptedBlockList: pResDesc = "The memory manager's block list has been corrupted.";
			case AK_InsufficientMemory: pResDesc = "Memory error.";
			case AK_Cancelled: pResDesc = "The requested action was cancelled (not an error).";
			case AK_UnknownBankID: pResDesc = "Trying to load a bank using an ID which is not defined.";
			case AK_IsProcessing: pResDesc = "Asynchronous pipeline component is processing.";
			case AK_BankReadError: pResDesc = "Error while reading a bank.";
			case AK_InvalidSwitchType: pResDesc = "Invalid switch type (used with the switch container)";
			case AK_VoiceDone: pResDesc = "Internal use only.";
			case AK_UnknownEnvironment: pResDesc = "This environment is not defined.";
			case AK_EnvironmentInUse: pResDesc = "This environment is used by an object.";
			case AK_UnknownObject: pResDesc = "This object is not defined.";
			case AK_NoConversionNeeded: pResDesc = "Audio data already in target format, no conversion to perform.";
			case AK_FormatNotReady: pResDesc = "Source format not known yet.";
			case AK_WrongBankVersion: pResDesc = "The bank version is not compatible with the current bank reader.";
			case AK_DataReadyNoProcess: pResDesc = "The provider has some data but does not process it (virtual voices).";
			case AK_FileNotFound: pResDesc = "File not found.";
			case AK_DeviceNotReady: pResDesc = "IO device not ready (may be because the tray is open)";
			case AK_CouldNotCreateSecBuffer: pResDesc = "The direct sound secondary buffer creation failed.";
			case AK_BankAlreadyLoaded: pResDesc = "The bank load failed because the bank is already loaded.";
			case AK_RenderedFX: pResDesc = "The effect on the node is rendered.";
			case AK_ProcessNeeded: pResDesc = "A routine needs to be executed on some CPU.";
			case AK_ProcessDone: pResDesc = "The executed routine has finished its execution.";
			case AK_MemManagerNotInitialized: pResDesc = "The memory manager should have been initialized at this point.";
			case AK_StreamMgrNotInitialized: pResDesc = "The stream manager should have been initialized at this point.";
			case AK_SSEInstructionsNotSupported: pResDesc = "The machine does not support SSE instructions (required on PC).";
			case AK_Busy: pResDesc = "The system is busy and could not process the request.";
			case AK_UnsupportedChannelConfig: pResDesc = "Channel configuration is not supported in the current execution context.";
			case AK_PluginMediaNotAvailable: pResDesc = "Plugin media is not available for effect.";
			case AK_MustBeVirtualized: pResDesc = "Sound was Not Allowed to play.";
			case AK_CommandTooLarge: pResDesc = "SDK command is too large to fit in the command queue.";
			case AK_RejectedByFilter: pResDesc = "A play request was rejected due to the MIDI filter parameters.";
			case AK_InvalidCustomPlatformName: pResDesc = "Detecting incompatibility between Custom platform of banks and custom platform of connected application";
			default:
				break;
		}

		desc.setFmt("Err(%i): %s", res, pResDesc);
		return desc.c_str();
	}
}

X_NAMESPACE_END