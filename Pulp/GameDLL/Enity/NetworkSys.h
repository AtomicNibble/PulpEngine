#pragma once


X_NAMESPACE_BEGIN(game)

namespace entity
{
    class NetworkSystem
    {
    public:
        bool init(void);

        void clientUpdate(ECS& reg, float frac);
    };

} // namespace entity

X_NAMESPACE_END