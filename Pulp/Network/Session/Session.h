#pragma once


X_NAMESPACE_BEGIN(net)

X_DECLARE_ENUM(SessionState)(
    
    Idle,
    Loading,
    InGame
);


class Session : public ISession
{
public:
    Session(IPeer* pPeer, core::MemoryArenaBase* arena);

    void runUpdate(void) X_FINAL;

    void sendChatMsg(const char* pMsg);


    X_INLINE IPeer* getPeer(void) const;

private:
    IPeer* pPeer_;
    core::MemoryArenaBase* arena_;

    SystemHandle pleb_;
};


X_INLINE IPeer* Session::getPeer(void) const
{
    return pPeer_;
}

X_NAMESPACE_END
