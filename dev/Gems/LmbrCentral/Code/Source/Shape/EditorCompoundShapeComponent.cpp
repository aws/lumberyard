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
#include "LmbrCentral_precompiled.h"
#include "CompoundShapeComponent.h"
#include "EditorCompoundShapeComponent.h"

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    void EditorCompoundShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorCompoundShapeComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("Configuration", &EditorCompoundShapeComponent::m_configuration)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorCompoundShapeComponent>(
                    "Compound Shape", "The Compound Shape component allows two or more shapes to be combined to create more complex shapes")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/ColliderSphere.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Sphere.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-shapes.html")
                    ->DataElement(0, &EditorCompoundShapeComponent::m_configuration, "Configuration", "Compound Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCompoundShapeComponent::ConfigurationChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                        ;
            }
        }
    }

    void EditorCompoundShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto component = gameEntity->CreateComponent<CompoundShapeComponent>();
        if (component)
        {
            component->m_configuration = m_configuration;
        }
    }

	AZ::Aabb EditorCompoundShapeComponent::GetEncompassingAabb()
	{
		AZ::Aabb finalAabb = AZ::Aabb::CreateNull();

		for (AZ::EntityId childEntity : m_configuration.GetChildEntities())
		{
			AZ::Aabb childAabb = AZ::Aabb::CreateNull();
			EBUS_EVENT_ID_RESULT(childAabb, childEntity, ShapeComponentRequestsBus, GetEncompassingAabb);
			if (childAabb.IsValid())
			{
				finalAabb.AddAabb(childAabb);
			}
		}
		return finalAabb;
	}

	bool EditorCompoundShapeComponent::IsPointInside(const AZ::Vector3& point)
	{
		bool result = false;
		for (AZ::EntityId childEntity : m_configuration.GetChildEntities())
		{
			EBUS_EVENT_ID_RESULT(result, childEntity, ShapeComponentRequestsBus, IsPointInside, point);
			if (result)
			{
				break;
			}
		}
		return result;
	}

	float EditorCompoundShapeComponent::DistanceSquaredFromPoint(const AZ::Vector3& point)
	{
		float smallestDistanceSquared = FLT_MAX;
		for (AZ::EntityId childEntity : m_configuration.GetChildEntities())
		{
			float currentDistanceSquared = FLT_MAX;
			EBUS_EVENT_ID_RESULT(currentDistanceSquared, childEntity, ShapeComponentRequestsBus, DistanceSquaredFromPoint, point);
			if (currentDistanceSquared < smallestDistanceSquared)
			{
				smallestDistanceSquared = currentDistanceSquared;
			}
		}
		return smallestDistanceSquared;
	}

	bool EditorCompoundShapeComponent::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, AZ::VectorFloat& distance)
	{
		bool intersection = false;
		for (const AZ::EntityId childEntity : m_configuration.GetChildEntities())
		{
			ShapeComponentRequestsBus::EventResult(intersection, childEntity, &ShapeComponentRequests::IntersectRay, src, dir, distance);
			if (intersection)
			{
				return true;
			}
		}

		return false;
	}

	void EditorCompoundShapeComponent::OnEntityActivated(const AZ::EntityId& id)
	{ 
		m_currentlyActiveChildren++;
		ShapeComponentNotificationsBus::MultiHandler::BusConnect(id);

		if (ShapeComponentRequestsBus::Handler::BusIsConnected() && CompoundShapeComponentRequestsBus::Handler::BusIsConnected())
		{
			EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
		}
	}

	void EditorCompoundShapeComponent::OnEntityDeactivated(const AZ::EntityId& id)
	{
		m_currentlyActiveChildren--;
		ShapeComponentNotificationsBus::MultiHandler::BusDisconnect(id);
		EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
	}

	void EditorCompoundShapeComponent::OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons changeReason)
	{
		if (changeReason == ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged)
		{
			EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
		}
		else if (changeReason == ShapeComponentNotifications::ShapeChangeReasons::TransformChanged)
		{
			// If there are multiple shapes in a compound shape, then moving one of them changes the overall compound shape, otherwise the transform change is bubbled up directly
			EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, (m_currentlyActiveChildren > 1) ? ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged
				: ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
		}
	}

    void EditorCompoundShapeComponent::Activate()
    {
		for (const AZ::EntityId& childEntity : m_configuration.GetChildEntities())
		{
			AZ::EntityBus::MultiHandler::BusConnect(childEntity);
		}

		ShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
        CompoundShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void EditorCompoundShapeComponent::Deactivate()
    {
		if(AZ::EntityBus::MultiHandler::BusIsConnected())
		{
			AZ::EntityBus::MultiHandler::BusDisconnect();
		}
		
		ShapeComponentRequestsBus::Handler::BusDisconnect();
        CompoundShapeComponentRequestsBus::Handler::BusDisconnect();
    }
    
    bool EditorCompoundShapeComponent::HasShapeComponentReferencingEntityId(const AZ::EntityId& entityId)
    {
        const AZStd::list<AZ::EntityId>& childEntities = m_configuration.GetChildEntities();
        auto it = AZStd::find(childEntities.begin(), childEntities.end(), entityId);
        if (it != childEntities.end())
        {
            return true;
        }

        bool result = false;
        for (const AZ::EntityId& childEntityId : childEntities)
        {
            if (childEntityId == GetEntityId())
            {
                // This can happen when we have multiple entities selected and use the picker to set the reference to one of the selected entities
                return true;
            }

            if (childEntityId.IsValid())
            {
                CompoundShapeComponentRequestsBus::EventResult(result, childEntityId, &CompoundShapeComponentRequests::HasShapeComponentReferencingEntityId, entityId);

                if (result)
                {
                    return true;
                }
            }
        }

        return false;
    }

    AZ::u32 EditorCompoundShapeComponent::ConfigurationChanged()
    {
        AZStd::list<AZ::EntityId>& childEntities = const_cast<AZStd::list<AZ::EntityId>&>(m_configuration.GetChildEntities());
        auto selfIt = AZStd::find(childEntities.begin(), childEntities.end(), GetEntityId());
        if (selfIt != childEntities.end())
        {
            QMessageBox(QMessageBox::Warning,
                "Input Error",
                "You cannot set a compound shape component to reference itself!",
                QMessageBox::Ok, QApplication::activeWindow()).exec();

            *selfIt = AZ::EntityId();
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        bool result = false;
        for (auto childIt = childEntities.begin(); childIt != childEntities.end(); ++childIt)
        {
            CompoundShapeComponentRequestsBus::EventResult(result, *childIt, &CompoundShapeComponentRequests::HasShapeComponentReferencingEntityId, GetEntityId());
            if (result)
            {
                QMessageBox(QMessageBox::Warning,
                    "Endless Loop Warning",
                    "Having circular references within a compound shape results in an endless loop!  Clearing the reference.",
                    QMessageBox::Ok, QApplication::activeWindow()).exec();

                *childIt = AZ::EntityId();
                return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
            }
        }

		RefreshChildrenBusConnexions();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

	void EditorCompoundShapeComponent::RefreshChildrenBusConnexions()
	{
		m_currentlyActiveChildren = 0;
		AZ::EntityBus::MultiHandler::BusDisconnect();
		
		for (const AZ::EntityId& childEntity : m_configuration.GetChildEntities())
		{
			AZ::EntityBus::MultiHandler::BusConnect(childEntity);
		}
	}
} // namespace LmbrCentral
