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


#include "CryLegacy_precompiled.h"
#include "IRemoteControl.h"
#include "RConServerListener.h"

CRConServerListener CRConServerListener::s_singleton;

IRemoteControlServer* CRConServerListener::s_rcon_server = NULL;

CRConServerListener& CRConServerListener::GetSingleton(IRemoteControlServer* rcon_server)
{
    s_rcon_server = rcon_server;

    return s_singleton;
}

CRConServerListener& CRConServerListener::GetSingleton()
{
    return s_singleton;
}

CRConServerListener::CRConServerListener()
{
}

CRConServerListener::~CRConServerListener()
{
}

void CRConServerListener::Update()
{
    for (TCommandsMap::const_iterator it = m_commands.begin(); it != m_commands.end(); ++it)
    {
        gEnv->pConsole->AddOutputPrintSink(this);
#if defined(CVARS_WHITELIST)
        ICVarsWhitelist* pCVarsWhitelist = gEnv->pSystem->GetCVarsWhiteList();
        bool execute = (pCVarsWhitelist) ? pCVarsWhitelist->IsWhiteListed(it->second.c_str(), false) : true;
        if (execute)
#endif // defined(CVARS_WHITELIST)
        {
            gEnv->pConsole->ExecuteString(it->second.c_str());
        }
        gEnv->pConsole->RemoveOutputPrintSink(this);

        s_rcon_server->SendResult(it->first, m_output);
        m_output.resize(0);
    }

    m_commands.clear();
}

void CRConServerListener::Print(const char* inszText)
{
    m_output += string().Format("%s\n", inszText);
}

void CRConServerListener::OnStartResult(bool started, EResultDesc desc)
{
    if (started)
    {
        gEnv->pLog->LogToConsole("RCON: server successfully started");
    }
    else
    {
        string sdesc;
        switch (desc)
        {
        case eRD_Failed:
            sdesc = "failed starting server";
            break;

        case eRD_AlreadyStarted:
            sdesc = "server already started";
            break;
        }
        gEnv->pLog->LogToConsole("RCON: %s", sdesc.c_str());

        gEnv->pConsole->ExecuteString("rcon_stopserver");
    }
}

void CRConServerListener::OnClientAuthorized(string clientAddr)
{
    gEnv->pLog->LogToConsole("RCON: client from %s is authorized", clientAddr.c_str());
}

void CRConServerListener::OnAuthorizedClientLeft(string clientAddr)
{
    gEnv->pLog->LogToConsole("RCON: client from %s is gone", clientAddr.c_str());
}

void CRConServerListener::OnClientCommand(uint32 commandId, string command)
{
    // execute the command on the server
    gEnv->pLog->LogToConsole("RCON: received remote control command: [%08x]%s", commandId, command.c_str());

    m_commands[commandId] = command;
}

