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
#include "LegacyTimeDemoRecorder_precompiled.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "LegacyTimeDemoRecorderSystemComponent.h"

namespace LegacyTimeDemoRecorder
{
    void LegacyTimeDemoRecorderSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LegacyTimeDemoRecorderSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<LegacyTimeDemoRecorderSystemComponent>("LegacyTimeDemoRecorder", "[Provides an implementation of ITimeDemoRecorder. This logic was originally in CryAction.]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void LegacyTimeDemoRecorderSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("LegacyTimeDemoRecorderService"));
    }

    void LegacyTimeDemoRecorderSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LegacyTimeDemoRecorderService"));
    }

    void LegacyTimeDemoRecorderSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void LegacyTimeDemoRecorderSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void LegacyTimeDemoRecorderSystemComponent::Init()
    {
    }

    void LegacyTimeDemoRecorderSystemComponent::Activate()
    {
        if (m_canActivateTimeRecorder)
        {
            delete m_legacyTimeDemoRecorder;
            m_legacyTimeDemoRecorder = new CTimeDemoRecorder();
        }
        LegacyTimeDemoRecorderRequestBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
    }

    void LegacyTimeDemoRecorderSystemComponent::Deactivate()
    {
        CrySystemEventBus::Handler::BusConnect();
        LegacyTimeDemoRecorderRequestBus::Handler::BusDisconnect();

        delete m_legacyTimeDemoRecorder;
        m_legacyTimeDemoRecorder = nullptr;
    }

    LegacyTimeDemoRecorderSystemComponent::~LegacyTimeDemoRecorderSystemComponent()
    {
        delete m_legacyTimeDemoRecorder;
        m_legacyTimeDemoRecorder = nullptr;
    }

    void LegacyTimeDemoRecorderSystemComponent::OnCrySystemInitialized(ISystem&, const SSystemInitParams&)
    {
        m_canActivateTimeRecorder = true;
        if (GetEntity()->GetState() == AZ::Entity::ES_ACTIVE && !m_legacyTimeDemoRecorder)
        {
            delete m_legacyTimeDemoRecorder;
            m_legacyTimeDemoRecorder = new CTimeDemoRecorder();
        }
    }
}
