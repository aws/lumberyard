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

#include <AzToolsFramework/Picking/DefaultContextBoundManager.h>

namespace AzToolsFramework
{
    namespace Picking
    {
        class BoundShapeInterface;

        /**
         * Handle creating, destroying and storing all active manipulator
         * bounds for performing raycasts/picking against.
         */
        class ManipulatorBoundManager
            : public DefaultContextBoundManager
        {
        public:
            AZ_CLASS_ALLOCATOR(ManipulatorBoundManager, AZ::SystemAllocator, 0);

            explicit ManipulatorBoundManager(ManipulatorManagerId manipulatorManagerId);
            ~ManipulatorBoundManager() = default;

            /**
             * Perform Raycast against bounds - store all rays hits and sort by hit distance.
             */
            void RaySelect(RaySelectInfo &rayInfo) override;

        protected:
            AZStd::shared_ptr<BoundShapeInterface> CreateShape(const BoundRequestShapeBase& ptrShape, RegisteredBoundId id, AZ::u64 userContext) override;
            void DeleteShape(AZStd::shared_ptr<BoundShapeInterface> pShape) override;

        private:
            AZStd::vector<AZStd::shared_ptr<BoundShapeInterface>> m_bounds; ///< All current manipulator bounds.
        };
    } // namespace Picking
} // namespace AzToolsFramework