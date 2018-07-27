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

#ifndef CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_USESMARTOBJECT_H
#define CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_USESMARTOBJECT_H
#pragma once

#include "MovementBlock_UseExactPositioningBase.h"

namespace Movement
{
    namespace MovementBlocks
    {
        // If the exact positioning fails to position the character at the
        // start of the smart object it reports the error through the bubbles
        // and teleports to the end of it, and then proceeds with the plan.

        class UseSmartObject
            : public UseExactPositioningBase
        {
        public:
            typedef UseExactPositioningBase Base;

            UseSmartObject(const CNavPath& path, const PathPointDescriptor::OffMeshLinkData& mnmData, const MovementStyle& style);
            virtual const char* GetName() const override { return "UseSmartObject"; }
            virtual bool InterruptibleNow() const { return m_state == Prepare; }
            virtual void Begin(IMovementActor& actor) override;
            virtual Status Update(const MovementUpdateContext& context) override;
            void SetUpcomingPath(const CNavPath& path);
            void SetUpcomingStyle(const MovementStyle& style);

        protected:
            virtual void OnTraverseStarted(const MovementUpdateContext& context) override;

        private:
            virtual UseExactPositioningBase::TryRequestingExactPositioningResult TryRequestingExactPositioning(const MovementUpdateContext& context) override;
            virtual void HandleExactPositioningError(const MovementUpdateContext& context) override;

            Vec3 GetSmartObjectEndPosition() const;

        private:
            CNavPath m_upcomingPath;
            MovementStyle m_upcomingStyle;
            const PathPointDescriptor::OffMeshLinkData m_smartObjectMNMData;
            float m_timeSpentWaitingForSmartObjectToBecomeFree;
        };
    }
}

#endif // CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTBLOCK_USESMARTOBJECT_H
