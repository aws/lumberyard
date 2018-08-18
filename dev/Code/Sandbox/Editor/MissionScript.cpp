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
#include "MissionScript.h"
#include <IScriptSystem.h>

#define EVENT_PREFIX "Event_"

struct CMissionScriptMethodsDump
    : public IScriptTableDumpSink
{
    QStringList methods;
    QStringList events;
    void OnElementFound(int nIdx, ScriptVarType type){ /*ignore non string indexed values*/};
    virtual void OnElementFound(const char* sName, ScriptVarType type)
    {
        if (type == svtFunction)
        {
            if (strncmp(sName, EVENT_PREFIX, 6) == 0)
            {
                events.push_back(sName + 6);
            }
            else
            {
                methods.push_back(sName);
            }
        }
    }
};

CMissionScript::CMissionScript()
{
    m_sFilename = "";
}

CMissionScript::~CMissionScript()
{
}

bool CMissionScript::Load()
{
    if (m_sFilename.isEmpty())
    {
        return true;
    }

    // Parse .lua file.
    IScriptSystem* script = GetIEditor()->GetSystem()->GetIScriptSystem();
    if (!script || !script->ExecuteFile(m_sFilename.toUtf8().data(), true, true))
    {
        QString msg = QString("Unable to execute script '") + QString(GetIEditor()->GetMasterCDFolder()) + m_sFilename + "'. Check syntax ! Script not loaded.";
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, msg.toUtf8().data());
        return false;
    }
    SmartScriptTable pMission(script, true);
    if (!script->GetGlobalValue("Mission", pMission))
    {
        QString msg = "Unable to find script-table 'Mission'. Check mission script ! Mission Script not loaded.";
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, msg.toUtf8().data());
        return false;
    }
    CMissionScriptMethodsDump dump;
    pMission->Dump(&dump);
    m_methods = dump.methods;
    m_events = dump.events;

    // Sort methods and events alphabetically.
    std::sort(m_methods.begin(), m_methods.end());
    std::sort(m_events.begin(), m_events.end());
    return true;
}

void CMissionScript::Edit()
{
    if (m_sFilename.isEmpty())
    {
        return;
    }

    CFileUtil::EditTextFile(m_sFilename.toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
void CMissionScript::OnReset()
{
    IScriptSystem* pScriptSystem = GetIEditor()->GetSystem()->GetIScriptSystem();
    if (!pScriptSystem)
    {
        return;
    }

    SmartScriptTable pMission(pScriptSystem, true);
    if (!pScriptSystem->GetGlobalValue("Mission", pMission))
    {
        return;
    }

    HSCRIPTFUNCTION scriptFunction;
    if (pMission->GetValue("OnReset", scriptFunction))
    {
        Script::CallMethod(pMission, "OnReset");
    }
}

//////////////////////////////////////////////////////////////////////////
void CMissionScript::SetScriptFile(const QString& file)
{
    m_sFilename = file;
}
