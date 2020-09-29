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
#pragma once

#include "LegacyTerrainCVars.h"

struct ISystem;
struct I3DEngine;
struct IObjManager;
struct IVisAreaManager;

//! Base class
class LegacyTerrainBase
{
public:
    static ISystem* GetSystem() { return m_pSystem; }
    static I3DEngine* Get3DEngine() { return m_p3DEngine; }
    static IObjManager* GetObjManager() { return m_pObjManager; }
    float GetCurTimeSec() { return m_pTimer->GetCurrTime(); }
    float GetCurAsyncTimeSec() { return m_pTimer->GetAsyncTime().GetSeconds(); }
    static IVisAreaManager* GetVisAreaManager() { return m_p3DEngine->GetIVisAreaManager(); }
    static IRenderer* GetRenderer() { return m_pRenderer; }
    static IPhysicalWorld* GetPhysicalWorld() { return m_pPhysicalWorld; }
    static ICryPak* GetPak() { return m_pCryPak; }
    static ILog* GetLog() { return m_pLog; }
    static bool IsEditor() { return m_bEditor; }
    static LegacyTerrainCVars* GetCVars() { return &m_cvars; }
    static IConsole* GetConsole() { return m_pConsole; }
    static int GetCVarAsInteger(const char* cvarName);
    static float GetCVarAsFloat(const char* cvarName);

    CRenderObject* GetIdentityCRenderObject(int nThreadID)
    {
        CRenderObject* pCRenderObject = GetRenderer()->EF_GetObject_Temp(nThreadID);
        if (!pCRenderObject)
        {
            return NULL;
        }
        pCRenderObject->m_II.m_Matrix.SetIdentity();
        return pCRenderObject;
    }

    static void SetSystem(ISystem* pSystem);

private:
    static ISystem* m_pSystem;
    static I3DEngine* m_p3DEngine;
    static IObjManager* m_pObjManager;
    static ITimer* m_pTimer;
    static IRenderer* m_pRenderer;
    static IPhysicalWorld* m_pPhysicalWorld;
    static ICryPak* m_pCryPak;
    static ILog* m_pLog;
    static IConsole* m_pConsole;

    static bool m_bEditor;
    static LegacyTerrainCVars m_cvars;
};
