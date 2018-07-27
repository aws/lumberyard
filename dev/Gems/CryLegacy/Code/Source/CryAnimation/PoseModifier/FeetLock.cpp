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

#include <CryExtension/Impl/ClassWeaver.h>
#include <CryExtension/Impl/ICryFactoryRegistryImpl.h>
#include <CryExtension/CryCreateClassInstance.h>

#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include "../CharacterInstance.h"
#include "../Model.h"
#include "../CharacterManager.h"

#include "PoseModifierHelper.h"
#include "FeetLock.h"


/*

CFeetPoseStore

*/

CRYREGISTER_CLASS(CFeetPoseStore)

//

CFeetPoseStore::CFeetPoseStore()
{
}

CFeetPoseStore::~CFeetPoseStore()
{
}


// IAnimationPoseModifier
bool CFeetPoseStore::Execute(const SAnimationPoseModifierParams& params)
{
    Skeleton::CPoseData* pPoseData = Skeleton::CPoseData::GetPoseData(params.pPoseData);
    if (!pPoseData)
    {
        return false;
    }

    const CDefaultSkeleton& rDefaultSkeleton    = (const CDefaultSkeleton&)params.GetIDefaultSkeleton();
    QuatT* pRelPose = pPoseData->GetJointsRelative();
    QuatT* pAbsPose = pPoseData->GetJointsAbsolute();

    for (uint32 h = 0; h < MAX_FEET_AMOUNT; h++)
    {
        m_pFeetData[h].m_IsEndEffector = 0;
        uint32 hinit = rDefaultSkeleton.m_strFeetLockIKHandle[h].size();
        if (hinit == 0)
        {
            continue;
        }
        const char* strLIKSolver = rDefaultSkeleton.m_strFeetLockIKHandle[h].c_str();
        LimbIKDefinitionHandle nHandle = CCrc32::ComputeLowercase(strLIKSolver);
        int32 idxDefinition = rDefaultSkeleton.GetLimbDefinitionIdx(nHandle);
        if (idxDefinition < 0)
        {
            continue;
        }
        const IKLimbType& rIKLimbType = rDefaultSkeleton.m_IKLimbTypes[idxDefinition];
        uint32 numLinks = rIKLimbType.m_arrRootToEndEffector.size();
        int32 nRootTdx = rIKLimbType.m_arrRootToEndEffector[0];
        QuatT qWorldEndEffector = pRelPose[nRootTdx];
        for (uint32 i = 1; i < numLinks; i++)
        {
            int32 cid = rIKLimbType.m_arrRootToEndEffector[i];
            qWorldEndEffector = qWorldEndEffector * pRelPose[cid];
            qWorldEndEffector.q.Normalize();
        }
        m_pFeetData[h].m_WorldEndEffector = qWorldEndEffector;
        ANIM_ASSERT(m_pFeetData[h].m_WorldEndEffector.IsValid());
        m_pFeetData[h].m_IsEndEffector = 1;
    }



#ifdef _DEBUG_CRYANIMATION
    uint32 numJoints = rDefaultSkeleton.GetJointCount();
    for (uint32 j = 0; j < numJoints; j++)
    {
        CRY_ASSERT(pRelPose[j].q.IsUnit());
        CRY_ASSERT(pAbsPose[j].q.IsUnit());
        CRY_ASSERT(pRelPose[j].IsValid());
        CRY_ASSERT(pAbsPose[j].IsValid());
    }
#endif

    return false;
}

/*

CFeetPoseRestore

*/

CRYREGISTER_CLASS(CFeetPoseRestore)

//

CFeetPoseRestore::CFeetPoseRestore()
{
}

CFeetPoseRestore::~CFeetPoseRestore()
{
}

// IAnimationPoseModifier
bool CFeetPoseRestore::Execute(const SAnimationPoseModifierParams& params)
{
    Skeleton::CPoseData* pPoseData = Skeleton::CPoseData::GetPoseData(params.pPoseData);
    if (!pPoseData)
    {
        return false;
    }

    const CDefaultSkeleton& rDefaultSkeleton    = (const CDefaultSkeleton&)params.GetIDefaultSkeleton();
    QuatT* const __restrict pAbsPose = pPoseData->GetJointsAbsolute();
    QuatT* const __restrict pRelPose = pPoseData->GetJointsRelative();

    for (uint32 i = 0; i < MAX_FEET_AMOUNT; i++)
    {
        if (m_pFeetData[i].m_IsEndEffector == 0)
        {
            continue;
        }
        const char* strLIKSolver = rDefaultSkeleton.m_strFeetLockIKHandle[i].c_str();
        LimbIKDefinitionHandle nHandle = CCrc32::ComputeLowercase(strLIKSolver);
        int32 idxDefinition = rDefaultSkeleton.GetLimbDefinitionIdx(nHandle);
        if (idxDefinition < 0)
        {
            continue;
        }
        ANIM_ASSERT(m_pFeetData->m_WorldEndEffector.IsValid());
        const IKLimbType& rIKLimbType = rDefaultSkeleton.m_IKLimbTypes[idxDefinition];
        uint32 numLinks = rIKLimbType.m_arrRootToEndEffector.size();
        int32 lFootParentIdx = rIKLimbType.m_arrRootToEndEffector[numLinks - 2];
        int32 lFootIdx = rIKLimbType.m_arrRootToEndEffector[numLinks - 1];
        PoseModifierHelper::IK_Solver(PoseModifierHelper::GetDefaultSkeleton(params), nHandle, m_pFeetData[i].m_WorldEndEffector.t, *pPoseData);
        pAbsPose[lFootIdx] = m_pFeetData[i].m_WorldEndEffector;
        pRelPose[lFootIdx] = pAbsPose[lFootParentIdx].GetInverted() * m_pFeetData[i].m_WorldEndEffector;
    }


#ifdef _DEBUG_CRYANIMATION
    uint32 numJoints = rDefaultSkeleton.GetJointCount();
    for (uint32 j = 0; j < numJoints; j++)
    {
        CRY_ASSERT(pRelPose[j].q.IsUnit());
        CRY_ASSERT(pAbsPose[j].q.IsUnit());
        CRY_ASSERT(pRelPose[j].IsValid());
        CRY_ASSERT(pAbsPose[j].IsValid());
    }
#endif

    return true;
}

/*

CFeetLock

*/

CFeetLock::CFeetLock()
{
    ::CryCreateClassInstance<IAnimationPoseModifier>(
        "AnimationPoseModifier_FeetPoseStore", m_store);
    //CRY_ASSERT_MESSAGE(m_store.get() != nullptr, "CFeetLock::CFeetLock: not able to allocate feet pose store");

    CFeetPoseStore* pStore = static_cast<CFeetPoseStore*>(m_store.get());
    if (pStore)
    {
        pStore->m_pFeetData = &m_FeetData[0];
    }

    ::CryCreateClassInstance<IAnimationPoseModifier>(
        "AnimationPoseModifier_FeetPoseRestore", m_restore);
    CRY_ASSERT_MESSAGE(m_restore.get() != nullptr, "CFeetLock::CFeetLock: not able to allocate feet pose store");

    CFeetPoseRestore* pRestore = static_cast<CFeetPoseRestore*>(m_restore.get());
    if (pRestore)
    {
        pRestore->m_pFeetData = &m_FeetData[0];
    }
}
