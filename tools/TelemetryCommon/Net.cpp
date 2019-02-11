#include "stdafx.h"
#include "Net.h"
#include "PacketTypes.h"

#include <cstdio>

bool readPacket(platform::SOCKET& socket, char* pBuffer, int& bufLengthInOut)
{
    // this should return complete packets or error.
    int bytesRead = 0;

    int bufLength = sizeof(PacketBase);

    while (1) {
        int maxReadSize = bufLength - bytesRead;
        int res = platform::recv(socket, &pBuffer[bytesRead], maxReadSize, 0);

        if (res == 0) {
            printf("Connection closing...\n");
            return false;
        }
        else if (res < 0) {
            printf("recv failed with error: %d\n", platform::WSAGetLastError());
            return false;
        }

        bytesRead += res;

        printf("got: %d bytes\n", res);

        if (bytesRead == sizeof(PacketBase)) 
        {
            auto* pHdr = reinterpret_cast<const PacketBase*>(pBuffer);
            if (pHdr->dataSize == 0) {
                printf("Client sent packet with length zero...\n");
                return false;
            }

            if (pHdr->dataSize > bufLengthInOut) {
                printf("Client sent oversied packet...\n");
                return false;
            }

            bufLength = pHdr->dataSize;
        }

        if (bytesRead == bufLength) {
            bufLengthInOut = bytesRead;
            return true;
        }
        else if (bytesRead > bufLength) {
            printf("Overread packet bytesRead: %d recvbuflen: %d\n", bytesRead, bufLength);
            return false;
        }
    }
}
