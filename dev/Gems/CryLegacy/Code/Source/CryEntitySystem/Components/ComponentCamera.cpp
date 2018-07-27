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
#include "CryLegacy_precompiled.h"
#include "ComponentCamera.h"
#include "ISerialize.h"
#include "Components/IComponentSerialization.h"

DECLARE_DEFAULT_COMPONENT_FACTORY(CComponentCamera, IComponentCamera)

//////////////////////////////////////////////////////////////////////////
CComponentCamera::CComponentCamera()
    : m_pEntity(NULL)
{
}

//////////////////////////////////////////////////////////////////////////
void CComponentCamera::Initialize(const SComponentInitializer& init)
{
    m_pEntity = init.m_pEntity;
    m_pEntity->GetComponent<IComponentSerialization>()->Register<CComponentCamera>(SerializationOrder::Camera, *this, &CComponentCamera::Serialize, &CComponentCamera::SerializeXML, &CComponentCamera::NeedSerialize, &CComponentCamera::GetSignature);
    UpdateMaterialCamera();
}

//////////////////////////////////////////////////////////////////////////
bool CComponentCamera::InitComponent(IEntity* pEntity, SEntitySpawnParams& params)
{
    UpdateMaterialCamera();

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentCamera::Reload(IEntity* pEntity, SEntitySpawnParams& params)
{
    UpdateMaterialCamera();
}

//////////////////////////////////////////////////////////////////////////
void CComponentCamera::ProcessEvent(SEntityEvent& event)
{
    switch (event.event)
    {
    case ENTITY_EVENT_INIT:
    case ENTITY_EVENT_XFORM:
    {
        UpdateMaterialCamera();
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentCamera::UpdateMaterialCamera()
{
    _smart_ptr<IMaterial> pMaterial = m_pEntity->GetMaterial();
    if (pMaterial)
    {
        CCamera cam = GetISystem()->GetViewCamera();
        Matrix34 wtm = m_pEntity->GetWorldTM();
        wtm.OrthonormalizeFast();
        cam.SetMatrix(wtm);
        cam.SetFrustum(cam.GetViewSurfaceX(), cam.GetViewSurfaceZ(), m_camera.GetFov(), cam.GetNearPlane(), cam.GetFarPlane(), cam.GetPixelAspectRatio());
    }
}

//////////////////////////////////////////////////////////////////////////
void CComponentCamera::SetCamera(CCamera& cam)
{
    m_camera = cam;
    UpdateMaterialCamera();
}

//////////////////////////////////////////////////////////////////////////
bool CComponentCamera::GetSignature(TSerialize signature)
{
    signature.BeginGroup("ComponentCamera");
    signature.EndGroup();
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CComponentCamera::Serialize(TSerialize ser)
{
}
