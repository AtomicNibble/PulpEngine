#include "stdafx.h"
#include "NetworkSys.h"

#include <IFrameData.h>
#include <MetaTable.h>
#include <SnapShot.h>

#include <Containers\FixedBitStream.h>

X_NAMESPACE_BEGIN(game)

namespace entity
{
    bool NetworkSystem::init(void)
    {

        return true;
    }

    void writeData(core::FixedBitStreamBase& bs, net::CompTable* pTable, const uint8_t* pData)
    {
        for (size_t i = 0; i < pTable->numProps(); i++)
        {
            auto& prop = pTable->getProp(i);

            // write it.
            auto type = prop.getType();
            auto offset = prop.getFieldOffset();
            auto numBits = prop.getNumBits();

            auto* pFieldData = (pData + offset);

            switch (type)
            {
                case net::CompPropType::Vector:
                    numBits *= 3;
                    break;
                case net::CompPropType::Quaternion:
                    numBits *= 4;
                    break;

                case net::CompPropType::Int:
                case net::CompPropType::Int64:
                case net::CompPropType::Float:
                case net::CompPropType::VectorXY:
                case net::CompPropType::String:
                case net::CompPropType::DataTable:
                    X_ASSERT_NOT_IMPLEMENTED();
                    break;
                
                default:
                    X_NO_SWITCH_DEFAULT_ASSERT
                    break;
            }

            bs.writeBits(pFieldData, numBits);
        }
    }

    void NetworkSystem::buildSnapShot(core::FrameTimeData& timeInfo, EnitiyRegister& reg, net::SnapShot& snap)
    {
        X_UNUSED(timeInfo, reg, snap);

        core::FixedBitStreamStack<256> bs;

        auto view = reg.view<NetworkSync, TransForm>();
        for (auto entity : view)
        {
            // you want to be synced, fuck you!
            auto& netSync = reg.get<NetworkSync>(entity);
            X_UNUSED(netSync);

            // lets start with just transforms, mmmmmmmmmm.
            auto& trans = reg.get<TransForm>(entity);

            // what i want to happen is ent component is serialized using it's meta table.
            writeData(bs, TransForm::pMetaTable_, reinterpret_cast<const uint8_t*>(&trans));

            // okay the data is in the bit stream, magic!
            // i will need to deal with components, not all componentes will ne synced (i'm guessing)
            // but maybe i should just assume they are, and then just use the indexes to refrence them.
            // as basically for a given ent I need to comunicate components.
            // so a bit flag maybe? depends how many components we get.
            // maybe a huffman tree?

            // the next problem is how to give ents ids?
            // that are the same on all systems.
            // can I just use the enity id?
            // probs not, for now it should work.

            snap.addObject(entity, bs);

            bs.reset();
        }
    }


} // namespace entity

X_NAMESPACE_END