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

#include "StdAfx.h"
#include "LiveMocap.h"

/*

  CLiveMocap

*/

CLiveMocap::CLiveMocap()
{
    ::GetIEditor()->RegisterNotifyListener(this);
}

CLiveMocap::~CLiveMocap()
{
    ::GetIEditor()->UnregisterNotifyListener(this);
    while (!m_connections.empty())
    {
        m_connections.back()->Release();
        m_connections.pop_back();
    }
}

//

bool CLiveMocap::Initialize()
{
    return true;
}

void CLiveMocap::Update()
{
    size_t connectionCount = m_connections.size();
    for (size_t i = 0; i < connectionCount; ++i)
    {
        if (!m_connections[i]->IsConnected())
        {
            continue;
        }

        m_connections[i]->Update();
    }
}

void CLiveMocap::CreateConnection(const char* name, const char* path)
{
    CLiveMocapConnection* pConnection =
        CLiveMocapConnection::Create(name, path);
    if (!pConnection)
    {
        return;
    }

    m_connections.push_back(pConnection);
}

void CLiveMocap::AddConnection(const char* name)
{
    string connectionName;
    connectionName.Format("%s_%i", name, GetConnectionCount());
    string pathToDll;

#ifdef WIN64
    pathToDll.Format(BINFOLDER_NAME "/LiveMocap/%s.dll", name);
#else
    pathToDll.Format("Bin32/LiveMocap/%s.dll", name);
#endif

    CreateConnection(connectionName.c_str(), pathToDll.c_str());
}

// IEditorNotifyListener

void CLiveMocap::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (event == eNotify_OnIdleUpdate)
    {
        Update();
        return;
    }

    if (event == eNotify_OnCloseScene)
    {
        size_t connectionCount = m_connections.size();
        for (size_t i = 0; i < connectionCount; ++i)
        {
            m_connections[i]->Reset();
        }
    }
}

