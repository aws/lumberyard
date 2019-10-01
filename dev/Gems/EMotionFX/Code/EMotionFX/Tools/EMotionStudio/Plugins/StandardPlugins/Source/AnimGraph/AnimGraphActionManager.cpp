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

#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/AnimGraphReferenceNode.h>
#include <EMotionStudio/EMStudioSDK/Source/MetricsEventSender.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <MCore/Source/StringConversions.h>
#include <QColorDialog>


namespace EMStudio
{
    AnimGraphActionFilter AnimGraphActionFilter::CreateDisallowAll()
    {
        AnimGraphActionFilter result;

        result.m_createNodes = false;
        result.m_editNodes = false;
        result.m_createConnections = false;
        result.m_editConnections = false;
        result.m_copyAndPaste = false;
        result.m_setEntryNode = false;
        result.m_activateState = false;
        result.m_delete = false;
        result.m_editNodeGroups = false;

        return result;
    }


    AnimGraphActionManager::AnimGraphActionManager(AnimGraphPlugin* plugin)
        : m_plugin(plugin)
        , m_pasteOperation(PasteOperation::None)
    {
    }

    AnimGraphActionManager::~AnimGraphActionManager()
    {
    }

    bool AnimGraphActionManager::GetIsReadyForPaste() const
    {
        return m_pasteOperation != PasteOperation::None;
    }

    void AnimGraphActionManager::ShowNodeColorPicker(EMotionFX::AnimGraphNode* animGraphNode)
    {
        const AZ::Color color = animGraphNode->GetVisualizeColor();
        const AZ::Color originalColor = color;
        QColor initialColor;
        initialColor.setRgbF(color.GetR(), color.GetG(), color.GetB(), color.GetA());

        QColorDialog dialog(initialColor);
        auto changeNodeColor = [animGraphNode](const QColor& color)
        {
            const AZ::Color newColor(color.red()/255.0f, color.green()/255.0f, color.blue()/255.0f, 1.0f);
            animGraphNode->SetVisualizeColor(newColor);
        };

        // Show live the color the user is choosing
        connect(&dialog, &QColorDialog::currentColorChanged, changeNodeColor);
        if (dialog.exec() != QDialog::Accepted)
        {
            changeNodeColor(initialColor);
        }
    }

    void AnimGraphActionManager::Copy()
    {
        m_pasteItems.clear();
        const QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        for (const QModelIndex& selectedIndex : selectedIndexes)
        {
            m_pasteItems.emplace_back(selectedIndex);
        }
        if (!m_pasteItems.empty())
        {
            m_pasteOperation = PasteOperation::Copy;
        }
    }

    void AnimGraphActionManager::Cut()
    {
        m_pasteItems.clear();
        const QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        for (const QModelIndex& selectedIndex : selectedIndexes)
        {
            m_pasteItems.emplace_back(selectedIndex);
        }
        if (!m_pasteItems.empty())
        {
            m_pasteOperation = PasteOperation::Cut;
        }
    }

    void AnimGraphActionManager::Paste(const QModelIndex& parentIndex, const QPoint& pos)
    {
        if (!GetIsReadyForPaste() || !parentIndex.isValid())
        {
            return;
        }

        AZStd::vector<EMotionFX::AnimGraphNode*> nodesToCopy;
        for (const QPersistentModelIndex& modelIndex : m_pasteItems)
        {
            // User could have deleted nodes in between the copy/cut and the paste operation, check that they are valid.
            if (modelIndex.isValid())
            {
                const AnimGraphModel::ModelItemType itemType = modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
                if (itemType == AnimGraphModel::ModelItemType::NODE)
                {
                    EMotionFX::AnimGraphNode* node = modelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                    nodesToCopy.emplace_back(node);
                }
            }
        }

        EMotionFX::AnimGraphNode* targetParentNode = parentIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

        if (!nodesToCopy.empty())
        {
            MCore::CommandGroup commandGroup(AZStd::string(m_pasteOperation == PasteOperation::Copy ? "Copy" : "Cut") + " and paste nodes");
            
            CommandSystem::AnimGraphCopyPasteData copyPasteData;
            CommandSystem::ConstructCopyAnimGraphNodesCommandGroup(&commandGroup, targetParentNode, nodesToCopy, pos.x(), pos.y(), m_pasteOperation == PasteOperation::Cut, copyPasteData, false);
            
            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }

        m_pasteOperation = PasteOperation::None;
        m_pasteItems.clear();
    }
    
    void AnimGraphActionManager::SetEntryState()
    {
        QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();

        if (!selectedIndexes.empty())
        {
            const QModelIndex firstSelectedNode = selectedIndexes.front();

            const EMotionFX::AnimGraphNode* node = firstSelectedNode.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

            const AZStd::string command = AZStd::string::format("AnimGraphSetEntryState -animGraphID %i -entryNodeName \"%s\"",
                node->GetAnimGraph()->GetID(),
                node->GetName());
            AZStd::string commandResult;
            if (!GetCommandManager()->ExecuteCommand(command, commandResult))
            {
                MCore::LogError(commandResult.c_str());
            }
        }
    }

    void AnimGraphActionManager::AddWildCardTransition()
    {
        QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        if (selectedIndexes.empty())
        {
            return;
        }

        MCore::CommandGroup commandGroup;

        for (QModelIndex selectedModelIndex : selectedIndexes)
        {
            if (selectedModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() != AnimGraphModel::ModelItemType::NODE)
            {
                continue;
            }

            EMotionFX::AnimGraphNode* node = selectedModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            
            // Only handle states, skip blend tree nodes.
            EMotionFX::AnimGraphNode* parentNode = node->GetParentNode();
            if (!parentNode || azrtti_typeid(parentNode) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                continue;
            }

            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

            const uint32 numWildcardTransitions = stateMachine->CalcNumWildcardTransitions(node);
            const bool isEven = (numWildcardTransitions % 2 == 0);
            const uint32 evenTransitions = numWildcardTransitions / 2;

            // space the wildcard transitions in case we're dealing with multiple ones
            uint32 endOffsetX = 0;
            uint32 endOffsetY = 0;

            // if there is no wildcard transition yet, we can skip directly
            if (numWildcardTransitions > 0)
            {
                if (isEven)
                {
                    endOffsetY = evenTransitions * 15;
                }
                else
                {
                    endOffsetX = (evenTransitions + 1) * 15;
                }
            }

            const AZStd::string commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"\" -targetNode \"%s\" -sourcePort 0 -targetPort 0 -startOffsetX 0 -startOffsetY 0 -endOffsetX %i -endOffsetY %i -transitionType \"%s\"",
                node->GetAnimGraph()->GetID(),
                node->GetName(),
                endOffsetX,
                endOffsetY,
                azrtti_typeid<EMotionFX::AnimGraphStateTransition>().ToString<AZStd::string>().c_str());

            commandGroup.AddCommandString(commandString);
        }

        if (commandGroup.GetNumCommands() > 0)
        {
            commandGroup.SetGroupName(AZStd::string::format("Add wildcard transition%s", commandGroup.GetNumCommands() > 1 ? "s" : ""));

            AZStd::string commandResult;
            AZ_Error("EMotionFX", GetCommandManager()->ExecuteCommandGroup(commandGroup, commandResult), commandResult.c_str());
        }
    }

    void AnimGraphActionManager::SetSelectedEnabled(bool enabled)
    {
        const QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        if (selectedIndexes.empty())
        {
            return;
        }

        MCore::CommandGroup commandGroup(enabled ? "Enable nodes" : "Disable nodes");

        for (const QModelIndex& modelIndex : selectedIndexes)
        {
            if (modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::NODE)
            {
                const EMotionFX::AnimGraphNode* node = modelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

                if (node->GetSupportsDisable())
                {
                    commandGroup.AddCommandString(AZStd::string::format("AnimGraphAdjustNode -animGraphID %i -name \"%s\" -enabled %s",
                        node->GetAnimGraph()->GetID(),
                        node->GetName(),
                        AZStd::to_string(enabled).c_str()));
                }
            }
        }

        if (commandGroup.GetNumCommands() > 0)
        {
            AZStd::string commandResult;
            if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, commandResult))
            {
                MCore::LogError(commandResult.c_str());
            }
        }
    }

    void AnimGraphActionManager::EnableSelected()
    {
        SetSelectedEnabled(true);
    }

    void AnimGraphActionManager::DisableSelected()
    {
        SetSelectedEnabled(false);
    }

    void AnimGraphActionManager::MakeVirtualFinalNode()
    {
        const QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();

        if (selectedIndexes.size() == 1)
        {
            const QModelIndex firstSelectedNode = selectedIndexes.front();
            EMotionFX::AnimGraphNode* node = firstSelectedNode.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            if (azrtti_typeid(node->GetParentNode()) == azrtti_typeid<EMotionFX::BlendTree>())
            {
                EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(node->GetParentNode());
                if (node == blendTree->GetFinalNode())
                {
                    blendTree->SetVirtualFinalNode(nullptr);
                }
                else
                {
                    blendTree->SetVirtualFinalNode(node);
                }

                // update the virtual final node
                m_plugin->GetGraphWidget()->SetVirtualFinalNode(firstSelectedNode);
            }
        }
    }

    void AnimGraphActionManager::RestoreVirtualFinalNode()
    {
        const QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();

        if (selectedIndexes.size() == 1)
        {
            const QModelIndex firstSelectedNode = selectedIndexes.front();
            EMotionFX::AnimGraphNode* node = firstSelectedNode.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            if (azrtti_typeid(node->GetParentNode()) == azrtti_typeid<EMotionFX::BlendTree>())
            {
                // set its new virtual final node
                EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(node->GetParentNode());
                blendTree->SetVirtualFinalNode(nullptr);

                // update the virtual final node
                const QModelIndexList finalNodes = m_plugin->GetAnimGraphModel().FindModelIndexes(blendTree->GetFinalNode());
                m_plugin->GetGraphWidget()->SetVirtualFinalNode(finalNodes.front());
            }
        }
    }

    void AnimGraphActionManager::DeleteSelectedNodes()
    {
        if (m_plugin->GetAnimGraphModel().CheckAnySelectedNodeBelongsToReferenceGraph())
        {
            return;
        }

        const AZStd::unordered_map<EMotionFX::AnimGraph*, AZStd::vector<EMotionFX::AnimGraphNode*>> nodesByAnimGraph = m_plugin->GetAnimGraphModel().GetSelectedObjectsOfType<EMotionFX::AnimGraphNode>();
        if (!nodesByAnimGraph.empty())
        {
            MCore::CommandGroup commandGroup("Delete anim graph nodes");
            AZ::u32 numNodes = 0;

            for (const AZStd::pair<EMotionFX::AnimGraph*, AZStd::vector<EMotionFX::AnimGraphNode*>>& animGraphAndNodes : nodesByAnimGraph)
            {
                numNodes += static_cast<AZ::u32>(animGraphAndNodes.second.size());
                CommandSystem::DeleteNodes(&commandGroup, animGraphAndNodes.first, animGraphAndNodes.second, true);
            }

            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
            else
            {
                MetricsEventSender::SendDeleteNodesAndConnectionsEvent(numNodes, 0);
            }
        }
    }

    void AnimGraphActionManager::NavigateToNode()
    {
        const QModelIndexList selectedIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();

        if (selectedIndexes.size() == 1)
        {
            const QModelIndex firstSelectedNode = selectedIndexes.front();
            m_plugin->GetAnimGraphModel().Focus(firstSelectedNode);
        }
    }


    void AnimGraphActionManager::OpenReferencedAnimGraph(EMotionFX::AnimGraphReferenceNode* referenceNode)
    {
        EMotionFX::AnimGraph* referencedGraph = referenceNode->GetReferencedAnimGraph();
        if (referencedGraph)
        {
            EMotionFX::MotionSet* motionSet = referenceNode->GetMotionSet();
            ActivateGraphForSelectedActors(referencedGraph, motionSet);
        }
    }


    void AnimGraphActionManager::ActivateGraphForSelectedActors(EMotionFX::AnimGraph* animGraph, EMotionFX::MotionSet* motionSet)
    {
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selectionList.GetNumSelectedActorInstances();

        if (numActorInstances == 0)
        {
            // No need to issue activation commands
            m_plugin->SetActiveAnimGraph(animGraph);
            return;
        }

        MCore::CommandGroup commandGroup("Activate anim graph");
        commandGroup.AddCommandString("RecorderClear -force true");

        // Activate the anim graph each selected actor instance.
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();

            // Use the given motion set in case it is valid, elsewise use the one previously set to the actor instance.
            uint32 motionSetId = MCORE_INVALIDINDEX32;
            if (motionSet)
            {
                motionSetId = motionSet->GetID();
            }
            else
            {
                // if a motionSet is not passed, then we try to get the one from the anim graph instance
                if (animGraphInstance)
                {
                    EMotionFX::MotionSet* animGraphInstanceMotionSet = animGraphInstance->GetMotionSet();
                    if (animGraphInstanceMotionSet)
                    {
                        motionSetId = animGraphInstanceMotionSet->GetID();
                    }
                }
            }

            commandGroup.AddCommandString(AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %d -motionSetID %d", actorInstance->GetID(), animGraph->GetID(), motionSetId));
        }

        if (commandGroup.GetNumCommands() > 0)
        {
            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
            m_plugin->SetActiveAnimGraph(animGraph);
        }
    }

}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.moc>
