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
#include "AreaSolidTool.h"
#include "Objects/AreaSolidObject.h"
#include "EditMode/ObjectMode.h"
#include "Tools/BaseTool.h"
#include "ToolFactory.h"
#include "Util/ShortcutManager.h"

namespace
{
    IViewPaneClass* g_AreaSolidToolDesc = nullptr;
}

const GUID& AreaSolidTool::GetClassID()
{
    static const GUID AREASOLID_TOOL_GUID = {
        0xdf8d2e5b, 0x597a, 0x40d4, { 0x8f, 0x1b, 0x75, 0x90, 0xae, 0x7c, 0x90, 0x71 }
    };
    return AREASOLID_TOOL_GUID;
}

void AreaSolidTool::RegisterTool(CRegistrationContext& rc)
{
    if (!g_AreaSolidToolDesc)
    {
        g_AreaSolidToolDesc = new CQtViewClass<AreaSolidTool>("EditTool.AreaSolidTool", "Brush", ESYSTEM_CLASS_EDITTOOL);
        rc.pClassFactory->RegisterClass(g_AreaSolidToolDesc);
    }
}

void AreaSolidTool::UnregisterTool(CRegistrationContext& rc)
{
    if (g_AreaSolidToolDesc)
    {
        rc.pClassFactory->UnregisterClass(g_AreaSolidToolDesc->ClassID());
        delete g_AreaSolidToolDesc;
        g_AreaSolidToolDesc = nullptr;
    }
}


void AreaSolidTool::SetUserData(const char* key, void* userData)
{
    AreaSolidObject* pAreaSolid = (AreaSolidObject*)userData;
    if (!pAreaSolid)
    {
        return;
    }

    SetBaseObject(pAreaSolid);
    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->AcceptUndo(QString("New ") + pAreaSolid->GetTypeName());
    }
}

void AreaSolidTool::BeginEditParams(IEditor* ie, int flags)
{
    DesignerTool::BeginEditParams(ie, flags);

    if (m_pMenuPanel)
    {
        DisableTool(m_pMenuPanel, CD::eDesigner_Object);
        DisableTool(m_pMenuPanel, CD::eDesigner_Separate);
        DisableTool(m_pMenuPanel, CD::eDesigner_Copy);
        DisableTool(m_pMenuPanel, CD::eDesigner_ArrayClone);
        DisableTool(m_pMenuPanel, CD::eDesigner_CircleClone);
        DisableTool(m_pMenuPanel, CD::eDesigner_UVMapping);
        DisableTool(m_pMenuPanel, CD::eDesigner_Export);
        DisableTool(m_pMenuPanel, CD::eDesigner_Boolean);
        DisableTool(m_pMenuPanel, CD::eDesigner_Bevel);
        DisableTool(m_pMenuPanel, CD::eDesigner_Magnet);
        DisableTool(m_pMenuPanel, CD::eDesigner_Lathe);
        DisableTool(m_pMenuPanel, CD::eDesigner_Bevel);
        DisableTool(m_pMenuPanel, CD::eDesigner_Subdivision);
        DisableTool(m_pMenuPanel, CD::eDesigner_SmoothingGroup);
        m_pMenuPanel->SetDesignerTool(this);
        SwitchTool(CD::eDesigner_Box);
    }

    if (m_pSettingPanel)
    {
        m_pSettingPanel->SetDesignerTool(this);
    }
}

void AreaSolidTool::EndEditParams()
{
    LeaveCurrentTool();
    DesignerTool::EndEditParams();
}

bool AreaSolidTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        if (CD::IsSelectElementMode(m_Tool))
        {
            LeaveCurrentTool();
            return false;
        }
        else if (m_Tool == CD::eDesigner_Pivot)
        {
            SwitchToSelectTool();
            return true;
        }
    }

    if (ShortcutManager::the().Process(nChar))
    {
        return true;
    }

    BaseTool* pTool = GetTool(m_Tool);
    if (pTool == NULL)
    {
        return false;
    }

    bool bContinueEdit = pTool->OnKeyDown(view, nChar, nRepCnt, nFlags);
    if (!bContinueEdit)
    {
        CD::GetDesigner()->ReleaseObjectGizmo();
    }

    return bContinueEdit;
}

#include <Tools/AreaSolidTool.moc>