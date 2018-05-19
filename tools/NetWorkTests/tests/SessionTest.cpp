#include "stdafx.h"
X_NAMESPACE_BEGIN(net)

namespace
{

    class Game : public IGameCallbacks
    {
    public:

        // IGameCallbacks
        void onUserCmdReceive(net::NetGUID guid, core::FixedBitStreamBase& bs) {
            X_UNUSED(guid, bs);
        }

        // ~IGameCallbacks


    private:

    };

} // namespace

TEST(net, Session)
{
    net::INet* pNet = gEnv->pNet;
    net::IPeer* pServer = pNet->createPeer();
    net::IPeer* pPeer = pNet->createPeer();

    Game server;
    Game client;

    net::SocketDescriptor sd(SERVER_PORT_BASE);
    net::SocketDescriptor sd2(SERVER_PORT_BASE + 1);

    auto res = pServer->init(16, sd);
    if (res != net::StartupResult::Started) {
        return;
    }

    pServer->setMaximumIncomingConnections(8);

    res = pPeer->init(1, sd2);
    if (res != net::StartupResult::Started) {
        return;
    }

    auto* pClientSes = pNet->createSession(pServer, &server);
    auto* pSeverSes = pNet->createSession(pPeer, &client);

    // what do i want to actually test.
    // 1) make sure a peer can connect disconnect and connect again
    // 2) check a peer can join a game in progress and get correct state.
    // 3) verify chat?
    // 4) check handling of dropped user cmd's / snap shots per peer.
    // 5) check various state behaviour, like failing to connect results in idle.
    // do the hookie pokie.


    pNet->deletePeer(pServer);
    pNet->deletePeer(pPeer);
}

X_NAMESPACE_END
