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
#include <IRenderAuxGeom.h>
#include "CharacterInstance.h"
#include <float.h>

void CSkeletonAnim::SetDesiredMotionParam(EMotionParamID nParameterID, float fParameter, float deltaTime222)
{
    if (nParameterID >= eMotionParamID_COUNT)
    {
        return; //not a valid parameter
    }
    //we store the parameters in the run-time structure of the ParametricSampler
    for (int layer = 0; layer < ISkeletonAnim::LayerCount; ++layer)
    {
        const int animCount = GetNumAnimsInFIFO(layer);
        for (int i = 0; i < animCount; i++)
        {
            CAnimation& anim = GetAnimFromFIFO(layer, i);
            if (anim.GetParametricSampler() == 0)
            {
                continue;
            }

            const uint32 blendingOut = (i < animCount - 1);

            //It's a Parametric Animation
            SParametricSampler& lmg = *anim.GetParametricSampler();
            for (uint32 d = 0; d < lmg.m_numDimensions; d++)
            {
                if (lmg.m_MotionParameterID[d] == nParameterID)
                {
                    const uint32 locked = (lmg.m_MotionParameterFlags[d] & CA_Dim_LockedParameter);
                    const uint32 init  = lmg.m_MotionParameterFlags[d] & CA_Dim_Initialized;
                    if (init == 0 || (locked == 0 && blendingOut == 0)) //if already initialized and locked or blending out, then we can't change the parameter any more
                    {
                        lmg.m_MotionParameter[d] = fParameter;
                        lmg.m_MotionParameterFlags[d] |= CA_Dim_Initialized;
                    }
                }
            }
        }
    }
}

bool CSkeletonAnim::GetDesiredMotionParam(EMotionParamID id, float& value) const
{
    for (int layer = 0; layer < ISkeletonAnim::LayerCount; ++layer)
    {
        uint samplerCount = GetNumAnimsInFIFO(layer);
        uint samplerMax = MAX_EXEC_QUEUE;
        if (samplerMax > samplerCount)
        {
            samplerMax = samplerCount;
        }
        if (samplerCount > samplerMax)
        {
            samplerCount = samplerMax;
        }

        uint samplerActiveCount = 0;
        for (uint i = 0; i < samplerCount; ++i)
        {
            const CAnimation& animation = GetAnimFromFIFO(layer, i);
            samplerActiveCount += animation.IsActivated() ? 1 : 0;
        }

        if (samplerActiveCount > samplerMax)
        {
            samplerActiveCount = samplerMax;
        }
        samplerMax = samplerActiveCount;

        for (int i = samplerMax - 1; i >= 0; --i)
        {
            const CAnimation& animation = GetAnimFromFIFO(layer, i);
            const SParametricSampler* pParametric = animation.GetParametricSampler();
            if (!pParametric)
            {
                continue;
            }

            for (int motionParam = 0; motionParam < MAX_LMG_DIMENSIONS; ++motionParam)
            {
                if (pParametric->m_MotionParameterID[motionParam] == id)
                {
                    value = pParametric->m_MotionParameter[motionParam];
                    return true;
                }
            }
        }
    }

    return false;
}
