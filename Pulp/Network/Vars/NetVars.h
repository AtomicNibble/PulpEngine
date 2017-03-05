#pragma once



X_NAMESPACE_DECLARE(core,
	struct ICVar;
)

X_NAMESPACE_BEGIN(net)


class NetVars
{
public:
	NetVars();
	~NetVars() = default;

	void registerVars(void);

	X_INLINE int32_t debugEnabled(void) const;
	X_INLINE int32_t debugSocketsEnabled(void) const;

	X_INLINE bool rlConnectionsPerIP(void) const;
	X_INLINE int32_t rlConnectionsPerIPThreshMS(void) const;
	X_INLINE int32_t rlConnectionsPerIPBanTimeMS(void) const;

	X_INLINE int32_t dropPartialConnectionsMS(void) const;

	X_INLINE int32_t pingTimeMS(void) const;



private:
	int32_t debug_;
	int32_t debugSockets_;
	int32_t rlconnectionsPerIp_;
	int32_t rlconnectionsPerIpThreshMS_;
	int32_t rlconnectionsPerIpBanTimeMS_;

	int32_t dropPartialConnectionsMS_;
	int32_t pingTimeMS_;

};


X_NAMESPACE_END


#include "NetVars.inl"