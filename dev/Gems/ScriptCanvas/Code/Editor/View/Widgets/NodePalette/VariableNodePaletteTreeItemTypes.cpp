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
#include <qmenu.h>

#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

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

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

namespace ScriptCanvasEditor
{
    ///////////////////////////////////
    // CreateGetVariableNodeMimeEvent
    ///////////////////////////////////

    void CreateGetVariableNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateGetVariableNodeMimeEvent, CreateNodeMimeEvent>()
                ->Version(0)
                ->Field("VariableId", &CreateGetVariableNodeMimeEvent::m_variableId)
                ;
        }
    }

    CreateGetVariableNodeMimeEvent::CreateGetVariableNodeMimeEvent(const ScriptCanvas::VariableId& variableId)
        : m_variableId(variableId)
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateGetVariableNodeMimeEvent::CreateNode(const AZ::EntityId& scriptCanvasGraphId) const
    {
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropObject, "{A9784FF3-E749-4EB4-B5DB-DF510F7CD151}", scriptCanvasGraphId);
        return Nodes::CreateGetVariableNode(m_variableId, scriptCanvasGraphId);
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

    GetVariableNodePaletteTreeItem::GetVariableNodePaletteTreeItem()
        : DraggableNodePaletteTreeItem("Get Variable", ScriptCanvasEditor::AssetEditorId)
    {
        SetToolTip("After specifying a variable name, this node will expose output slots that return the specified variable's values.\nVariable names must begin with # (for example, #MyVar).");
    }

    GetVariableNodePaletteTreeItem::GetVariableNodePaletteTreeItem(const ScriptCanvas::VariableId& variableId, const AZ::EntityId& scriptCanvasGraphId)
        : DraggableNodePaletteTreeItem("", ScriptCanvasEditor::AssetEditorId)
        , m_variableId(variableId)
    {
        AZStd::string_view variableName;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableName, scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableName, variableId);
        OnVariableRenamed(variableName);

        ScriptCanvas::VariableNotificationBus::Handler::BusConnect(m_variableId);

        ScriptCanvas::Data::Type scriptCanvasType = ScriptCanvas::Data::Type::Invalid();        
        ScriptCanvas::VariableRequestBus::EventResult(scriptCanvasType, m_variableId, &ScriptCanvas::VariableRequests::GetType);

        if (scriptCanvasType.IsValid())
        {
            AZ::Uuid azType = ScriptCanvas::Data::ToAZType(scriptCanvasType);

            AZStd::string colorPalette;
            GraphCanvas::StyleManagerRequestBus::EventResult(colorPalette, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetDataPaletteStyle, azType);

            SetTitlePalette(colorPalette);
        }
    }

    GetVariableNodePaletteTreeItem::~GetVariableNodePaletteTreeItem()
    {
        ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
    }

    void GetVariableNodePaletteTreeItem::OnVariableRenamed(AZStd::string_view variableName)
    {
        AZStd::string fullName = AZStd::string::format("Get %s", variableName.data());
        SetName(fullName.c_str());

        AZStd::string tooltip = AZStd::string::format("This node returns %s's values", variableName.data());
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
            serializeContext->Class<CreateSetVariableNodeMimeEvent, CreateNodeMimeEvent>()
                ->Version(0)
                ->Field("VariableId", &CreateSetVariableNodeMimeEvent::m_variableId)
                ;
        }
    }

    CreateSetVariableNodeMimeEvent::CreateSetVariableNodeMimeEvent(const ScriptCanvas::VariableId& variableId)
        : m_variableId(variableId)
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateSetVariableNodeMimeEvent::CreateNode(const AZ::EntityId& scriptCanvasGraphId) const
    {
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropObject, "{D855EE9C-74E0-4760-AA0F-239ADF7507B6}", scriptCanvasGraphId);
        return Nodes::CreateSetVariableNode(m_variableId, scriptCanvasGraphId);
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

    SetVariableNodePaletteTreeItem::SetVariableNodePaletteTreeItem()
        : GraphCanvas::DraggableNodePaletteTreeItem("Set Variable", ScriptCanvasEditor::AssetEditorId)
    {
        SetToolTip("This node changes a variable's values according to the data connected to the input slots");
    }

    SetVariableNodePaletteTreeItem::SetVariableNodePaletteTreeItem(const ScriptCanvas::VariableId& variableId, const AZ::EntityId& scriptCanvasGraphId)
        : GraphCanvas::DraggableNodePaletteTreeItem("", ScriptCanvasEditor::AssetEditorId)
        , m_variableId(variableId)
    {
        AZStd::string_view variableName;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableName, scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableName, variableId);
        OnVariableRenamed(variableName);

        ScriptCanvas::VariableNotificationBus::Handler::BusConnect(m_variableId);

        ScriptCanvas::Data::Type scriptCanvasType = ScriptCanvas::Data::Type::Invalid();
        ScriptCanvas::VariableRequestBus::EventResult(scriptCanvasType, m_variableId, &ScriptCanvas::VariableRequests::GetType);

        if (scriptCanvasType.IsValid())
        {
            AZ::Uuid azType = ScriptCanvas::Data::ToAZType(scriptCanvasType);

            AZStd::string colorPalette;
            GraphCanvas::StyleManagerRequestBus::EventResult(colorPalette, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetDataPaletteStyle, azType);

            SetTitlePalette(colorPalette);
        }
    }

    SetVariableNodePaletteTreeItem::~SetVariableNodePaletteTreeItem()
    {
        ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
    }

    void SetVariableNodePaletteTreeItem::OnVariableRenamed(AZStd::string_view variableName)
    {
        AZStd::string fullName = AZStd::string::format("Set %s", variableName.data());
        SetName(fullName.c_str());

        AZStd::string tooltip = AZStd::string::format("This node changes %s's values according to the data connected to the input slots", variableName.data());
        SetToolTip(tooltip.c_str());
    }

    GraphCanvas::GraphCanvasMimeEvent* SetVariableNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateSetVariableNodeMimeEvent(m_variableId);
    }

    ////////////////////////////////////////
    // CreateGetOrSetVariableNodeMimeEvent
    ////////////////////////////////////////

    void CreateGetOrSetVariableNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateGetOrSetVariableNodeMimeEvent, SpecializedCreateNodeMimeEvent>()
                ->Version(0)
                ->Field("VariableId", &CreateGetOrSetVariableNodeMimeEvent::m_variableId)
                ;
        }
    }

    CreateGetOrSetVariableNodeMimeEvent::CreateGetOrSetVariableNodeMimeEvent(const ScriptCanvas::VariableId& variableId)
        : m_variableId(variableId)
    {
    }

    bool CreateGetOrSetVariableNodeMimeEvent::ExecuteEvent(const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId)
    {
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropHandler, AZ::Uuid("{CE31F6F6-1536-4C97-BB59-863408ABA736}"), graphCanvasGraphId);

        NodeIdPair nodeId = ConstructNode(graphCanvasGraphId, sceneDropPosition);

        if (nodeId.m_graphCanvasId.IsValid())
        {
            AZ::EntityId gridId;
            GraphCanvas::SceneRequestBus::EventResult(gridId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

            AZ::Vector2 offset;
            GraphCanvas::GridRequestBus::EventResult(offset, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

            sceneDropPosition += offset;
        }

        return nodeId.m_graphCanvasId.IsValid();
    }

    ScriptCanvasEditor::NodeIdPair CreateGetOrSetVariableNodeMimeEvent::ConstructNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition)
    {
        Metrics::MetricsEventsBus::Broadcast(&Metrics::MetricsEventRequests::SendNodeMetric, ScriptCanvasEditor::Metrics::Events::Canvas::DropObject, "{924C1192-C32A-4A35-B146-2739AB4383DB}", graphCanvasGraphId);

        AZ::EntityId scriptCanvasGraphId;
        GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasGraphId, graphCanvasGraphId);

        ScriptCanvasEditor::NodeIdPair nodeIdPair;

        AZ::EntityId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

        GraphCanvas::GraphCanvasGraphicsView* graphicsView = nullptr;
        GraphCanvas::ViewRequestBus::EventResult(graphicsView, viewId, &GraphCanvas::ViewRequests::AsGraphicsView);

        if (graphicsView)
        {
            AZStd::string variableName;
            ScriptCanvas::VariableRequestBus::EventResult(variableName, m_variableId, &ScriptCanvas::VariableRequests::GetName);

            QMenu menu(graphicsView);

            QAction* createGet = new QAction(QString("Get %1").arg(variableName.c_str()), &menu);
            menu.addAction(createGet);

            QAction* createSet = new QAction(QString("Set %1").arg(variableName.c_str()), &menu);
            menu.addAction(createSet);

            QAction* result = menu.exec(QCursor::pos());

            if (result == createGet)
            {
                CreateGetVariableNodeMimeEvent createGetVariableNode(m_variableId);
                nodeIdPair = createGetVariableNode.CreateNode(scriptCanvasGraphId);
            }
            else if (result == createSet)
            {
                CreateSetVariableNodeMimeEvent createSetVariableNode(m_variableId);
                nodeIdPair = createSetVariableNode.CreateNode(scriptCanvasGraphId);
            }

            if (nodeIdPair.m_graphCanvasId.IsValid() && nodeIdPair.m_scriptCanvasId.IsValid())
            {
                GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, nodeIdPair.m_graphCanvasId, scenePosition);
                GraphCanvas::SceneMemberUIRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);
            }
        }

        return nodeIdPair;
    }

    ////////////////////////////////////////
    // VariableCategoryNodePaletteTreeItem
    ////////////////////////////////////////

    VariableCategoryNodePaletteTreeItem::VariableCategoryNodePaletteTreeItem(const QString& displayName)
        : NodePaletteTreeItem(displayName, ScriptCanvasEditor::AssetEditorId)
    {
    }

    void VariableCategoryNodePaletteTreeItem::PreOnChildAdded(GraphCanvasTreeItem* item)
    {
        // Force elements to display in the order they were added rather then alphabetical.
        static_cast<NodePaletteTreeItem*>(item)->SetItemOrdering(GetChildCount());
    }

    //////////////////////////////////////////
    // LocalVariablesListNodePaletteTreeItem
    //////////////////////////////////////////

    LocalVariablesListNodePaletteTreeItem::LocalVariablesListNodePaletteTreeItem(const QString& displayName)
        : NodePaletteTreeItem(displayName, ScriptCanvasEditor::AssetEditorId)
    {
        GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);
    }

    void LocalVariablesListNodePaletteTreeItem::OnActiveGraphChanged(const AZ::EntityId& graphCanvasGraphId)
    {
        AZ::EntityId scriptCanvasGraphId;
        GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasGraphId, graphCanvasGraphId);

        if (m_scriptCanvasGraphId != scriptCanvasGraphId)
        {
            GraphItemCommandNotificationBus::Handler::BusDisconnect(m_scriptCanvasGraphId);
            ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusDisconnect(m_scriptCanvasGraphId);
            m_scriptCanvasGraphId = scriptCanvasGraphId;

            if (m_scriptCanvasGraphId.IsValid())
            {
                ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusConnect(m_scriptCanvasGraphId);
                GraphItemCommandNotificationBus::Handler::BusConnect(m_scriptCanvasGraphId);
            }

            RefreshVariableList();
        }
    }

    void LocalVariablesListNodePaletteTreeItem::PostRestore(const UndoData&)
    {
        RefreshVariableList();
    }

    void LocalVariablesListNodePaletteTreeItem::OnVariableAddedToGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view /*variableName*/)
    {
        LocalVariableNodePaletteTreeItem* localVariableTreeItem = CreateChildNode<LocalVariableNodePaletteTreeItem>(variableId, m_scriptCanvasGraphId);
        localVariableTreeItem->PopulateChildren();
    }

    void LocalVariablesListNodePaletteTreeItem::OnVariableRemovedFromGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view /*variableName*/)
    {
        int rows = GetChildCount();

        for (int i = 0; i < rows; ++i)
        {
            LocalVariableNodePaletteTreeItem* treeItem = static_cast<LocalVariableNodePaletteTreeItem*>(FindChildByRow(i));

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

        const ScriptCanvas::GraphVariableMapping* variableMapping = nullptr;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableMapping, m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::GetVariables);

        if (variableMapping != nullptr)
        {
            for (const auto& mapPair : (*variableMapping))
            {
                LocalVariableNodePaletteTreeItem* rootItem = this->CreateChildNode<LocalVariableNodePaletteTreeItem>(mapPair.first, m_scriptCanvasGraphId);
                rootItem->PopulateChildren();
            }
        }

        UnblockSignals();
        SignalLayoutChanged();
    }

    /////////////////////////////////////
    // LocalVariableNodePaletteTreeItem
    /////////////////////////////////////

    LocalVariableNodePaletteTreeItem::LocalVariableNodePaletteTreeItem(ScriptCanvas::VariableId variableId, const AZ::EntityId& scriptCanvasGraphId)
        : NodePaletteTreeItem("", ScriptCanvasEditor::AssetEditorId)
        , m_scriptCanvasGraphId(scriptCanvasGraphId)
        , m_variableId(variableId)
    {
        AZStd::string_view variableName;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableName, m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableName, variableId);
        OnVariableRenamed(variableName);

        ScriptCanvas::VariableNotificationBus::Handler::BusConnect(variableId);
    }

    LocalVariableNodePaletteTreeItem::~LocalVariableNodePaletteTreeItem()
    {
        ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
    }

    void LocalVariableNodePaletteTreeItem::PopulateChildren()
    {
        if (GetChildCount() == 0)
        {
            SignalLayoutAboutToBeChanged();
            BlockSignals();
            CreateChildNode<GetVariableNodePaletteTreeItem>(GetVariableId(), m_scriptCanvasGraphId);
            CreateChildNode<SetVariableNodePaletteTreeItem>(GetVariableId(), m_scriptCanvasGraphId);
            UnblockSignals();
            SignalLayoutChanged();
        }
    }

    const ScriptCanvas::VariableId& LocalVariableNodePaletteTreeItem::GetVariableId() const
    {
        return m_variableId;
    }

    void LocalVariableNodePaletteTreeItem::OnVariableRenamed(AZStd::string_view variableName)
    {
        SetName(variableName.data());
    }
}