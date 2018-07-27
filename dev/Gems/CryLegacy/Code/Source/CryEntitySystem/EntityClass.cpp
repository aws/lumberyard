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

#include "CryLegacy_precompiled.h"
#include "EntityClass.h"
#include "EntityScript.h"

//////////////////////////////////////////////////////////////////////////
CEntityClass::CEntityClass()
{
    m_pfnComponentUserCreate = NULL;
    m_pComponentUserData = NULL;
    m_pPropertyHandler = NULL;
    m_pEventHandler = NULL;
    m_pScriptFileHandler = NULL;

    m_pEntityScript = NULL;
    m_bScriptLoaded = false;
}

//////////////////////////////////////////////////////////////////////////
CEntityClass::~CEntityClass()
{
    SAFE_RELEASE(m_pEntityScript);
}

//////////////////////////////////////////////////////////////////////////
IScriptTable* CEntityClass::GetScriptTable() const
{
    IScriptTable* pScriptTable = NULL;

    if (m_pEntityScript)
    {
        CEntityScript* pScript = (CEntityScript*)m_pEntityScript;
        pScriptTable = (pScript ? pScript->GetScriptTable() : NULL);
    }

    return pScriptTable;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityClass::LoadScript(bool bForceReload)
{
    bool bRes = true;
    if (m_pEntityScript)
    {
        CEntityScript* pScript = (CEntityScript*)m_pEntityScript;
        bRes = pScript->LoadScript(bForceReload);

        m_bScriptLoaded = true;
    }

    if (m_pScriptFileHandler && bForceReload)
    {
        m_pScriptFileHandler->ReloadScriptFile();
    }

    if (m_pPropertyHandler && bForceReload)
    {
        m_pPropertyHandler->RefreshProperties();
    }

    if (m_pEventHandler && bForceReload)
    {
        m_pEventHandler->RefreshEvents();
    }

    return bRes;
}

/////////////////////////////////////////////////////////////////////////
int CEntityClass::GetEventCount()
{
    if (m_pEventHandler)
    {
        return m_pEventHandler->GetEventCount();
    }

    if (!m_bScriptLoaded)
    {
        LoadScript(false);
    }

    if (!m_pEntityScript)
    {
        return 0;
    }

    return ((CEntityScript*)m_pEntityScript)->GetEventCount();
}

//////////////////////////////////////////////////////////////////////////
CEntityClass::SEventInfo CEntityClass::GetEventInfo(int nIndex)
{
    SEventInfo info;

    if (m_pEventHandler)
    {
        IEntityEventHandler::SEventInfo eventInfo;

        if (m_pEventHandler->GetEventInfo(nIndex, eventInfo))
        {
            info.name = eventInfo.name;
            info.bOutput = (eventInfo.type == IEntityEventHandler::Output);

            switch (eventInfo.valueType)
            {
            case IEntityEventHandler::Int:
                info.type = EVT_INT;
                break;
            case IEntityEventHandler::Float:
                info.type = EVT_FLOAT;
                break;
            case IEntityEventHandler::Bool:
                info.type = EVT_BOOL;
                break;
            case IEntityEventHandler::Vector:
                info.type = EVT_VECTOR;
                break;
            case IEntityEventHandler::Entity:
                info.type = EVT_ENTITY;
                break;
            case IEntityEventHandler::String:
                info.type = EVT_STRING;
                break;
            default:
                assert(0);
                break;
            }
            info.type = (EventValueType)eventInfo.valueType;
        }
        else
        {
            info.name = "";
            info.bOutput = false;
        }

        return info;
    }

    if (!m_bScriptLoaded)
    {
        LoadScript(false);
    }

    assert(nIndex >= 0 && nIndex < GetEventCount());

    if (m_pEntityScript)
    {
        const SEntityScriptEvent& scriptEvent = ((CEntityScript*)m_pEntityScript)->GetEvent(nIndex);
        info.name = scriptEvent.name.c_str();
        info.bOutput = scriptEvent.bOutput;
        info.type = scriptEvent.valueType;
    }
    else
    {
        info.name = "";
        info.bOutput = false;
    }

    return info;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityClass::FindEventInfo(const char* sEvent, SEventInfo& event)
{
    if (!m_bScriptLoaded)
    {
        LoadScript(false);
    }

    if (!m_pEntityScript)
    {
        return false;
    }

    const SEntityScriptEvent* pScriptEvent = ((CEntityScript*)m_pEntityScript)->FindEvent(sEvent);
    if (!pScriptEvent)
    {
        return false;
    }

    event.name = pScriptEvent->name.c_str();
    event.bOutput = pScriptEvent->bOutput;
    event.type = pScriptEvent->valueType;

    return true;
}

//////////////////////////////////////////////////////////////////////////
TEntityAttributeArray& CEntityClass::GetClassAttributes()
{
    return m_classAttributes;
}

//////////////////////////////////////////////////////////////////////////
const TEntityAttributeArray& CEntityClass::GetClassAttributes() const
{
    return m_classAttributes;
}

//////////////////////////////////////////////////////////////////////////
TEntityAttributeArray& CEntityClass::GetEntityAttributes()
{
    return m_entityAttributes;
}

//////////////////////////////////////////////////////////////////////////
const TEntityAttributeArray& CEntityClass::GetEntityAttributes() const
{
    return m_entityAttributes;
}

void CEntityClass::SetName(const char* sName)
{
    m_sName = sName;
}

void CEntityClass::SetScriptFile(const char* sScriptFile)
{
    m_sScriptFile = sScriptFile;
}

void CEntityClass::SetEntityScript(IEntityScript* pScript)
{
    m_pEntityScript = pScript;
}

void CEntityClass::SetComponentUserCreateFunc(ComponentUserCreateFunc pFunc, void* pUserData /*=NULL */)
{
    m_pfnComponentUserCreate = pFunc;
    m_pComponentUserData = pUserData;
}

void CEntityClass::SetPropertyHandler(IEntityPropertyHandler* pPropertyHandler)
{
    m_pPropertyHandler = pPropertyHandler;
}

void CEntityClass::SetEventHandler(IEntityEventHandler* pEventHandler)
{
    m_pEventHandler = pEventHandler;
}

void CEntityClass::SetScriptFileHandler(IEntityScriptFileHandler* pScriptFileHandler)
{
    m_pScriptFileHandler = pScriptFileHandler;
}

IEntityPropertyHandler* CEntityClass::GetPropertyHandler() const
{
    return m_pPropertyHandler;
}

IEntityEventHandler* CEntityClass::GetEventHandler() const
{
    return m_pEventHandler;
}

IEntityScriptFileHandler* CEntityClass::GetScriptFileHandler() const
{
    return m_pScriptFileHandler;
}

const SEditorClassInfo& CEntityClass::GetEditorClassInfo() const
{
    return m_EditorClassInfo;
}

void CEntityClass::SetEditorClassInfo(const SEditorClassInfo& editorClassInfo)
{
    m_EditorClassInfo = editorClassInfo;
}

//////////////////////////////////////////////////////////////////////////
void CEntityClass::SetEntityAttributes(const TEntityAttributeArray& attributes)
{
    m_entityAttributes = attributes;
}

//////////////////////////////////////////////////////////////////////////
void CEntityClass::SetClassAttributes(const TEntityAttributeArray& attributes)
{
    m_classAttributes = attributes;
}


