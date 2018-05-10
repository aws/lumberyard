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

#include <ScriptCanvas/Bus/UndoBus.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>

namespace ScriptCanvasEditor
{
    class ScopedUndoBatch
    {
    public:
        ScopedUndoBatch(AZStd::string_view undoLabel);
        ~ScopedUndoBatch();

    private:
        ScopedUndoBatch(const ScopedUndoBatch&) = delete;
        ScopedUndoBatch& operator=(const ScopedUndoBatch&) = delete;
    };

    class UndoCache;

    class SceneUndoState
    {
    public:
        AZ_CLASS_ALLOCATOR(SceneUndoState, AZ::SystemAllocator, 0);

        SceneUndoState(AzToolsFramework::UndoSystem::IUndoNotify* undoNotify);
        ~SceneUndoState();

        void BeginUndoBatch(AZStd::string_view label);
        void EndUndoBatch();

        AZStd::unique_ptr<UndoCache> m_undoCache;
        AZStd::unique_ptr<AzToolsFramework::UndoSystem::UndoStack> m_undoStack;

        AzToolsFramework::UndoSystem::URSequencePoint* m_currentUndoBatch = nullptr;
    };

    class UndoManager
        : public UndoRequestBus::Handler
        , public AzToolsFramework::UndoSystem::IUndoNotify
        , public GraphCanvas::AssetEditorNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(UndoManager, AZ::SystemAllocator, 0);

        UndoManager();
        ~UndoManager();

        UndoCache* GetActiveSceneUndoCache() override;
        UndoCache* GetSceneUndoCache(AZ::EntityId scriptCanvasGraphId);

        AZStd::unique_ptr<SceneUndoState> ExtractSceneUndoState(AZ::EntityId scriptCanvasGraphId);
        void InsertUndoState(AZ::EntityId scriptCanvasGraphId, AZStd::unique_ptr<SceneUndoState> sceneUndoState);

        void BeginUndoBatch(AZStd::string_view label) override;
        void EndUndoBatch() override;

        void AddUndo(AzToolsFramework::UndoSystem::URSequencePoint* sequencePoint) override;

        void AddGraphItemChangeUndo(AZ::Entity* scriptCanvasEntity, AZStd::string_view undoLabel) override;
        void AddGraphItemAdditionUndo(AZ::Entity* scriptCanvasEntity, AZStd::string_view undoLabel) override;
        void AddGraphItemRemovalUndo(AZ::Entity* scriptCanvasEntity, AZStd::string_view undoLabel) override;

        void Undo() override;
        void Redo() override;
        void Reset() override;

        UndoData CreateUndoData(AZ::EntityId scriptCanvasEntityId) override;

        bool IsInUndoRedo() const { return m_isInUndo; }

        //! IUndoNotify
        void OnUndoStackChanged() override;
        ////
        
        // GraphCanvas::GraphCanvasEditorNotificationBus
        void OnGraphLoaded(const GraphCanvas::GraphId& graphCanvasGraphId) override;
        void OnGraphUnloaded(const GraphCanvas::GraphId& graphCanvasGraphId) override;
        void OnGraphRefreshed(const GraphCanvas::GraphId& oldGraphCanvasGraphId, const GraphCanvas::GraphId& newGraphCanvasGraphId) override;

        void OnActiveGraphChanged(const GraphCanvas::GraphId& activeGraphCanvasGraph) override;
        ////

    protected:

        SceneUndoState* FindActiveUndoState() const;
        SceneUndoState* FindUndoState(const GraphCanvas::GraphId& sceneId) const;

        GraphCanvas::GraphId m_activeGraphCanvasGraphId;

        AZStd::unordered_map< GraphCanvas::GraphId, SceneUndoState* > m_undoMapping;

        bool m_isInUndo = false;
        bool m_canUndo = false;
        bool m_canRedo = false;
    };
}