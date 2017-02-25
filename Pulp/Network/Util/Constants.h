#pragma once


#ifdef INVALID_SOCKET
#undef INVALID_SOCKET
#endif // !INVALID_SOCKET

X_NAMESPACE_BEGIN(net)


static const SocketHandle INVALID_SOCKET = SocketHandle(~0);


X_NAMESPACE_END
