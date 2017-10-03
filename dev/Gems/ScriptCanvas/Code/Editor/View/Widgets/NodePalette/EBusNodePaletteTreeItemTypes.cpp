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

#include <AzCore/Serialization/SerializeContext.h>

#include <QCoreApplication>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>

#include "EBusNodePaletteTreeItemTypes.h"

#include "Editor/Components/IconComponent.h"

#include "Editor/Nodes/NodeUtils.h"
#include "Editor/Translation/TranslationHelper.h"

#include "ScriptCanvas/Bus/RequestBus.h"
#include "Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h"

#include <Core/Attributes.h>
#include <Editor/Metrics.h>
#include <Libraries/Core/EBusEventHandler.h>
#include <Libraries/Core/Method.h>

namespace ScriptCanvasEditor
{
    //////////////////////////////
    // CreateEBusSenderMimeEvent
    //////////////////////////////

    void CreateEBusSenderMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateEBusSenderMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ->Field("BusName", &CreateEBusSenderMimeEvent::m_busName)
                ->Field("EventName", &CreateEBusSenderMimeEvent::m_eventName)
                ;
        }
    }

    CreateEBusSenderMimeEvent::CreateEBusSenderMimeEvent(const QString& busName, const QString& eventName)
        : m_busName(busName.toUtf8().data())
        , m_eventName(eventName.toUtf8().data())
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateEBusSenderMimeEvent::CreateNode(const AZ::EntityId& graphId) const
    {
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropSender, AZ::AzTypeInfo<ScriptCanvas::Nodes::Core::Method>::Uuid(), graphId);
        return Nodes::CreateObjectOrObjectMethodNode(m_busName, m_eventName, graphId);
    }

    /////////////////////////////////
    // EBusSendEventPaletteTreeItem
    /////////////////////////////////

    const QString& EBusSendEventPaletteTreeItem::GetDefaultIcon()
    {
        static QString defaultIcon;

        if (defaultIcon.isEmpty())
        {
            defaultIcon = IconComponent::LookupClassIcon(ScriptCanvas::Nodes::Core::EBusEventHandler::RTTI_Type()).c_str();
        }

        return defaultIcon;
    }

    EBusSendEventPaletteTreeItem::EBusSendEventPaletteTreeItem(const QString& busName, const QString& eventName)
        : DraggableNodePaletteTreeItem(eventName, GetDefaultIcon())
        , m_busName(busName)
        , m_eventName(eventName)
    {
        AZStd::string displayEventName = TranslationHelper::GetKeyTranslation(TranslationContextGroup::EbusSender, m_busName.toUtf8().data(), m_eventName.toUtf8().data(), TranslationItemType::Node, TranslationKeyId::Name);

        if (displayEventName.empty())
        {
            SetName(m_eventName);
        }
        else
        {
            SetName(displayEventName.c_str());
        }

        AZStd::string displayEventTooltip = TranslationHelper::GetKeyTranslation(TranslationContextGroup::EbusSender, m_busName.toUtf8().data(), m_eventName.toUtf8().data(), TranslationItemType::Node, TranslationKeyId::Tooltip);

        if (!displayEventTooltip.empty())
        {
            SetToolTip(displayEventTooltip.c_str());
        }
    }

    GraphCanvas::GraphCanvasMimeEvent* EBusSendEventPaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateEBusSenderMimeEvent(m_busName, m_eventName);
    }

    ///////////////////////////////
    // CreateEBusHandlerMimeEvent
    ///////////////////////////////

    void CreateEBusHandlerMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateEBusHandlerMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ->Field("BusName", &CreateEBusHandlerMimeEvent::m_busName)
                ;
        }
    }

    CreateEBusHandlerMimeEvent::CreateEBusHandlerMimeEvent(const AZStd::string& busName)
        : m_busName(busName)
    {
    }

    CreateEBusHandlerMimeEvent::CreateEBusHandlerMimeEvent(const QString& busName)
        : m_busName(busName.toUtf8().data())
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateEBusHandlerMimeEvent::CreateNode(const AZ::EntityId& graphId) const
    {
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropHandler, AZ::AzTypeInfo<ScriptCanvas::Nodes::Core::EBusEventHandler>::Uuid(), graphId);
        return Nodes::CreateEbusWrapperNode(m_busName, graphId);
    }

    ////////////////////////////////////
    // CreateEBusHandlerEventMimeEvent
    ////////////////////////////////////

    void CreateEBusHandlerEventMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateEBusHandlerEventMimeEvent, SpecializedCreateNodeMimeEvent>()
                ->Version(0)
                ->Field("BusName", &CreateEBusHandlerEventMimeEvent::m_busName)
                ->Field("EventName", &CreateEBusHandlerEventMimeEvent::m_eventName)
                ;
        }
    }

    CreateEBusHandlerEventMimeEvent::CreateEBusHandlerEventMimeEvent(const AZStd::string& busName, const AZStd::string& eventName)
        : m_busName(busName)
        , m_eventName(eventName)
    {
    }

    CreateEBusHandlerEventMimeEvent::CreateEBusHandlerEventMimeEvent(const QString& busName, const QString& eventName)
        : m_busName(busName.toUtf8().data())
        , m_eventName(eventName.toUtf8().data())
    {
    }

    NodeIdPair CreateEBusHandlerEventMimeEvent::ConstructNode(const AZ::EntityId& sceneId, const AZ::Vector2& scenePosition)
    {
        NodeIdPair eventNode = CreateEventNode(sceneId, scenePosition);

        CreateEBusHandlerMimeEvent ebusMimeEvent(m_busName);

        AZ::Vector2 position = scenePosition;
        if (ebusMimeEvent.ExecuteEvent(position, position, sceneId))
        {
            NodeIdPair handlerNode = ebusMimeEvent.GetCreatedPair();

            GraphCanvas::WrappedNodeConfiguration configuration;
            EBusNodeDescriptorRequestBus::EventResult(configuration, handlerNode.m_graphCanvasId, &EBusNodeDescriptorRequests::GetEventConfiguration, m_eventName);

            GraphCanvas::WrapperNodeRequestBus::Event(handlerNode.m_graphCanvasId, &GraphCanvas::WrapperNodeRequests::WrapNode, eventNode.m_graphCanvasId, configuration);
        }

        return eventNode;
    }

    bool CreateEBusHandlerEventMimeEvent::ExecuteEvent(const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& sceneId)
    {
        NodeIdPair eventNode = CreateEventNode(sceneId, sceneDropPosition);

        if (eventNode.m_graphCanvasId.IsValid())
        {
            GraphCanvas::NodeUIRequestBus::Event(eventNode.m_graphCanvasId, &GraphCanvas::NodeUIRequests::SetSelected, true);
            ScriptCanvasEditor::NodeCreationNotificationBus::Event(sceneId, &ScriptCanvasEditor::NodeCreationNotifications::OnGraphCanvasNodeCreated, eventNode.m_graphCanvasId);

            sceneDropPosition += AZ::Vector2(20, 20);
        }
        return eventNode.m_graphCanvasId.IsValid();
    }

    NodeIdPair CreateEBusHandlerEventMimeEvent::CreateEventNode(const AZ::EntityId& sceneId, const AZ::Vector2& scenePosition) const
    {
        AZ::EntityId graphId;
        GeneralRequestBus::BroadcastResult(graphId, &GeneralRequests::GetGraphId, sceneId);

        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropHandler, AZ::AzTypeInfo<ScriptCanvas::Nodes::Core::EBusEventHandler>::Uuid(), graphId);

        NodeIdPair nodeIdPair = Nodes::CreateEbusEventNode(m_busName, m_eventName, graphId);

        if (nodeIdPair.m_graphCanvasId.IsValid())
        {
            GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::AddNode, nodeIdPair.m_graphCanvasId, scenePosition);
        }

        return nodeIdPair;
    }

    ///////////////////////////////////
    // EBusHandleEventPaletteTreeItem
    ///////////////////////////////////

    const QString& EBusHandleEventPaletteTreeItem::GetDefaultIcon()
    {
        static QString defaultIcon;

        if (defaultIcon.isEmpty())
        {
            defaultIcon = IconComponent::LookupClassIcon(ScriptCanvas::Nodes::Core::Method::RTTI_Type()).c_str();
        }

        return defaultIcon;
    }

    EBusHandleEventPaletteTreeItem::EBusHandleEventPaletteTreeItem(const QString& busName, const QString& eventName)
        : DraggableNodePaletteTreeItem(eventName, GetDefaultIcon())
        , m_busName(busName)
        , m_eventName(eventName)
    {
        AZStd::string displayEventName = TranslationHelper::GetKeyTranslation(TranslationContextGroup::EbusHandler, m_busName.toUtf8().data(), m_eventName.toUtf8().data(), TranslationItemType::Node, TranslationKeyId::Name);

        if (displayEventName.empty())
        {
            SetName(m_eventName);
        }
        else
        {
            SetName(displayEventName.c_str());
        }

        AZStd::string displayEventTooltip = TranslationHelper::GetKeyTranslation(TranslationContextGroup::EbusHandler, m_busName.toUtf8().data(), m_eventName.toUtf8().data(), TranslationItemType::Node, TranslationKeyId::Tooltip);

        if (!displayEventTooltip.empty())
        {
            SetToolTip(displayEventTooltip.c_str());
        }
    }

    GraphCanvas::GraphCanvasMimeEvent* EBusHandleEventPaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateEBusHandlerEventMimeEvent(m_busName, m_eventName);
    }
}