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

#include <QGraphicsWidget>

#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <Styling/StyleHelper.h>
#include <Ui/RootVisualNotificationsHelper.h>

namespace GraphCanvas
{
    //! Base class to handle a bunch of the weird quirky stuff that the NodeFrames need to manage.
    //! Will not paint anything.
    class NodeFrameGraphicsWidget
        : public RootVisualNotificationsHelper<QGraphicsWidget>
        , public RootVisualRequestBus::Handler
        , public GeometryNotificationBus::Handler
        , public VisualRequestBus::Handler
        , public NodeNotificationBus::Handler
        , public NodeUIRequestBus::Handler
        , public StyleNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(NodeFrameGraphicsWidget, "{33B9DFFB-9E40-4D55-82A7-85468F7E7790}");
        AZ_CLASS_ALLOCATOR(NodeFrameGraphicsWidget, AZ::SystemAllocator, 0);

        // Do not allow Serialization of Graphics Ui classes
        static void Reflect(AZ::ReflectContext*) = delete;

        NodeFrameGraphicsWidget(const AZ::EntityId& nodeVisual);
        ~NodeFrameGraphicsWidget() override = default;

        void Activate();
        void Deactivate();

        // RootVisualRequestBus
        QGraphicsItem* GetRootGraphicsItem() override;
        QGraphicsLayoutItem* GetRootGraphicsLayoutItem() override;
        ////

        // GeometryNotificationBus
        using NodeNotifications::OnPositionChanged;
        void OnPositionChanged(const AZ::EntityId& entityId, const AZ::Vector2& position) override;
        ////

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

        // QGraphicsWidget
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent) override;
        ////

        // QGraphicsItem
        void contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent) override;
        QSizeF sizeHint(Qt::SizeHint which, const QSizeF& constraint = QSizeF()) const override;
        ////

        // VisualRequestBus
        QGraphicsItem* AsGraphicsItem() override;

        bool Contains(const AZ::Vector2& position) const override;
        ////

        // NodeNotificationBus
        void OnNodeActivated() override;

        void OnNodeWrapped(const AZ::EntityId& wrappingNode) override;
        ////

        // NodeUIRequestBus
        void SetSelected(bool selected) override;
        bool IsSelected() const;

        void StartItemMove(const QPointF& initialPosition, const QPointF& currentPosition) override;

        void SetSnapToGrid(bool snapToGrid) override;
        void SetGrid(AZ::EntityId gridId) override;

        qreal GetCornerRadius() const override;

        bool IsWrapped() const override;
        ////
		
	protected:

        virtual void OnActivated();
        virtual void OnDeactivated();
        virtual void OnRefreshStyle();

    protected:
        NodeFrameGraphicsWidget(const NodeFrameGraphicsWidget&) = delete;

        Styling::StyleHelper m_style;

        bool m_isWrapped;
    };
}
