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
#include <AzCore/Component/Component.h>
#include <Maestro/Bus/SequenceAgentComponentBus.h>

namespace Maestro
{
    class SequenceAgentExternalCreator
        : public AZ::Component
        , public SequenceAgentExternalBus::Handler
    {
    public:
        AZ_COMPONENT(SequenceAgentExternalCreator, "{AA979CD3-B47B-4710-9689-F20F4CD476BB}");

        ~SequenceAgentExternalCreator() override {}

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        AZ::EntityId AddToEntity(AZ::EntityId entityId) override;
    protected:
        // Required Reflect function.
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("SequenceAgentExternalCreator", 0x201150fe));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("SequenceAgentExternalCreator", 0x201150fe));
        }

    };
} // namespace Maestro