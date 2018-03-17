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
#include "DesignerObject.h"
#include "Core/ModelCompiler.h"
#include "Material/Material.h"
#include "Material/MaterialManager.h"
#include "Tools/DesignerTool.h"
#include "Util/Converter.h"
#include "ObjectCreateTool.h"
#include "IGizmoManager.h"
#include "Base64.h"
#include "GameEngine.h"
#include "ToolFactory.h"
#include "Core/BrushHelper.h"

DesignerObject::DesignerObject()
{
    m_pCompiler = new CD::ModelCompiler(CD::eCompiler_General);
    m_nBrushUniqFileId = CD::s_nGlobalBrushDesignerFileId;
    m_EngineFlags.m_pObj = this;
    ++CD::s_nGlobalBrushDesignerFileId;
}

bool DesignerObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    SetMaterial(CD::BrushDesignerDefaultMaterial);

    if (prev)
    {
        DesignerObject* pGeomObj = static_cast<DesignerObject*>(prev);

        CD::Model* pModel = pGeomObj->GetModel();
        if (pModel)
        {
            CD::Model* pClonedModel = new CD::Model(*pModel);
            SetModel(pClonedModel);
        }

        CD::ModelCompiler* pFrontBrush = pGeomObj->GetCompiler();
        if (pFrontBrush)
        {
            CD::ModelCompiler* pClonedBrush = new CD::ModelCompiler(*pFrontBrush);
            SetCompiler(pClonedBrush);
            CD::UpdateAll(MainContext(), CD::eUT_Mesh);
        }
    }

    return CBaseObject::Init(ie, prev, file);
}

void DesignerObject::Display(DisplayContext& dc)
{
    CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
    bool bLastSelected = pSelection && pSelection->GetCount() > 0 && this == pSelection->GetObject(pSelection->GetCount() - 1);

    bool bDisplay2D = dc.flags & DISPLAY_2D;
    if (bDisplay2D)
    {
        if (!bLastSelected)
        {
            dc.PushMatrix(GetWorldTM());
            GetModel()->Display(dc);
            AABB bbox;
            GetBoundBox(bbox);
            if (!(dc.flags & DISPLAY_HIDENAMES))
            {
                dc.DrawTextLabel(bbox.GetCenter(), 1.5, GetName().toUtf8().data());
            }
            dc.PopMatrix();
        }
    }
    else
    {
        DrawDefault(dc);
    }

    if (bLastSelected)
    {
        AABB worldAABB;
        GetBoundBox(worldAABB);
        dc.SetColor(CD::kSelectedColor);
        dc.SetDrawInFrontMode(true);
        dc.DrawWireBox(worldAABB.min, worldAABB.max);
        if (!(dc.flags & DISPLAY_HIDENAMES))
        {
            dc.DrawTextLabel(worldAABB.GetCenter(), 1.5, GetName().toUtf8().data());
        }
        dc.SetDrawInFrontMode(false);
    }

    if (IsHighlighted() && GetCompiler() && gSettings.viewports.bHighlightMouseOverGeometry)
    {
        if (!gDesignerSettings.bExclusiveMode)
        {
            _smart_ptr<IStatObj> pStatObj = NULL;
            if (GetCompiler()->GetIStatObj(&pStatObj))
            {
                SGeometryDebugDrawInfo dd;
                dd.tm = GetWorldTM();
                dd.tm = dc.GetView()->GetScreenTM() * dd.tm;
                dd.color = ColorB(250, 0, 250, 30);
                dd.lineColor = ColorB(255, 255, 0, 160);
                dd.bExtrude = true;
                pStatObj->DebugDraw(dd);
            }
        }
    }

    if (!IsSelected())
    {
        dc.PushMatrix(GetWorldTM());

        ColorB oldColor = dc.GetColor();
        int oldLineWidth = dc.GetLineWidth();

        dc.SetColor(ColorB(0, 0, 2));
        dc.SetLineWidth(2);
        for (int i = 0, iPolygonCount(GetModel()->GetPolygonCount()); i < iPolygonCount; ++i)
        {
            CD::PolygonPtr pPolygon = GetModel()->GetPolygon(i);
            if (!pPolygon->IsOpen())
            {
                continue;
            }
            pPolygon->Display(dc);
        }
        dc.SetColor(oldColor);
        dc.SetLineWidth(oldLineWidth);

        dc.PopMatrix();
    }
}

void DesignerObject::GetBoundBox(AABB& box)
{
    if (GetModel())
    {
        for (int i = 0; i < 2; ++i)
        {
            if (!GetModel()->IsEmpty(i))
            {
                box.SetTransformedAABB(GetWorldTM(), GetModel()->GetBoundBox(i));
                return;
            }
        }
    }

    BrushVec3 vWorldPos = GetWorldTM().GetTranslation();
    box = AABB(vWorldPos, vWorldPos);
}

void DesignerObject::GetLocalBounds(AABB& box)
{
    if (GetModel())
    {
        box = GetModel()->GetBoundBox(0);
    }
    else
    {
        box = AABB(Vec3(0, 0, 0), Vec3(0, 0, 0));
    }
}

bool DesignerObject::HitTest(HitContext& hc)
{
    if (CD::HitTest(MainContext(), hc))
    {
        hc.object = this;
        return true;
    }
    else if (GetModel())
    {
        BrushMatrix34 invMat = GetWorldTM().GetInverted();
        BrushVec3 localRaySrc = invMat.TransformPoint(hc.raySrc);
        BrushVec3 localRayDir = invMat.TransformVector(hc.rayDir);
        for (int i = 0, iPolygonCount(GetModel()->GetPolygonCount()); i < iPolygonCount; ++i)
        {
            CD::PolygonPtr pPolygon = GetModel()->GetPolygon(i);
            if (!pPolygon->IsOpen())
            {
                continue;
            }
            for (int k = 0, iEdgeCount(pPolygon->GetEdgeCount()); k < iEdgeCount; ++k)
            {
                BrushEdge3D edge = pPolygon->GetEdge(k);
                if (CD::HitTestEdge(localRaySrc, localRayDir, pPolygon->GetPlane().Normal(), edge, hc.view))
                {
                    hc.object = this;
                    return true;
                }
            }
        }
    }
    return false;
}

IPhysicalEntity* DesignerObject::GetCollisionEntity() const
{
    if (GetCompiler() == NULL)
    {
        return NULL;
    }
    if (GetCompiler()->GetRenderNode())
    {
        return GetCompiler()->GetRenderNode()->GetPhysics();
    }
    return NULL;
}

void DesignerObject::BeginEditParams(IEditor* ie, int flags)
{
    DesignerBaseObject<CBaseObject>::BeginEditParams(ie, flags | OBJECT_COLLAPSE_OBJECTPANEL);

    if (!(flags & OBJECT_CREATE))
    {
        SwitchToDesignerEditTool();
    }
}

void DesignerObject::BeginEditMultiSelParams(bool bAllOfSameType)
{
    DesignerBaseObject<CBaseObject>::BeginEditMultiSelParams(bAllOfSameType);

    if (!bAllOfSameType)
    {
        return;
    }

    SwitchToDesignerEditTool();
}

void DesignerObject::SwitchToDesignerEditTool()
{
    CEditTool* pEditTool = GetIEditor()->GetEditTool();
    if (!pEditTool)
    {
        return;
    }


    if (!qobject_cast<DesignerTool*>(pEditTool))
    {
        GetIEditor()->SetEditTool("EditTool.DesignerTool", false);
        pEditTool = GetIEditor()->GetEditTool();
        DESIGNER_ASSERT(qobject_cast<DesignerTool*>(pEditTool));
        if (!qobject_cast<DesignerTool*>(pEditTool))
        {
            return;
        }
    }
}

void DesignerObject::Serialize(CObjectArchive& ar)
{
    CBaseObject::Serialize(ar);

    XmlNodeRef xmlNode = ar.node;

    if (ar.bLoading)
    {
        QString typeName("Designer");
        xmlNode->getAttr("Type", typeName);

        bool bDesignerBinary = false;
        xmlNode->getAttr("DesignerBinary", bDesignerBinary);

        XmlNodeRef brushNode = xmlNode->findChild("Brush");
        if (brushNode)
        {
            bool bDesignerObject = true;
            if (!brushNode->haveAttr("DesignerModeFlags"))
            {
                bDesignerObject = false;
            }

            if (!GetCompiler())
            {
                SetCompiler(new CD::ModelCompiler(CD::eCompiler_General));
            }

            if (!GetModel())
            {
                SetModel(new CD::Model);
            }

            if (bDesignerObject)
            {
                if (ar.bUndo || !bDesignerBinary)
                {
                    GetModel()->Serialize(brushNode, ar.bLoading, ar.bUndo);
                }
            }
            else
            {
                bool bConvertSuccess = CD::Converter::ConvertSolidXMLToDesignerObject(brushNode, this);
                DESIGNER_ASSERT(bConvertSuccess);
            }
        }

        if (ar.bUndo)
        {
            if (GetCompiler())
            {
                GetCompiler()->DeleteAllRenderNodes();
            }
        }

        if (GetCompiler())
        {
            int nRenderFlag = ERF_HAS_CASTSHADOWMAPS | ERF_CASTSHADOWMAPS;
            ar.node->getAttr("RndFlags", nRenderFlag);
            if (nRenderFlag & ERF_CASTSHADOWMAPS)
            {
                nRenderFlag |= ERF_HAS_CASTSHADOWMAPS;
            }
            GetCompiler()->SetRenderFlags(nRenderFlag);
            int nStatObjFlag = 0;
            if (ar.node->getAttr("StaticObjFlags", nStatObjFlag))
            {
                GetCompiler()->SetStaticObjFlags(nStatObjFlag);
            }
            float viewDistanceMultiplier = CD::kDefaultViewDist;
            ar.node->getAttr("ViewDistRatio", viewDistanceMultiplier);
            GetCompiler()->SetViewDistanceMultiplier(viewDistanceMultiplier);

            XmlNodeRef meshNode = ar.node->findChild("Mesh");
            if (meshNode)
            {
                const char* encodedStr;
                int nVersion = 0;
                meshNode->getAttr("Version", nVersion);
                if (meshNode->getAttr("BinaryData", &encodedStr))
                {
                    int nLength = strlen(encodedStr);
                    if (nLength > 0)
                    {
                        int nDestBufferLen = Base64::decodedsize_base64(nLength);
                        std::vector<char> buffer;
                        buffer.resize(nDestBufferLen, 0);
                        Base64::decode_base64(&buffer[0], encodedStr, nLength, false);
                        GetCompiler()->LoadMesh(nVersion, buffer, this, GetModel());
                        bDesignerBinary = true;
                    }
                }
            }
        }

        if (ar.bUndo || !bDesignerBinary)
        {
            if (GetModel()->GetSubdivisionLevel() > 0)
            {
                GetModel()->ResetDB(CD::eDBRF_Vertex);
            }
            CD::UpdateAll(MainContext(), CD::eUT_Mesh);
        }
    }
    else
    {
        if (GetModel())
        {
            XmlNodeRef brushNode = xmlNode->newChild("Brush");
            GetModel()->Serialize(brushNode, ar.bLoading, ar.bUndo);
        }
        if (m_pCompiler)
        {
            ar.node->setAttr("RndFlags", ERF_GET_WRITABLE(GetCompiler()->GetRenderFlags()));
            ar.node->setAttr("StaticObjFlags", ERF_GET_WRITABLE(GetCompiler()->GetStaticObjFlags()));
            ar.node->setAttr("ViewDistMultiplier", GetCompiler()->GetViewDistanceMultiplier());
            if (!GetCompiler()->IsValid())
            {
                GetCompiler()->Compile(this, GetModel());
            }
            // too much polygon count causes too big xml node so only a mesh below the maximum polygon count(in this case 10000) should have mesh node.
            // actually a mesh node is for speeding loading time up so if mesh node doesn't remain, a mesh will be calculated.
            if (GetCompiler()->IsValid() && !ar.bUndo && GetCompiler()->GetPolygonCount() < 60000)
            {
                XmlNodeRef meshNode = xmlNode->newChild("Mesh");
                std::vector<char> meshBuffer;
                const int nMeshVersion = 2;
                if (GetCompiler()->SaveMesh(nMeshVersion, meshBuffer, this, GetModel()))
                {
                    unsigned int meshBufferSize = meshBuffer.size();
                    std::vector<char> encodedStr(Base64::encodedsize_base64(meshBufferSize) + 1);
                    int nReturnedSize = Base64::encode_base64(&encodedStr[0], &meshBuffer[0], meshBufferSize, true);
                    meshNode->setAttr("Version", nMeshVersion);
                    meshNode->setAttr("BinaryData", &encodedStr[0]);
                }
            }
        }
    }
}

void DesignerObject::SetMaterial(CMaterial* mtl)
{
    CEditTool* pEditTool = GetIEditor()->GetEditTool();
    bool bAssignedSubmaterial = false;

    if (mtl == NULL)
    {
        mtl = GetIEditor()->GetMaterialManager()->LoadMaterial(CD::BrushDesignerDefaultMaterial);
    }

    CMaterial* pParentMat = mtl ? mtl->GetParent() : NULL;
    if (pParentMat && pParentMat == GetMaterial())
    {
        for (int i = 0, iSubMatCount(pParentMat->GetSubMaterialCount()); i < iSubMatCount; ++i)
        {
            if (pParentMat->GetSubMaterial(i) != mtl)
            {
                continue;
            }
            if (qobject_cast<DesignerTool*>(pEditTool))
            {
                ((DesignerTool*)pEditTool)->SetSubMatID(i, GetModel());
            }
            GetCompiler()->Compile(this, GetModel());
            bAssignedSubmaterial = true;
            break;
        }
    }

    if (!bAssignedSubmaterial)
    {
        if (!pParentMat)
        {
            CBaseObject::SetMaterial(mtl);
        }
        else if (pParentMat != GetMaterial())
        {
            CBaseObject::SetMaterial(pParentMat);
            SetMaterial(mtl);
        }
    }

    UpdateEngineNode();
}

void DesignerObject::SetMaterial(const QString& materialName)
{
    CMaterial* pMaterial = NULL;
    pMaterial = GetIEditor()->GetMaterialManager()->LoadMaterial(materialName);
    if (!pMaterial || (pMaterial && pMaterial->IsDummy()))
    {
        _smart_ptr<IMaterial> pMatInfo = GetIEditor()->Get3DEngine()->GetMaterialManager()->LoadMaterial(materialName.toUtf8().data(), false);
        if (pMatInfo)
        {
            GetIEditor()->GetMaterialManager()->OnCreateMaterial(pMatInfo);
            pMaterial = GetIEditor()->GetMaterialManager()->FromIMaterial(pMatInfo);
        }
    }
    CBaseObject::SetMaterial(pMaterial);

    UpdateEngineNode();
}

void DesignerObject::SetMaterialLayersMask(uint32 nLayersMask)
{
    CBaseObject::SetMaterialLayersMask(nLayersMask);
    UpdateEngineNode();
}

void DesignerObject::SetMinSpec(uint32 nSpec, bool bSetChildren)
{
    DesignerBaseObject<CBaseObject>::SetMinSpec(nSpec, bSetChildren);
    if (GetCompiler() && GetCompiler()->GetRenderNode())
    {
        GetCompiler()->GetRenderNode()->SetMinSpec(GetMinSpec());
    }
}

bool DesignerObject::IsSimilarObject(CBaseObject* pObject)
{
    if (pObject->GetClassDesc() == GetClassDesc() && metaObject() == pObject->metaObject())
    {
        return true;
    }
    return false;
}

void DesignerObject::OnEvent(ObjectEvent event)
{
    CBaseObject::OnEvent(event);
    switch (event)
    {
    case EVENT_INGAME:
    {
        if (CheckFlags(OBJFLAG_SUBOBJ_EDITING))
        {
            EndSubObjectSelection();
        }
        DesignerTool* pDesignerTool = CD::GetDesigner();
        if (pDesignerTool && !GetIEditor()->GetGameEngine()->GetSimulationMode())
        {
            pDesignerTool->LeaveCurrentTool();
        }
    }
    break;
    case EVENT_OUTOFGAME:
    {
        DesignerTool* pDesignerTool = CD::GetDesigner();
        if (pDesignerTool)
        {
            pDesignerTool->EnterCurrentTool();
        }
    }
    break;
    case EVENT_HIDE_HELPER:
        if (IsSelected() && GetCompiler())
        {
            _smart_ptr<IStatObj> obj = NULL;
            if (GetCompiler()->GetIStatObj(&obj))
            {
                int flag = obj->GetFlags();
                flag &= ~STATIC_OBJECT_HIDDEN;
                obj->SetFlags(flag);
            }
        }
        break;
    }
}

void DesignerObject::UpdateVisibility(bool visible)
{
    CBaseObject::UpdateVisibility(visible);

    if (GetCompiler() == NULL)
    {
        return;
    }

    if (GetCompiler()->GetRenderNode())
    {
        int renderFlag = GetCompiler()->GetRenderFlags();
        if (!visible || IsHiddenBySpec())
        {
            renderFlag |= ERF_HIDDEN;
        }
        else
        {
            renderFlag &= ~ERF_HIDDEN;
        }

        GetCompiler()->SetRenderFlags(renderFlag);

        _smart_ptr<IStatObj> pStatObj;
        if (GetCompiler()->GetIStatObj(&pStatObj))
        {
            int flag = pStatObj->GetFlags();
            if (visible)
            {
                flag  &= (~STATIC_OBJECT_HIDDEN);
            }
            else
            {
                flag |= STATIC_OBJECT_HIDDEN;
            }
            pStatObj->SetFlags(flag);
        }
    }
}

void DesignerObject::WorldToLocalRay(Vec3& raySrc, Vec3& rayDir) const
{
    raySrc = m_invertTM.TransformPoint(raySrc);
    rayDir = m_invertTM.TransformVector(rayDir).GetNormalized();
}

void DesignerObject::InvalidateTM(int nWhyFlags)
{
    CBaseObject::InvalidateTM(nWhyFlags);
    UpdateEngineNode();
    m_invertTM = GetWorldTM();
    m_invertTM.Invert();
}

void DesignerObject::GenerateGameFilename(QString& generatedFileName) const
{
    generatedFileName = QString("%level%/Brush/designer_%1.%2").arg(m_nBrushUniqFileId).arg(QStringLiteral(CRY_GEOMETRY_FILE_EXT));
}

void DesignerObject::DrawDimensions(DisplayContext& dc, AABB* pMergedBoundBox)
{
    if (!HasMeasurementAxis() || CD::GetDesigner() || !gDesignerSettings.bDisplayDimensionHelper)
    {
        return;
    }
    AABB localBoundBox;
    GetLocalBounds(localBoundBox);
    DrawDimensionsImpl(dc, localBoundBox, pMergedBoundBox);
}

IRenderNode* DesignerObject::GetEngineNode() const
{
    if (!GetCompiler() || !GetCompiler()->GetRenderNode())
    {
        return NULL;
    }
    return GetCompiler()->GetRenderNode();
}

void DesignerObject::OnMaterialChanged(MaterialChangeFlags change)
{
    DesignerTool* pTool = CD::GetDesigner();
    if (pTool)
    {
        pTool->MaterialChanged();
    }
}

namespace CD
{
    SDesignerEngineFlags::SDesignerEngineFlags()
        : m_pObj(NULL)
    {
        outdoor = false;
        castShadows = true;
        supportSecVisArea = false;
        rainOccluder = false;
        hideable = false;
        viewDistanceMultiplier = 1.0f;
        excludeFromTriangulation = false;
        noDynWater = false;
        noStaticDecals = false;
        excludeCollision = false;
        occluder = false;
    }

    void SDesignerEngineFlags::Set()
    {
        viewDistanceMultiplier = m_pObj->GetCompiler()->GetViewDistanceMultiplier();
        int flags = m_pObj->GetCompiler()->GetRenderFlags();
        int statobjFlags = m_pObj->GetCompiler()->GetStaticObjFlags();
        outdoor = (flags & ERF_OUTDOORONLY) != 0;
        rainOccluder = (flags & ERF_RAIN_OCCLUDER) != 0;
        castShadows =   (flags & ERF_CASTSHADOWMAPS) != 0;
        supportSecVisArea = (flags & ERF_REGISTER_BY_BBOX) != 0;
        occluder = (flags & ERF_GOOD_OCCLUDER) != 0;
        hideable = (flags & ERF_HIDABLE) != 0;
        noDynWater = (flags & ERF_NODYNWATER) != 0;
        noStaticDecals = (flags & ERF_NO_DECALNODE_DECALS) != 0;
        excludeFromTriangulation = (flags & ERF_EXCLUDE_FROM_TRIANGULATION) != 0;
        excludeCollision = (statobjFlags & STATIC_OBJECT_NO_PLAYER_COLLIDE) != 0;
    }

    void SDesignerEngineFlags::Serialize(Serialization::IArchive& ar)
    {
        ar(castShadows, "castShadows", "Cast Shadows");
        ar(supportSecVisArea, "supportSecVisArea", "Support Second Visarea");
        ar(outdoor, "outdoor", "Outdoor");
        ar(rainOccluder, "rainOccluder", "Rain Occluder");
        ar(viewDistanceMultiplier, "ratioViewDist", "View Distance Ratio");
        ar(excludeFromTriangulation, "excludeFromTriangulation", "AI Exclude From Triangulation");
        ar(hideable, "hideable", "AI Hideable");
        ar(noDynWater, "noDynWater", "No Dynamic Water");
        ar(noStaticDecals, "noStaticDecals", "No Static Decal");
        ar(excludeCollision, "excludeCollision", "Exclude Collision");
        ar(occluder, "occluder", "Occluder");
        if (ar.IsInput())
        {
            Update();
        }
    }

    namespace
    {
        void ModifyFlag(int& nFlags, int flag, int clearFlag, bool var)
        {
            nFlags = (var) ? (nFlags | flag) : (nFlags & (~clearFlag));
        }
        void ModifyFlag(int& nFlags, int flag, bool var)
        {
            ModifyFlag(nFlags, flag, flag, var);
        }
    }

    void SDesignerEngineFlags::Update()
    {
        int nFlags = m_pObj->GetCompiler()->GetRenderFlags();
        int statobjFlags = m_pObj->GetCompiler()->GetStaticObjFlags();

        ModifyFlag(nFlags, ERF_OUTDOORONLY, outdoor);
        ModifyFlag(nFlags, ERF_RAIN_OCCLUDER, rainOccluder);
        ModifyFlag(nFlags, ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS, ERF_CASTSHADOWMAPS, castShadows);
        ModifyFlag(nFlags, ERF_REGISTER_BY_BBOX, supportSecVisArea);
        ModifyFlag(nFlags, ERF_HIDABLE, hideable);
        ModifyFlag(nFlags, ERF_EXCLUDE_FROM_TRIANGULATION, excludeFromTriangulation);
        ModifyFlag(nFlags, ERF_NODYNWATER, noDynWater);
        ModifyFlag(nFlags, ERF_NO_DECALNODE_DECALS, noStaticDecals);
        ModifyFlag(nFlags, ERF_GOOD_OCCLUDER, occluder);
        ModifyFlag(statobjFlags, STATIC_OBJECT_NO_PLAYER_COLLIDE, excludeCollision);

        m_pObj->GetCompiler()->SetViewDistanceMultiplier(viewDistanceMultiplier);
        m_pObj->GetCompiler()->SetRenderFlags(nFlags);
        m_pObj->GetCompiler()->SetStaticObjFlags(statobjFlags);
        m_pObj->GetCompiler()->Compile(m_pObj, m_pObj->GetModel(), 0, true);
        CD::UpdateAll(m_pObj->MainContext(), CD::eUT_SyncPrefab);
    }
}

#include <Objects/DesignerObject.moc>