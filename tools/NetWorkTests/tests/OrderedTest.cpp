#include "stdafx.h"

#include <Random\MultiplyWithCarry.h>

X_NAMESPACE_BEGIN(net)

TEST(net, OrderedPacketsTest)
{
    net::INet* pNet = gEnv->pNet;
    net::IPeer* pServer = pNet->createPeer();
    net::IPeer* pPeer = pNet->createPeer();

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

    const uint8_t PACKET_ID = 0x7f;

    core::StopWatch timer;
    core::TimeVal testTime = core::TimeVal::fromMS(5000);
    core::TimeVal endTime = timer.GetTimeVal() + testTime;

    std::array<int32_t, MAX_ORDERED_STREAMS> streamsCnts, recivedStreamCnts;
    streamsCnts.fill(0);
    recivedStreamCnts.fill(0);

    core::Array<uint8_t> data(g_arena);
    data.resize(1024 * 60);

    // now we need to wait for connection to complete.
    int32_t packetsSent = 0;
    int32_t packetsRecived = 0;
    int32_t numTestsComplete = 0;

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
            packetsSent = 0;
            packetsRecived = 0;

            // sends packets on diffrent streams.
            uint32_t numStream = gEnv->xorShift.randRange(1u, 10u);
            for (uint32_t x = 0; x < numStream; x++) {
                size_t stream = gEnv->xorShift.randIndex(MAX_ORDERED_STREAMS);
                uint32_t num = gEnv->xorShift.randRange(2u, 10u);

                for (uint32_t i = 0; i < num; i++) {
                    core::FixedBitStreamNoneOwning bs(data.begin(), data.end(), false);
                    bs.write(PACKET_ID);
                    bs.write(streamsCnts[stream]++);
                    bs.write(stream);

                    // send variang sized packets.
                    uint32_t length = gEnv->xorShift.randRange(static_cast<uint32_t>(bs.sizeInBytes()),
                        static_cast<uint32_t>(data.size()));

                    length = static_cast<uint32_t>(data.size());

                    // send the packet
                    pPeer->send(data.data(), length, net::PacketPriority::High, net::PacketReliability::ReliableOrdered, serverHandle, stream);
                    ++packetsSent;
                }
            }

            // now send single packets on varing streams.
            uint32_t numPacket = gEnv->xorShift.randRange(1u, 10u);
            for (uint32_t x = 0; x < numPacket; x++) {
                size_t stream = gEnv->xorShift.randIndex(MAX_ORDERED_STREAMS);

                core::FixedBitStreamNoneOwning bs(data.begin(), data.end(), false);
                bs.write(PACKET_ID);
                bs.write(streamsCnts[stream]++);
                bs.write(stream);

                // send variang sized packets.
                uint32_t length = gEnv->xorShift.randRange(static_cast<uint32_t>(bs.sizeInBytes()),
                    static_cast<uint32_t>(data.size()));

                // send the packet
                pPeer->send(data.data(), length, net::PacketPriority::High, net::PacketReliability::ReliableOrdered, serverHandle, stream);
                ++packetsSent;
            }

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
                    X_LOG0("SeqTest", "Recived packet. length: %" PRIu32, pPacket->bitLength);

                    int32_t packetNumber;
                    int32_t streamIdx;

                    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);
                    bs.read(packetNumber);
                    bs.read(streamIdx);

                    // skipped any?
                    if (packetNumber > recivedStreamCnts[streamIdx]) {
                        // out of order.
                        X_ASSERT_UNREACHABLE();
                    }
                    else if (packetNumber < recivedStreamCnts[streamIdx]) {
                        // out of order.
                        X_ASSERT_UNREACHABLE();
                    }
                    else {
                        X_LOG0("SeqTest", "Got packet: ^5%" PRIi32 "^7 on channel ^5%" PRIi32,
                            packetNumber, streamIdx);
                    }

                    recivedStreamCnts[streamIdx] = packetNumber + 1;
                    ++packetsRecived;
                }
                else {
                }
            }

            if (packetsRecived >= packetsSent) {
                EXPECT_EQ(packetsSent, packetsRecived);

                ++numTestsComplete;

                if (timer.GetTimeVal() >= endTime) {
                    curState = State::Complete;
                }
                else {
                    curState = State::Sending;
                }
            }
        }
        else if (curState == State::Complete) {
            core::HumanSize::Str sizeStr;
            auto elpased = timer.GetMilliSeconds();

            X_LOG0("SeqTest", "test complete.");
            X_LOG0("SeqTest", "Num sets: %" PRIi32, numTestsComplete);
            X_LOG0("SeqTest", "elapsed: ^5%.2fms", elpased);
            break;
        }
    }

    pNet->deletePeer(pServer);
    pNet->deletePeer(pPeer);
}

X_NAMESPACE_END
