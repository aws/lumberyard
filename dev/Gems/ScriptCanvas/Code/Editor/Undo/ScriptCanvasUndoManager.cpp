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
#include <Editor/Undo/ScriptCanvasUndoManager.h>
#include <Editor/Undo/ScriptCanvasUndoCache.h>
#include <Editor/Undo/ScriptCanvasGraphCommand.h>
#include <ScriptCanvas/Bus/UndoBus.h>

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

    UndoManager::SceneUndoState::SceneUndoState(const AZ::EntityId& entityId, AzToolsFramework::UndoSystem::IUndoNotify* undoNotify)
        : m_undoStack(AZStd::make_unique<AzToolsFramework::UndoSystem::UndoStack>(c_undoLimit, undoNotify))
        , m_undoCache(AZStd::make_unique<UndoCache>())
    {        
    }

    void UndoManager::SceneUndoState::BeginUndoBatch(AZStd::string_view label)
    {
        if (!m_currentUndoBatch)
        {
            m_currentUndoBatch = aznew AzToolsFramework::UndoSystem::URSequencePoint(label, 0);
        }
        else
        {
            auto parentUndoBatch = m_currentUndoBatch;

            m_currentUndoBatch = aznew AzToolsFramework::UndoSystem::URSequencePoint(label, 0);
            m_currentUndoBatch->SetParent(parentUndoBatch);
        }
    }

    void UndoManager::SceneUndoState::EndUndoBatch()
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

    UndoManager::SceneUndoState::~SceneUndoState()
    {
        delete m_currentUndoBatch;
    }

    UndoManager::UndoManager()
    {
        MainWindowNotificationBus::Handler::BusConnect();
        UndoRequestBus::Handler::BusConnect();
    }

    UndoManager::~UndoManager()
    {
        MainWindowNotificationBus::Handler::BusDisconnect();
        UndoRequestBus::Handler::BusDisconnect();

        for (auto& mapPair : m_undoMapping)
        {
            delete mapPair.second;
        }
    }

    UndoCache* UndoManager::GetUndoCache()
    {
        SceneUndoState* sceneUndoState = FindActiveUndoState();

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

    UndoData UndoManager::CreateUndoData(AZ::EntityId entityId)
    {
        GraphCanvas::SceneData* sceneData;
        GraphCanvas::SceneRequestBus::EventResult(sceneData, entityId, &GraphCanvas::SceneRequests::GetSceneData);

        ScriptCanvas::GraphData* graphData;
        ScriptCanvas::GraphRequestBus::EventResult(graphData, entityId, &ScriptCanvas::GraphRequests::GetGraphData);

        UndoData undoData;
        if (graphData && sceneData)
        {
            undoData.m_sceneData = *sceneData;
            undoData.m_graphData = *graphData;
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

    void UndoManager::OnActiveSceneChanged(const AZ::EntityId& sceneId)
    {
        m_activeScene = sceneId;
        OnUndoStackChanged();
    }

    void UndoManager::OnSceneLoaded(const AZ::EntityId& sceneId)
    {
        if (!sceneId.IsValid())
        {
            return;
        }

        auto mapIter = m_undoMapping.find(sceneId);

        if (mapIter == m_undoMapping.end())
        {
            m_undoMapping[sceneId] = aznew SceneUndoState(sceneId, this);
        }
    }

    void UndoManager::OnSceneUnloaded(const AZ::EntityId& sceneId)
    {
        auto mapIter = m_undoMapping.find(sceneId);

        if (mapIter != m_undoMapping.end())
        {
            delete mapIter->second;
            m_undoMapping.erase(mapIter);
        }
    }

    void UndoManager::OnSceneRefreshed(const AZ::EntityId& oldSceneId, const AZ::EntityId& newSceneId)
    {
        if (!oldSceneId.IsValid())
        {
            OnSceneLoaded(newSceneId);
            return;
        }

        // Switches the undo stack when the scene gets remapped(i.e. when it's saved as)
        auto oldSceneIter = m_undoMapping.find(oldSceneId);
        auto newSceneIter = m_undoMapping.find(newSceneId);

        if (oldSceneIter != m_undoMapping.find(oldSceneId)
            && newSceneIter == m_undoMapping.end())
        {
            m_undoMapping[newSceneId] = oldSceneIter->second;
            m_undoMapping.erase(oldSceneIter);
        }
    }

    UndoManager::SceneUndoState* UndoManager::FindActiveUndoState() const
    {
        return FindUndoState(m_activeScene);
    }

    UndoManager::SceneUndoState* UndoManager::FindUndoState(const AZ::EntityId& sceneId) const
    {
        SceneUndoState* undoState = nullptr;

        auto mapIter = m_undoMapping.find(sceneId);

        if (mapIter != m_undoMapping.end())
        {
            undoState = mapIter->second;
        }

        return undoState;
    }

} // namespace ScriptCanvasEditor
