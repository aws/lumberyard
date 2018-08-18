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

#include <QGraphicsItem>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/GraphicsItems/PulseBus.h>

namespace GraphCanvas
{
    class AnimatedPulseControlPoint
    {
    public:
        AZ_CLASS_ALLOCATOR(AnimatedPulseControlPoint, AZ::SystemAllocator, 0);
        
        AnimatedPulseControlPoint(const QPointF& startPoint, const QPointF& endPoint);
        ~AnimatedPulseControlPoint() = default;
        
        QPointF GetPoint(float percent) const;        
        
    private:
    
        QPointF m_startPoint;
        QPointF m_delta;
    };
    
    class AnimatedPulseConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(AnimatedPulseControlPoint, AZ::SystemAllocator, 0);
        
        AnimatedPulseConfiguration();
        ~AnimatedPulseConfiguration() = default;

        bool                                        m_enableGradient;
        QColor                                      m_drawColor;
        float                                       m_durationSec;        
        qreal                                       m_zValue;
        AZStd::vector< AnimatedPulseControlPoint >  m_controlPoints;
    };
    
    class AnimatedPulse
        : public QGraphicsItem
        , public AZ::TickBus::Handler
        , public PulseRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(AnimatedPulse, AZ::SystemAllocator, 0);

        AnimatedPulse(const AnimatedPulseConfiguration& pulseConfiguration);
        ~AnimatedPulse() = default;

        AZ::EntityId GetId() const;
        
        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;
        ////
        
        // GraphicsItem
        QRectF boundingRect() const override;
        
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
        ////

        // PulseRequestBus
        AnimatedPulse* GetPulse() override;
        ////
        
    private:
    
        QRectF m_boundingRect;
    
        float m_elapsedDuration;        
        AnimatedPulseConfiguration m_configuration;

        AZ::EntityId m_id;
    };
}