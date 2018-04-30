#include "stdafx.h"
#include "Session.h"


#include <I3DEngine.h>
#include <IPrimativeContext.h>
#include <IFont.h>

X_NAMESPACE_BEGIN(net)

Session::Session(IPeer* pPeer, core::MemoryArenaBase* arena) :
    pPeer_(X_ASSERT_NOT_NULL(pPeer)),
    arena_(X_ASSERT_NOT_NULL(arena))
{

}

void Session::runUpdate(void)
{
    pPeer_->runUpdate();

    Packet* pPacket = nullptr;
    for (pPacket = pPeer_->receive(); pPacket; pPeer_->freePacket(pPacket), pPacket = pPeer_->receive()) 
    {
        X_LOG0("Session", "Recived packet: bitLength: %" PRIu32, pPacket->bitLength);
        
        core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

        switch(pPacket->getID()) 
        {
            case MessageID::ConnectionRequestAccepted:
            {
                X_LOG0("Session", "Connected to server");
                pleb_ = pPacket->systemHandle;
                break;
            }
            case MessageID::ChatMsg:
            {
                // it's a chat msg.
                static char buf[256] = { '\0' };

                core::zero_object(buf);

                auto len = bs.read<int16_t>();
                bs.read(buf, len);

                X_LOG0("Char", "Msg: %s", buf);

                auto* pPRim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::PERSISTENT);
                font::TextDrawContext con;
                con.col = Col_Red;
                con.pFont = gEnv->pFontSys->GetDefault();

                Matrix33f ang = Matrix33f::createRotation(Vec3f(1.f,0.f,0.f), ::toRadians(-90.f));
                
                static int32_t num = 0;

                pPRim->drawText(Vec3f(0.f,0.f,80.f - (num * 18.f)), ang, con, buf);

                num++;
                break;
            }

            default:
                break;
        }

    }

}

void Session::sendChatMsg(const char* pMsg)
{
    core::FixedBitStreamStack<256> bs;

    auto len = core::strUtil::strlen(pMsg);
    auto len16 = safe_static_cast<uint16_t>(len);

    bs.write(MessageID::ChatMsg);
    bs.write(len16);
    bs.write(pMsg, len);

    pPeer_->send(bs.data(), bs.sizeInBytes(), PacketPriority::High, PacketReliability::Reliable, pleb_);

}

X_NAMESPACE_END