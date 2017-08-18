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
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/vector.h>

#include <CloudGemAWSScriptBehaviors/CloudGemAWSScriptBehaviorsBus.h>
#include "AWSBehaviorBase.h"

namespace CloudGemAWSScriptBehaviors
{
    class CloudGemAWSScriptBehaviorsSystemComponent
        : public AZ::Component
        , protected CloudGemAWSScriptBehaviorsRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CloudGemAWSScriptBehaviorsSystemComponent, "{03573861-C1BB-4316-AB72-49DFF1DBC694}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // used with testing right now, behaviors should generally be created in the AddBehaviors implementation
//        static void InjectBehavior(AZStd::unique_ptr<AWSBehaviorBase> behavior);

    protected:

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        static void AddBehaviors(); // Add any behaviors you derived from AWSBehaviorBase to the implementation of this function

        static AZStd::vector<AZStd::unique_ptr<AWSBehaviorBase> > m_behaviors;
        static bool m_alreadyAddedBehaviors;
    };
}
