/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#include "CloudGemFramework_precompiled.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/base.h>

#include <CloudGemFrameworkEditorSystemComponent.h>

#include <CloudCanvas/CloudCanvasIdentityBus.h>

#include <CloudCanvas/ICloudCanvas.h>

namespace CloudGemFramework
{

    const char* CloudGemFrameworkEditorSystemComponent::COMPONENT_DISPLAY_NAME = "CloudGemFrameworkEditor";
    const char* CloudGemFrameworkEditorSystemComponent::COMPONENT_DESCRIPTION = "Provides Editor functionality used by other Cloud Canvas gems.";
    const char* CloudGemFrameworkEditorSystemComponent::COMPONENT_CATEGORY = "CloudCanvas";
    const char* CloudGemFrameworkEditorSystemComponent::SERVICE_NAME = "CloudGemFrameworkEditorService";

    void CloudGemFrameworkEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudGemFrameworkEditorSystemComponent, AZ::Component>()
                ->Version(0)
                ->SerializerForEmptyClass();

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<CloudGemFrameworkEditorSystemComponent>(COMPONENT_DISPLAY_NAME, COMPONENT_DESCRIPTION)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, COMPONENT_CATEGORY)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC(COMPONENT_CATEGORY))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {

        }
    }

    void CloudGemFrameworkEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC(SERVICE_NAME));
    }

    void CloudGemFrameworkEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC(SERVICE_NAME));
    }

    void CloudGemFrameworkEditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CloudGemFrameworkEditorSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CloudGemFrameworkEditorSystemComponent::Init()
    {
    }

    void CloudGemFrameworkEditorSystemComponent::Activate()
    {
        CloudCanvas::CloudCanvasEditorRequestBus::Handler::BusConnect();
    }

    void CloudGemFrameworkEditorSystemComponent::Deactivate()
    {
        CloudCanvas::CloudCanvasEditorRequestBus::Handler::BusDisconnect();
    }

    CloudCanvas::AWSClientCredentials CloudGemFrameworkEditorSystemComponent::GetCredentials()
    {
        return m_editorCredentials;
    }

    void CloudGemFrameworkEditorSystemComponent::SetCredentials(const CloudCanvas::AWSClientCredentials& credentials)
    {
        m_editorCredentials = credentials;
    }

    bool CloudGemFrameworkEditorSystemComponent::ApplyConfiguration()
    {
        if (GetISystem()->GetGlobalEnvironment()->IsEditorGameMode())
        {
            EBUS_EVENT(CloudGemFramework::CloudCanvasPlayerIdentityBus, ResetPlayerIdentity);
        }
        // Return true to indicate it's been handled so if we're a game client we don't need to take action
        return true;
    }
}
