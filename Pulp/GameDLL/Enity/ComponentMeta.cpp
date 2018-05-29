#include "stdafx.h"
#include "EnityComponents.h"

#include <MetaTable.h>

/*
    So I need a way to serialize components for the network, and I want to be able to deifne custom packing.
    and also not sync every field.

    So I need to define some meta for hte component fields.

    Poitentially I will be able to use this meta for processing ent descripts from disc.
    Will see.


*/

X_NAMESPACE_BEGIN(game)


namespace entity
{
    IMPLEMENT_META(TransForm)
        net::CompPropQuat(ADD_FIELD(quat)),
        net::CompPropVec(ADD_FIELD(pos)),
    END_META()


    IMPLEMENT_META(Health)
        net::CompPropInt(ADD_FIELD(hp)),
        net::CompPropInt(ADD_FIELD(max)),
    END_META()

    IMPLEMENT_META(SoundObject)
        net::CompPropVec(ADD_FIELD(offset)),
    END_META()

    IMPLEMENT_META(Player)
        net::CompPropInt(ADD_FIELD(currentWpn)),
    END_META()

    IMPLEMENT_META(Inventory)
        net::CompPropArray(ADD_FIELD_ARRAY(clip)),
        net::CompPropArray(ADD_FIELD_ARRAY(ammo)),
    END_META()

} // namespace entity

X_NAMESPACE_END
