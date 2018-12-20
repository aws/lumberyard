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
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/Utils.h>

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Variable/VariableBus.h>

#include <GraphCanvas/Components/SceneBus.h>

#include <Editor/Undo/ScriptCanvasGraphCommand.h>
#include <Editor/Undo/ScriptCanvasUndoCache.h>

#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>

namespace ScriptCanvasEditor
{
    GraphItemCommand::GraphItemCommand(AZStd::string_view friendlyName)
        : AzToolsFramework::UndoSystem::URSequencePoint(friendlyName, 0)
    {
    }

    void GraphItemCommand::Undo()
    {
    }

    void GraphItemCommand::Redo()
    {
    }

    void GraphItemCommand::Capture(AZ::Entity*, bool)
    {
    }

    void GraphItemCommand::RestoreItem(const AZStd::vector<AZ::u8>&)
    {
    }

    void GraphItemChangeCommand::DeleteOldGraphData(const UndoData& oldData)
    {
        GraphCanvas::SceneRequestBus::Event(m_graphCanvasGraphId, &GraphCanvas::SceneRequests::DeleteGraphData, oldData.m_graphCanvasGraphData);
        ScriptCanvas::GraphVariableManagerRequestBus::Event(m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::DeleteVariableData, oldData.m_variableData);
    }

    void GraphItemChangeCommand::ActivateRestoredGraphData(const UndoData& restoredData)
    {
        // Activate Graph Canvas Scene Data
        AZStd::unordered_set<AZ::Entity*> entities;
        restoredData.m_graphCanvasGraphData.CollectEntities(entities);

        for (AZ::Entity* entity : entities)
        {
            if (entity->GetState() == AZ::Entity::ES_CONSTRUCTED)
            {
                entity->Init();
            }
        }

        // Reset VariableData on the GraphVariableManager before re-init node entities, as the GetVariableNode/SetVariable Node
        // queries the VariableRequestBus
        ScriptCanvas::GraphVariableManagerRequestBus::Event(m_scriptCanvasGraphId, &ScriptCanvas::GraphVariableManagerRequests::SetVariableData, restoredData.m_variableData);

        // Init Script Canvas Graph Data
        for (auto entityContainer : { restoredData.m_graphData.m_nodes })
        {
            for (AZ::Entity* entity : entityContainer)
            {
                if (entity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                {
                    entity->Init();
                }
            }
        }

        for (auto entityContainer : { restoredData.m_graphData.m_connections })
        {
            for (AZ::Entity* entity : entityContainer)
            {
                if (entity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                {
                    entity->Init();
                }
            }
        }

        ScriptCanvas::GraphRequestBus::Event(m_scriptCanvasGraphId, &ScriptCanvas::GraphRequests::AddGraphData, restoredData.m_graphData);
        GraphCanvas::SceneRequestBus::Event(m_graphCanvasGraphId, &GraphCanvas::SceneRequests::AddGraphData, restoredData.m_graphCanvasGraphData);
    }

    //// Graph Item Change Command
    GraphItemChangeCommand::GraphItemChangeCommand(AZStd::string_view friendlyName)
        : GraphItemCommand(friendlyName)
    {
    }

    void GraphItemChangeCommand::Undo()
    {
        RestoreItem(m_undoState);
    }

    void GraphItemChangeCommand::Redo()
    {
        RestoreItem(m_redoState);
    }

    void GraphItemChangeCommand::Capture(AZ::Entity* scriptCanvasEntity, bool captureUndo)
    {
        if (!scriptCanvasEntity)
        {
            return;
        }

        m_scriptCanvasGraphId = scriptCanvasEntity->GetId();
        EditorGraphRequestBus::EventResult(m_graphCanvasGraphId, m_scriptCanvasGraphId, &EditorGraphRequests::GetGraphCanvasGraphId);

        UndoCache* undoCache{};
        UndoRequestBus::BroadcastResult(undoCache, &UndoRequests::GetSceneUndoCache, m_scriptCanvasGraphId);
        if (!undoCache)
        {
            AZ_Error("Script Canvas", false, "Unable to find ScriptCanvas Undo Cache. Most likely the Undo Manager has no active scene");
            return;
        }

        if (captureUndo)
        {
            AZ_Assert(m_undoState.empty(), "Attempting to capture undo twice");
            m_undoState = undoCache->Retrieve(m_scriptCanvasGraphId);
            if (m_undoState.empty())
            {
                undoCache->UpdateCache(m_scriptCanvasGraphId);
                m_undoState = undoCache->Retrieve(m_scriptCanvasGraphId);
            }

            undoCache->UpdateCache(m_scriptCanvasGraphId);
        }
        else
        {
            UndoData undoData;
            UndoRequestBus::BroadcastResult(undoData, &UndoRequests::CreateUndoData, scriptCanvasEntity->GetId());

            AZ::SerializeContext* serializeContext{};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            m_redoState.clear();
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> byteStream(&m_redoState);
            AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&byteStream, *serializeContext, AZ::DataStream::ST_BINARY);
            if (!objStream->WriteClass(&undoData))
            {
                AZ_Assert(false, "Unable to serialize Script Canvas scene and graph data for undo/redo");
                return;
            }

            objStream->Finalize();
        }
    }

    void GraphItemChangeCommand::RestoreItem(const AZStd::vector<AZ::u8>& restoreBuffer)
    {
        if (restoreBuffer.empty())
        {
            return;
        }

        AZ::Entity* scriptCanvasEntity;
        AZ::ComponentApplicationBus::BroadcastResult(scriptCanvasEntity, &AZ::ComponentApplicationRequests::FindEntity, m_scriptCanvasGraphId);
        if (!scriptCanvasEntity)
        {
            return;
        }

        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ::IO::MemoryStream byteStream(restoreBuffer.data(), restoreBuffer.size());

        UndoData oldData;
        UndoRequestBus::BroadcastResult(oldData, &UndoRequests::CreateUndoData, m_scriptCanvasGraphId);

        // Remove old Script Canvas data
        GraphItemCommandNotificationBus::Event(m_scriptCanvasGraphId, &GraphItemCommandNotificationBus::Events::PreRestore, oldData);
        DeleteOldGraphData(oldData);

        UndoData restoreData;
        bool restoreSuccess = AZ::Utils::LoadObjectFromStreamInPlace(byteStream, restoreData, serializeContext, AZ::ObjectStream::FilterDescriptor(AZ::Data::AssetFilterNoAssetLoading));
        if (restoreSuccess)
        {
            ActivateRestoredGraphData(restoreData);

            UndoCache* undoCache{};
            UndoRequestBus::BroadcastResult(undoCache, &UndoRequests::GetSceneUndoCache, m_scriptCanvasGraphId);
            if (!undoCache)
            {
                AZ_Assert(false, "Unable to find ScriptCanvas Undo Cache. Most likely the ScriptCanvasEditor Undo Manager has not been created");
                return;
            }
            undoCache->UpdateCache(m_scriptCanvasGraphId);

            GraphItemCommandNotificationBus::Event(m_scriptCanvasGraphId, &GraphItemCommandNotifications::PostRestore, restoreData);
        }
    }

    //// Graph Item Add Command
    GraphItemAddCommand::GraphItemAddCommand(AZStd::string_view friendlyName)
        : GraphItemChangeCommand(friendlyName)
    {
    }

    void GraphItemAddCommand::Undo()
    {
        RestoreItem(m_undoState);
    }

    void GraphItemAddCommand::Redo()
    {
        RestoreItem(m_redoState);
    }

    void GraphItemAddCommand::Capture(AZ::Entity* scriptCanvasEntity, bool)
    {
        GraphItemChangeCommand::Capture(scriptCanvasEntity, false);
    }

    //// Graph Item Removal Command
    GraphItemRemovalCommand::GraphItemRemovalCommand(AZStd::string_view friendlyName)
        : GraphItemChangeCommand(friendlyName)
    {
    }

    void GraphItemRemovalCommand::Undo()
    {
        RestoreItem(m_undoState);
    }

    void GraphItemRemovalCommand::Redo()
    {
        RestoreItem(m_redoState);
    }

    void GraphItemRemovalCommand::Capture(AZ::Entity* scriptCanvasEntity, bool)
    {
        GraphItemChangeCommand::Capture(scriptCanvasEntity, true);
    }
}
