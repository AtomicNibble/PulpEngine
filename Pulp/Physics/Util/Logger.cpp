#include "stdafx.h"
#include "Logger.h"

X_NAMESPACE_BEGIN(physics)

void PhysxLogger::reportError(physx::PxErrorCode::Enum code, const char* message,
    const char* file, int line)
{
    switch (code) {
        case physx::PxErrorCode::eNO_ERROR:
            X_LOG0("Physx", "noErr: %s file: %s line: %i", message, file, line);
            break;

        case physx::PxErrorCode::eDEBUG_INFO:
            X_LOG0("Physx", "DbgInfo: %s file: %s line: %i", message, file, line);
            break;

        case physx::PxErrorCode::eDEBUG_WARNING:
            X_WARNING("Physx", "Debug: %s file: %s line: %i", message, file, line);
            break;

        case physx::PxErrorCode::eINVALID_PARAMETER:
            X_ERROR("Physx", "InvalidParam: %s file: %s line: %i", message, file, line);
            break;

        case physx::PxErrorCode::eINVALID_OPERATION:
            X_ERROR("Physx", "InvalidOp: %s file: %s line: %i", message, file, line);
            X_BREAKPOINT;
            break;

        case physx::PxErrorCode::eOUT_OF_MEMORY:
            X_ERROR("Physx", "OutOfMemmory: %s file: %s line: %i", message, file, line);
            break;

        case physx::PxErrorCode::eINTERNAL_ERROR:
            X_ERROR("Physx", "Err: %s file: %s line: %i", message, file, line);
            break;

        case physx::PxErrorCode::eABORT:
            X_FATAL("Physx", "Abort: %s file: %s line: %i", message, file, line);
            break;

        case physx::PxErrorCode::ePERF_WARNING:
            X_WARNING("Physx", "Performance: %s file: %s line: %i", message, file, line);
            break;

        default:
            X_ASSERT_UNREACHABLE();
            break;
    }
}

X_NAMESPACE_END