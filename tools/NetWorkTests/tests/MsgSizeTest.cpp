#include "stdafx.h"

X_NAMESPACE_BEGIN(net)

TEST(net, msgSizeTest)
{
    net::INet* pNet = gEnv->pNet;
    net::IPeer* pServer = pNet->createPeer();
    net::IPeer* pPeer = pNet->createPeer();

    // create a server and a peer and send diffrent size msg's
    // some bigger than MTU.

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
    uint8_t buf[1024 * 8];

    const uint8_t myPacketID = 0x7f;

    for (size_t i = 0; i < sizeof(buf); i++) {
        buf[i] = i % 255;
    }

    buf[0] = myPacketID;

    size_t recivedCount = 0;
    size_t sendCount = 10;
    size_t testSize = 1; // starting size

    size_t packetsSent = 0;
    size_t bytesSent = 0;
    size_t bytesRecived = 0;

    core::StopWatch timer;

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

            core::Thread::sleep(1);
        }
        else if (curState == State::Sending) {
            // send some data.

            X_LOG1("ServerTest", "Sending: %" PRIuS " packets of length: %" PRIuS, sendCount, testSize);

            for (size_t i = 0; i < sendCount; i++) {
                pPeer->send(buf, testSize, net::PacketPriority::High, net::PacketReliability::ReliableOrdered, serverHandle);

                bytesSent += testSize;
            }

            packetsSent += sendCount;

            recivedCount = 0;
            curState = State::Reciving;
        }
        else if (curState == State::Reciving) {
            // don't care for peer packets.
            pPeer->clearPackets();

            // we should be reciving data.
            net::Packet* pPacket = nullptr;
            for (pPacket = pServer->receive(); pPacket; pServer->freePacket(pPacket), pPacket = pServer->receive()) {
                if (pPacket->getID() == myPacketID) {
                    // we got the packet data, check it's correct.
                    X_LOG2("ServerTest", "Recived packet. length: %" PRIu32, pPacket->bitLength);

                    X_ASSERT(pPacket->bitLength == core::bitUtil::bytesToBits(testSize), "Recived incorrect packet size")(pPacket->bitLength, testSize); 

                    for (size_t i = 1; i < testSize; i++) {
                        X_ASSERT(pPacket->pData[i] == i % 255, "Data invalid")(); 
                    }

                    bytesRecived += testSize;

                    ++recivedCount;
                    if (recivedCount == sendCount) {
                        X_LOG1("ServerTest", "Recived all packets");

                        if (testSize == sizeof(buf)) {
                            curState = State::Complete;
                        }
                        else {
                            curState = State::Sending;
                            testSize += 1;
                            testSize = core::Min(testSize, sizeof(buf));
                        }
                    }
                }
            }
        }
        else if (curState == State::Complete) {
            auto elpased = timer.GetMilliSeconds();

            core::HumanSize::Str sizeStr;

            X_LOG0("ServerTest", "test complete.");
            X_LOG0("ServerTest", "elapsed: ^5%.2fms", elpased);
            X_LOG0("ServerTest", "packetSent: ^5%" PRIuS, packetsSent);
            X_LOG0("ServerTest", "bytesSent: ^5%s", core::HumanSize::toString(sizeStr, bytesSent));
            X_LOG0("ServerTest", "bytesRecived: ^5%s", core::HumanSize::toString(sizeStr, bytesRecived));
            break;
        }
    }

    pNet->deletePeer(pServer);
    pNet->deletePeer(pPeer);
}

X_NAMESPACE_END
