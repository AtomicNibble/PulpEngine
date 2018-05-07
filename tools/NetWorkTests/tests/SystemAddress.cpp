#include "stdafx.h"

#include <SystemAddress.h>

X_NAMESPACE_BEGIN(net)

TEST(net, SystemAddress)
{
    net::INet* pNet = gEnv->pNet;

    net::IPStr ipStr;

    net::SystemAddress sa;
    EXPECT_TRUE(pNet->systemAddressFromHost(HostStr("www.google.com"), sa, IpVersion::Ipv4));
    pNet->systemAddressToString(sa, ipStr, true);

    X_LOG0("SysAddressTest", "google.com ipv4: \"%s\"", ipStr.c_str());

    net::SystemAddress sa6;
    EXPECT_TRUE(pNet->systemAddressFromHost(HostStr("www.google.com"), sa6, IpVersion::Ipv6));
    pNet->systemAddressToString(sa6, ipStr, true);

    X_LOG0("SysAddressTest", "google.com ipv6: \"%s\"", ipStr.c_str());
}

X_NAMESPACE_END