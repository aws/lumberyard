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
#include "LiveMocapConnection.h"

//

ILINE void LogWindowsLastError()
{
    DWORD errorEnum = ::GetLastError();
    LPTSTR errorString = 0;
    const char* errorMessage = "Failed to get error string.";
    if (::FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            errorEnum,
            0,
            (LPTSTR)&errorString,
            0,
            NULL))
    {
        errorMessage = errorString;
    }

    CryLog("LiveMocap Error(%x): %s\n", errorEnum, errorMessage);

    if (errorString)
    {
        ::LocalFree(errorString);
    }
}

/*

  CLiveMocapSystem

*/

class CLiveMocapSystem
    : public ILMSystem
{
    // ILMSystem
public:
    void Log(LMString message, ...)
    {
        char buffer[2048];

        va_list arguments;
        va_start(arguments, message);
        ::vsnprintf(buffer, sizeof(buffer), message, arguments);
        va_end(arguments);

        ::gEnv->pLog->Log(buffer);
    }
} liveMocapSystem;

/*

  CLiveMocapConnection

*/

CLiveMocapConnection* CLiveMocapConnection::Create(const char* name, const char* path)
{
    HMODULE hModule = ::LoadLibraryA(path);
    if (!hModule)
    {
        LogWindowsLastError();
        return NULL;
    }

    FLiveMocap LiveMocap = (FLiveMocap)
            ::GetProcAddress(hModule, "LiveMocap");
    if (!LiveMocap)
    {
        ::FreeLibrary(hModule);
        LogWindowsLastError();
        return NULL;
    }

    LMLiveMocap* pLiveMocap = LiveMocap(&::liveMocapSystem);
    if (!pLiveMocap)
    {
        ::FreeLibrary(hModule);
        LogWindowsLastError();
        return NULL;
    }

    CLiveMocapConnection* pConnection = new CLiveMocapConnection();
    pConnection->m_name = name;
    pConnection->m_hModule = hModule;
    pConnection->m_pLiveMocap = pLiveMocap;
    return pConnection;
}

//

CLiveMocapConnection::CLiveMocapConnection()
{
    m_hModule = NULL;
    m_pLiveMocap = NULL;

    m_bConnected = false;

    m_lastConnection = "Type the connection address...";
}

CLiveMocapConnection::~CLiveMocapConnection()
{
    if (m_pLiveMocap)
    {
        m_pLiveMocap->Release();
    }

    if (m_hModule)
    {
        ::FreeLibrary(m_hModule);
    }
}

//

bool CLiveMocapConnection::Connect(const char* address)
{
    if (IsConnected())
    {
        return true;
    }

    return m_bConnected = m_pLiveMocap->Connect(address, m_scene);
}

void CLiveMocapConnection::Disconnect()
{
    if (!IsConnected())
    {
        return;
    }

    m_pLiveMocap->Disconnect();
    IEntity* pEntity = m_scene.GetEntity();
    //  m_scene.Reset();
    m_scene.SetEntity(pEntity);
    m_bConnected = false;
}

bool CLiveMocapConnection::IsConnected()
{
    return m_bConnected;
}

void CLiveMocapConnection::Reset()
{
    m_scene.Reset();
}

void CLiveMocapConnection::Update()
{
    m_pLiveMocap->Update();
    m_scene.Update();
}
