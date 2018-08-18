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

#ifndef CRYINCLUDE_CRYANIMATION_SKELETON_H
#define CRYINCLUDE_CRYANIMATION_SKELETON_H
#pragma once

#include "Memory/Pool.h"
#include "JointState.h"

class CDefaultSkeleton;

namespace Skeleton
{
    class CPoseData
        :   public IAnimationPoseData
    {
    public:
        static CPoseData* GetPoseData(IAnimationPoseData* pPoseData)
        {
            if (pPoseData)
            {
                return (CPoseData*)pPoseData;
            }
            return NULL;
        }

    public:
        CPoseData();
        ~CPoseData();

    public:
        void SetMemoryPool(Memory::CPool* pMemoryPool) { m_pMemoryPool = pMemoryPool; }

        bool Initialize(uint jointCount);
        bool Initialize(const CDefaultSkeleton& skeleton);
        bool Initialize(const CPoseData& poseData);

        bool AllocateData(const uint jointCount);
        void AllocationRelease();
        uint GetAllocationLength() const;

        void ComputeAbsolutePose(const CDefaultSkeleton& rDefaultSkeleton, bool singleRoot = false);
        void ComputeRelativePose(const CDefaultSkeleton& rDefaultSkeleton, bool singleRoot = false);

        ILINE void GetMemoryUsage(ICrySizer* pSizer) const;

        int GetParentIndex(const CDefaultSkeleton& rDefaultSkeleton, const uint index) const;

        void ResetToDefault(const CDefaultSkeleton& rDefaultSkeleton);

        // IAnimationPoseData
    public:
        virtual uint GetJointCount() const;

        virtual void SetJointRelative(const uint index, const QuatT& transformation);
        virtual void SetJointRelativeP(const uint index, const Vec3& position);
        virtual void SetJointRelativeO(const uint index, const Quat& orientation);
        virtual const QuatT& GetJointRelative(const uint index) const;
        virtual void SetJointAbsolute(const uint index, const QuatT& transformation);

        virtual void SetJointAbsoluteP(const uint index, const Vec3& position);
        virtual void SetJointAbsoluteO(const uint index, const Quat& orientation);
        virtual const QuatT& GetJointAbsolute(const uint index) const;

        // OBSOLETE:
        // Use single joint Set/Get methods instead!
    public:
        ILINE QuatT* GetJointsRelative() { return m_pJointsRelative; }
        ILINE const QuatT* GetJointsRelative() const { return m_pJointsRelative; }

        ILINE QuatT* GetJointsAbsolute() { return m_pJointsAbsolute; }
        ILINE const QuatT* GetJointsAbsolute() const { return m_pJointsAbsolute; }

        ILINE JointState* GetJointsStatus() { return m_pJointsStatus; }
        ILINE const JointState* GetJointsStatus() const { return m_pJointsStatus; }

        ILINE const QuatT* GetJointsRelativeMain() const { return m_pJointsRelative; }
        ILINE const QuatT* GetJointsAbsoluteMain() const { return m_pJointsAbsolute; }
        ILINE const JointState* GetJointsStatusMain() const { return m_pJointsStatus; }

#ifndef _RELEASE
        void ValidateRelative(const CDefaultSkeleton& rDefaultSkeleton) const;
        void ValidateAbsolute(const CDefaultSkeleton& rDefaultSkeleton) const;
        void Validate(const CDefaultSkeleton& rDefaultSkeleton) const;
#else
        ILINE void ValidateRelative(const CDefaultSkeleton& rDefaultSkeleton) const { }
        ILINE void ValidateAbsolute(const CDefaultSkeleton& rDefaultSkeleton) const { }
        ILINE void Validate(const CDefaultSkeleton& rDefaultSkeleton) const { }
#endif

    private:
        uint m_jointCount;
        QuatT* m_pJointsRelative;
        QuatT* m_pJointsAbsolute;
        JointState* m_pJointsStatus;

        Memory::CPool* m_pMemoryPool;
    };

    ILINE void CPoseData::GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_pJointsRelative, m_jointCount * sizeof(QuatT));
        pSizer->AddObject(m_pJointsAbsolute, m_jointCount * sizeof(QuatT));
        pSizer->AddObject(m_pJointsStatus, m_jointCount * sizeof(JointState));
    }
}; // namespace Skeleton

#endif // CRYINCLUDE_CRYANIMATION_SKELETON_H
