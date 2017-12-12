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
#include "ToolCommon.h"
#include "Core/BrushHelper.h"
#include "Tools/DesignerTool.h"
#include "Objects/PrefabObject.h"
#include "Util/ElementManager.h"
#include "UIs/UICommon.h"

namespace CD
{
    bool IsCreationTool(EDesignerTool tool)
    {
        return tool == eDesigner_Box || tool == eDesigner_Sphere ||
               tool == eDesigner_Cylinder || tool == eDesigner_Cone ||
               tool == eDesigner_Rectangle || tool == eDesigner_Disc ||
               tool == eDesigner_Stair || tool == eDesigner_Line ||
               tool == eDesigner_Curve || tool == eDesigner_StairProfile;
    }

    bool IsSelectElementMode(EDesignerTool mode)
    {
        return mode == eDesigner_Vertex || mode == eDesigner_Edge || mode == eDesigner_Face ||
               mode == eDesigner_VertexEdge || mode == eDesigner_VertexFace || mode == eDesigner_EdgeFace ||
               mode == eDesigner_VertexEdgeFace;
    }

    bool IsEdgeSelectMode(EDesignerTool mode)
    {
        return mode == eDesigner_Edge ||
               mode == eDesigner_VertexEdge || mode == eDesigner_EdgeFace ||
               mode == eDesigner_VertexEdgeFace;
    }

    void GetSelectedObjectList(std::vector<SSelectedInfo>& selections)
    {
        CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
        if (pSelection == NULL)
        {
            return;
        }

        for (int i = 0, iCount(pSelection->GetCount()); i < iCount; ++i)
        {
            SSelectedInfo selectedInfo;
            CBaseObject* pObj = pSelection->GetObject(i);
            if (pObj == NULL || pObj->GetType() != OBJTYPE_SOLID && pObj->GetType() != OBJTYPE_VOLUMESOLID)
            {
                continue;
            }

            selectedInfo.m_pObj = pObj;

            GetCompiler(pObj, selectedInfo.m_pCompiler);
            GetModel(pObj, selectedInfo.m_pModel);

            selections.push_back(selectedInfo);
        }
    }

    DesignerTool* GetDesigner()
    {
        return qobject_cast<DesignerTool*>(GetIEditor()->GetEditTool());
        {
        }
    }

    void Log(const char* format, ...)
    {
        char szBuffer[MAX_WARNING_LENGTH];
        va_list args;
        va_start(args, format);
        int count = vsnprintf_s(szBuffer, sizeof(szBuffer), sizeof(szBuffer) - 1, format, args);
        szBuffer[count - 1] = '\0';
        va_end(args);
        GetIEditor()->WriteToConsole(szBuffer);
    }

    void SyncPrefab(SMainContext& mc)
    {
        if (!mc.IsValid())
        {
            return;
        }

        mc.pObject->UpdateGroup();
        mc.pObject->UpdatePrefab();

        CBaseObject* pSelected = GetIEditor()->GetSelectedObject();
        if (pSelected && pSelected != mc.pObject && pSelected->GetParent())
        {
            DESIGNER_ASSERT(qobject_cast<CPrefabObject*>(pSelected->GetParent()));
            CPrefabObject* pParent = ((CPrefabObject*)pSelected->GetParent());
            pParent->SetPrefab(pParent->GetPrefab(), true);
        }
    }

    void SyncMirror(CD::Model* pModel)
    {
        if (!pModel->CheckModeFlag(CD::eDesignerMode_Mirror))
        {
            return;
        }

        pModel->RemovePolygonsWithSpecificFlagsPlane(CD::ePolyFlag_Mirrored);

        for (int i = 0, iPolygonSize(pModel->GetPolygonCount()); i < iPolygonSize; ++i)
        {
            CD::PolygonPtr pPolygon = pModel->GetPolygon(i);
            if (pPolygon->CheckFlags(CD::ePolyFlag_Mirrored))
            {
                continue;
            }
            AddMirroredPolygon(pModel, pPolygon, CD::eOpType_Add);
        }
    }

    void RunTool(EDesignerTool tool)
    {
        if (CD::GetDesigner())
        {
            CD::GetDesigner()->SwitchTool(tool, false, false);
        }
    }

    void UpdateAll(SMainContext& mc, int updateType)
    {
        if (!mc.IsValid())
        {
            return;
        }

        bool bMirror = mc.pModel->CheckModeFlag(eDesignerMode_Mirror);

        if (updateType & eUT_Mirror)
        {
            if (bMirror)
            {
                SyncMirror(mc.pModel);
            }
        }

        if (updateType & eUT_CenterPivot)
        {
            if (gDesignerSettings.bKeepPivotCenter && !bMirror)
            {
                PivotToCenter(mc.pObject, mc.pModel);
            }
        }

        if (updateType & eUT_DataBase)
        {
            mc.pModel->ResetDB(eDBRF_ALL);
        }

        if (updateType & eUT_Mesh)
        {
            mc.pCompiler->Compile(mc.pObject, mc.pModel);
        }

        if (updateType & eUT_GameResource)
        {
            UpdateGameResource(mc.pObject);
        }

        if (updateType & eUT_SyncPrefab)
        {
            SyncPrefab(mc);
        }
    }

    void UpdateDrawnEdges(CD::SMainContext& mc)
    {
        ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

        mc.pModel->ClearExcludedEdgesInDrawing();

        for (int i = 0, iElementSize(pSelected->GetCount()); i < iElementSize; ++i)
        {
            const SElement& element = pSelected->Get(i);
            if (element.IsEdge())
            {
                DESIGNER_ASSERT(element.m_Vertices.size() == 2);
                mc.pModel->AddExcludedEdgeInDrawing(BrushEdge3D(element.m_Vertices[0], element.m_Vertices[1]));
            }
            else if (element.IsFace() && element.m_pPolygon->IsOpen())
            {
                for (int a = 0, iEdgeCount(element.m_pPolygon->GetEdgeCount()); a < iEdgeCount; ++a)
                {
                    BrushEdge3D e = element.m_pPolygon->GetEdge(a);
                    mc.pModel->AddExcludedEdgeInDrawing(e);
                }
            }
        }
    }

    BrushMatrix34 GetOffsetTM(ITransformManipulator* pManipulator, const BrushVec3& vOffset, const BrushMatrix34& worldTM)
    {
        RefCoordSys coordSys = GetIEditor()->GetReferenceCoordSys();
        int editMode = GetIEditor()->GetEditMode();

        BrushMatrix34 worldRefTM(ToBrushMatrix34(pManipulator->GetTransformation(coordSys)));
        BrushMatrix34 invWorldTM = worldTM.GetInverted();
        BrushMatrix34 modRefFrame = invWorldTM * worldRefTM;
        BrushMatrix34 modRefFrameInverse = worldRefTM.GetInverted() * worldTM;

        if (editMode == eEditModeMove)
        {
            return modRefFrame * BrushMatrix34::CreateTranslationMat(worldRefTM.GetInverted().TransformVector(vOffset)) * modRefFrameInverse;
        }
        else if (editMode == eEditModeRotate)
        {
            return modRefFrame * BrushMatrix34::CreateRotationXYZ(Ang3_tpl<BrushFloat>(-vOffset)) * modRefFrameInverse;
        }
        else if (editMode == eEditModeScale)
        {
            return modRefFrame * BrushMatrix34::CreateScale(vOffset) * modRefFrameInverse;
        }

        return BrushMatrix34::CreateIdentity();
    }

    void MessageBox(const QString& title, const QString& msg)
    {
        QMessageBox::warning(QApplication::activeWindow(), title, msg);
    }
}