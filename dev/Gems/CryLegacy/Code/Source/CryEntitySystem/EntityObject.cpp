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

#include "CryLegacy_precompiled.h"
#include "EntityObject.h"
#include "Entity.h"
#include "Force.h"

#include <IRenderer.h>
#include <IEntityRenderState.h>
#include <I3DEngine.h>
#include <ICryAnimation.h>
#include <IGeomCache.h>

#define MAX_CHARACTER_LOD 10

namespace
{
    Matrix34 sIdentMatrix = Matrix34::CreateIdentity();
}

//////////////////////////////////////////////////////////////////////////
CEntityObject::~CEntityObject()
{
    ReleaseObjects();
    if (m_pXForm)
    {
        delete m_pXForm;
    }
    FreeCameraSpacePos();
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::ReleaseObjects()
{
    if (pStatObj)
    {
        pStatObj->Release();
        pStatObj = NULL;
    }
    else if (pCharacter)
    {
        if (ISkeletonAnim* pSkeletonAnim = pCharacter->GetISkeletonAnim())
        {
            pSkeletonAnim->SetEventCallback(0, 0);
        }

        if (ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose())
        {
            pSkeletonPose->DestroyCharacterPhysics(0);
        }

        pCharacter->Release();
        pCharacter = NULL;
    }
    else if (pLight)
    {
        pLight->ReleaseNode();
        pLight = NULL;
    }
    else if (IParticleEmitter* pEmitter = GetParticleEmitter())
    {
        pEmitter->Activate(false);
        pEmitter->Release();
        pEmitter->SetEntity(NULL, 0);
        pChildRenderNode = 0;
    }
    else if (pChildRenderNode)
    {
        pChildRenderNode->ReleaseNode();
        pChildRenderNode = 0;
    }
    else if (pFoliage)
    {
        pFoliage->Release();
        pFoliage = 0;
    }

    if (m_pRNTmpData)
    {
        gEnv->p3DEngine->FreeRNTmpData(&m_pRNTmpData);
    }
    assert(!m_pRNTmpData);
}


bool CEntityObject::GetLocalBounds(AABB& bounds)
{
    if (pStatObj)
    {
        bounds.Add(pStatObj->GetAABB());
        return true;
    }
    else if (pCharacter)
    {
        bounds.Add(pCharacter->GetAABB());
        return true;
    }
    else if (pLight)
    {
        bounds.Add(Vec3_Zero, 0.1f);
        return true;
    }
    else if (pChildRenderNode)
    {
        pChildRenderNode->GetLocalBounds(bounds);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
IParticleEmitter* CEntityObject::GetParticleEmitter() const
{
    if (pChildRenderNode && pChildRenderNode->GetRenderNodeType() == eERType_ParticleEmitter)
    {
        return static_cast<IParticleEmitter*>(pChildRenderNode);
    }
    else
    {
        return NULL;
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetParent(CEntityObject* pParentSlot)
{
    pParent = pParentSlot;
    if (!m_pXForm)
    {
        m_pXForm = new SEntityObjectXForm;
    }
    bWorldTMValid = false;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetLocalTM(const Matrix34& localTM)
{
    if (!m_pXForm)
    {
        m_pXForm = new SEntityObjectXForm;
    }

    m_pXForm->localTM = localTM;
    bWorldTMValid = false;
    if (pParent)
    {
        pParent->bWorldTMValid = false;
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::SetCameraSpacePos(const Vec3& cameraSpacePos)
{
    if (m_pCameraSpacePos == NULL)
    {
        m_pCameraSpacePos = new Vec3;
    }
    *m_pCameraSpacePos = cameraSpacePos;
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::GetCameraSpacePos(Vec3& cameraSpacePos)
{
    if (m_pCameraSpacePos)
    {
        cameraSpacePos = *m_pCameraSpacePos;
    }
    else
    {
        cameraSpacePos = ZERO;
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::FreeCameraSpacePos()
{
    SAFE_DELETE(m_pCameraSpacePos);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::UpdateWorldTM(IEntity* pEntity)
{
    if (!pParent)
    {
        // Add entity matrix as parent.
        if (!m_pXForm)
        {
            m_worldTM = pEntity->GetWorldTM();
        }
        else
        {
            m_worldTM = pEntity->GetWorldTM() * m_pXForm->localTM;
        }
    }
    else
    {
        if (!m_pXForm)
        {
            m_worldTM = pParent->GetWorldTM(pEntity);
        }
        else
        {
            m_worldTM = pParent->GetWorldTM(pEntity) * m_pXForm->localTM;
        }
    }
    bWorldTMValid = true;
    bObjectMoved = true;
}

//////////////////////////////////////////////////////////////////////////
const Matrix34& CEntityObject::GetLocalTM() const
{
    if (m_pXForm)
    {
        return m_pXForm->localTM;
    }
    return sIdentMatrix;
};

//////////////////////////////////////////////////////////////////////////
const Matrix34& CEntityObject::GetWorldTM(IEntity* pEntity)
{
    if (!bWorldTMValid)
    {
        UpdateWorldTM(pEntity);
    }
    return m_worldTM;
}

CLodValue CEntityObject::ComputeLod(int wantedLod, const SRenderingPassInfo& passInfo)
{
    if (pCharacter)
    {
        return pCharacter->ComputeLod(wantedLod, passInfo);
    }

    return CLodValue(wantedLod);
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::Render(IEntity* pEntity, SRendParams& rParams, int nRndFlags, CComponentRender* pRenderComponent, const SRenderingPassInfo& passInfo)
{
    if (!bWorldTMValid)
    {
        UpdateWorldTM(pEntity);
    }

    // Override with custom slot material.
    _smart_ptr<IMaterial> pPrevMtl = rParams.pMaterial;
    if (pMaterial)
    {
        rParams.pMaterial = pMaterial;
    }

    int nOldObjectFlags = rParams.dwFObjFlags;
    rParams.dwFObjFlags |= dwFObjFlags;


    if (flags & ENTITY_SLOT_RENDER_AFTER_POSTPROCESSING)
    {
        rParams.dwFObjFlags |= FOB_RENDER_AFTER_POSTPROCESSING;
    }

    //////////////////////////////////////////////////////////////////////////

    rParams.pInstance = this;

    const bool bIsInCameraSpace = (flags & ENTITY_SLOT_RENDER_NEAREST) != 0;

    // Draw static object.
    if (pStatObj)
    {
        rParams.pMatrix = &m_worldTM;
        rParams.pFoliage = pFoliage;
        rParams.ppRNTmpData = &m_pRNTmpData;

        rParams.nSubObjHideMask = nSubObjHideMask;

        // make sure object motion blur can be applied to this object
        if (bObjectMoved)
        {
            rParams.dwFObjFlags |= FOB_DYNAMIC_OBJECT;
            bObjectMoved = false;
        }

        Matrix34 entityTM;
        if (bIsInCameraSpace)
        {
            rParams.pMatrix = &entityTM;
            entityTM = m_worldTM;

            // Camera space
            if (m_pCameraSpacePos)
            {
                // Use camera space relative position
                entityTM.SetTranslation(*m_pCameraSpacePos);
            }
            else
            {
                // We don't have camera space relative position, so calculate it out from world space
                // (This will not have the precision advantages of camera space rendering)
                entityTM.AddTranslation(-gEnv->pSystem->GetViewCamera().GetPosition());
            }
        }

        if (rParams.pMatrix->IsValid())
        {
            pStatObj->Render(rParams, passInfo);
        }
        else
        {
            EntityWarning("CEntityObject::Render: Invalid world matrix: %s", pEntity->GetEntityTextDescription());
        }
    }
    else if (pCharacter)
    {
        QuatTS Offset;
        Matrix34 PhysLocation(pEntity->GetWorldTM());
        if (m_pXForm)
        {
            Offset = QuatTS(m_pXForm->localTM);
        }
        else
        {
            //CRY_FIXME(03,12,2009,"Animation & Rendering of entities needs to be re-written to avoid derivation of local offset due to float inaccuracy - Richard Semmens");

            if (!Matrix34::IsEquivalent(PhysLocation, m_worldTM))
            {
                Matrix34 invPhysLocation = PhysLocation.GetInverted();

                Matrix34 matOffset = invPhysLocation * m_worldTM;

                Offset = QuatTS(matOffset);
            }
            else
            {
                Offset.SetIdentity();
            }
        }

        if (bIsInCameraSpace)
        {
            // Camera space
            if (m_pCameraSpacePos)
            {
                // Use camera space relative position
                const Matrix33 camRot = Matrix33(gEnv->pSystem->GetViewCamera().GetViewMatrix());
                PhysLocation.SetTranslation(*m_pCameraSpacePos * camRot);
            }
            else
            {
                // We don't have camera space relative position, so calculate it out from world space
                // (This will not have the precision advantages of camera space rendering)
                PhysLocation.AddTranslation(-gEnv->pSystem->GetViewCamera().GetPosition());
            }
            Offset.SetIdentity();
        }

        rParams.pMatrix = &PhysLocation;
        //rParams.pInstance = pCharacter;

        // Disable hand-placed (static) decals on characters
        rParams.dwFObjFlags |= FOB_DYNAMIC_OBJECT;

        pCharacter->Render(rParams, Offset, passInfo);

        const uint32 renderComponentFlags = pRenderComponent->GetFlags();
        if (!passInfo.IsShadowPass() || (renderComponentFlags & CComponentRender::FLAG_ANIMATE_OFFSCREEN_SHADOW))
        {
            // If We render character, make sure it is also gets animation activated.
            if (!pEntity->IsInActiveList())
            {
                pEntity->ActivateForNumUpdates(8);
            }
        }
    }
    else if (pChildRenderNode)
    {
        rParams.pMatrix = &m_worldTM;
        //rParams.pInstance = pChildRenderNode;

        pChildRenderNode->m_dwRndFlags = nRndFlags;
        pChildRenderNode->Render(rParams, passInfo);
    }

    rParams.pMaterial = pPrevMtl;
    rParams.dwFObjFlags = nOldObjectFlags;

    if (!passInfo.IsShadowPass())   // Should also ignore rendering into the recursion.
    {
        if (pFoliage)
        {
            pFoliage->SetFlags(pFoliage->GetFlags() & ~IFoliage::FLAG_FROZEN | -(int)(rParams.nMaterialLayers & MTL_LAYER_FROZEN) & IFoliage::FLAG_FROZEN);
            static ICVar* g_pWindActivationDist = gEnv->pConsole->GetCVar("e_FoliageWindActivationDist");
            float maxdist = g_pWindActivationDist ? g_pWindActivationDist->GetFVal() : 0.0f;
            Vec3 pos = m_worldTM.GetTranslation();
            if (pStatObj && (gEnv->pSystem->GetViewCamera().GetPosition() - pos).len2() < sqr(maxdist) && gEnv->p3DEngine->GetWind(AABB(pos), false).len2() > 101.0f)
            {
                pStatObj->PhysicalizeFoliage(pEntity->GetPhysics(), m_worldTM, pFoliage, 0, 4);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnXForm(IEntity* pEntity)
{
    UpdateWorldTM(pEntity);

    if (!m_worldTM.IsValid())
    {
        EntityWarning("CEntityObject::OnXForm: Invalid world matrix: %s", pEntity->GetEntityTextDescription());
        return;
    }

    if (pLight)
    {
        ILightSource* pLightSource = pLight;
        // Update light positions.
        CDLight* pDLight = &pLightSource->GetLightProperties();

        pDLight->SetPosition(m_worldTM.GetTranslation());
        pDLight->SetMatrix(m_worldTM);
        pDLight->m_sName = pEntity->GetName(); // For debugging only.
        pDLight->m_nEntityId = pEntity->GetId();
        pEntity->UpdateLightClipVolumes(*pDLight);
        pLightSource->SetMatrix(m_worldTM);
        pLight->SetName(pEntity->GetName());
    }
    else if (pChildRenderNode)
    {
        pChildRenderNode->SetScale(pEntity->GetScale());
        pChildRenderNode->SetMatrix(m_worldTM);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnEntityEvent(IEntity* pEntity, SEntityEvent const& event)
{
    if (pChildRenderNode)
    {
        pChildRenderNode->OnEntityEvent(pEntity, event);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::OnNotSeenTimeout()
{
    //if (pCharacter)
    //  pCharacter->ReleaseTemporaryResources();
}

//////////////////////////////////////////////////////////////////////////
void CEntityObject::Update(IEntity* pEntity, bool bVisible, bool& bBoundsChanged)
{
    bBoundsChanged = false;
    if (pCharacter)
    {
        int characterFlags = pCharacter->GetFlags();
        if ((flags & ENTITY_SLOT_RENDER) || (characterFlags & CS_FLAG_UPDATE_ALWAYS))
        {
            bBoundsChanged = true; // Lets bounds of character change constantly.
            if (characterFlags & CS_FLAG_UPDATE)
            {
                if (!bWorldTMValid)
                {
                    UpdateWorldTM(pEntity);
                }

                QuatTS AnimatedCharacter = QuatTS(m_worldTM);
                const CCamera& camera = GetISystem()->GetViewCamera();
                float fDistance = (camera.GetPosition() - AnimatedCharacter.t).GetLength();
                float fZoomFactor = 0.001f + 0.999f * (RAD2DEG(camera.GetFov()) / 60.f);

                SAnimationProcessParams params;
                params.locationAnimation = AnimatedCharacter;
                params.bOnRender = 0;
                params.zoomAdjustedDistanceFromCamera = fDistance * fZoomFactor;
                pCharacter->StartAnimationProcessing(params);
            }
        }
    }
#if defined(USE_GEOM_CACHES)
    else
    {
        IGeomCacheRenderNode* pGeomCacheRenderNode = GetGeomCacheRenderNode();

        if (pGeomCacheRenderNode && pGeomCacheRenderNode->DidBoundsChange())
        {
            bBoundsChanged = true;
        }
    }
#endif
}

void CEntityObject::GetMemoryUsage(ICrySizer* pSizer) const
{
    {
        SIZER_COMPONENT_NAME(pSizer, "CEntityObject Allocator");
        pSizer->AddObject(g_Alloc_EntitySlot);
    }

    pSizer->AddObject(pCharacter);
    pSizer->AddObject(pStatObj);
    pSizer->AddObject(pLight);
    pSizer->AddObject(pChildRenderNode);
    pSizer->AddObject(pMaterial);
}

//////////////////////////////////////////////////////////////////////////
