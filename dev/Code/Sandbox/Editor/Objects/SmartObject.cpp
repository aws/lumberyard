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
#include "SmartObject.h"

#include "IRenderAuxGeom.h"
#include "Include/IDisplayViewport.h"
#include "Include/IIconManager.h"
#include "PanelTreeBrowser.h"
#include "SmartObjects/SmartObjectsEditorDialog.h"
#include <IAgent.h>

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

CSmartObject::CSmartObject()
{
    m_entityClass = "SmartObject";
    m_pStatObj = NULL;
    m_pHelperMtl = NULL;
    m_pClassTemplate = NULL;
}

CSmartObject::~CSmartObject()
{
    if (m_pStatObj)
    {
        m_pStatObj->Release();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSmartObject::Done()
{
    CEntityObject::Done();
}

bool CSmartObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    SetColor(QColor(255, 255, 0));
    bool res = CEntityObject::Init(ie, prev, file);

    return res;
}

float CSmartObject::GetRadius()
{
    return 0.5f;
}

void CSmartObject::Display(DisplayContext& dc)
{
    const Matrix34& wtm = GetWorldTM();

    if (IsFrozen())
    {
        dc.SetFreezeColor();
    }
    else
    {
        dc.SetColor(GetColor());
    }

    if (!GetIStatObj())
    {
        dc.RenderObject(eStatObject_Anchor, wtm);
    }
    else if (!(dc.flags & DISPLAY_2D))
    {
        float color[4];
        color[0] = dc.GetColor().r * (1.0f / 255.0f);
        color[1] = dc.GetColor().g * (1.0f / 255.0f);
        color[2] = dc.GetColor().b * (1.0f / 255.0f);
        color[3] = dc.GetColor().a * (1.0f / 255.0f);

        SRenderingPassInfo passInfo = SRenderingPassInfo::CreateGeneralPassRenderingInfo(GetIEditor()->GetSystem()->GetViewCamera());

        Matrix34 tempTm = wtm;
        SRendParams rp;
        rp.pMatrix = &tempTm;
        rp.AmbientColor = ColorF(color[0], color[1], color[2], 1);
        rp.fAlpha = color[3];
        //rp.nShaderTemplate = EFT_HELPER;
        if (m_pStatObj)
        {
            m_pStatObj->Render(rp, passInfo);
        }
    }

    dc.SetColor(GetColor());

    if (IsSelected() || IsHighlighted())
    {
        dc.PushMatrix(wtm);
        if (!GetIStatObj())
        {
            float r = GetRadius();
            dc.DrawWireBox(-Vec3(r, r, r), Vec3(r, r, r));
        }
        else
        {
            dc.DrawWireBox(m_pStatObj->GetBoxMin(), m_pStatObj->GetBoxMax());
        }
        dc.PopMatrix();

        // this is now done in CEntity::DrawDefault
        //  if ( gEnv->pAISystem )
        //      gEnv->pAISystem->DrawSOClassTemplate( GetIEntity() );
    }

    DrawDefault(dc);
}

bool CSmartObject::HitTest(HitContext& hc)
{
    if (GetIStatObj())
    {
        float hitEpsilon = hc.view->GetScreenScaleFactor(GetWorldPos()) * 0.01f;
        float hitDist;

        float fScale = GetScale().x;
        AABB boxScaled;
        GetLocalBounds(boxScaled);
        boxScaled.min *= fScale;
        boxScaled.max *= fScale;

        Matrix34 invertWTM = GetWorldTM();
        invertWTM.Invert();

        Vec3 xformedRaySrc = invertWTM.TransformPoint(hc.raySrc);
        Vec3 xformedRayDir = invertWTM.TransformVector(hc.rayDir);
        xformedRayDir.Normalize();

        Vec3 intPnt;
        // Check intersection with bbox edges.
        if (Intersect::Ray_AABBEdge(xformedRaySrc, xformedRayDir, boxScaled, hitEpsilon, hitDist, intPnt))
        {
            hc.dist = xformedRaySrc.GetDistance(intPnt);
            hc.object = this;
            return true;
        }

        return false;
    }

    Vec3 origin = GetWorldPos();
    float radius = GetRadius();

    Vec3 w = origin - hc.raySrc;
    w = hc.rayDir.Cross(w);
    float d = w.GetLength();

    if (d < radius + hc.distanceTolerance)
    {
        hc.dist = hc.raySrc.GetDistance(origin);
        return true;
    }
    return false;
}

void CSmartObject::GetLocalBounds(AABB& box)
{
    if (GetIStatObj())
    {
        box.min = m_pStatObj->GetBoxMin();
        box.max = m_pStatObj->GetBoxMax();
    }
    else
    {
        float r = GetRadius();
        box.min = -Vec3(r, r, r);
        box.max = Vec3(r, r, r);
    }
}

//////////////////////////////////////////////////////////////////////////
IStatObj* CSmartObject::GetIStatObj()
{
    if (m_pStatObj)
    {
        return m_pStatObj;
    }

    ISmartObjectManager::IStatObjPtr* ppStatObjects = NULL;
    IEntity*    pEntity(GetIEntity());

    if (gEnv->pAISystem && pEntity)
    {
        ISmartObjectManager* piSmartObjManager = gEnv->pAISystem->GetSmartObjectManager();

        uint32 numSOFound = 0;
        piSmartObjManager->GetSOClassTemplateIStatObj(pEntity, NULL, numSOFound);
        if (numSOFound)
        {
            ppStatObjects = new ISmartObjectManager::IStatObjPtr[numSOFound];
            if (piSmartObjManager->GetSOClassTemplateIStatObj(pEntity, ppStatObjects, numSOFound) == 0)
            {
                SAFE_DELETE_ARRAY(ppStatObjects);
            }
        }
    }

    if (ppStatObjects)
    {
        m_pStatObj = ppStatObjects[0];
        m_pStatObj->AddRef();

        SAFE_DELETE_ARRAY(ppStatObjects);

        return m_pStatObj;
    }

    return NULL;
}

#define HELPER_MATERIAL "Editor/Objects/Helper"

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CSmartObject::GetHelperMaterial()
{
    if (!m_pHelperMtl)
    {
        m_pHelperMtl = GetIEditor()->Get3DEngine()->GetMaterialManager()->LoadMaterial(HELPER_MATERIAL);
    }
    return m_pHelperMtl;
};

//////////////////////////////////////////////////////////////////////////
void CSmartObject::OnPropertyChange(IVariable* var)
{
    if (m_pStatObj)
    {
        IStatObj* pOld = (IStatObj*) GetIStatObj();
        m_pStatObj = NULL;
        GetIStatObj();
        if (pOld)
        {
            pOld->Release();
        }
    }
}

void CSmartObject::GetScriptProperties(CEntityScript* pEntityScript, XmlNodeRef xmlProperties, XmlNodeRef xmlProperties2)
{
    CEntityObject::GetScriptProperties(pEntityScript, xmlProperties, xmlProperties2);

    m_pProperties->AddOnSetCallback(functor(*this, &CSmartObject::OnPropertyChange));
}

//////////////////////////////////////////////////////////////////////////
void CSmartObject::BeginEditParams(IEditor* ie, int flags)
{
    if (m_pProperties)
    {
        m_pProperties->AddOnSetCallback(functor(*this, &CSmartObject::OnPropertyChange));
    }

    CEntityObject::BeginEditParams(ie, flags);
}

//////////////////////////////////////////////////////////////////////////
void CSmartObject::EndEditParams(IEditor* ie)
{
    if (m_pProperties)
    {
        m_pProperties->RemoveOnSetCallback(functor(*this, &CSmartObject::OnPropertyChange));
    }

    CEntityObject::EndEditParams(ie);


    Reload(true);
}

void CSmartObject::OnEvent(ObjectEvent eventID)
{
    CBaseObject::OnEvent(eventID);

    switch (eventID)
    {
    case EVENT_INGAME:
    case EVENT_OUTOFGAME:
        SAFE_RELEASE(m_pStatObj);
        break;
    }
}

#include <Objects/SmartObject.moc>