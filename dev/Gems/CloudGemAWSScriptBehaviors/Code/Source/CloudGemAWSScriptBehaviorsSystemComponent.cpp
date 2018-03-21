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

#include "CloudGemAWSScriptBehaviors_precompiled.h"

#include "CloudGemAWSScriptBehaviorsSystemComponent.h"
#include "AWSBehaviorLambda.h"
#include "AWSBehaviorURL.h"
#include "AWSBehaviorHTTP.h"
#include "AWSBehaviorS3Download.h"
#include "AWSBehaviorS3Upload.h"
#include "AWSBehaviorS3Presign.h"
#include "AWSBehaviorMap.h"
#include "AWSBehaviorJSON.h"
#include "AWSBehaviorAPI.h"

namespace CloudGemAWSScriptBehaviors
{
    // Perhaps move this to a different file for code gen?
    void CloudGemAWSScriptBehaviorsSystemComponent::AddBehaviors()
    {
        if (!m_alreadyAddedBehaviors)
        {
            m_behaviors.push_back(AZStd::make_unique<AWSBehaviorLambda>());
            m_behaviors.push_back(AZStd::make_unique<AWSBehaviorURL>());
            m_behaviors.push_back(AZStd::make_unique<AWSBehaviorHTTP>());
            m_behaviors.push_back(AZStd::make_unique<AWSBehaviorS3Download>());
            m_behaviors.push_back(AZStd::make_unique<AWSBehaviorS3Upload>());
            m_behaviors.push_back(AZStd::make_unique<AWSBehaviorS3Presign>());
            m_behaviors.push_back(AZStd::make_unique<StringMap>());
            m_behaviors.push_back(AZStd::make_unique<AWSBehaviorJSON>());
            m_behaviors.push_back(AZStd::make_unique<AWSBehaviorAPI>());
            m_alreadyAddedBehaviors = true;
        }
    }

    AZStd::vector<AZStd::unique_ptr<AWSBehaviorBase>> CloudGemAWSScriptBehaviorsSystemComponent::m_behaviors;
    bool CloudGemAWSScriptBehaviorsSystemComponent::m_alreadyAddedBehaviors = false;

    //void CloudGemAWSScriptBehaviorsSystemComponent::InjectBehavior(AZStd::unique_ptr<AWSBehaviorBase> behavior)
    //{
    //    m_behaviors.push_back(behavior);
    //}

    void CloudGemAWSScriptBehaviorsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AddBehaviors();

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemAWSScriptBehaviorsSystemComponent, AZ::Component>()
                ->Version(0);

            for (auto&& behavior : m_behaviors)
            {
                behavior->ReflectSerialization(serialize);
            }

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemAWSScriptBehaviorsSystemComponent>("CloudGemAWSScriptBehaviors", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;

                for (auto&& behavior : m_behaviors)
                {
                    behavior->ReflectEditParameters(ec);
                }
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            for (auto&& behavior : m_behaviors)
            {
                behavior->ReflectBehaviors(behaviorContext);
            }
        }
    }

    void CloudGemAWSScriptBehaviorsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CloudGemAWSScriptBehaviorsService"));
    }

    void CloudGemAWSScriptBehaviorsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CloudGemAWSScriptBehaviorsService"));
    }

    void CloudGemAWSScriptBehaviorsSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("CloudGemFrameworkService"));
    }

    void CloudGemAWSScriptBehaviorsSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CloudGemAWSScriptBehaviorsSystemComponent::Init()
    {
        for (auto&& behavior : m_behaviors)
        {
            behavior->Init();
        }
    }

    void CloudGemAWSScriptBehaviorsSystemComponent::Activate()
    {
        CloudGemAWSScriptBehaviorsRequestBus::Handler::BusConnect();

        for (auto&& behavior : m_behaviors)
        {
            behavior->Activate();
        }
    }

    void CloudGemAWSScriptBehaviorsSystemComponent::Deactivate()
    {
        CloudGemAWSScriptBehaviorsRequestBus::Handler::BusDisconnect();

        for (auto&& behavior : m_behaviors)
        {
            behavior->Deactivate();
        }

        // this forces the vector to release its capacity, clear/shrink_to_fit is not
        m_behaviors.swap(AZStd::vector<AZStd::unique_ptr<AWSBehaviorBase> >());
    }
}

