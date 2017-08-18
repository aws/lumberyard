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

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <LmbrCentral/Cinematics/SequenceAgentComponentBus.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    class SequenceAgent
    {
        friend class AZ::SerializeContext;
        
    protected:
        // This pure virtual is required for the Editor and RunTime to find the componentTypeId - in the Editor
        // it accounts for the GenericComponentWrapper component
        virtual const AZ::Uuid& GetComponentTypeUuid(const AZ::Component& component) const = 0;

        // this is called on Activate() - it traverses all components on the given entity and fills in m_addressToBehaviorVirtualPropertiesMap
        // with all virtual properties on EBuses it finds. Calling it clears out any previously mapped virtualProperties
        void CacheAllVirtualPropertiesFromBehaviorContext(AZ::Entity* entity);

        AZ::Uuid GetVirtualPropertyTypeId(const LmbrCentral::SequenceComponentRequests::AnimatablePropertyAddress& animatedAddress) const;

        void GetAnimatedPropertyValue(LmbrCentral::SequenceComponentRequests::AnimatedValue& returnValue, AZ::EntityId entityId, const LmbrCentral::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress);
        bool SetAnimatedPropertyValue(AZ::EntityId entityId, const LmbrCentral::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress, const LmbrCentral::SequenceComponentRequests::AnimatedValue& value);

        AZStd::unordered_map<LmbrCentral::SequenceComponentRequests::AnimatablePropertyAddress, AZ::BehaviorEBus::VirtualProperty*> m_addressToBehaviorVirtualPropertiesMap;
    };
} // namespace LmbrCentral

