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

#include <AzCore/Math/Crc.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/Picking/ContextBoundAPI.h>

namespace AzToolsFramework
{
    namespace Picking
    {
        /**
         * This class serves as the base class for querying and storing all the bounds for the objects you can 
         * pick from a viewport. Derived classes can implement their own collision and intersection detection.
         */
        class DefaultContextBoundManager 
            : public ContextBoundManagerRequestBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(DefaultContextBoundManager, AZ::SystemAllocator, 0);

            DefaultContextBoundManager(AZ::u32 manipulatorManagerId);
            virtual ~DefaultContextBoundManager();

            //////////////////////////////////////////////////////////////////////////
            /// ContextBoundManagerRequestBus::Handler
            RegisteredBoundId UpdateOrRegisterBound(const BoundRequestShapeBase& ptrShape, RegisteredBoundId id, AZ::u64 userContext = 0) override;
            void UnregisterBound(RegisteredBoundId boundId) override;
            void RaySelect(RaySelectInfo &rayInfo);
            //////////////////////////////////////////////////////////////////////////

            void SetBoundValidity(RegisteredBoundId boundId, bool isValid);

        protected:

            virtual AZStd::shared_ptr<BoundShapeInterface> CreateShape(const BoundRequestShapeBase& ptrShape, RegisteredBoundId id, AZ::u64 userContext);
            virtual void DeleteShape(AZStd::shared_ptr<BoundShapeInterface> pShape);

            AZStd::unordered_map<RegisteredBoundId, AZStd::shared_ptr<BoundShapeInterface>> m_boundIdToShapeMap;

            static RegisteredBoundId m_boundsGlobalId;
        };
    } // namespace Picking
} // namespace AzToolsFramework

