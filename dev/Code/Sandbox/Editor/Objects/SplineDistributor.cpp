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
#include "Geometry/EdMesh.h"
#include "SplineDistributor.h"
#include "Viewport.h"
#include "Material/Material.h"


//////////////////////////////////////////////////////////////////////////
CSplineDistributor::CSplineDistributor()
    : m_pGeometry(nullptr)
    , m_bGameObjectCreated(false)
{
    mv_step = 4.0f;
    mv_follow = true;
    mv_zAngle = 0.0f;

    mv_outdoor = false;
    mv_castShadowMaps = true;
    mv_rainOccluder = true;
    mv_registerByBBox = false;
    mv_hideable = 0;
    mv_ratioLOD = 100;
    mv_viewDistanceMultiplier = 1.0f;
    mv_noDecals = false;
    mv_recvWind = false;
    mv_integrQuality = 0;
    mv_Occluder = false;

    SetColor(Vec2Rgb(Vec3(0.4f, 0.5f, 0.4f)));

    AddVariable(mv_geometryFile, "Geometry", functor(*this, &CSplineDistributor::OnGeometryChange), IVariable::DT_OBJECT);
    AddVariable(mv_step, "StepSize", "Step Size", functor(*this, &CSplineDistributor::OnParamChange));
    mv_step.SetLimits(0.25f, 100.f);
    AddVariable(mv_follow, "Follow", "Follow Spline", functor(*this, &CSplineDistributor::OnParamChange));
    AddVariable(mv_zAngle, "ZAngle", "Z Angle", functor(*this, &CSplineDistributor::OnParamChange));
    mv_zAngle.SetLimits(-360.f, 360.f);

    AddVariable(mv_outdoor, "OutdoorOnly", functor(*this, &CSplineDistributor::OnParamChange));
    AddVariable(mv_castShadowMaps, "CastShadowMaps", functor(*this, &CSplineDistributor::OnParamChange));
    AddVariable(mv_rainOccluder, "RainOccluder", functor(*this, &CSplineDistributor::OnParamChange));
    AddVariable(mv_registerByBBox, "SupportSecondVisarea", functor(*this, &CSplineDistributor::OnParamChange));
    AddVariable(mv_hideable, "Hideable", functor(*this, &CSplineDistributor::OnParamChange));
    AddVariable(mv_ratioLOD, "LodRatio", functor(*this, &CSplineDistributor::OnParamChange));
    AddVariable(mv_viewDistanceMultiplier, "ViewDistanceMultiplier", functor(*this, &CSplineDistributor::OnParamChange));
    AddVariable(mv_excludeFromTriangulation, "NotTriangulate", functor(*this, &CSplineDistributor::OnParamChange));
    // keep for future implementation
    //AddVariable( mv_aiRadius, "AIRadius", functor(*this,&CSplineDistributor::OnAIRadiusVarChange) );
    AddVariable(mv_noDecals, "NoStaticDecals", functor(*this, &CSplineDistributor::OnParamChange));
    AddVariable(mv_recvWind, "RecvWind", functor(*this, &CSplineDistributor::OnParamChange));
    AddVariable(mv_Occluder, "Occluder", functor(*this, &CSplineDistributor::OnParamChange));

    mv_ratioLOD.SetLimits(0, 255);
    mv_viewDistanceMultiplier.SetLimits(0.0f, IRenderNode::VIEW_DISTANCE_MULTIPLIER_MAX);
}


//////////////////////////////////////////////////////////////////////////
void CSplineDistributor::Done()
{
    FreeGameData();
    CSplineObject::Done();
}


//////////////////////////////////////////////////////////////////////////
bool CSplineDistributor::CreateGameObject()
{
    m_bGameObjectCreated = true;
    OnUpdate();
    return true;
}


//////////////////////////////////////////////////////////////////////////
void CSplineDistributor::SetObjectCount(int num)
{
    int oldNum = m_renderNodes.size();
    if (oldNum < num)
    {
        m_renderNodes.resize(num);
        for (int i = oldNum; i < num; ++i)
        {
            m_renderNodes[i] = GetIEditor()->Get3DEngine()->CreateRenderNode(eERType_Brush);
        }
    }
    else
    {
        for (int i = num; i < oldNum; ++i)
        {
            GetIEditor()->Get3DEngine()->DeleteRenderNode(m_renderNodes[i]);
        }
        m_renderNodes.resize(num);
    }
}


//////////////////////////////////////////////////////////////////////////
void CSplineDistributor::FreeGameData()
{
    SetObjectCount(0);

    // Release Mesh.
    if (m_pGeometry)
    {
        m_pGeometry->RemoveUser();
    }
    m_pGeometry = 0;
}


//////////////////////////////////////////////////////////////////////////
void CSplineDistributor::OnUpdate()
{
    if (!m_bGameObjectCreated)
    {
        return;
    }
    SetObjectCount(int(GetSplineLength() / mv_step) + 1);

    UpdateEngineNode();
}


//////////////////////////////////////////////////////////////////////////
void CSplineDistributor::UpdateVisibility(bool visible)
{
    if (visible == CheckFlags(OBJFLAG_INVISIBLE))
    {
        CSplineObject::UpdateVisibility(visible);
    }
}


//////////////////////////////////////////////////////////////////////////
XmlNodeRef CSplineDistributor::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    return 0;
}


//////////////////////////////////////////////////////////////////////////
void CSplineDistributor::OnParamChange(IVariable* pVariable)
{
    OnUpdate();
}


//////////////////////////////////////////////////////////////////////////
void CSplineDistributor::OnGeometryChange(IVariable* pVariable)
{
    LoadGeometry(mv_geometryFile);
}

void CSplineDistributor::OnMaterialChanged(MaterialChangeFlags change)
{
    OnUpdate();
}

void CSplineDistributor::SetMinSpec(uint32 nSpec, bool bSetChildren)
{
    CSplineObject::SetMinSpec(nSpec, bSetChildren);
    OnUpdate();
}


//////////////////////////////////////////////////////////////////////////
void CSplineDistributor::LoadGeometry(const QString& filename)
{
    if (m_pGeometry)
    {
        if (m_pGeometry->IsSameObject(filename.toUtf8().data()))
        {
            return;
        }

        m_pGeometry->RemoveUser();
    }

    GetIEditor()->GetErrorReport()->SetCurrentFile(filename);
    m_pGeometry = CEdMesh::LoadMesh(filename.toUtf8().data());
    if (m_pGeometry)
    {
        m_pGeometry->AddUser();
    }
    GetIEditor()->GetErrorReport()->SetCurrentFile("");

    if (m_pGeometry)
    {
        UpdateEngineNode();
    }
}


//////////////////////////////////////////////////////////////////////////
void CSplineDistributor::InvalidateTM(int nWhyFlags)
{
    CBaseObject::InvalidateTM(nWhyFlags);

    if (!(nWhyFlags & eObjectUpdateFlags_RestoreUndo)) // Can skip updating game object when restoring undo.
    {
        UpdateEngineNode(true);
    }
}


//////////////////////////////////////////////////////////////////////////
void CSplineDistributor::UpdateEngineNode(bool bOnlyTransform)
{
    if (m_renderNodes.empty() || GetPointCount() == 0)
    {
        return;
    }

    float length = GetSplineLength();
    int numObjects = m_renderNodes.size();

    for (int i = 0; i < numObjects; ++i)
    {
        IRenderNode* pRenderNode = m_renderNodes[i];

        // Set brush render flags.
        int renderFlags = 0;
        if (mv_outdoor)
        {
            renderFlags |= ERF_OUTDOORONLY;
        }
        if (mv_castShadowMaps)
        {
            renderFlags |= ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS;
        }
        if (mv_rainOccluder)
        {
            renderFlags |= ERF_RAIN_OCCLUDER;
        }
        if (mv_registerByBBox)
        {
            renderFlags |= ERF_REGISTER_BY_BBOX;
        }
        if (IsHidden() || IsHiddenBySpec())
        {
            renderFlags |= ERF_HIDDEN;
        }
        if (mv_hideable == 1)
        {
            renderFlags |= ERF_HIDABLE;
        }
        if (mv_hideable == 2)
        {
            renderFlags |= ERF_HIDABLE_SECONDARY;
        }
        if (mv_excludeFromTriangulation)
        {
            renderFlags |= ERF_EXCLUDE_FROM_TRIANGULATION;
        }
        if (mv_noDecals)
        {
            renderFlags |= ERF_NO_DECALNODE_DECALS;
        }
        if (mv_recvWind)
        {
            renderFlags |= ERF_RECVWIND;
        }
        if (mv_Occluder)
        {
            renderFlags |= ERF_GOOD_OCCLUDER;
        }

        if (pRenderNode->GetRndFlags() & ERF_COLLISION_PROXY)
        {
            renderFlags |= ERF_COLLISION_PROXY;
        }

        if (pRenderNode->GetRndFlags() & ERF_RAYCAST_PROXY)
        {
            renderFlags |= ERF_RAYCAST_PROXY;
        }

        if (IsSelected() && gSettings.viewports.bHighlightSelectedGeometry)
        {
            renderFlags |= ERF_SELECTED;
        }

        pRenderNode->SetRndFlags(renderFlags);

        pRenderNode->SetMinSpec(GetMinSpec());
        pRenderNode->SetViewDistanceMultiplier(mv_viewDistanceMultiplier);
        pRenderNode->SetLodRatio(mv_ratioLOD);

        pRenderNode->SetMaterialLayers(GetMaterialLayersMask());

        if (m_pGeometry)
        {
            Matrix34A tm;

            tm.SetIdentity();

            if (mv_zAngle != 0.0f)
            {
                Matrix34A tmRot;
                tmRot.SetRotationZ(mv_zAngle / 180.f  * g_PI);
                tm = tmRot * tm;
            }

            int index;
            float pos = GetPosByDistance(i * mv_step, index);
            Vec3 p = GetBezierPos(index, pos);

            if (mv_follow)
            {
                Matrix34A tmPos;
                Vec3 t = GetBezierTangent(index, pos);
                Vec3 b = GetLocalBezierNormal(index, pos);
                Vec3 n = t.Cross(b);
                tmPos.SetFromVectors(t, b, n, p);
                tm = tmPos * tm;
            }
            else
            {
                Matrix34A tmPos;
                tmPos.SetTranslationMat(p);
                tm = tmPos * tm;
            }

            tm = GetWorldTM() * tm;

            pRenderNode->SetEntityStatObj(0, m_pGeometry->GetIStatObj(), &tm);
        }
        else
        {
            pRenderNode->SetEntityStatObj(0, 0, 0);
        }

        //IStatObj *pStatObj = pRenderNode->GetEntityStatObj();
        //if (pStatObj)
        //{
        //  float r = mv_aiRadius;
        //  pStatObj->SetAIVegetationRadius(r);
        //}

        // Fast exit if only transformation needs to be changed.
        if (bOnlyTransform)
        {
            continue;
        }


        if (GetMaterial())
        {
            GetMaterial()->AssignToEntity(pRenderNode);
        }
        else
        {
            // Reset all material settings for this node.
            pRenderNode->SetMaterial(0);
        }


        pRenderNode->SetLayerId(0);
    }
}


//////////////////////////////////////////////////////////////////////////
void CSplineDistributor::SetLayerId(uint16 nLayerId)
{
    for (size_t i = 0; i < m_renderNodes.size(); ++i)
    {
        if (m_renderNodes[i])
        {
            m_renderNodes[i]->SetLayerId(nLayerId);
        }
    }
}





//////////////////////////////////////////////////////////////////////////
// Class Description of SplineDistributor.


//////////////////////////////////////////////////////////////////////////
class CSplineDistributorClassDesc
    : public CObjectClassDesc
{
public:
    REFGUID ClassID()
    {
        // {0435F565-46C5-45AE-82F8-223845B850D7}
        static const GUID guid = {
            0x435f565, 0x46c5, 0x45ae, { 0x82, 0xf8, 0x22, 0x38, 0x45, 0xb8, 0x50, 0xd7 }
        };
        return guid;
    }
    ObjectType GetObjectType() { return OBJTYPE_SHAPE; }
    QString ClassName() { return "SplineDistributor"; }
    QString Category() { return "Misc"; }
    QObject* CreateQObject() const override { return new CSplineDistributor; }
    int GameCreationOrder() { return 50; }
};

REGISTER_CLASS_DESC(CSplineDistributorClassDesc);

#include <Objects/SplineDistributor.moc>