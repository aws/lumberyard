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
#include <GraphCanvas/Components/NodePropertyDisplay/ComboBoxDataInterface.h>
#include <GraphCanvas/Components/MimeDataHandlerBus.h>
#include <Widgets/GraphCanvasComboBox.h>

namespace GraphCanvas
{
    class GraphCanvasLabel;
    class ComboBoxNodePropertyDisplay;

    class ComboBoxGraphicsEventFilter
        : public QGraphicsItem
    {
    public:
        AZ_CLASS_ALLOCATOR(ComboBoxGraphicsEventFilter, AZ::SystemAllocator, 0);
        ComboBoxGraphicsEventFilter(ComboBoxNodePropertyDisplay* propertyDisplay);

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
        ComboBoxNodePropertyDisplay* m_owner;
    };

    class ComboBoxNodePropertyDisplay
        : public NodePropertyDisplay
        , public GeometryNotificationBus::Handler
        , public ViewNotificationBus::Handler
    {
        enum class DragState
        {
            Idle,
            Valid,
            Invalid
        };

        friend class ComboBoxGraphicsEventFilter;

    public:
        AZ_CLASS_ALLOCATOR(ComboBoxNodePropertyDisplay, AZ::SystemAllocator, 0);
        ComboBoxNodePropertyDisplay(ComboBoxDataInterface* dataInterface);
        virtual ~ComboBoxNodePropertyDisplay();

        // NodePropertyDisplay
        void RefreshStyle() override;
        void UpdateDisplay() override;

        QGraphicsLayoutItem* GetDisabledGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetDisplayGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetEditableGraphicsLayoutItem() override;
        ////

        void dragEnterEvent(QGraphicsSceneDragDropEvent* dragDropEvent);
        void dragLeaveEvent(QGraphicsSceneDragDropEvent* dragDropEvent);
        void dropEvent(QGraphicsSceneDragDropEvent* dragDropEvent);

        // GeometryNotificationBus
        void OnPositionChanged(const AZ::EntityId& targetEntity, const AZ::Vector2& position) override;
        ////

        // ViewNotificationBus
        void OnZoomChanged(qreal zoomLevel) override;
        ////

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

        void OnMenuAboutToDisplay();
        void UpdateMenuDisplay(const ViewId& viewId, bool forceUpdate = false);

        bool m_menuDisplayDirty;

        ComboBoxDataInterface*          m_dataInterface;
        ComboBoxGraphicsEventFilter*    m_eventFilter;

        GraphCanvasLabel*               m_disabledLabel;
        GraphCanvasComboBox*            m_comboBox;
        QGraphicsProxyWidget*           m_proxyWidget;
        GraphCanvasLabel*               m_displayLabel;

        DragState                       m_dragState;
    };
}