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

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/NodeGroupMenuActions/NodeGroupContextMenuAction.h>

namespace GraphCanvas
{
    class CreateNodeGroupMenuAction
        : public NodeGroupContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateNodeGroupMenuAction, AZ::SystemAllocator, 0);

        CreateNodeGroupMenuAction(QObject* parent, bool collapseGroup);
        virtual ~CreateNodeGroupMenuAction() = default;

        void RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId) override;
        SceneReaction TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos) override;

    private:
        bool m_collapseGroup;
    };

    class CreateNewNodeGroupMenuAction
        : public CreateNodeGroupMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CreateNewNodeGroupMenuAction, AZ::SystemAllocator, 0);
        CreateNewNodeGroupMenuAction(QObject* parent);

        void RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId) override;
    };

    class UngroupNodeGroupMenuAction
        : public NodeGroupContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(UngroupNodeGroupMenuAction, AZ::SystemAllocator, 0);

        UngroupNodeGroupMenuAction(QObject* parent);
        virtual ~UngroupNodeGroupMenuAction() = default;

        void RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId) override;
        SceneReaction TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos) override;

    private:
        AZ::EntityId m_groupTarget;
    };

    class CollapseNodeGroupMenuAction
        : public NodeGroupContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(CollapseNodeGroupMenuAction, AZ::SystemAllocator, 0);

        CollapseNodeGroupMenuAction(QObject* parent);
        virtual ~CollapseNodeGroupMenuAction() = default;

        void RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId) override;
        SceneReaction TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos) override;

    private:
        AZ::EntityId m_groupTarget;
    };

    class ExpandNodeGroupMenuAction
        : public NodeGroupContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(ExpandNodeGroupMenuAction, AZ::SystemAllocator, 0);

        ExpandNodeGroupMenuAction(QObject* parent);
        virtual ~ExpandNodeGroupMenuAction() = default;

        void RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId) override;
        SceneReaction TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos) override;

    private:
        AZ::EntityId m_groupTarget;
    };

    class EditGroupTitleMenuAction
        : public NodeGroupContextMenuAction
    {
    public:
        AZ_CLASS_ALLOCATOR(EditGroupTitleMenuAction, AZ::SystemAllocator, 0);

        EditGroupTitleMenuAction(QObject* parent);
        virtual ~EditGroupTitleMenuAction() = default;

        void RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId) override;
        SceneReaction TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos) override;

    private:

        AZ::EntityId m_groupTarget;
    };
}