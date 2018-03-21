/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef AZCORE_DIRECT_SOCKET_H
#define AZCORE_DIRECT_SOCKET_H

#include <AzCore/IPC/SharedMemory.h>
#include <AzCore/Memory/SystemAllocator.h>

#if defined(AZ_PLATFORM_WINDOWS) // Currently only windows support

namespace AZ
{
    /**
     * Emulate basic connectionless (UDP) socket behavior using \ref SharedMemory.
     * IMPORTANT: As with UDP socket behavior it's not guaranteed that all send will make it.
     * Generally all data is reliable UDP, but if one of the applications crashes or exit while
     * it sends or receives data we will clear all the data. So if you want your socket to work
     * properly even when the other apps crash/exit unexpectedly (so the OS must clean) you can just
     * just add a message id (which you increment every time) this time you can be sure the packets
     * are consecutive. If this is something we need often, we can write ReliableDirectSocket which
     * implements this basic strategy.
     */
    class DirectSocket
    {
        SharedMemoryRingBuffer      m_receiveBuffer;
        struct Connection
        {
            SharedMemoryRingBuffer  sendBuffer;
            DWORD                   lastSend;
        };
        Connection          m_connectionCache[16];
        struct MessageHeader
        {
            unsigned int  nameSize;
            unsigned int  dataSize;
        };
        typedef SharedMemoryRingBuffer::MemoryGuard MemoryGuard;

        Connection* MapNameToConnection(const char* name);
        DirectSocket(const DirectSocket&);
        DirectSocket& operator=(const DirectSocket&);
    public:

        AZ_CLASS_ALLOCATOR(DirectSocket, SystemAllocator, 0)

        static const int InvalidConnection = -1;

        // name is the same as port when using sockets. of you pass NULL for a name an automatic unique name will be generated for you.
        DirectSocket(const char* name = NULL, unsigned int receiveBufferSize = 256* 1024);
        ~DirectSocket();

        unsigned int SendTo(const void* data, unsigned int dataSize, const char* to);
        unsigned int Receive(void* data, unsigned int dataSize, char* from, unsigned int fromSize);
    };
}

#endif // AZ_PLATFORM_WINDOWS

#endif // AZCORE_DIRECT_SOCKET_H
#pragma once
