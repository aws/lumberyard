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

#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>
#include <QMenu>
#include <QMimeData>

#include <Components/NodePropertyDisplays/ComboBoxNodePropertyDisplay.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Utils/ConversionUtils.h>
#include <GraphCanvas/Widgets/NodePropertyBus.h>
#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    ////////////////////////////////
    // ComboBoxGraphicsEventFilter
    ////////////////////////////////
    ComboBoxGraphicsEventFilter::ComboBoxGraphicsEventFilter(ComboBoxNodePropertyDisplay* propertyDisplay)
        : QGraphicsItem(nullptr)
        , m_owner(propertyDisplay)
    {
    }

    bool ComboBoxGraphicsEventFilter::sceneEventFilter(QGraphicsItem*, QEvent* event)
    {
        switch (event->type())
        {
        case QEvent::GraphicsSceneDragEnter:
            m_owner->dragEnterEvent(static_cast<QGraphicsSceneDragDropEvent*>(event));
            break;
        case QEvent::GraphicsSceneDragLeave:
            m_owner->dragLeaveEvent(static_cast<QGraphicsSceneDragDropEvent*>(event));
            break;
        case QEvent::GraphicsSceneDrop:
            m_owner->dropEvent(static_cast<QGraphicsSceneDragDropEvent*>(event));
            break;
        default:
            break;
        }

        return event->isAccepted();
    }

    ////////////////////////////////
    // ComboBoxNodePropertyDisplay
    ////////////////////////////////
    ComboBoxNodePropertyDisplay::ComboBoxNodePropertyDisplay(ComboBoxDataInterface* dataInterface)
        : m_dataInterface(dataInterface)
        , m_comboBox(nullptr)
        , m_proxyWidget(nullptr)
        , m_dragState(DragState::Idle)
    {
        m_dataInterface->RegisterDisplay(this);

        m_disabledLabel = aznew GraphCanvasLabel();
        m_displayLabel = aznew GraphCanvasLabel();
    }

    void ComboBoxNodePropertyDisplay::ShowContextMenu(const QPoint& pos)
    {
        if (m_comboBox)
        {
            m_dataInterface->OnShowContextMenu(m_comboBox, pos);
        }
        else
        {
            AZ_Error("GraphCanvas", false, "m_propertyEntityIdCtrl doesn't exist!");
        }
    }

    ComboBoxNodePropertyDisplay::~ComboBoxNodePropertyDisplay()
    {
        CleanupProxyWidget();
        delete m_disabledLabel;
        delete m_displayLabel;

        delete m_dataInterface;
    }

    void ComboBoxNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("entityId").c_str());

        AZStd::string styleName = NodePropertyDisplay::CreateDisplayLabelStyle("entityId");
        m_displayLabel->SetSceneStyle(GetSceneId(), styleName.c_str());

        QSizeF minimumSize = m_displayLabel->minimumSize();
        QSizeF maximumSize = m_displayLabel->maximumSize();

        if (m_comboBox)
        {
            m_comboBox->setMinimumSize(minimumSize.width(), minimumSize.height());
            m_comboBox->setMaximumSize(maximumSize.width(), maximumSize.height());
        }
    }

    void ComboBoxNodePropertyDisplay::UpdateDisplay()
    {
        const AZStd::string& displayValue = m_dataInterface->GetDisplayString();

        if (m_comboBox)
        {
            QModelIndex selectedIndex = m_dataInterface->GetAssignedIndex();
            m_comboBox->SetSelectedIndex(selectedIndex);
        }

        QString displayLabel = displayValue.c_str();
        if (displayLabel.isEmpty())
        {
            displayLabel = "<None>";
        }

        m_displayLabel->SetLabel(displayLabel.toUtf8().data());

        if (m_proxyWidget)
        {
            m_proxyWidget->update();
        }
    }

    QGraphicsLayoutItem* ComboBoxNodePropertyDisplay::GetDisabledGraphicsLayoutItem()
    {
        CleanupProxyWidget();
        return m_disabledLabel;
    }

    QGraphicsLayoutItem* ComboBoxNodePropertyDisplay::GetDisplayGraphicsLayoutItem()
    {
        CleanupProxyWidget();
        return m_displayLabel;
    }

    QGraphicsLayoutItem* ComboBoxNodePropertyDisplay::GetEditableGraphicsLayoutItem()
    {
        SetupProxyWidget();
        return m_proxyWidget;
    }

    void ComboBoxNodePropertyDisplay::dragEnterEvent(QGraphicsSceneDragDropEvent* dragDropEvent)
    {
        bool isConnected = false;
        SlotRequestBus::EventResult(isConnected, GetId(), &SlotRequests::HasConnections);

        m_dragState = DragState::Invalid;

        if (!isConnected)
        {
            const QMimeData* dropMimeData = dragDropEvent->mimeData();
            if (m_dataInterface->ShouldAcceptMimeData(dropMimeData))
            {
                dragDropEvent->accept();
                dragDropEvent->acceptProposedAction();

                m_dragState = DragState::Valid;
            }
        }

        Styling::StyleHelper& styleHelper = m_displayLabel->GetStyleHelper();
        switch (m_dragState)
        {
        case DragState::Valid:
        {
            styleHelper.AddSelector(Styling::States::ValidDrop);
            break;
        }
        case DragState::Invalid:
        {
            styleHelper.AddSelector(Styling::States::InvalidDrop);
            break;
        }
        default:
            break;
        }

        m_displayLabel->update();
    }

    void ComboBoxNodePropertyDisplay::dragLeaveEvent(QGraphicsSceneDragDropEvent* dragDropEvent)
    {
        dragDropEvent->accept();
        ResetDragState();
    }

    void ComboBoxNodePropertyDisplay::dropEvent(QGraphicsSceneDragDropEvent* dropEvent)
    {
        if (m_dragState == DragState::Valid)
        {
            const QMimeData* dropMimeData = dropEvent->mimeData();

            if (m_dataInterface->HandleMimeData(dropMimeData))
            {
                dropEvent->accept();
                UpdateDisplay();
            }
        }

        ResetDragState();
    }

    void ComboBoxNodePropertyDisplay::OnPositionChanged(const AZ::EntityId& targetEntity, const AZ::Vector2& position)
    {
        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, GetNodeId(), &SceneMemberRequests::GetScene);

        ViewId viewId;
        SceneRequestBus::EventResult(viewId, graphId, &SceneRequests::GetViewId);

        UpdateMenuDisplay(viewId);
    }

    void ComboBoxNodePropertyDisplay::OnZoomChanged(qreal zoomLevel)
    {
        const ViewId* viewId = ViewNotificationBus::GetCurrentBusId();

        if (viewId)
        {
            UpdateMenuDisplay((*viewId));
        }
    }

    void ComboBoxNodePropertyDisplay::OnIdSet()
    {
        QGraphicsItem* ownerItem = nullptr;
        VisualRequestBus::EventResult(ownerItem, GetId(), &VisualRequests::AsGraphicsItem);

        if (ownerItem)
        {
            ownerItem->setAcceptDrops(true);

            QGraphicsScene* scene = ownerItem->scene();

            if (scene)
            {
                if (m_dataInterface->EnableDropHandling())
                {
                    ComboBoxGraphicsEventFilter* filter = aznew ComboBoxGraphicsEventFilter(this);
                    scene->addItem(filter);
                    ownerItem->installSceneEventFilter(filter);
                }
            }
        }
    }

    void ComboBoxNodePropertyDisplay::ResetDragState()
    {
        Styling::StyleHelper& styleHelper = m_displayLabel->GetStyleHelper();
        switch (m_dragState)
        {
        case DragState::Valid:
            styleHelper.RemoveSelector(Styling::States::ValidDrop);
            break;
        case DragState::Invalid:
            styleHelper.RemoveSelector(Styling::States::InvalidDrop);
            break;
        default:
            break;
        }

        m_displayLabel->update();

        m_dragState = DragState::Idle;
    }

    void ComboBoxNodePropertyDisplay::EditStart()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::LockEditState, this);
        TryAndSelectNode();
    }

    void ComboBoxNodePropertyDisplay::SubmitValue()
    {
        if (m_comboBox)
        {
            QModelIndex index = m_comboBox->GetSelectedIndex();
            m_dataInterface->AssignIndex(index);
        }
        else
        {
            AZ_Error("GraphCanvas", false, "m_comboBox doesn't exist!");
        }

        UpdateDisplay();
    }

    void ComboBoxNodePropertyDisplay::EditFinished()
    {
        SubmitValue();
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);
    }

    void ComboBoxNodePropertyDisplay::SetupProxyWidget()
    {
        if (!m_comboBox)
        {
            m_proxyWidget = new QGraphicsProxyWidget();

            m_proxyWidget->setFlag(QGraphicsItem::ItemIsFocusable, true);
            m_proxyWidget->setFocusPolicy(Qt::StrongFocus);

            m_comboBox = aznew GraphCanvasComboBox(m_dataInterface->GetItemInterface());
            m_comboBox->setContextMenuPolicy(Qt::CustomContextMenu);

            QObject::connect(m_comboBox, &AzToolsFramework::PropertyEntityIdCtrl::customContextMenuRequested, [this](const QPoint& pos) { this->ShowContextMenu(pos); });

            QObject::connect(m_comboBox, &GraphCanvasComboBox::SelectedIndexChanged, [this](const QModelIndex& index) { this->SubmitValue(); });

            QObject::connect(m_comboBox, &GraphCanvasComboBox::OnFocusIn, [this]() { this->EditStart();  });
            QObject::connect(m_comboBox, &GraphCanvasComboBox::OnFocusOut, [this]() { this->EditFinished();  });
            QObject::connect(m_comboBox, &GraphCanvasComboBox::OnMenuAboutToDisplay, [this]() { this->OnMenuAboutToDisplay(); });

            m_proxyWidget->setWidget(m_comboBox);

            RegisterShortcutDispatcher(m_comboBox);
            UpdateDisplay();
            RefreshStyle();

            GraphId graphId;
            SceneMemberRequestBus::EventResult(graphId, GetNodeId(), &SceneMemberRequests::GetScene);

            ViewId viewId;
            SceneRequestBus::EventResult(viewId, graphId, &SceneRequests::GetViewId);

            m_comboBox->RegisterViewId(viewId);

            m_menuDisplayDirty = true;

            ViewNotificationBus::Handler::BusConnect(viewId);
            GeometryNotificationBus::Handler::BusConnect(GetNodeId());
        }
    }

    void ComboBoxNodePropertyDisplay::CleanupProxyWidget()
    {
        if (m_comboBox)
        {
            UnregisterShortcutDispatcher(m_comboBox);
            delete m_comboBox; // NB: this implicitly deletes m_proxy widget
            m_comboBox = nullptr;
            m_proxyWidget = nullptr;

            m_menuDisplayDirty = false;

            GeometryNotificationBus::Handler::BusDisconnect(GetNodeId());
        }
    }

    void ComboBoxNodePropertyDisplay::OnMenuAboutToDisplay()
    {
        if (m_menuDisplayDirty)
        {
            GraphId graphId;
            SceneMemberRequestBus::EventResult(graphId, GetNodeId(), &SceneMemberRequests::GetScene);

            ViewId viewId;
            SceneRequestBus::EventResult(viewId, graphId, &SceneRequests::GetViewId);

            const bool forceUpdate = true;
            UpdateMenuDisplay(viewId, forceUpdate);

            m_menuDisplayDirty = false;
        }
    }

    void ComboBoxNodePropertyDisplay::UpdateMenuDisplay(const ViewId& viewId, bool forceUpdate)
    {
        if (m_proxyWidget && m_comboBox && (m_comboBox->IsMenuVisible() || forceUpdate))
        {
            QPointF scenePoint = m_proxyWidget->mapToScene(QPoint(0, m_proxyWidget->size().height()));
            QPointF widthPoint = m_proxyWidget->mapToScene(QPoint(m_proxyWidget->size().width(), m_proxyWidget->size().height()));

            AZ::Vector2 globalPoint;
            ViewRequestBus::EventResult(globalPoint, viewId, &ViewRequests::MapToGlobal, ConversionUtils::QPointToVector(scenePoint));

            AZ::Vector2 globalWidthPoint;
            ViewRequestBus::EventResult(globalWidthPoint, viewId, &ViewRequests::MapToGlobal, ConversionUtils::QPointToVector(widthPoint));

            m_comboBox->SetAnchorPoint(ConversionUtils::AZToQPoint(globalPoint).toPoint());

            qreal menuWidth = globalPoint.GetDistance(globalWidthPoint);
            m_comboBox->SetMenuWidth(menuWidth);
        }
        else
        {
            m_menuDisplayDirty = true;
        }
    }

#include <Source/Components/NodePropertyDisplays/ComboBoxNodePropertyDisplay.moc>
}