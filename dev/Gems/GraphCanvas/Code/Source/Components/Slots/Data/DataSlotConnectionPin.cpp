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

#include <Components/Slots/Data/DataSlotConnectionPin.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Styling/definitions.h>

namespace GraphCanvas
{
    //////////////////////////
    // DataSlotConnectionPin
    //////////////////////////
    
    DataSlotConnectionPin::DataSlotConnectionPin(const AZ::EntityId& slotId)
        : SlotConnectionPin(slotId)
        , m_colorPalette(nullptr)
    {
    }
    
    DataSlotConnectionPin::~DataSlotConnectionPin()
    {    
    }
    
    void DataSlotConnectionPin::OnRefreshStyle()
    {
        m_style.SetStyle(m_slotId, Styling::Elements::DataConnectionPin);

        size_t typeCount = 0;
        DataSlotRequestBus::EventResult(typeCount, GetEntityId(), &DataSlotRequests::GetContainedTypesCount);

        m_containerColorPalettes.clear();

        for (size_t i = 0; i < typeCount; ++i)
        {
            const Styling::StyleHelper* colorPalette = nullptr;
            DataSlotRequestBus::EventResult(colorPalette, GetEntityId(), &DataSlotRequests::GetContainedTypeColorPalette, i);
            if (colorPalette)
            {
                m_containerColorPalettes.emplace_back(colorPalette);
            }
        }
            
        DataSlotRequestBus::EventResult(m_colorPalette, GetEntityId(), &DataSlotRequests::GetDataColorPalette);

        update();
    }
    
    void DataSlotConnectionPin::DrawConnectionPin(QPainter *painter, QRectF drawRect, bool isConnected)
    {
        DataSlotType dataType = DataSlotType::Unknown;
        DataSlotRequestBus::EventResult(dataType, GetEntityId(), &DataSlotRequests::GetDataSlotType);

        qreal radius = (AZ::GetMin(drawRect.width(), drawRect.height()) * 0.5) - m_style.GetBorder().width();

        QPen pen = m_style.GetBorder();

        if (m_colorPalette)
        {
            pen.setColor(m_colorPalette->GetColor(Styling::Attribute::LineColor));
        }

        painter->setPen(pen);

        AZ::EntityId variableId;
        DataSlotRequestBus::EventResult(variableId, GetEntityId(), &DataSlotRequests::GetVariableId);

        // If our variable slots are not connected, we just want to draw the empty
        // circle.
        if (dataType == DataSlotType::Value
            || (dataType == DataSlotType::Reference && !variableId.IsValid()))
        {
            // Add fill color for slots if it is connected
            if (isConnected)
            {
                QBrush brush = painter->brush();

                if (m_colorPalette)
                {
                    brush.setColor(m_colorPalette->GetColor(Styling::Attribute::BackgroundColor));
                }
                else
                {
                    brush.setColor(pen.color());
                }

                painter->setBrush(brush);
            }

            painter->drawEllipse(drawRect.center(), radius, radius);
        }
        else if (dataType == DataSlotType::Container)
        {
            QRectF rect(drawRect.center().x() - radius, drawRect.center().y() - radius, radius * 2, radius * 2);

            QLinearGradient penGradient;
            QLinearGradient fillGradient;

            if (m_containerColorPalettes.size() > 0)
            {
                penGradient = QLinearGradient(rect.topLeft(), rect.bottomRight());
                fillGradient = QLinearGradient(rect.topLeft(), rect.bottomRight());

                penGradient.setColorAt(0, m_containerColorPalettes[0]->GetColor(Styling::Attribute::LineColor));
                fillGradient.setColorAt(0, m_containerColorPalettes[0]->GetColor(Styling::Attribute::BackgroundColor));

                double transition = 0.1 * (1.0 / m_containerColorPalettes.size());

                for (size_t i = 1; i < m_containerColorPalettes.size(); ++i)
                {
                    double transitionStart = AZStd::max(0.0, (double)i / m_containerColorPalettes.size() - (transition * 0.5));
                    double transitionEnd   = AZStd::min(1.0, (double)i / m_containerColorPalettes.size() + (transition * 0.5));

                    penGradient.setColorAt(transitionStart, m_containerColorPalettes[i - 1]->GetColor(Styling::Attribute::LineColor));
                    penGradient.setColorAt(transitionEnd, m_containerColorPalettes[i]->GetColor(Styling::Attribute::LineColor));

                    fillGradient.setColorAt(transitionStart, m_containerColorPalettes[i - 1]->GetColor(Styling::Attribute::BackgroundColor));
                    fillGradient.setColorAt(transitionEnd, m_containerColorPalettes[i]->GetColor(Styling::Attribute::BackgroundColor));
                }

                penGradient.setColorAt(1, m_containerColorPalettes[m_containerColorPalettes.size() - 1]->GetColor(Styling::Attribute::LineColor));
                fillGradient.setColorAt(1, m_containerColorPalettes[m_containerColorPalettes.size() - 1]->GetColor(Styling::Attribute::BackgroundColor));

                pen.setBrush(penGradient);
                painter->setPen(pen);
            }

            // Add fill color for slots if it is connected
            if (isConnected)
            {
                QBrush brush = painter->brush();

                if (m_containerColorPalettes.size() > 0)
                {
                    brush = fillGradient;
                }
                if (m_colorPalette)
                {
                    brush.setColor(m_colorPalette->GetColor(Styling::Attribute::BackgroundColor));
                }
                else
                {
                    brush.setColor(pen.color());
                }

                painter->setBrush(brush);
            }

            painter->drawRect(rect);
        }
        else if (dataType != DataSlotType::Unknown)
        {
            painter->save();

            QRectF filledHalfRect = QRectF(drawRect.x(), drawRect.y(), drawRect.width() * 0.5, drawRect.height());
            QRectF unfilledHalfRect = filledHalfRect;
            unfilledHalfRect.moveLeft(drawRect.center().x());

            ConnectionType connectionType = CT_Invalid;
            SlotRequestBus::EventResult(connectionType, GetEntityId(), &SlotRequests::GetConnectionType);

            if (connectionType == CT_Output)
            {
                QRectF swapRect = filledHalfRect;
                filledHalfRect = unfilledHalfRect;
                unfilledHalfRect = swapRect;
            }
             
            // Draw the unfilled half
            painter->setClipRect(filledHalfRect);
            painter->drawEllipse(drawRect.center(), radius, radius);

            // Add fill color for slots if it is connected
            if (m_colorPalette)
            {
                QBrush brush = painter->brush();
                brush.setColor(m_colorPalette->GetColor(Styling::Attribute::BackgroundColor));
                painter->setBrush(brush);
            }

            painter->setClipRect(unfilledHalfRect);
            painter->drawEllipse(drawRect.center(), radius, radius);

            painter->restore();
        }
    }

}