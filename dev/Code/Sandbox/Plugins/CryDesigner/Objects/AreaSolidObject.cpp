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
#include "AreaSolidObject.h"
#include <Objects/ui_AreaSolidPanel.h>
#include "../Viewport.h"
#include <IEntitySystem.h>
#include "SurfaceInfoPicker.h"
#include "Core/Model.h"
#include "Core/PolygonDecomposer.h"
#include "Core/BrushHelper.h"
#include "Tools/AreaSolidTool.h"
#include "Controls/ToolButton.h"
#include "ToolFactory.h"
#include "Core/BrushHelper.h"
#include <IEntityHelper.h>
#include "Components/IComponentArea.h"

#include <QScopedPointer>
#include <QWidget>

class AreaSolidPanel
    : public QWidget
{
public:
    AreaSolidPanel(AreaSolidObject* pSolid, QWidget* pParent = nullptr)
        : QWidget(pParent)
        , ui(new Ui::AreaSolidPanel)
    {
        m_pAreaSolid = pSolid;

        ui->setupUi(this);

        if (m_pAreaSolid)
        {
            ui->EDIT_AREASOLID->SetToolClass(&AreaSolidTool::staticMetaObject, "object", m_pAreaSolid);
        }
    }

    void GotoEditModeAsync()
    {
        QMetaObject::invokeMethod(ui->EDIT_AREASOLID, "clicked", Qt::QueuedConnection);
    }

protected:
    AreaSolidObject* m_pAreaSolid;
    QScopedPointer<Ui::AreaSolidPanel> ui;
};

int AreaSolidObject::s_nGlobalFileAreaSolidID = 0;

AreaSolidObject::AreaSolidObject()
{
    m_nActivateEditAreaSolidRollUpID = 0;
    m_pActivateEditAreaSolidPanel = NULL;
    m_pCompiler = new CD::ModelCompiler(0);
    m_areaId = -1;
    m_edgeWidth = 0;
    mv_groupId = 0;
    mv_priority = 0;
    m_entityClass = "AreaSolid";
    m_bAreaModified = false;
    m_nUniqFileIdForExport = (s_nGlobalFileAreaSolidID++);

    GetModel()->SetModeFlag(CD::eDesignerMode_FrameRemainAfterDrill);

    CBaseObject::SetMaterial("Editor/Materials/areasolid");
    m_pSizer = GetIEditor()->GetSystem()->CreateSizer();
    SetColor(RGB(0, 0, 255));
}

AreaSolidObject::~AreaSolidObject()
{
    m_pSizer->Release();
}

bool AreaSolidObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    bool res = CEntityObject::Init(ie, prev, file);

    if (m_pEntity)
    {
        m_pEntity->GetOrCreateComponent<IComponentArea>();
    }

    if (prev)
    {
        AreaSolidObject* pAreaSolid = static_cast<AreaSolidObject*>(prev);
        CD::ModelCompiler* pCompiler = pAreaSolid->GetCompiler();
        CD::Model* pModel = pAreaSolid->GetModel();
        if (pCompiler && pModel)
        {
            SetCompiler(new CD::ModelCompiler(*pCompiler));
            SetModel(new CD::Model(*pModel));
            CD::UpdateAll(MainContext(), CD::eUT_Mesh);
        }
    }

    return res;
}

void AreaSolidObject::InitVariables()
{
    AddVariable(m_areaId, "AreaId", functor(*this, &AreaSolidObject::OnAreaChange));
    AddVariable(m_edgeWidth, "FadeInZone", functor(*this, &AreaSolidObject::OnSizeChange));
    AddVariable(mv_groupId, "GroupId", functor(*this, &AreaSolidObject::OnAreaChange));
    AddVariable(mv_priority, "Priority", functor(*this, &AreaSolidObject::OnAreaChange));
    AddVariable(mv_filled, "Filled", functor(*this, &AreaSolidObject::OnAreaChange));
}

bool AreaSolidObject::CreateGameObject()
{
    bool bRes = DesignerBaseObject<CAreaObjectBase>::CreateGameObject();
    if (bRes)
    {
        UpdateGameArea();
    }
    return bRes;
}

bool AreaSolidObject::HitTest(HitContext& hc)
{
    if (!GetCompiler())
    {
        return false;
    }

    Vec3 pnt;
    Vec3 localRaySrc;
    Vec3 localRayDir;
    CSurfaceInfoPicker::RayWorldToLocal(GetWorldTM(), hc.raySrc, hc.rayDir, localRaySrc, localRayDir);
    bool bHit = false;

    if (Intersect::Ray_AABB(localRaySrc, localRayDir, GetModel()->GetBoundBox(), pnt))
    {
        Vec3 prevSrc = hc.raySrc;
        Vec3 prevDir = hc.rayDir;
        hc.raySrc = localRaySrc;
        hc.rayDir = localRayDir;

        if (GetCompiler())
        {
            bHit = CD::HitTest(MainContext(), hc);
        }

        hc.raySrc = prevSrc;
        hc.rayDir = prevDir;

        Vec3 localHitPos = localRaySrc + localRayDir * hc.dist;
        Vec3 worldHitPos = GetWorldTM().TransformPoint(localHitPos);
        hc.dist = (worldHitPos - hc.raySrc).len();
    }

    if (bHit)
    {
        hc.object = this;
    }

    return bHit;
}

void AreaSolidObject::GetBoundBox(AABB& box)
{
    if (GetCompiler())
    {
        box.SetTransformedAABB(GetWorldTM(), GetModel()->GetBoundBox());
    }
    else
    {
        box = AABB(Vec3(0, 0, 0), Vec3(0, 0, 0));
    }
}

void AreaSolidObject::GetLocalBounds(AABB& box)
{
    if (GetCompiler())
    {
        box = GetModel()->GetBoundBox();
    }
    else
    {
        box = AABB(Vec3(0, 0, 0), Vec3(0, 0, 0));
    }
}

void AreaSolidObject::BeginEditParams(IEditor* ie, int flags)
{
    if (!m_pActivateEditAreaSolidPanel)
    {
        m_pActivateEditAreaSolidPanel = new AreaSolidPanel(this);
        m_nActivateEditAreaSolidRollUpID = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, tr("Activate Edit Tool"), m_pActivateEditAreaSolidPanel, false);
        if (GetModel()->IsEmpty())
        {
            m_pActivateEditAreaSolidPanel->GotoEditModeAsync();
        }
    }

    DesignerBaseObject<CAreaObjectBase>::BeginEditParams(ie, flags);
}

void AreaSolidObject::EndEditParams(IEditor* ie)
{
    DesignerBaseObject<CAreaObjectBase>::EndEditParams(ie);

    if (m_nActivateEditAreaSolidRollUpID)
    {
        ie->RemoveRollUpPage(ROLLUP_OBJECTS, m_nActivateEditAreaSolidRollUpID);
    }
    m_nActivateEditAreaSolidRollUpID = 0;
    m_pActivateEditAreaSolidPanel = NULL;
}

void AreaSolidObject::Serialize(CObjectArchive& ar)
{
    DesignerBaseObject<CAreaObjectBase>::Serialize(ar);

    XmlNodeRef xmlNode = ar.node;

    if (ar.bLoading)
    {
        XmlNodeRef brushNode = xmlNode->findChild("Brush");

        if (!GetModel())
        {
            SetModel(new CD::Model);
        }

        GetModel()->SetModeFlag(CD::eDesignerMode_FrameRemainAfterDrill);

        if (brushNode)
        {
            GetModel()->Serialize(brushNode, ar.bLoading, ar.bUndo);
        }

        if (ar.bUndo)
        {
            if (GetCompiler())
            {
                GetCompiler()->DeleteAllRenderNodes();
            }
        }

        SetAreaId(m_areaId);

        CD::UpdateAll(MainContext());
    }
    else if (GetCompiler())
    {
        XmlNodeRef brushNode = xmlNode->newChild("Brush");
        GetModel()->Serialize(brushNode, ar.bLoading, ar.bUndo);
    }
}

XmlNodeRef AreaSolidObject::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    XmlNodeRef objNode = DesignerBaseObject<CAreaObjectBase>::Export(levelPath, xmlNode);
    XmlNodeRef areaNode = objNode->findChild("Area");
    if (areaNode)
    {
        QString gameFilename;
        GenerateGameFilename(gameFilename);
        areaNode->setAttr("AreaSolidFileName", static_cast<const char*>(gameFilename.toUtf8()));
    }
    return objNode;
}

void AreaSolidObject::SetAreaId(int nAreaId)
{
    m_areaId = nAreaId;
    UpdateGameArea();
}

void AreaSolidObject::UpdateGameArea()
{
    if (!m_pEntity)
    {
        return;
    }

    IComponentAreaPtr pArea = m_pEntity->GetOrCreateComponent<IComponentArea>();
    if (!pArea)
    {
        return;
    }

    pArea->SetProximity(m_edgeWidth);
    pArea->SetID(m_areaId);
    pArea->SetGroup(mv_groupId);
    pArea->SetPriority(mv_priority);

    pArea->ClearEntities();
    for (int i = 0; i < GetEntityCount(); i++)
    {
        CEntityObject* pEntity = GetEntity(i);
        if (pEntity && pEntity->GetIEntity())
        {
            pArea->AddEntity(pEntity->GetIEntity()->GetId());
        }
    }

    if (GetCompiler() == NULL)
    {
        return;
    }

    CD::Model* pModel(GetModel());
    const float MinimumAreaSolidRadius = 0.1f;
    if (GetModel()->GetBoundBox().GetRadius() <= MinimumAreaSolidRadius)
    {
        return;
    }

    MODEL_SHELF_RECONSTRUCTOR(GetModel());
    GetModel()->SetShelf(0);

    std::vector<CD::PolygonPtr> polygonList;
    pModel->GetPolygonList(polygonList);

    pArea->BeginSettingSolid(GetWorldTM());
    for (int i = 0, iPolygonSize(polygonList.size()); i < iPolygonSize; ++i)
    {
        CD::FlexibleMesh mesh;
        PolygonDecomposer decomposer;
        decomposer.TriangulatePolygon(polygonList[i], mesh);

        std::vector<std::vector<Vec3> > faces;
        CD::ConvertMeshFacesToFaces(mesh.vertexList, mesh.faceList, faces);

        AddConvexhullToEngineArea(pArea.get(), faces, true);
        std::vector<CD::PolygonPtr> innerPolygons;
        polygonList[i]->GetSeparatedPolygons(innerPolygons, CD::eSP_InnerHull);
        for (int k = 0, iSize(innerPolygons.size()); k < iSize; ++k)
        {
            innerPolygons[k]->ReverseEdges();
            if (pModel->QueryEquivalentPolygon(innerPolygons[k]))
            {
                continue;
            }

            mesh.Clear();
            faces.clear();

            PolygonDecomposer decomposer;
            decomposer.TriangulatePolygon(innerPolygons[k], mesh);
            CD::ConvertMeshFacesToFaces(mesh.vertexList, mesh.faceList, faces);

            AddConvexhullToEngineArea(pArea.get(), faces, false);
        }
    }
    pArea->EndSettingSolid();

    m_pSizer->Reset();
    pArea->GetMemoryUsage(m_pSizer);
}

void AreaSolidObject::AddConvexhullToEngineArea(IComponentArea* pArea, std::vector<std::vector<Vec3> >& faces, bool bObstructrion)
{
    for (int a = 0, iFaceSize(faces.size()); a < iFaceSize; ++a)
    {
        pArea->AddConvexHullToSolid(&faces[a][0], bObstructrion, faces[a].size());
    }
}

void AreaSolidObject::InvalidateTM(int nWhyFlags)
{
    DesignerBaseObject<CAreaObjectBase>::InvalidateTM(nWhyFlags);
    UpdateEngineNode();
    UpdateGameArea();
}

void AreaSolidObject::Display(DisplayContext& dc)
{
    DrawDefault(dc);

    if (!GetCompiler())
    {
        return;
    }

    for (int i = 0, iCount(GetEntityCount()); i < GetEntityCount(); i++)
    {
        CEntityObject* pEntity = GetEntity(i);
        if (!pEntity || !pEntity->GetIEntity())
        {
            continue;
        }

        BrushVec3 vNearestPos;
        if (!QueryNearestPos(pEntity->GetWorldPos(), vNearestPos))
        {
            continue;
        }

        dc.DrawLine(pEntity->GetWorldPos(), CD::ToVec3(vNearestPos), ColorF(1.0f, 1.0f, 1.0f, 0.7f), ColorF(1.0f, 1.0f, 1.0f, 0.7f));
    }

    DisplayMemoryUsage(dc);
}

void AreaSolidObject::DisplayMemoryUsage(DisplayContext& dc)
{
    if (IsEmpty() || !CD::GetDesigner())
    {
        return;
    }

    AABB aabb;
    GetBoundBox(aabb);

    QString sizeBuffer;
    sizeBuffer = tr("Size : %1 KB").arg(m_pSizer->GetTotalSize() * 0.001f, 0, 'f', 4);
    DrawTextOn2DBox(dc, aabb.GetCenter(), sizeBuffer.toUtf8().data(), 1.4f, ColorF(1, 1, 1, 1), ColorF(0, 0, 0, 0.25f));
}

void AreaSolidObject::GenerateGameFilename(QString& generatedFileName) const
{
    generatedFileName = QString("%level%/Brush/areasolid%1").arg(m_nUniqFileIdForExport);
}

void AreaSolidObject::SetMaterial(CMaterial* mtl)
{
    CBaseObject::SetMaterial(mtl);
    UpdateEngineNode();
}

void AreaSolidObject::OnEvent(ObjectEvent event)
{
    DesignerBaseObject<CAreaObjectBase>::OnEvent(event);

    switch (event)
    {
    case EVENT_INGAME:
    {
        if (GetCompiler())
        {
            GetCompiler()->SetRenderFlags(GetCompiler()->GetRenderFlags() | ERF_HIDDEN);
        }
        if (CD::GetDesigner())
        {
            CD::GetDesigner()->LeaveCurrentTool();
        }
    }
    break;

    case EVENT_OUTOFGAME:
        if (GetCompiler())
        {
            GetCompiler()->SetRenderFlags(GetCompiler()->GetRenderFlags() & (~ERF_HIDDEN));
        }
        break;
    }
}

CD::ModelCompiler* AreaSolidObject::GetCompiler() const
{
    if (!m_pCompiler)
    {
        m_pCompiler = new CD::ModelCompiler(0);
    }
    return m_pCompiler;
}

#include <Objects/AreaSolidObject.moc>