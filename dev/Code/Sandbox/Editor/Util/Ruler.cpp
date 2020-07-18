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

// Description : Ruler helper for Editor to determine distances


#include "StdAfx.h"
#include "AI/AIManager.h"
#include "Ruler.h"
#include "Viewport.h"

#include "IRenderAuxGeom.h"

#include <AzQtComponents/Components/HighDpiHelperFunctions.h>
#include <QGuiApplication>

//////////////////////////////////////////////////////////////////////////
CRuler::CRuler()
    : m_bActive(false)
    , m_MouseOverObject(GUID_NULL)
    , m_pPathAgents(NULL)
    , m_numPathAgents(0)
    , m_sphereScale(0.5f)
    , m_sphereTrans(0.5f)
{
    // can't create path agents here, AI manager doesn't know how many there are yet
}

//////////////////////////////////////////////////////////////////////////
CRuler::~CRuler()
{
    SetActive(false);

    delete [] m_pPathAgents;
}

//////////////////////////////////////////////////////////////////////////
bool CRuler::HasQueuedPaths() const
{
    for (uint32 i = 0; i < m_numPathAgents; ++i)
    {
        if (m_pPathAgents[i].HasQueuedPaths())
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CRuler::SetActive(bool bActive)
{
    if (m_bActive != bActive)
    {
        m_bActive = bActive;

        // create path agents if they don't already exist
        if (m_bActive && m_pPathAgents == NULL)
        {
            CAIManager* pAIManager = GetIEditor()->GetAI();
            m_numPathAgents = pAIManager->GetNumPathTypes();
            m_pPathAgents = new CRulerPathAgent[m_numPathAgents];

            for (uint32 i = 0; i < m_numPathAgents; ++i)
            {
                string name = pAIManager->GetPathTypeName(i);
                const AgentPathfindingProperties* pPfP = pAIManager->GetPFPropertiesOfPathType(name);
                if (pPfP)
                {
                    m_pPathAgents[i].SetType(pPfP->id, name);
                }
            }
        }

        if (m_bActive)
        {
            m_sphereScale = gSettings.gizmo.rulerSphereScale;
            m_sphereTrans = gSettings.gizmo.rulerSphereTrans;
        }

        // Reset
        m_startPoint.Reset();
        m_endPoint.Reset();

        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(m_MouseOverObject);
        if (pObject)
        {
            pObject->SetHighlight(false);
        }
        m_MouseOverObject = GUID_NULL;

        for (uint32 i = 0; i < m_numPathAgents; ++i)
        {
            m_pPathAgents[i].ClearLastRequest();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CRuler::Update()
{
    if (!IsActive())
    {
        return;
    }

    if (CheckVirtualKey(Qt::Key_Escape))
    {
        SetActive(false);
        return;
    }

    static const ColorF colours[] =
    {
        Col_Blue,
        Col_Green,
        Col_Red,
        Col_Yellow,
        Col_Magenta,
        Col_Black,
    };

    IRenderer* pRenderer = GetIEditor()->GetSystem()->GetIRenderer();
    CRY_ASSERT(pRenderer);
    IRenderAuxGeom* pAuxGeom = pRenderer->GetIRenderAuxGeom();
    CRY_ASSERT(pAuxGeom);

    CViewport* pActiveView = GetIEditor()->GetActiveView();
    if (pActiveView)
    {
        // Draw where cursor currently is
        if (!IsObjectSelectMode(pActiveView))
        {
            QPoint vCursorPoint = QCursor::pos();
            pActiveView->ScreenToClient(vCursorPoint);
            vCursorPoint = QHighDpi::toNativePixels(vCursorPoint, QGuiApplication::screenAt(vCursorPoint));
            Vec3 vCursorWorldPos = pActiveView->SnapToGrid(pActiveView->ViewToWorld(vCursorPoint));
            Vec3 vOffset(0.1f, 0.1f, 0.1f);
            pAuxGeom->SetRenderFlags(e_Def3DPublicRenderflags | e_AlphaBlended);
            pAuxGeom->DrawSphere(vCursorWorldPos, m_sphereScale, ColorF(0.5, 0.5, 0.5, m_sphereTrans));
            pAuxGeom->DrawAABB(AABB(vCursorWorldPos - vOffset * m_sphereScale, vCursorWorldPos + vOffset * m_sphereScale), false, ColorF(1.0f, 0.0f, 0.0f, 1.0f), eBBD_Faceted);
        }

        uint32 x = 12, y = 60;

        if (!m_startPoint.IsEmpty())
        {
            //pAuxGeom->DrawSphere(m_startPoint.GetPos(), 1.0f, ColorB(255,255,255,255));
            m_startPoint.Render(pRenderer);
        }
        if (!m_endPoint.IsEmpty())
        {
            //pAuxGeom->DrawSphere(m_endPoint.GetPos(), 1.0f, ColorB(255,255,255,255));
            m_endPoint.Render(pRenderer);

            pAuxGeom->DrawLine(m_startPoint.GetPos(), ColorB(255, 255, 255, 255), m_endPoint.GetPos(), ColorB(255, 255, 255, 255));

            string sTempText;

            // Compute distance and output results
            // TODO: Consider movement speed outputs here as well?
            const float fDistance = m_startPoint.GetDistance(m_endPoint);
            sTempText.Format("Straight-line distance: %.3f", fDistance);

            // Draw mid text
            float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
            pRenderer->Draw2dLabel(x, y, 2.0f, white, false, sTempText.c_str());
            y += 18;

            CAIManager* pAIManager = GetIEditor()->GetAI();

            for (uint32 i = 0; i < m_numPathAgents; ++i)
            {
                uint32 colourIndex = i % m_numPathAgents;
                m_pPathAgents[i].Render(pRenderer, colours[colourIndex]);

                // Check path agent status
                string sMidText;
                sMidText.Format("%s (type %d) ", m_pPathAgents[i].GetName(), m_pPathAgents[i].GetType());
                if (m_pPathAgents[i].HasQueuedPaths())
                {
                    sMidText += "Path Finder: Pending...";
                }
                else if (m_pPathAgents[i].GetLastPathSuccess())
                {
                    string sTempText;
                    sTempText.Format("Path Finder: %.3f", m_pPathAgents[i].GetLastPathDist());
                    sMidText += sTempText;
                }
                else
                {
                    sMidText += "Path Finder: No Path Found";
                }

                float col[4] = { colours[colourIndex].r, colours[colourIndex].g, colours[colourIndex].b, 1.0f };
                pRenderer->Draw2dLabel(x, y, 2.0f, col, false, sMidText.c_str());
                y += 18;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CRuler::IsObjectSelectMode(CViewport* pView) const
{
    const bool bShiftDown = CheckVirtualKey(Qt::Key_Shift);

    return (bShiftDown);
}

//////////////////////////////////////////////////////////////////////////
void CRuler::UpdateRulerPoint(CViewport* pView, const QPoint& point, CRulerPoint& rulerPoint, bool bRequestPath)
{
    CRY_ASSERT(pView);

    const bool bObjectSelect = IsObjectSelectMode(pView);

    rulerPoint.SetHelperSettings(m_sphereScale, m_sphereTrans);

    // Do entity hit check
    if (bObjectSelect)
    {
        HitContext hitInfo;
        pView->HitTest(point, hitInfo);

        CBaseObject* pHitObj = hitInfo.object;
        rulerPoint.Set(pHitObj);
    }
    else
    {
        Vec3 vWorldPoint = pView->SnapToGrid(pView->ViewToWorld(point));
        rulerPoint.Set(vWorldPoint);
    }

    if (bRequestPath)
    {
        RequestPath();
    }
    else
    {
        for (uint32 i = 0; i < m_numPathAgents; ++i)
        {
            m_pPathAgents[i].ClearLastRequest();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CRuler::RequestPath()
{
    for (uint32 i = 0; i < m_numPathAgents; ++i)
    {
        m_pPathAgents[i].RequestPath(m_startPoint, m_endPoint);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CRuler::MouseCallback(CViewport* pView, EMouseEvent event, QPoint& point, int flags)
{
    bool bResult = IsActive();

    if (bResult)
    {
        switch (event)
        {
        case eMouseMove:
            OnMouseMove(pView, point, flags);
            break;
        case eMouseLUp:
            OnLButtonUp(pView, point, flags);
            break;
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
void CRuler::OnMouseMove(CViewport* pView, QPoint& point, int flags)
{
    GUID hitGUID = GUID_NULL;

    if (IsObjectSelectMode(pView))
    {
        // Check for hit entity
        HitContext hitInfo;
        pView->HitTest(point, hitInfo);
        CBaseObject* pHitObj = hitInfo.object;
        if (pHitObj)
        {
            hitGUID = pHitObj->GetId();
        }
    }

    if (hitGUID != m_MouseOverObject)
    {
        // Kill highlight on old
        CBaseObject* pOldObj = GetIEditor()->GetObjectManager()->FindObject(m_MouseOverObject);
        if (pOldObj)
        {
            pOldObj->SetHighlight(false);
        }

        CBaseObject* pHitObj = GetIEditor()->GetObjectManager()->FindObject(hitGUID);
        if (pHitObj)
        {
            pHitObj->SetHighlight(true);
        }

        m_MouseOverObject = hitGUID;
    }
}

//////////////////////////////////////////////////////////////////////////
void CRuler::OnLButtonUp(CViewport* pView, QPoint& point, int flags)
{
    if (m_startPoint.IsEmpty())
    {
        UpdateRulerPoint(pView, point, m_startPoint, false);
    }
    else if (m_endPoint.IsEmpty())
    {
        UpdateRulerPoint(pView, point, m_endPoint, true);
    }
    else
    {
        UpdateRulerPoint(pView, point, m_startPoint, false);
        m_endPoint.Reset();
    }
}
