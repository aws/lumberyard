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

#include <Components/NodePropertyDisplay/BooleanDataInterface.h>
#include <Components/NodePropertyDisplay/NodePropertyDisplay.h>

class QPushButton;
class QGraphicsProxyWidget;

namespace GraphCanvas
{
    class GraphCanvasLabel;

    class BooleanNodePropertyDisplay
        : public NodePropertyDisplay 
    {    
    public:
        AZ_CLASS_ALLOCATOR(BooleanNodePropertyDisplay, AZ::SystemAllocator, 0);
        BooleanNodePropertyDisplay(BooleanDataInterface* dataInterface);
        virtual ~BooleanNodePropertyDisplay();
    
        // NodePropertyDisplay
        void RefreshStyle() override;
        void UpdateDisplay() override;
        
        QGraphicsLayoutItem* GetDisabledGraphicsLayoutItem() const override;
        QGraphicsLayoutItem* GetDisplayGraphicsLayoutItem() const override;
        QGraphicsLayoutItem* GetEditableGraphicsLayoutItem() const override;
        ////
    
    private:

        void InvertValue();
    
        BooleanDataInterface*   m_dataInterface;
    
        GraphCanvasLabel*       m_disabledLabel;
        GraphCanvasLabel*       m_displayLabel;
        QPushButton*            m_pushButton;
        QGraphicsProxyWidget*   m_proxyWidget;
    };
}