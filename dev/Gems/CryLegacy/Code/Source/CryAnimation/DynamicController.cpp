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
#include "ICryAnimation.h"
#include "DynamicController.h"
#include "Model.h"

CDynamicControllerJoint::CDynamicControllerJoint(const CDefaultSkeleton* parentSkeleton, uint32 dynamicControllerId)
    : IDynamicController(parentSkeleton, dynamicControllerId)
    , m_nJointIndex(-1)
{
    const CDefaultSkeleton::SJoint* pJoints = &parentSkeleton->m_arrModelJoints[0];
    uint32 jointCount = parentSkeleton->GetJointCount();

    for (uint32 i = 0; i < jointCount; ++i)
    {
        if (pJoints[i].m_nJointCRC32 == dynamicControllerId)
        {
            m_nJointIndex = i;
            break;
        }
    }

    if (m_nJointIndex < 0)
    {
        CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "Could not find dynamic controller joint with ID %i in skeleton %s, there has been an error in the skeleton setup", dynamicControllerId, parentSkeleton->GetModelFilePath());
    }
}

float CDynamicControllerJoint::GetValue(const ICharacterInstance& charInstance) const
{
    const QuatT poseData = charInstance.GetISkeletonPose()->GetRelJointByID(m_nJointIndex);

    return clamp_tpl(poseData.t.x, 0.0f, 1.0f);
}
