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

#ifndef CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAP_H
#define CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAP_H
#pragma once


#include "../../SDKs/LiveMocap/LiveMocap.h"
#include "../LiveMocap/LiveMocapConnection.h"

class CLiveMocap
    : public IEditorNotifyListener
{
public:
    static CLiveMocap& Instance()
    {
        static CLiveMocap instance;

        // TEMP
        static bool bOnce = true;
        if (bOnce)
        {
            bOnce = !instance.Initialize();
        }

        return instance;
    }

private:
    CLiveMocap();
    ~CLiveMocap();

public:
    bool Initialize();
    void Update();

    uint32 GetConnectionCount() { return (uint32)m_connections.size(); }
    CLiveMocapConnection* GetConnection(uint32 index) { return m_connections[index]; }
    void AddConnection(const char* name);

private:
    void CreateConnection(const char* name, const char* path);

    // IEditorNotifyListener
public:
    void OnEditorNotifyEvent(EEditorNotifyEvent event);

private:
    std::vector<CLiveMocapConnection*> m_connections;
};

#endif // CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAP_H
