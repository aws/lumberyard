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

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>

#include <AzCore/Component/Entity.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/VisualBus.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <Editor/Nodes/NodeUtils.h>
#include <Editor/Nodes/ScriptCanvasAssetNode.h>

#include "NodeContextMenu.h"

namespace
{
    QAction* GetGroupAction(const QPointF& scenePos, const AZ::EntityId& nodeId, QObject* parent)
    {
        auto* action = new QAction({ QObject::tr("Group selection") }, parent);
        action->setToolTip(QObject::tr("Add this node and any other selected nodes to a new sub scene"));
        action->setStatusTip(QObject::tr("Add this node and any other selected nodes to a new sub scene"));

        QAction::connect(action,
            &QAction::triggered,
            [nodeId, &scenePos](bool)
        {
            AZ::EntityId sceneId;
            GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, nodeId, &GraphCanvas::SceneMemberRequests::GetScene);

            AZ::Entity* sceneEntity{};
            AZ::ComponentApplicationBus::BroadcastResult(sceneEntity, &AZ::ComponentApplicationRequests::FindEntity, sceneId);
            AZ::EntityId graphId;
            ScriptCanvas::SystemRequestBus::BroadcastResult(graphId, &ScriptCanvas::SystemRequests::FindGraphId, sceneEntity);

            auto nodeIdPair = ScriptCanvasEditor::Nodes::CreateNode(azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAssetNode>(), graphId, "");
            AZ::Entity* entity{};
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, nodeIdPair.m_scriptCanvasId);
            auto scriptCanvasAssetNode = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvasEditor::ScriptCanvasAssetNode>(entity);
            if (scriptCanvasAssetNode)
            {

                // Add this node to the set of selected nodes in this scene
                AZStd::vector<AZ::EntityId> selectedNodes;
                GraphCanvas::SceneRequestBus::EventResult(selectedNodes, sceneId, &GraphCanvas::SceneRequests::GetSelectedNodes);
                auto nodeIt = AZStd::find(selectedNodes.begin(), selectedNodes.end(), nodeId);
                if (nodeIt == selectedNodes.end())
                {
                    selectedNodes.push_back(nodeId);
                }

                AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset> scriptCanvasAsset;
                ScriptCanvasEditor::DocumentContextRequestBus::BroadcastResult(scriptCanvasAsset, &ScriptCanvasEditor::DocumentContextRequests::CreateScriptCanvasAsset, "");
                scriptCanvasAssetNode->SetAsset(scriptCanvasAsset);
                scriptCanvasAssetNode->SetAssetDataStoredInternally(true); //Group data is stored ScriptCanvas Node serialized data and not referenced

                AZ::Entity* assetEntity = scriptCanvasAssetNode->GetScriptCanvasEntity();
                if (assetEntity)
                {
                    if (assetEntity->GetState() == AZ::Entity::ES_CONSTRUCTED)
                    {
                        assetEntity->Init();
                    }
                    if (assetEntity->GetState() == AZ::Entity::ES_INIT)
                    {
                        assetEntity->Activate();
                    }
                    GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::Cut, selectedNodes);
                    GraphCanvas::SceneRequestBus::Event(assetEntity->GetId(), &GraphCanvas::SceneRequests::Paste);
                }

                GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::AddNode, nodeIdPair.m_graphCanvasId, AZ::Vector2(scenePos.x(), scenePos.y()));
                GraphCanvas::SceneMemberUIRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);
                ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::PostUndoPoint, sceneId);
            }

            // Undo
        });

        return action;
    }

    QAction* GetUnGroupAction(const AZ::EntityId& nodeId, QObject* parent)
    {
        auto* action = new QAction({ QObject::tr("UnGroup node") }, parent);
        action->setToolTip(QObject::tr("Moves nodes in the sub scene to it's parent scenes and removes the sub scene"));
        action->setStatusTip(QObject::tr("Moves nodes in the sub scene to it's parent scenes and removes the sub scene"));
        action->setEnabled(false);

        AZStd::any* nodeUserData{};
        GraphCanvas::NodeRequestBus::EventResult(nodeUserData, nodeId, &GraphCanvas::NodeRequests::GetUserData);
        auto scNodeId = nodeUserData && nodeUserData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(nodeUserData) : AZ::EntityId();

        AZ::Entity* entity{};
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, scNodeId);
        auto scriptCanvasAssetNode = entity ? AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvasEditor::ScriptCanvasAssetNode>(entity) : nullptr;
        action->setEnabled(scriptCanvasAssetNode && scriptCanvasAssetNode->GetAssetDataStoredInternally());

        QObject::connect(action,
            &QAction::triggered,
            [nodeId, scriptCanvasAssetNode](bool checked)
        {
            AZ::EntityId graphId = scriptCanvasAssetNode->GetGraphId();
            AZ::Entity* scriptCanvasEntity{};
            ScriptCanvas::GraphRequestBus::EventResult(scriptCanvasEntity, graphId, &ScriptCanvas::GraphRequests::GetGraphEntity);
            AZ::Entity* childEntity = scriptCanvasAssetNode->GetScriptCanvasEntity();
            if (scriptCanvasEntity && childEntity)
            {
                AZ::Vector2 nodePos = AZ::Vector2::CreateZero();
                GraphCanvas::GeometryRequestBus::EventResult(nodePos, nodeId, &GraphCanvas::GeometryRequests::GetPosition);

                AZStd::vector<AZ::EntityId> sceneNodes{};
                GraphCanvas::SceneRequestBus::EventResult(sceneNodes, childEntity->GetId(), &GraphCanvas::SceneRequests::GetNodes);
                GraphCanvas::SceneRequestBus::Event(childEntity->GetId(), &GraphCanvas::SceneRequests::Cut, sceneNodes);
                GraphCanvas::SceneRequestBus::Event(scriptCanvasEntity->GetId(), &GraphCanvas::SceneRequests::Paste);

                ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::DeleteNodes, scriptCanvasEntity->GetId(), AZStd::vector<AZ::EntityId>{nodeId});

                // Undo
                ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::PostUndoPoint, scriptCanvasEntity->GetId());
            }
        });

        return action;
    }

    AZStd::vector<AZ::EntityId> GetAggregatedSceneAndActiveSelectedNodes(const AZ::EntityId& sceneId, const AZ::EntityId& activeNodeId)
    {
        // Get the already selected nodes and add this node to the selection
        AZStd::vector<AZ::EntityId> selectedNodes;
        GraphCanvas::SceneRequestBus::EventResult(selectedNodes, sceneId, &GraphCanvas::SceneRequests::GetSelectedNodes);
        auto nodeIt = AZStd::find(selectedNodes.begin(), selectedNodes.end(), activeNodeId);
        if (nodeIt == selectedNodes.end())
        {
            selectedNodes.push_back(activeNodeId);
        }

        return selectedNodes;
    }

    QAction* GetNodeCopyAction(const AZ::EntityId& nodeId, QObject* parent)
    {
        // Get the already selected nodes and add this node to the selection
        AZ::EntityId sceneId;
        GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, nodeId, &GraphCanvas::SceneMemberRequests::GetScene);
        AZStd::vector<AZ::EntityId> selectedNodes = GetAggregatedSceneAndActiveSelectedNodes(sceneId, nodeId);

        bool isMultipleNodes = selectedNodes.size() > 1;
        auto* action = new QAction({ isMultipleNodes ? QObject::tr("Copy Nodes") : QObject::tr("Copy Node") }, parent);

        QString tipMessage = isMultipleNodes ? QObject::tr("Copy these nodes to the clipboard") : QObject::tr("Copy this node to the clipboard");
        action->setToolTip(tipMessage);
        action->setStatusTip(tipMessage);

        QObject::connect(action,
            &QAction::triggered,
            [sceneId, selectedNodes](bool)
            {
                GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::Copy, selectedNodes);
            });

        return action;
    }

    QAction* GetNodeCutAction(const AZ::EntityId& nodeId, QObject* parent)
    {
        // Get the already selected nodes and add this node to the selection
        AZ::EntityId sceneId;
        GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, nodeId, &GraphCanvas::SceneMemberRequests::GetScene);

        AZStd::vector<AZ::EntityId> selectedNodes = GetAggregatedSceneAndActiveSelectedNodes(sceneId, nodeId);

        bool isMultipleNodes = selectedNodes.size() > 1;
        auto* action = new QAction({ isMultipleNodes ? QObject::tr("Cut Nodes") : QObject::tr("Cut Node") }, parent);

        QString tipMessage = isMultipleNodes ? QObject::tr("Cut these nodes to the clipboard") : QObject::tr("Cut this node to the clipboard");
        action->setToolTip(tipMessage);
        action->setStatusTip(tipMessage);

        QObject::connect(action,
            &QAction::triggered,
            [sceneId, selectedNodes](bool)
            {
                GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::Cut, selectedNodes);
            });

        return action;
    }

    QAction* GetNodeDuplicateAction(const AZ::EntityId& nodeId, QObject* parent)
    {
        // Get the already selected nodes and add this node to the selection
        AZ::EntityId sceneId;
        GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, nodeId, &GraphCanvas::SceneMemberRequests::GetScene);

        AZStd::vector<AZ::EntityId> selectedNodes = GetAggregatedSceneAndActiveSelectedNodes(sceneId, nodeId);

        bool isMultipleNodes = selectedNodes.size() > 1;
        auto* action = new QAction({ isMultipleNodes ? QObject::tr("Duplicate Nodes") : QObject::tr("Duplicate Node") }, parent);

        QString tipMessage = isMultipleNodes ? QObject::tr("Duplicate these nodes") : QObject::tr("Duplicate this node");
        action->setToolTip(tipMessage);
        action->setStatusTip(tipMessage);

        QObject::connect(action,
            &QAction::triggered,
            [sceneId, selectedNodes](bool)
        {
            GraphCanvas::SceneRequestBus::Event(sceneId, &GraphCanvas::SceneRequests::Duplicate, selectedNodes);
        });

        return action;
    }

    QAction* GetNodeDeleteAction(const AZ::EntityId& nodeId, QObject* parent)
    {
        AZ::EntityId sceneId;
        GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, nodeId, &GraphCanvas::SceneMemberRequests::GetScene);

        AZStd::vector<AZ::EntityId> selectedNodes = GetAggregatedSceneAndActiveSelectedNodes(sceneId, nodeId);

        bool isMultipleNodes = selectedNodes.size() > 1;
        auto* action = new QAction({ isMultipleNodes ? QObject::tr("Delete Nodes") : QObject::tr("Delete Node") }, parent);

        QString tipMessage = isMultipleNodes ? QObject::tr("Delete these nodes") : QObject::tr("Delete this node");
        action->setToolTip(tipMessage);
        action->setStatusTip(tipMessage);

        QObject::connect(action,
            &QAction::triggered,
            [selectedNodes, sceneId](bool)
            {
                ScriptCanvasEditor::GeneralRequestBus::Broadcast(&ScriptCanvasEditor::GeneralRequests::DeleteNodes, sceneId, selectedNodes);
            });

        return action;
    }
} // anonymous namespace.

namespace ScriptCanvasEditor
{
    NodeContextMenu::NodeContextMenu(const QPointF& scenePos, const AZ::EntityId& nodeId)
        : QMenu()
    {
        addAction(GetNodeCutAction(nodeId, this));
        addAction(GetNodeCopyAction(nodeId, this));
        addAction(GetNodeDuplicateAction(nodeId, this));
        addAction(GetNodeDeleteAction(nodeId, this));
        addSeparator();

        // Temporarily moving Group / Ungroup actions until further UI support is added
        //addAction(GetGroupAction(scenePos, nodeId, this));
        //addAction(GetUnGroupAction(nodeId, this));
    }
}
