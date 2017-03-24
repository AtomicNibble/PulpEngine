#include "stdafx.h"


#include <SystemAddress.h>

X_NAMESPACE_BEGIN(net)


TEST(net, SystemAddress)
{

	net::INet* pNet = gEnv->pNet;

	net::IPStr ipStr;

	net::SystemAddress sa;
	pNet->systemAddressFromHost(HostStr("www.google.com"), sa, IpVersion::Ipv4);
	pNet->systemAddressToString(sa, ipStr);

	net::SystemAddress sa6;
	pNet->systemAddressFromHost(HostStr("www.google.com"), sa6, IpVersion::Ipv6);
	pNet->systemAddressToString(sa6, ipStr);

}


X_NAMESPACE_END