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
        entIdMap_.fill(INVALID_ENT_ID);

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

    void NetworkSystem::buildSnapShot(EnitiyRegister& reg, net::SnapShot& snap, physics::IScene* pScene)
    {
        X_UNUSED(reg, snap);

        physics::ScopedLock lock(pScene, physics::LockAccess::Read);

        core::FixedBitStreamStack<256> bs; // max-size?

        auto view = reg.view<NetworkSync, TransForm>();
        for (auto entityId : view)
        { 
            // you want to be synced, fuck you!
            auto& netSync = reg.get<NetworkSync>(entityId);
            X_UNUSED(netSync);

            // lets start with just transforms, mmmmmmmmmm.
            auto& trans = reg.get<TransForm>(entityId);
            auto mask = reg.getComponentMask(entityId);

            bs.write(entityId);
            bs.write(mask);

            // what i want to happen is ent component is serialized using it's meta table.
            writeData(bs, TransForm::pMetaTable_, reinterpret_cast<const uint8_t*>(&trans));

            // okay the data is in the bit stream, magic!
            // i will need to deal with components, not all componentes will be synced (i'm guessing)
            // but maybe i should just assume they are, and then just use the indexes to refrence them.
            // as basically for a given ent I need to comunicate components.
            // so a bit flag maybe? depends how many components we get.
            // maybe a huffman tree?

            // the next problem is how to give ents ids?
            // that are the same on all systems.
            // can I just use the enity id?
            // probs not, for now it should work.
            if (reg.has<DynamicObject>(entityId))
            {
                auto& col = reg.get<DynamicObject>(entityId);

                col.writeToSnapShot(pScene, bs);
            }

            snap.addObject(entityId, bs);

            bs.reset();
        }
    }

    void NetworkSystem::applySnapShot(EnitiyRegister& reg,
        const net::SnapShot& snap, physics::IScene* pScene, engine::IWorld3D* p3DWorld)
    {
        X_UNUSED(reg);

        physics::ScopedLock lock(pScene, physics::LockAccess::Write);

        // I need snapshots to be able to handle adding and removing ents.
        // but also adding and removing components.
        // think i might have some kinda blocks for that.
        // how do i even keep track of components been added and removed lol?
        // for now we just send them fucking all, and check.
        // i would actually like to send component data in blocks in the snapshot.
        // then can update all physics things in a batch.
        // would mean i'd need some sort of mapping data.
        // maybe we don't do SOA in the snapshot but build arrays for the updates?
        // kinda lame, if can encode the compnent info nice SOA in snapshots work well.

        /*
        
            Transforms:
                - ids
                - Trans: 1
                - Trans: 2
                - Trans: 3
                - Trans: 4

            Physics:
                - ids
                - Props: 1
                - Props: 2
                - Props: 5

            Guess we could just sort them and do id ranges?

            So just trying to think..
            what things are we going to be sending data wise?
            - position
            - ammo counts
            - current weapon
            - inventory.
            
            There is also a lot of ents the client can just be told to spawn and handle the lifetime locally.
            These should not be part of the snapshot I think.

            What if i just make it so all network synced ents are below a certain id, then they can awlays map directly.
            Something like 256, sounds resonable.

            So the id's of ent's will always match up, nice.
            only problem is maybe memory waste with the ECS, but can sort that later.
        */
        

        for (size_t i = 0; i < snap.getNumObjects(); i++)
        {
            auto bs = snap.getMessageByIndex(i);
            
            auto remoteEntityId = bs.read<entity::EntityId>();

            auto entityId = entIdMap_[remoteEntityId];

            if (bs.isEos())
            {
                X_ASSERT(reg.isValid(entityId), "Enitity id is not valid")(entityId);

                // destroy the ent.
                // TODO: cleanup.
                reg.destroy(entityId);

                entIdMap_[remoteEntityId] = INVALID_ENT_ID;
                continue;
            }

            // spawn?
            if (entityId == INVALID_ENT_ID)
            {
                if (entityId < net::MAX_PLAYERS)
                {
                    // this is a player.
                    // make a player..
                    entityId = reg.create<entity::TransForm>();


                }
                else
                {
                    // we make the ent and the normal path should handle adding the components.
                    entityId = reg.create<entity::TransForm>();

                }
                
                entIdMap_[remoteEntityId] = entityId;
            }
           

            X_ASSERT(reg.isValid(entityId), "Enitity id is not valid")(entityId);

            auto localMask = reg.getComponentMask(entityId);
            decltype(localMask) mask;
            bs.read(mask);

            // comonents changed?
            if (mask != localMask)
            {
                // well shit on my tits.
                if ((mask & localMask).any())
                {
                    // add
                    X_ASSERT_NOT_IMPLEMENTED();
                }
                else
                {
                    // remove
                    X_ASSERT_NOT_IMPLEMENTED();
                }
            }


            auto& trans = reg.get<TransForm>(entityId);
            
            // we always have transform?
            readData(bs, TransForm::pMetaTable_, reinterpret_cast<uint8_t*>(&trans));

            // for everything else do i just have to check the flags?


            if (reg.has<DynamicObject>(mask))
            {
                auto& dyn = reg.get<DynamicObject>(entityId);
                dyn.readFromSnapShot(pScene, bs);
            }

            if (reg.has<Player>(mask))
            {
               // auto& ply = reg.get<Player>(entityId);


            }

            if (reg.has<Inventory>(mask))
            {


            }

            if (reg.has<MeshRenderer>(mask))
            {
                auto& rend = reg.get<MeshRenderer>(entityId);
                p3DWorld->updateRenderEnt(rend.pRenderEnt, trans);
            }

            if (reg.has<CharacterController>(mask))
            {
                auto& con = reg.get<CharacterController>(entityId);

                con.pController->setFootPosition(Vec3d(trans.pos));
            }
        }
    }

} // namespace entity

X_NAMESPACE_END