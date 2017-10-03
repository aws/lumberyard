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

#include <cmath>

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <Components/GridVisualComponent.h>

namespace GraphCanvas
{
    ////////////////////////
    // GridVisualComponent
    ////////////////////////
    void GridVisualComponent::Reflect(AZ::ReflectContext * context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GridVisualComponent>()
                ->Version(1)
                ;
        }
    }

    GridVisualComponent::GridVisualComponent()
    {
    }

    void GridVisualComponent::Init()
    {
        m_gridVisualUi = AZStd::make_unique <GridGraphicsItem>(*this);
        m_gridVisualUi->Init();
    }

    void GridVisualComponent::Activate()
    {
        GridRequestBus::EventResult(m_majorPitch, GetEntityId(), &GridRequests::GetMajorPitch);
        GridRequestBus::EventResult(m_minorPitch, GetEntityId(), &GridRequests::GetMinorPitch);

        VisualRequestBus::Handler::BusConnect(GetEntityId());
        RootVisualRequestBus::Handler::BusConnect(GetEntityId());
        GridNotificationBus::Handler::BusConnect(GetEntityId());

        m_gridVisualUi->Activate();
    }

    void GridVisualComponent::Deactivate()
    {
        VisualRequestBus::Handler::BusDisconnect();
        RootVisualRequestBus::Handler::BusDisconnect();
        GridNotificationBus::Handler::BusDisconnect();

        m_gridVisualUi->Deactivate();
    }

    QGraphicsItem* GridVisualComponent::AsGraphicsItem()
    {
        return m_gridVisualUi.get();
    }

    bool GridVisualComponent::Contains(const AZ::Vector2 &) const
    {
        return false;
    }

    QGraphicsItem* GridVisualComponent::GetRootGraphicsItem()
    {
        return m_gridVisualUi.get();
    }

    QGraphicsLayoutItem* GridVisualComponent::GetRootGraphicsLayoutItem()
    {
        return nullptr;
    }

    void GridVisualComponent::OnMajorPitchChanged(const AZ::Vector2& pitch)
    {
        if (!pitch.IsClose(m_majorPitch))
        {
            m_majorPitch = pitch;
            if (m_gridVisualUi)
            {
                m_gridVisualUi->update();
            }
        }
    }

    void GridVisualComponent::OnMinorPitchChanged(const AZ::Vector2& pitch)
    {
        if (!pitch.IsClose(m_minorPitch))
        {
            m_minorPitch = pitch;
            if (m_gridVisualUi)
            {
                m_gridVisualUi->update();
            }
        }
    }

    /////////////////////
    // GridGraphicsItem
    /////////////////////
    GridGraphicsItem::GridGraphicsItem(GridVisualComponent& gridVisual)
        : RootVisualNotificationsHelper(gridVisual.GetEntityId())
        , m_gridVisual(gridVisual)
    {
        setFlags(QGraphicsItem::ItemUsesExtendedStyleOption);
        setZValue(-10000);
        setAcceptHoverEvents(false);
        setData(GraphicsItemName, QStringLiteral("DefaultGridVisual/%1").arg(static_cast<AZ::u64>(GetEntityId()), 16, 16, QChar('0')));
    }

    void GridGraphicsItem::Init()
    {
    }

    void GridGraphicsItem::Activate()
    {
        StyleNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void GridGraphicsItem::Deactivate()
    {
        StyleNotificationBus::Handler::BusDisconnect();
    }

    void GridGraphicsItem::OnStyleChanged()
    {
        m_style.SetStyle(GetEntityId());
    }

    QRectF GridGraphicsItem::boundingRect(void) const
    {
        return{ QPointF{ -100000.0, -100000.0 }, QPointF{ 100000.0, 100000.0 } };
    }

    void GridGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
        painter->save();
        painter->setClipRect(option->exposedRect);

        QColor backgroundColor = m_style.GetColor(Styling::Attribute::BackgroundColor);
        painter->fillRect(option->exposedRect, backgroundColor);

        QPen majorPen = m_style.GetPen(Styling::Attribute::GridMajorWidth, Styling::Attribute::GridMajorStyle, Styling::Attribute::GridMajorColor, Styling::Attribute::CapStyle, true);
        QPen minorPen = m_style.GetPen(Styling::Attribute::GridMinorWidth, Styling::Attribute::GridMinorStyle, Styling::Attribute::GridMinorColor, Styling::Attribute::CapStyle, true);

        qreal minimumVisualPitch = 5.;
        GridRequestBus::EventResult(minimumVisualPitch, GetEntityId(), &GridRequests::GetMinimumVisualPitch);

        qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());
        if (m_gridVisual.m_majorPitch.GetX() * lod < minimumVisualPitch || m_gridVisual.m_majorPitch.GetY() * lod < minimumVisualPitch)
        {
            return;
        }

        bool drawMinor = (m_gridVisual.m_minorPitch.GetX() * lod) >= minimumVisualPitch &&
            (m_gridVisual.m_minorPitch.GetY() * lod) >= minimumVisualPitch;
        qreal integral = std::floor(option->exposedRect.left() / m_gridVisual.m_majorPitch.GetX());


        QSizeF size = boundingRect().size() / 2.;

        for (qreal x = (integral - 1.) * m_gridVisual.m_majorPitch.GetX(); x <= option->exposedRect.right(); x += m_gridVisual.m_majorPitch.GetX())
        {
            painter->setPen(majorPen);
            painter->drawLine(QPointF{ x, -size.height() }, QPointF{ x, size.height() });

            if (drawMinor)
            {
                painter->setPen(minorPen);
                for (qreal minorX = x + m_gridVisual.m_minorPitch.GetX(); minorX < x + m_gridVisual.m_majorPitch.GetX(); minorX += m_gridVisual.m_minorPitch.GetX())
                {
                    painter->drawLine(QPointF{ minorX, -size.height() }, QPointF{ minorX, size.height() });
                }
            }
        }

        integral = std::floor(option->exposedRect.top() / m_gridVisual.m_majorPitch.GetY());

        for (qreal y = (integral - 1.) * m_gridVisual.m_majorPitch.GetY(); y <= option->exposedRect.bottom(); y += m_gridVisual.m_majorPitch.GetY())
        {
            painter->setPen(majorPen);
            painter->drawLine(QPointF{ -size.width(), y }, QPointF{ size.width(), y });

            if (drawMinor)
            {
                painter->setPen(minorPen);
                for (qreal minorY = y + m_gridVisual.m_minorPitch.GetY(); minorY < y + m_gridVisual.m_majorPitch.GetY(); minorY += m_gridVisual.m_minorPitch.GetY())
                {
                    painter->drawLine(QPointF{ -size.width(), minorY }, QPointF{ size.width(), minorY });
                }
            }
        }

        painter->restore();
    }
}