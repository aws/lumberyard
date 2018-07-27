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
#include <platform.h>
#include "AnimUtils.h"
#include "ICryAnimation.h"

void AnimUtils::StartAnimation(ICharacterInstance* pCharacter, const char* pAnimName)
{
    CryCharAnimationParams params(0);

    ISkeletonAnim* pISkeletonAnim = (pCharacter ? pCharacter->GetISkeletonAnim() : 0);

    if (pISkeletonAnim)
    {
        pISkeletonAnim->StopAnimationsAllLayers();
    }

    params.m_nFlags |= (CA_MANUAL_UPDATE | CA_REPEAT_LAST_KEY);

    if (pISkeletonAnim)
    {
        pISkeletonAnim->StartAnimation(pAnimName, params);
    }
}

void AnimUtils::SetAnimationTime(ICharacterInstance* pCharacter, float fNormalizedTime)
{
    assert(fNormalizedTime >= 0.0f && fNormalizedTime <= 1.0f);
    ISkeletonAnim* pISkeletonAnim = (pCharacter ? pCharacter->GetISkeletonAnim() : 0);
    float timeToSet = max(0.0f, fNormalizedTime);

    if (pISkeletonAnim)
    {
        pISkeletonAnim->SetLayerNormalizedTime(0, timeToSet);
    }
}

void AnimUtils::StopAnimations(ICharacterInstance* pCharacter)
{
    ISkeletonAnim* pISkeletonAnim = (pCharacter ? pCharacter->GetISkeletonAnim() : 0);

    if (pISkeletonAnim)
    {
        pISkeletonAnim->StopAnimationsAllLayers();
    }
}