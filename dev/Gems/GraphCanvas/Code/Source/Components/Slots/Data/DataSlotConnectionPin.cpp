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

        if (m_colorPalette == nullptr)
        {
            AZ::Uuid dataType = AZ::Uuid();
            DataSlotRequestBus::EventResult(dataType, GetEntityId(), &DataSlotRequests::GetDataTypeId);

            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

            EditorId editorId;
            SceneRequestBus::EventResult(editorId, sceneId, &SceneRequests::GetEditorId);

            StyleManagerRequestBus::EventResult(m_colorPalette, editorId, &StyleManagerRequests::FindDataColorPalette, dataType);
        }

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
            || !variableId.IsValid())
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