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

#include "LinkTool.h"
#include "Viewport.h"
#include "Objects/EntityObject.h"
#include "Objects/MiscEntities.h"
#include <QMenu>
#include <QCursor>

#ifdef LoadCursor
#undef LoadCursor
#endif

namespace
{
    const float kGeomCacheNodePivotSizeScale = 0.0025f;
}

//////////////////////////////////////////////////////////////////////////
CLinkTool::CLinkTool()
    : m_nodeName(nullptr)
    , m_pGeomCacheRenderNode(nullptr)
{
    m_pChild = NULL;
    SetStatusText("Click on object and drag a link to a new parent");

    m_hLinkCursor = CMFCUtils::LoadCursor(IDC_POINTER_LINK);
    m_hLinkNowCursor = CMFCUtils::LoadCursor(IDC_POINTER_LINKNOW);
    m_hCurrCursor = &m_hLinkCursor;
}

//////////////////////////////////////////////////////////////////////////
CLinkTool::~CLinkTool()
{
}

//////////////////////////////////////////////////////////////////////////
void CLinkTool::LinkObject(CBaseObject* pChild, CBaseObject* pParent)
{
    if (pChild == NULL)
    {
        return;
    }

    if (ChildIsValid(pParent, pChild))
    {
        CUndo undo("Link Object");

        if (qobject_cast<CEntityObject*>(pChild))
        {
            static_cast<CEntityObject*>(pChild)->SetAttachTarget("");
            static_cast<CEntityObject*>(pChild)->SetAttachType(CEntityObject::eAT_Pivot);
        }

        pParent->AttachChild(pChild, true);

        QString str;
        str = tr("%1 attached to %2").arg(pChild->GetName(), pParent->GetName());
        SetStatusText(str);
    }
    else
    {
        SetStatusText("Error: Cyclic linking or already linked.");
    }
}

#if defined(USE_GEOM_CACHES)
//////////////////////////////////////////////////////////////////////////
void CLinkTool::LinkToGeomCacheNode(CEntityObject* pChild, CGeomCacheEntity* pParent, const char* target)
{
    if (pChild == NULL)
    {
        return;
    }

    if (ChildIsValid(pParent, pChild))
    {
        CUndo undo("Link to Geometry Cache Node");

        pChild->SetAttachTarget(target);
        pChild->SetAttachType(CEntityObject::eAT_GeomCacheNode);
        pParent->AttachChild(pChild, true);

        QString str;
        str = tr("%1 attached to %2").arg(pChild->GetName(), pParent->GetName());
        SetStatusText(str);
    }
    else
    {
        SetStatusText("Error: Cyclic linking or already linked.");
    }
}
#endif

//////////////////////////////////////////////////////////////////////////
void CLinkTool::LinkToBone(CEntityObject* pChild, CEntityObject* pParent, CViewport* view)
{
    if (pChild == pParent)
    {
        SetStatusText("Error: Parent and Child can't be same.");
        return;
    }

    if (pChild->GetParent() == NULL)
    {
        IEntity* pIEntity = pParent->GetIEntity();
        ICharacterInstance* pCharacter = pIEntity->GetCharacter(0);
        ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
        IDefaultSkeleton& rIDefaultSkeleton = pCharacter->GetIDefaultSkeleton();

        // Show a sorted pop-up menu for selecting a bone.
        QMenu boneMenu(qobject_cast<QWidget*>(view->qobject()));

        const uint32 numJoints = rIDefaultSkeleton.GetJointCount();
        std::map<string, uint32> joints;

        for (uint32 i = 0; i < numJoints; ++i)
        {
            joints[rIDefaultSkeleton.GetJointNameByID(i)] = i;
        }

        int count = 0;
        for (auto it = joints.begin(); it != joints.end(); ++it)
        {
            QAction* action = boneMenu.addAction(QString((*it).first));
            action->setData((*it).second);
            count++;
        }

        QAction* userSelection = boneMenu.exec(QCursor::pos());
        if (!userSelection)
        {
            return;
        }

        unsigned int item = userSelection->data().toInt();

        const char* pTarget = rIDefaultSkeleton.GetJointNameByID(item);

        CUndo undo("Link to Bone");
        pChild->SetAttachTarget(pTarget);
        pChild->SetAttachType(CEntityObject::eAT_CharacterBone);
        pParent->AttachChild(pChild, true);

        QString str;
        str = tr("%1 attached to the bone '%2' of %3").arg(pChild->GetName(), pTarget, pParent->GetName());
        SetStatusText(str);
    }
    else
    {
        SetStatusText("Error: Already linked.");
    }
}

//////////////////////////////////////////////////////////////////////////
void CLinkTool::LinkSelectedToParent(CBaseObject* pParent)
{
    if (pParent)
    {
        if (IsRelevant(pParent))
        {
            CSelectionGroup* pSel = GetIEditor()->GetSelection();
            if (!pSel->GetCount())
            {
                return;
            }
            CUndo undo("Link Object(s)");
            for (int i = 0; i < pSel->GetCount(); i++)
            {
                CBaseObject* pChild = pSel->GetObject(i);
                if (pChild == pParent)
                {
                    continue;
                }
                LinkObject(pChild, pParent);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CLinkTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    view->SetCursorString("");

    m_hCurrCursor = &m_hLinkCursor;
    if (event == eMouseLDown)
    {
        HitContext hitInfo;
        view->HitTest(point, hitInfo);
        CBaseObject* obj = hitInfo.object;
        if (obj)
        {
            if (IsRelevant(obj))
            {
                m_StartDrag = obj->GetWorldPos();
                m_pChild = obj;
            }
        }
    }
    else if (event == eMouseLUp)
    {
        HitContext hitInfo;
        view->HitTest(point, hitInfo);
        CBaseObject* obj = hitInfo.object;
        if (obj)
        {
            if (IsRelevant(obj))
            {
                ISkeletonPose* pSkeletonPose = NULL;
#if defined(USE_GEOM_CACHES)
                IGeomCacheRenderNode* pGeomCacheRenderNode = NULL;
#endif

                if (qobject_cast<CEntityObject*>(obj) && qobject_cast<CEntityObject*>(m_pChild))
                {
                    CEntityObject* pEntity = static_cast<CEntityObject*>(obj);
                    IEntity* pIEntity = pEntity->GetIEntity();
                    if (pIEntity)
                    {
                        ICharacterInstance* pCharacter = pIEntity->GetCharacter(0);
                        if (pCharacter)
                        {
                            pSkeletonPose = pCharacter->GetISkeletonPose();
                        }
#if defined(USE_GEOM_CACHES)
                        else if (hitInfo.name && qobject_cast<CGeomCacheEntity*>(obj))
                        {
                            pGeomCacheRenderNode = pIEntity->GetGeomCacheRenderNode(0);
                        }
#endif
                    }
                }

                if (pSkeletonPose)
                {
                    LinkToBone(static_cast<CEntityObject*>(m_pChild), static_cast<CEntityObject*>(obj), view);
                }
#if defined(USE_GEOM_CACHES)
                else if (pGeomCacheRenderNode && hitInfo.name)
                {
                    LinkToGeomCacheNode(static_cast<CEntityObject*>(m_pChild), static_cast<CGeomCacheEntity*>(obj), hitInfo.name);
                }
#endif
                else
                {
                    CSelectionGroup* pSelectionGroup = GetIEditor()->GetSelection();
                    int nGroupCount = pSelectionGroup->GetCount();
                    if (pSelectionGroup && nGroupCount > 1)
                    {
                        LinkSelectedToParent(obj);
                    }
                    if (!pSelectionGroup || nGroupCount <= 1  || !pSelectionGroup->IsContainObject(m_pChild))
                    {
                        LinkObject(m_pChild, obj);
                    }
                }
            }
        }
        m_pChild = NULL;
    }
    else if (event == eMouseMove)
    {
        m_EndDrag = view->ViewToWorld(point);
        m_nodeName = nullptr;
        m_pGeomCacheRenderNode = nullptr;

        HitContext hitInfo;
        if (view->HitTest(point, hitInfo))
        {
            m_EndDrag = hitInfo.raySrc + hitInfo.rayDir * hitInfo.dist;
        }

        CBaseObject* obj = hitInfo.object;
        if (obj)
        {
            if (IsRelevant(obj))
            {
                QString name = obj->GetName();
                if (hitInfo.name)
                {
                    name += QString("\n  ") + hitInfo.name;
                }

                // Set Cursors.
                view->SetCursorString(name);
                if (m_pChild)
                {
                    if (ChildIsValid(obj, m_pChild))
                    {
                        m_hCurrCursor = &m_hLinkNowCursor;
                    }
                }
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLinkTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        // Cancel selection.
        GetIEditor()->SetEditTool(0);
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CLinkTool::Display(DisplayContext& dc)
{
    if (m_pChild && m_EndDrag != Vec3(ZERO))
    {
        ColorF lineColor = (m_hCurrCursor == &m_hLinkNowCursor) ? ColorF(0, 1, 0) : ColorF(1, 0, 0);
        dc.DrawLine(m_StartDrag, m_EndDrag, lineColor, lineColor);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CLinkTool::ChildIsValid(CBaseObject* pParent, CBaseObject* pChild, int nDir)
{
    if (!pParent)
    {
        return false;
    }
    if (!pChild)
    {
        return false;
    }
    if (pParent == pChild)
    {
        return false;
    }

    // Legacy entities and AZ entities shouldn't be linked.
    if ((pParent->GetType() == OBJTYPE_AZENTITY) != (pChild->GetType() == OBJTYPE_AZENTITY))
    {
        return false;
    }

    CBaseObject* pObj;
    if (nDir & 1)
    {
        if (pObj = pChild->GetParent())
        {
            if (!ChildIsValid(pParent, pObj, 1))
            {
                return false;
            }
        }
    }
    if (nDir & 2)
    {
        for (int i = 0; i < pChild->GetChildCount(); i++)
        {
            if (pObj = pChild->GetChild(i))
            {
                if (!ChildIsValid(pParent, pObj, 2))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLinkTool::OnSetCursor(CViewport* vp)
{
    vp->SetCursor(*m_hCurrCursor);
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CLinkTool::DrawObjectHelpers(CBaseObject* pObject, DisplayContext& dc)
{
    if (!m_pChild)
    {
        return;
    }

#if defined(USE_GEOM_CACHES)
    if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntityObject = (CEntityObject*)pObject;

        IEntity* pEntity = pEntityObject->GetIEntity();
        if (pEntity)
        {
            IGeomCacheRenderNode* pGeomCacheRenderNode = pEntity->GetGeomCacheRenderNode(0);
            if (pGeomCacheRenderNode)
            {
                if (CheckVirtualKey(Qt::Key_Shift))
                {
                    const Matrix34& worldTM = pEntity->GetWorldTM();

                    dc.DepthTestOff();

                    const uint nodeCount = pGeomCacheRenderNode->GetNodeCount();
                    for (uint i = 0; i < nodeCount; ++i)
                    {
                        const char* pNodeName = pGeomCacheRenderNode->GetNodeName(i);
                        Vec3 nodePosition = worldTM.TransformPoint(pGeomCacheRenderNode->GetNodeTransform(i).GetTranslation());

                        AABB entityWorldBounds;
                        pEntity->GetWorldBounds(entityWorldBounds);
                        Vec3 aabbSize = entityWorldBounds.GetSize();
                        float aabbMinSide = std::min(std::min(aabbSize.x, aabbSize.y), aabbSize.z);

                        Vec3 boxSizeHalf(kGeomCacheNodePivotSizeScale * aabbMinSide);

                        ColorB color = ColorB(0, 0, 255, 255);
                        if (pNodeName == m_nodeName && pGeomCacheRenderNode == m_pGeomCacheRenderNode)
                        {
                            color = ColorB(0, 255, 0, 255);
                        }

                        dc.SetColor(color);
                        dc.DrawWireBox(nodePosition - boxSizeHalf, nodePosition + boxSizeHalf);
                    }

                    dc.DepthTestOn();
                }

                if (pGeomCacheRenderNode == m_pGeomCacheRenderNode)
                {
                    SGeometryDebugDrawInfo dd;
                    dd.tm = pEntity->GetSlotWorldTM(0);
                    dd.bExtrude = true;
                    dd.color = ColorB(250, 0, 250, 30);
                    dd.lineColor = ColorB(255, 255, 0, 160);

                    pGeomCacheRenderNode->DebugDraw(dd, 0.01f, m_hitNodeIndex);
                }
            }
        }
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
bool CLinkTool::HitTest(CBaseObject* pObject, HitContext& hc)
{
    if (!m_pChild)
    {
        return false;
    }

    bool bHit = false;

    m_hitNodeIndex = 0;

#if defined(USE_GEOM_CACHES)
    if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntityObject = (CEntityObject*)pObject;

        IEntity* pEntity = pEntityObject->GetIEntity();
        if (pEntity)
        {
            IGeomCacheRenderNode* pGeomCacheRenderNode = pEntity->GetGeomCacheRenderNode(0);
            if (pGeomCacheRenderNode)
            {
                float nearestDist = std::numeric_limits<float>::max();

                if (CheckVirtualKey(Qt::Key_Shift))
                {
                    Ray ray(hc.raySrc, hc.rayDir);
                    const Matrix34& worldTM = pEntity->GetWorldTM();

                    const uint nodeCount = pGeomCacheRenderNode->GetNodeCount();
                    for (uint i = 0; i < nodeCount; ++i)
                    {
                        Vec3 nodePosition = worldTM.TransformPoint(pGeomCacheRenderNode->GetNodeTransform(i).GetTranslation());

                        AABB entityWorldBounds;
                        pEntity->GetWorldBounds(entityWorldBounds);
                        Vec3 aabbSize = entityWorldBounds.GetSize();
                        float aabbMinSide = std::min(std::min(aabbSize.x, aabbSize.y), aabbSize.z);

                        Vec3 boxSizeHalf(kGeomCacheNodePivotSizeScale * aabbMinSide);

                        AABB nodeAABB(nodePosition - boxSizeHalf, nodePosition + boxSizeHalf);

                        Vec3 hitPoint;
                        if (Intersect::Ray_AABB(ray, nodeAABB, hitPoint))
                        {
                            const float hitDist = hitPoint.GetDistance(hc.raySrc);

                            if (hitDist < nearestDist)
                            {
                                nearestDist = hitDist;
                                bHit = true;

                                m_hitNodeIndex = i;
                                hc.object = pObject;
                                hc.dist = 0.0f;
                                hc.name = pGeomCacheRenderNode->GetNodeName(i);
                                m_nodeName = hc.name;
                                m_pGeomCacheRenderNode = pGeomCacheRenderNode;
                            }
                        }
                    }
                }
                else
                {
                    SRayHitInfo hitInfo;
                    ZeroStruct(hitInfo);
                    hitInfo.inReferencePoint = hc.raySrc;
                    hitInfo.inRay = Ray(hitInfo.inReferencePoint, hc.rayDir.GetNormalized());
                    hitInfo.bInFirstHit = false;
                    hitInfo.bUseCache = false;

                    if (pGeomCacheRenderNode->RayIntersection(hitInfo, NULL, &m_hitNodeIndex))
                    {
                        nearestDist = hitInfo.fDistance;
                        bHit = true;

                        hc.object = pObject;
                        hc.dist = 0.0f;
                        hc.name = pGeomCacheRenderNode->GetNodeName(m_hitNodeIndex);
                        m_nodeName = hc.name;
                        m_pGeomCacheRenderNode = pGeomCacheRenderNode;
                    }
                }
            }
        }
    }
#endif

    return bHit;
}

#include <LinkTool.moc>