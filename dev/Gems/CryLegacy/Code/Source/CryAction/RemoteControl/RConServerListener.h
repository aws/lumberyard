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

// Description : IRemoteControlServerListener implementation


#ifndef CRYINCLUDE_CRYACTION_REMOTECONTROL_RCONSERVERLISTENER_H
#define CRYINCLUDE_CRYACTION_REMOTECONTROL_RCONSERVERLISTENER_H
#pragma once

class CRConServerListener
    : public IRemoteControlServerListener
    , public IOutputPrintSink
{
public:
    static CRConServerListener& GetSingleton(IRemoteControlServer* rcon_server);
    static CRConServerListener& GetSingleton();

    void Update();

private:
    CRConServerListener();
    ~CRConServerListener();

    void OnStartResult(bool started, EResultDesc desc);

    void OnClientAuthorized(string clientAddr);

    void OnAuthorizedClientLeft(string clientAddr);

    void OnClientCommand(uint32 commandId, string command);

    string m_output;
    void Print(const char* inszText);

    typedef std::map<uint32, string> TCommandsMap;
    TCommandsMap m_commands;

    static CRConServerListener s_singleton;
    static IRemoteControlServer* s_rcon_server;
};

#endif // CRYINCLUDE_CRYACTION_REMOTECONTROL_RCONSERVERLISTENER_H

