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

#include "EditorBaseShapeComponent.h"
#include <LmbrCentral/Shape/CompoundShapeComponentBus.h>

namespace LmbrCentral
{
    class EditorCompoundShapeComponent
        : public EditorBaseShapeComponent
		, private ShapeComponentRequestsBus::Handler
		, private ShapeComponentNotificationsBus::MultiHandler
		, private CompoundShapeComponentRequestsBus::Handler
		, public AZ::EntityBus::MultiHandler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorCompoundShapeComponent, "{837AA0DF-9C14-4311-8410-E7983E1F4B8D}", EditorBaseShapeComponent);
        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

		// ShapeComponent::Handler implementation
		AZ::Crc32 GetShapeType() override
		{
			return AZ::Crc32("Compound");
		}

		AZ::Aabb GetEncompassingAabb() override;
		bool IsPointInside(const AZ::Vector3& point) override;
		float DistanceSquaredFromPoint(const AZ::Vector3& point) override;
		bool IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance) override;

        // EditorComponentSelectionNotificationsBus::Handler
        AZ::u32 GetBoundingBoxDisplayType() override { return AzToolsFramework::EditorComponentSelectionRequests::NoBoundingBox; }

		// EntityEvents
		void OnEntityActivated(const AZ::EntityId&) override;
		void OnEntityDeactivated(const AZ::EntityId&) override;

		// ShapeComponentNotificationsBus::MultiHandler
		void OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons) override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            EditorBaseShapeComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("CompoundShapeService", 0x4f7c640a));
        }

        AZ::u32 ConfigurationChanged();
		void RefreshChildrenBusConnexions();

    private:
        // CompoundShapeComponentRequestsBus::Handler
        CompoundShapeConfiguration GetCompoundShapeConfiguration() override
        {
            return m_configuration;
        }

        bool HasShapeComponentReferencingEntityId(const AZ::EntityId& entityId) override;

        CompoundShapeConfiguration m_configuration; ///< Stores configuration for this component.
		int m_currentlyActiveChildren = 0; ///< Number of compound shape children shape entities that are currently active.
    };
} // namespace LmbrCentral