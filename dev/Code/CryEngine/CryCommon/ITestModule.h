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

#ifndef CRYINCLUDE_CRYACTION_TESTSYSTEM_ITESTMODULE_H
#define CRYINCLUDE_CRYACTION_TESTSYSTEM_ITESTMODULE_H
#pragma once

typedef enum
{
    TM_NONE,
    TM_GLOBAL,
    TM_TIMEDEMO,
    TM_LAST // leave this at last
} ETestModuleType;

struct ITestModule
{
    virtual ~ITestModule(){}
    virtual void StartSession() = 0;
    virtual void StartRecording(IConsoleCmdArgs* pArgs) = 0;
    virtual void StopSession() = 0;
    virtual void PreUpdate() = 0;
    virtual void Update() = 0;
    virtual void Record(bool enable) = 0;
    virtual void Play(bool enable) = 0;
    virtual void PlayInit(IConsoleCmdArgs* pArgs) = 0;
    virtual void Pause(bool paused) = 0;
    virtual bool RecordFrame() = 0;
    virtual bool PlayFrame() = 0;
    virtual void Restart() = 0;
    virtual float RenderInfo(float y = 0) = 0;
    virtual ETestModuleType GetType() const = 0;
    virtual void ParseParams(XmlNodeRef node) = 0;
    virtual void SetVariable(const char* name, const char* szValue) = 0;
    virtual void SetVariable(const char* name, float value) = 0;
    virtual int GetNumberOfFrames() = 0;
    virtual int GetTotalPolysRecorded() = 0;
    virtual void EndLog() = 0;
    virtual void GetMemoryUsage(ICrySizer* s) const = 0;
    // common stuff
    bool m_bEnabled;
    void Enable(bool en) {m_bEnabled = en; }
    bool IsEnabled() const {return m_bEnabled; }
};

#endif // CRYINCLUDE_CRYACTION_TESTSYSTEM_ITESTMODULE_H
