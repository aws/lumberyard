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

#include <GraphCanvas/Editor/AssetEditorBus.h>

#include <GraphCanvas/Widgets/NodePalette/TreeItems/DraggableNodePaletteTreeItem.h>
#include "CreateNodeMimeEvent.h"

#include "Editor/Undo/ScriptCanvasGraphCommand.h"

#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvasEditor
{
    // <GetVariableNode>
    class CreateGetVariableNodeMimeEvent
        : public CreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateGetVariableNodeMimeEvent, "{A9784FF3-E749-4EB4-B5DB-DF510F7CD151}", CreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateGetVariableNodeMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateGetVariableNodeMimeEvent() = default;
        explicit CreateGetVariableNodeMimeEvent(const ScriptCanvas::VariableId& variableId);
        ~CreateGetVariableNodeMimeEvent() = default;

        ScriptCanvasEditor::NodeIdPair CreateNode(const AZ::EntityId& graphId) const override;

    private:
        ScriptCanvas::VariableId m_variableId;
    };

    class GetVariableNodePaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
        , public ScriptCanvas::VariableNotificationBus::Handler
    {
    public:
        AZ_RTTI(GetVariableNodePaletteTreeItem, "{0589E084-2E57-4650-96BF-E42DA17D7731}", GraphCanvas::DraggableNodePaletteTreeItem)
        AZ_CLASS_ALLOCATOR(GetVariableNodePaletteTreeItem, AZ::SystemAllocator, 0);
        static const QString& GetDefaultIcon();

        GetVariableNodePaletteTreeItem();
        GetVariableNodePaletteTreeItem(const ScriptCanvas::VariableId& variableId, const AZ::EntityId& scriptCanvasGraphId);
        ~GetVariableNodePaletteTreeItem();

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;

        // VariableNotificationBus::Handler
        void OnVariableRenamed(AZStd::string_view variableName) override;
        ////

        const ScriptCanvas::VariableId& GetVariableId() const;

    private:
        ScriptCanvas::VariableId m_variableId;
    };
    // </GetVariableNode>

    // <SetVariableNode>
    class CreateSetVariableNodeMimeEvent
        : public CreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateSetVariableNodeMimeEvent, "{D855EE9C-74E0-4760-AA0F-239ADF7507B6}", CreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateSetVariableNodeMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateSetVariableNodeMimeEvent() = default;
        explicit CreateSetVariableNodeMimeEvent(const ScriptCanvas::VariableId& variableId);
        ~CreateSetVariableNodeMimeEvent() = default;

        ScriptCanvasEditor::NodeIdPair CreateNode(const AZ::EntityId& graphId) const override;        

    private:
        ScriptCanvas::VariableId m_variableId;
    };

    class SetVariableNodePaletteTreeItem
        : public GraphCanvas::DraggableNodePaletteTreeItem
        , public ScriptCanvas::VariableNotificationBus::Handler
    {
    public:
        AZ_RTTI(SetVariableNodePaletteTreeItem, "{BCFD5653-6621-4BAC-BD8E-71EC6190062F}", GraphCanvas::DraggableNodePaletteTreeItem)
        AZ_CLASS_ALLOCATOR(SetVariableNodePaletteTreeItem, AZ::SystemAllocator, 0);
        static const QString& GetDefaultIcon();

        SetVariableNodePaletteTreeItem();
        SetVariableNodePaletteTreeItem(const ScriptCanvas::VariableId& variableId, const AZ::EntityId& scriptCanvasGraphId);
        ~SetVariableNodePaletteTreeItem();

        // VariableNotificationBus::Handler
        void OnVariableRenamed(AZStd::string_view variableName) override;
        ////

        const ScriptCanvas::VariableId& GetVariableId() const;

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;
    private:
        ScriptCanvas::VariableId m_variableId;
    };
    // </GetVariableNode>

    // <GetOrSetVariableNodeMimeEvent>
    class CreateGetOrSetVariableNodeMimeEvent
        : public MultiCreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateGetOrSetVariableNodeMimeEvent, "{924C1192-C32A-4A35-B146-2739AB4383DB}", MultiCreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateGetOrSetVariableNodeMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateGetOrSetVariableNodeMimeEvent() = default;
        explicit CreateGetOrSetVariableNodeMimeEvent(const ScriptCanvas::VariableId& variableId);
        ~CreateGetOrSetVariableNodeMimeEvent() = default;

        bool ExecuteEvent(const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId) override;
        ScriptCanvasEditor::NodeIdPair ConstructNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition) override;

        AZStd::vector< GraphCanvas::GraphCanvasMimeEvent* > CreateMimeEvents() const override;

    private:

        ScriptCanvas::VariableId m_variableId;
    };
    // </GetOrSetVariableNodeMimeEvent>

    // <VariableCategoryNodePaletteTreeItem>
    class VariableCategoryNodePaletteTreeItem
        : public GraphCanvas::NodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(VariableCategoryNodePaletteTreeItem, AZ::SystemAllocator, 0);
        VariableCategoryNodePaletteTreeItem(AZStd::string_view displayName);
        ~VariableCategoryNodePaletteTreeItem() = default;

    private:

        void PreOnChildAdded(GraphCanvasTreeItem* item) override;
    };
    // </VariableNodePaeltteTreeItem>

    // <LocalVariablesListNodePaletteTreeItem>
    class LocalVariablesListNodePaletteTreeItem
        : public GraphCanvas::NodePaletteTreeItem
        , public GraphCanvas::AssetEditorNotificationBus::Handler
        , public ScriptCanvas::GraphVariableManagerNotificationBus::Handler
        , public GraphItemCommandNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(LocalVariablesListNodePaletteTreeItem, AZ::SystemAllocator, 0);
        LocalVariablesListNodePaletteTreeItem(AZStd::string_view displayName);
        ~LocalVariablesListNodePaletteTreeItem() = default;

        // GraphCanvas::AssetEditorNotificationBus
        void OnActiveGraphChanged(const GraphCanvas::GraphId& graphCanvasGraphId) override;
        ////

        // GraphItemCommandNotificationBus
        void PostRestore(const UndoData& undoData) override;
        ////

        // GraphVariableManagerNotificationBus
        void OnVariableAddedToGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override;
        void OnVariableRemovedFromGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override;
        ////

    private:

        void RefreshVariableList();

        AZ::EntityId m_scriptCanvasGraphId;
    };
    // </LocalVariablesNodePaletteTreeItem>

    // <LocalVariableNodePaletteTreeItem>
    class LocalVariableNodePaletteTreeItem
        : public GraphCanvas::NodePaletteTreeItem
        , public ScriptCanvas::VariableNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(LocalVariableNodePaletteTreeItem, AZ::SystemAllocator, 0);
        LocalVariableNodePaletteTreeItem(ScriptCanvas::VariableId variableTreeItem, const AZ::EntityId& scriptCanvasGraphId);
        ~LocalVariableNodePaletteTreeItem();

        void PopulateChildren();
        const ScriptCanvas::VariableId& GetVariableId() const;

        // VariableNotificationBus
        void OnVariableRenamed(AZStd::string_view) override;
        ////

    private:
        AZ::EntityId             m_scriptCanvasGraphId;
        ScriptCanvas::VariableId m_variableId;
    };

    // </VariableNodePaletteTreeItem>
}