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

#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>

#include <Components/NodePropertyDisplays/EntityIdNodePropertyDisplay.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Widgets/NodePropertyBus.h>
#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    ////////////////////////////////
    // EntityIdGraphicsEventFilter
    ////////////////////////////////
    EntityIdGraphicsEventFilter::EntityIdGraphicsEventFilter(EntityIdNodePropertyDisplay* propertyDisplay)
        : QGraphicsItem(nullptr)
        , m_owner(propertyDisplay)
    {
    }

    bool EntityIdGraphicsEventFilter::sceneEventFilter(QGraphicsItem*, QEvent* event)
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
    // EntityIdNodePropertyDisplay
    ////////////////////////////////
    EntityIdNodePropertyDisplay::EntityIdNodePropertyDisplay(EntityIdDataInterface* dataInterface)
        : m_dataInterface(dataInterface)
        , m_dragState(DragState::Idle)
    {
        // This label is never displayed. Used to just get the name.
        m_entityIdLabel.setProperty("HasNoWindowDecorations", true);

        m_dataInterface->RegisterDisplay(this);
        
        m_disabledLabel = aznew GraphCanvasLabel();
        m_displayLabel = aznew GraphCanvasLabel();
        
        m_proxyWidget = new QGraphicsProxyWidget();
        
        m_proxyWidget->setFlag(QGraphicsItem::ItemIsFocusable, true);
        m_proxyWidget->setFocusPolicy(Qt::StrongFocus);

        m_propertyEntityIdCtrl = aznew AzToolsFramework::PropertyEntityIdCtrl();
        m_propertyEntityIdCtrl->setProperty("HasNoWindowDecorations", true);

        m_propertyEntityIdCtrl->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(m_propertyEntityIdCtrl, &AzToolsFramework::PropertyEntityIdCtrl::customContextMenuRequested, [this](const QPoint& pos) { this->ShowContextMenu(pos); });

        QObject::connect(m_propertyEntityIdCtrl, &AzToolsFramework::PropertyEntityIdCtrl::OnPickStart, [this]() { this->EditStart(); });
        QObject::connect(m_propertyEntityIdCtrl, &AzToolsFramework::PropertyEntityIdCtrl::OnPickComplete, [this]() { this->EditFinished(); });
        QObject::connect(m_propertyEntityIdCtrl, &AzToolsFramework::PropertyEntityIdCtrl::OnEntityIdChanged, [this]() { this->SubmitValue(); });
        
        m_proxyWidget->setWidget(m_propertyEntityIdCtrl);

        RegisterShortcutDispatcher(m_propertyEntityIdCtrl);
    }

    void EntityIdNodePropertyDisplay::ShowContextMenu(const QPoint& pos)
    {
        m_dataInterface->OnShowContextMenu(m_propertyEntityIdCtrl, pos);
    }
    
    EntityIdNodePropertyDisplay::~EntityIdNodePropertyDisplay()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);

        delete m_dataInterface;
        delete m_disabledLabel;
        delete m_proxyWidget;
        delete m_displayLabel;
    }
    
    void EntityIdNodePropertyDisplay::RefreshStyle()
    {
        m_disabledLabel->SetSceneStyle(GetSceneId(), NodePropertyDisplay::CreateDisabledLabelStyle("entityId").c_str());

        AZStd::string styleName = NodePropertyDisplay::CreateDisplayLabelStyle("entityId");
        m_displayLabel->SetSceneStyle(GetSceneId(), styleName.c_str());
        
        QSizeF minimumSize = m_displayLabel->minimumSize();
        QSizeF maximumSize = m_displayLabel->maximumSize();

        m_propertyEntityIdCtrl->setMinimumSize(minimumSize.width(), minimumSize.height());
        m_propertyEntityIdCtrl->setMaximumSize(maximumSize.width(), maximumSize.height());
    }
    
    void EntityIdNodePropertyDisplay::UpdateDisplay()
    {
        AZ::EntityId valueEntityId = m_dataInterface->GetEntityId();

        if (!AZ::EntityBus::Handler::BusIsConnectedId(valueEntityId))
        {
            AZ::EntityBus::Handler::BusDisconnect();

            if (valueEntityId.IsValid())
            {
                AZ::EntityBus::Handler::BusConnect(valueEntityId);
            }
        }

        const AZStd::string& name = m_dataInterface->GetNameOverride();

        m_propertyEntityIdCtrl->SetCurrentEntityId(valueEntityId, false, name);
        m_entityIdLabel.SetEntityId(valueEntityId, name);

        QString displayLabel = m_entityIdLabel.text();
        if (displayLabel.isEmpty())
        {
            displayLabel = "<None>";
        }
            
        m_displayLabel->SetLabel(displayLabel.toUtf8().data());
        m_proxyWidget->update();
    }
    
    QGraphicsLayoutItem* EntityIdNodePropertyDisplay::GetDisabledGraphicsLayoutItem() const
    {
        return m_disabledLabel;
    }
    
    QGraphicsLayoutItem* EntityIdNodePropertyDisplay::GetDisplayGraphicsLayoutItem() const
    {
        return m_displayLabel;
    }
    
    QGraphicsLayoutItem* EntityIdNodePropertyDisplay::GetEditableGraphicsLayoutItem() const
    {
        return m_proxyWidget;
    }

    void EntityIdNodePropertyDisplay::OnEntityNameChanged(const AZStd::string& name)
    {
        UpdateDisplay();
    }

    void EntityIdNodePropertyDisplay::dragEnterEvent(QGraphicsSceneDragDropEvent* dragDropEvent)
    {
        bool isConnected = false;
        SlotRequestBus::EventResult(isConnected, GetId(), &SlotRequests::HasConnections);

        m_dragState = DragState::Invalid;

        if (!isConnected)
        {
            const QMimeData* dropMimeData = dragDropEvent->mimeData();
            if (dropMimeData->hasFormat(AzToolsFramework::EditorEntityIdContainer::GetMimeType()))
            {
                QByteArray arrayData = dropMimeData->data(AzToolsFramework::EditorEntityIdContainer::GetMimeType());

                AzToolsFramework::EditorEntityIdContainer entityIdListContainer;
                if (entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()))
                {
                    // We can only support a drop containing a single entityId.
                    if (entityIdListContainer.m_entityIds.size() == 1)
                    {
                        dragDropEvent->accept();
                        dragDropEvent->acceptProposedAction();

                        m_dragState = DragState::Valid;
                    }
                }
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

    void EntityIdNodePropertyDisplay::dragLeaveEvent(QGraphicsSceneDragDropEvent* dragDropEvent)
    {        
        dragDropEvent->accept();
        ResetDragState();
    }

    void EntityIdNodePropertyDisplay::dropEvent(QGraphicsSceneDragDropEvent* dropEvent)
    {
        if (m_dragState == DragState::Valid)
        {
            const QMimeData* dropMimeData = dropEvent->mimeData();
            if (dropMimeData->hasFormat(AzToolsFramework::EditorEntityIdContainer::GetMimeType()))
            {
                QByteArray arrayData = dropMimeData->data(AzToolsFramework::EditorEntityIdContainer::GetMimeType());

                AzToolsFramework::EditorEntityIdContainer entityIdListContainer;
                if (entityIdListContainer.FromBuffer(arrayData.constData(), arrayData.size()))
                {
                    if (entityIdListContainer.m_entityIds.size() == 1)
                    {
                        dropEvent->accept();
                        m_propertyEntityIdCtrl->SetCurrentEntityId(entityIdListContainer.m_entityIds.front(), true, "");
                    }
                }
            }
        }

        ResetDragState();
    }

    void EntityIdNodePropertyDisplay::OnIdSet()
    {
        QGraphicsItem* ownerItem = nullptr;
        SceneMemberUIRequestBus::EventResult(ownerItem, GetId(), &SceneMemberUIRequests::GetRootGraphicsItem);

        if (ownerItem)
        {
            ownerItem->setAcceptDrops(true);

            QGraphicsScene* scene = ownerItem->scene();

            if (scene)
            {
                EntityIdGraphicsEventFilter* filter = aznew EntityIdGraphicsEventFilter(this);
                scene->addItem(filter);
                ownerItem->installSceneEventFilter(filter);
            }
        }
    }

    void EntityIdNodePropertyDisplay::ResetDragState()
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
    
    void EntityIdNodePropertyDisplay::EditStart()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::LockEditState, this);
        TryAndSelectNode();
    }
    
    void EntityIdNodePropertyDisplay::SubmitValue()
    {
        m_dataInterface->SetEntityId(m_propertyEntityIdCtrl->GetEntityId());
        UpdateDisplay();
    }

    void EntityIdNodePropertyDisplay::EditFinished()
    {
        SubmitValue();
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);
    }

#include <Source/Components/NodePropertyDisplays/EntityIdNodePropertyDisplay.moc>
}