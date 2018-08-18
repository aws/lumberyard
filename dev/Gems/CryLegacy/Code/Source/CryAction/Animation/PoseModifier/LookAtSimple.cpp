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

#include "LookAtSimple.h"

using namespace AnimPoseModifier;

/*
CLookAtSimple
*/

CRYREGISTER_CLASS(CLookAtSimple)

//

CLookAtSimple::CLookAtSimple()
{
    m_state.jointId = -1;
    m_state.jointOffsetRelative.zero();
    m_state.targetGlobal.zero();
    m_state.weight = 1.0f;
};

CLookAtSimple::~CLookAtSimple()
{
}

//

bool CLookAtSimple::ValidateJointId(IDefaultSkeleton& rIDefaultSkeleton)
{
    if (m_stateExecute.jointId < 0)
    {
        return false;
    }
    return m_stateExecute.jointId < rIDefaultSkeleton.GetJointCount();
}

// IAnimationPoseModifier

bool CLookAtSimple::Prepare(const SAnimationPoseModifierParams& params)
{
    m_stateExecute = m_state;
    return true;
}

bool CLookAtSimple::Execute(const SAnimationPoseModifierParams& params)
{
    const IDefaultSkeleton& rIDefaultSkeleton = params.GetIDefaultSkeleton();
    const QuatT& transformation = params.pPoseData->GetJointAbsolute(m_stateExecute.jointId);

    Vec3 offsetAbsolute = m_stateExecute.jointOffsetRelative * transformation.q;
    Vec3 targetAbsolute = m_stateExecute.targetGlobal - params.location.t;
    Vec3 directionAbsolute = (targetAbsolute - (transformation.t + offsetAbsolute)) * params.location.q;
    directionAbsolute.Normalize();

    params.pPoseData->SetJointAbsoluteO(m_stateExecute.jointId, Quat::CreateNlerp(
            transformation.q, Quat::CreateRotationVDir(directionAbsolute, -gf_PI * 0.5f), m_stateExecute.weight));

    int32 parentJointId = params.GetIDefaultSkeleton().GetJointParentIDByID(m_stateExecute.jointId);
    if (parentJointId < 0)
    {
        params.pPoseData->SetJointRelative(m_stateExecute.jointId, transformation);
        return true;
    }

    params.pPoseData->SetJointRelative(m_stateExecute.jointId,
        params.pPoseData->GetJointAbsolute(parentJointId).GetInverted() * transformation);
    return true;
}

void CLookAtSimple::Synchronize()
{
}
