#include "stdafx.h"

#include <Containers\Array.h>
#include <Random\MultiplyWithCarry.h>

#include <Hashing\sha1.h>

X_NAMESPACE_BEGIN(net)

TEST(net, LargePacketTest)
{
    net::INet* pNet = gEnv->pNet;
    net::IPeer* pServer = pNet->createPeer();
    net::IPeer* pPeer = pNet->createPeer();

    // create a server and a peer and send diffrent size msg's
    // some bigger than MTU.

    net::SocketDescriptor sd(SERVER_PORT_BASE);
    net::SocketDescriptor sd2(SERVER_PORT_BASE + 1);

    auto res = pServer->init(16, &sd, 1);
    if (res != net::StartupResult::Started) {
        return;
    }

    pServer->setMaximumIncomingConnections(8);

    res = pPeer->init(1, &sd2, 1);
    if (res != net::StartupResult::Started) {
        return;
    }

    // connect to server.
    auto connectRes = pPeer->connect(net::IPStr("127.0.0.1"), SERVER_PORT_BASE);
    if (connectRes != net::ConnectionAttemptResult::Started) {
        return;
    }

    net::NetGUID serverGuid = pServer->getMyGUID();
    net::NetGUID clientGuid = pPeer->getMyGUID();

    net::SystemHandle serverHandle = net::INVALID_SYSTEM_HANDLE;
    net::SystemHandle clientHandle = net::INVALID_SYSTEM_HANDLE;

    enum class State
    {
        Connecting,
        Sending,
        Reciving,
        Complete
    };

    State curState = State::Connecting;
    int32_t connectionFinishNum = 0;

    const uint8_t PACKET_ID = 0x7f;
    const size_t PACKET_SIZE = 1024 * 1024 * 64; // 64mb packet :D

    core::Array<uint8_t> data(g_arena);
    data.resize(PACKET_SIZE);

    // fill it with junk
    for (auto& b : data) {
        b = safe_static_cast<uint8_t>(gEnv->xorShift.randRange(0u, 255u));
    }

    data[0] = PACKET_ID;

    // we verify the recived data is same with sha1.
    core::Hash::SHA1Digest sendDigest, reciveDigest;
    core::Hash::SHA1 sha1;

    sha1.update(data.data(), data.size());
    sendDigest = sha1.finalize();

    core::StopWatch timer;
    float elpased = 0.f;

    // now we need to wait for connection to complete.
    while (1) {
        pServer->runUpdate();
        pPeer->runUpdate();

        if (curState == State::Connecting) {
            net::Packet* pPacket = nullptr;

            // wait for server to get handshake
            for (pPacket = pServer->receive(); pPacket; pServer->freePacket(pPacket), pPacket = pServer->receive()) {
                if (pPacket->getID() == net::MessageID::ConnectionRequestHandShake) {
                    ++connectionFinishNum;

                    if (pPacket->guid == clientGuid) {
                        clientHandle = pPacket->systemHandle;
                    }
                }
            }

            for (pPacket = pPeer->receive(); pPacket; pPeer->freePacket(pPacket), pPacket = pPeer->receive()) {
                if (pPacket->getID() == net::MessageID::ConnectionRequestAccepted) {
                    ++connectionFinishNum;

                    if (pPacket->guid == serverGuid) {
                        serverHandle = pPacket->systemHandle;
                    }
                }
            }

            if (connectionFinishNum == 2) {
                X_LOG0("ServerTest", "Client and server are connected");
                ASSERT_NE(net::INVALID_SYSTEM_HANDLE, serverHandle);
                ASSERT_NE(net::INVALID_SYSTEM_HANDLE, clientHandle);

                curState = State::Sending;

                timer.Start();
            }

            core::Thread::Sleep(1);
        }
        else if (curState == State::Sending) {
            // send the packet
            pPeer->send(data.data(), data.size(), net::PacketPriority::High, net::PacketReliability::ReliableOrdered, serverHandle);
            curState = State::Reciving;
        }
        else if (curState == State::Reciving) {
            // don't care for peer packets.
            pPeer->clearPackets();

            // we should be reciving data.
            net::Packet* pPacket = nullptr;
            for (pPacket = pServer->receive(); pPacket; pServer->freePacket(pPacket), pPacket = pServer->receive()) {
                if (pPacket->getID() == PACKET_ID) {
                    // we got the packet data, check it's correct.
                    X_LOG0("ServerTest", "Recived packet. length: %" PRIu32, pPacket->bitLength);

                    ASSERT_EQ(core::bitUtil::bytesToBits(PACKET_SIZE), pPacket->bitLength);

                    elpased = timer.GetMilliSeconds();

                    // calculate sha1 of it.
                    sha1.reset();
                    sha1.update(data.data(), data.size());
                    reciveDigest = sha1.finalize();

                    EXPECT_EQ(sendDigest, reciveDigest);

                    curState = State::Complete;
                }
                else {
                }
            }
        }
        else if (curState == State::Complete) {
            core::HumanSize::Str sizeStr;

            X_LOG0("ServerTest", "test complete.");
            X_LOG0("ServerTest", "elapsed: ^5%.2fms", elpased);
            X_LOG0("ServerTest", "packetSize: ^5%s", core::HumanSize::toString(sizeStr, PACKET_SIZE));
            break;
        }
    }

    pNet->deletePeer(pServer);
    pNet->deletePeer(pPeer);
}

X_NAMESPACE_END
