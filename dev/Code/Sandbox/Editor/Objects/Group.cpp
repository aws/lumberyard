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

// Description : CGroup implementation.


#include "StdAfx.h"
#include "Group.h"
#include "../Viewport.h"
#include "../DisplaySettings.h"
#include "../GroupPanel.h"
#include "EntityObject.h"

CGroupPanel* CGroup::s_pPanel = NULL;
int CGroup::s_panelID = 0;


//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CGroup::CGroup()
{
    m_opened = false;
    m_bAlwaysDrawBox = true;
    m_ignoreChildModify = false;
    m_bbox.min = m_bbox.max = Vec3(0, 0, 0);
    m_bBBoxValid = false;
    m_bUpdatingPivot = false;
    SetColor(QColor(0, 255, 0)); // Green
}

//////////////////////////////////////////////////////////////////////////
void CGroup::Done()
{
    DeleteAllMembers();
    CBaseObject::Done();
}

//////////////////////////////////////////////////////////////////////////
void CGroup::DeleteAllMembers()
{
    TBaseObjects members;
    for (size_t i = 0; i < m_members.size(); ++i)
    {
        members.push_back(m_members[i]);
    }

    for (size_t i = 0; i < members.size(); ++i)
    {
        GetObjectManager()->DeleteObject(members[i]);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CGroup::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    m_ie = ie;
    bool res = CBaseObject::Init(ie, prev, file);
    if (prev)
    {
        InvalidateBBox();

        CGroup* prevObj = (CGroup*)prev;

        if (prevObj)
        {
            if (prevObj->IsOpen())
            {
                Open();
            }
        }
    }
    return res;
}

//////////////////////////////////////////////////////////////////////////
void CGroup::AddMember(CBaseObject* pMember, bool bKeepPos)
{
    if (!pMember->IsChildOf(this))
    {
        if (stl::push_back_unique(m_members, pMember))
        {
            // when this method is called in group object, the bKeepPos for AttachChild will be ignored.
            AttachChild(pMember, bKeepPos);
            // Signal prefab if attached to any
            if (CGroup* pPrefab = (CGroup*)pMember->GetPrefab())
            {
                pPrefab->AddMember(pMember, true);
            }
        }
    }

    InvalidateBBox();
}

//////////////////////////////////////////////////////////////////////////
void CGroup::RemoveMember(CBaseObject* pMember, bool bKeepPos)
{
    if (pMember == nullptr || pMember->GetGroup() != this)
    {
        return;
    }

    if (stl::find_and_erase(m_members, pMember))
    {
        // Signal prefab if attached to any
        if (CGroup* pPrefab = (CGroup*)pMember->GetPrefab())
        {
            pPrefab->RemoveMember(pMember, true);
        }
    }
    pMember->DetachThis(bKeepPos);
    InvalidateBBox();

    UpdateGroup();
}

//////////////////////////////////////////////////////////////////////////
void CGroup::RemoveChild(CBaseObject* child)
{
    bool bMemberChild = stl::find_and_erase(m_members, child);
    CBaseObject::RemoveChild(child);
    if (bMemberChild)
    {
        InvalidateBBox();
    }
}

//////////////////////////////////////////////////////////////////////////
void CGroup::GetBoundBox(AABB& box)
{
    if (!m_bBBoxValid)
    {
        CalcBoundBox();
    }

    box.SetTransformedAABB(GetWorldTM(), m_bbox);
}

//////////////////////////////////////////////////////////////////////////
void CGroup::GetLocalBounds(AABB& box)
{
    if (!m_bBBoxValid)
    {
        CalcBoundBox();
    }
    box = m_bbox;
}

//////////////////////////////////////////////////////////////////////////
bool CGroup::HitTest(HitContext& hc)
{
    bool selected = false;
    if (m_opened)
    {
        selected = HitTestMembers(hc);
    }

    if (!selected)
    {
        Vec3 p;

        Matrix34 invertWTM = GetWorldTM();
        Vec3 worldPos = invertWTM.GetTranslation();
        invertWTM.Invert();

        Vec3 xformedRaySrc = invertWTM.TransformPoint(hc.raySrc);
        Vec3 xformedRayDir = invertWTM.TransformVector(hc.rayDir).GetNormalized();

        float epsilonDist = max(.1f, hc.view->GetScreenScaleFactor(worldPos) * 0.01f);
        epsilonDist *= max(0.0001f, min(invertWTM.GetColumn0().GetLength(), min(invertWTM.GetColumn1().GetLength(), invertWTM.GetColumn2().GetLength())));
        float hitDist;

        float tr = hc.distanceTolerance / 2 + 1;
        AABB boundbox, box;
        GetLocalBounds(boundbox);
        box.min = boundbox.min - Vec3(tr + epsilonDist, tr + epsilonDist, tr + epsilonDist);
        box.max = boundbox.max + Vec3(tr + epsilonDist, tr + epsilonDist, tr + epsilonDist);
        if (Intersect::Ray_AABB(xformedRaySrc, xformedRayDir, box, p))
        {
            if (Intersect::Ray_AABBEdge(xformedRaySrc, xformedRayDir, boundbox, epsilonDist, hitDist, p))
            {
                hc.dist = xformedRaySrc.GetDistance(p);
                hc.object = this;
                return true;
            }
            else
            {
                // Check if any childs of closed group selected.
                if (!m_opened)
                {
                    if (HitTestMembers(hc))
                    {
                        hc.object = this;
                        return true;
                    }
                }
            }
        }
    }

    return selected;
}

//////////////////////////////////////////////////////////////////////////
bool CGroup::HitHelperTestForChildObjects(HitContext& hc)
{
    bool result = false;

    if (m_opened)
    {
        for (int i = 0, iChildCount(GetChildCount()); i < iChildCount; ++i)
        {
            CBaseObject* pChild = GetChild(i);
            result = result || pChild->HitHelperTest(hc);
        }
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////
bool CGroup::HitTestMembers(HitContext& hcOrg)
{
    float mindist = FLT_MAX;

    HitContext hc = hcOrg;

    CBaseObject* selected = 0;
    int numMembers = static_cast<int>(m_members.size());
    for (int i = 0; i < numMembers; ++i)
    {
        CBaseObject* obj = m_members[i];

        if (GetObjectManager()->HitTestObject(obj, hc))
        {
            if (hc.dist < mindist)
            {
                mindist = hc.dist;
                // If collided object specified, accept it, otherwise take tested object itself.
                if (hc.object)
                {
                    selected = hc.object;
                }
                else
                {
                    selected = obj;
                }
                hc.object = 0;
            }
        }
    }
    if (selected)
    {
        hcOrg.object = selected;
        hcOrg.dist = mindist;
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CGroup::BeginEditParams(IEditor* ie, int flags)
{
    if (s_panelID)
    {
        ie->RemoveRollUpPage(ROLLUP_OBJECTS, s_panelID);
        s_pPanel = 0;
        s_panelID = 0;
    }

    m_ie = ie;
    CBaseObject::BeginEditParams(ie, flags);

    s_pPanel = new CGroupPanel(this);
    s_panelID = ie->AddRollUpPage(ROLLUP_OBJECTS, tr("Operations"), s_pPanel);
}

//////////////////////////////////////////////////////////////////////////
void CGroup::EndEditParams(IEditor* ie)
{
    if (s_panelID)
    {
        ie->RemoveRollUpPage(ROLLUP_OBJECTS, s_panelID);
        s_pPanel = 0;
        s_panelID = 0;
    }

    CBaseObject::EndEditParams(ie);
}

//////////////////////////////////////////////////////////////////////////
void CGroup::OnContextMenu(QMenu* menu)
{
    CBaseObject::OnContextMenu(menu);
}

//////////////////////////////////////////////////////////////////////////
void CGroup::Display(DisplayContext& dc)
{
    bool hideNames = dc.flags & DISPLAY_HIDENAMES;

    DrawDefault(dc, GetColor());

    dc.PushMatrix(GetWorldTM());

    AABB boundbox;
    GetLocalBounds(boundbox);

    if (IsSelected())
    {
        dc.SetColor(ColorB(0, 255, 0, 255));
        dc.DrawWireBox(boundbox.min, boundbox.max);
        dc.DepthTestOff();
        dc.SetColor(GetColor(), 0.15f);
        dc.DrawSolidBox(boundbox.min, boundbox.max);
        dc.DepthTestOn();
    }
    else
    {
        if (m_bAlwaysDrawBox)
        {
            if (IsFrozen())
            {
                dc.SetFreezeColor();
            }
            else
            {
                dc.SetColor(GetColor());
            }
            dc.DrawWireBox(boundbox.min, boundbox.max);
        }
    }
    dc.PopMatrix();
}

//////////////////////////////////////////////////////////////////////////
void CGroup::Serialize(CObjectArchive& ar)
{
    CBaseObject::Serialize(ar);

    if (ar.bLoading)
    {
        // Loading.
        ar.node->getAttr("Opened", m_opened);
        SerializeMembers(ar);
        if (ar.IsReconstructingPrefab())
        {
            m_opened = false;
        }
    }
    else
    {
        ar.node->setAttr("Opened", m_opened);
        SerializeMembers(ar);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGroup::SerializeMembers(CObjectArchive& ar)
{
    XmlNodeRef xmlNode = ar.node;
    if (ar.bLoading)
    {
        if (!ar.bUndo)
        {
            int num = static_cast<int>(m_members.size());
            for (int i = 0; i < num; ++i)
            {
                CBaseObject* member = m_members[i];
                member->DetachThis(true);
            }
            m_members.clear();

            // Loading.
            XmlNodeRef childsRoot = xmlNode->findChild("Objects");
            if (childsRoot)
            {
                // Load all childs from XML archive.
                int numObjects = childsRoot->getChildCount();
                for (int i = 0; i < numObjects; ++i)
                {
                    XmlNodeRef objNode = childsRoot->getChild(i);
                    ar.LoadObject(objNode);
                }
                InvalidateBBox();
            }
        }
    }
    else
    {
        if (m_members.size() > 0 && !ar.bUndo && !ar.IsSavingInPrefab())
        {
            // Saving.
            XmlNodeRef root = xmlNode->newChild("Objects");
            ar.node = root;

            // Save all child objects to XML.
            int num = static_cast<int>(m_members.size());
            for (int i = 0; i < num; ++i)
            {
                CBaseObject* obj = m_members[i];
                ar.SaveObject(obj, true);
            }
        }
    }
    ar.node = xmlNode;
}

//////////////////////////////////////////////////////////////////////////
void CGroup::Ungroup()
{
    StoreUndo("Ungroup");

    std::vector<CBaseObjectPtr> members = m_members;

    int num = static_cast<int>(members.size());
    for (int i = 0; i < num; ++i)
    {
        CBaseObject* pMember = members[i];
        if (pMember)
        {
            pMember->DetachThis(true);
            if (CGroup* pParentGroup = GetGroup())
            {
                pParentGroup->AddMember(pMember);
            }
            // Select detached objects.
            GetObjectManager()->SelectObject(pMember);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CGroup::Open()
{
    m_opened = true;
    GetIEditor()->Notify(eNotify_OnOpenGroup);
}

//////////////////////////////////////////////////////////////////////////
void CGroup::Close()
{
    m_opened = false;
    Sync();
    UpdateGroup();
    GetIEditor()->Notify(eNotify_OnCloseGroup);
}

//////////////////////////////////////////////////////////////////////////
void CGroup::RecursivelyGetBoundBox(CBaseObject* object, AABB& box, const Matrix34& parentTM)
{
    Matrix34 worldTM = parentTM * object->GetLocalTM();
    AABB b;
    object->GetLocalBounds(b);
    b.SetTransformedAABB(worldTM, b);
    box.Add(b.min);
    box.Add(b.max);

    int numChilds = object->GetChildCount();
    if (numChilds > 0)
    {
        for (int i = 0; i < numChilds; ++i)
        {
            RecursivelyGetBoundBox(object->GetChild(i), box, worldTM);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CGroup::CalcBoundBox()
{
    Matrix34 identityTM;
    identityTM.SetIdentity();

    // Calc local bounds box..
    AABB box;
    box.Reset();

    int numMembers = static_cast<int>(m_members.size());
    for (int i = 0; i < numMembers; ++i)
    {
        RecursivelyGetBoundBox(m_members[i], box, identityTM);
    }

    if (numMembers == 0)
    {
        box.min = Vec3(-1, -1, -1);
        box.max = Vec3(1, 1, 1);
    }

    m_bbox = box;
    m_bBBoxValid = true;
}

//////////////////////////////////////////////////////////////////////////
void CGroup::OnChildModified()
{
    if (m_ignoreChildModify)
    {
        return;
    }

    InvalidateBBox();
}

//! Select objects within specified distance from given position.
int CGroup::SelectObjects(const AABB& box, bool bUnselect)
{
    int numSel = 0;

    AABB objBounds;
    uint32 hideMask = GetIEditor()->GetDisplaySettings()->GetObjectHideMask();
    int num = static_cast<int>(m_members.size());
    for (int i = 0; i < num; ++i)
    {
        CBaseObject* obj = m_members[i];

        if (obj->IsHidden())
        {
            continue;
        }

        if (obj->IsFrozen())
        {
            continue;
        }

        if (obj->GetGroup())
        {
            continue;
        }

        obj->GetBoundBox(objBounds);
        if (box.IsIntersectBox(objBounds))
        {
            numSel++;
            if (!bUnselect)
            {
                GetObjectManager()->SelectObject(obj);
            }
            else
            {
                GetObjectManager()->UnselectObject(obj);
            }
        }
        // If its group.
        if (obj->metaObject() == &CGroup::staticMetaObject)
        {
            numSel += ((CGroup*)obj)->SelectObjects(box, bUnselect);
        }
    }
    return numSel;
}

//////////////////////////////////////////////////////////////////////////
void CGroup::OnEvent(ObjectEvent event)
{
    CBaseObject::OnEvent(event);

    switch (event)
    {
    case EVENT_DBLCLICK:
        if (IsOpen())
        {
            int numMembers = static_cast<int>(m_members.size());
            for (int i = 0; i < numMembers; ++i)
            {
                GetObjectManager()->SelectObject(m_members[i]);
            }
        }
        break;

    default:
    {
        int numMembers = static_cast<int>(m_members.size());
        for (int i = 0; i < numMembers; ++i)
        {
            m_members[i]->OnEvent(event);
        }
    }
    break;
    }
};

//////////////////////////////////////////////////////////////////////////
void CGroup::BindToParent()
{
    CBaseObject* parent = GetParent();
    if (qobject_cast<CEntityObject*>(parent))
    {
        CEntityObject* parentEntity = (CEntityObject*)parent;

        IEntity* ientParent = parentEntity->GetIEntity();
        if (ientParent)
        {
            for (int i = 0; i < GetChildCount(); ++i)
            {
                CBaseObject* child = GetChild(i);
                if (qobject_cast<CEntityObject*>(child))
                {
                    CEntityObject* childEntity = (CEntityObject*)child;
                    if (childEntity->GetIEntity())
                    {
                        ientParent->AttachChild(childEntity->GetIEntity(), IEntity::ATTACHMENT_KEEP_TRANSFORMATION);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CGroup::DetachThis(bool bKeepPos)
{
    CBaseObject* parent = GetParent();
    if (qobject_cast<CEntityObject*>(parent))
    {
        CEntityObject* parentEntity = (CEntityObject*)parent;

        IEntity* ientParent = parentEntity->GetIEntity();
        if (ientParent)
        {
            for (int i = 0; i < GetChildCount(); ++i)
            {
                CBaseObject* child = GetChild(i);
                if (qobject_cast<CEntityObject*>(child))
                {
                    CEntityObject* childEntity = (CEntityObject*)child;
                    if (childEntity->GetIEntity())
                    {
                        childEntity->GetIEntity()->DetachThis(IEntity::ATTACHMENT_KEEP_TRANSFORMATION);
                    }
                }
            }
        }
    }

    CBaseObject::DetachThis(bKeepPos);
}

void CGroup::PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx)
{
    if (qobject_cast<CGroup*>(pFromObject))
    {
        CBaseObject* pFromParent = pFromObject->GetParent();
        if (pFromParent)
        {
            SetFloorNumber(pFromObject->GetFloorNumber());
            CBaseObject* pFromParentInContext = ctx.FindClone(pFromParent);
            if (pFromParentInContext)
            {
                pFromParentInContext->AddMember(this, false);
            }
            else
            {
                pFromParent->AddMember(this, false);
            }
        }
        CloneChildren(pFromObject);
    }
    else
    {
        CBaseObject::PostClone(pFromObject, ctx);
    }
}

void CGroup::SetMaterial(CMaterial* mtl)
{
    if (mtl)
    {
        for (int i = 0, iNumberSize(m_members.size()); i < iNumberSize; ++i)
        {
            m_members[i]->SetMaterial(mtl);
        }
    }
    CBaseObject::SetMaterial(mtl);
}

void CGroup::UpdatePivot(const Vec3& newWorldPivotPos)
{
    if (m_bUpdatingPivot)
    {
        return;
    }

    m_bUpdatingPivot = true;

    const Matrix34& worldTM = GetWorldTM();
    Matrix34 invWorldTM = worldTM.GetInverted();

    Vec3 offset = worldTM.GetTranslation() - newWorldPivotPos;

    for (int i = 0, iChildCount(GetChildCount()); i < iChildCount; ++i)
    {
        Vec3 childWorldPos = worldTM.TransformPoint(GetChild(i)->GetPos());
        GetChild(i)->SetPos(invWorldTM.TransformPoint(childWorldPos + offset));
    }

    CBaseObject::SetWorldPos(newWorldPivotPos);
    m_bUpdatingPivot = false;
}

#include <Objects/Group.moc>