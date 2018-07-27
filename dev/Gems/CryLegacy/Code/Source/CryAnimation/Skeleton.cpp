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
#include "Skeleton.h"

#include "Model.h"

namespace Skeleton {
    /*
    CPoseData
    */

    CPoseData::CPoseData()
        : m_jointCount(0)
        , m_pJointsRelative(NULL)
        , m_pJointsAbsolute(NULL)
        , m_pJointsStatus(NULL)
        , m_pMemoryPool(NULL)
    {
    }

    CPoseData::~CPoseData()
    {
        AllocationRelease();
    }

    //

    bool CPoseData::AllocateData(const uint jointCount)
    {
        if (m_jointCount == jointCount)
        {
            return true;
        }

        AllocationRelease();

        size_t size =
            Align(jointCount * sizeof(QuatT), 16) +
            Align(jointCount * sizeof(QuatT), 16) +
            Align(jointCount * sizeof(JointState), 16);

        uint8* pBase = NULL;
        if (m_pMemoryPool)
        {
            pBase = (uint8*)m_pMemoryPool->Allocate(size);
        }
        else
        {
            pBase = (uint8*)CryModuleMemalign(size, 16);
        }

        if (!pBase)
        {
            AllocationRelease();
            return false;
        }

        m_pJointsRelative = (QuatT*)pBase;
        pBase += jointCount * sizeof(QuatT);
        m_pJointsAbsolute = (QuatT*)pBase;
        pBase += jointCount * sizeof(QuatT);
        m_pJointsStatus = (JointState*)pBase;

        m_jointCount = jointCount;
        return true;
    }

    void CPoseData::AllocationRelease()
    {
        if (m_pMemoryPool)
        {
            if (m_pJointsRelative)
            {
                m_pMemoryPool->Free(m_pJointsRelative);
            }
        }
        else
        {
            if (m_pJointsRelative)
            {
                CryModuleMemalignFree(m_pJointsRelative);
            }
        }

        m_jointCount = 0;
        m_pJointsRelative = NULL;
        m_pJointsAbsolute = NULL;
        m_pJointsStatus = NULL;
    }

    uint CPoseData::GetAllocationLength() const
    {
        return m_jointCount * (sizeof(QuatT) + sizeof(QuatT) + sizeof(JointState));
    }

    bool CPoseData::Initialize(uint jointCount)
    {
        if (!AllocateData(jointCount))
        {
            return false;
        }
        return true;
    }

    bool CPoseData::Initialize(const CDefaultSkeleton& skeleton)
    {
        MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Character Pose Data");

        if (!Initialize(skeleton.GetJointCount()))
        {
            return false;
        }

        ::memcpy(m_pJointsRelative, skeleton.m_poseDefaultData.m_pJointsRelative, m_jointCount * sizeof(QuatT));
        ::memcpy(m_pJointsAbsolute, skeleton.m_poseDefaultData.m_pJointsAbsolute, m_jointCount * sizeof(QuatT));

        return true;
    }

    bool CPoseData::Initialize(const CPoseData& poseData)
    {
        MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Character Pose Data Cloned");
        if (!AllocateData(poseData.GetJointCount()))
        {
            return false;
        }

        size_t totalSize = Align(m_jointCount * sizeof(QuatT), 16) + Align(m_jointCount * sizeof(QuatT), 16) + Align(m_jointCount * sizeof(JointState), 16);

        cryMemcpy(m_pJointsRelative, poseData.m_pJointsRelative, totalSize);

        return true;
    }

    // IAnimationPoseData

    uint CPoseData::GetJointCount() const
    {
        return m_jointCount;
    }

    void CPoseData::SetJointRelative(const uint index, const QuatT& transformation)
    {
        if (index >= m_jointCount)
        {
            CRY_ASSERT(index < m_jointCount);
            return;
        }
        QuatT* pJointsRelative = m_pJointsRelative;
        pJointsRelative[index] = transformation;
    }

    void CPoseData::SetJointRelativeP(const uint index, const Vec3& position)
    {
        if (index >= m_jointCount)
        {
            CRY_ASSERT(index < m_jointCount);
            return;
        }
        QuatT* pJointsRelative = m_pJointsRelative;
        pJointsRelative[index].t = position;
    }

    void CPoseData::SetJointRelativeO(const uint index, const Quat& orientation)
    {
        if (index >= m_jointCount)
        {
            CRY_ASSERT(index < m_jointCount);
            return;
        }
        QuatT* pJointsRelative = m_pJointsRelative;
        pJointsRelative[index].q = orientation;
    }

    const QuatT& CPoseData::GetJointRelative(const uint index) const
    {
        CRY_ASSERT(index < m_jointCount);
        const QuatT* pJointsRelative = m_pJointsRelative;
        return pJointsRelative[index];
    }

    void CPoseData::SetJointAbsolute(const uint index, const QuatT& transformation)
    {
        if (index >= m_jointCount)
        {
            CRY_ASSERT(index < m_jointCount);
            return;
        }
        QuatT* pJointsAbsolute = m_pJointsAbsolute;
        pJointsAbsolute[index] = transformation;
    }

    void CPoseData::SetJointAbsoluteP(const uint index, const Vec3& position)
    {
        if (index >= m_jointCount)
        {
            CRY_ASSERT(index < m_jointCount);
            return;
        }
        QuatT* pJointsAbsolute = m_pJointsAbsolute;
        pJointsAbsolute[index].t = position;
    }

    void CPoseData::SetJointAbsoluteO(const uint index, const Quat& orientation)
    {
        if (index >= m_jointCount)
        {
            CRY_ASSERT(index < m_jointCount);
            return;
        }
        QuatT* pJointsAbsolute = m_pJointsAbsolute;
        pJointsAbsolute[index].q = orientation;
    }

    const QuatT& CPoseData::GetJointAbsolute(const uint index) const
    {
        CRY_ASSERT(index < m_jointCount);
        const QuatT* pJointsAbsolute = m_pJointsAbsolute;
        return pJointsAbsolute[index];
    }

    //

    void CPoseData::ComputeAbsolutePose(const CDefaultSkeleton& rDefaultSkeleton, bool singleRoot)
    {
        QuatT* const __restrict pRelativePose = GetJointsRelative();
        QuatT* const __restrict pAbsolutePose = GetJointsAbsolute();

        const CDefaultSkeleton::SJoint* pJoints = &rDefaultSkeleton.m_arrModelJoints[0];

        if (singleRoot)
        {
            CryPrefetch(&pJoints[0].m_idxParent);
            pAbsolutePose[0] = pRelativePose[0];
            for (uint i = 1; i < m_jointCount; ++i)
            {
#ifndef _DEBUG
                CryPrefetch(&pJoints[i + 1].m_idxParent);
                CryPrefetch(&pAbsolutePose[i + 4]);
                CryPrefetch(&pRelativePose[i + 4]);
#endif
                ANIM_ASSERT(pRelativePose[i].q.IsUnit());
                ANIM_ASSERT(pRelativePose[i].IsValid());
                ANIM_ASSERT(pAbsolutePose[pJoints[i].m_idxParent].q.IsUnit());
                ANIM_ASSERT(pAbsolutePose[pJoints[i].m_idxParent].IsValid());
                QuatT pose = pAbsolutePose[pJoints[i].m_idxParent] * pRelativePose[i];
                pAbsolutePose[i].t = pose.t;
                pAbsolutePose[i].q = pose.q.GetNormalizedSafe();

                ANIM_ASSERT(pAbsolutePose[i].q.IsUnit());
                ANIM_ASSERT(pAbsolutePose[i].q.IsValid());
                ANIM_ASSERT(pAbsolutePose[i].t.IsValid());
            }
        }
        else
        {
            CryPrefetch(&pJoints[0].m_idxParent);
            for (uint i = 0; i < m_jointCount; ++i)
            {
#ifndef _DEBUG
                CryPrefetch(&pJoints[i + 1].m_idxParent);
                CryPrefetch(&pAbsolutePose[i + 4]);
                CryPrefetch(&pRelativePose[i + 4]);
#endif
                int32 p = pJoints[i].m_idxParent;
                if (p < 0)
                {
                    pAbsolutePose[i] = pRelativePose[i];
                }
                else
                {
                    pAbsolutePose[i] = pAbsolutePose[p] * pRelativePose[i];
                }
            }
        }
    }

    void CPoseData::ComputeRelativePose(const CDefaultSkeleton& rDefaultSkeleton, bool singleRoot)
    {
        QuatT* const __restrict pRelativePose = GetJointsRelative();
        QuatT* const __restrict pAbsolutePose = GetJointsAbsolute();

        const CDefaultSkeleton::SJoint* pJoints = &rDefaultSkeleton.m_arrModelJoints[0];

        if (singleRoot)
        {
            pRelativePose[0] = pAbsolutePose[0];
            for (uint i = 1; i < m_jointCount; ++i)
            {
                int32 p = pJoints[i].m_idxParent;
                pRelativePose[i] = pAbsolutePose[p].GetInverted() * pAbsolutePose[i];
            }
        }
        else
        {
            for (uint i = 0; i < m_jointCount; ++i)
            {
                int32 p = pJoints[i].m_idxParent;
                if (p < 0)
                {
                    pRelativePose[i] = pAbsolutePose[i];
                }
                else
                {
                    pRelativePose[i] = pAbsolutePose[p].GetInverted() * pAbsolutePose[i];
                }
            }
        }
    }

    int CPoseData::GetParentIndex(const CDefaultSkeleton& rDefaultSkeleton, const uint index) const
    {
        return rDefaultSkeleton.GetJointParentIDByID(int(index));
    }

    void CPoseData::ResetToDefault(const CDefaultSkeleton& rDefaultSkeleton)
    {
        QuatT* const __restrict pRelativePose = GetJointsRelative();
        ::memcpy(pRelativePose, rDefaultSkeleton.m_poseDefaultData.m_pJointsRelative, m_jointCount * sizeof(QuatT));

        QuatT* const __restrict pAbsolutePose = GetJointsAbsolute();
        ::memcpy(pAbsolutePose, rDefaultSkeleton.m_poseDefaultData.m_pJointsAbsolute, m_jointCount * sizeof(QuatT));
    }

#ifndef _RELEASE
    uint g_result;
    PREFAST_SUPPRESS_WARNING(6262);
    void CPoseData::ValidateRelative(const CDefaultSkeleton& rDefaultSkeleton) const
    {
        const CDefaultSkeleton::SJoint* pJoints = &rDefaultSkeleton.m_arrModelJoints[0];

        if (Console::GetInst().ca_Validate)
        {
            CRY_ASSERT(m_jointCount <= MAX_JOINT_AMOUNT);
            QuatT g_pAbsolutePose[MAX_JOINT_AMOUNT];
            AABB rAABB;
            rAABB.min.Set(+99999.0f, +99999.0f, +99999.0f);
            rAABB.max.Set(-99999.0f, -99999.0f, -99999.0f);
            const QuatT* const __restrict pRelativePose = GetJointsRelative();
            for (uint i = 0; i < m_jointCount; ++i)
            {
                int32 p = pJoints[i].m_idxParent;
                if (p < 0)
                {
                    g_pAbsolutePose[i] = pRelativePose[i];
                }
                else
                {
                    g_pAbsolutePose[i] = g_pAbsolutePose[p] * pRelativePose[i];
                }
            }

            if (m_jointCount)
            {
                for (uint i = 1; i < m_jointCount; i++)
                {
                    rAABB.Add(g_pAbsolutePose[i].t);
                }
                g_result = uint(g_pAbsolutePose[m_jointCount - 1].q.GetLength() + g_pAbsolutePose[m_jointCount - 1].t.GetLength());
                g_result += uint(rAABB.min.GetLength());
                g_result += uint(rAABB.max.GetLength());
            }
        }
    }

    PREFAST_SUPPRESS_WARNING(6262);
    void CPoseData::ValidateAbsolute(const CDefaultSkeleton& rDefaultSkeleton) const
    {
        const CDefaultSkeleton::SJoint* pJoints = &rDefaultSkeleton.m_arrModelJoints[0];
        if (Console::GetInst().ca_Validate)
        {
            QuatT g_pRelativePose[MAX_JOINT_AMOUNT];
            AABB rAABB;
            rAABB.min.Set(+99999.0f, +99999.0f, +99999.0f);
            rAABB.max.Set(-99999.0f, -99999.0f, -99999.0f);
            const QuatT* const __restrict pAbsolutePose = GetJointsAbsolute();
            for (uint i = 0; i < m_jointCount; ++i)
            {
                int32 p = pJoints[i].m_idxParent;
                if (p < 0)
                {
                    g_pRelativePose[i] = pAbsolutePose[i];
                }
                else
                {
                    g_pRelativePose[i] = pAbsolutePose[p].GetInverted() * pAbsolutePose[i] * g_pRelativePose[p];
                }
            }

            if (m_jointCount)
            {
                for (uint i = 1; i < m_jointCount; i++)
                {
                    rAABB.Add(pAbsolutePose[i].t);
                }
                g_result = uint(g_pRelativePose[m_jointCount - 1].q.GetLength() + g_pRelativePose[m_jointCount - 1].t.GetLength());
                g_result += uint(rAABB.min.GetLength());
                g_result += uint(rAABB.max.GetLength());
            }
        }
    }

    void CPoseData::Validate(const CDefaultSkeleton& rDefaultSkeleton) const
    {
        ValidateRelative(rDefaultSkeleton);
        ValidateAbsolute(rDefaultSkeleton);
    }
#endif
} //namespace Skeleton
