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

// Description : Named pipe client for data stream from StatsTool


#ifndef CRYINCLUDE_TELEMETRYPLUGIN_PIPECLIENT_H
#define CRYINCLUDE_TELEMETRYPLUGIN_PIPECLIENT_H
#pragma once


#include "CryThread.h"

//////////////////////////////////////////////////////////////////////////

struct IPipeClientListener
{
    virtual void OnMessage(const string& msg) = 0;
};

//////////////////////////////////////////////////////////////////////////

class CPipeClient
    : public CryThread<CPipeClient>
{
public:
    static const uint32 DEFAULT_BUFFER_SIZE = 10 * 1024;

    CPipeClient();
    ~CPipeClient();

    void OpenConnection(const char* pipeName, IPipeClientListener* listener, uint32 bufferSize = DEFAULT_BUFFER_SIZE);
    void CloseConnection();
    bool isConnected();

    virtual void Run();

private:
    bool TryOpenConnection();
    void ReceiveMessage();

private:
    char* m_buffer;
    uint32 m_bufSize;

    // HACK: IsStarted() of CryThread has data races
    bool m_isRunning;
    string m_pipeName;
    HANDLE m_pipe;
    string m_message;
    IPipeClientListener* m_listener;
};

#endif // CRYINCLUDE_TELEMETRYPLUGIN_PIPECLIENT_H
