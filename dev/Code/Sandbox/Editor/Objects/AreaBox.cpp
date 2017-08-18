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

// Description : CAreaBox implementation.


#include "StdAfx.h"
#include "AreaBox.h"
#include "Viewport.h"
#include <Controls/ReflectedPropertyControl/ReflectedPropertiesPanel.h>
#include "PickEntitiesPanel.h"

#include <IEntity.h>
#include <IEntityHelper.h>
#include "Components/IComponentArea.h"

CPickEntitiesPanel* CAreaObjectBase::m_pEntitiesPickPanel = NULL;
int CAreaObjectBase::m_nEntitiesPickRollupId = 0;

//////////////////////////////////////////////////////////////////////////
void CAreaObjectBase::BeginEditParams(IEditor* ie, int flags)
{
    CEntityObject::BeginEditParams(ie, flags);
    if (!m_pEntitiesPickPanel)
    {
        m_pEntitiesPickPanel = new CPickEntitiesPanel;
        m_nEntitiesPickRollupId = ie->AddRollUpPage(ROLLUP_OBJECTS, "Attached Entities", m_pEntitiesPickPanel);
    }
    if (m_pEntitiesPickPanel)
    {
        m_pEntitiesPickPanel->SetOwner(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAreaObjectBase::EndEditParams(IEditor* ie)
{
    if (m_nEntitiesPickRollupId)
    {
        ie->RemoveRollUpPage(ROLLUP_OBJECTS, m_nEntitiesPickRollupId);
    }
    m_nEntitiesPickRollupId = 0;
    m_pEntitiesPickPanel = NULL;
    CEntityObject::EndEditParams(ie);
}

//////////////////////////////////////////////////////////////////////////
void CAreaObjectBase::PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx)
{
    CEntityObject::PostClone(pFromObject, ctx);

    CAreaObjectBase* pFrom = (CAreaObjectBase*)pFromObject;
    // Clone event targets.
    if (!pFrom->m_entities.empty())
    {
        int numEntities = pFrom->m_entities.size();
        for (int i = 0; i < numEntities; i++)
        {
            GUID guid = ctx.ResolveClonedID(pFrom->m_entities[i]);
            if (guid != GUID_NULL)
            {
                m_entities.push_back(guid);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAreaObjectBase::AddEntity(CBaseObject* pEntity)
{
    assert(pEntity);
    // Check if this entity already binded.
    if (std::find(m_entities.begin(), m_entities.end(), pEntity->GetId()) != m_entities.end())
    {
        return;
    }

    StoreUndo("Add Entity");
    m_entities.push_back(pEntity->GetId());
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
void CAreaObjectBase::RemoveEntity(int index)
{
    assert(index >= 0 && index < m_entities.size());
    StoreUndo("Remove Entity");

    m_entities.erase(m_entities.begin() + index);
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
CEntityObject* CAreaObjectBase::GetEntity(int index)
{
    assert(index >= 0 && index < m_entities.size());
    CBaseObject* pObject = GetObjectManager()->FindObject(m_entities[index]);
    if (qobject_cast<CEntityObject*>(pObject))
    {
        return (CEntityObject*)pObject;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CAreaObjectBase::Serialize(CObjectArchive& ar)
{
    CEntityObject::Serialize(ar);

    XmlNodeRef xmlNode = ar.node;
    if (ar.bLoading)
    {
        m_entities.clear();
        XmlNodeRef entities = xmlNode->findChild("Entities");
        if (entities)
        {
            for (int i = 0; i < entities->getChildCount(); i++)
            {
                XmlNodeRef ent = entities->getChild(i);
                GUID entityId;
                ent->getAttr("Id", entityId);
                entityId = ar.ResolveID(entityId);
                m_entities.push_back(entityId);
            }
        }
    }
    else
    {
        if (!m_entities.empty())
        {
            XmlNodeRef nodes = xmlNode->newChild("Entities");
            for (int i = 0; i < m_entities.size(); i++)
            {
                XmlNodeRef entNode = nodes->newChild("Entity");
                entNode->setAttr("Id", m_entities[i]);
            }
        }
    }
}

void CAreaObjectBase::Reload(bool bReloadScript)
{
    CEntityObject::Reload(bReloadScript);
    OnAreaChange(NULL);
}

void CAreaObjectBase::DrawEntityLinks(DisplayContext& dc)
{
    if (m_entities.empty())
    {
        return;
    }

    Vec3 vcol = Vec3(1, 1, 1);
    for (int i = 0; i < m_entities.size(); i++)
    {
        CBaseObject* obj = GetEntity(i);
        if (!obj)
        {
            continue;
        }
        dc.DrawLine(GetWorldPos(), obj->GetWorldPos(), ColorF(vcol.x, vcol.y, vcol.z, 0.7f), ColorF(1, 1, 1, 0.7f));
    }
}

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

int CAreaBox::m_rollupMultyId = 0;
ReflectedPropertiesPanel* CAreaBox::m_pSoundPropertiesPanel = nullptr;
int CAreaBox::m_nSoundPanelID = 0;
CVarBlockPtr CAreaBox::m_pSoundPanelVarBlock = NULL;

//////////////////////////////////////////////////////////////////////////
CAreaBox::CAreaBox()
{
    m_areaId = -1;
    m_edgeWidth = 0;
    mv_width = 4;
    mv_length = 4;
    mv_height = 1;
    mv_groupId = 0;
    mv_priority = 0;
    mv_filled = false;
    mv_displaySoundInfo = false;
    m_entityClass = "AreaBox";

    // Resize for 6 sides
    m_abObstructSound.resize(6);

    m_box.min = Vec3(-mv_width / 2, -mv_length / 2, 0);
    m_box.max = Vec3(mv_width / 2, mv_length / 2, mv_height);

    m_bIgnoreGameUpdate = 1;

    if (!m_pSoundPanelVarBlock) // Static
    {
        m_pSoundPanelVarBlock = new CVarBlock;
    }
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::InitVariables()
{
    AddVariable(m_areaId, "AreaId", functor(*this, &CAreaBox::OnAreaChange));
    AddVariable(m_edgeWidth, "FadeInZone", functor(*this, &CAreaBox::OnSizeChange));
    AddVariable(mv_width, "Width", functor(*this, &CAreaBox::OnSizeChange));
    AddVariable(mv_length, "Length", functor(*this, &CAreaBox::OnSizeChange));
    AddVariable(mv_height, "Height", functor(*this, &CAreaBox::OnSizeChange));
    AddVariable(mv_groupId, "GroupId", functor(*this, &CAreaBox::OnAreaChange));
    AddVariable(mv_priority, "Priority", functor(*this, &CAreaBox::OnAreaChange));
    AddVariable(mv_filled, "DisplayFilled", functor(*this, &CAreaBox::OnAreaChange));
    AddVariable(mv_displaySoundInfo, "DisplaySoundInfo", functor(*this, &CAreaBox::OnSoundParamsChange));
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::Done()
{
    CEntityObject::Done();
}

//////////////////////////////////////////////////////////////////////////
bool CAreaBox::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    m_bIgnoreGameUpdate = 1;

    SetColor(QColor(0, 0, 255));
    bool res = CEntityObject::Init(ie, prev, file);

    if (m_pEntity)
    {
        m_pEntity->GetOrCreateComponent<IComponentArea>();
    }

    if (prev)
    {
        m_abObstructSound   = ((CAreaBox*)prev)->m_abObstructSound;
        assert(m_abObstructSound.size() == 6);
    }

    m_bIgnoreGameUpdate = 0;

    return res;
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::GetLocalBounds(AABB& box)
{
    box = m_box;
}

//////////////////////////////////////////////////////////////////////////
bool CAreaBox::HitTest(HitContext& hc)
{
    Vec3 p;
    /*BBox box;
    box = m_box;
    box.Transform( GetWorldTM() );
    float tr = hc.distanceTollerance/2;
    box.min -= Vec3(tr,tr,tr);
    box.max += Vec3(tr,tr,tr);
    if (box.IsIntersectRay(hc.raySrc,hc.rayDir,p ))
    {
        hc.dist = Vec3(hc.raySrc - p).Length();
        return true;
    }*/
    Matrix34 invertWTM = GetWorldTM();
    Vec3 worldPos = invertWTM.GetTranslation();
    invertWTM.Invert();

    Vec3 xformedRaySrc = invertWTM.TransformPoint(hc.raySrc);
    Vec3 xformedRayDir = invertWTM.TransformVector(hc.rayDir).GetNormalized();

    float epsilonDist = max(.1f, hc.view->GetScreenScaleFactor(worldPos) * 0.01f);
    epsilonDist *= max(0.0001f, min(invertWTM.GetColumn0().GetLength(), min(invertWTM.GetColumn1().GetLength(), invertWTM.GetColumn2().GetLength())));
    float hitDist;

    float tr = hc.distanceTolerance / 2 + 1;
    AABB box;
    box.min = m_box.min - Vec3(tr + epsilonDist, tr + epsilonDist, tr + epsilonDist);
    box.max = m_box.max + Vec3(tr + epsilonDist, tr + epsilonDist, tr + epsilonDist);

    if (Intersect::Ray_AABB(xformedRaySrc, xformedRayDir, box, p))
    {
        if (Intersect::Ray_AABBEdge(xformedRaySrc, xformedRayDir, m_box, epsilonDist, hitDist, p))
        {
            hc.dist = xformedRaySrc.GetDistance(p);
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::BeginEditParams(IEditor* ie, int flags)
{
    CAreaObjectBase::BeginEditParams(ie, flags);

    // Make sure to first remove any old sound-obstruction-roll-up-page in case EndEditParams() didn't get called on a previous instance
    if (m_nSoundPanelID != 0)
    {
        // Internally a var block holds "IVariablePtr", on destroy delete is already called
        m_pSoundPanelVarBlock->DeleteAllVariables();

        ie->RemoveRollUpPage(ROLLUP_OBJECTS, m_nSoundPanelID);
        m_nSoundPanelID = 0;
        m_pSoundPropertiesPanel = new ReflectedPropertiesPanel;
        m_pSoundPropertiesPanel->Setup();
    }
    else if (!m_pSoundPropertiesPanel) // Static
    {
        m_pSoundPropertiesPanel = new ReflectedPropertiesPanel;
        m_pSoundPropertiesPanel->Setup();
    }

    if (m_bIgnoreGameUpdate == 0)
    {
        UpdateSoundPanelParams();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::EndEditParams(IEditor* ie)
{
    if (m_nSoundPanelID != 0)
    {
        // Internally a var block holds "IVariablePtr", on destroy delete is already called
        m_pSoundPanelVarBlock->DeleteAllVariables();

        ie->RemoveRollUpPage(ROLLUP_OBJECTS, m_nSoundPanelID);
        m_nSoundPanelID                 = 0;
        m_pSoundPropertiesPanel = NULL;
    }

    CAreaObjectBase::EndEditParams(ie);
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::BeginEditMultiSelParams(bool bAllOfSameType)
{
    CAreaObjectBase::BeginEditMultiSelParams(bAllOfSameType);

    if (!bAllOfSameType)
    {
        return;
    }

    // Set to 1 to indicate that we're multi-selecting
    m_rollupMultyId = 1;
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::EndEditMultiSelParams()
{
    // We're done with multi-selection
    m_rollupMultyId = 0;

    CAreaObjectBase::EndEditMultiSelParams();
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::OnAreaChange(IVariable* pVar)
{
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::OnSizeChange(IVariable* pVar)
{
    Vec3 size(0, 0, 0);
    size.x = mv_width;
    size.y = mv_length;
    size.z = mv_height;

    m_box.min = -size / 2;
    m_box.max = size / 2;
    // Make volume position bottom of bounding box.
    m_box.min.z = 0;
    m_box.max.z = size.z;
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::Display(DisplayContext& dc)
{
    if (!mv_displaySoundInfo)
    {
        QColor wireColor, solidColor;
        float wireOffset = 0;
        float alpha = 0.3f;
        if (IsSelected())
        {
            wireColor = dc.GetSelectedColor();
            solidColor = GetColor();
            wireOffset = -0.1f;
        }
        else
        {
            wireColor = GetColor();
            solidColor = GetColor();
        }

        //dc.renderer->SetCullMode( R_CULL_DISABLE );

        dc.PushMatrix(GetWorldTM());
        AABB box = m_box;

        bool bFrozen = IsFrozen();

        if (bFrozen)
        {
            dc.SetFreezeColor();
        }
        //////////////////////////////////////////////////////////////////////////
        if (!bFrozen)
        {
            dc.SetColor(solidColor, alpha);
        }

        if (IsSelected())
        {
            dc.DepthWriteOff();
            dc.DrawSolidBox(box.min, box.max);
            dc.DepthWriteOn();
        }

        if (!bFrozen)
        {
            dc.SetColor(wireColor, 1);
        }

        if (IsSelected())
        {
            dc.SetLineWidth(3.0f);
            dc.DrawWireBox(box.min, box.max);
            dc.SetLineWidth(0);
        }
        else
        {
            dc.DrawWireBox(box.min, box.max);
        }
        if (m_edgeWidth)
        {
            float fFadeScale = m_edgeWidth / 200.0f;
            AABB InnerBox = box;
            Vec3 EdgeDist = InnerBox.max - InnerBox.min;
            InnerBox.min.x += EdgeDist.x * fFadeScale;
            InnerBox.max.x -= EdgeDist.x * fFadeScale;
            InnerBox.min.y += EdgeDist.y * fFadeScale;
            InnerBox.max.y -= EdgeDist.y * fFadeScale;
            InnerBox.min.z += EdgeDist.z * fFadeScale;
            InnerBox.max.z -= EdgeDist.z * fFadeScale;
            dc.DrawWireBox(InnerBox.min, InnerBox.max);
        }
        if (mv_filled)
        {
            dc.SetAlpha(0.2f);
            dc.DrawSolidBox(box.min, box.max);
        }
        //////////////////////////////////////////////////////////////////////////

        dc.PopMatrix();

        DrawEntityLinks(dc);
        DrawDefault(dc);
    }
    else
    {
        ColorB const oObstructionFilled(255, 0, 0, 120);
        ColorB const oObstructionFilledNotSelected(255, 0, 0, 30);
        ColorB const oObstructionNotFilled(255, 0, 0, 10);
        ColorB const oNoObstructionFilled(0, 255, 0, 120);
        ColorB const oNoObstructionFilledNotSelected(0, 255, 0, 30);
        ColorB const oNoObstructionNotFilled(0, 255, 0, 10);

        QColor oWireColor;
        if (IsSelected())
        {
            oWireColor = dc.GetSelectedColor();
        }
        else
        {
            oWireColor = GetColor();
        }

        dc.SetColor(oWireColor);

        dc.PushMatrix(GetWorldTM());

        if (mv_height > 0.0f)
        {
            dc.DrawWireBox(m_box.min, m_box.max);
        }

        Vec3 const v3BoxMin = m_box.min;
        Vec3 const v3BoxMax = m_box.max;

        // Now build the 6 segments
        float const fLength = v3BoxMax.x - v3BoxMin.x;
        float const fWidth  = v3BoxMax.y - v3BoxMin.y;
        float const fHeight = v3BoxMax.z - v3BoxMin.z;

        Vec3 const v0(v3BoxMin);
        Vec3 const v1(Vec3(v3BoxMin.x + fLength,  v3BoxMin.y,                 v3BoxMin.z));
        Vec3 const v2(Vec3(v3BoxMin.x + fLength,  v3BoxMin.y + fWidth,  v3BoxMin.z));
        Vec3 const v3(Vec3(v3BoxMin.x,                  v3BoxMin.y + fWidth,  v3BoxMin.z));
        Vec3 const v4(Vec3(v3BoxMin.x,                  v3BoxMin.y,                 v3BoxMin.z + fHeight));
        Vec3 const v5(Vec3(v3BoxMin.x + fLength,  v3BoxMin.y,                 v3BoxMin.z + fHeight));
        Vec3 const v6(Vec3(v3BoxMin.x + fLength,  v3BoxMin.y + fWidth,  v3BoxMin.z + fHeight));
        Vec3 const v7(Vec3(v3BoxMin.x,                  v3BoxMin.y + fWidth,  v3BoxMin.z + fHeight));

        // Describe the box faces
        SBoxFace const aoBoxFaces[6] =
        {
            SBoxFace(&v0, &v4, &v5, &v1),
            SBoxFace(&v1, &v5, &v6, &v2),
            SBoxFace(&v2, &v6, &v7, &v3),
            SBoxFace(&v3, &v7, &v4, &v0),
            SBoxFace(&v4, &v5, &v6, &v7),
            SBoxFace(&v0, &v1, &v2, &v3)
        };

        // Draw each side
        unsigned int const nSideCount = ((mv_height > 0.0f) ? 6 : 4);
        for (unsigned int i = 0; i < nSideCount; ++i)
        {
            if (mv_height == 0.0f)
            {
                if (IsSelected())
                {
                    if (m_abObstructSound[i])
                    {
                        dc.SetColor(oObstructionFilled);
                    }
                    else
                    {
                        dc.SetColor(oNoObstructionFilled);
                    }
                }
                else
                {
                    dc.SetColor(oWireColor);
                }

                dc.DrawLine(*(aoBoxFaces[i].pP1), *(aoBoxFaces[i].pP4));
            }

            if (mv_height > 0.0f)
            {
                // Manipulate the color so it looks a little thicker and redder
                if (mv_filled || gSettings.viewports.bFillSelectedShapes)
                {
                    ColorB const nColor = dc.GetColor();

                    if (m_abObstructSound[i])
                    {
                        dc.SetColor(IsSelected() ? oObstructionFilled : oObstructionFilledNotSelected);
                    }
                    else
                    {
                        dc.SetColor(IsSelected() ? oNoObstructionFilled : oNoObstructionFilledNotSelected);
                    }

                    dc.CullOff();
                    dc.DepthWriteOff();

                    dc.DrawQuad(*(aoBoxFaces[i].pP1), *(aoBoxFaces[i].pP2), *(aoBoxFaces[i].pP3), *(aoBoxFaces[i].pP4));

                    dc.DepthWriteOn();
                    dc.CullOn();
                    dc.SetColor(nColor);
                }
                else if (IsSelected())
                {
                    ColorB const nColor = dc.GetColor();

                    if (m_abObstructSound[i])
                    {
                        dc.SetColor(oObstructionNotFilled);
                    }
                    else
                    {
                        dc.SetColor(oNoObstructionNotFilled);
                    }

                    dc.CullOff();
                    dc.DepthWriteOff();

                    dc.DrawQuad(*(aoBoxFaces[i].pP1), *(aoBoxFaces[i].pP2), *(aoBoxFaces[i].pP3), *(aoBoxFaces[i].pP4));

                    dc.DepthWriteOn();
                    dc.CullOn();
                    dc.SetColor(nColor);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        dc.PopMatrix();

        DrawEntityLinks(dc);
        DrawDefault(dc);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::InvalidateTM(int nWhyFlags)
{
    CEntityObject::InvalidateTM(nWhyFlags);
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::Serialize(CObjectArchive& ar)
{
    m_bIgnoreGameUpdate = 1;
    CAreaObjectBase::Serialize(ar);
    m_bIgnoreGameUpdate = 0;

    XmlNodeRef xmlNode = ar.node;
    if (ar.bLoading)
    {
        AABB box;
        xmlNode->getAttr("BoxMin", box.min);
        xmlNode->getAttr("BoxMax", box.max);

        SetAreaId(m_areaId);
        SetBox(box);
    }
    else
    {
        // Saving.
        //      xmlNode->setAttr( "AreaId",m_areaId );
        xmlNode->setAttr("BoxMin", m_box.min);
        xmlNode->setAttr("BoxMax", m_box.max);
    }

    // Now serialize the sound variables
    if (m_pSoundPanelVarBlock)
    {
        if (ar.bLoading)
        {
            // Create the variables
            // First clear the remains out
            m_pSoundPanelVarBlock->DeleteAllVariables();

            for (size_t i = 0; i < ((mv_height > 0.0f) ? 6 : 4); ++i)
            {
                CVariable<bool>* const pvTemp = new CVariable<bool>();

                stack_string cTemp;
                cTemp.Format("Side%d", i + 1);

                // And add each to the block
                m_pSoundPanelVarBlock->AddVariable(pvTemp, cTemp);
            }

            // Now read in the data
            XmlNodeRef pSoundDataNode = xmlNode->findChild("SoundData");
            if (pSoundDataNode)
            {
                // Make sure we stay backwards compatible and also find the old "Side:" formatting
                // This can go out once every level got saved with the new formatting
                char const* const pcTemp = pSoundDataNode->getAttr("Side:1");
                if (pcTemp && pcTemp[0])
                {
                    // We have the old formatting
                    m_pSoundPanelVarBlock->DeleteAllVariables();

                    for (size_t i = 0; i < ((mv_height > 0.0f) ? 6 : 4); ++i)
                    {
                        CVariable<bool>* const pvTempOld = new CVariable<bool>();

                        stack_string cTempOld;
                        cTempOld.Format("Side:%d", i + 1);

                        // And add each to the block
                        m_pSoundPanelVarBlock->AddVariable(pvTempOld, cTempOld);
                    }
                }

                m_pSoundPanelVarBlock->Serialize(pSoundDataNode, true);
            }

            // Copy the data to the "bools" list
            // First clear the remains out
            m_abObstructSound.clear();

            unsigned int const nVarCount = m_pSoundPanelVarBlock->GetNumVariables();
            for (unsigned int i = 0; i < nVarCount; ++i)
            {
                IVariable const* const pTemp = m_pSoundPanelVarBlock->GetVariable(i);
                if (pTemp)
                {
                    bool bTemp = false;
                    pTemp->Get(bTemp);
                    m_abObstructSound.push_back(bTemp);

                    if (i >= nVarCount - 2)
                    {
                        // Here check for backwards compatibility reasons if "ObstructRoof" and "ObstructFloor" still exists
                        bool bTemp = false;
                        if (i == nVarCount - 2)
                        {
                            if (xmlNode->getAttr("ObstructRoof", bTemp))
                            {
                                m_abObstructSound[i] = bTemp;
                            }
                        }
                        else
                        {
                            if (xmlNode->getAttr("ObstructFloor", bTemp))
                            {
                                m_abObstructSound[i] = bTemp;
                            }
                        }
                    }
                }
            }

            // Since the var block is a static object clear it for the next object to be empty
            m_pSoundPanelVarBlock->DeleteAllVariables();
        }
        else
        {
            XmlNodeRef pSoundDataNode = xmlNode->newChild("SoundData");
            if (pSoundDataNode)
            {
                // TODO: What if mv_height or/and mv_width or/and mv_length == 0 ??
                // For now just serialize all data
                // First clear the remains out
                m_pSoundPanelVarBlock->DeleteAllVariables();

                // Create the variables
                assert(m_abObstructSound.size() == 6);

                size_t nIndex = 0;
                tSoundObstructionIterConst const ItEnd = m_abObstructSound.end();
                for (tSoundObstructionIterConst It = m_abObstructSound.begin(); It != ItEnd; ++It)
                {
                    bool const bObstructed  = (bool)(*It);

                    CVariable<bool>* const pvTemp = new CVariable<bool>();
                    pvTemp->Set(bObstructed);
                    stack_string cTemp;
                    cTemp.Format("Side%d", ++nIndex);

                    // And add each to the block
                    m_pSoundPanelVarBlock->AddVariable(pvTemp, cTemp);
                }

                m_pSoundPanelVarBlock->Serialize(pSoundDataNode, false);
                m_pSoundPanelVarBlock->DeleteAllVariables();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CAreaBox::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    XmlNodeRef objNode = CEntityObject::Export(levelPath, xmlNode);
    return objNode;
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::SetAreaId(int nAreaId)
{
    m_areaId = nAreaId;
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
int CAreaBox::GetAreaId()
{
    return m_areaId;
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::SetBox(AABB box)
{
    m_box = box;
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
AABB CAreaBox::GetBox()
{
    return m_box;
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::PostLoad(CObjectArchive& ar)
{
    CAreaObjectBase::PostLoad(ar);
    // After loading Update game structure.
    UpdateGameArea();
}

//////////////////////////////////////////////////////////////////////////
bool CAreaBox::CreateGameObject()
{
    bool bRes = CEntityObject::CreateGameObject();
    if (bRes)
    {
        UpdateGameArea();
    }
    return bRes;
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::UpdateGameArea()
{
    if (m_bIgnoreGameUpdate == 0 && m_pEntity)
    {
        IComponentAreaPtr pArea = m_pEntity->GetOrCreateComponent<IComponentArea>();
        if (!pArea)
        {
            return;
        }

        bool abObstructSound[6];

        size_t nIndex = 0;
        tSoundObstructionIterConst const ItEnd = m_abObstructSound.end();
        for (tSoundObstructionIterConst It = m_abObstructSound.begin(); It != ItEnd; ++It)
        {
            // Here we "unpack" the data! (1 bit*nPointsCount to 1 byte*nPointsCount)
            bool const bObstructed  = (bool)(*It);
            abObstructSound[nIndex] = bObstructed;
            ++nIndex;
        }

        UpdateSoundPanelParams();
        pArea->SetBox(m_box.min, m_box.max, &abObstructSound[0], 6);
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
    }
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::OnSoundParamsChange(IVariable* var)
{
    if (m_bIgnoreGameUpdate == 0)
    {
        if (!mv_displaySoundInfo)
        {
            if (m_nSoundPanelID != 0)
            {
                // Internally a var block holds "IVariablePtr", on destroy delete is already called
                m_pSoundPanelVarBlock->DeleteAllVariables();

                GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS, m_nSoundPanelID);
                m_nSoundPanelID                 = 0;
                m_pSoundPropertiesPanel = 0;
            }
        }
        else
        {
            if (!m_pSoundPropertiesPanel)
            {
                m_pSoundPropertiesPanel = new ReflectedPropertiesPanel;
                m_pSoundPropertiesPanel->Setup();
            }
        }

        UpdateSoundPanelParams();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::OnPointChange(IVariable* var)
{
    if (m_pSoundPanelVarBlock)
    {
        assert(m_abObstructSound.size() == 6);
        unsigned int const nVarCount = m_pSoundPanelVarBlock->GetNumVariables();
        for (unsigned int i = 0; i < nVarCount; ++i)
        {
            if (m_pSoundPanelVarBlock->GetVariable(i) == var)
            {
                if (m_pEntity)
                {
                    IComponentAreaPtr pArea = m_pEntity->GetOrCreateComponent<IComponentArea>();
                    if (pArea)
                    {
                        bool bValue = false;
                        var->Get(bValue);
                        pArea->SetSoundObstructionOnAreaFace(i, bValue);
                        m_abObstructSound[i] = bValue;
                    }
                }
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAreaBox::UpdateSoundPanelParams()
{
    if (!m_rollupMultyId && mv_displaySoundInfo && m_pSoundPropertiesPanel)
    {
        if (!m_nSoundPanelID)
        {
            m_nSoundPanelID = GetIEditor()->AddRollUpPage(ROLLUP_OBJECTS, "Sound Obstruction", m_pSoundPropertiesPanel);
        }

        m_pSoundPropertiesPanel->DeleteVars();

        if (m_nSoundPanelID)
        {
            if (m_pSoundPanelVarBlock)
            {
                if (!m_pSoundPanelVarBlock->IsEmpty())
                {
                    m_pSoundPanelVarBlock->DeleteAllVariables();
                }

                if (!m_abObstructSound.empty())
                {
                    // If the box hasn't got a height subtract 2 sides
                    assert(m_abObstructSound.size() == 6);
                    size_t const nVarCount = m_abObstructSound.size() - ((mv_height > 0.0f) ? 0 : 2);
                    for (size_t i = 0; i < nVarCount; ++i)
                    {
                        CVariable<bool>* const pvTemp = new CVariable<bool>();
                        pvTemp->Set(m_abObstructSound[i]);
                        pvTemp->AddOnSetCallback(functor(*this, &CAreaBox::OnPointChange));

                        stack_string cTemp;
                        cTemp.Format("Side:%d", i + 1);

                        m_pSoundPanelVarBlock->AddVariable(pvTemp, cTemp);
                    }
                }

                m_pSoundPropertiesPanel->AddVars(m_pSoundPanelVarBlock);
            }
        }
    }
}

#include <Objects/AreaBox.moc>