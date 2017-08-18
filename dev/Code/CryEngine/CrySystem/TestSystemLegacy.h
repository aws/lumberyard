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

#pragma once
#ifndef CRYINCLUDE_CRYSYSTEM_TESTSYSTEMLEGACY_H
#define CRYINCLUDE_CRYSYSTEM_TESTSYSTEMLEGACY_H

#include "ITestSystem.h"                // ITestSystem
#include "Log.h"                                // CLog

// needed for external test application
class CTestSystemLegacy
    : public ITestSystem
{
public:
    // constructor
    CTestSystemLegacy();
    // destructor
    virtual ~CTestSystemLegacy();

    void Init(IConsole* pConsole);

    // interface ITestSystem -----------------------------------------------

    virtual void ApplicationTest(const char* szParam);
    virtual void Update();
    virtual void BeforeRender();
    virtual void AfterRender() {}
    virtual ILog* GetILog() { return m_pLog; }
    virtual void Release() { delete this; }
    virtual void SetTimeDemoInfo(STimeDemoInfo* pTimeDemoInfo);
    virtual STimeDemoInfo* GetTimeDemoInfo();
    virtual void QuitInNSeconds(const float fInNSeconds);

private: // --------------------------------------------------------------

    // TGA screenshot
    void ScreenShot(const char* szDirectory, const char* szFilename);
    //
    void LogLevelStats();
    // useful when running through a lot of tests
    void DeactivateCrashDialog();

private: // --------------------------------------------------------------

    string                      m_sParameter;                           // "" if not in test mode
    CLog*                      m_pLog;                                      //
    int                             m_iRenderPause;                     // counts down every render to delay some processing
    float                           m_fQuitInNSeconds;              // <=0 means it's deactivated

    uint32                      m_bFirstUpdate : 1;
    uint32                      m_bApplicationTest : 1;

    STimeDemoInfo*      m_pTimeDemoInfo;

    friend class CLevelListener;
};

#endif // CRYINCLUDE_CRYSYSTEM_TESTSYSTEMLEGACY_H
