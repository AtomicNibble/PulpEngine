#pragma once

#include <INetwork.h>

X_NAMESPACE_BEGIN(game)

struct UserNetMappings
{
    using PlayerGuidArr = std::array<net::NetGUID, net::MAX_PLAYERS>;
    using SystemHandleArr = std::array<net::SystemHandle, net::MAX_PLAYERS>;

public:
    UserNetMappings() :
        pSession(nullptr),
        localPlayerIdx(-1)
    {
    }

    void reset()
    {
        localPlayerIdx = -1;
        lobbyUserGuids.fill(net::NetGUID());
        sysHandles.fill(net::INVALID_SYSTEM_HANDLE);
    }

    void resetIndex(int32_t idx)
    {
        lobbyUserGuids[idx] = net::NetGUID();
        sysHandles[idx] = net::INVALID_SYSTEM_HANDLE;

        // should this ever happen?
        if (idx == localPlayerIdx) {
            X_ASSERT_NOT_IMPLEMENTED();
        }
    }

    void addUser(int32_t idx, net::UserInfo& info)
    {
        X_ASSERT(!lobbyUserGuids[idx].isValid(), "Slow taken")(idx);

        lobbyUserGuids[idx] = info.guid;
        sysHandles[idx] = info.systemHandle;

        if (info.guid == myGuid) {
            localPlayerIdx = idx;
        }
    }

    int32_t findFreeSlot(void) const
    {
        for (int32_t i = 0; i < static_cast<int32_t>(lobbyUserGuids.size()); i++) {
            if (!lobbyUserGuids[i].isValid()) {
                return i;
            }
        }

        return -1;
    }

    int32_t getNumUsers(void) const
    {
        return safe_static_cast<int32_t>(std::count_if(lobbyUserGuids.begin(), lobbyUserGuids.end(), [](const net::NetGUID& guid) -> bool {
            return guid.isValid();
        }));
    }

    int32_t getPlayerIdxForGuid(net::NetGUID guid) const
    {
        auto it = std::find(lobbyUserGuids.begin(), lobbyUserGuids.end(), guid);
        if (it != lobbyUserGuids.end()) {
            auto idx = std::distance(lobbyUserGuids.begin(), it);
            return safe_static_cast<int32_t>(idx);
        }

        return -1;
    }

    bool guidPresent(net::NetGUID guid) const
    {
        return std::find(lobbyUserGuids.begin(), lobbyUserGuids.end(), guid) != lobbyUserGuids.end();
    }

    net::NetGUID getLocalPlayerGUID(void) const
    {
        X_ASSERT(localPlayerIdx >= 0 && lobbyUserGuids[localPlayerIdx].isValid(), "Local player index is not valid")(localPlayerIdx);
        return lobbyUserGuids[localPlayerIdx];
    }

public:
    net::ISession* pSession; // TODO: feels a bit hacky.
    net::NetGUID myGuid;
    int32_t localPlayerIdx;
    PlayerGuidArr lobbyUserGuids;
    SystemHandleArr sysHandles;
};

X_NAMESPACE_END