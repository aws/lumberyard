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
#include "EditorInspectorComponent.h"
#include <AzCore/Serialization/EditContext.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorInspectorComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorInspectorComponent, EditorComponentBase>()
                    ->Field("ComponentOrderArray", &EditorInspectorComponent::m_componentOrderArray);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorInspectorComponent>("Inspector Component Order", "Edit-time entity inspector state")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                            ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::HideOnAdd | AZ::Edit::SliceFlags::PushWhenHidden)
                            ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                        ;
                }
            }
        }

        void EditorInspectorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorInspectorService", 0xc7357f25));
        }

        void EditorInspectorComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorInspectorService", 0xc7357f25));
        }

        EditorInspectorComponent::~EditorInspectorComponent()
        {
            // We disconnect from the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorInspectorComponentRequestBus::Handler::BusDisconnect();
        }

        void EditorInspectorComponent::Init()
        {
            // We connect to the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorInspectorComponentRequestBus::Handler::BusConnect(GetEntityId());
        }

        ComponentOrderArray EditorInspectorComponent::GetComponentOrderArray()
        {
            return m_componentOrderArray;
        }

        void EditorInspectorComponent::SetComponentOrderArray(const ComponentOrderArray& componentOrderArray)
        {
            m_componentOrderArray = componentOrderArray;
            EditorInspectorComponentNotificationBus::Event(GetEntityId(), &EditorInspectorComponentNotificationBus::Events::OnComponentOrderChanged);
        }

    } // namespace Components
} // namespace AzToolsFramework
