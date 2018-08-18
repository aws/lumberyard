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

#include "stdafx.h"
#include "PipeClient.h"

//////////////////////////////////////////////////////////////////////////

CPipeClient::CPipeClient()
    : m_pipe(INVALID_HANDLE_VALUE)
    , m_isRunning(false)
    , m_buffer(0)
    , m_listener(0)
{
}

//////////////////////////////////////////////////////////////////////////

CPipeClient::~CPipeClient()
{
    delete[] m_buffer;
}

//////////////////////////////////////////////////////////////////////////

bool CPipeClient::isConnected()
{
    return m_isRunning;
}

//////////////////////////////////////////////////////////////////////////

void CPipeClient::OpenConnection(const char* pipeName, IPipeClientListener* listener, uint32 bufferSize)
{
    CryAutoLock<CryMutex> l(GetLock());

    if (m_isRunning)
    {
        return;
    }

    if (bufferSize == 0)
    {
        bufferSize = DEFAULT_BUFFER_SIZE;
    }

    m_bufSize = bufferSize;
    m_buffer = new char[m_bufSize];
    m_message.reserve(m_bufSize);
    m_listener = listener;

    m_pipeName = pipeName;

    m_isRunning = true;
    Start(*this);
}

//////////////////////////////////////////////////////////////////////////

void CPipeClient::CloseConnection()
{
    m_isRunning = false;
    Stop();
}

//////////////////////////////////////////////////////////////////////////

void CPipeClient::Run()
{
    CryThreadSetName(-1, "TelemetryPipeThread");

    while (m_isRunning && !TryOpenConnection())
    {
        /*nop*/
        ;
    }

    // Stop called while trying to connect
    if (!m_isRunning)
    {
        return;
    }

    // Now connected, start reading messages
    for (;; )
    {
        ReceiveMessage();

        if (!m_isRunning || m_message.empty())
        {
            break;
        }

        if (m_listener)
        {
            m_listener->OnMessage(m_message);
        }
    }


    { /* locked */
        CryAutoLock<CryMutex> l(GetLock());

        m_isRunning = false;

        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;

        delete[] m_buffer;
        m_buffer = 0;
        m_bufSize = 0;
    }
}

//////////////////////////////////////////////////////////////////////////

bool CPipeClient::TryOpenConnection()
{
    static const int CONNECTION_TIMEOUT = 500;

    if (WaitNamedPipe(m_pipeName.c_str(), CONNECTION_TIMEOUT) == 0)
    {
        return false;
    }

    m_pipe = CreateFile(
            m_pipeName.c_str(),
            GENERIC_READ,
            0,
            0,
            OPEN_EXISTING,
            0,
            0);

    return m_pipe != INVALID_HANDLE_VALUE;
}

//////////////////////////////////////////////////////////////////////////

void CPipeClient::ReceiveMessage()
{
    m_message.clear();

    int msgLength = 0;
    DWORD bytesRead;
    if (!ReadFile(m_pipe, &msgLength, sizeof(msgLength), &bytesRead, 0) || bytesRead != sizeof(msgLength))
    {
        return;
    }

    DWORD bytesLeft = msgLength;

    while (bytesLeft)
    {
        if (!ReadFile(m_pipe, m_buffer, std::min<DWORD>(m_bufSize - 1, bytesLeft), &bytesRead, 0) || bytesRead == 0)
        {
            m_message.clear();
            return;
        }

        m_buffer[bytesRead] = '\0';

        m_message += m_buffer;
        bytesLeft -= bytesRead;
    }
}

//////////////////////////////////////////////////////////////////////////
