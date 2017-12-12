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
#include "ClipVolumeTool.h"
#include "EditMode/ObjectMode.h"
#include "Core/BrushHelper.h"
#include "Tools/BaseTool.h"
#include "ToolFactory.h"
#include "Util/ShortcutManager.h"

namespace
{
    IViewPaneClass* g_clipVolumeClassDesc = nullptr;
}

const GUID& ClipVolumeTool::GetClassID()
{
    static const GUID CLIPVOLUME_TOOL_GUID =
    {
        0xffa00a8, 0x476, 0x4398, { 0x99, 0x38, 0x7a, 0xec, 0x7c, 0x73, 0x6b, 0xeb }
    };
    return CLIPVOLUME_TOOL_GUID;
}

void ClipVolumeTool::RegisterTool(CRegistrationContext& rc)
{
    if (!g_clipVolumeClassDesc)
    {
        g_clipVolumeClassDesc = new CQtViewClass<ClipVolumeTool>("EditTool.ClipVolumeTool", "Brush", ESYSTEM_CLASS_EDITTOOL);
        rc.pClassFactory->RegisterClass(g_clipVolumeClassDesc);
    }
}

void ClipVolumeTool::UnregisterTool(CRegistrationContext& rc)
{
    if (g_clipVolumeClassDesc)
    {
        rc.pClassFactory->UnregisterClass(g_clipVolumeClassDesc->ClassID());
        delete g_clipVolumeClassDesc;
        g_clipVolumeClassDesc = nullptr;
    }
}

void ClipVolumeTool::SetUserData(const char* key, void* userData)
{
    CBaseObject* pClipVolume = (CBaseObject*)userData;
    if (!pClipVolume)
    {
        return;
    }

    SetBaseObject(pClipVolume);

    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->AcceptUndo(QString("New ") + pClipVolume->GetTypeName());
    }
}

void ClipVolumeTool::BeginEditParams(IEditor* ie, int flags)
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

void ClipVolumeTool::EndEditParams()
{
    LeaveCurrentTool();
    DesignerTool::EndEditParams();
}

bool ClipVolumeTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
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

void ClipVolumeTool::OnManipulatorMouseEvent(CViewport* view, ITransformManipulator* pManipulator, EMouseEvent event, QPoint& point, int flags, bool bHitGizmo)
{
    DesignerTool::OnManipulatorMouseEvent(view, pManipulator, event, point, flags, bHitGizmo);

    if (BaseTool* pTool = GetCurrentTool())
    {
        if (event == eMouseLUp)
        {
            CD::UpdateGameResource(m_pBaseObject);
        }
    }
}

#include <Tools/ClipVolumeTool.moc>