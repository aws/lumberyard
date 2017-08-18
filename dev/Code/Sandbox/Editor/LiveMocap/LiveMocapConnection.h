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

#ifndef CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAPCONNECTION_H
#define CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAPCONNECTION_H
#pragma once


#include "../../SDKs/LiveMocap/LiveMocap.h"
#include "LiveMocapScene.h"

class CLiveMocapConnection
{
public:
    static CLiveMocapConnection* Create(const char* name, const char* path);

private:
    CLiveMocapConnection();
    ~CLiveMocapConnection();

public:
    void Release() { delete this; }

    const char* GetName() { return m_name; }

    CLiveMocapScene& GetScene() { return m_scene; }

    bool Connect(const char* address);
    void Disconnect();

    bool IsConnected();

    void Reset();

    void Update();

private:
    string m_name;
    HMODULE m_hModule;
    LMLiveMocap* m_pLiveMocap;

    CLiveMocapScene m_scene;

    bool m_bConnected; // TEMP

public:
    string m_lastConnection;
};

#endif // CRYINCLUDE_EDITOR_LIVEMOCAP_LIVEMOCAPCONNECTION_H
