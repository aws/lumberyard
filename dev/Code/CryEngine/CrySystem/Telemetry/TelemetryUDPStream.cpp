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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Telemetry UDP stream.


#include "StdAfx.h"
#include "TelemetryUDPStream.h"

namespace Telemetry
{
    CUDPStream::CUDPStream()
        : m_initialized(false)
#ifdef WIN32
        , m_outputSocket(INVALID_SOCKET)
#endif //WIN32
    {
    }

    CUDPStream::~CUDPStream()
    {
        Shutdown();
    }

    bool CUDPStream::Init(const string& serverIP)
    {
#ifdef WIN32
        WSADATA data;

        int         result = WSAStartup(MAKEWORD(2, 2), &data);

        if (result != 0)
        {
            PrintLastWSAError();

            return false;
        }

        m_outputSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (m_outputSocket == INVALID_SOCKET)
        {
            PrintLastWSAError();

            return false;
        }

        m_serverAddress.sin_family          = AF_INET;
        m_serverAddress.sin_port                = htons(1111);
#pragma warning( push )
#pragma warning(disable: 4996)
        m_serverAddress.sin_addr.s_addr = inet_addr(serverIP.c_str());
#pragma warning( pop )
#endif //WIN32

        m_initialized = true;

        return true;
    }

    void CUDPStream::Shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

#ifdef WIN32
        if (m_outputSocket != INVALID_SOCKET)
        {
            closesocket(m_outputSocket);

            m_outputSocket = INVALID_SOCKET;
        }

        WSACleanup();
#endif //WIN32

        m_initialized = false;
    }

    void CUDPStream::Release()
    {
        delete this;
    }

    void CUDPStream::Write(const uint8* pData, size_t size)
    {
#ifdef WIN32
        CRY_ASSERT(pData);

        CRY_ASSERT(size);

        if (m_outputSocket != INVALID_SOCKET)
        {
            if (sendto(m_outputSocket, (const char*)pData, size, 0, (sockaddr*)&m_serverAddress, sizeof(m_serverAddress)) == SOCKET_ERROR)
            {
                PrintLastWSAError();
            }
        }
#endif //WIN32
    }

#ifdef WIN32
    void CUDPStream::PrintLastWSAError() const
    {
        int error = WSAGetLastError();

        if (error != WSAEWOULDBLOCK)
        {
            LPSTR   pErrorMsg = NULL;

            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, error, 0, (LPSTR)&pErrorMsg, 0, 0);

            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "WSA Error : %s\n", pErrorMsg);

            LocalFree(pErrorMsg);
        }
    }
#endif //WIN32
}