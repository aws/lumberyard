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

#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Vector2.h>

#include <QMenu>
#include <QWidgetAction>

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        class NodePaletteDockWidget;
    }

    class CreateNodeAction
        : public QAction
    {
        Q_OBJECT;
    protected:
        CreateNodeAction(const QString& text, QObject* parent);

    public:
        virtual ~CreateNodeAction() = default;

        virtual void RefreshAction(const AZ::EntityId& scriptCanvasSceneId) = 0;

        // Returns whether or not the action needs to create an Undo state after executing.
        virtual bool TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos) = 0;
    };

    class AddSelectedEntitiesAction
        : public CreateNodeAction
    {
        Q_OBJECT
    public:
        AddSelectedEntitiesAction(QObject* parent);
        virtual ~AddSelectedEntitiesAction() = default;

        void RefreshAction(const AZ::EntityId& scriptCanvasSceneId) override;
        bool TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos) override;
    };

    class CutGraphSelectionAction
        : public CreateNodeAction
    {
        Q_OBJECT
    public:
        CutGraphSelectionAction(QObject* parent);
        virtual ~CutGraphSelectionAction() = default;

        void RefreshAction(const AZ::EntityId& scriptCanvasSceneId) override;
        bool TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos) override;
    };

    class CopyGraphSelectionAction
        : public CreateNodeAction
    {
        Q_OBJECT
    public:
        CopyGraphSelectionAction(QObject* parent);
        virtual ~CopyGraphSelectionAction() = default;

        void RefreshAction(const AZ::EntityId& scriptCanvasSceneId) override;
        bool TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos) override;
    };

    class PasteGraphSelectionAction
        : public CreateNodeAction
    {
        Q_OBJECT
    public:
        PasteGraphSelectionAction(QObject* parent);
        virtual ~PasteGraphSelectionAction() = default;

        void RefreshAction(const AZ::EntityId& scriptCanvasSceneId) override;
        bool TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos) override;
    };

    class DuplicateGraphSelectionAction
        : public CreateNodeAction
    {
        Q_OBJECT
    public:
        DuplicateGraphSelectionAction(QObject* parent);
        virtual ~DuplicateGraphSelectionAction() = default;

        void RefreshAction(const AZ::EntityId& scriptCanvasSceneId) override;
        bool TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos) override;
    };

    class DeleteGraphSelectionAction
        : public CreateNodeAction
    {
        Q_OBJECT
    public:
        DeleteGraphSelectionAction(QObject* parent);
        virtual ~DeleteGraphSelectionAction() = default;

        void RefreshAction(const AZ::EntityId& scriptCanvasSceneId) override;
        bool TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos) override;
    };

    class AddCommentAction
        : public CreateNodeAction
    {
        Q_OBJECT
    public:
        AddCommentAction(QObject* parent);
        virtual ~AddCommentAction() = default;

        void RefreshAction(const AZ::EntityId& scriptCanvasSceneId) override;
        bool TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos) override;
    };

    class EndpointSelectionAction
        : public QAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(EndpointSelectionAction, AZ::SystemAllocator, 0);

        EndpointSelectionAction(const GraphCanvas::Endpoint& endpoint);
        ~EndpointSelectionAction() = default;

        const GraphCanvas::Endpoint& GetEndpoint() const;

    private:
        GraphCanvas::Endpoint m_endpoint;
    };

    class CreateBlockCommentAction
        : public CreateNodeAction
    {
        Q_OBJECT
    public:
        CreateBlockCommentAction(QObject* parent);
        virtual ~CreateBlockCommentAction() = default;

        void RefreshAction(const AZ::EntityId& scriptCanvasSceneId) override;
        bool TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos) override;
    };

    class AddBookmarkAction
        : public CreateNodeAction
    {
        Q_OBJECT
    public:
        AddBookmarkAction(QObject* parent);
        virtual ~AddBookmarkAction() = default;

        void RefreshAction(const AZ::EntityId& scriptCanvasSceneId) override;
        bool TriggerAction(const AZ::EntityId& scriptCanvasSceneId, const AZ::Vector2& scenePos) override;
    };

    class CreateNodeContextMenu
        : public QMenu
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(CreateNodeContextMenu, AZ::SystemAllocator, 0);

        CreateNodeContextMenu();
        ~CreateNodeContextMenu() = default;

        void DisableCreateActions();
        void RefreshActions(const AZ::EntityId& scriptCanvasSceneId);

        void ResetSourceSlotFilter();
        void FilterForSourceSlot(const AZ::EntityId& scriptCanvasGraphId, const AZ::EntityId& sourceSlotId);
        const Widget::NodePaletteDockWidget* GetNodePalette() const;

    public slots:

        void HandleContextMenuSelection();
        void SetupDisplay();

    protected:
        void keyPressEvent(QKeyEvent* keyEvent) override;
        
        AZ::EntityId            m_sourceSlotId;
        Widget::NodePaletteDockWidget*    m_palette;
    };
}
