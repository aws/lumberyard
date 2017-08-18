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
#include "EditorEntitySortComponent.h"
#include <AzCore/Serialization/EditContext.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorEntitySortComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorEntitySortComponent, EditorComponentBase>()
                    ->Field("ChildEntityOrderArray", &EditorEntitySortComponent::m_childEntityOrderArray);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorEntitySortComponent>("Child Entity Sort Order", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                            ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                            ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::HideOnAdd | AZ::Edit::SliceFlags::PushWhenHidden)
                        ;
                }
            }
        }

        void EditorEntitySortComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorChildEntitySortService", 0x916caa82));
        }

        void EditorEntitySortComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorChildEntitySortService", 0x916caa82));
        }

        EditorEntitySortComponent::~EditorEntitySortComponent()
        {
            // We disconnect from the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorEntitySortRequestBus::Handler::BusDisconnect();
        }

        EntityOrderArray EditorEntitySortComponent::GetChildEntityOrderArray()
        {
            return m_childEntityOrderArray;
        }

        void EditorEntitySortComponent::SetChildEntityOrderArray(const EntityOrderArray& entityOrderArray)
        {
            m_childEntityOrderArray = entityOrderArray;
        }

        void EditorEntitySortComponent::Init()
        {
            // We connect to the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorEntitySortRequestBus::Handler::BusConnect(GetEntityId());
        }
    }
} // namespace AzToolsFramework
