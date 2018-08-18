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
#include "VehicleViewThirdPerson_Bone.h"

const char* CVehicleViewThirdPersonBone::m_name = "ThirdPersonBone";

CVehicleViewThirdPersonBone::CVehicleViewThirdPersonBone()
    : m_directionBoneId(-1)
    , m_offset(0.f, 0.f, 0.f)
    , m_distance(0.f)
    , m_position(ZERO)
    , m_rotation(ZERO)
{
    m_additionalRotation.SetIdentity();
}

CVehicleViewThirdPersonBone::~CVehicleViewThirdPersonBone()
{
}

bool CVehicleViewThirdPersonBone::Init(IVehicleSeat* pSeat, const CVehicleParams& table)
{
    if (!CVehicleViewBase::Init(pSeat, table))
    {
        return false;
    }

    if (ICharacterInstance* pCharacterInstance = m_pVehicle->GetEntity()->GetCharacter(0))
    {
        IDefaultSkeleton& rIDefaultSkeleton =  pCharacterInstance->GetIDefaultSkeleton();
        {
            if (table.haveAttr("DirectionBone"))
            {
                const char* boneName = table.getAttr("DirectionBone");
                m_directionBoneId = rIDefaultSkeleton.GetJointIDByName(boneName);
            }

            if (CVehicleParams paramsTable = table.findChild(m_name))
            {
                if (paramsTable.haveAttr("rotation"))
                {
                    Vec3 angles;
                    paramsTable.getAttr("rotation", angles);
                    angles *= gf_PI / 180.0f;

                    m_additionalRotation.SetRotationXYZ((Ang3)angles);
                }

                paramsTable.getAttr("offset", m_offset);
                paramsTable.getAttr("distance", m_distance);
            }
        }
    }

    return true;
}

void CVehicleViewThirdPersonBone::OnAction(const TVehicleActionId actionId, int activationMode, float value)
{
    //ToDo: Zoom
}

void CVehicleViewThirdPersonBone::UpdateView(SViewParams& viewParams, EntityId playerId)
{
    viewParams.position = m_position;
    viewParams.rotation = m_rotation;

    IActor* pActor = CCryAction::GetCryAction()->GetIActorSystem()->GetActor(playerId);
    if (pActor && pActor->IsClient())
    {
        pActor->SetViewInVehicle(viewParams.rotation);
    }
}

void CVehicleViewThirdPersonBone::OnStartUsing(EntityId passengerId)
{
    CVehicleViewBase::OnStartUsing(passengerId);
}

void CVehicleViewThirdPersonBone::Update(const float frameTime)
{
    CVehicleViewBase::Update(frameTime);

    IEntity* pVehicleEntity = m_pVehicle->GetEntity();
    const Matrix34& rWorldTM = pVehicleEntity->GetWorldTM();

    QuatT worldRot(rWorldTM);
    Vec3 worldPos(ZERO);

    Vec3 vTotalOffset = m_offset;
    vTotalOffset.y -= m_distance;

    if (ICharacterInstance* pCharacterInstance = pVehicleEntity->GetCharacter(0))
    {
        ISkeletonPose* pSkeletonPose = pCharacterInstance->GetISkeletonPose();
        if (pSkeletonPose)
        {
            const QuatT& rJointWorld = pSkeletonPose->GetAbsJointByID(m_directionBoneId);
            worldRot = (worldRot * rJointWorld * QuatT(m_additionalRotation, Vec3(ZERO)));
            worldPos = worldRot * vTotalOffset;
        }
    }

    m_position = worldPos;
    m_rotation = worldRot.q;
}

DEFINE_VEHICLEOBJECT(CVehicleViewThirdPersonBone);
