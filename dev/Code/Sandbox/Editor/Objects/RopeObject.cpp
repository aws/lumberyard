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

// Description : CTagPoint implementation.


#include "StdAfx.h"

#include "RopeObject.h"
#include <I3DEngine.h>
#include "Material/Material.h"
#include <Controls/ReflectedPropertyControl/ReflectedPropertiesPanel.h>
#include "ShapePanel.h"
#include <IEntityHelper.h>
#include "Components/IComponentRope.h"

//////////////////////////////////////////////////////////////////////////

#define LINE_CONNECTED_COLOR QColor(0, 255, 0)
#define LINE_DISCONNECTED_COLOR QColor(255, 255, 0)
#define ROPE_PHYS_SEGMENTS_MAX 100

//////////////////////////////////////////////////////////////////////////
class CRopePanelUI
    : public ReflectedPropertiesPanel
{
public:
    _smart_ptr<CVarBlock> pVarBlock;
    _smart_ptr<CRopeObject> m_pObject;

    CSmartVariable<float> mv_thickness;
    CSmartVariable<int> mv_numSegments;
    CSmartVariable<int> mv_numSides;
    CSmartVariable<int> mv_numSubVerts;
    CSmartVariable<int> mv_numPhysSegments;
    CSmartVariable<float> mv_texTileU;
    CSmartVariable<float> mv_texTileV;
    CSmartVariable<float> mv_anchorsRadius;
    CSmartVariable<bool> mv_smooth;

    CSmartVariableArray mv_PhysicsGroup;
    CSmartVariableArray mv_PhysicsGroupAdv;

    CSmartVariable<float> mv_mass;
    CSmartVariable<float> mv_tension;
    CSmartVariable<float> mv_friction;
    CSmartVariable<float> mv_frictionPull;

    CSmartVariable<Vec3> mv_wind;
    CSmartVariable<float> mv_windVariation;

    CSmartVariable<float> mv_airResistance;
    CSmartVariable<float> mv_waterResistance;

    //CSmartVariable<float> mv_jointLimit;
    CSmartVariable<float> mv_maxForce;

    CSmartVariable<bool> mv_awake;
    CSmartVariable<int> mv_maxIters;
    CSmartVariable<float> mv_maxTimeStep;
    CSmartVariable<float> mv_stiffness;
    CSmartVariable<float> mv_hardness;
    CSmartVariable<float> mv_damping;
    CSmartVariable<float> mv_sleepSpeed;

    CSmartVariable<bool> mv_collision;
    CSmartVariable<bool> mv_noCollisionAtt;
    CSmartVariable<bool> mv_noCollisionPlayer;
    CSmartVariable<bool> mv_subdivide;
    CSmartVariable<bool> mv_bindEnds;
    CSmartVariable<bool> mv_nonshootable;
    CSmartVariable<bool> mv_disabled;
    CSmartVariable<bool> mv_attach0;
    CSmartVariable<bool> mv_attach1;
    CSmartVariable<bool> mv_shadows;

    // Sound related
    CSmartVariableArray mv_SoundGroup;
    CSmartVariable<QString> mv_soundEventName;
    CSmartVariable<int> mv_numSegmentAttachTo;
    CSmartVariable<float> mv_soundPosOffset;

    CRopePanelUI()
    {
        pVarBlock = new CVarBlock;

        mv_thickness->SetLimits(0.001f, 10000);
        mv_numSegments->SetLimits(2, 1000);
        mv_numSides->SetLimits(2, 100);
        mv_numSubVerts->SetLimits(1, 100);
        mv_numPhysSegments->SetLimits(1, ROPE_PHYS_SEGMENTS_MAX);
        mv_numSegmentAttachTo->SetLimits(1, ROPE_PHYS_SEGMENTS_MAX);
        mv_soundPosOffset->SetLimits(0.0f, 1.0f);

        //      pVarBlock->AddVariable( mv_selfShadowing,"Self Shadowing" );
        AddVariable(pVarBlock, mv_thickness, "Radius");
        AddVariable(pVarBlock, mv_smooth, "Smooth");
        AddVariable(pVarBlock, mv_numSegments, "Num Segments");
        AddVariable(pVarBlock, mv_numSides, "Num Sides");

        AddVariable(mv_PhysicsGroup, mv_subdivide, "Subdivide");
        AddVariable(mv_PhysicsGroup, mv_numSubVerts, "Max Subdiv Verts");

        AddVariable(pVarBlock, mv_texTileU, "Texture U Tiling");
        AddVariable(pVarBlock, mv_texTileV, "Texture V Tiling");
        AddVariable(pVarBlock, mv_shadows, "CastShadows");

        AddVariable(pVarBlock, mv_bindEnds, "Bind Ends Radius");
        AddVariable(pVarBlock, mv_anchorsRadius, "Bind Radius");

        AddVariable(pVarBlock, mv_PhysicsGroup, "Physics Params");

        AddVariable(mv_PhysicsGroup, mv_numPhysSegments, "Physical Segments");
        AddVariable(mv_PhysicsGroupAdv, mv_mass, "Mass");
        AddVariable(mv_PhysicsGroup, mv_tension, "Tension");
        AddVariable(mv_PhysicsGroup, mv_friction, "Friction");
        AddVariable(mv_PhysicsGroupAdv, mv_frictionPull, "Friction Pull");

        AddVariable(mv_PhysicsGroup, mv_wind, "Wind");
        AddVariable(mv_PhysicsGroup, mv_windVariation, "Wind Variation");
        AddVariable(mv_PhysicsGroup, mv_airResistance, "Air Resistance");
        AddVariable(mv_PhysicsGroup, mv_waterResistance, "Water Resistance");
        //AddVariable( mv_PhysicsGroup,mv_jointLimit,"Joint Limit" );
        AddVariable(mv_PhysicsGroupAdv, mv_maxForce, "Max Force");
        mv_maxForce->SetDescription("Maximal force rope can withstand before breaking");

        AddVariable(mv_PhysicsGroupAdv, mv_awake, "Awake");
        AddVariable(mv_PhysicsGroupAdv, mv_maxIters, "Solver Iterations");
        AddVariable(mv_PhysicsGroupAdv, mv_maxTimeStep, "Max Timestep");
        AddVariable(mv_PhysicsGroupAdv, mv_stiffness, "Stiffness");
        AddVariable(mv_PhysicsGroupAdv, mv_hardness, "Contact Hardness");
        AddVariable(mv_PhysicsGroupAdv, mv_damping, "Damping");
        AddVariable(mv_PhysicsGroupAdv, mv_sleepSpeed, "Sleep Speed");

        AddVariable(mv_PhysicsGroup, mv_collision, "Check Collisions");
        AddVariable(mv_PhysicsGroup, mv_noCollisionAtt, "Ignore Attachment Collisions");
        AddVariable(mv_PhysicsGroup, mv_noCollisionPlayer, "Ignore Player Collisions");

        AddVariable(mv_PhysicsGroup, mv_nonshootable, "Non-shootable");
        AddVariable(mv_PhysicsGroup, mv_disabled, "Disabled");
        AddVariable(mv_PhysicsGroup, mv_attach0, "StaticAttachStart");
        AddVariable(mv_PhysicsGroup, mv_attach1, "StaticAttachEnd");

        AddVariable(mv_PhysicsGroup, mv_PhysicsGroupAdv, "Advanced");

        // Sound related
        AddVariable(pVarBlock, mv_SoundGroup, "Sound Data");
        AddVariable(mv_SoundGroup, mv_soundEventName, "Name", IVariable::DT_AUDIO_TRIGGER);
        AddVariable(mv_SoundGroup, mv_numSegmentAttachTo, "Segment");
        AddVariable(mv_SoundGroup, mv_soundPosOffset, "PosOffset");
    }

    void AddVariables()
    {
        SetVarBlock(pVarBlock, functor(*this, &CRopePanelUI::OnVarChange));
    }
    void SyncUI(IRopeRenderNode::SRopeParams& rp, bool bCopyToUI, IVariable* pModifiedVar = NULL)
    {
        SyncValue(mv_thickness, rp.fThickness, bCopyToUI, pModifiedVar);
        SyncValue(mv_numSegments, rp.nNumSegments, bCopyToUI, pModifiedVar);
        SyncValue(mv_numSides, rp.nNumSides, bCopyToUI, pModifiedVar);
        SyncValue(mv_numSubVerts, rp.nMaxSubVtx, bCopyToUI, pModifiedVar);
        SyncValue(mv_texTileU, rp.fTextureTileU, bCopyToUI, pModifiedVar);
        SyncValue(mv_texTileV, rp.fTextureTileV, bCopyToUI, pModifiedVar);
        SyncValue(mv_anchorsRadius, rp.fAnchorRadius, bCopyToUI, pModifiedVar);
        SyncValue(mv_numPhysSegments, rp.nPhysSegments, bCopyToUI, pModifiedVar);

        SyncValue(mv_mass, rp.mass, bCopyToUI, pModifiedVar);
        SyncValue(mv_tension, rp.tension, bCopyToUI, pModifiedVar);
        SyncValue(mv_friction, rp.friction, bCopyToUI, pModifiedVar);
        SyncValue(mv_frictionPull, rp.frictionPull, bCopyToUI, pModifiedVar);
        SyncValue(mv_wind, rp.wind, bCopyToUI, pModifiedVar);
        SyncValue(mv_windVariation, rp.windVariance, bCopyToUI, pModifiedVar);
        SyncValue(mv_airResistance, rp.airResistance, bCopyToUI, pModifiedVar);
        SyncValue(mv_waterResistance, rp.waterResistance, bCopyToUI, pModifiedVar);
        //SyncValue( mv_jointLimit,rp.jointLimit,bCopyToUI,pModifiedVar );
        SyncValue(mv_maxForce, rp.maxForce, bCopyToUI, pModifiedVar);
        SyncValue(mv_maxIters, rp.nMaxIters, bCopyToUI, pModifiedVar);
        SyncValue(mv_maxTimeStep, rp.maxTimeStep, bCopyToUI, pModifiedVar);
        SyncValue(mv_stiffness, rp.stiffness, bCopyToUI, pModifiedVar);
        SyncValue(mv_hardness, rp.hardness, bCopyToUI, pModifiedVar);
        SyncValue(mv_damping, rp.damping, bCopyToUI, pModifiedVar);
        SyncValue(mv_sleepSpeed, rp.sleepSpeed, bCopyToUI, pModifiedVar);

        SyncValueFlag(mv_awake, rp.nFlags, IRopeRenderNode::eRope_Awake, bCopyToUI, pModifiedVar);
        SyncValueFlag(mv_bindEnds, rp.nFlags, IRopeRenderNode::eRope_BindEndPoints, bCopyToUI, pModifiedVar);
        SyncValueFlag(mv_collision, rp.nFlags, IRopeRenderNode::eRope_CheckCollisinos, bCopyToUI, pModifiedVar);
        SyncValueFlag(mv_noCollisionAtt, rp.nFlags, IRopeRenderNode::eRope_NoAttachmentCollisions, bCopyToUI, pModifiedVar);
        SyncValueFlag(mv_noCollisionPlayer, rp.nFlags, IRopeRenderNode::eRope_NoPlayerCollisions, bCopyToUI, pModifiedVar);
        SyncValueFlag(mv_subdivide, rp.nFlags, IRopeRenderNode::eRope_Subdivide, bCopyToUI, pModifiedVar);
        SyncValueFlag(mv_smooth, rp.nFlags, IRopeRenderNode::eRope_Smooth, bCopyToUI, pModifiedVar);
        SyncValueFlag(mv_nonshootable, rp.nFlags, IRopeRenderNode::eRope_Nonshootable, bCopyToUI, pModifiedVar);
        SyncValueFlag(mv_disabled, rp.nFlags, IRopeRenderNode::eRope_Disabled, bCopyToUI, pModifiedVar);
        SyncValueFlag(mv_attach0, rp.nFlags, IRopeRenderNode::eRope_StaticAttachStart, bCopyToUI, pModifiedVar);
        SyncValueFlag(mv_attach1, rp.nFlags, IRopeRenderNode::eRope_StaticAttachEnd, bCopyToUI, pModifiedVar);
        SyncValueFlag(mv_shadows, rp.nFlags, IRopeRenderNode::eRope_CastShadows, bCopyToUI, pModifiedVar);
    }
    void SetObject(CRopeObject* pObject)
    {
        m_pObject = pObject;
        if (pObject)
        {
            DeleteVars();
            SyncUI(pObject->m_ropeParams, true);

            // Sound related
            SyncValue(mv_soundEventName, pObject->m_ropeSoundData.sName, true);
            SyncValue(mv_numSegmentAttachTo, pObject->m_ropeSoundData.nSegementToAttachTo, true);
            SyncValue(mv_soundPosOffset, pObject->m_ropeSoundData.fOffset, true);

            AddVariables();
        }
    }
    void OnVarChange(IVariable* pVar)
    {
        CSelectionGroup* selection = GetIEditor()->GetSelection();
        for (int i = 0; i < selection->GetCount(); i++)
        {
            CBaseObject* pObj = selection->GetObject(i);
            if (qobject_cast<CRopeObject*>(pObj))
            {
                CRopeObject* pObject = (CRopeObject*)pObj;
                if (CUndo::IsRecording())
                {
                    pObject->StoreUndo("Rope Modify Undo");
                }
                SyncUI(pObject->m_ropeParams, false, pVar);

                for (int i = 0; i < mv_SoundGroup->GetNumVariables(); ++i)
                {
                    if (mv_SoundGroup->GetVariable(i) == pVar || mv_numPhysSegments.GetVar() == pVar)
                    {
                        // A sound variable or one that influences a sound variable has been updated
                        SyncValue(mv_soundEventName, pObject->m_ropeSoundData.sName, false, pVar);

                        // Make sure we do not exceed segment count and do not fall below 1
                        if (mv_numSegmentAttachTo > pObject->m_ropeParams.nPhysSegments)
                        {
                            mv_numSegmentAttachTo->Set(pObject->m_ropeParams.nPhysSegments);
                        }
                        else if (mv_numSegmentAttachTo < 1)
                        {
                            mv_numSegmentAttachTo->Set(1);
                        }

                        SyncValue(mv_numSegmentAttachTo, pObject->m_ropeSoundData.nSegementToAttachTo, false, (pVar == mv_numPhysSegments.GetVar()) ? mv_numSegmentAttachTo.GetVar() : pVar);
                        SyncValue(mv_soundPosOffset, pObject->m_ropeSoundData.fOffset, false, pVar);
                        pObject->UpdateSoundData();
                        break;
                    }
                }

                pObject->m_bAreaModified = true;
                pObject->UpdateGameArea(false);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Helper functions.
    //////////////////////////////////////////////////////////////////////////
    template <class T>
    void SyncValue(CSmartVariable<T>& var, T& value, bool bCopyToUI, IVariable* pSrcVar = NULL)
    {
        if (bCopyToUI)
        {
            var = value;
        }
        else
        {
            if (!pSrcVar || pSrcVar == var.GetVar())
            {
                value = var;
            }
        }
    }
    void SyncValueFlag(CSmartVariable<bool>& var, int& nFlags, int flag, bool bCopyToUI, IVariable* pVar = NULL)
    {
        if (bCopyToUI)
        {
            var = (nFlags & flag) != 0;
        }
        else
        {
            if (!pVar || var.GetVar() == pVar)
            {
                nFlags = (var) ? (nFlags | flag) : (nFlags & (~flag));
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void AddVariable(CVariableBase& varArray, CVariableBase& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE)
    {
        if (varName)
        {
            var.SetName(varName);
        }
        var.SetDataType(dataType);
        varArray.AddVariable(&var);
    }
    //////////////////////////////////////////////////////////////////////////
    void AddVariable(CVarBlock* vars, CVariableBase& var, const char* varName, unsigned char dataType = IVariable::DT_SIMPLE)
    {
        if (varName)
        {
            var.SetName(varName);
        }
        var.SetDataType(dataType);
        vars->AddVariable(&var);
    }
};

inline void RopeParamsToXml(IRopeRenderNode::SRopeParams& rp, XmlNodeRef& node, bool bLoad)
{
    if (bLoad)
    {
        // Load.
        node->getAttr("flags", rp.nFlags);
        node->getAttr("radius", rp.fThickness);
        node->getAttr("anchor_radius", rp.fAnchorRadius);
        node->getAttr("num_seg", rp.nNumSegments);
        node->getAttr("num_sides", rp.nNumSides);
        node->getAttr("radius", rp.fThickness);
        node->getAttr("texu", rp.fTextureTileU);
        node->getAttr("texv", rp.fTextureTileV);
        node->getAttr("ph_num_seg", rp.nPhysSegments);
        node->getAttr("ph_sub_vtxs", rp.nMaxSubVtx);
        node->getAttr("mass", rp.mass);
        node->getAttr("friction", rp.friction);
        node->getAttr("friction_pull", rp.frictionPull);
        node->getAttr("wind", rp.wind);
        node->getAttr("wind_var", rp.windVariance);
        node->getAttr("air_resist", rp.airResistance);
        node->getAttr("water_resist", rp.waterResistance);
        node->getAttr("joint_lim", rp.jointLimit);
        node->getAttr("max_force", rp.maxForce);
        node->getAttr("tension", rp.tension);
        node->getAttr("max_iters", rp.nMaxIters);
        node->getAttr("max_time_step", rp.maxTimeStep);
        node->getAttr("stiffness", rp.stiffness);
        node->getAttr("hardness", rp.hardness);
        node->getAttr("damping", rp.damping);
        node->getAttr("sleepSpeed", rp.sleepSpeed);
    }
    else
    {
        // Save.
        node->setAttr("flags", rp.nFlags);
        node->setAttr("radius", rp.fThickness);
        node->setAttr("anchor_radius", rp.fAnchorRadius);
        node->setAttr("num_seg", rp.nNumSegments);
        node->setAttr("num_sides", rp.nNumSides);
        node->setAttr("radius", rp.fThickness);
        node->setAttr("texu", rp.fTextureTileU);
        node->setAttr("texv", rp.fTextureTileV);
        node->setAttr("ph_num_seg", rp.nPhysSegments);
        node->setAttr("ph_sub_vtxs", rp.nMaxSubVtx);
        node->setAttr("mass", rp.mass);
        node->setAttr("friction", rp.friction);
        node->setAttr("friction_pull", rp.frictionPull);
        node->setAttr("wind", rp.wind);
        node->setAttr("wind_var", rp.windVariance);
        node->setAttr("air_resist", rp.airResistance);
        node->setAttr("water_resist", rp.waterResistance);
        node->setAttr("joint_lim", rp.jointLimit);
        node->setAttr("max_force", rp.maxForce);
        node->setAttr("tension", rp.tension);
        node->setAttr("max_iters", rp.nMaxIters);
        node->setAttr("max_time_step", rp.maxTimeStep);
        node->setAttr("stiffness", rp.stiffness);
        node->setAttr("hardness", rp.hardness);
        node->setAttr("damping", rp.damping);
        node->setAttr("sleepSpeed", rp.sleepSpeed);
    }
}

namespace
{
    CRopePanelUI* s_ropePanelUI = NULL;
    int s_ropePanelUI_Id = 0;
}

//////////////////////////////////////////////////////////////////////////
CRopeObject::CRopeObject()
{
    mv_closed = false;
    //mv_castShadow = true;
    //mv_recvShadow = true;

    m_bPerVertexHeight = false;
    SetColor(LINE_DISCONNECTED_COLOR);

    m_entityClass = "RopeEntity";

    ZeroStruct(m_ropeParams);

    m_ropeParams.nFlags = IRopeRenderNode::eRope_BindEndPoints | IRopeRenderNode::eRope_CheckCollisinos;
    m_ropeParams.fThickness = 0.02f;
    m_ropeParams.fAnchorRadius = 0.1f;
    m_ropeParams.nNumSegments = 8;
    m_ropeParams.nNumSides = 4;
    m_ropeParams.nMaxSubVtx = 3;
    m_ropeParams.nPhysSegments = 8;
    m_ropeParams.mass = 1.0f;
    m_ropeParams.friction = 2;
    m_ropeParams.frictionPull = 2;
    m_ropeParams.wind.Set(0, 0, 0);
    m_ropeParams.windVariance = 0;
    m_ropeParams.waterResistance = 0;
    m_ropeParams.jointLimit = 0;
    m_ropeParams.maxForce = 0;
    m_ropeParams.airResistance = 0;
    m_ropeParams.fTextureTileU = 1.0f;
    m_ropeParams.fTextureTileV = 10.0f;
    m_ropeParams.nMaxIters = 650;
    m_ropeParams.maxTimeStep = 0.02f;
    m_ropeParams.stiffness = 10.0f;
    m_ropeParams.hardness = 20.0f;
    m_ropeParams.damping = 0.2f;
    m_ropeParams.sleepSpeed = 0.04f;

    memset(&m_linkedObjects, 0, sizeof(m_linkedObjects));

    m_endLinksDisplayUpdateCounter = 0;

    m_bIsSimulating = false;
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::InitVariables()
{
}

/////////////////////////////////////////////////////// ///////////////////
bool CRopeObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    bool bResult =
        CShapeObject::Init(ie, prev, file);
    if (bResult && prev)
    {
        m_ropeParams = ((CRopeObject*)prev)->m_ropeParams;
    }
    else
    {
    }
    return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::Done()
{
    CShapeObject::Done();
}

//////////////////////////////////////////////////////////////////////////
bool CRopeObject::CreateGameObject()
{
    CShapeObject::CreateGameObject();
    if (GetIEntity())
    {
        m_bAreaModified = true;
        UpdateGameArea(false);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::InvalidateTM(int nWhyFlags)
{
    CShapeObject::InvalidateTM(nWhyFlags);

    m_endLinksDisplayUpdateCounter = 0;
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::SetMaterial(CMaterial* mtl)
{
    CShapeObject::SetMaterial(mtl);
    IRopeRenderNode* pRopeNode = GetRenderNode();
    if (pRopeNode != NULL)
    {
        if (mtl)
        {
            mtl->AssignToEntity(pRopeNode);
        }
        else
        {
            pRopeNode->SetMaterial(0);
        }
        uint8 nMaterialLayersFlags = GetMaterialLayersMask();
        pRopeNode->SetMaterialLayers(nMaterialLayersFlags);
    }
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::Display(DisplayContext& dc)
{
    bool bPrevShowIcons = gSettings.viewports.bShowIcons;
    const Matrix34& wtm = GetWorldTM();

    bool bLineWidth = 0;

    if (m_points.size() > 1)
    {
        IRopeRenderNode* pRopeNode = GetRenderNode();
        int nLinkedMask = 0;
        if (pRopeNode)
        {
            nLinkedMask = pRopeNode->GetLinkedEndsMask();
        }

        m_endLinksDisplayUpdateCounter++;
        if (pRopeNode && m_endLinksDisplayUpdateCounter > 10 && gEnv->pPhysicalWorld->GetPhysVars()->lastTimeStep == 0.0f)
        {
            m_endLinksDisplayUpdateCounter = 0;

            IRopeRenderNode::SEndPointLink links[2];
            pRopeNode->GetEndPointLinks(links);

            float s = m_ropeParams.fAnchorRadius;

            bool bCalcBBox = false;
            for (int i = 0; i < 2; i++)
            {
                if (links[i].pPhysicalEntity && m_linkedObjects[i].m_nPhysEntityId == 0)
                {
                    bCalcBBox = true;
                }
                else if (m_linkedObjects[i].m_nPhysEntityId && links[i].pPhysicalEntity)
                {
                    pe_status_pos ppos;
                    int nPointIndex = (i == 0) ? 0 : GetPointCount() - 1;
                    IPhysicalEntity* pPhysEnt = gEnv->pPhysicalWorld->GetPhysicalEntityById(m_linkedObjects[i].m_nPhysEntityId);
                    if (pPhysEnt != links[i].pPhysicalEntity)
                    {
                        links[i].pPhysicalEntity->GetStatus(&ppos);
                        bCalcBBox = true;
                    }
                    else
                    {
                        pPhysEnt->GetStatus(&ppos);
                        if (!IsEquivalent(m_linkedObjects[i].object_pos, ppos.pos) || !Quat::IsEquivalent(m_linkedObjects[i].object_q, ppos.q))
                        {
                            Matrix34 tm(ppos.q);
                            tm.SetTranslation(ppos.pos);
                            m_points[nPointIndex] = wtm.GetInverted().TransformPoint(tm.TransformPoint(m_linkedObjects[i].offset));
                            bCalcBBox = true;
                        }
                    }
                    if (bCalcBBox)
                    {
                        m_linkedObjects[i].object_pos = ppos.pos;
                        m_linkedObjects[i].object_q = ppos.q;
                    }
                }
                else
                {
                    m_linkedObjects[i].m_nPhysEntityId = 0;
                }
            }
            if (bCalcBBox)
            {
                CalcBBox();
            }
        }

        if (IsSelected())
        {
            gSettings.viewports.bShowIcons = false; // Selected Ropes hide icons.

            Vec3 p0 = wtm.TransformPoint(m_points[0]);
            Vec3 p1 = wtm.TransformPoint(m_points[m_points.size() - 1]);

            float s = m_ropeParams.fAnchorRadius;

            if (nLinkedMask & 0x01)
            {
                dc.SetColor(ColorB(0, 255, 0));
            }
            else
            {
                dc.SetColor(ColorB(255, 0, 0));
            }
            dc.DrawWireBox(p0 - Vec3(s, s, s), p0 + Vec3(s, s, s));

            if (nLinkedMask & 0x02)
            {
                dc.SetColor(ColorB(0, 255, 0));
            }
            else
            {
                dc.SetColor(ColorB(255, 0, 0));
            }
            dc.DrawWireBox(p1 - Vec3(s, s, s), p1 + Vec3(s, s, s));
        }

        if ((nLinkedMask & 3) == 3)
        {
            SetColor(LINE_CONNECTED_COLOR);
        }
        else
        {
            SetColor(LINE_DISCONNECTED_COLOR);
        }
    }

    CShapeObject::Display(dc);
    gSettings.viewports.bShowIcons = bPrevShowIcons;
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::UpdateGameArea(bool bRemove)
{
    if (bRemove)
    {
        return;
    }
    if (!m_bAreaModified)
    {
        return;
    }

    IRopeRenderNode* pRopeNode = GetRenderNode();
    if (pRopeNode)
    {
        std::vector<Vec3> points;
        if (GetPointCount() > 1)
        {
            const Matrix34& wtm = GetWorldTM();
            points.resize(GetPointCount());
            for (int i = 0; i < GetPointCount(); i++)
            {
                points[i] = GetPoint(i);
            }
            if (!m_points[0].IsEquivalent(m_points[1]))
            {
                pRopeNode->SetMatrix(GetWorldTM());

                pRopeNode->SetName(GetName().toUtf8().data());
                CMaterial* mtl = GetMaterial();
                if (mtl)
                {
                    mtl->AssignToEntity(pRopeNode);
                }

                uint8 nMaterialLayersFlags = GetMaterialLayersMask();
                pRopeNode->SetMaterialLayers(nMaterialLayersFlags);

                pRopeNode->SetParams(m_ropeParams);

                pRopeNode->SetPoints(&points[0], points.size());

                IRopeRenderNode::SEndPointLink links[2];
                pRopeNode->GetEndPointLinks(links);
                for (int i = 0; i < 2; i++)
                {
                    m_linkedObjects[i].offset = links[i].offset;
                    m_linkedObjects[i].m_nPhysEntityId = gEnv->pPhysicalWorld->GetPhysicalEntityId(links[i].pPhysicalEntity);
                    if (links[i].pPhysicalEntity)
                    {
                        pe_status_pos ppos;
                        links[i].pPhysicalEntity->GetStatus(&ppos);
                        m_linkedObjects[i].object_pos = ppos.pos;
                        m_linkedObjects[i].object_q = ppos.q;
                    }
                }

                // Both ends are connected.
                if (links[0].pPhysicalEntity && links[1].pPhysicalEntity)
                {
                    //m_linkedObjects[0].pObject = GetObjectManager()->FindPhysicalObjectOwner(links[0].pPhysicalEntity);

                    //SetColor(LINE_CONNECTED_COLOR);
                }
                else
                {
                    //SetColor(LINE_DISCONNECTED_COLOR);
                }
            }
        }

        // Stop a sound if we're simulating
        if (m_bIsSimulating)
        {
            pRopeNode->ResetRopeSound();
        }

        m_bAreaModified = false;
    }
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::OnParamChange(IVariable* var)
{
    if (!m_bIgnoreGameUpdate)
    {
        m_bAreaModified = true;
        UpdateGameArea(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::BeginEditParams(IEditor* ie, int flags)
{
    CBaseObject::BeginEditParams(ie, flags);  // First add standart dialogs.

    if (!s_ropePanelUI)
    {
        s_ropePanelUI = new CRopePanelUI();
        s_ropePanelUI->Setup();
        s_ropePanelUI_Id = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, tr("Rope Params"), s_ropePanelUI);
    }
    s_ropePanelUI->SetMultiSelect(false);
    s_ropePanelUI->SetObject(this);
    s_ropePanelUI->SetUpdateObjectCallback(functor(*this, &CBaseObject::OnPropertyChanged));

    if (!m_panel)
    {
        auto panel = new CRopePanel;
        m_panel = panel;
        m_rollupId = ie->AddRollUpPage(ROLLUP_OBJECTS, "Edit Rope", panel);
    }
    if (m_panel)
    {
        m_panel->SetShape(this);
    }

    CEntityObject::BeginEditParams(ie, flags);  // At the end add entity dialogs.
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::EndEditParams(IEditor* ie)
{
    if (s_ropePanelUI)
    {
        s_ropePanelUI->ClearUpdateCallback();
    }

    CShapeObject::EndEditParams(ie);

    if (s_ropePanelUI)
    {
        s_ropePanelUI->DeleteVars();
        GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, s_ropePanelUI_Id);
        s_ropePanelUI = 0;
        s_ropePanelUI_Id = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::BeginEditMultiSelParams(bool bAllOfSameType)
{
    CEntityObject::BeginEditMultiSelParams(bAllOfSameType);
    if (bAllOfSameType)
    {
        if (!s_ropePanelUI)
        {
            s_ropePanelUI = new CRopePanelUI();
            s_ropePanelUI->Setup();
            s_ropePanelUI_Id = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, tr("Rope Params"), s_ropePanelUI);
        }
        s_ropePanelUI->SetObject(this);
        s_ropePanelUI->SetMultiSelect(true);
        if (s_ropePanelUI)
        {
            s_ropePanelUI->SetUpdateObjectCallback(functor(*this, &CBaseObject::OnMultiSelPropertyChanged));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::EndEditMultiSelParams()
{
    if (s_ropePanelUI)
    {
        s_ropePanelUI->ClearUpdateCallback();
    }

    CShapeObject::EndEditMultiSelParams();

    if (s_ropePanelUI)
    {
        GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, s_ropePanelUI_Id);
        s_ropePanelUI = 0;
        s_ropePanelUI_Id = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::OnEvent(ObjectEvent event)
{
    CShapeObject::OnEvent(event);
    switch (event)
    {
    case EVENT_INGAME:
    case EVENT_OUTOFGAME:
    {
        IRopeRenderNode* pRenderNode = GetRenderNode();
        if (pRenderNode)
        {
            pRenderNode->ResetPoints();
        }

        m_bIsSimulating = (event == EVENT_INGAME);
        UpdateSoundData();
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::Serialize(CObjectArchive& ar)
{
    if (ar.bLoading)
    {
        // Loading.
        XmlNodeRef xmlNodeRope = ar.node->findChild("Rope");
        if (xmlNodeRope)
        {
            RopeParamsToXml(m_ropeParams, xmlNodeRope, ar.bLoading);

            // Sound related
            XmlNodeRef const xmlNodeSound = xmlNodeRope->findChild("Sound");
            if (xmlNodeSound)
            {
                xmlNodeSound->getAttr("Name", m_ropeSoundData.sName);
                xmlNodeSound->getAttr("SegmentToAttachTo", m_ropeSoundData.nSegementToAttachTo);
                xmlNodeSound->getAttr("Offset", m_ropeSoundData.fOffset);
            }
        }
    }
    else
    {
        // Saving.
        XmlNodeRef xmlNodeRope = ar.node->newChild("Rope");
        RopeParamsToXml(m_ropeParams, xmlNodeRope, ar.bLoading);

        // Sound related
        if (!m_ropeSoundData.sName.isEmpty())
        {
            XmlNodeRef const xmlNodeSound = xmlNodeRope->newChild("Sound");
            xmlNodeSound->setAttr("Name", m_ropeSoundData.sName.toUtf8().data());
            xmlNodeSound->setAttr("SegmentToAttachTo", m_ropeSoundData.nSegementToAttachTo);
            xmlNodeSound->setAttr("Offset", m_ropeSoundData.fOffset);
        }
    }

    CShapeObject::Serialize(ar);
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::PostLoad(CObjectArchive& ar)
{
    CShapeObject::PostLoad(ar);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CRopeObject::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    XmlNodeRef objNode = CShapeObject::Export(levelPath, xmlNode);
    if (objNode)
    {
        // Export Rope params.
        XmlNodeRef xmlNodeRope = objNode->newChild("Rope");
        RopeParamsToXml(m_ropeParams, xmlNodeRope, false);

        // Export Points
        if (!m_points.empty())
        {
            const Matrix34& wtm = GetWorldTM();
            XmlNodeRef points = xmlNodeRope->newChild("Points");
            for (int i = 0; i < m_points.size(); i++)
            {
                XmlNodeRef pnt = points->newChild("Point");
                pnt->setAttr("Pos", m_points[i]);
            }
        }

        // Sound related
        if (!m_ropeSoundData.sName.isEmpty())
        {
            XmlNodeRef const xmlNodeSound = xmlNodeRope->newChild("Sound");
            xmlNodeSound->setAttr("Name", m_ropeSoundData.sName.toUtf8().data());
            xmlNodeSound->setAttr("SegmentToAttachTo", m_ropeSoundData.nSegementToAttachTo);
            xmlNodeSound->setAttr("Offset", m_ropeSoundData.fOffset);
        }
    }
    return objNode;
}

//////////////////////////////////////////////////////////////////////////
IRopeRenderNode* CRopeObject::GetRenderNode() const
{
    if (m_pEntity)
    {
        IComponentRopePtr pRopeComponent = m_pEntity->GetOrCreateComponent<IComponentRope>();
        if (pRopeComponent)
        {
            return pRopeComponent->GetRopeRenderNode();
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::CalcBBox()
{
    if (m_points.size() < 2)
    {
        m_bbox.min = m_bbox.max = Vec3(0, 0, 0);
        return;
    }

    // When UnSelecting.
    // Reposition rope, so that rope object position is in the middle of the rope.

    const Matrix34& wtm = GetWorldTM();

    Vec3 wp0 = wtm.TransformPoint(m_points[0]);
    Vec3 wp1 = wtm.TransformPoint(m_points[1]);

    Vec3 center = (wp0 + wp1) * 0.5f;
    if (!center.IsEquivalent(wtm.GetTranslation(), 0.01f))
    {
        // Center should move.
        Vec3 offset = wtm.GetInverted().TransformVector(center - wtm.GetTranslation());
        for (int i = 0; i < m_points.size(); i++)
        {
            m_points[i] -= offset;
        }
        Matrix34 ltm = GetLocalTM();
        CUndoSuspend undoSuspend;
        SetPos(GetPos() + wtm.TransformVector(offset));
    }

    // First point must always be 0,0,0.
    m_bbox.Reset();
    for (int i = 0; i < m_points.size(); i++)
    {
        m_bbox.Add(m_points[i]);
    }
    if (m_bbox.min.x > m_bbox.max.x)
    {
        m_bbox.min = m_bbox.max = Vec3(0, 0, 0);
    }
    AABB box;
    box.SetTransformedAABB(GetWorldTM(), m_bbox);
    m_lowestHeight = box.min.z;
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::SetSelected(bool bSelect)
{
    if (IsSelected() && !bSelect && GetPointCount() > 1)
    {
        CalcBBox();
    }
    CShapeObject::SetSelected(bSelect);
}

//////////////////////////////////////////////////////////////////////////
void CRopeObject::UpdateSoundData()
{
    IRopeRenderNode* const pRopeRenderNode = GetRenderNode();
    if (pRopeRenderNode)
    {
        if (m_bIsSimulating)
        {
            pRopeRenderNode->SetRopeSound(m_ropeSoundData.sName.toUtf8().data(), m_ropeSoundData.nSegementToAttachTo, m_ropeSoundData.fOffset);
        }
        else
        {
            pRopeRenderNode->StopRopeSound();
        }
    }
}

#include <Objects/RopeObject.moc>