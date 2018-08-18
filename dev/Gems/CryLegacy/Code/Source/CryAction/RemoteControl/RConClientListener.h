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

// Description : IRemoteControlClientListener implementation

#ifndef CRYINCLUDE_CRYACTION_REMOTECONTROL_RCONCLIENTLISTENER_H
#define CRYINCLUDE_CRYACTION_REMOTECONTROL_RCONCLIENTLISTENER_H
#pragma once

#include <IRemoteControl.h>


class CRConClientListener
    : public IRemoteControlClientListener
{
public:
    static CRConClientListener& GetSingleton(IRemoteControlClient* rcon_client);

    void OnConnectResult(bool okay, EResultDesc desc);

    void OnSessionStatus(bool connected, EStatusDesc desc);

    void OnCommandResult(uint32 commandId, string command, string result);

    bool IsSessionAuthorized() const;

private:
    CRConClientListener();
    ~CRConClientListener();

    bool m_sessionAuthorized;

    static CRConClientListener s_singleton;

    static IRemoteControlClient* s_rcon_client;
};

#endif // CRYINCLUDE_CRYACTION_REMOTECONTROL_RCONCLIENTLISTENER_H

