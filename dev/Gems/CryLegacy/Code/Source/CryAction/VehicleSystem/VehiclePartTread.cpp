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

// Description : Implements a part for vehicles which uses animated characters


#include "CryLegacy_precompiled.h"

#include "ICryAnimation.h"
#include "IVehicleSystem.h"

#include "CryAction.h"
#include "Vehicle.h"
#include "VehicleComponent.h"
#include "VehiclePartSubPartWheel.h"
#include "VehiclePartTread.h"
#include "VehicleUtils.h"
#include "IShader.h"
#include <CryExtension/CryCreateClassInstance.h>
#include "Components/IComponentRender.h"

//------------------------------------------------------------------------
CVehiclePartTread::CVehiclePartTread()
{
    m_pCharInstance = NULL;
    m_slot = -1;
    m_lastWheelIndex = 0;
    m_lastUpdate = 0.f;
    m_pMaterial = 0;
    m_lastWheelW = 0.0f;
    m_damageRatio = 0.0f;
    m_uvRateMax = 0.f;
    m_pShaderResources = NULL;
    m_pMaterial = NULL;
    CryCreateClassInstance("AnimationPoseModifier_OperatorQueue", m_operatorQueue);
}

//------------------------------------------------------------------------
bool CVehiclePartTread::Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType)
{
    if (!CVehiclePartBase::Init(pVehicle, table, parent, initInfo, eVPT_Tread))
    {
        return false;
    }

    CVehicleParams subTable = table.findChild("Tread"); // Tread subtable
    if (!subTable)
    {
        return false;
    }

    string filename = subTable.getAttr("filename");
    if (filename.empty())
    {
        return false;
    }

    subTable.getAttr("uvRateMax", m_uvRateMax);

    if (table.haveAttr("component"))
    {
        if (CVehicleComponent* pComponent = static_cast<CVehicleComponent*>(m_pVehicle->GetComponent(table.getAttr("component"))))
        {
            pComponent->AddPart(this);
        }
    }

    m_slot = GetEntity()->LoadCharacter(m_slot, filename);

    m_pCharInstance = GetEntity()->GetCharacter(m_slot);
    if (!m_pCharInstance)
    {
        return false;
    }

    if (subTable.haveAttr("materialName"))
    {
        string materialName = subTable.getAttr("materialName");
        materialName.MakeLower();

        _smart_ptr<IMaterial> pMaterial = 0;
        const char* subMtlName = 0;
        int subMtlSlot = -1;

        // find tread material
        IComponentRenderPtr pRenderComponent = GetEntity()->GetComponent<IComponentRender>();
        if (pRenderComponent)
        {
            pMaterial = pRenderComponent->GetRenderMaterial(m_slot);
            if (pMaterial)
            {
                // if matname doesn't fit, look in submaterials
                if (string(pMaterial->GetName()).MakeLower().find(materialName) == string::npos)
                {
                    for (int i = 0; i < pMaterial->GetSubMtlCount(); ++i)
                    {
                        if (string(pMaterial->GetSubMtl(i)->GetName()).MakeLower().find(materialName) != string::npos)
                        {
                            subMtlName = pMaterial->GetSubMtl(i)->GetName();
                            subMtlSlot = i;
                            break;
                        }
                    }
                }
            }
        }

        if (pMaterial)
        {
            // clone
            _smart_ptr<IMaterial> pCloned = pMaterial->GetMaterialManager()->CloneMultiMaterial(pMaterial, subMtlName);
            if (pCloned)
            {
                pRenderComponent->SetSlotMaterial(m_slot, pCloned);
                pMaterial = pCloned;
                m_pMaterial = pMaterial;
            }

            if (subMtlSlot > -1)
            {
                m_pShaderResources = pMaterial->GetShaderItem(subMtlSlot).m_pShaderResources;
            }
            else
            {
                m_pShaderResources = pMaterial->GetShaderItem().m_pShaderResources;
            }
        }

        if (m_pShaderResources)
        {
            for ( auto iter=m_pShaderResources->GetTexturesResourceMap()->begin() ; iter!= m_pShaderResources->GetTexturesResourceMap()->end() ; ++iter )
            {
                SEfResTexture*      pTextureRes = &(iter->second);
                SEfTexModificator&  modif = *pTextureRes->AddModificator();

                modif.SetMember("m_eMoveType[0]", 2.0f);
                modif.SetMember("m_OscRate[0]", 0.0f);
                modif.SetMember("m_OscAmplitude[0]", 1.0f);
            }
        }
    }

    char wheelName[256];

    IDefaultSkeleton& rIDefaultSkeleton = m_pCharInstance->GetIDefaultSkeleton();
    for (int i = 0; i < rIDefaultSkeleton.GetJointCount(); ++i)
    {
        const char* boneName = rIDefaultSkeleton.GetJointNameByID(i);
        if (0 != boneName)
        {
            // extract wheel name
            const size_t len = strcspn(boneName, "_");
            if (len < strlen(boneName) && len < sizeof(wheelName))
            {
                cry_strcpy(wheelName, boneName, len);

                CVehiclePartSubPartWheel* pWheel = (CVehiclePartSubPartWheel*)m_pVehicle->GetPart(wheelName);
                if (pWheel)
                {
                    SWheelInfo wheelInfo;
                    wheelInfo.slot = pWheel->m_slot;
                    wheelInfo.jointId = i;
                    wheelInfo.pWheel = pWheel;
                    m_wheels.push_back(wheelInfo);

                    m_lastWheelIndex = pWheel->GetWheelIndex();
                }
            }
        }
    }

    m_pVehicle->SetObjectUpdate(this, IVehicle::eVOU_AlwaysUpdate);

    m_state = eVGS_Default;
    return true;
}

//------------------------------------------------------------------------
void CVehiclePartTread::Release()
{
    CVehiclePartBase::Release();
}

//------------------------------------------------------------------------
void CVehiclePartTread::Reset()
{
    if (m_damageRatio > 0.0f)
    {
        SetDamageRatio(0.0f);
    }

    SetUVSpeed(0.f);

    if (m_pCharInstance)
    {
        m_pCharInstance->GetISkeletonPose()->SetForceSkeletonUpdate(0);
    }

    CVehiclePartBase::Reset();
}

//------------------------------------------------------------------------
void CVehiclePartTread::OnEvent(const SVehiclePartEvent& event)
{
    CVehiclePartBase::OnEvent(event);

    if (event.type == eVPE_Damaged && event.fparam >= 1.0f)
    {
        SetDamageRatio(event.fparam);
    }
    else if (event.type == eVPE_Repair && event.fparam <= 0.25f)
    {
        SetDamageRatio(0.0f);
    }
}

//------------------------------------------------------------------------
bool CVehiclePartTread::ChangeState(EVehiclePartState state, int flags /* =0 */)
{
    EVehiclePartState old = m_state;
    bool bOK = CVehiclePartBase::ChangeState(state, flags);
    m_state = state;

    if (bOK && m_pVehicle->GetMovement())
    {
        if (m_state > old && m_state == IVehiclePart::eVGS_Damaged1)
        {
            SVehicleMovementEventParams params;
            m_pVehicle->GetMovement()->OnEvent(IVehicleMovement::eVME_TireBlown, params);
        }
        else if (m_state < old && m_state == eVGS_Default)
        {
            SVehicleMovementEventParams params;
            m_pVehicle->GetMovement()->OnEvent(IVehicleMovement::eVME_TireRestored, params);
        }
    }

    return bOK;
}

//------------------------------------------------------------------------
void CVehiclePartTread::SetDamageRatio(float value)
{
    CRY_ASSERT(value >= 0.0f);

    if (value >= 1.0f)
    {
        SetUVSpeed(0.f);
        Hide(true);
    }
    else if (value == 0.0f)
    {
        Hide(false);
    }

    m_damageRatio = value;
}

//------------------------------------------------------------------------
void CVehiclePartTread::InitGeometry()
{
}

//------------------------------------------------------------------------
void CVehiclePartTread::Physicalize()
{
    Update(1.0f);
}

//------------------------------------------------------------------------
const Matrix34& CVehiclePartTread::GetLocalTM(bool relativeToParentPart, bool forced)
{
    return GetEntity()->GetSlotLocalTM(m_slot, relativeToParentPart);
}

//------------------------------------------------------------------------
const Matrix34& CVehiclePartTread::GetWorldTM()
{
    return GetEntity()->GetSlotWorldTM(m_slot);
}

//------------------------------------------------------------------------
const AABB& CVehiclePartTread::GetLocalBounds()
{
    if (m_pCharInstance)
    {
        m_bounds = m_pCharInstance->GetAABB();
        m_bounds.SetTransformedAABB(GetEntity()->GetSlotLocalTM(m_slot, false), m_bounds);
    }
    else
    {
        GetEntity()->GetLocalBounds(m_bounds);
    }

    return m_bounds;
}

//------------------------------------------------------------------------
void CVehiclePartTread::Update(const float frameTime)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

    if (!m_pCharInstance)
    {
        return;
    }

    ISkeletonPose* pSkeletonPose = m_pCharInstance->GetISkeletonPose();

    pSkeletonPose->SetForceSkeletonUpdate(0);

    // force zero if vehicle stopped
    if (m_pVehicle->GetStatus().speed < 0.001f)
    {
        SetUVSpeed(0.0f);
        return;
    }

    if (frameTime > 0.f && !(m_pVehicle->GetStatus().speed > 0.001f && m_damageRatio < 1.f && m_pVehicle->GetGameObject()->IsProbablyVisible() && !m_pVehicle->IsProbablyDistant()))
    {
        return;
    }

    // if visible and not distant and moving and not destroyed, we need a tread update in next frame
    pSkeletonPose->SetForceSkeletonUpdate(1);
    m_pVehicle->NeedsUpdate();

    // animate the UV texture according to the wheels speed
    if (m_uvRateMax > 0.0f)
    {
        IPhysicalEntity* pPhysics = GetEntity()->GetPhysics();
        pe_status_wheel wheelStatus;
        wheelStatus.iWheel = m_lastWheelIndex;

        if (pPhysics && pPhysics->GetStatus(&wheelStatus) != 0)
        {
            if (wheelStatus.w != m_lastWheelW && (m_lastUpdate += frameTime) >= VehicleCVars().v_treadUpdateTime)
            {
                m_lastWheelW = wheelStatus.w;
                SetUVSpeed(wheelStatus.w * wheelStatus.r);

                m_lastUpdate = 0.f;
            }
        }
    }

    // deform the tread to follow the wheels
    QuatT absRoot = pSkeletonPose->GetAbsJointByID(0);

    for (TWheelInfoVector::const_iterator ite = m_wheels.begin(), end = m_wheels.end(); ite != end; ++ite)
    {
        const SWheelInfo& wheelInfo = *ite;
        const Matrix34& slotTM = GetEntity()->GetSlotLocalTM(wheelInfo.slot, true);
        VALIDATE_MAT(slotTM);

        if (m_operatorQueue)
        {
            m_operatorQueue->PushPosition(wheelInfo.jointId,
                IAnimationOperatorQueue::eOp_Override, slotTM.GetTranslation());
        }

#if ENABLE_VEHICLE_DEBUG
        if (VehicleCVars().v_debugdraw == 4)
        {
            Vec3 local = GetEntity()->GetWorldTM().GetInverted() * slotTM.GetTranslation();
            gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(GetEntity()->GetWorldTM() * (local + Vec3((float)sgn(local.x) * 0.5f, 0.f, 0.f)), 0.1f, ColorB(0, 0, 255, 255));
        }
#endif
    }
    ISkeletonAnim* pSkeletonAnim = m_pCharInstance->GetISkeletonAnim();
    pSkeletonAnim->PushPoseModifier(VEH_ANIM_POSE_MODIFIER_LAYER, m_operatorQueue, "VehiclePartAnimatedJoint");
}

//------------------------------------------------------------------------
void CVehiclePartTread::SetUVSpeed(float wheelSpeed)
{
    if (!m_pShaderResources)
    {
        return;
    }

    for ( auto iter=m_pShaderResources->GetTexturesResourceMap()->begin() ; iter!= m_pShaderResources->GetTexturesResourceMap()->end() ; ++iter )
    {
        SEfResTexture*      pTextureRes = &(iter->second);
        SEfTexModificator&  modif = *pTextureRes->AddModificator();

        //minimum speed
        float oscSpeed = (abs(wheelSpeed) > 0.2f) ? -wheelSpeed : 0.f;

        modif.SetMember("m_OscRate[0]", oscSpeed);
    }
}

//------------------------------------------------------------------------
void CVehiclePartTread::Serialize(TSerialize ser, EEntityAspects aspects)
{
    float damageRatio = m_damageRatio;

    CVehiclePartBase::Serialize(ser, aspects);

    if (ser.GetSerializationTarget() != eST_Network)
    {
        if (ser.IsReading())
        {
            if (damageRatio != m_damageRatio)
            {
                SetDamageRatio(m_damageRatio);
            }
        }
    }
}

DEFINE_VEHICLEOBJECT(CVehiclePartTread);
