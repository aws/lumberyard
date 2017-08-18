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
#include "EditorEntityModelComponent.h"
#include "EditorEntityModel.h"
#include "EditorEntityContextBus.h"

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorEntityModelComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorEntityModelComponent>()
                    ; // Empty class
            }
        }

        void EditorEntityModelComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorEntityModelService", 0x9d215543));
        }

        void EditorEntityModelComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorEntityModelService", 0x9d215543));
        }

        EditorEntityModelComponent::EditorEntityModelComponent()
        {
        }

        EditorEntityModelComponent::~EditorEntityModelComponent()
        {
        }

        void EditorEntityModelComponent::Activate()
        {
            m_entityModel = AZStd::make_unique<EditorEntityModel>();
            ToolsApplicationEvents::Bus::Handler::BusConnect();
        }

        void EditorEntityModelComponent::Deactivate()
        {
            ToolsApplicationEvents::Bus::Handler::BusDisconnect();
            m_entityModel.reset();
        }

        void EditorEntityModelComponent::EntityParentChanged(AZ::EntityId entityId, AZ::EntityId newParentId, AZ::EntityId oldParentId)
        {
            m_entityModel->ChangeEntityParent(entityId, newParentId, oldParentId);
        }

        void EditorEntityModelComponent::EntityDeregistered(AZ::EntityId entityId)
        {
            bool isTrackingEntity = m_entityModel->IsTrackingEntity(entityId);

            bool isEditorEntity = false;
            EditorEntityContextRequestBus::BroadcastResult(isEditorEntity, &EditorEntityContextRequests::IsEditorEntity, entityId);

            // Remove this assert if it is determined that the system is unable to identify context by the time this event is broadcast
            AZ_Assert(isTrackingEntity == isEditorEntity, "We are tracking an entity that ended up not being an editor entity when it is deregistered");
            if (!isTrackingEntity)
            {
                return;
            }

            m_entityModel->RemoveEntity(entityId);
        }

        void EditorEntityModelComponent::EntityRegistered(AZ::EntityId entityId)
        {
            bool isEditorEntity = false;
            EditorEntityContextRequestBus::BroadcastResult(isEditorEntity, &EditorEntityContextRequests::IsEditorEntity, entityId);
            if (!isEditorEntity)
            {
                return;
            }

            m_entityModel->AddEntity(entityId);
        }
    }
} // namespace AzToolsFramework