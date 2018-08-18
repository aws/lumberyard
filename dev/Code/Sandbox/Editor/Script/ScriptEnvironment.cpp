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
#include "ScriptEnvironment.h"

#include "IEditor.h"
#include "Objects/EntityObject.h"
#include <AzCore/std/string/conversions.h>

EditorScriptEnvironment::EditorScriptEnvironment()
{
    GetIEditor()->RegisterNotifyListener(this);
}

EditorScriptEnvironment::~EditorScriptEnvironment()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

void EditorScriptEnvironment::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (event == eNotify_OnSelectionChange)
    {
        SelectionChanged();
    }
}

void EditorScriptEnvironment::RegisterWithScriptSystem()
{
    if (!gEnv->pScriptSystem)
    {
        return;
    }

    Init(gEnv->pScriptSystem, gEnv->pSystem);
    SetGlobalName("Editor");

    m_selection = SmartScriptTable(gEnv->pScriptSystem, false);
    m_pMethodsTable->SetValue("Selection", m_selection);

    // Editor.* functions go here
    RegisterTemplateFunction("Command", "commandName", *this, &EditorScriptEnvironment::Command);
}

void EditorScriptEnvironment::SelectionChanged()
{
    if (!m_selection)
    {
        return;
    }

    m_selection->Clear();

    CSelectionGroup* selectionGroup = GetIEditor()->GetSelection();
    int selectionCount = selectionGroup->GetCount();

    for (int i = 0, k = 0; i < selectionCount; ++i)
    {
        CBaseObject* object = selectionGroup->GetObject(i);
        if (object->GetType() == OBJTYPE_ENTITY)
        {
            CEntityObject* entity = (CEntityObject *)object;

            if (IEntity* iEntity = entity->GetIEntity())
            {
                m_selection->SetAt(++k, ScriptHandle(iEntity->GetId()));
            }
        }
    }
}

int EditorScriptEnvironment::Command(IFunctionHandler* pH, const char* commandName)
{
    GetIEditor()->GetCommandManager()->Execute(commandName);
    
    return pH->EndFunction();
}
