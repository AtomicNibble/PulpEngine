#pragma once

#include <ICompression.h>

X_NAMESPACE_DECLARE(core,
                    struct ICVar;)

X_NAMESPACE_BEGIN(net)

class NetVars
{
public:
    NetVars();
    ~NetVars() = default;

    void registerVars(void);

    X_INLINE uint16_t port(void) const;

    X_INLINE int32_t debugEnabled(void) const;
    X_INLINE int32_t debugIgnoredEnabled(void) const;
    X_INLINE int32_t debugDatagramEnabled(void) const;
    X_INLINE int32_t debugAckEnabled(void) const;
    X_INLINE int32_t debugNackEnabled(void) const;
    X_INLINE int32_t debugSocketsEnabled(void) const;

    X_INLINE bool rlConnectionsPerIP(void) const;
    X_INLINE int32_t rlConnectionsPerIPThreshMS(void) const;
    X_INLINE int32_t rlConnectionsPerIPBanTimeMS(void) const;

    X_INLINE int32_t defaultTimeoutMS(void) const;
    X_INLINE int32_t dropPartialConnectionsMS(void) const;
    X_INLINE int32_t pingTimeMS(void) const;
    X_INLINE int32_t unexpectedMsgBanTime(void) const;
    X_INLINE int32_t connectionBSPLimit(void) const;

    X_INLINE bool artificalNetworkEnabled(void) const;
    X_INLINE float32_t artificalPacketLoss(void) const;
    X_INLINE int32_t artificalPing(void) const;
    X_INLINE int32_t artificalPingVariance(void) const;
    X_INLINE bool ignorePasswordFromClientIfNotRequired(void) const;

    X_INLINE core::Compression::Algo::Enum ackCompAlgo(void) const;
    X_INLINE core::Compression::Algo::Enum compAlgo(void) const;

private:
    void Var_OnDefaultTimeoutChanged(core::ICVar* pVar);
    void Var_OnPingTimeChanged(core::ICVar* pVar);
    void Var_OnArtificalNetworkChanged(core::ICVar* pVar);

private:
    int32_t port_;
    int32_t debug_;
    int32_t debugIgnored_;
    int32_t debugDataGram_;
    int32_t debugAck_;
    int32_t debugNACk_;
    int32_t debugSockets_;
    int32_t rlconnectionsPerIp_;
    int32_t rlconnectionsPerIpThreshMS_;
    int32_t rlconnectionsPerIpBanTimeMS_;

    int32_t defaultTimeoutMS_;
    int32_t dropPartialConnectionsMS_;
    int32_t pingTimeMS_;
    int32_t unexpectedMsgBanTime_;
    int32_t connectionBSPLimit_;

    int32_t artificalNetwork_;
    float32_t artificalPacketLoss_;
    int32_t artificalPing_;
    int32_t artificalPingVariance_;
    int32_t ignorePasswordFromClientIfNotRequired_;

    int32_t ackCompAlgo_;
    int32_t compAlgo_;
};

X_NAMESPACE_END

#include "NetVars.inl"