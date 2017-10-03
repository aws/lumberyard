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

#include <GraphCanvas/Components/Nodes/Variable/VariableNodeBus.h>

#include "NodePaletteTreeItem.h"
#include "CreateNodeMimeEvent.h"

#include "Editor/View/Windows/MainWindowBus.h"
#include "Editor/Undo/ScriptCanvasGraphCommand.h"

namespace ScriptCanvasEditor
{   
    // <Variable Primitive>
    class CreateVariablePrimitiveNodeMimeEvent
        : public CreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateVariablePrimitiveNodeMimeEvent, "{AF8A8242-8E74-4EF4-975B-85CC54368D3A}", CreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateVariablePrimitiveNodeMimeEvent, AZ::SystemAllocator, 0);
        
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        CreateVariablePrimitiveNodeMimeEvent() = default;
        CreateVariablePrimitiveNodeMimeEvent(const AZ::Uuid& primitiveId);
        ~CreateVariablePrimitiveNodeMimeEvent() = default;

    protected:
        ScriptCanvasEditor::NodeIdPair CreateNode(const AZ::EntityId& graphId) const override;

    private:
        AZ::Uuid m_primitiveId;
    };
    
    class VariablePrimitiveNodePaletteTreeItem
        : public DraggableNodePaletteTreeItem
    {
    private:
        static const QString& GetDefaultIcon();
    public:
        AZ_CLASS_ALLOCATOR(VariablePrimitiveNodePaletteTreeItem, AZ::SystemAllocator, 0);
        VariablePrimitiveNodePaletteTreeItem(const AZ::Uuid& primitiveId, const QString& nodeName, const QString& iconPath);
        ~VariablePrimitiveNodePaletteTreeItem() = default;
        
        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;
        
    private:
        AZ::Uuid m_primitiveId;
    };
    // </Variable Primitive>
    
    // <Variable Object>
    class CreateVariableObjectNodeMimeEvent
        : public CreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateVariableObjectNodeMimeEvent, "{30BACA22-A976-42A2-ACA7-46C767EBDB3A}", CreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateVariableObjectNodeMimeEvent, AZ::SystemAllocator, 0);
        
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        CreateVariableObjectNodeMimeEvent() = default;
        CreateVariableObjectNodeMimeEvent(const QString& className);
        ~CreateVariableObjectNodeMimeEvent() = default;

    protected:
        ScriptCanvasEditor::NodeIdPair CreateNode(const AZ::EntityId& graphId) const override;

    private:
        AZStd::string m_className;
    };
    
    class VariableObjectNodePaletteTreeItem
        : public DraggableNodePaletteTreeItem
    {
    private:
        static const QString& GetDefaultIcon();
    public:
        AZ_CLASS_ALLOCATOR(VariableObjectNodePaletteTreeItem, AZ::SystemAllocator, 0);
        VariableObjectNodePaletteTreeItem(const QString& className);
        ~VariableObjectNodePaletteTreeItem() = default;
        
        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;
        
    private:
        QString m_className;
    };    
    // </Variable Object>

    // <GetVariableNode>
    class CreateGetVariableNodeMimeEvent
        : public SpecializedCreateNodeMimeEvent
    {
    public:
        AZ_RTTI(CreateGetVariableNodeMimeEvent, "{A9784FF3-E749-4EB4-B5DB-DF510F7CD151}", SpecializedCreateNodeMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateGetVariableNodeMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        CreateGetVariableNodeMimeEvent() = default;
        explicit CreateGetVariableNodeMimeEvent(const AZ::EntityId& variableId);
        ~CreateGetVariableNodeMimeEvent() = default;

        NodeIdPair ConstructNode(const AZ::EntityId& sceneId, const AZ::Vector2& sceneDropPosition) override;
        bool ExecuteEvent(const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& sceneId) override;

    private:
        AZ::EntityId m_variableId;
    };

    class GetVariableNodePaletteTreeItem
        : public DraggableNodePaletteTreeItem
        , public GraphCanvas::VariableNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(GetVariableNodePaletteTreeItem, AZ::SystemAllocator, 0);
        static const QString& GetDefaultIcon();

        GetVariableNodePaletteTreeItem(const QString& nodeName);
        GetVariableNodePaletteTreeItem(const AZ::EntityId& variableId);
        ~GetVariableNodePaletteTreeItem();

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;

        // VariableNotificationBus::Handler
        void OnNameChanged();
        ////

    private:
        AZ::EntityId m_variableId;
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
        explicit CreateSetVariableNodeMimeEvent(const AZ::EntityId& variableId);
        ~CreateSetVariableNodeMimeEvent() = default;

        ScriptCanvasEditor::NodeIdPair CreateNode(const AZ::EntityId& graphId) const override;

    private:
        AZ::EntityId m_variableId;
    };

    class SetVariableNodePaletteTreeItem
        : public DraggableNodePaletteTreeItem
        , public GraphCanvas::VariableNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(SetVariableNodePaletteTreeItem, AZ::SystemAllocator, 0);
        static const QString& GetDefaultIcon();

        SetVariableNodePaletteTreeItem(const QString& nodeName);
        SetVariableNodePaletteTreeItem(const AZ::EntityId& variableId);
        ~SetVariableNodePaletteTreeItem();

        // VariableNotificationBus::Handler
        void OnNameChanged() override;
        ////

        GraphCanvas::GraphCanvasMimeEvent* CreateMimeEvent() const override;
    private:
        AZ::EntityId m_variableId;
    };
    // </GetVariableNode>
    
    // <VariableCategoryNodePaletteTreeItem>
    class VariableCategoryNodePaletteTreeItem
        : public NodePaletteTreeItem
    {
    public:
        AZ_CLASS_ALLOCATOR(VariableCategoryNodePaletteTreeItem, AZ::SystemAllocator, 0);
        VariableCategoryNodePaletteTreeItem(const QString& displayName);
        ~VariableCategoryNodePaletteTreeItem() = default;
        
    private:
    
        void PreOnChildAdded(GraphCanvasTreeItem* item) override;
    };    
    // </VariableNodePaeltteTreeItem>
    
    // <LocalVariablesListNodePaletteTreeItem>
    class LocalVariablesListNodePaletteTreeItem
        : public NodePaletteTreeItem
        , public MainWindowNotificationBus::Handler
        , public GraphCanvas::SceneVariableNotificationBus::Handler
        , public GraphItemCommandNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(LocalVariablesListNodePaletteTreeItem, AZ::SystemAllocator, 0);
        LocalVariablesListNodePaletteTreeItem(const QString& displayName);
        ~LocalVariablesListNodePaletteTreeItem() = default;

        // MainWindowNotificationBus
        void OnActiveSceneChanged(const AZ::EntityId& sceneId) override;
        ////

        // GraphItemCommandNotificationBus
        void PostRestore(const UndoData& undoData) override;
        ////

        // SceneVariableNotificationBus
        void OnVariableCreated(const AZ::EntityId& variableId) override;
        void OnVariableDestroyed(const AZ::EntityId& variableId) override;
        ////

    private:

        void RefreshVariableList();

        AZ::EntityId m_sceneId;
    };
    // </LocalVariablesNodePaletteTreeItem>

    // <LocalVariableNodePaletteTreeItem>
    class LocalVariableNodePaletteTreeItem
        : public NodePaletteTreeItem
        , public GraphCanvas::VariableNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(LocalVariableNodePaletteTreeItem, AZ::SystemAllocator, 0);
        LocalVariableNodePaletteTreeItem(AZ::EntityId variableTreeItem);
        ~LocalVariableNodePaletteTreeItem();

        void PopulateChildren();
        const AZ::EntityId& GetVariableId() const;

        // VariableNotificationBus
        void OnNameChanged() override;
        ////

    private:
        AZ::EntityId m_variableId;
    };

    // </VariableNodePaletteTreeItem>
}