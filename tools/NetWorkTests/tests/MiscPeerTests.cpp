#include "stdafx.h"


X_NAMESPACE_BEGIN(net)

TEST(net, PeerInvalidPort)
{
    net::INet* pNet = gEnv->pNet;
    net::IPeer* pPeer = pNet->createPeer();

    std::array<net::SocketDescriptor,3> sd = { 
        net::SocketDescriptor(1337),
        net::SocketDescriptor(8080), 
        net::SocketDescriptor(0) 
    };
   
    auto res = pPeer->init(1, sd);
    ASSERT_EQ(net::StartupResult::InvalidPort, res);

    pNet->deletePeer(pPeer);
}

TEST(net, PeerInvalidMaxConn)
{
    net::INet* pNet = gEnv->pNet;
    net::IPeer* pPeer = pNet->createPeer();

    net::SocketDescriptor sd(SERVER_PORT_BASE);

    auto res = pPeer->init(0, sd);
    ASSERT_EQ(net::StartupResult::InvalidMaxCon, res);

    pNet->deletePeer(pPeer);
}


TEST(net, PeerInvalidSocketDescriptors)
{
    net::INet* pNet = gEnv->pNet;
    net::IPeer* pPeer = pNet->createPeer();

    net::SocketDescriptor sd(SERVER_PORT_BASE);

    auto res = pPeer->init(1, core::make_span(&sd, 0));
    ASSERT_EQ(net::StartupResult::InvalidSocketDescriptors, res);

    pNet->deletePeer(pPeer);
}

TEST(net, PeerPortInUse)
{
    net::INet* pNet = gEnv->pNet;
    net::IPeer* pServer = pNet->createPeer();
    net::IPeer* pPeer = pNet->createPeer();

    net::SocketDescriptor sd(SERVER_PORT_BASE);

    auto res = pServer->init(1, sd);
    ASSERT_EQ(net::StartupResult::Started, res);
    
    res = pPeer->init(1, sd);
    EXPECT_EQ(net::StartupResult::SocketPortInUse, res);

    pNet->deletePeer(pServer);
    pNet->deletePeer(pPeer);
}

X_NAMESPACE_END