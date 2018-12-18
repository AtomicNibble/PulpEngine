#include "stdafx.h"
#include "NetworkSys.h"

#include <IFrameData.h>
#include <MetaTable.h>
#include <SnapShot.h>
#include <IWorld3D.h>

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
                case net::CompPropType::Array:
                case net::CompPropType::String:
                case net::CompPropType::DataTable:
                    X_ASSERT_NOT_IMPLEMENTED();
                    break;
                
                default:
                    X_NO_SWITCH_DEFAULT_ASSERT;
                    break;
            }

            bs.writeBits(pFieldData, numBits);
        }
    }


    void readData(core::FixedBitStreamBase& bs, net::CompTable* pTable, uint8_t* pData)
    {
        for (size_t i = 0; i < pTable->numProps(); i++)
        {
            auto& prop = pTable->getProp(i);

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
                case net::CompPropType::Array:
                case net::CompPropType::DataTable:
                    X_ASSERT_NOT_IMPLEMENTED();
                    break;

                default:
                    X_NO_SWITCH_DEFAULT_ASSERT;
                        break;
            }

            bs.readBits(pFieldData, numBits);
        }
    }

    void NetworkSystem::buildSnapShot(EnitiyRegister& reg, net::SnapShot& snap)
    {
        X_UNUSED(reg, snap);

        core::FixedBitStreamStack<256> bs;

        auto view = reg.view<NetworkSync, TransForm>();
        for (auto entityId : view)
        {
            // you want to be synced, fuck you!
            auto& netSync = reg.get<NetworkSync>(entityId);
            X_UNUSED(netSync);

            // lets start with just transforms, mmmmmmmmmm.
            auto& trans = reg.get<TransForm>(entityId);

            bs.write(entityId);

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

            snap.addObject(entityId, bs);

            bs.reset();
        }
    }

    void NetworkSystem::applySnapShot(EnitiyRegister& reg,
        const net::SnapShot& snap, physics::IScene* pScene, engine::IWorld3D* p3DWorld)
    {
        X_UNUSED(reg);

        physics::ScopedLock lock(pScene, physics::LockAccess::Write);

        for (size_t i = 0; i < snap.getNumObjects(); i++)
        {
            auto bs = snap.getMessageByIndex(i);
            
            auto entityId = bs.read<entity::EntityId>();
            X_ASSERT(reg.has<NetworkSync>(entityId), "Enitity in shapshot not synced")(entityId);

            auto& trans = reg.get<TransForm>(entityId);

            readData(bs, TransForm::pMetaTable_, reinterpret_cast<uint8_t*>(&trans));

            // for a networked ent we need to know where the server wants us to be.
            // then move towards it.
            //

            if (reg.has<DynamicObject>(entityId))
            {
                auto& col = reg.get<DynamicObject>(entityId);

                pScene->setGlobalPose(col.actor, trans);
            }
            if (reg.has<Player>(entityId))
            {
               // auto& ply = reg.get<Player>(entityId);


            }

            if (reg.has<MeshRenderer>(entityId))
            {
                auto& rend = reg.get<MeshRenderer>(entityId);
                p3DWorld->updateRenderEnt(rend.pRenderEnt, trans);
            }

            if (reg.has<CharacterController>(entityId))
            {
                auto& con = reg.get<CharacterController>(entityId);

                con.pController->setFootPosition(Vec3d(trans.pos));
            }
        }
    }

} // namespace entity

X_NAMESPACE_END