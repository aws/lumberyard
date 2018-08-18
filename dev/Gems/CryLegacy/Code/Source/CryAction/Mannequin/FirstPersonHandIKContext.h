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

#ifndef CRYINCLUDE_CRYACTION_MANNEQUIN_FIRSTPERSONHANDIKCONTEXT_H
#define CRYINCLUDE_CRYACTION_MANNEQUIN_FIRSTPERSONHANDIKCONTEXT_H
#pragma once

#include <CryExtension/Impl/ClassWeaver.h>
#include "ICryAnimation.h"
#include "ICryMannequin.h"



class CFirstPersonHandIKContext
    : public IProceduralContext
{
private:
    struct SParams
    {
        SParams();
        SParams(IDefaultSkeleton* pIDefaultSkeleton);

        int m_weaponTargetIdx;
        int m_leftHandTargetIdx;
        int m_rightBlendIkIdx;
    };

public:
    PROCEDURAL_CONTEXT(CFirstPersonHandIKContext, "FirstPersonHandIK", 0xd8a55b349caa4b53, 0x89bcf1708d565bc3);

    virtual void Initialize(ICharacterInstance* pCharacterInstance);
    virtual void Finalize();
    virtual void Update(float timePassed);

    virtual void SetAimDirection(Vec3 aimDirection);
    virtual void AddRightOffset(QuatT offset);
    virtual void AddLeftOffset(QuatT offset);

private:
    SParams m_params;
    IAnimationOperatorQueuePtr m_pPoseModifier;
    ICharacterInstance* m_pCharacterInstance;

    QuatT m_rightOffset;
    QuatT m_leftOffset;
    Vec3 m_aimDirection;
    int m_instanceCount;
};


#endif // CRYINCLUDE_CRYACTION_MANNEQUIN_FIRSTPERSONHANDIKCONTEXT_H
