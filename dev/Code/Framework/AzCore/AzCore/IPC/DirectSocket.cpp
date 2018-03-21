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
#ifndef AZ_UNITY_BUILD

#include <AzCore/PlatformIncl.h>
#include <AzCore/IPC/DirectSocket.h>

using namespace AZ;

#if defined(AZ_PLATFORM_WINDOWS)

#include <stdio.h>

//=========================================================================
// DirectSocket
// [4/28/2011]
//=========================================================================
DirectSocket::DirectSocket(const char* name, unsigned int receiveBufferSize)
{
    char processName[128];
    if (receiveBufferSize == NULL)
    {
        DWORD processID = GetCurrentProcessId();
        DWORD time = GetTickCount();
        azsnprintf(processName, AZ_ARRAY_SIZE(processName), "DirectSocket-%d-%d", processID, time);
        name = processName;
    }
    bool isReady = m_receiveBuffer.Create(name, receiveBufferSize);
    (void)isReady;
    AZ_Assert(isReady, "Failed to create receive buffer with name %s and size %d", name, receiveBufferSize);
    m_receiveBuffer.Map();
}

//=========================================================================
// ~DirectSocket
// [4/28/2011]
//=========================================================================
DirectSocket::~DirectSocket()
{
}

//=========================================================================
// MapName
// [4/28/2011]
//=========================================================================
DirectSocket::Connection*
DirectSocket::MapNameToConnection(const char* name)
{
    // find the connection if not replace the oldest.
    bool isInCache = false;
    Connection* conn = &m_connectionCache[0];
    DWORD oldestSendTime = m_connectionCache[0].lastSend;
    for (int i = 0; i < AZ_ARRAY_SIZE(m_connectionCache); ++i)
    {
        if (m_connectionCache[i].sendBuffer.IsReady())
        {
            if (strcmp(m_connectionCache[i].sendBuffer.GetName(), name) == 0)
            {
                conn = &m_connectionCache[i];
                isInCache = true;
                break;
            }
            if (m_connectionCache[i].lastSend < oldestSendTime)
            {
                conn = &m_connectionCache[i];
                oldestSendTime = m_connectionCache[i].lastSend;
            }
        }
        else
        {
            conn = &m_connectionCache[i];
            break;
        }
    }

    if (!isInCache)
    {
        conn->sendBuffer.Close();
        conn->sendBuffer.Open(name);
        conn->sendBuffer.Map();
    }

    return conn;
}

//=========================================================================
// SendTo
// [4/28/2011]
//=========================================================================
unsigned int
DirectSocket::SendTo(const void* data, unsigned int dataSize, const char* to)
{
    MessageHeader mh;
    mh.nameSize = static_cast<unsigned int>(strlen(m_receiveBuffer.GetName()));
    mh.dataSize = dataSize;
    Connection* conn = MapNameToConnection(to);
    {
        MemoryGuard g(conn->sendBuffer);
        if (conn->sendBuffer.IsLockAbandoned())
        {
            conn->sendBuffer.Clear();
        }
        conn->lastSend = GetTickCount();
        if (conn->sendBuffer.MaxToWrite() < sizeof(mh) + mh.nameSize + dataSize)
        {
            return 0;
        }
        if (conn->sendBuffer.Write(&mh, sizeof(mh)) == 0)
        {
            AZ_Assert(false, "conn->sendBuffer.MaxToWrite function is wrong!");
        }
        if (conn->sendBuffer.Write(m_receiveBuffer.GetName(), mh.nameSize) == 0)
        {
            AZ_Assert(false, "conn->sendBuffer.MaxToWrite function is wrong!");
        }
        return conn->sendBuffer.Write(data, dataSize) ? dataSize : 0;
    }
}

//=========================================================================
// Receive
// [4/28/2011]
//=========================================================================
unsigned int
DirectSocket::Receive(void* data, unsigned int dataSize, char* from, unsigned int fromSize)
{
    MemoryGuard g(m_receiveBuffer);
    if (m_receiveBuffer.IsLockAbandoned())
    {
        m_receiveBuffer.Clear();
        return 0; // ignore the data.
    }
    MessageHeader mh;
    if (m_receiveBuffer.DataToRead() >= sizeof(mh))
    {
        m_receiveBuffer.Read(&mh, sizeof(mh));
        AZ_Warning("System", fromSize > mh.nameSize, "from name buffer is too small we need %d bytes, but you provide %d", mh.nameSize, fromSize);
        if (fromSize <= mh.nameSize)
        {
            return 0;
        }
        m_receiveBuffer.Read(from, mh.nameSize);
        from[mh.nameSize] = '\0';
        AZ_Warning("System", dataSize >= mh.dataSize, "Data buffer %d is not big enough to receive the message %d! We don't support half read!", dataSize, mh.dataSize);
        if (dataSize < mh.dataSize)
        {
            return 0;
        }
        return m_receiveBuffer.Read(data, mh.dataSize);
    }
    return 0;
}

#endif // AZ_PLATFORM_WINDOWS

#endif // #ifndef AZ_UNITY_BUILD
