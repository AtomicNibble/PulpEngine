#include "stdafx.h"

#include <SystemAddress.h>

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


class SessionTest : public ::testing::Test
{
protected:
    virtual void SetUp() X_OVERRIDE
    {
        pNet_ = gEnv->pNet;

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

        ASSERT_TRUE(i < 100) << "failed to connect";
    }

private:
    bool initSessions()
    {
        pServer_ = pNet_->createPeer();
        pPeer_ = pNet_->createPeer();

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
};



// what do i want to actually test.
// 1) make sure a peer can connect disconnect and connect again
// 2) check a peer can join a game in progress and get correct state.
// 3) verify chat?
// 4) check handling of dropped user cmd's / snap shots per peer.
// 5) check various state behaviour, like failing to connect results in idle.
// do the hookie pokie.


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

TEST_F(SessionTest, ConnectToPartyLobby)
{
    EXPECT_EQ(SessionStatus::Idle, pSeverSes_->getStatus());
    EXPECT_EQ(SessionStatus::Idle, pClientSes_->getStatus());

    MatchParameters params;
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

    MatchParameters params;
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
        // in this test it should awlays arrive.
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
            EXPECT_FALSE(pLobby->allPeersLoaded());
        }

        {
            auto* pLobby = pClientSes_->getLobby(LobbyType::Party);

            EXPECT_EQ(0, pLobby->getNumConnectedPeers());
            EXPECT_EQ(0, pLobby->getNumUsers());
            EXPECT_FALSE(pLobby->isHost());
            EXPECT_FALSE(pLobby->isPeer());
            EXPECT_FALSE(pLobby->allPeersLoaded());
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
        EXPECT_FALSE(pLobby->allPeersLoaded());
    }
}



X_NAMESPACE_END
