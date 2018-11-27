#include "EngineCommon.h"
#include "UserCmd.h"


#include "Containers\FixedBitStream.h"

X_NAMESPACE_BEGIN(net)

X_ENSURE_SIZE(UserCmd, 36); // More fields? update this;

void UserCmd::writeToBitStream(core::FixedBitStreamBase& bs) const
{
    // Myyyy wugga nugger
    bs.write(clientGameTimeMS);
    bs.write(serverGameTimeMS);
    bs.write(moveForwrd);
    bs.write(moveRight);
    bs.write(buttons);
    bs.write(angles);
    bs.write(mouseDelta);
    bs.write(impulse);
    bs.write(impulseSeq);
    bs.alignWriteToByteBoundry();
}

void UserCmd::fromBitStream(core::FixedBitStreamBase& bs)
{
    bs.read(clientGameTimeMS);
    bs.read(serverGameTimeMS);
    bs.read(moveForwrd);
    bs.read(moveRight);
    bs.read(buttons);
    bs.read(angles);
    bs.read(mouseDelta);
    bs.read(impulse);
    bs.read(impulseSeq);
    bs.alignReadToByteBoundry();
}

X_NAMESPACE_END
