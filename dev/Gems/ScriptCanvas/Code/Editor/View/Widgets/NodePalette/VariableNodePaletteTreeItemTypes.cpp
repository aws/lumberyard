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
#include <GraphCanvas/Components/Nodes/Variable/VariableNodeBus.h>

#include "VariableNodePaletteTreeItemTypes.h"

#include "Editor/Components/IconComponent.h"

#include "Editor/Nodes/NodeUtils.h"
#include "Editor/Translation/TranslationHelper.h"

#include "ScriptCanvas/Bus/RequestBus.h"
#include "Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h"

#include <Core/Attributes.h>
#include <Editor/Metrics.h>
#include <Libraries/Core/Assign.h>
#include <Libraries/Core/BehaviorContextObjectNode.h>
#include <Libraries/Core/Method.h>

namespace ScriptCanvasEditor
{
    /////////////////////////////////////////
    // CreateVariablePrimitiveNodeMimeEvent
    /////////////////////////////////////////

    void CreateVariablePrimitiveNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateVariablePrimitiveNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ->Field("PrimitiveId", &CreateVariablePrimitiveNodeMimeEvent::m_primitiveId)
            ;
        }
    }

    CreateVariablePrimitiveNodeMimeEvent::CreateVariablePrimitiveNodeMimeEvent(const AZ::Uuid& primitiveId)
        : m_primitiveId(primitiveId)
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateVariablePrimitiveNodeMimeEvent::CreateNode(const AZ::EntityId& graphId) const
    {
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropObject, m_primitiveId, graphId);
        return Nodes::CreateVariablePrimitiveNode(m_primitiveId, graphId);
    }

    /////////////////////////////////////////
    // VariablePrimitiveNodePaletteTreeItem
    /////////////////////////////////////////

    VariablePrimitiveNodePaletteTreeItem::VariablePrimitiveNodePaletteTreeItem(const AZ::Uuid& primitiveId, const QString& nodeName, const QString& iconPath)
        : DraggableNodePaletteTreeItem(nodeName, iconPath)
        , m_primitiveId(primitiveId)
    {
    }

    GraphCanvas::GraphCanvasMimeEvent* VariablePrimitiveNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateVariablePrimitiveNodeMimeEvent(m_primitiveId);
    } 

    //////////////////////////////////////
    // CreateVariableObjectNodeMimeEvent
    //////////////////////////////////////

    void CreateVariableObjectNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateVariableObjectNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ->Field("ClassName", &CreateVariableObjectNodeMimeEvent::m_className)
                ;
        }
    }

    CreateVariableObjectNodeMimeEvent::CreateVariableObjectNodeMimeEvent(const QString& className)
        : m_className(className.toUtf8().data())
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateVariableObjectNodeMimeEvent::CreateNode(const AZ::EntityId& graphId) const
    {
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropObject, AZ::AzTypeInfo<ScriptCanvas::Nodes::Core::BehaviorContextObjectNode>::Uuid(), graphId);
        return Nodes::CreateVariableObjectNode(m_className, graphId);
    }

    //////////////////////////////////////
    // VariableObjectNodePaletteTreeItem
    //////////////////////////////////////

    const QString& VariableObjectNodePaletteTreeItem::GetDefaultIcon()
    {
        static QString defaultIcon;

        if (defaultIcon.isEmpty())
        {
            defaultIcon = IconComponent::LookupClassIcon(ScriptCanvas::Nodes::Core::BehaviorContextObjectNode::RTTI_Type()).c_str();
        }

        return defaultIcon;
    }

    VariableObjectNodePaletteTreeItem::VariableObjectNodePaletteTreeItem(const QString& className)
        : DraggableNodePaletteTreeItem(className, GetDefaultIcon())
        , m_className(className)
    {
        AZStd::string displayClassName = TranslationHelper::GetClassKeyTranslation(TranslationContextGroup::ClassMethod, m_className.toUtf8().data(), TranslationKeyId::Name);

        if (displayClassName.empty())
        {
            SetName(m_className);
        }
        else
        {
            SetName(displayClassName.c_str());
        }

        AZStd::string displayEventTooltip = TranslationHelper::GetClassKeyTranslation(TranslationContextGroup::ClassMethod, m_className.toUtf8().data(), TranslationKeyId::Tooltip);

        if (!displayEventTooltip.empty())
        {
            SetToolTip(displayEventTooltip.c_str());
        }
    }

    GraphCanvas::GraphCanvasMimeEvent* VariableObjectNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateVariableObjectNodeMimeEvent(m_className);
    }

    ///////////////////////////////////
    // CreateGetVariableNodeMimeEvent
    ///////////////////////////////////

    void CreateGetVariableNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateGetVariableNodeMimeEvent, SpecializedCreateNodeMimeEvent>()
                ->Version(0)
                ->Field("VariableId", &CreateGetVariableNodeMimeEvent::m_variableId)
                ;
        }
    }

    CreateGetVariableNodeMimeEvent::CreateGetVariableNodeMimeEvent(const AZ::EntityId& variableId)
        : m_variableId(variableId)
    {
    }

    NodeIdPair CreateGetVariableNodeMimeEvent::ConstructNode(const AZ::EntityId& sceneId, const AZ::Vector2& scenePosition)
    {
        // Random UUID so we can track the Get Variable usages from the editor.
        AZ::EntityId graphId;
        GeneralRequestBus::BroadcastResult(graphId, &GeneralRequests::GetGraphId, sceneId);
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropHandler, AZ::Uuid("{73D471F6-899A-4C82-8D15-FB7BD1D4BCB8}"), graphId);

        NodeIdPair retVal;

        retVal = Nodes::CreateGetVariableNode(m_variableId);

        if (retVal.m_graphCanvasId.IsValid())
        {
            GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::AddNode, retVal.m_graphCanvasId, scenePosition);
            GraphCanvas::NodeUIRequestBus::Event(retVal.m_graphCanvasId, &GraphCanvas::NodeUIRequests::SetSelected, true);
        }

        return retVal;
    }

    bool CreateGetVariableNodeMimeEvent::ExecuteEvent(const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& sceneId)
    {
        NodeIdPair nodeIdPair = ConstructNode(sceneId, sceneDropPosition);

        if (nodeIdPair.m_graphCanvasId.IsValid())
        {
            ScriptCanvasEditor::NodeCreationNotificationBus::Event(sceneId, &ScriptCanvasEditor::NodeCreationNotifications::OnGraphCanvasNodeCreated, nodeIdPair.m_graphCanvasId);
            sceneDropPosition += AZ::Vector2(20, 20);
        }

        return nodeIdPair.m_graphCanvasId.IsValid();
    }

    ///////////////////////////////////
    // GetVariableNodePaletteTreeItem
    ///////////////////////////////////

    const QString& GetVariableNodePaletteTreeItem::GetDefaultIcon()
    {
        static QString defaultIcon;

        if (defaultIcon.isEmpty())
        {
            defaultIcon = IconComponent::LookupClassIcon(AZ::Uuid()).c_str();
        }

        return defaultIcon;
    }

    GetVariableNodePaletteTreeItem::GetVariableNodePaletteTreeItem(const QString& nodeName)
        : DraggableNodePaletteTreeItem(nodeName, GetDefaultIcon())
    {
        SetToolTip("After specifying a variable name, this node will expose output slots that return the specified variable's values.\nVariable names must begin with # (for example, #MyVar).");
    }

    GetVariableNodePaletteTreeItem::GetVariableNodePaletteTreeItem(const AZ::EntityId& variableId)
        : DraggableNodePaletteTreeItem("", GetDefaultIcon())
        , m_variableId(variableId)
    {
        OnNameChanged();
        GraphCanvas::VariableNotificationBus::Handler::BusConnect(m_variableId);
    }

    GetVariableNodePaletteTreeItem::~GetVariableNodePaletteTreeItem()
    {
        GraphCanvas::VariableNotificationBus::Handler::BusDisconnect();
    }

    void GetVariableNodePaletteTreeItem::OnNameChanged()
    {
        AZStd::string displayName;
        GraphCanvas::VariableRequestBus::EventResult(displayName, m_variableId, &GraphCanvas::VariableRequests::GetVariableName);

        AZStd::string fullName = AZStd::string::format("Get %s", displayName.c_str());
        SetName(fullName.c_str());

        AZStd::string tooltip = AZStd::string::format("This node returns %s's values", displayName.c_str());
        SetToolTip(tooltip.c_str());
    }

    GraphCanvas::GraphCanvasMimeEvent* GetVariableNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateGetVariableNodeMimeEvent(m_variableId);
    }

    ///////////////////////////////////
    // CreateSetVariableNodeMimeEvent
    ///////////////////////////////////

    void CreateSetVariableNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateSetVariableNodeMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(0)
                ->Field("VariableId", &CreateSetVariableNodeMimeEvent::m_variableId)
                ;
        }
    }

    CreateSetVariableNodeMimeEvent::CreateSetVariableNodeMimeEvent(const AZ::EntityId& variableId)
        : m_variableId(variableId)
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateSetVariableNodeMimeEvent::CreateNode(const AZ::EntityId& graphId) const
    {
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropObject, AZ::AzTypeInfo<ScriptCanvas::Nodes::Core::Assign>::Uuid(), graphId);
        return Nodes::CreateSetVariableNode(m_variableId, graphId);
    }

    ///////////////////////////////////
    // SetVariableNodePaletteTreeItem
    ///////////////////////////////////

    const QString& SetVariableNodePaletteTreeItem::GetDefaultIcon()
    {
        static QString defaultIcon;

        if (defaultIcon.isEmpty())
        {
            defaultIcon = IconComponent::LookupClassIcon(AZ::Uuid()).c_str();
        }

        return defaultIcon;
    }

    SetVariableNodePaletteTreeItem::SetVariableNodePaletteTreeItem(const QString& nodeName)
        : DraggableNodePaletteTreeItem(nodeName, GetDefaultIcon())
    {
        SetToolTip("This node changes a variable's values according to the data connected to the input slots");
    }

    SetVariableNodePaletteTreeItem::SetVariableNodePaletteTreeItem(const AZ::EntityId& variableId)
        : DraggableNodePaletteTreeItem("", GetDefaultIcon())
        , m_variableId(variableId)
    {
        OnNameChanged();
        GraphCanvas::VariableNotificationBus::Handler::BusConnect(m_variableId);
    }

    SetVariableNodePaletteTreeItem::~SetVariableNodePaletteTreeItem()
    {
        GraphCanvas::VariableNotificationBus::Handler::BusDisconnect();
    }

    void SetVariableNodePaletteTreeItem::OnNameChanged()
    {
        AZStd::string displayName;
        GraphCanvas::VariableRequestBus::EventResult(displayName, m_variableId, &GraphCanvas::VariableRequests::GetVariableName);        

        AZStd::string fullName = AZStd::string::format("Set %s", displayName.c_str());
        SetName(fullName.c_str());

        AZStd::string tooltip = AZStd::string::format("This node changes %s's values according to the data connected to the input slots", displayName.c_str());
        SetToolTip(tooltip.c_str());
    }

    GraphCanvas::GraphCanvasMimeEvent* SetVariableNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateSetVariableNodeMimeEvent(m_variableId);
    }

    ////////////////////////////////////////
    // VariableCategoryNodePaletteTreeItem
    ////////////////////////////////////////

    VariableCategoryNodePaletteTreeItem::VariableCategoryNodePaletteTreeItem(const QString& displayName)
        : NodePaletteTreeItem(displayName)
    {
    }

    void VariableCategoryNodePaletteTreeItem::PreOnChildAdded(GraphCanvasTreeItem* item)
    {
        // Force elements to display in the order they were added rather then alphabetical.
        static_cast<NodePaletteTreeItem*>(item)->SetItemOrdering(GetNumChildren());
    }

    //////////////////////////////////////////
    // LocalVariablesListNodePaletteTreeItem
    //////////////////////////////////////////

    LocalVariablesListNodePaletteTreeItem::LocalVariablesListNodePaletteTreeItem(const QString& displayName)
        : NodePaletteTreeItem(displayName)
    {
        MainWindowNotificationBus::Handler::BusConnect();
    }

    void LocalVariablesListNodePaletteTreeItem::OnActiveSceneChanged(const AZ::EntityId& sceneId)
    {
        if (m_sceneId != sceneId)
        {
            GraphItemCommandNotificationBus::Handler::BusDisconnect(m_sceneId);
            GraphCanvas::SceneVariableNotificationBus::Handler::BusDisconnect(m_sceneId);
            m_sceneId = sceneId;

            if (m_sceneId.IsValid())
            {
                GraphCanvas::SceneVariableNotificationBus::Handler::BusConnect(m_sceneId);
                GraphItemCommandNotificationBus::Handler::BusConnect(m_sceneId);
            }

            RefreshVariableList();
        }
    }

    void LocalVariablesListNodePaletteTreeItem::PostRestore(const UndoData&)
    {
        RefreshVariableList();
    }

    void LocalVariablesListNodePaletteTreeItem::OnVariableCreated(const AZ::EntityId& variableId)
    {
        LocalVariableNodePaletteTreeItem* localVariableTreeItem = CreateChildNode<LocalVariableNodePaletteTreeItem>(variableId);        
        localVariableTreeItem->PopulateChildren();
    }

    void LocalVariablesListNodePaletteTreeItem::OnVariableDestroyed(const AZ::EntityId& variableId)
    {
        int rows = GetNumChildren();

        for (int i = 0; i < rows; ++i)
        {
            LocalVariableNodePaletteTreeItem* treeItem = static_cast<LocalVariableNodePaletteTreeItem*>(ChildForRow(i));

            if (treeItem->GetVariableId() == variableId)
            {
                RemoveChild(treeItem);
                break;
            }
        }
    }

    void LocalVariablesListNodePaletteTreeItem::RefreshVariableList()
    {
        // Need to let the child clear signal out 
        ClearChildren();

        SignalLayoutAboutToBeChanged();
        BlockSignals();

        GraphCanvas::SceneVariableRequestBus::EnumerateHandlersId(m_sceneId, [this](GraphCanvas::SceneVariableRequests* variableRequests)
        {
            AZ::EntityId variableId = variableRequests->GetVariableId();

            LocalVariableNodePaletteTreeItem* rootItem = this->CreateChildNode<LocalVariableNodePaletteTreeItem>(variableId);
            rootItem->PopulateChildren();

            return true;
        });

        UnblockSignals();
        SignalLayoutChanged();
    }

    /////////////////////////////////////
    // LocalVariableNodePaletteTreeItem
    /////////////////////////////////////

    LocalVariableNodePaletteTreeItem::LocalVariableNodePaletteTreeItem(AZ::EntityId variableId)
        : NodePaletteTreeItem("")
        , m_variableId(variableId)
    {
        OnNameChanged();
        GraphCanvas::VariableNotificationBus::Handler::BusConnect(variableId);
    }

    LocalVariableNodePaletteTreeItem::~LocalVariableNodePaletteTreeItem()
    {
        GraphCanvas::VariableNotificationBus::Handler::BusDisconnect();
    }

    void LocalVariableNodePaletteTreeItem::PopulateChildren()
    {
        if (GetNumChildren() == 0)
        {
            SignalLayoutAboutToBeChanged();
            BlockSignals();
            CreateChildNode<GetVariableNodePaletteTreeItem>(GetVariableId());
            CreateChildNode<SetVariableNodePaletteTreeItem>(GetVariableId());
            UnblockSignals();
            SignalLayoutChanged();
        }
    }

    const AZ::EntityId& LocalVariableNodePaletteTreeItem::GetVariableId() const
    {
        return m_variableId;
    }

    void LocalVariableNodePaletteTreeItem::OnNameChanged()
    {
        AZStd::string displayName;
        GraphCanvas::VariableRequestBus::EventResult(displayName, GetVariableId(), &GraphCanvas::VariableRequests::GetVariableName);

        SetName(displayName.c_str());
    }
}