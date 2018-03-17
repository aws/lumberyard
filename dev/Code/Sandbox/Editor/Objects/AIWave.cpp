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
#include "AIWave.h"

#include "AIWavePanel.h"

#include "AI/AIManager.h"


//////////////////////////////////////////////////////////////////////////
CAIWaveObject::CAIWaveObject()
{
    m_entityClass = "AIWave";
}

void CAIWaveObject::BeginEditParams(IEditor* ie, int flags)
{
    CBaseObject::BeginEditParams(ie, flags);    // First add standard dialogs

    if (!CEntityObject::m_panel)
    {
        CEntityObject::m_panel = new CAIWavePanel;
        CEntityObject::m_rollupId = AddUIPage((QString("Entity: ") + m_entityClass).toUtf8().data(), CEntityObject::m_panel);
    }
    if (CEntityObject::m_panel && CEntityObject::m_panel->isVisible())
    {
        CEntityObject::m_panel->SetEntity(this);
        static_cast<CAIWavePanel*>(CEntityObject::m_panel)->UpdateAssignedAIsPanel();
    }

    CEntityObject::BeginEditParams(ie, flags);  // At the end add entity dialogs.
}

void CAIWaveObject::BeginEditMultiSelParams(bool bAllOfSameType)
{
    CBaseObject::BeginEditMultiSelParams(bAllOfSameType);

    if (!CEntityObject::m_panel)
    {
        CEntityObject::m_panel = new CAIWavePanel;
        CEntityObject::m_rollupId = AddUIPage(QString(m_entityClass + " and other Entities").toUtf8().data(), CEntityObject::m_panel);
    }
    if (CEntityObject::m_panel && CEntityObject::m_panel->isVisible())
    {
        static_cast<CAIWavePanel*>(CEntityObject::m_panel)->UpdateAssignedAIsPanel();
    }
}

void CAIWaveObject::EndEditMultiSelParams()
{
    if (m_rollupId != 0)
    {
        RemoveUIPage(m_rollupId);
    }
    m_rollupId = 0;
    m_panel = 0;

    CBaseObject::EndEditMultiSelParams();
}

void CAIWaveObject::SetName(const QString& newName)
{
    QString oldName = GetName();

    CEntityObject::SetName(newName);

    GetIEditor()->GetAI()->GetAISystem()->Reset(IAISystem::RESET_INTERNAL);

    GetObjectManager()->FindAndRenameProperty2("aiwave_Wave", oldName, newName);
}

#include <Objects/AIWave.moc>