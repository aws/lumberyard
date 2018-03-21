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
#include "UVMappingTool.h"
#include "Tools/DesignerTool.h"
#include "Core/Model.h"
#include "Viewport.h"
#include "Util/Undo.h"
#include "Objects/DesignerObject.h"
#include "Serialization/Decorators/EditorActionButton.h"
#include "Serialization/Math.h"
#include "Serialization/Enum.h"
#include "ToolFactory.h"
#include "Core/BrushHelper.h"

using Serialization::ActionButton;
using namespace CD;

namespace CD
{
    void STextureMappingParameter::Serialize(Serialization::IArchive& ar)
    {
#ifndef DISABLE_UVMAPPING_WINDOW
        ar(ActionButton([this] {m_pUVMappingTool->OpenUVMappingWnd();
                }), "UV Mapping Window", "UV Mapping Window");
#endif

        if (ar.OpenBlock("Mapping", "Mapping"))
        {
            ar(m_UVOffset, "UVOffset", "UV offset");
            ar(m_ScaleOffset, "ScaleOffset", "Scale offset");
            ar(Serialization::Range(m_Rotate, 0.0f, 360.0f), "Rotate", "Rotate");
            ar.CloseBlock();
        }

        if (ar.OpenBlock("Alignment", "Alignment"))
        {
            ar(m_TilingXY, "Tiling", "Tiling");
            ar(ActionButton(std::bind(&UVMappingTool::OnFitTexture, m_pUVMappingTool)), "FitTexture", "^Fit Texture");
            ar(ActionButton(std::bind(&UVMappingTool::OnReset, m_pUVMappingTool)), "Reset", "^Reset");
            ar.CloseBlock();
        }

        if (ar.OpenBlock("SubMaterial", "Sub Material"))
        {
            ar(ActionButton(std::bind(&UVMappingTool::OnSelectPolygons, m_pUVMappingTool)), "Select", "^Select");
            ar(ActionButton(std::bind(&UVMappingTool::OnAssignSubMatID, m_pUVMappingTool)), "Assign", "^Assign");
            ar.CloseBlock();
        }

        if (ar.OpenBlock("CopyPaste", "Copy/Paste"))
        {
            ar(ActionButton(std::bind(&UVMappingTool::OnCopy, m_pUVMappingTool)), "Copy", "^Copy");
            ar(ActionButton(std::bind(&UVMappingTool::OnPaste, m_pUVMappingTool)), "Paste", "^Paste");
            ar.CloseBlock();
        }
    }

    STexInfo ConvertToTexInfo(const STextureMappingParameter& texParam)
    {
        STexInfo texInfo;
        texInfo.shift[0] = texParam.m_UVOffset.x;
        texInfo.shift[1] = texParam.m_UVOffset.y;
        texInfo.scale[0] = texParam.m_ScaleOffset.x;
        texInfo.scale[1] = texParam.m_ScaleOffset.y;
        texInfo.rotate = texParam.m_Rotate;
        return texInfo;
    }

    STextureMappingParameter ConvertToTextureMappingParam(const STexInfo& texInfo)
    {
        STextureMappingParameter texParam;
        texParam.m_UVOffset.x = texInfo.shift[0];
        texParam.m_UVOffset.y = texInfo.shift[1];
        texParam.m_ScaleOffset.x = texInfo.scale[0];
        texParam.m_ScaleOffset.y = texInfo.scale[1];
        texParam.m_Rotate = texInfo.rotate;
        return texParam;
    }

    int UpdateTypeForTexture = CD::eUT_Mesh | CD::eUT_Mirror | CD::eUT_SyncPrefab;
}

void UVMappingTool::Enter()
{
    SelectTool::Enter();
    MoveSelectedElements();

    if (GetIEditor()->GetEditMode() == eEditModeRotate)
    {
        GetIEditor()->SetEditMode(eEditModeRotateCircle);
    }

    int nCurrentEditMode = GetIEditor()->GetEditMode();

    GetIEditor()->SetEditMode(eEditModeMove);
    GetIEditor()->SetReferenceCoordSys(COORDS_WORLD);
    GetIEditor()->SetEditMode(eEditModeScale);
    GetIEditor()->SetReferenceCoordSys(COORDS_WORLD);
    GetIEditor()->SetEditMode(nCurrentEditMode);

    if (GetIEditor()->GetEditMode() == eEditModeRotateCircle)
    {
        GetIEditor()->SetReferenceCoordSys(COORDS_LOCAL);
    }

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    CD::GetDesigner()->UpdateTMManipulatorBasedOnElements(pSelected);
}

void UVMappingTool::Leave()
{
    if (GetModel())
    {
        GetModel()->MoveShelf(1, 0);
        GetModel()->SetShelf(0);
        UpdateAll(CD::eUT_Mesh | CD::eUT_SyncPrefab);
    }
    SelectTool::Leave();
}

void UVMappingTool::OpenUVMappingWnd()
{
    GetIEditor()->ExecuteCommand(QStringLiteral("general.open_pane '%1'").arg(ToolName()));
}

void UVMappingTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    bool bOnlyIncludeCube = CheckVirtualKey(Qt::Key_Space);
    ElementManager pickedElements;

    CD::PolygonList polygonsOnSecondShelf;
    {
        MODEL_SHELF_RECONSTRUCTOR(GetModel());
        GetModel()->SetShelf(1);
        GetModel()->GetPolygonList(polygonsOnSecondShelf);
        GetModel()->MoveShelf(1, 0);
    }

    if (!pickedElements.Pick(GetBaseObject(), GetModel(), view, point, m_nPickFlag, bOnlyIncludeCube, NULL))
    {
        GetIEditor()->ShowTransformManipulator(false);
    }

    if (!polygonsOnSecondShelf.empty())
    {
        MODEL_SHELF_RECONSTRUCTOR(GetModel());
        for (int i = 0, iCount(polygonsOnSecondShelf.size()); i < iCount; ++i)
        {
            GetModel()->SetShelf(0);
            GetModel()->RemovePolygon(polygonsOnSecondShelf[i]);
            GetModel()->SetShelf(1);
            GetModel()->AddPolygon(polygonsOnSecondShelf[i], CD::eOpType_Add);
        }
    }

    if (!pickedElements.IsEmpty() && pickedElements[0].IsFace() && pickedElements[0].m_pPolygon)
    {
        UpdatePanel(pickedElements[0].m_pPolygon->GetTexInfo());
        CD::GetDesigner()->GetDesignerPanel()->SetSubMatID(pickedElements[0].m_pPolygon->GetMaterialID());
    }

    SelectTool::OnLButtonDown(view, nFlags, point);
}

void UVMappingTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    SelectTool::OnLButtonUp(view, nFlags, point);
    MoveSelectedElements();
}

void UVMappingTool::MoveSelectedElements()
{
    MODEL_SHELF_RECONSTRUCTOR(GetModel());
    GetModel()->MoveShelf(1, 0);
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    for (int i = 0, iCount(pSelected->GetCount()); i < iCount; ++i)
    {
        GetModel()->SetShelf(1);
        GetModel()->AddPolygon((*pSelected)[i].m_pPolygon, CD::eOpType_Add);
        GetModel()->SetShelf(0);
        GetModel()->RemovePolygon((*pSelected)[i].m_pPolygon);
    }
    UpdateAll(CD::eUT_Mesh | CD::eUT_SyncPrefab);
}

bool UVMappingTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        return CD::GetDesigner()->EndCurrentEditSession();
    }
    return true;
}

bool UVMappingTool::QueryPolygon(const BrushVec3& localRaySrc, const BrushVec3& localRayDir, int& nOutPolygonIndex, bool& bOutNew) const
{
    bOutNew = false;
    GetModel()->SetShelf(1);
    if (!GetModel()->QueryPolygon(localRaySrc, localRayDir, nOutPolygonIndex))
    {
        GetModel()->SetShelf(0);
        if (!GetModel()->QueryPolygon(localRaySrc, localRayDir, nOutPolygonIndex))
        {
            return false;
        }
        bOutNew = true;
    }
    return true;
}

void UVMappingTool::ApplyTextureInfo(const CD::STexInfo& texInfo, int nModifiedParts)
{
    CUndo undo("Change Texture Info of a designer");

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    for (int i = 0, iCount(pSelected->GetCount()); i < iCount; ++i)
    {
        if (!(*pSelected)[i].m_pPolygon)
        {
            continue;
        }
        SetTexInfoToPolygon((*pSelected)[i].m_pPolygon, texInfo, nModifiedParts);
    }
}

void UVMappingTool::FitTexture(float fTileU, float fTileV)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    for (int i = 0, iCount(pSelected->GetCount()); i < iCount; ++i)
    {
        if (!(*pSelected)[i].m_pPolygon)
        {
            continue;
        }

        const AABB& polygonAABB = (*pSelected)[i].m_pPolygon->GetBoundBox();
        CD::STexInfo texInfo;
        CD::FitTexture(ToFloatPlane((*pSelected)[i].m_pPolygon->GetPlane()), polygonAABB.min, polygonAABB.max, fTileU, fTileV, texInfo);
        SetTexInfoToPolygon((*pSelected)[i].m_pPolygon, texInfo, eTexParam_All);

        if (i == 0)
        {
            UpdatePanel(texInfo);
        }
    }
}

void UVMappingTool::SetSubMatID(int nSubMatID)
{
    CD::GetDesigner()->GetDesignerPanel()->SetSubMatID(nSubMatID + 1);
}

void UVMappingTool::SelectPolygonsByMatID(int matID)
{
    MODEL_SHELF_RECONSTRUCTOR(GetModel());

    GetModel()->MoveShelf(1, 0);
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    pSelected->Clear();

    GetModel()->SetShelf(0);
    for (int i = 0, iPolygonCount(GetModel()->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        GetModel()->SetShelf(0);
        CD::PolygonPtr pPolygon = GetModel()->GetPolygon(i);
        if (pPolygon == NULL)
        {
            continue;
        }
        if (pPolygon->GetMaterialID() == matID)
        {
            GetModel()->SetShelf(1);
            GetModel()->AddPolygon(pPolygon, CD::eOpType_Add);
            SElement element;
            element.SetFace(GetBaseObject(), pPolygon);
            pSelected->Add(element);
        }
    }

    GetModel()->SetShelf(0);
    for (int i = 0, iSelectionCount(pSelected->GetCount()); i < iSelectionCount; ++i)
    {
        GetModel()->RemovePolygon((*pSelected)[i].m_pPolygon);
    }

    CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
}

bool UVMappingTool::GetTexInfoOfSelectedPolygon(CD::STexInfo& outTexInfo) const
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->IsEmpty())
    {
        return false;
    }
    if (!(*pSelected)[0].m_pPolygon)
    {
        return false;
    }
    outTexInfo = (*pSelected)[0].m_pPolygon->GetTexInfo();
    return true;
}

void UVMappingTool::SetTexInfoToPolygon(CD::PolygonPtr pPolygon, const CD::STexInfo& texInfo, int nModifiedParts)
{
    const CD::STexInfo& polyTexInfo = pPolygon->GetTexInfo();

    CD::STexInfo newTexInfo;

    newTexInfo.shift[0] = nModifiedParts & eTexParam_Offset ? texInfo.shift[0] : polyTexInfo.shift[0];
    newTexInfo.shift[1] = nModifiedParts & eTexParam_Offset ? texInfo.shift[1] : polyTexInfo.shift[1];

    newTexInfo.scale[0] = nModifiedParts & eTexParam_Scale ? texInfo.scale[0] : polyTexInfo.scale[0];
    newTexInfo.scale[1] = nModifiedParts & eTexParam_Scale ? texInfo.scale[1] : polyTexInfo.scale[1];

    newTexInfo.rotate = nModifiedParts & eTexParam_Rotate ? texInfo.rotate : polyTexInfo.rotate;

    if (pPolygon->CheckFlags(CD::ePolyFlag_Mirrored))
    {
        CD::STexInfo mirroredTexInfo(newTexInfo);
        mirroredTexInfo.rotate = -mirroredTexInfo.rotate;
        pPolygon->SetTexInfo(mirroredTexInfo);
    }
    else
    {
        pPolygon->SetTexInfo(newTexInfo);
    }
}

void UVMappingTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginSceneSave:
        GetModel()->MoveShelf(1, 0);
        UpdateAll(CD::eUT_Mesh);
        break;
    case eNotify_OnEndSceneSave:
        MoveSelectedElements();
        break;
    }
}

void UVMappingTool::OnManipulatorDrag(CViewport* pView, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const BrushVec3& value)
{
    if (m_MouseDownContext.m_TexInfos.empty())
    {
        return;
    }

    BrushMatrix34 offsetTM;
    offsetTM = CD::GetOffsetTM(pManipulator, value, GetWorldTM());

    MODEL_SHELF_RECONSTRUCTOR(GetModel());

    int editMode = GetIEditor()->GetEditMode();
    if (editMode == eEditModeMove)
    {
        Vec3 vTranslation = offsetTM.GetTranslation();
        BrushVec3 vNormal(0, 0, 0);

        for (int i = 0, iCount(m_MouseDownContext.m_TexInfos.size()); i < iCount; ++i)
        {
            CD::PolygonPtr pPolygon(m_MouseDownContext.m_TexInfos[i].first);
            CD::STexInfo texInfo(m_MouseDownContext.m_TexInfos[i].second);

            SBrushPlane<float> p(ToVec3(pPolygon->GetPlane().Normal()), ToFloat(pPolygon->GetPlane().Distance()));
            Vec3 basis_u, basis_v;
            CD::CalcTextureBasis(p, texInfo, basis_u, basis_v);

            BrushFloat tu = (BrushFloat)basis_u.Dot(-vTranslation);
            BrushFloat tv = (BrushFloat)basis_v.Dot(-vTranslation);

            texInfo.shift[0] = texInfo.shift[0] + tu;
            texInfo.shift[1] = texInfo.shift[1] + tv;

            SetTexInfoToPolygon(pPolygon, texInfo, CD::eTexParam_Offset);
            UpdatePanel(texInfo);

            vNormal += pPolygon->GetPlane().Normal();
        }

        UpdateShelf(1);
        CD::GetDesigner()->UpdateTMManipulator(m_MouseDownContext.m_MouseDownPos + vTranslation, vNormal.GetNormalized());
    }
    else if (editMode == eEditModeRotateCircle)
    {
        for (int i = 0, iCount(m_MouseDownContext.m_TexInfos.size()); i < iCount; ++i)
        {
            CD::PolygonPtr pPolygon(m_MouseDownContext.m_TexInfos[i].first);
            CD::STexInfo texInfo(m_MouseDownContext.m_TexInfos[i].second);

            BrushVec3 tn = pPolygon->GetPlane().Normal();
            if (std::abs(tn.x) > std::abs(tn.y) && std::abs(tn.x) > std::abs(tn.z))
            {
                tn.y = tn.z = 0;
            }
            else if (std::abs(tn.y) > std::abs(tn.x) && std::abs(tn.y) > std::abs(tn.z))
            {
                tn.x = tn.z = 0;
            }
            else
            {
                tn.x = tn.y = 0;
            }

            float fDeltaRotation = (180.0F / CD::PI) * value.x;
            if (tn.y < 0 || tn.x > 0 || tn.z > 0)
            {
                fDeltaRotation = -fDeltaRotation;
            }

            texInfo.rotate -= fDeltaRotation;
            SetTexInfoToPolygon(pPolygon, texInfo, CD::eTexParam_Rotate);
            UpdatePanel(texInfo);
        }
        UpdateShelf(1);
    }
    else if (editMode == eEditModeScale)
    {
        Vec3 vScale(offsetTM.m00, offsetTM.m11, offsetTM.m22);
        for (int i = 0, iCount(m_MouseDownContext.m_TexInfos.size()); i < iCount; ++i)
        {
            CD::PolygonPtr pPolygon(m_MouseDownContext.m_TexInfos[i].first);
            CD::STexInfo texInfo(m_MouseDownContext.m_TexInfos[i].second);

            SBrushPlane<float> p(ToVec3(pPolygon->GetPlane().Normal()), ToFloat(pPolygon->GetPlane().Distance()));
            Vec3 basis_u, basis_v;
            float fBackupRotate = texInfo.rotate;
            texInfo.rotate = 0;
            CD::CalcTextureBasis(p, texInfo, basis_u, basis_v);
            texInfo.rotate = fBackupRotate;

            BrushFloat tu = std::abs((BrushFloat)basis_u.Dot(vScale));
            BrushFloat tv = std::abs((BrushFloat)basis_v.Dot(vScale));

            texInfo.scale[0] += tu - 1;
            texInfo.scale[1] += tv - 1;

            if (texInfo.scale[0] < 0.01f)
            {
                texInfo.scale[0] = 0.01f;
            }
            if (texInfo.scale[1] < 0.01f)
            {
                texInfo.scale[1] = 0.01f;
            }

            SetTexInfoToPolygon(pPolygon, texInfo, CD::eTexParam_Scale);
            UpdatePanel(texInfo);
        }
        UpdateShelf(1);
    }
}

void UVMappingTool::OnManipulatorMouseEvent(CViewport* pView, ITransformManipulator* pManipulator, EMouseEvent event, QPoint& point, int flags, bool bHitGizmo)
{
    m_bHitGizmo = bHitGizmo;
    UpdateCursor(pView, bHitGizmo);

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    std::vector<CD::PolygonPtr> polygons;
    for (int i = 0, iSelectionCount(pSelected->GetCount()); i < iSelectionCount; ++i)
    {
        if ((*pSelected)[i].IsFace() && (*pSelected)[i].m_pPolygon)
        {
            polygons.push_back((*pSelected)[i].m_pPolygon);
        }
    }
    if (polygons.empty())
    {
        return;
    }

    if (event == eMouseLDown)
    {
        m_MouseDownContext.Init();
        m_MouseDownContext.m_MouseDownPos = GetWorldTM().GetInverted().TransformPoint(ToBrushVec3(pManipulator->GetTransformation(COORDS_WORLD).GetTranslation()));
        for (int i = 0, iPolygonCount(polygons.size()); i < iPolygonCount; ++i)
        {
            m_MouseDownContext.m_TexInfos.push_back(std::pair<CD::PolygonPtr, CD::STexInfo>(polygons[i], polygons[i]->GetTexInfo()));
        }
    }

    if (event == eMouseLUp)
    {
        m_MouseDownContext.Init();
    }
}

void UVMappingTool::RecordTextureMappingUndo(const char* sUndoDescription) const
{
    if (CUndo::IsRecording() && GetBaseObject())
    {
        CUndo::Record(new CD::UndoTextureMapping(GetBaseObject(), sUndoDescription));
    }
}

void UVMappingTool::ShowGizmo()
{
    CD::GetDesigner()->UpdateTMManipulatorBasedOnElements(CD::GetDesigner()->GetSelectedElements());
}

void UVMappingTool::UpdatePanel(const CD::STexInfo& texInfo)
{
    CD::STextureMappingParameter texParam = ConvertToTextureMappingParam(texInfo);
    texParam.m_TilingXY = m_TexMappingParameter.m_TilingXY;
    texParam.m_pUVMappingTool = this;
    m_TexMappingParameter = texParam;
    m_PrevMappingParameter = m_TexMappingParameter;
    GetPanel()->Update();
}

void UVMappingTool::OnFitTexture()
{
    CUndo undo("Fit TextureUV");
    RecordTextureMappingUndo("Fit TextureUV");
    FitTexture(m_TexMappingParameter.m_TilingXY.x, m_TexMappingParameter.m_TilingXY.x);
    UpdateAll(UpdateTypeForTexture);
}

void UVMappingTool::OnReset()
{
    CD::STexInfo texInfo;
    CUndo undo("Reset TextureUV");
    RecordTextureMappingUndo("Reset TextureUV");
    ApplyTextureInfo(texInfo, eTexParam_All);
    UpdateAll(UpdateTypeForTexture);
    UpdatePanel(texInfo);
}

void UVMappingTool::OnSelectPolygons()
{
    SelectPolygonsByMatID(CD::GetDesigner()->GetCurrentSubMatID());
}

void UVMappingTool::OnAssignSubMatID()
{
    CUndo undo("Assign Material ID");
    RecordTextureMappingUndo("Assign Material ID");
    CD::AssignMatIDToSelectedPolygons(CD::GetDesigner()->GetCurrentSubMatID());
    UpdateAll(UpdateTypeForTexture);
}

void UVMappingTool::OnCopy()
{
    m_CopiedTexMappingParameter = m_TexMappingParameter;
}

void UVMappingTool::OnPaste()
{
    m_TexMappingParameter = m_CopiedTexMappingParameter;
    CD::STexInfo texInfo = ConvertToTexInfo(m_TexMappingParameter);
    ApplyTextureInfo(texInfo, eTexParam_All);
    UpdatePanel(texInfo);
    UpdateAll(UpdateTypeForTexture);
}

void UVMappingTool::OnChangeParameter(bool continuous)
{
    CD::STexInfo texInfo = ConvertToTexInfo(m_TexMappingParameter);

    int nModifiedParts = 0;

    if (!CD::IsEquivalent(m_TexMappingParameter.m_UVOffset, m_PrevMappingParameter.m_UVOffset))
    {
        nModifiedParts |= eTexParam_Offset;
    }

    if (!CD::IsEquivalent(m_TexMappingParameter.m_ScaleOffset, m_PrevMappingParameter.m_ScaleOffset))
    {
        nModifiedParts |= eTexParam_Scale;
    }

    if (m_TexMappingParameter.m_Rotate != m_PrevMappingParameter.m_Rotate)
    {
        nModifiedParts |= eTexParam_Rotate;
    }

    ApplyTextureInfo(texInfo, nModifiedParts);
    UpdateAll(UpdateTypeForTexture);

    m_PrevMappingParameter = m_TexMappingParameter;
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_UVMapping, CD::eToolGroup_Surface, "UV Mapping", UVMappingTool, PropertyTreePanel<UVMappingTool>)