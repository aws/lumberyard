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

#pragma once

#include "EMotionFXConfig.h"
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/RTTI/RTTI.h>
#include <Integration/System/SystemCommon.h>

namespace EMotionFX
{
    class ActorInstance;

    class EMFX_API DebugDraw
    {
    public:      
        AZ_RTTI(DebugDraw, "{44B1A0DB-422E-40D2-B0BC-54B9D7536E1A}");
        AZ_CLASS_ALLOCATOR_DECL

        struct EMFX_API Line
        {
            AZ::Vector3 m_start;
            AZ::Vector3 m_end;
            AZ::Color m_startColor;
            AZ::Color m_endColor;

            AZ_INLINE Line(const AZ::Vector3& start, const AZ::Vector3& end, const AZ::Color& startColor, const AZ::Color& endColor)
                : m_start(start)
                , m_end(end)
                , m_startColor(startColor) 
                , m_endColor(endColor) 
            {}

            AZ_INLINE Line(const AZ::Vector3& start, const AZ::Vector3& end, const AZ::Color& color)
                : Line(start, end, color, color)
            {}
        };

        class EMFX_API ActorInstanceData
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            ActorInstanceData(ActorInstance* actorInstance)
                : m_actorInstance(actorInstance)
            {
            }

            AZ_INLINE size_t GetNumLines() const                    { return m_lines.size(); }
            AZ_INLINE const AZStd::vector<Line>& GetLines() const   { return m_lines; }

            AZ_INLINE void AddLine(const Line& line)                { m_lines.emplace_back(line); }
            AZ_INLINE void AddLine(const AZ::Vector3& start, const AZ::Vector3& end, const AZ::Color& color) { m_lines.emplace_back(Line(start, end, color, color)); }
            AZ_INLINE void AddLine(const AZ::Vector3& start, const AZ::Vector3& end, const AZ::Color& startColor, const AZ::Color& endColor) { m_lines.emplace_back(Line(start, end, startColor, endColor)); }

            void Clear();
            void Lock();
            void Unlock();

        private:
            void ClearLines();

            ActorInstance*          m_actorInstance;
            AZStd::recursive_mutex  m_mutex;
            AZStd::vector<Line>     m_lines;
        };

        using ActorInstanceDataSet = AZStd::unordered_map<ActorInstance*, ActorInstanceData*>;

        virtual ~DebugDraw() = default;

        void Clear();
        void Lock();
        void Unlock();

        ActorInstanceData* GetActorInstanceData(ActorInstance* actorInstance);
        const ActorInstanceDataSet& GetActorInstanceData() const;

    private:
        friend class ActorInstance;

        ActorInstanceData* RegisterActorInstance(ActorInstance* actorInstance);
        void UnregisterActorInstance(ActorInstance* actorInstance);

        ActorInstanceDataSet m_actorInstanceData;
        AZStd::recursive_mutex m_mutex;
    };
}   // namespace EMotionFX