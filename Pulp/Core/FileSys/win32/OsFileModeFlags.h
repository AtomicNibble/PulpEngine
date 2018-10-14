#pragma once

#ifndef X_FILESYS_MODE_FLAGS_H_
#define X_FILESYS_MODE_FLAGS_H_

X_NAMESPACE_BEGIN(core)

namespace mode
{
    static DWORD GetSeekValue(IFileSys::SeekMode::Enum type)
    {
        switch (type) {
            case SeekMode::SET:
                return FILE_BEGIN;
            case SeekMode::CUR:
                return FILE_CURRENT;
            case SeekMode::END:
                return FILE_END;
            default:
                X_ASSERT_UNREACHABLE();
        }

        return FILE_BEGIN;
    }

    static DWORD GetAccess(IFileSys::FileFlags mode)
    {
        DWORD val = 0;

        if (mode.IsSet(FileFlag::WRITE)) {
            val |= FILE_WRITE_DATA;
        }
        if (mode.IsSet(FileFlag::READ)) {
            val |= FILE_READ_DATA;
        }
        if (mode.IsSet(FileFlag::APPEND)) {
            val |= FILE_APPEND_DATA;
        }

        X_ASSERT(val != 0, "File must have one of the following modes, READ, WRITE, APPEND")(mode.ToInt());
        return val;
    }

    static DWORD GetShareMode(IFileSys::FileFlags mode)
    {
        DWORD val = 0;

        if (mode.IsSet(FileFlag::SHARE)) {
            if (mode.IsSet(FileFlag::READ)) {
                val |= FILE_SHARE_READ;
            }
            if (mode.IsSet(FileFlag::WRITE)) {
                val |= FILE_SHARE_WRITE;
            }

            // would i even want delete, or should i always included?
            val |= FILE_SHARE_DELETE;
        }
        return val;
    }

    static DWORD GetCreationDispo(IFileSys::FileFlags mode)
    {
        if (mode.IsSet(FileFlag::RECREATE)) {
            return CREATE_ALWAYS;
        }
        if (mode.IsSet(FileFlag::READ)) {
            return OPEN_EXISTING;
        }
        if (mode.IsSet(FileFlag::APPEND)) {
            return OPEN_EXISTING;
        }
        if (mode.IsSet(FileFlag::WRITE)) {
            return CREATE_NEW;
        }

        X_ASSERT(false, "Creation mode unkown one of the following must be set: READ, WRITE, RECREATE, APPEND")(mode.ToInt());
        return 0;
    }

    static DWORD GetFlagsAndAtt(IFileSys::FileFlags mode, bool async)
    {
        DWORD flag = FILE_FLAG_SEQUENTIAL_SCAN;

        if (mode.IsSet(FileFlag::RANDOM_ACCESS)) {
            flag = FILE_FLAG_RANDOM_ACCESS;
        }
        if (mode.IsSet(FileFlag::WRITE_FLUSH)) {
            flag |= FILE_FLAG_WRITE_THROUGH;
        }
        if (mode.IsSet(FileFlag::NOBUFFER)) {
            flag |= FILE_FLAG_NO_BUFFERING;
        }
        if (async) { // dirty potato !
            flag |= FILE_FLAG_OVERLAPPED;
        }

        //	if (flag == FILE_FLAG_SEQUENTIAL_SCAN && mode.IsSet(FileFlag::WRITE))
        //		return FILE_ATTRIBUTE_NORMAL;

        return flag;
    }
} // namespace mode

X_NAMESPACE_END

#endif // X_FILESYS_MODE_FLAGS_H_