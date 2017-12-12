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
#include "precompiled.h"

#include <QPainter>

#include <Components/Nodes/General/GeneralNodeFrameComponent.h>

#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/tools.h>
#include <Styling/StyleHelper.h>

namespace GraphCanvas
{
    //////////////////////////////
    // GeneralNodeFrameComponent
    //////////////////////////////

    void GeneralNodeFrameComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GeneralNodeFrameComponent>()
                ->Version(1)
                ;
        }
    }

    GeneralNodeFrameComponent::GeneralNodeFrameComponent()
        : m_frameWidget(nullptr)
    {

    }

    void GeneralNodeFrameComponent::Init()
    {
        m_frameWidget = aznew GeneralNodeFrameGraphicsWidget(GetEntityId());
    }

    void GeneralNodeFrameComponent::Activate()
    {
        NodeNotificationBus::Handler::BusConnect(GetEntityId());

        m_frameWidget->Activate();
    }

    void GeneralNodeFrameComponent::Deactivate()
    {
        m_frameWidget->Deactivate();

        NodeNotificationBus::Handler::BusDisconnect();
    }

    void GeneralNodeFrameComponent::OnNodeActivated()
    {
        QGraphicsLayout* layout = nullptr;
        NodeLayoutRequestBus::EventResult(layout, GetEntityId(), &NodeLayoutRequests::GetLayout);
        m_frameWidget->setLayout(layout);
    }

    ///////////////////////////////////
    // GeneralNodeFrameGraphicsWidget
    ///////////////////////////////////

    GeneralNodeFrameGraphicsWidget::GeneralNodeFrameGraphicsWidget(const AZ::EntityId& entityKey)
        : NodeFrameGraphicsWidget(entityKey)
    {

        setZValue(m_style.GetAttribute(Styling::Attribute::ZValue, 0));
    }

    void GeneralNodeFrameGraphicsWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
        QPen border = m_style.GetBorder();
        QBrush background = m_style.GetBrush(Styling::Attribute::BackgroundColor);

        if (border.style() != Qt::NoPen || background.color().alpha() > 0)
        {
            qreal cornerRadius = GetCornerRadius();

            border.setJoinStyle(Qt::PenJoinStyle::MiterJoin); // sharp corners
            painter->setPen(border);
            painter->setBrush(background);

            qreal halfBorder = border.widthF() / 2.;
            QRectF adjusted = boundingRect().marginsRemoved(QMarginsF(halfBorder, halfBorder, halfBorder, halfBorder));

            if (cornerRadius >= 1.0)
            {
                painter->drawRoundedRect(adjusted, cornerRadius, cornerRadius);
            }
            else
            {
                painter->drawRect(adjusted);
            }
        }

        QGraphicsWidget::paint(painter, option, widget);
    }
}