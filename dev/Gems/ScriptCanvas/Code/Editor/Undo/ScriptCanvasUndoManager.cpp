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

#include <precompiled.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Undo/ScriptCanvasUndoManager.h>
#include <Editor/Undo/ScriptCanvasUndoCache.h>
#include <Editor/Undo/ScriptCanvasGraphCommand.h>
#include <ScriptCanvas/Bus/UndoBus.h>
#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>

namespace ScriptCanvasEditor
{
    static const int c_undoLimit = 100;

    ScopedUndoBatch::ScopedUndoBatch(AZStd::string_view label)
    {
        UndoRequestBus::Broadcast(&UndoRequests::BeginUndoBatch, label);
    }

    ScopedUndoBatch::~ScopedUndoBatch()
    {
        UndoRequestBus::Broadcast(&UndoRequests::EndUndoBatch);
    }

    SceneUndoState::SceneUndoState(AzToolsFramework::UndoSystem::IUndoNotify* undoNotify)
        : m_undoStack(AZStd::make_unique<AzToolsFramework::UndoSystem::UndoStack>(c_undoLimit, undoNotify))
        , m_undoCache(AZStd::make_unique<UndoCache>())
    {
    }

    void SceneUndoState::BeginUndoBatch(AZStd::string_view label)
    {
        if (!m_currentUndoBatch)
        {
            m_currentUndoBatch = aznew AzToolsFramework::UndoSystem::BatchCommand(label, 0);
        }
        else
        {
            auto parentUndoBatch = m_currentUndoBatch;

            m_currentUndoBatch = aznew AzToolsFramework::UndoSystem::BatchCommand(label, 0);
            m_currentUndoBatch->SetParent(parentUndoBatch);
        }
    }

    void SceneUndoState::EndUndoBatch()
    {
        if (!m_currentUndoBatch)
        {
            return;
        }

        if (m_currentUndoBatch->GetParent())
        {
            // pop one up
            m_currentUndoBatch = m_currentUndoBatch->GetParent();
        }
        else
        {
            // we're at root
            if (m_currentUndoBatch->HasRealChildren() && m_undoStack)
            {
                m_undoStack->Post(m_currentUndoBatch);
            }
            else
            {
                delete m_currentUndoBatch;
            }

            m_currentUndoBatch = nullptr;
        }
    }

    SceneUndoState::~SceneUndoState()
    {
        delete m_currentUndoBatch;
    }

    UndoManager::UndoManager()
    {
        GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);
        UndoRequestBus::Handler::BusConnect();
    }

    UndoManager::~UndoManager()
    {
        UndoRequestBus::Handler::BusDisconnect();
        GraphCanvas::AssetEditorNotificationBus::Handler::BusDisconnect();

        for (auto& mapPair : m_undoMapping)
        {
            delete mapPair.second;
        }
    }

    UndoCache* UndoManager::GetActiveSceneUndoCache()
    {
        SceneUndoState* sceneUndoState = FindActiveUndoState();

        if (sceneUndoState)
        {
            return sceneUndoState->m_undoCache.get();
        }

        return nullptr;
    }

    UndoCache* UndoManager::GetSceneUndoCache(AZ::EntityId scriptCanvasGraphId)
    {
        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

        SceneUndoState* sceneUndoState = FindUndoState(graphCanvasGraphId);

        if (sceneUndoState)
        {
            return sceneUndoState->m_undoCache.get();
        }

        return nullptr;
    }

    void UndoManager::BeginUndoBatch(AZStd::string_view label)
    {
        SceneUndoState* sceneUndoState = FindActiveUndoState();

        if (sceneUndoState)
        {
            sceneUndoState->BeginUndoBatch(label);
        }
    }

    void UndoManager::EndUndoBatch()
    {
        SceneUndoState* sceneUndoState = FindActiveUndoState();

        if (sceneUndoState)
        {
            sceneUndoState->EndUndoBatch();
        }
    }

    void UndoManager::Redo()
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();
        SceneUndoState* sceneUndoState = FindActiveUndoState();

        if (sceneUndoState)
        {
            AZ_Warning("Script Canvas", !sceneUndoState->m_currentUndoBatch, "Script Canvas Editor has an open undo batch when performing a redo operation");

            if (sceneUndoState->m_undoStack->CanRedo())
            {
                m_isInUndo = true;
                sceneUndoState->m_undoStack->Redo();
                m_isInUndo = false;
            }
        }
    }

    void UndoManager::Undo()
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();
        SceneUndoState* sceneUndoState = FindActiveUndoState();

        if (sceneUndoState)
        {
            AZ_Warning("Script Canvas", !sceneUndoState->m_currentUndoBatch, "Script Canvas Editor has an open undo batch when performing an undo operation");

            if (sceneUndoState->m_undoStack->CanUndo())
            {
                m_isInUndo = true;
                sceneUndoState->m_undoStack->Undo();
                m_isInUndo = false;
            }
        }
    }

    void UndoManager::Reset()
    {
        SceneUndoState* sceneUndoState = FindActiveUndoState();

        if (sceneUndoState)
        {
            AZ_Warning("Script Canvas", !sceneUndoState->m_currentUndoBatch, "Script Canvas Editor has an open undo batch when reseting the undo stack");
            sceneUndoState->m_undoStack->Reset();
        }
    }


    void UndoManager::AddUndo(AzToolsFramework::UndoSystem::URSequencePoint* sequencePoint)
    {
        SceneUndoState* sceneUndoState = FindActiveUndoState();

        if (sceneUndoState)
        {
            if (!sceneUndoState->m_currentUndoBatch)
            {
                sceneUndoState->m_undoStack->Post(sequencePoint);
            }
            else
            {
                sequencePoint->SetParent(sceneUndoState->m_currentUndoBatch);
            }
        }
    }

    void UndoManager::AddGraphItemChangeUndo(AZ::Entity* scriptCanvasEntity, AZStd::string_view undoLabel)
    {
        auto command = aznew GraphItemChangeCommand(undoLabel);
        command->Capture(scriptCanvasEntity, true);
        command->Capture(scriptCanvasEntity, false);
        AddUndo(command);
    }

    void UndoManager::AddGraphItemAdditionUndo(AZ::Entity* scriptCanvasEntity, AZStd::string_view undoLabel)
    {
        auto command = aznew GraphItemAddCommand(undoLabel);
        command->Capture(scriptCanvasEntity, false);
        AddUndo(command);
    }

    void UndoManager::AddGraphItemRemovalUndo(AZ::Entity* scriptCanvasEntity, AZStd::string_view undoLabel)
    {
        auto command = aznew GraphItemRemovalCommand(undoLabel);
        command->Capture(scriptCanvasEntity, true);
        AddUndo(command);
    }

    UndoData UndoManager::CreateUndoData(AZ::EntityId scriptCanvasEntityId)
    {
        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasEntityId, &EditorGraphRequests::GetGraphCanvasGraphId);

        GraphCanvas::GraphModelRequestBus::Event(graphCanvasGraphId, &GraphCanvas::GraphModelRequests::OnSaveDataDirtied, scriptCanvasEntityId);
        GraphCanvas::GraphModelRequestBus::Event(graphCanvasGraphId, &GraphCanvas::GraphModelRequests::OnSaveDataDirtied, graphCanvasGraphId);

        UndoData undoData;

        ScriptCanvas::GraphData* graphData{};
        ScriptCanvas::GraphRequestBus::EventResult(graphData, scriptCanvasEntityId, &ScriptCanvas::GraphRequests::GetGraphData);

        const ScriptCanvas::VariableData* varData{};
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(varData, scriptCanvasEntityId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableDataConst);
        
        if (graphData && varData)
        {
            undoData.m_graphData = *graphData;
            undoData.m_variableData = *varData;

            EditorGraphRequestBus::EventResult(undoData.m_visualSaveData, scriptCanvasEntityId, &EditorGraphRequests::GetGraphCanvasSaveData);
        }

        return undoData;
    }

    void UndoManager::OnUndoStackChanged()
    {
        SceneUndoState* undoState = FindActiveUndoState();

        m_canUndo = false;
        m_canRedo = false;

        if (undoState)
        {
            m_canUndo = undoState->m_undoStack->CanUndo();
            m_canRedo = undoState->m_undoStack->CanRedo();
        }

        EBUS_EVENT(UndoNotificationBus, UndoNotifications::OnCanUndoChanged, m_canUndo);
        EBUS_EVENT(UndoNotificationBus, UndoNotifications::OnCanRedoChanged, m_canRedo);
    }

    void UndoManager::OnActiveGraphChanged(const GraphCanvas::GraphId& graphCanvasGraphId)
    {
        m_activeGraphCanvasGraphId.SetInvalid();
        m_activeGraphCanvasGraphId = graphCanvasGraphId;
        
        OnUndoStackChanged();
    }

    void UndoManager::OnGraphLoaded(const GraphCanvas::GraphId& graphCanvasGraphId)
    {
        AZ::EntityId scriptCanvasGraphId;
        GeneralRequestBus::BroadcastResult(scriptCanvasGraphId, &GeneralRequests::GetScriptCanvasGraphId, graphCanvasGraphId);

        if (!scriptCanvasGraphId.IsValid())
        {
            return;
        }

        auto mapIter = m_undoMapping.find(graphCanvasGraphId);

        if (mapIter == m_undoMapping.end())
        {
            m_undoMapping[graphCanvasGraphId] = aznew SceneUndoState(this);
        }
    }

    void UndoManager::OnGraphUnloaded(const GraphCanvas::GraphId& graphCanvasGraphId)
    {
        auto mapIter = m_undoMapping.find(graphCanvasGraphId);

        if (mapIter != m_undoMapping.end())
        {
            delete mapIter->second;
            m_undoMapping.erase(mapIter);
            if (m_activeGraphCanvasGraphId == graphCanvasGraphId)
            {
                m_activeGraphCanvasGraphId.SetInvalid();
            }
        }
    }

    void UndoManager::OnGraphRefreshed(const GraphCanvas::GraphId& oldGraphCanvasGraphId, const AZ::EntityId& newGraphCanvasGraphId)
    {
        if (!oldGraphCanvasGraphId.IsValid())
        {
            OnGraphLoaded(newGraphCanvasGraphId);
            return;
        }

        // Switches the undo stack when the scene gets remapped(i.e. when it's saved as)
        auto oldSceneIter = m_undoMapping.find(oldGraphCanvasGraphId);
        auto newSceneIter = m_undoMapping.find(newGraphCanvasGraphId);

        if (oldSceneIter != m_undoMapping.find(oldGraphCanvasGraphId)
            && newSceneIter == m_undoMapping.end())
        {
            m_undoMapping[newGraphCanvasGraphId] = oldSceneIter->second;
            m_undoMapping.erase(oldSceneIter);
            if (m_activeGraphCanvasGraphId == oldGraphCanvasGraphId)
            {
                m_activeGraphCanvasGraphId = newGraphCanvasGraphId;
            }
        }
    }

    AZStd::unique_ptr<SceneUndoState> UndoManager::ExtractSceneUndoState(AZ::EntityId scriptCanvasGraphId)
    {
        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

        if (!graphCanvasGraphId.IsValid())
        {
            return {};
        }

        AZStd::unique_ptr<SceneUndoState> extractUndoState;
        auto mapIter = m_undoMapping.find(graphCanvasGraphId);
        if (mapIter != m_undoMapping.end())
        {
            extractUndoState.reset(mapIter->second);
            m_undoMapping.erase(mapIter);
            if (m_activeGraphCanvasGraphId == graphCanvasGraphId)
            {
                m_activeGraphCanvasGraphId.SetInvalid();
            }
        }

        return extractUndoState;
    }

    void UndoManager::InsertUndoState(AZ::EntityId scriptCanvasGraphId, AZStd::unique_ptr<SceneUndoState> sceneUndoState)
    {
        AZ::EntityId graphCanvasGraphId;
        EditorGraphRequestBus::EventResult(graphCanvasGraphId, scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

        if (!graphCanvasGraphId.IsValid() || !sceneUndoState)
        {
            return;
        }

        delete m_undoMapping[graphCanvasGraphId];
        m_undoMapping[graphCanvasGraphId] = sceneUndoState.release();
    }

    SceneUndoState* UndoManager::FindActiveUndoState() const
    {
        return FindUndoState(m_activeGraphCanvasGraphId);
    }

    SceneUndoState* UndoManager::FindUndoState(const GraphCanvas::GraphId& graphCanvasGraphId) const
    {
        SceneUndoState* undoState = nullptr;

        auto mapIter = m_undoMapping.find(graphCanvasGraphId);

        if (mapIter != m_undoMapping.end())
        {
            undoState = mapIter->second;
        }

        return undoState;
    }

} // namespace ScriptCanvasEditor
