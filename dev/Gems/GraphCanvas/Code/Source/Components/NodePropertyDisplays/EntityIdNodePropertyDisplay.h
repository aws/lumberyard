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

class QEvent;

#include <AzCore/Component/EntityBus.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyEntityIdCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/EntityIdQLabel.hxx>

#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplay/EntityIdDataInterface.h>
#include <GraphCanvas/Components/MimeDataHandlerBus.h>

namespace GraphCanvas
{
    class GraphCanvasLabel;
    class EntityIdNodePropertyDisplay;

    class EntityIdGraphicsEventFilter
        : public QGraphicsItem
    {
    public:
        AZ_CLASS_ALLOCATOR(EntityIdGraphicsEventFilter, AZ::SystemAllocator, 0);
        EntityIdGraphicsEventFilter(EntityIdNodePropertyDisplay* propertyDisplay);

        bool sceneEventFilter(QGraphicsItem* watched, QEvent* event);

        // QGraphicsItem overrides
        QRectF boundingRect() const override
        {
            return QRectF();
        }

        void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override
        {
        }

    private:
        EntityIdNodePropertyDisplay* m_owner;
    };
    
    class EntityIdNodePropertyDisplay
        : public NodePropertyDisplay
        , public AZ::EntityBus::Handler
    {
        enum class DragState
        {
            Idle,
            Valid,
            Invalid
        };

        friend class EntityIdGraphicsEventFilter;

    public:
        AZ_CLASS_ALLOCATOR(EntityIdNodePropertyDisplay, AZ::SystemAllocator, 0);
        EntityIdNodePropertyDisplay(EntityIdDataInterface* dataInterface);
        virtual ~EntityIdNodePropertyDisplay();
    
        // NodePropertyDisplay
        void RefreshStyle() override;
        void UpdateDisplay() override;
        
        QGraphicsLayoutItem* GetDisabledGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetDisplayGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetEditableGraphicsLayoutItem() override;
        ////

        // AZ::EntityBus
        void OnEntityNameChanged(const AZStd::string& name) override;
        ////

        void dragEnterEvent(QGraphicsSceneDragDropEvent* dragDropEvent);
        void dragLeaveEvent(QGraphicsSceneDragDropEvent* dragDropEvent);
        void dropEvent(QGraphicsSceneDragDropEvent* dragDropEvent);

    protected:

        void ShowContextMenu(const QPoint&);

        void OnIdSet() override;
    
    private:

        void ResetDragState();

        void EditStart();
        void EditFinished();
        void SubmitValue();
        void SetupProxyWidget();
        void CleanupProxyWidget();
    
        EntityIdDataInterface*  m_dataInterface;
        EntityIdGraphicsEventFilter* m_eventFilter;
    
        GraphCanvasLabel*                           m_disabledLabel;
        AzToolsFramework::PropertyEntityIdCtrl*     m_propertyEntityIdCtrl;
        AzToolsFramework::EntityIdQLabel            m_entityIdLabel;
        QGraphicsProxyWidget*                       m_proxyWidget;
        GraphCanvasLabel*                           m_displayLabel;

        DragState                                   m_dragState;
    };
}