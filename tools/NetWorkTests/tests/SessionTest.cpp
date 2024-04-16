#include "stdafx.h"

#include <SystemAddress.h>
#include <SnapShot.h>

#include <IConsole.h>

X_NAMESPACE_BEGIN(net)

using namespace core::string_view_literals;

namespace
{

    class Game : public IGameCallbacks
    {
    public:

        // IGameCallbacks
        void onUserCmdReceive(net::NetGUID guid, core::FixedBitStreamBase& bs) X_FINAL {
            X_UNUSED(guid, bs);
        }

        void applySnapShot(const net::SnapShot& snap) X_FINAL {
            X_UNUSED(snap);
        }

        void buildSnapShot(net::SnapShot& snap) X_FINAL {
            X_UNUSED(snap);
        }


        void setInterpolation(float fraction, int32_t serverGameTimeMS, int32_t ssStartTimeMS, int32_t ssEndTimeMS) X_FINAL {
            X_UNUSED(fraction, serverGameTimeMS, ssStartTimeMS, ssEndTimeMS);
        }

        bool handlePacket(net::Packet* pPacket) X_FINAL {
            X_UNUSED(pPacket);
            return false;
        }

        // ~IGameCallbacks


    private:

    };


    struct TestMatchParameters : MatchParameters
    {
        TestMatchParameters() {
            mapName.set("test");
        }

    };

} // namespace


class SessionTest : public ::testing::Test
{
protected:
    virtual void SetUp() X_OVERRIDE
    {
        pNet_ = gEnv->pNet;

        core::ICVar* pTimeout = gEnv->pConsole->getCVar("net_default_timeout"_sv);
        defaultTimeout_ = core::TimeVal::fromMS(pTimeout->GetInteger());

        core::ICVar* pWait = gEnv->pConsole->getCVar("net_wait_for_players"_sv);
        waitForPlayers_ = pWait->GetInteger() != 0;


        ASSERT_TRUE(initSessions());
    }


    virtual void TearDown() X_OVERRIDE
    {
        pNet_->deleteSession(pClientSes_);
        pNet_->deleteSession(pSeverSes_);

        pNet_->deletePeer(pServer_);
        pNet_->deletePeer(pPeer_);
    }

    void pump()
    {
        pSeverSes_->update();
        pClientSes_->update();
    }


    void connectToServer()
    {
        net::SystemAddress sa;
        sa.setToLoopback();
        sa.setPortFromHostByteOrder(SERVER_PORT_BASE);
        pClientSes_->connect(sa);

        auto status = pClientSes_->getStatus();

        ASSERT_NE(SessionStatus::Idle, status);

        int32_t i = 0;
        for (; i < 300; i++)
        {
            pump();

            status = pClientSes_->getStatus();

            if (status == SessionStatus::Connecting)
            {
                core::Thread::sleep(5);
            }
            else
            {
                break;
            }
        }

        ASSERT_TRUE(i < 300) << "failed to connect";
    }

    void createAndJoinGameServer()
    {
        ASSERT_EQ(SessionStatus::Idle, pSeverSes_->getStatus());
        ASSERT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

        TestMatchParameters params;
        params.mode = GameMode::Cooperative;
        params.flags.Set(MatchFlag::Online);

        pSeverSes_->createPartyLobby(params);
        ASSERT_EQ(SessionStatus::Connecting, pSeverSes_->getStatus());

        pump();
        ASSERT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());

        connectToServer();

        ASSERT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());
        ASSERT_EQ(SessionStatus::PartyLobby, pClientSes_->getStatus());

        // users are in our lobby.
        pSeverSes_->createMatch(params);
        ASSERT_EQ(SessionStatus::Connecting, pSeverSes_->getStatus());

        pump();
        ASSERT_EQ(SessionStatus::GameLobby, pSeverSes_->getStatus());

        // The host should of moved to game lobby, and peers have been notified to join.
        // but they won't of yet received the packet.
        ASSERT_EQ(SessionStatus::GameLobby, pSeverSes_->getStatus());
        ASSERT_EQ(SessionStatus::PartyLobby, pClientSes_->getStatus());

        // The client should enter Connecting them GameLobby
        auto lastStatus = pClientSes_->getStatus();
        auto status = lastStatus;

        int32_t i;
        for (i = 0; i < 100; i++)
        {
            pump();

            status = pClientSes_->getStatus();

            if (status != lastStatus)
            {
                if (status == SessionStatus::PartyLobby)
                {
                    // waiting for client to be told to move.
                }
                if (status == SessionStatus::Connecting)
                {
                    X_LOG0("LobbyPartyToGame", "Connecting to lobby");
                    EXPECT_EQ(SessionStatus::PartyLobby, lastStatus);
                }
                else if (status == SessionStatus::GameLobby)
                {
                    X_LOG0("LobbyPartyToGame", "Joined game lobby");
                    EXPECT_EQ(SessionStatus::Connecting, lastStatus);
                    break;
                }
                else
                {
                    ASSERT_TRUE(false) << "Unexpected status";
                }

                lastStatus = status;
            }

            core::Thread::sleep(5);
        }

        ASSERT_TRUE(i < 100) << "failed to move lobby";

        // game lobby buddies?
        ASSERT_EQ(SessionStatus::GameLobby, pSeverSes_->getStatus());
        ASSERT_EQ(SessionStatus::GameLobby, pClientSes_->getStatus());
    }

private:
    bool initSessions()
    {
        pServer_ = pNet_->createPeer();
        pPeer_ = pNet_->createPeer();

        pServer_->setDrainSockets(true);
        pPeer_->setDrainSockets(true);

        net::SocketDescriptor sd(SERVER_PORT_BASE);
        net::SocketDescriptor sd2(SERVER_PORT_BASE + 1);

        auto res = pServer_->init(16, sd);
        if (res != net::StartupResult::Started) {
            return false;
        }

        pServer_->setMaximumIncomingConnections(8);

        res = pPeer_->init(1, sd2);
        if (res != net::StartupResult::Started) {
            return false;
        }

        pSeverSes_ = pNet_->createSession(pServer_, &server_);
        pClientSes_ = pNet_->createSession(pPeer_, &client_);
        return true;
    }

protected:
    net::INet* pNet_;
    net::IPeer* pServer_;
    net::IPeer* pPeer_;

    net::ISession* pSeverSes_;
    net::ISession* pClientSes_;

    Game server_;
    Game client_;

    // some vars
    core::TimeVal defaultTimeout_;
    bool waitForPlayers_;
};



// what do i want to actually test.
// 1) make sure a peer can connect disconnect and connect again
// 2) check a peer can join a game in progress and get correct state.
// 3) verify chat?
// 4) check handling of dropped user cmd's / snap shots per peer.
// 5) check various state behaviour, like failing to connect results in idle.
// do the hookie pokie.

TEST_F(SessionTest, SPLoadAndQuit)
{
    EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

    TestMatchParameters params;
    pSeverSes_->createMatch(params);
    EXPECT_EQ(SessionStatus::Connecting, pSeverSes_->getStatus());

    pump();
    EXPECT_EQ(SessionStatus::GameLobby, pSeverSes_->getStatus());

    auto* pGameLobby = pSeverSes_->getLobby(LobbyType::Game);
    EXPECT_TRUE(pGameLobby->isActive());
    EXPECT_EQ(1, pGameLobby->getNumUsers());
    EXPECT_EQ(0, pGameLobby->getNumConnectedPeers());
    EXPECT_TRUE(pGameLobby->isHost());


    pSeverSes_->startMatch();
    EXPECT_EQ(SessionStatus::Loading, pSeverSes_->getStatus());
    EXPECT_TRUE(pGameLobby->allPeersLoaded());

    pSeverSes_->finishedLoading();
    EXPECT_EQ(SessionStatus::InGame, pSeverSes_->getStatus());
    EXPECT_TRUE(pGameLobby->allPeersLoaded()); // we have no peers.

    pSeverSes_->quitToMenu();
    EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

    EXPECT_FALSE(pGameLobby->isActive());
}


TEST_F(SessionTest, ConnectToIdleHostFail)
{
    EXPECT_EQ(SessionStatus::Idle, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

    net::SystemAddress sa;
    sa.setToLoopback();
    sa.setPortFromHostByteOrder(SERVER_PORT_BASE);
    pClientSes_->connect(sa);

    auto status = pClientSes_->getStatus();

    EXPECT_NE(SessionStatus::Idle, status);

    // we only know the result based on status.
    // poll !
    int32_t i = 0;
    for (; i < 300; i++)
    {
        pump();

        status = pClientSes_->getStatus();

        if (status == SessionStatus::Connecting)
        {
            core::Thread::sleep(10);
        }
        else
        {
            EXPECT_EQ(SessionStatus::Idle, status);
            break;
        }
    }

    ASSERT_TRUE(i < 300) << "failed to timeout";

    EXPECT_EQ(SessionStatus::Idle, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());
}

TEST_F(SessionTest, ConnectToOfflineLobbyFail)
{
    EXPECT_EQ(SessionStatus::Idle, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

    TestMatchParameters params;
    params.flags.Remove(MatchFlag::Online);
    pSeverSes_->createPartyLobby(params);

    EXPECT_EQ(SessionStatus::Connecting, pSeverSes_->getStatus());

    pump();

    EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());

    // we only know the result based on status.
    // poll !
    connectToServer();

    // Make sure the client could not connect.
    // TODO: make sure we got told to go away?
    EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

    // server
    {
        auto* pLobby = pSeverSes_->getLobby(LobbyType::Party);

        EXPECT_EQ(0, pLobby->getNumConnectedPeers());
        EXPECT_EQ(1, pLobby->getNumUsers());
        EXPECT_TRUE(pLobby->isHost());
        EXPECT_FALSE(pLobby->isPeer());
        EXPECT_TRUE(pLobby->allPeersLoaded()); // we have no peers, so technically they are loaded. 
    }

    {
        auto* pLobby = pClientSes_->getLobby(LobbyType::Party);

        EXPECT_EQ(0, pLobby->getNumConnectedPeers());
        EXPECT_EQ(0, pLobby->getNumUsers());
        EXPECT_FALSE(pLobby->isHost());
        EXPECT_FALSE(pLobby->isPeer());
        EXPECT_TRUE(pLobby->allPeersLoaded());
    }
}

TEST_F(SessionTest, ConnectToPartyLobby)
{
    EXPECT_EQ(SessionStatus::Idle, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

    TestMatchParameters params;
    params.flags.Set(MatchFlag::Online);
    pSeverSes_->createPartyLobby(params);

    EXPECT_EQ(SessionStatus::Connecting, pSeverSes_->getStatus());

    pump();

    EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());

    // we only know the result based on status.
    // poll !
    connectToServer();

    // we have connected to the server.
    // check some states.
    EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::PartyLobby, pClientSes_->getStatus());

    // server
    {
        auto* pLobby = pSeverSes_->getLobby(LobbyType::Party);

        EXPECT_EQ(1, pLobby->getNumConnectedPeers());
        EXPECT_EQ(2, pLobby->getNumUsers());
        EXPECT_TRUE(pLobby->isHost());
        EXPECT_FALSE(pLobby->isPeer());
        EXPECT_FALSE(pLobby->allPeersLoaded());
    }

    {
        auto* pLobby = pClientSes_->getLobby(LobbyType::Party);

        EXPECT_EQ(1, pLobby->getNumConnectedPeers());
        EXPECT_EQ(2, pLobby->getNumUsers());
        EXPECT_FALSE(pLobby->isHost());
        EXPECT_TRUE(pLobby->isPeer());
        EXPECT_FALSE(pLobby->allPeersLoaded());
    }
}

TEST_F(SessionTest, ReConnectToPartyLobby)
{
    EXPECT_EQ(SessionStatus::Idle, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

    TestMatchParameters params;
    params.flags.Set(MatchFlag::Online);
    pSeverSes_->createPartyLobby(params);

    EXPECT_EQ(SessionStatus::Connecting, pSeverSes_->getStatus());

    pump();

    for (int32_t loop = 0; loop < 5; loop++)
    {
        EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());

        net::SystemAddress sa;
        sa.setToLoopback();
        sa.setPortFromHostByteOrder(SERVER_PORT_BASE);
        pClientSes_->connect(sa);

        auto status = pClientSes_->getStatus();

        EXPECT_NE(SessionStatus::Idle, status);

        int32_t i = 0;
        for (; i < 300; i++)
        {
            pump();

            status = pClientSes_->getStatus();

            if (status == SessionStatus::Connecting)
            {
                core::Thread::sleep(5);
            }
            else
            {
                EXPECT_EQ(SessionStatus::PartyLobby, status);
                break;
            }
        }

        ASSERT_TRUE(i < 100) << "failed to connect";

        // we have connected to the server.
        // check some states.
        EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());
        EXPECT_EQ(SessionStatus::PartyLobby, pClientSes_->getStatus());


        // now have the client leave.
        pClientSes_->disconnect();

        // the client should send disconnect msg to host, but it might not arrive.
        // in this test it should always arrive.
        // it would just timeout in the real world.
        // what i want to test is that we can re join after.
        // so wait for the peer to leave hosts list.
        pump();

        EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());
        EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

        for (i = 0; i < 300; i++)
        {
            pump();

            auto* pLobby = pSeverSes_->getLobby(LobbyType::Party);

            if (pLobby->getNumUsers() == 2)
            {
                core::Thread::sleep(50);
            }
            else
            {
                EXPECT_EQ(1, pLobby->getNumUsers());
                break;
            }
        }

        ASSERT_TRUE(i < 300) << "peer failed to disconnect from host";

        // check we disconnected.
        EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());
        EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

        // server
        {
            auto* pLobby = pSeverSes_->getLobby(LobbyType::Party);

            EXPECT_EQ(0, pLobby->getNumConnectedPeers());
            EXPECT_EQ(1, pLobby->getNumUsers());
            EXPECT_TRUE(pLobby->isHost());
            EXPECT_FALSE(pLobby->isPeer());
            EXPECT_FALSE(pLobby->hasActivePeers());
        }

        {
            auto* pLobby = pClientSes_->getLobby(LobbyType::Party);

            EXPECT_EQ(0, pLobby->getNumConnectedPeers());
            EXPECT_EQ(0, pLobby->getNumUsers());
            EXPECT_FALSE(pLobby->isHost());
            EXPECT_FALSE(pLobby->isPeer());
            EXPECT_FALSE(pLobby->hasActivePeers());
        }

    }

    EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

    pSeverSes_->quitToMenu();

    EXPECT_EQ(SessionStatus::Idle, pSeverSes_->getStatus());

    {
        auto* pLobby = pSeverSes_->getLobby(LobbyType::Party);

        EXPECT_EQ(0, pLobby->getNumConnectedPeers());
        EXPECT_EQ(0, pLobby->getNumUsers());
        EXPECT_FALSE(pLobby->isHost());
        EXPECT_FALSE(pLobby->isPeer());
        EXPECT_FALSE(pLobby->hasActivePeers());
    }
}

TEST_F(SessionTest, LobbyPartyToGameAndOut)
{
    // Host a party lobby, have peer join.
    // Host enters a game lobby, peer should follow.
    // Host leaves game lobby, peer follows.
    // both host and peer never leave party lobby.

    EXPECT_EQ(SessionStatus::Idle, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

    TestMatchParameters params;
    params.flags.Set(MatchFlag::Online);
    pSeverSes_->createPartyLobby(params);
    EXPECT_EQ(SessionStatus::Connecting, pSeverSes_->getStatus());

    pump();
    EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());

    connectToServer();

    EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::PartyLobby, pClientSes_->getStatus());

    // users are in our lobby.

    pSeverSes_->createMatch(params);

    EXPECT_EQ(SessionStatus::Connecting, pSeverSes_->getStatus());

    pump();
    EXPECT_EQ(SessionStatus::GameLobby, pSeverSes_->getStatus());

    // The host should of moved to game lobby, and peers have been notified to join.
    // but they won't of yet received the packet.
    EXPECT_EQ(SessionStatus::GameLobby, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::PartyLobby, pClientSes_->getStatus());

    // The client should enter Connecting them GameLobby
    auto lastStatus = pClientSes_->getStatus();
    auto status = lastStatus;

    int32_t i;
    for (i = 0; i < 100; i++)
    {
        pump();

        status = pClientSes_->getStatus();

        if (status != lastStatus)
        {
            if (status == SessionStatus::PartyLobby)
            {
                // waiting for client to be told to move.
            }
            if (status == SessionStatus::Connecting)
            {
                X_LOG0("LobbyPartyToGame", "Connecting to lobby");
                EXPECT_EQ(SessionStatus::PartyLobby, lastStatus);
            }
            else if (status == SessionStatus::GameLobby)
            {
                X_LOG0("LobbyPartyToGame", "Joined game lobby");
                EXPECT_EQ(SessionStatus::Connecting, lastStatus);
                break;
            }
            else
            {
                ASSERT_TRUE(false) << "Unexpected status";
            }

            lastStatus = status;
        }

        core::Thread::sleep(1);
    }

    ASSERT_TRUE(i < 100) << "failed to move lobby";

    // game lobby buddies?
    EXPECT_EQ(SessionStatus::GameLobby, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::GameLobby, pClientSes_->getStatus());


    // we should be in both lobbies.
    for (auto lobbyType : { LobbyType::Party, LobbyType::Game })
    {
        {
            auto* pLobby = pSeverSes_->getLobby(lobbyType);

            EXPECT_EQ(1, pLobby->getNumConnectedPeers());
            EXPECT_EQ(2, pLobby->getNumUsers());
            EXPECT_TRUE(pLobby->isHost());
            EXPECT_FALSE(pLobby->isPeer());
            EXPECT_FALSE(pLobby->allPeersLoaded());
        }

        {
            auto* pLobby = pClientSes_->getLobby(lobbyType);

            EXPECT_EQ(1, pLobby->getNumConnectedPeers());
            EXPECT_EQ(2, pLobby->getNumUsers());
            EXPECT_FALSE(pLobby->isHost());
            EXPECT_TRUE(pLobby->isPeer());
            EXPECT_FALSE(pLobby->allPeersLoaded());
        }
    }

    // now the host leaves the game.
    pSeverSes_->cancel();

    EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::GameLobby, pClientSes_->getStatus());

    // check the server shut it's lobby down.
    {
        auto* pPartyLobby = pSeverSes_->getLobby(LobbyType::Party);
        auto* pGameLobby = pSeverSes_->getLobby(LobbyType::Game);

        EXPECT_TRUE(pPartyLobby->isActive());
        EXPECT_FALSE(pGameLobby->isActive());
    }

    // now we should be told to leave the game lobby.
    for (i = 0; i < 100; i++)
    {
        pump();

        status = pClientSes_->getStatus();

        if (status == SessionStatus::GameLobby)
        {
            core::Thread::sleep(1);
        }
        else
        {
            // should be a instant change
            EXPECT_EQ(SessionStatus::PartyLobby, status);
            break;
        }
    }

    ASSERT_TRUE(i < 100) << "failed to move lobby";

    EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::PartyLobby, pClientSes_->getStatus());

    {
        auto* pPartyLobby = pSeverSes_->getLobby(LobbyType::Party);
        auto* pGameLobby = pSeverSes_->getLobby(LobbyType::Game);

        EXPECT_TRUE(pPartyLobby->isActive());
        EXPECT_FALSE(pGameLobby->isActive());

        EXPECT_EQ(1, pPartyLobby->getNumConnectedPeers());
        EXPECT_EQ(2, pPartyLobby->getNumUsers());
        EXPECT_TRUE(pPartyLobby->isHost());
        EXPECT_FALSE(pPartyLobby->isPeer());
        EXPECT_FALSE(pPartyLobby->allPeersLoaded());
    }

    {
        auto* pPartyLobby = pClientSes_->getLobby(LobbyType::Party);
        auto* pGameLobby = pClientSes_->getLobby(LobbyType::Game);

        EXPECT_TRUE(pPartyLobby->isActive());
        EXPECT_FALSE(pGameLobby->isActive());

        EXPECT_EQ(1, pPartyLobby->getNumConnectedPeers());
        EXPECT_EQ(2, pPartyLobby->getNumUsers());
        EXPECT_FALSE(pPartyLobby->isHost());
        EXPECT_TRUE(pPartyLobby->isPeer());
        EXPECT_FALSE(pPartyLobby->allPeersLoaded());
    }
}

TEST_F(SessionTest, PremoteToGameLobby)
{
    // if a host is in game lobby, and a peer connects that peer should move to game lobby.
    EXPECT_EQ(SessionStatus::Idle, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

    TestMatchParameters params;
    params.mode = GameMode::Cooperative;
    params.flags.Set(MatchFlag::Online);

    pSeverSes_->createPartyLobby(params);
    EXPECT_EQ(SessionStatus::Connecting, pSeverSes_->getStatus());

    pump();
    EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());

    pSeverSes_->createMatch(params);
    EXPECT_EQ(SessionStatus::Connecting, pSeverSes_->getStatus());

    pump();
    EXPECT_EQ(SessionStatus::GameLobby, pSeverSes_->getStatus());

    // connect
    connectToServer();

    // so we will join the party.
    // we should get told to join the game.
    EXPECT_EQ(SessionStatus::GameLobby, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::GameLobby, pClientSes_->getStatus());

    // We should be in both lobbies.
    for (auto lobbyType : { LobbyType::Party, LobbyType::Game })
    {
        {
            auto* pLobby = pSeverSes_->getLobby(lobbyType);

            EXPECT_EQ(1, pLobby->getNumConnectedPeers());
            EXPECT_EQ(2, pLobby->getNumUsers());
            EXPECT_TRUE(pLobby->isActive());
            EXPECT_TRUE(pLobby->isHost());
            EXPECT_FALSE(pLobby->isPeer());
            EXPECT_FALSE(pLobby->allPeersLoaded());
        }

        {
            auto* pLobby = pClientSes_->getLobby(lobbyType);

            EXPECT_EQ(1, pLobby->getNumConnectedPeers());
            EXPECT_EQ(2, pLobby->getNumUsers());
            EXPECT_TRUE(pLobby->isActive());
            EXPECT_FALSE(pLobby->isHost());
            EXPECT_TRUE(pLobby->isPeer());
            EXPECT_FALSE(pLobby->allPeersLoaded());
        }
    }
}

TEST_F(SessionTest, PremoteToInGame)
{
    // Have the peer join a game in progress.
    EXPECT_EQ(SessionStatus::Idle, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

    TestMatchParameters params;
    params.mode = GameMode::Cooperative;
    params.flags.Set(MatchFlag::Online);

    pSeverSes_->createPartyLobby(params);
    EXPECT_EQ(SessionStatus::Connecting, pSeverSes_->getStatus());

    pump();
    EXPECT_EQ(SessionStatus::PartyLobby, pSeverSes_->getStatus());

    pSeverSes_->createMatch(params);
    EXPECT_EQ(SessionStatus::Connecting, pSeverSes_->getStatus());

    pump();
    EXPECT_EQ(SessionStatus::GameLobby, pSeverSes_->getStatus());

    pSeverSes_->startMatch();
    EXPECT_EQ(SessionStatus::Loading, pSeverSes_->getStatus());

    pSeverSes_->finishedLoading();
    pump();
    EXPECT_EQ(SessionStatus::InGame, pSeverSes_->getStatus());

    auto* pServerGameLobby = pSeverSes_->getLobby(LobbyType::Game);
    EXPECT_EQ(0, pServerGameLobby->getNumConnectedPeers());
    EXPECT_EQ(1, pServerGameLobby->getNumUsers());
    EXPECT_TRUE(pServerGameLobby->allPeersLoaded());
    EXPECT_TRUE(pServerGameLobby->allPeersInGame());

    // connect
    connectToServer();


    EXPECT_EQ(1, pServerGameLobby->getNumConnectedPeers());
    EXPECT_EQ(2, pServerGameLobby->getNumUsers());
    EXPECT_FALSE(pServerGameLobby->allPeersLoaded());
    EXPECT_FALSE(pServerGameLobby->allPeersInGame());

    // so we will join the party.
    // we should get told to join the game.
    EXPECT_EQ(SessionStatus::InGame, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::GameLobby, pClientSes_->getStatus());

    pump();

    EXPECT_EQ(SessionStatus::InGame, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Loading, pClientSes_->getStatus());

    pClientSes_->finishedLoading();

    int32_t i;
    for (i = 0; i < 10; i++) {
        pump();
    }

    // even tho we have finished loading, we should not switch to in game till got snaps.
    EXPECT_EQ(SessionStatus::InGame, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Loading, pClientSes_->getStatus());

    // The server should know we are loaded tho
    EXPECT_TRUE(pServerGameLobby->allPeersLoaded());
    EXPECT_FALSE(pServerGameLobby->allPeersInGame());

    SnapShot snap0(g_arena);
    pSeverSes_->sendSnapShot(std::move(snap0));

    for (i = 0; i < 10; i++) {
        pump();
    }

    // make sure we are still loading after only getting one snapshop.
    EXPECT_EQ(SessionStatus::Loading, pClientSes_->getStatus());

    SnapShot snap1(g_arena);
    pSeverSes_->sendSnapShot(std::move(snap1));

    // sync in game with host.
    for (i = 0; i < 10; i++) {
        pump();
    }

    // Now we should be in game, and the host should know.
    EXPECT_EQ(SessionStatus::InGame, pClientSes_->getStatus());
    EXPECT_TRUE(pServerGameLobby->allPeersLoaded());
    EXPECT_TRUE(pServerGameLobby->allPeersInGame());
}

TEST_F(SessionTest, LoadGameSyncPeerFirst)
{
    // go through the process of players loading a game.
    createAndJoinGameServer();

    EXPECT_EQ(SessionStatus::GameLobby, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::GameLobby, pClientSes_->getStatus());

    pSeverSes_->startMatch();
    EXPECT_EQ(SessionStatus::Loading, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::GameLobby, pClientSes_->getStatus());

    auto status = pClientSes_->getStatus();

    // wait for client to start loading
    int32_t i;
    for (i = 0; i < 100; i++)
    {
        pump();

        status = pClientSes_->getStatus();

        if (status == SessionStatus::GameLobby)
        {
            core::Thread::sleep(1);
        }
        else
        {
            EXPECT_EQ(SessionStatus::Loading, status);
            break;
        }
    }

    ASSERT_TRUE(i < 100) << "Client failed to start loading";

    core::Thread::sleep(5);

    // The player finished loading, but should stay in loading state until told by host.
    pClientSes_->finishedLoading();

    EXPECT_EQ(SessionStatus::Loading, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Loading, pClientSes_->getStatus());

    auto* pGameLobby = pSeverSes_->getLobby(LobbyType::Game);
    EXPECT_TRUE(pGameLobby->isActive());
    EXPECT_EQ(2, pGameLobby->getNumUsers());
    EXPECT_EQ(1, pGameLobby->getNumConnectedPeers());
    EXPECT_EQ(0, pGameLobby->getNumConnectedPeersInGame());
    EXPECT_TRUE(pGameLobby->isHost());

    // this should be false, as host won't of received packet telling peer has loaded.
    EXPECT_FALSE(pGameLobby->allPeersLoaded());

    for (i = 0; i < 100; i++)
    {
        pump();

        if (pGameLobby->allPeersLoaded()) {
            break;
        }

    }

    ASSERT_TRUE(i < 100) << "Failed to receive loading done packet";

    EXPECT_EQ(1, pGameLobby->getNumConnectedPeers());
    EXPECT_EQ(1, pGameLobby->getNumConnectedPeersLoaded());
    ASSERT_TRUE(pGameLobby->allPeersLoaded());

    // okay now have to host finish loading.
    pSeverSes_->finishedLoading();

    EXPECT_EQ(SessionStatus::Loading, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Loading, pClientSes_->getStatus());

    pump();

    EXPECT_EQ(SessionStatus::InGame, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Loading, pClientSes_->getStatus());

    // Send a snap shot to the player.
    SnapShot snap0(g_arena);
    SnapShot snap1(g_arena);
    pSeverSes_->sendSnapShot(std::move(snap0));

    pump();
    EXPECT_EQ(SessionStatus::Loading, pClientSes_->getStatus());

    pSeverSes_->sendSnapShot(std::move(snap1));

    pump();
    EXPECT_EQ(SessionStatus::InGame, pClientSes_->getStatus());

    // the host don't know yet
    EXPECT_EQ(1, pGameLobby->getNumConnectedPeers());
    EXPECT_EQ(1, pGameLobby->getNumConnectedPeersLoaded());
    EXPECT_EQ(0, pGameLobby->getNumConnectedPeersInGame());

    pump();
    pump();

    // now they do.
    EXPECT_EQ(1, pGameLobby->getNumConnectedPeers());
    EXPECT_EQ(1, pGameLobby->getNumConnectedPeersLoaded());
    EXPECT_EQ(1, pGameLobby->getNumConnectedPeersInGame());

    // all good?
    EXPECT_EQ(SessionStatus::InGame, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::InGame, pClientSes_->getStatus());
}


TEST_F(SessionTest, LoadGameSyncHostFirst)
{
    createAndJoinGameServer();

    EXPECT_EQ(SessionStatus::GameLobby, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::GameLobby, pClientSes_->getStatus());

    pSeverSes_->startMatch();
    EXPECT_EQ(SessionStatus::Loading, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::GameLobby, pClientSes_->getStatus());

    auto status = pClientSes_->getStatus();

    // wait for client to start loading
    int32_t i;
    for (i = 0; i < 100; i++)
    {
        pump();

        status = pClientSes_->getStatus();

        if (status == SessionStatus::GameLobby)
        {
            core::Thread::sleep(1);
        }
        else
        {
            EXPECT_EQ(SessionStatus::Loading, status);
            break;
        }
    }

    ASSERT_TRUE(i < 100) << "Client failed to start loading";

    core::Thread::sleep(5);

    // The host finished loading, but should stay in loading state until told by host.
    pSeverSes_->finishedLoading();

    pump();
    pump();

    EXPECT_EQ(SessionStatus::Loading, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Loading, pClientSes_->getStatus());

    auto* pGameLobby = pSeverSes_->getLobby(LobbyType::Game);
    EXPECT_TRUE(pGameLobby->isActive());
    EXPECT_EQ(2, pGameLobby->getNumUsers());
    EXPECT_EQ(1, pGameLobby->getNumConnectedPeers());
    EXPECT_EQ(0, pGameLobby->getNumConnectedPeersInGame());
    EXPECT_TRUE(pGameLobby->isHost());

    // okay now have to peer finish loading.
    pClientSes_->finishedLoading();

    EXPECT_EQ(SessionStatus::Loading, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Loading, pClientSes_->getStatus());

    // wait for the peers.
    EXPECT_FALSE(pGameLobby->allPeersLoaded());

    for (i = 0; i < 100; i++)
    {
        pump();

        if (pGameLobby->allPeersLoaded()) {
            break;
        }
    }

    ASSERT_TRUE(i < 100) << "Failed to receive loading done packet";

    // The host will of switched to in game now players have loaded.
    pump();
    EXPECT_EQ(SessionStatus::InGame, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Loading, pClientSes_->getStatus());

    // pump a bit make sure players don't enter in game.
    for (i = 0; i < 10; i++) {
        pump();
    }

    EXPECT_EQ(SessionStatus::InGame, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Loading, pClientSes_->getStatus());

    // Send a snap shot to the player.
    SnapShot snap0(g_arena);
    SnapShot snap1(g_arena);
    pSeverSes_->sendSnapShot(std::move(snap0));

    pump();
    EXPECT_EQ(SessionStatus::Loading, pClientSes_->getStatus());

    pSeverSes_->sendSnapShot(std::move(snap1));

    pump();
    EXPECT_EQ(SessionStatus::InGame, pClientSes_->getStatus());

    // the host don't know yet
    EXPECT_EQ(1, pGameLobby->getNumConnectedPeers());
    EXPECT_EQ(0, pGameLobby->getNumConnectedPeersInGame());

    pump();
    pump();

    // now they do.
    EXPECT_EQ(1, pGameLobby->getNumConnectedPeers());
    EXPECT_EQ(1, pGameLobby->getNumConnectedPeersInGame());

    // all good?
    EXPECT_EQ(SessionStatus::InGame, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::InGame, pClientSes_->getStatus());
}

TEST_F(SessionTest, LoadGameTimeout)
{
    // we start loading and the transport dies for both client and server
    // make sure both client and server end up in correct state.
    createAndJoinGameServer();

    EXPECT_EQ(SessionStatus::GameLobby, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::GameLobby, pClientSes_->getStatus());

    pSeverSes_->startMatch();
    EXPECT_EQ(SessionStatus::Loading, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::GameLobby, pClientSes_->getStatus());

    pump();

    EXPECT_EQ(SessionStatus::Loading, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Loading, pClientSes_->getStatus());

    // have the server finish if they are going to wait.
    if (waitForPlayers_)
    {
        pSeverSes_->finishedLoading();

        pump();

        EXPECT_EQ(SessionStatus::Loading, pSeverSes_->getStatus());
        EXPECT_EQ(SessionStatus::Loading, pClientSes_->getStatus());
    }

    // okay now just sleep!
    core::Thread::sleep(defaultTimeout_.GetMilliSecondsAsInt32() + 100);

    // is once enough?
    pump();

    // now server should be in game and client idle.
    EXPECT_EQ(SessionStatus::InGame, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

    for (auto lobbyType : { LobbyType::Party, LobbyType::Game })
    {
        {
            auto* pLobby = pSeverSes_->getLobby(lobbyType);

            EXPECT_EQ(0, pLobby->getNumConnectedPeers());
            EXPECT_EQ(1, pLobby->getNumUsers());
            EXPECT_TRUE(pLobby->isActive());
            EXPECT_TRUE(pLobby->isHost());
            EXPECT_FALSE(pLobby->isPeer());
            EXPECT_TRUE(pLobby->allPeersLoaded());
        }

        {
            auto* pLobby = pClientSes_->getLobby(lobbyType);

            EXPECT_EQ(0, pLobby->getNumConnectedPeers());
            EXPECT_EQ(0, pLobby->getNumUsers());
            EXPECT_FALSE(pLobby->isActive());
            EXPECT_FALSE(pLobby->isHost());
            EXPECT_FALSE(pLobby->isPeer());
            EXPECT_TRUE(pLobby->allPeersLoaded());
        }
    }
}


X_NAMESPACE_END
