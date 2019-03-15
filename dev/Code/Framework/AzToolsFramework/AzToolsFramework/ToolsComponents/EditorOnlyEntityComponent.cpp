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
#include "stdafx.h"
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorOnlyEntityComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorOnlyEntityComponent, EditorComponentBase>()
                    ->Field("IsEditorOnly", &EditorOnlyEntityComponent::m_isEditorOnly)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorOnlyEntityComponent>("Editor-Only Flag Handler", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                            ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorOnlyEntityComponent::m_isEditorOnly, 
                            "Editor Only", 
                            "Marks the entity for editor-use only. If true, the entity will not be exported for use in runtime contexts (including dynamic slices).")
                        ;
                }
            }
        }

        void EditorOnlyEntityComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorOnlyEntityService", 0x7010c39d));
        }

        void EditorOnlyEntityComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorOnlyEntityService", 0x7010c39d));
        }

        EditorOnlyEntityComponent::EditorOnlyEntityComponent()
            : m_isEditorOnly(false)
        {

        }

        EditorOnlyEntityComponent::~EditorOnlyEntityComponent()
        {

        }

        void EditorOnlyEntityComponent::Init()
        {
            EditorComponentBase::Init();

            // Connect at Init()-time to allow slice compilation to query for editor only status.
            EditorOnlyEntityComponentRequestBus::Handler::BusConnect(GetEntityId());
        }

        void EditorOnlyEntityComponent::Activate()
        {
            EditorComponentBase::Activate();

            EditorOnlyEntityComponentRequestBus::Handler::BusConnect(GetEntityId());
        }

        void EditorOnlyEntityComponent::Deactivate()
        {
            EditorOnlyEntityComponentRequestBus::Handler::BusDisconnect();

            EditorComponentBase::Deactivate();
        }

        bool EditorOnlyEntityComponent::IsEditorOnlyEntity()
        {
            return m_isEditorOnly;
        }

        void EditorOnlyEntityComponent::SetIsEditorOnlyEntity(bool isEditorOnly)
        {
            if (isEditorOnly != m_isEditorOnly)
            {
                m_isEditorOnly = isEditorOnly;
                EditorOnlyEntityComponentNotificationBus::Broadcast(&EditorOnlyEntityComponentNotificationBus::Events::OnEditorOnlyChanged, GetEntityId(), m_isEditorOnly);
                SetDirty();
            }
        }

    } // namespace Components

} // namespace AzToolsFramework
