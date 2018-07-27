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

#ifndef CRYINCLUDE_CRYANIMATION_COMMAND_BUFFER_H
#define CRYINCLUDE_CRYANIMATION_COMMAND_BUFFER_H
#pragma once

#include "Skeleton.h"

class CDefaultSkeleton;
class CCharInstance;

namespace Command {
    // TODO: Needs rename
    enum
    {
        TmpBuffer = 0,
        TargetBuffer = 3,
    };

    class CState
    {
    public:
        CState()
        {
            Reset();
        }

    public:
        bool Initialize(CCharInstance* pInstance, const QuatTS& location);

        ILINE void Reset()
        {
            m_lod = 0;

            m_pJointMask = NULL;
            m_jointMaskCount = 0;
        }

        ILINE bool IsJointActive(uint32 nameCrc32) const
        {
            if (!m_pJointMask)
            {
                return true;
            }

            const uint32* pValue = std::lower_bound(&m_pJointMask[0], &m_pJointMask[m_jointMaskCount], nameCrc32);
            if (pValue == &m_pJointMask[m_jointMaskCount] || *pValue != nameCrc32)
            {
                return false;
            }

            return true;
        }

    public:
        CDefaultSkeleton* m_pDefaultSkeleton;

        QuatTS m_location;

        Skeleton::CPoseData* m_pPoseData;

        uint32 m_jointCount;

        uint32 m_lod;

        f32 m_timeDelta;

        const uint32* m_pJointMask;
        uint32 m_jointMaskCount;

        // NOTE: Do not access this, here only for PoseModifier back-compat.
        CCharInstance* m_pInstance;
    };

    class CBuffer
    {
    public:
        ILINE uint32 GetLengthTotal() const { return sizeof(m_pBuffer); }
        ILINE uint32 GetLengthUsed() const { return uint32(m_pCommands - m_pBuffer); }
        ILINE uint32 GetLengthFree() const { return GetLengthTotal() - GetLengthUsed(); }
        ILINE uint32 GetCommandCount() const { return m_commandCount; }

        bool Initialize(CCharInstance* pInstance, const QuatTS& location);

        void SetPoseData(Skeleton::CPoseData& poseData);

        template <class Type>
        Type* CreateCommand()
        {
            static_assert(0 == (sizeof(Type) & 3), "Illegal size for command type (must be a multiple of 4 bytes)");

            uint32 lengthFree = GetLengthFree();
            if (lengthFree < sizeof(Type))
            {
                CryFatalError("CryAnimation: CommandBuffer overflow!");
                return NULL;
            }

            Type* pCommand = (Type*)m_pCommands;
            *(uint8*)pCommand = Type::ID;
            m_pCommands += sizeof(Type);
            m_commandCount++;
            return pCommand;
        }

        void Execute();

    private:
        void DebugDraw();

    private:
        uint8 m_pBuffer[2048];

        uint8* m_pCommands;
        uint8 m_commandCount;

        CState m_state;

        // NOTE: Do not access this, here only for PoseModifier back-compat.
        CCharInstance* m_pInstance;
    };
} // namespace Command

#endif // CRYINCLUDE_CRYANIMATION_COMMAND_BUFFER_H
