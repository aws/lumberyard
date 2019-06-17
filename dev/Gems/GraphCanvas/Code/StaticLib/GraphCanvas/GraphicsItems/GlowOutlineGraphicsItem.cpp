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

#include <QGraphicsBlurEffect>
#include <QPropertyAnimation>

#include <GraphCanvas/GraphicsItems/GlowOutlineGraphicsItem.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Utils/GraphUtils.h>

namespace GraphCanvas
{
    ////////////////////////////
    // GlowOutlineGraphicsItem 
    ////////////////////////////

    GlowOutlineGraphicsItem::GlowOutlineGraphicsItem(const FixedGlowOutlineConfiguration& configuration)    
        : m_pulseTime(0)
        , m_currentTime(0)
        , m_opacityStart(1.0)
        , m_opacityEnd(0.0)
    {
        setPath(configuration.m_painterPath);

        ConfigureGlowOutline(configuration);
    }

    GlowOutlineGraphicsItem::GlowOutlineGraphicsItem(const SceneMemberGlowOutlineConfiguration& configuration)
        : m_trackingSceneMember(configuration.m_sceneMember)
        , m_pulseTime(0)
        , m_currentTime(0)
        , m_opacityStart(1.0)
        , m_opacityEnd(0.0)
    {
        ConfigureGlowOutline(configuration);

        if (configuration.m_sceneMember.IsValid())
        {
            if (GraphUtils::IsConnection(m_trackingSceneMember))
            {
                ConnectionVisualNotificationBus::Handler::BusConnect(m_trackingSceneMember);
            }
            else if (GraphUtils::IsNode(m_trackingSceneMember))
            {
                GeometryNotificationBus::Handler::BusConnect(m_trackingSceneMember);
            }

            GraphCanvas::GraphId graphId;
            GraphCanvas::SceneMemberRequestBus::EventResult(graphId, m_trackingSceneMember, &GraphCanvas::SceneMemberRequests::GetScene);

            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, graphId, &GraphCanvas::SceneRequests::GetViewId);

            GraphCanvas::ViewNotificationBus::Handler::BusConnect(viewId);

            qreal zoomLevel = 0.0f;

            GraphCanvas::ViewRequestBus::EventResult(zoomLevel, viewId, &GraphCanvas::ViewRequests::GetZoomLevel);

            OnZoomChanged(zoomLevel);
        }

        UpdateOutlinePath();
    }

    void GlowOutlineGraphicsItem::OnTick(float delta, AZ::ScriptTimePoint timePoint)
    {
        m_currentTime += delta;

        while (m_currentTime >= m_pulseTime)
        {
            AZStd::swap(m_opacityStart, m_opacityEnd);

            m_currentTime -= m_pulseTime;
        }

        qreal value = AZ::Lerp(m_opacityStart, m_opacityEnd, (m_currentTime / m_pulseTime));
        setOpacity(value);
    }

    void GlowOutlineGraphicsItem::OnConnectionPathUpdated()
    {
        UpdateOutlinePath();
    }

    void GlowOutlineGraphicsItem::OnPositionChanged(const AZ::EntityId& /*targetEntity*/, const AZ::Vector2& /*position*/)
    {
        UpdateOutlinePath();
    }

    void GlowOutlineGraphicsItem::OnBoundsChanged()
    {
        UpdateOutlinePath();
    }

    void GlowOutlineGraphicsItem::OnZoomChanged(qreal zoomLevel)
    {
        zoomLevel = AZ::GetClamp(zoomLevel, GraphCanvas::ViewLimits::ZOOM_MIN, GraphCanvas::ViewLimits::ZOOM_MAX);

        // We want to invert our range to make this easier to work with.
        // 0.0f == ZOOM_MAX which is the closest zoom, and we don't want to scale anything.
        // 1.0f == ZOOM_MIN which is the furthest out zoom, and we need to scale up
        float zoomPercent = (GraphCanvas::ViewLimits::ZOOM_RANGE - (zoomLevel - GraphCanvas::ViewLimits::ZOOM_MIN)) / GraphCanvas::ViewLimits::ZOOM_RANGE;

        // We never want to scale down. So always set ourselves to 1 if we would be otherwise
        float scaleFactor = AZ::GetMax(1.0f, 5.0f * zoomPercent);

        QPen currentPen = pen();

        currentPen.setWidth(m_defaultPenWidth * scaleFactor);

        setPen(currentPen);
    }

    void GlowOutlineGraphicsItem::UpdateOutlinePath()
    {
        QPainterPath outlinePath;
        SceneMemberUIRequestBus::EventResult(outlinePath, m_trackingSceneMember, &SceneMemberUIRequests::GetOutline);

        setPath(outlinePath);
    }

    void GlowOutlineGraphicsItem::ConfigureGlowOutline(const GlowOutlineConfiguration& outlineConfiguration)
    {
        setPen(outlineConfiguration.m_pen);

        QGraphicsBlurEffect* blurEffect = new QGraphicsBlurEffect();

        blurEffect->setBlurRadius(outlineConfiguration.m_blurRadius);

        setGraphicsEffect(blurEffect);

        setZValue(outlineConfiguration.m_zValue);

        m_opacityStart = outlineConfiguration.m_maxAlpha;
        m_opacityEnd = outlineConfiguration.m_minAlpha;  

        m_defaultPenWidth = outlineConfiguration.m_pen.width();

        // Assuming 0 will just be a solid visualization
        if (outlineConfiguration.m_pulseRate.count() > 0)
        {
            m_pulseTime = (outlineConfiguration.m_pulseRate.count() * 0.5) * 0.001;

            if (m_pulseTime > 0)
            {
                AZ::TickBus::Handler::BusConnect();
            }
        }
    }
}