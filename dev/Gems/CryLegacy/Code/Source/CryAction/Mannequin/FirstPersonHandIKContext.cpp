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
#include <CryExtension/CryCreateClassInstance.h>
#include "FirstPersonHandIKContext.h"

namespace PoseAlignerBones
{
    namespace IKTarget
    {
        namespace LeftHand
        {
            static const char* RiflePos = "Bip01 LHand2RiflePos_IKTarget";
            static const char* Weapon = "Bip01 RHand2Weapon_IKTarget";
        }

        namespace RightHand
        {
            static const char* RiflePos = "RHand2RiflePos_IKTarget";
        }
    }

    namespace IKBlend
    {
        namespace RightHand
        {
            static const char* RiflePos = "RHand2RiflePos_IKBlend";
        }
    }
}

CFirstPersonHandIKContext::SParams::SParams()
    :   m_weaponTargetIdx(-1)
    , m_leftHandTargetIdx(-1)
    ,   m_rightBlendIkIdx(-1)
{
}



CFirstPersonHandIKContext::SParams::SParams(IDefaultSkeleton* pIDefaultSkeleton)
{
    m_weaponTargetIdx       = pIDefaultSkeleton->GetJointIDByName(PoseAlignerBones::IKTarget::RightHand::RiflePos);
    m_leftHandTargetIdx = pIDefaultSkeleton->GetJointIDByName(PoseAlignerBones::IKTarget::LeftHand::Weapon);
    m_rightBlendIkIdx = pIDefaultSkeleton->GetJointIDByName(PoseAlignerBones::IKBlend::RightHand::RiflePos);

    assert(m_weaponTargetIdx != -1);
    assert(m_leftHandTargetIdx != -1);
    assert(m_rightBlendIkIdx != -1);
}




CFirstPersonHandIKContext::CFirstPersonHandIKContext()
    :   m_pCharacterInstance(0)
    ,   m_rightOffset(IDENTITY)
    ,   m_leftOffset(IDENTITY)
    ,   m_aimDirection(1.0f, 0.0f, 0.0f)
    ,   m_instanceCount(0)
{
}



CFirstPersonHandIKContext::~CFirstPersonHandIKContext()
{
}



void CFirstPersonHandIKContext::Initialize(ICharacterInstance* pCharacterInstance)
{
    ++m_instanceCount;

    if (m_pCharacterInstance != 0)
    {
        return;
    }

    m_pCharacterInstance = pCharacterInstance;
    CryCreateClassInstance("AnimationPoseModifier_OperatorQueue", m_pPoseModifier);
    m_params = SParams(&m_pCharacterInstance->GetIDefaultSkeleton());
}



void CFirstPersonHandIKContext::Finalize()
{
    --m_instanceCount;
    CRY_ASSERT(m_instanceCount >= 0);
}



void CFirstPersonHandIKContext::Update(float timePassed)
{
    if (m_instanceCount <= 0)
    {
        return;
    }

    m_pCharacterInstance->GetISkeletonAnim()->PushPoseModifier(15, m_pPoseModifier, "FirstPersonHandIKContext");

    const IAnimationOperatorQueue::EOp set = IAnimationOperatorQueue::eOp_Override;
    const IAnimationOperatorQueue::EOp additive = IAnimationOperatorQueue::eOp_Additive;

    int16 blendParentId = m_pCharacterInstance->GetIDefaultSkeleton().GetJointParentIDByID(m_params.m_rightBlendIkIdx);
    QuatT absBlendParentPose = m_pCharacterInstance->GetISkeletonPose()->GetAbsJointByID(blendParentId);
    Vec3 relBlandPos = Vec3(1.0f, 0.0f, 0.0f);
    m_pPoseModifier->PushPosition(m_params.m_rightBlendIkIdx, set, absBlendParentPose * relBlandPos);

    Quat view = Quat::CreateRotationVDir(m_aimDirection);
    Quat invView = view.GetInverted();

    QuatT rightHand = QuatT(IDENTITY);
    rightHand.t = view * m_rightOffset.t;
    rightHand.q = view * (m_rightOffset.q * invView);
    m_pPoseModifier->PushPosition(m_params.m_weaponTargetIdx, additive, rightHand.t);
    m_pPoseModifier->PushOrientation(m_params.m_weaponTargetIdx, additive, rightHand.q);

    QuatT leftHand = QuatT(IDENTITY);
    leftHand.t = view * m_leftOffset.t;
    leftHand.q = view * (m_leftOffset.q * invView);
    m_pPoseModifier->PushPosition(m_params.m_leftHandTargetIdx, additive, leftHand.t);
    m_pPoseModifier->PushOrientation(m_params.m_leftHandTargetIdx, additive, leftHand.q);

    m_rightOffset = QuatT(IDENTITY);
    m_leftOffset = QuatT(IDENTITY);
}



void CFirstPersonHandIKContext::SetAimDirection(Vec3 aimDirection)
{
    m_aimDirection = aimDirection;
}



void CFirstPersonHandIKContext::AddRightOffset(QuatT offset)
{
    m_rightOffset = m_rightOffset * offset;
}



void CFirstPersonHandIKContext::AddLeftOffset(QuatT offset)
{
    m_leftOffset = m_leftOffset * offset;
}




CRYREGISTER_CLASS(CFirstPersonHandIKContext);
