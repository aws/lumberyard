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

#include <functional>

#include <QGraphicsLayoutItem>
#include <QGraphicsGridLayout>
#include <QGraphicsScene>
#include <qgraphicssceneevent.h>

#include <AzCore/RTTI/TypeInfo.h>

#include <Components/Nodes/Wrapper/WrapperNodeLayoutComponent.h>

#include <Components/Nodes/NodeComponent.h>
#include <Components/Nodes/General/GeneralNodeFrameComponent.h>
#include <Components/Nodes/General/GeneralSlotLayoutComponent.h>
#include <Components/Nodes/General/GeneralNodeTitleComponent.h>
#include <Components/StylingComponent.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/tools.h>
#include <Styling/StyleHelper.h>

namespace GraphCanvas
{    
    //////////////////////
    // WrappedNodeLayout
    //////////////////////
    
    WrapperNodeLayoutComponent::WrappedNodeLayout::WrappedNodeLayout(WrapperNodeLayoutComponent& wrapperLayoutComponent)
        : QGraphicsWidget(nullptr)
        , m_wrapperLayoutComponent(wrapperLayoutComponent)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

        m_layout = new QGraphicsLinearLayout(Qt::Vertical);
        m_layout->setInstantInvalidatePropagation(true);

        setLayout(m_layout);
    }

    WrapperNodeLayoutComponent::WrappedNodeLayout::~WrappedNodeLayout()
    {
    }
    
    void WrapperNodeLayoutComponent::WrappedNodeLayout::RefreshStyle()
    {
        prepareGeometryChange();

        m_styleHelper.SetStyle(m_wrapperLayoutComponent.GetEntityId(), Styling::Elements::WrapperNode::NodeLayout);

        qreal margin = m_styleHelper.GetAttribute(Styling::Attribute::Margin, 0.0);
        setContentsMargins(margin, margin, margin, margin);

        setMinimumSize(m_styleHelper.GetMinimumSize());
        setMaximumSize(m_styleHelper.GetMaximumSize());
        
        // Layout spacing
        m_layout->setSpacing(m_styleHelper.GetAttribute(Styling::Attribute::Spacing, 0.0));
        
        m_layout->invalidate();
        m_layout->updateGeometry();

        updateGeometry();
        adjustSize();
        update();
    }
    
    void WrapperNodeLayoutComponent::WrappedNodeLayout::RefreshLayout()
    {
        prepareGeometryChange();
        ClearLayout();
        LayoutItems();
    }
    
    void WrapperNodeLayoutComponent::WrappedNodeLayout::LayoutItems()
    {
        if (!m_wrapperLayoutComponent.m_wrappedNodes.empty())
        {
            setVisible(true);
            for (const AZ::EntityId& nodeId : m_wrapperLayoutComponent.m_wrappedNodes)
            {
                QGraphicsLayoutItem* rootLayoutItem = nullptr;
                RootVisualRequestBus::EventResult(rootLayoutItem, nodeId, &RootVisualRequests::GetRootGraphicsLayoutItem);

                if (rootLayoutItem)
                {
                    m_layout->addItem(rootLayoutItem);
                }
            }
            
            adjustSize();
            updateGeometry();
            update();
        }
        else
        {
            setVisible(false);
        }
    }
    
    void WrapperNodeLayoutComponent::WrappedNodeLayout::ClearLayout()
    {
        while (m_layout->count() > 0)
        {
            QGraphicsLayoutItem* layoutItem = m_layout->itemAt(m_layout->count() - 1);
            m_layout->removeAt(m_layout->count() - 1);
            layoutItem->setParentLayoutItem(nullptr);
        }
    }

    ////////////////////////////////////
    // WrappedNodeActionGraphicsWidget
    ////////////////////////////////////

    WrapperNodeLayoutComponent::WrappedNodeActionGraphicsWidget::WrappedNodeActionGraphicsWidget(WrapperNodeLayoutComponent& wrapperLayoutComponent)
        : QGraphicsWidget(nullptr)
        , m_wrapperLayoutComponent(wrapperLayoutComponent)
        , m_styleState(StyleState::Empty)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFlag(ItemIsFocusable);
        setAcceptHoverEvents(true);
        setAcceptDrops(true);

        QGraphicsLinearLayout* paddingLayout = new QGraphicsLinearLayout(Qt::Vertical);
        paddingLayout->setContentsMargins(6, 6, 6, 6);
        paddingLayout->setSpacing(0);

        m_displayLabel = new GraphCanvasLabel();
        m_displayLabel->setZValue(1);
        m_displayLabel->setFlag(ItemIsFocusable);
        m_displayLabel->setFocusPolicy(Qt::StrongFocus);
        m_displayLabel->setAcceptDrops(true);
        m_displayLabel->setAcceptHoverEvents(true);
        m_displayLabel->setAcceptedMouseButtons(Qt::MouseButton::LeftButton);

        paddingLayout->addItem(m_displayLabel);
        setLayout(paddingLayout);
    }

    void WrapperNodeLayoutComponent::WrappedNodeActionGraphicsWidget::OnAddedToScene()
    {
        m_displayLabel->installSceneEventFilter(this);
    }

    void WrapperNodeLayoutComponent::WrappedNodeActionGraphicsWidget::RefreshStyle()
    {
        switch (m_styleState)
        {
        case StyleState::Empty:
            m_displayLabel->SetStyle(m_wrapperLayoutComponent.GetEntityId(), Styling::Elements::WrapperNode::ActionLabelEmptyNodes);
            break;
        case StyleState::HasElements:
            m_displayLabel->SetStyle(m_wrapperLayoutComponent.GetEntityId(), Styling::Elements::WrapperNode::ActionLabel);
            break;
        }
    }

    void WrapperNodeLayoutComponent::WrappedNodeActionGraphicsWidget::SetActionString(const QString& displayString)
    {
        m_displayLabel->SetLabel(displayString.toUtf8().data());
    }

    void WrapperNodeLayoutComponent::WrappedNodeActionGraphicsWidget::SetStyleState(StyleState state)
    {
        if (m_styleState != state)
        {
            m_styleState = state;
            RefreshStyle();
        }
    }

    bool WrapperNodeLayoutComponent::WrappedNodeActionGraphicsWidget::sceneEventFilter(QGraphicsItem*, QEvent* event)
    {
        switch (event->type())
        {
            case QEvent::GraphicsSceneDragEnter:
            {
                QGraphicsSceneDragDropEvent* dragDropEvent = static_cast<QGraphicsSceneDragDropEvent*>(event);

                if (m_wrapperLayoutComponent.ShouldAcceptDrop(dragDropEvent->mimeData()))
                {
                    m_acceptDrop = true;

                    dragDropEvent->accept();
                    dragDropEvent->acceptProposedAction();

                    m_displayLabel->GetStyleHelper().AddSelector(Styling::States::ValidDrop);
                }
                else
                {
                    m_acceptDrop = false;

                    m_displayLabel->GetStyleHelper().AddSelector(Styling::States::InvalidDrop);
                }

                m_displayLabel->update();
                return true;
            }
            case QEvent::GraphicsSceneDragLeave:
            {
                event->accept();
                if (m_acceptDrop)
                {
                    m_displayLabel->GetStyleHelper().RemoveSelector(Styling::States::ValidDrop);

                    m_acceptDrop = false;
                    m_wrapperLayoutComponent.OnDragLeave();
                }
                else
                {
                    m_displayLabel->GetStyleHelper().RemoveSelector(Styling::States::InvalidDrop);
                }

                m_displayLabel->update();

                return true;
            }
            case QEvent::GraphicsSceneDrop:
            {
                if (m_acceptDrop)
                {
                    m_displayLabel->GetStyleHelper().RemoveSelector(Styling::States::ValidDrop);
                }
                else
                {
                    m_displayLabel->GetStyleHelper().RemoveSelector(Styling::States::InvalidDrop);
                }

                m_displayLabel->update();
                break;
            }
            case QEvent::GraphicsSceneMousePress:
            {
                return true;
            }
            case QEvent::GraphicsSceneMouseRelease:
            {
                QGraphicsSceneMouseEvent* mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);
                m_wrapperLayoutComponent.OnActionWidgetClicked(mouseEvent->scenePos(), mouseEvent->screenPos());
                return true;
            }
            default:
                break;
        }

        return false;
    }
    
    ///////////////////////////////
    // WrapperNodeLayoutComponent
    ///////////////////////////////
    
    void WrapperNodeLayoutComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<WrappedNodeConfiguration>()
                ->Version(1)
                ->Field("LayoutOrder", &WrappedNodeConfiguration::m_layoutOrder)
                ->Field("ElementOrder", &WrappedNodeConfiguration::m_elementOrdering)
            ;
            serializeContext->Class<WrapperNodeLayoutComponent>()
                ->Version(2)
                ->Field("ElementOrdering", &WrapperNodeLayoutComponent::m_elementCounter)
                ->Field("WrappedNodeConfigurations", &WrapperNodeLayoutComponent::m_wrappedNodeConfigurations)
            ;
        }
    }

    AZ::Entity* WrapperNodeLayoutComponent::CreateWrapperNodeEntity(const char* nodeType)
    {
        // Create this Node's entity.
        AZ::Entity* entity = NodeComponent::CreateCoreNodeEntity();

        entity->CreateComponent<GeneralNodeFrameComponent>();
        entity->CreateComponent<StylingComponent>(Styling::Elements::WrapperNode::Node, AZ::EntityId(), nodeType);
        entity->CreateComponent<WrapperNodeLayoutComponent>();
        entity->CreateComponent<GeneralNodeTitleComponent>();
        entity->CreateComponent<GeneralSlotLayoutComponent>();

        return entity;
    }

    WrapperNodeLayoutComponent::WrapperNodeLayoutComponent()
        : m_elementCounter(0)
        , m_wrappedNodes(WrappedNodeConfigurationComparator(&m_wrappedNodeConfigurations))
        , m_title(nullptr)
        , m_slots(nullptr)
        , m_wrappedNodeLayout(nullptr)
    {
    }

    WrapperNodeLayoutComponent::~WrapperNodeLayoutComponent()
    {
        ClearLayout();
    }
	
	void WrapperNodeLayoutComponent::Init()
    {
        NodeLayoutComponent::Init();

        m_layout = new QGraphicsLinearLayout(Qt::Vertical);
        GetLayoutAs<QGraphicsLinearLayout>()->setInstantInvalidatePropagation(true);
        
        m_wrappedNodeLayout = aznew WrappedNodeLayout((*this));
        m_wrapperNodeActionWidget = aznew WrappedNodeActionGraphicsWidget((*this));
        
        for (auto& mapPair : m_wrappedNodeConfigurations)
        {
            m_wrappedNodes.insert(mapPair.first);
        }
    }

    void WrapperNodeLayoutComponent::Activate()
    {
        NodeLayoutComponent::Activate();

        NodeNotificationBus::MultiHandler::BusConnect(GetEntityId());
        WrapperNodeRequestBus::Handler::BusConnect(GetEntityId());
    }

    void WrapperNodeLayoutComponent::Deactivate()
    {
        ClearLayout();

        NodeLayoutComponent::Deactivate();

        NodeNotificationBus::MultiHandler::BusDisconnect();

        WrapperNodeRequestBus::Handler::BusDisconnect();
        StyleNotificationBus::Handler::BusDisconnect();
    }

    void WrapperNodeLayoutComponent::SetActionString(const QString& actionString)
    {
        m_wrapperNodeActionWidget->SetActionString(actionString);
    }

    AZStd::vector< AZ::EntityId > WrapperNodeLayoutComponent::GetWrappedNodeIds() const
    {
        return AZStd::vector< AZ::EntityId >(m_wrappedNodes.begin(), m_wrappedNodes.end());
    }
    
    void WrapperNodeLayoutComponent::WrapNode(const AZ::EntityId& nodeId, const WrappedNodeConfiguration& nodeConfiguration)
    {
        if (m_wrappedNodeConfigurations.find(nodeId) == m_wrappedNodeConfigurations.end())
        {
            NodeNotificationBus::MultiHandler::BusConnect(nodeId);
            NodeNotificationBus::Event(nodeId, &NodeNotifications::OnNodeWrapped, GetEntityId());
            WrapperNodeNotificationBus::Event(GetEntityId(), &WrapperNodeNotifications::OnWrappedNode, nodeId);

            m_wrappedNodeConfigurations[nodeId] = nodeConfiguration;
            m_wrappedNodeConfigurations[nodeId].m_elementOrdering = m_elementCounter;
            
            ++m_elementCounter;
            
            m_wrappedNodes.insert(nodeId);
            m_wrappedNodeLayout->RefreshLayout();

            RefreshActionStyle();
        }
    }
    
    void WrapperNodeLayoutComponent::UnwrapNode(const AZ::EntityId& nodeId)
    {
        auto configurationIter = m_wrappedNodeConfigurations.find(nodeId);

        if (configurationIter != m_wrappedNodeConfigurations.end())
        {
            NodeNotificationBus::MultiHandler::BusDisconnect(nodeId);
            NodeNotificationBus::Event(nodeId, &NodeNotifications::OnNodeUnwrapped, GetEntityId());
            WrapperNodeNotificationBus::Event(GetEntityId(), &WrapperNodeNotifications::OnUnwrappedNode, nodeId);

            m_wrappedNodes.erase(nodeId);
            m_wrappedNodeConfigurations.erase(configurationIter);

            m_wrappedNodeLayout->RefreshLayout();

            RefreshActionStyle();
        }
    }

    void WrapperNodeLayoutComponent::OnNodeActivated()
    {
        AZ::EntityId nodeId = (*NodeNotificationBus::GetCurrentBusId());

        if (nodeId == GetEntityId())
        {
            CreateLayout();
        }
    }

    void WrapperNodeLayoutComponent::OnNodeAboutToSerialize(SceneSerialization& sceneSerialization)
    {
        AZ::EntityId nodeId = (*NodeNotificationBus::GetCurrentBusId());

        if (nodeId == GetEntityId())
        {
            for (const AZ::EntityId& entityId : m_wrappedNodes)
            {
                AZ::Entity* wrappedNodeEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(wrappedNodeEntity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

                sceneSerialization.GetSceneData().m_nodes.insert(wrappedNodeEntity);
            }
        }
    }

    void WrapperNodeLayoutComponent::OnNodeDeserialized(const SceneSerialization& sceneSerialization)
    {
        AZ::EntityId nodeId = (*NodeNotificationBus::GetCurrentBusId());

        if (nodeId == GetEntityId())
        {
            m_elementCounter = 0;
            m_wrappedNodes.clear();

            WrappedNodeConfigurationMap oldConfigurations = m_wrappedNodeConfigurations;
            m_wrappedNodeConfigurations.clear();

            for (auto& configurationPair : oldConfigurations)
            {
                if (sceneSerialization.FindRemappedEntityId(configurationPair.first).IsValid())
                {
                    m_wrappedNodeConfigurations.insert(configurationPair);
                    m_wrappedNodes.insert(configurationPair.first);
                }
            }
        }
    }

    void WrapperNodeLayoutComponent::OnAddedToScene(const AZ::EntityId& sceneId)
    {
        AZ::EntityId nodeId = (*NodeNotificationBus::GetCurrentBusId());

        if (nodeId == GetEntityId())
        {
            for (const AZ::EntityId& node : m_wrappedNodes)
            {
                NodeNotificationBus::MultiHandler::BusConnect(node);

                // Test to make sure the node is activated before we signal out anything to it.
                //
                // We listen for when the node activates, so these calls will be handled there.
                if (NodeRequestBus::FindFirstHandler(node) != nullptr)
                {
                    NodeNotificationBus::Event(node, &NodeNotifications::OnNodeWrapped, GetEntityId());
                    WrapperNodeNotificationBus::Event(GetEntityId(), &WrapperNodeNotifications::OnWrappedNode, node);
                }
            }

            RefreshActionStyle();
            UpdateLayout();
            OnStyleChanged();

            // Event filtering for graphics items can only be done by items in the same scene.
            // and by objects that are in a scene. So I need to wait for them to be added to the
            // scene before installing my filters.
            m_wrapperNodeActionWidget->OnAddedToScene();

            StyleNotificationBus::Handler::BusConnect(GetEntityId());
        }
        else
        {
            NodeNotificationBus::Event(nodeId, &NodeNotifications::OnNodeWrapped, GetEntityId());
            WrapperNodeNotificationBus::Event(GetEntityId(), &WrapperNodeNotifications::OnWrappedNode, nodeId);

            // Sort ick, but should work for now.
            // Mostly ick because it'll redo this layout waaaay to many times.
            UpdateLayout();
        }
    }

    void WrapperNodeLayoutComponent::OnRemovedFromScene(const AZ::EntityId& sceneId)
    {
        AZ::EntityId nodeId = (*NodeNotificationBus::GetCurrentBusId());

        if (nodeId == GetEntityId())
        {
            AZStd::unordered_set< AZ::EntityId > deleteNodes(m_wrappedNodes.begin(), m_wrappedNodes.end());
            SceneRequestBus::Event(sceneId, &SceneRequests::Delete, deleteNodes);
        }
        else
        {
            UnwrapNode(nodeId);
        }
    }

    void WrapperNodeLayoutComponent::OnStyleChanged()
    {
        m_styleHelper.SetStyle(GetEntityId());

        qreal border = m_styleHelper.GetAttribute(Styling::Attribute::BorderWidth, 0.0);
        qreal spacing = m_styleHelper.GetAttribute(Styling::Attribute::Spacing, 0.0);
        qreal margin = m_styleHelper.GetAttribute(Styling::Attribute::Margin, 0.0);

        GetLayoutAs<QGraphicsLinearLayout>()->setContentsMargins(margin + border, margin + border, margin + border, margin + border);
        GetLayoutAs<QGraphicsLinearLayout>()->setSpacing(spacing);

        m_wrappedNodeLayout->RefreshStyle();
        m_wrapperNodeActionWidget->RefreshStyle();

        RefreshDisplay();
    }

    void WrapperNodeLayoutComponent::RefreshActionStyle()
    {
        if (!m_wrappedNodes.empty())
        {
            m_wrapperNodeActionWidget->SetStyleState(WrappedNodeActionGraphicsWidget::StyleState::HasElements);
        }
        else
        {
            m_wrapperNodeActionWidget->SetStyleState(WrappedNodeActionGraphicsWidget::StyleState::Empty);
        }
    }

    bool WrapperNodeLayoutComponent::ShouldAcceptDrop(const QMimeData* mimeData) const
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        bool shouldAcceptDrop = false;
        WrapperNodeActionRequestBus::EventResult(shouldAcceptDrop, sceneId, &WrapperNodeActionRequests::ShouldAcceptDrop, GetEntityId(), mimeData);

        if (shouldAcceptDrop)
        {
            WrapperNodeActionRequestBus::Event(sceneId, &WrapperNodeActionRequests::AddWrapperDropTarget, GetEntityId());
        }
        return shouldAcceptDrop;
    }

    void WrapperNodeLayoutComponent::OnDragLeave() const
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        WrapperNodeActionRequestBus::Event(sceneId, &WrapperNodeActionRequests::RemoveWrapperDropTarget, GetEntityId());
    }

    void WrapperNodeLayoutComponent::OnActionWidgetClicked(const QPointF& scenePoint, const QPoint& screenPoint) const
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        SceneUIRequestBus::Event(sceneId, &SceneUIRequests::OnWrapperNodeActionWidgetClicked, GetEntityId(), m_wrapperNodeActionWidget->boundingRect().toRect(), scenePoint, screenPoint);
    }
    
    void WrapperNodeLayoutComponent::ClearLayout()
    {
        if (m_layout)
        {
            for (int i=m_layout->count() - 1; i >= 0; --i)
            {
                m_layout->removeAt(i);
            }
        }
    }

    void WrapperNodeLayoutComponent::CreateLayout()
    {
        ClearLayout();

        if (m_title == nullptr)
        {
            NodeTitleRequestBus::EventResult(m_title, GetEntityId(), &NodeTitleRequests::GetGraphicsWidget);
        }

        if (m_slots == nullptr)
        {
            NodeSlotsRequestBus::EventResult(m_slots, GetEntityId(), &NodeSlotsRequests::GetGraphicsLayoutItem);
        }

        if (m_title)
        {
            GetLayoutAs<QGraphicsLinearLayout>()->addItem(m_title);
        }

        if (m_slots)
        {
            GetLayoutAs<QGraphicsLinearLayout>()->addItem(m_slots);
        }

        GetLayoutAs<QGraphicsLinearLayout>()->addItem(m_wrappedNodeLayout);
        GetLayoutAs<QGraphicsLinearLayout>()->addItem(m_wrapperNodeActionWidget);
    }
    
    void WrapperNodeLayoutComponent::UpdateLayout()
    {
        m_wrappedNodeLayout->RefreshLayout();
        
        RefreshDisplay();
    }
    
    void WrapperNodeLayoutComponent::RefreshDisplay()
    {
        m_layout->invalidate();
        m_layout->updateGeometry();
    }
}