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
#include <precompiled.h>

#include <QGraphicsSceneMouseEvent>

#include <AzCore/Serialization/EditContext.h>

#include <Components/GeometryComponent.h>

#include <Components/Nodes/NodeComponent.h>
#include <GraphCanvas/tools.h>

namespace GraphCanvas
{
    //////////////////////
    // GeometryComponent
    //////////////////////
    const float GeometryComponent::IS_CLOSE_TOLERANCE = 0.001;

    void GeometryComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<GeometryComponent>()
            ->Version(3)
            ->Field("Position", &GeometryComponent::m_position)
        ;
    }

    GeometryComponent::GeometryComponent()
        : m_position(AZ::Vector2::CreateZero())
    {
    }

    GeometryComponent::~GeometryComponent()
    {
        GeometryRequestBus::Handler::BusDisconnect();
        GeometryCommandBus::Handler::BusDisconnect();
    }

    void GeometryComponent::Init()
    {
        GeometryRequestBus::Handler::BusConnect(GetEntityId());
        GeometryCommandBus::Handler::BusConnect(GetEntityId());
    }

    void GeometryComponent::Activate()
    {
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void GeometryComponent::Deactivate()
    {
        VisualNotificationBus::Handler::BusDisconnect();
        SceneMemberNotificationBus::Handler::BusDisconnect();
    }

    void GeometryComponent::OnSceneSet(const AZ::EntityId& scene)
    {
        VisualNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void GeometryComponent::SetPosition(const AZ::Vector2& position)
    {
        if (!position.IsClose(m_position))
        {
            m_position = position;
            GeometryNotificationBus::Event(GetEntityId(), &GeometryNotifications::OnPositionChanged, GetEntityId(), m_position);
        }
    }

    AZ::Vector2 GeometryComponent::GetPosition() const
    {
        return m_position;
    }

    void GeometryComponent::OnItemChange(const AZ::EntityId& entityId, QGraphicsItem::GraphicsItemChange change, const QVariant& value)
    {
        AZ_Assert(entityId == GetEntityId(), "EIDs should match");

        switch (change)
        {
        case QGraphicsItem::ItemPositionChange:
        {
            QPointF qt = value.toPointF();
            SetPosition(::AZ::Vector2(qt.x(), qt.y()));
            break;
        }
        default:
            break;
        }
    }

    bool GeometryComponent::OnMousePress(const AZ::EntityId&, const QGraphicsSceneMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton)
        {
            m_oldPosition = m_position;
        }
        return false;
    }

    /*!
    The mouse release event determines if the item has moved a minimum distance to record as an Undo Point
    for a node move event.
    */
    bool GeometryComponent::OnMouseRelease(const AZ::EntityId&, const QGraphicsSceneMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton)
        {
            if (!m_oldPosition.IsClose(m_position))
            {
                AZ::EntityId sceneId;
                SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);
                if (sceneId.IsValid())
                {
                    SceneNotificationBus::Event(sceneId, &SceneNotifications::OnItemMouseMoveComplete, GetEntityId(), event);
                }
            }
        }

        return false;
    }
}