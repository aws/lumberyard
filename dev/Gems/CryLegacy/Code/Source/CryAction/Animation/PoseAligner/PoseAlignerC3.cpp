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
#include <CryExtension/Impl/ClassWeaver.h>
#include <ICryAnimation.h>

#include "PoseAligner.h"
#include "PoseAlignerBoneNames.h"

#define UNKNOWN_GROUND_HEIGHT -1E10f

// This code does the initialization of the PoseAligner. Currently all values
// are hard-coded here, however the plan is to have all of these be read from
// auxiliary parameters in the CHRPARAMS or as part of the entity properties.

ILINE bool InitializePoseAlignerBipedHuman(PoseAligner::CPose& pose, IEntity* entity, AZ::EntityId entityId, ICharacterInstance& character)
{
    IDefaultSkeleton& rIDefaultSkeleton = character.GetIDefaultSkeleton();
    int jointIndexRoot = rIDefaultSkeleton.GetJointIDByName(PoseAlignerBones::Biped::Pelvis);
    if (jointIndexRoot < 0)
    {
        return false;
    }
    int jointIndexFrontLeft = rIDefaultSkeleton.GetJointIDByName(PoseAlignerBones::Biped::LeftPlaneTarget);
    if (jointIndexFrontLeft < 0)
    {
        return false;
    }
    int jointIndexFrontRight = rIDefaultSkeleton.GetJointIDByName(PoseAlignerBones::Biped::RightPlaneTarget);
    if (jointIndexFrontRight < 0)
    {
        return false;
    }

    int jointIndexLeftBlend = rIDefaultSkeleton.GetJointIDByName(PoseAlignerBones::Biped::LeftPlaneWeight);
    int jointIndexRightBlend = rIDefaultSkeleton.GetJointIDByName(PoseAlignerBones::Biped::RightPlaneWeight);

    if (!pose.Initialize(entity, entityId, character, jointIndexRoot))
    {
        return false;
    }

    pose.SetRootOffsetMinMax(-0.4f, 0.0f);

    PoseAligner::SChainDesc chainDesc;
    chainDesc.name = "Left";
    chainDesc.eType = IAnimationPoseAlignerChain::eType_Limb;
    chainDesc.solver = CCrc32::ComputeLowercase("LftLeg01");
    chainDesc.contactJointIndex = jointIndexFrontLeft;
    chainDesc.targetBlendJointIndex = jointIndexLeftBlend;
    chainDesc.bBlendProcedural = true;
    chainDesc.bTargetSmoothing = true;
    chainDesc.offsetMin = Vec3(0.0f, 0.0f, 0.0f);
    chainDesc.offsetMax = Vec3(0.0f, 0.0f, 0.6f);
    {
        PoseAligner::CContactRaycastPtr pContactRaycast = new PoseAligner::CContactRaycast(entity);
        if (pContactRaycast)
        {
            pContactRaycast->SetLength(1.0f);
            chainDesc.pContactReporter = pContactRaycast;
        }
        if (!pose.CreateChain(chainDesc))
        {
            return false;
        }
    }

    chainDesc.name = "Right";
    chainDesc.eType = IAnimationPoseAlignerChain::eType_Limb;
    chainDesc.solver = CCrc32::ComputeLowercase("RgtLeg01");
    chainDesc.contactJointIndex = jointIndexFrontRight;
    chainDesc.targetBlendJointIndex = jointIndexRightBlend;
    chainDesc.bBlendProcedural = true;
    chainDesc.bTargetSmoothing = true;
    chainDesc.offsetMin = Vec3(0.0f, 0.0f, 0.0f);
    chainDesc.offsetMax = Vec3(0.0f, 0.0f, 0.6f);
    {
        PoseAligner::CContactRaycastPtr pContactRaycast = new PoseAligner::CContactRaycast(entity);
        if (pContactRaycast)
        {
            pContactRaycast->SetLength(1.0f);
            chainDesc.pContactReporter = pContactRaycast;
        }
        if (!pose.CreateChain(chainDesc))
        {
            return false;
        }
    }

    return pose.GetChainCount() != 0;
}

class CPoseAlignerLimbIK
    : PoseAligner::CPose
{
    CRYGENERATE_CLASS(CPoseAlignerLimbIK, "PoseAlignerLimbIK", 0xf5381a4c1374ff00, 0x8de19ba730cf572b)

public:
    virtual bool Initialize(IEntity& entity);
    virtual bool Initialize(AZ::EntityId id, ICharacterInstance& character);
private:

    bool InitializePoseAligner(IEntity* entity, AZ::EntityId id, ICharacterInstance& character);
};

CPoseAlignerLimbIK::CPoseAlignerLimbIK()
{
}

CPoseAlignerLimbIK::~CPoseAlignerLimbIK()
{
}

bool CPoseAlignerLimbIK::Initialize(IEntity& entity)
{
    ICharacterInstance* pCharacter = entity.GetCharacter(0);
    if (!pCharacter)
    {
        return false;
    }

    return InitializePoseAligner(&entity, AZ::EntityId(), *pCharacter);
}

bool CPoseAlignerLimbIK::Initialize(AZ::EntityId id, ICharacterInstance& character)
{
    return InitializePoseAligner(nullptr, id, character);
}

bool CPoseAlignerLimbIK::InitializePoseAligner(IEntity* entity, AZ::EntityId entityId, ICharacterInstance& character)
{
    if (m_bInitialized)
    {
        return m_pEntity != NULL;
    }

    if (!InitializePoseAlignerBipedHuman(*this, entity, entityId, character))
    {
        m_bInitialized = true;
        return false;
    }

    m_bInitialized = true;
    return GetChainCount() != 0;
}

CRYREGISTER_CLASS(CPoseAlignerLimbIK)
