#include "stdafx.h"
#include "AkResult.h"

X_NAMESPACE_BEGIN(sound)

namespace AkResult
{
    const char* ToString(AKRESULT res, Description& desc)
    {
        const char* pResDesc = "Err(%i) <ukn>";

        switch (res) {
            case AK_NotImplemented:
                pResDesc = "This feature is not implemented.";
                break;
            case AK_Success:
                pResDesc = "The operation was successful.";
                break;
            case AK_Fail:
                pResDesc = "The operation failed.";
                break;
            case AK_PartialSuccess:
                pResDesc = "The operation succeeded partially.";
                break;
            case AK_NotCompatible:
                pResDesc = "Incompatible formats";
                break;
            case AK_AlreadyConnected:
                pResDesc = "The stream is already connected to another node.";
                break;
            case AK_NameNotSet:
                pResDesc = "Trying to open a file when its name was not set";
                break;
            case AK_InvalidFile:
                pResDesc = "An unexpected value causes the file to be invalid.";
                break;
            case AK_AudioFileHeaderTooLarge:
                pResDesc = "The file header is too large.";
                break;
            case AK_MaxReached:
                pResDesc = "The maximum was reached.";
                break;
            case AK_InputsInUsed:
                pResDesc = "Inputs are currently used.";
                break;
            case AK_OutputsInUsed:
                pResDesc = "Outputs are currently used.";
                break;
            case AK_InvalidName:
                pResDesc = "The name is invalid.";
                break;
            case AK_NameAlreadyInUse:
                pResDesc = "The name is already in use.";
                break;
            case AK_InvalidID:
                pResDesc = "The ID is invalid.";
                break;
            case AK_IDNotFound:
                pResDesc = "The ID was not found.";
                break;
            case AK_InvalidInstanceID:
                pResDesc = "The InstanceID is invalid.";
                break;
            case AK_NoMoreData:
                pResDesc = "No more data is available from the source.";
                break;
            case AK_NoSourceAvailable:
                pResDesc = "There is no child (source) associated with the node.";
                break;
            case AK_StateGroupAlreadyExists:
                pResDesc = "The StateGroup already exists.";
                break;
            case AK_InvalidStateGroup:
                pResDesc = "The StateGroup is not a valid channel.";
                break;
            case AK_ChildAlreadyHasAParent:
                pResDesc = "The child already has a parent.";
                break;
            case AK_InvalidLanguage:
                pResDesc = "The language is invalid (applies to the Low-Level I/O).";
                break;
            case AK_CannotAddItseflAsAChild:
                pResDesc = "It is not possible to add itself as its own child.";
                break;
            case AK_UserNotInList:
                pResDesc = "This user is not there.";
                break;
            case AK_NoTransitionPoint:
                pResDesc = "Not in use.";
                break;
            case AK_InvalidParameter:
                pResDesc = "Something is not within bounds.";
                break;
            case AK_ParameterAdjusted:
                pResDesc = "Something was not within bounds and was relocated to the nearest OK value.";
                break;
            case AK_IsA3DSound:
                pResDesc = "The sound has 3D parameters.";
                break;
            case AK_NotA3DSound:
                pResDesc = "The sound does not have 3D parameters.";
                break;
            case AK_ElementAlreadyInList:
                pResDesc = "The item could not be added because it was already in the list.";
                break;
            case AK_PathNotFound:
                pResDesc = "This path is not known.";
                break;
            case AK_PathNoVertices:
                pResDesc = "Stuff in vertices before trying to start it";
                break;
            case AK_PathNotRunning:
                pResDesc = "Only a running path can be paused.";
                break;
            case AK_PathNotPaused:
                pResDesc = "Only a paused path can be resumed.";
                break;
            case AK_PathNodeAlreadyInList:
                pResDesc = "This path is already there.";
                break;
            case AK_PathNodeNotInList:
                pResDesc = "This path is not there.";
                break;
            case AK_VoiceNotFound:
                pResDesc = "Unknown in our voices list";
                break;
            case AK_DataNeeded:
                pResDesc = "The consumer needs more.";
                break;
            case AK_NoDataNeeded:
                pResDesc = "The consumer does not need more.";
                break;
            case AK_DataReady:
                pResDesc = "The provider has available data.";
                break;
            case AK_NoDataReady:
                pResDesc = "The provider does not have available data.";
                break;
            case AK_NoMoreSlotAvailable:
                pResDesc = "Not enough space to load bank.";
                break;
            case AK_SlotNotFound:
                pResDesc = "Bank error.";
                break;
            case AK_ProcessingOnly:
                pResDesc = "No need to fetch new data.";
                break;
            case AK_MemoryLeak:
                pResDesc = "Debug mode only.";
                break;
            case AK_CorruptedBlockList:
                pResDesc = "The memory manager's block list has been corrupted.";
                break;
            case AK_InsufficientMemory:
                pResDesc = "Memory error.";
                break;
            case AK_Cancelled:
                pResDesc = "The requested action was cancelled (not an error).";
                break;
            case AK_UnknownBankID:
                pResDesc = "Trying to load a bank using an ID which is not defined.";
                break;
            case AK_IsProcessing:
                pResDesc = "Asynchronous pipeline component is processing.";
                break;
            case AK_BankReadError:
                pResDesc = "Error while reading a bank.";
                break;
            case AK_InvalidSwitchType:
                pResDesc = "Invalid switch type (used with the switch container)";
                break;
            case AK_VoiceDone:
                pResDesc = "Internal use only.";
                break;
            case AK_UnknownEnvironment:
                pResDesc = "This environment is not defined.";
                break;
            case AK_EnvironmentInUse:
                pResDesc = "This environment is used by an object.";
                break;
            case AK_UnknownObject:
                pResDesc = "This object is not defined.";
                break;
            case AK_NoConversionNeeded:
                pResDesc = "Audio data already in target format, no conversion to perform.";
                break;
            case AK_FormatNotReady:
                pResDesc = "Source format not known yet.";
                break;
            case AK_WrongBankVersion:
                pResDesc = "The bank version is not compatible with the current bank reader.";
                break;
            case AK_DataReadyNoProcess:
                pResDesc = "The provider has some data but does not process it (virtual voices).";
                break;
            case AK_FileNotFound:
                pResDesc = "File not found.";
                break;
            case AK_DeviceNotReady:
                pResDesc = "IO device not ready (may be because the tray is open)";
                break;
            case AK_CouldNotCreateSecBuffer:
                pResDesc = "The direct sound secondary buffer creation failed.";
                break;
            case AK_BankAlreadyLoaded:
                pResDesc = "The bank load failed because the bank is already loaded.";
                break;
            case AK_RenderedFX:
                pResDesc = "The effect on the node is rendered.";
                break;
            case AK_ProcessNeeded:
                pResDesc = "A routine needs to be executed on some CPU.";
                break;
            case AK_ProcessDone:
                pResDesc = "The executed routine has finished its execution.";
                break;
            case AK_MemManagerNotInitialized:
                pResDesc = "The memory manager should have been initialized at this point.";
                break;
            case AK_StreamMgrNotInitialized:
                pResDesc = "The stream manager should have been initialized at this point.";
                break;
            case AK_SSEInstructionsNotSupported:
                pResDesc = "The machine does not support SSE instructions (required on PC).";
                break;
            case AK_Busy:
                pResDesc = "The system is busy and could not process the request.";
                break;
            case AK_UnsupportedChannelConfig:
                pResDesc = "Channel configuration is not supported in the current execution context.";
                break;
            case AK_PluginMediaNotAvailable:
                pResDesc = "Plugin media is not available for effect.";
                break;
            case AK_MustBeVirtualized:
                pResDesc = "Sound was Not Allowed to play.";
                break;
            case AK_CommandTooLarge:
                pResDesc = "SDK command is too large to fit in the command queue.";
                break;
            case AK_RejectedByFilter:
                pResDesc = "A play request was rejected due to the MIDI filter parameters.";
                break;
            case AK_InvalidCustomPlatformName:
                pResDesc = "Detecting incompatibility between Custom platform of banks and custom platform of connected application";
                break;
            default:
                break;
        }

        desc.setFmt("Err(%i): %s", res, pResDesc);
        return desc.c_str();
    }
} // namespace AkResult

X_NAMESPACE_END