#pragma once

#include <INetwork.h>

X_NAMESPACE_BEGIN(game)


struct UserNetMappings
{
    using PlayerGuidArr = std::array<net::NetGUID, net::MAX_PLAYERS>;

public:
    UserNetMappings() :
        localPlayerIdx(-1)
    {
    }

    void reset() {
        localPlayerIdx = 0;
        lobbyUserGuids.fill(net::NetGUID());
    }

    int32_t getPlayerIdxForGuid(net::NetGUID guid) const {
        auto it = std::find(lobbyUserGuids.begin(), lobbyUserGuids.end(), guid);
        if (it != lobbyUserGuids.end()) {
            auto idx = std::distance(lobbyUserGuids.begin(), it);
            return safe_static_cast<int32_t>(idx);
        }

        return -1;
    }

    bool guidPresent(net::NetGUID guid) const {
        return std::find(lobbyUserGuids.begin(), lobbyUserGuids.end(), guid) != lobbyUserGuids.end();
    }

public:
    net::NetGUID myGuid;
    int32_t localPlayerIdx;
    PlayerGuidArr lobbyUserGuids;
};


X_NAMESPACE_END