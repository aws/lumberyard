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

// include required headers
#include "AnimGraphPlugin.h"
#include "BlendGraphWidget.h"
#include "NodeGraph.h"
#include "NodeGroupWindow.h"
#include "NodePaletteWidget.h"
#include "BlendGraphViewWidget.h"
#include "BlendGraphWidgetCallback.h"
#include "GraphNode.h"
#include "NavigateWidget.h"
#include "AttributesWindow.h"
#include "GameControllerWindow.h"
#include "BlendResourceWidget.h"
#include "BlendTreeVisualNode.h"
#include "StateGraphNode.h"
#include "ParameterWindow.h"
#include "GraphNodeFactory.h"
#include "RecorderWidget.h"
#include <QDockWidget>
#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringIDGenerator.h>

#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include <MysticQt/Source/DialogStack.h>

#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>

#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>


namespace EMStudio
{
    bool SetFirstSelectedAnimGraphActive()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        EMotionFX::AnimGraph* firstSelectedAnimGraph = CommandSystem::GetCommandManager()->GetCurrentSelection().GetFirstAnimGraph();
        animGraphPlugin->SetActiveAnimGraph(firstSelectedAnimGraph);
        return true;
    }


    bool AnimGraphPlugin::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return SetFirstSelectedAnimGraphActive();
    }
    bool AnimGraphPlugin::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return SetFirstSelectedAnimGraphActive();
    }
    bool AnimGraphPlugin::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return SetFirstSelectedAnimGraphActive();
    }
    bool AnimGraphPlugin::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return SetFirstSelectedAnimGraphActive();
    }
    bool AnimGraphPlugin::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)   { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return SetFirstSelectedAnimGraphActive(); }
    bool AnimGraphPlugin::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)      { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return SetFirstSelectedAnimGraphActive(); }


    //----------------------------------------------------------------------------------------------------------------------------------
    // CreateBlendNode callback
    //----------------------------------------------------------------------------------------------------------------------------------
    // callback when creating a node
    bool AnimGraphPlugin::CommandCreateBlendNodeCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        GetOutlinerManager()->FireItemModifiedEvent();

        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        // get the anim graph to work on
        MCore::String outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // get the emfx node
        CommandSystem::CommandAnimGraphCreateNode* blendCommand = (CommandSystem::CommandAnimGraphCreateNode*)command;
        EMotionFX::AnimGraphNode* emfxNode = animGraph->RecursiveFindNodeByID(blendCommand->mNodeID);

        // insert the new item inside the tree
        NavigateWidget* navigateWidget = animGraphPlugin->GetNavigateWidget();
        QTreeWidgetItem* parent = nullptr;
        if (emfxNode->GetParentNode())
        {
            parent = navigateWidget->FindItem(emfxNode->GetParentNode()->GetName());
        }
        navigateWidget->InsertNode(navigateWidget->GetTreeWidget(), parent, emfxNode, false);

        // get the EMotion FX parent node
        EMotionFX::AnimGraphNode* parentNode = emfxNode->GetParentNode();

        // try to find the visual graph related to this node
        NodeGraph* visualGraph;
        if (parentNode)
        {
            visualGraph = animGraphPlugin->FindGraphForNode(parentNode->GetID(), animGraph);
        }
        else
        {
            visualGraph = animGraphPlugin->FindGraphForNode(MCORE_INVALIDINDEX32, animGraph);
        }

        // create the new node and add it to the visual graph
        GraphNode* node = visualGraph->FindNodeByID(emfxNode->GetID());
        if (node == nullptr)
        {
            if (parentNode == nullptr || parentNode->GetType() == EMotionFX::AnimGraphStateMachine::TYPE_ID)
            {
                node = new StateGraphNode(animGraphPlugin, emfxNode);

                // in case the parent node is not nullptr and the parent node is a state machine
                if (parentNode && parentNode->GetType() == EMotionFX::AnimGraphStateMachine::TYPE_ID)
                {
                    // type cast the parent node to a state machine
                    EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

                    // in case the emfx node is the entry node sync it with the visual graph
                    if (stateMachine->GetEntryState() == emfxNode)
                    {
                        visualGraph->SetEntryNode(node);
                    }
                }

                visualGraph->AddNode(node);
            }
            else
            {
                node = animGraphPlugin->GetGraphNodeFactory()->CreateGraphNode(emfxNode->GetType(), emfxNode->GetName());
                if (node == nullptr)
                {
                    BlendTreeVisualNode* blendTreeVisualNode = new BlendTreeVisualNode(animGraphPlugin, emfxNode, false);
                    visualGraph->AddNode(blendTreeVisualNode);
                    blendTreeVisualNode->Sync();
                    blendTreeVisualNode->Update(QRect(std::numeric_limits<int32>::lowest() / 4, std::numeric_limits<int32>::max() / 4, std::numeric_limits<int32>::max() / 2, std::numeric_limits<int32>::max() / 2), QPoint(std::numeric_limits<int32>::max() / 2, std::numeric_limits<int32>::max() / 2));
                    node = blendTreeVisualNode;
                }
                else
                {
                    visualGraph->AddNode(node);
                }
            }
        }

        // make sure the correct emfx node is linked to our graph node
        AnimGraphVisualNode* animGraphVisualNode = static_cast<AnimGraphVisualNode*>(node);
        animGraphVisualNode->SetEMFXNode(emfxNode);

        node->SetDeletable(emfxNode->GetIsDeletable());
        node->SetBaseColor(emfxNode->GetVisualColor());
        node->SetHasChildIndicatorColor(emfxNode->GetHasChildIndicatorColor());

        //const int32 width = node->CalcRequiredWidth();
        //const int32 height = node->CalcRequiredHeight();

        //if (commandLine.GetValueAsBool("center", command))
        //{
        //  const QPoint newPos( emfxNode->GetVisualPosX() - width/2, emfxNode->GetVisualPosY() - height/2);
        //  node->MoveAbsolute( newPos );
        //  emfxNode->SetVisualPos( newPos.x(), newPos.y() );
        //}
        //else
        {
            const QPoint newPos(emfxNode->GetVisualPosX(), emfxNode->GetVisualPosY());
            node->MoveAbsolute(newPos);
            emfxNode->SetVisualPos(newPos.x(), newPos.y());
        }

        node->SetID(emfxNode->GetID());
        node->SetIsCollapsed(emfxNode->GetIsCollapsed());

        // sync the graph node with the emfx node
        animGraphVisualNode->Sync();
        //LogInfo("Visual Node '%s' created and synced.", node->GetName());

        return true;
    }


    // callback when undoing creation
    bool AnimGraphPlugin::CommandCreateBlendNodeCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);

        GetOutlinerManager()->FireItemModifiedEvent();

        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        // get the emfx node
        CommandSystem::CommandAnimGraphCreateNode* blendCommand = (CommandSystem::CommandAnimGraphCreateNode*)command;

        // get the anim graph
        const uint32 animGraphID = blendCommand->mAnimGraphID;
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (animGraph == nullptr)
        {
            MCore::LogError("The anim graph with id '%i' does not exist anymore.", animGraphID);
            return false;
        }

        // get the emfx node
        EMotionFX::AnimGraphNode* emfxNode = animGraph->RecursiveFindNodeByID(blendCommand->mNodeID);
        if (emfxNode == nullptr)
        {
            return true;
        }

        // get the item in the tree navigate tree widget, and remove it
        NavigateWidget* navigateWidget = animGraphPlugin->GetNavigateWidget();
        navigateWidget->RemoveTreeRow(emfxNode->GetName());

        // find the visual graph and visual node
        NodeGraph* visualGraph;
        GraphNode* graphNode;
        if (animGraphPlugin->FindGraphAndNode(emfxNode->GetParentNode(), emfxNode->GetID(), &visualGraph, &graphNode, animGraph) == false)
        {
            if (emfxNode->GetType() == EMotionFX::BlendTreeParameterNode::TYPE_ID)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        // remove the node from the graph
        visualGraph->RemoveNode(graphNode);

        // in case we removed a parent of the currently active graph
        EMotionFX::AnimGraphNode* currentAnimGraphNode = animGraphPlugin->GetGraphWidget()->GetCurrentNode();
        if (animGraphPlugin->CheckIfIsParentNodeRecursively(currentAnimGraphNode, emfxNode) ||
            emfxNode == currentAnimGraphNode)
        {
            EMotionFX::AnimGraphNode* node = nullptr;
            animGraphPlugin->GetNavigateWidget()->ShowGraph(node, true);
        }

        return true;
    }



    //----------------------------------------------------------------------------------------------------------------------------------
    // AdjustBlendNode callback
    //----------------------------------------------------------------------------------------------------------------------------------
    // callback when creating a node
    bool AnimGraphPlugin::CommandAdjustBlendNodeCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        // get the anim graph to work on
        MCore::String outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // get the emfx node
        CommandSystem::CommandAnimGraphAdjustNode* blendCommand = (CommandSystem::CommandAnimGraphAdjustNode*)command;
        EMotionFX::AnimGraphNode* emfxNode = animGraph->RecursiveFindNodeByID(blendCommand->GetNodeID());

        // get the new name
        MCore::String newName;
        commandLine.GetValue("newName", command, &newName);

        // rename the graph (change its ID)
        uint32 nodeID;
        if (newName.GetIsEmpty() == false)
        {
            nodeID = MCore::GetStringIDGenerator().GenerateIDForString(blendCommand->GetOldName().AsChar());
            animGraphPlugin->RenameGraph(nodeID, emfxNode->GetID(), animGraph);
        }
        else
        {
            nodeID = emfxNode->GetID();
        }

        // find the visual graph and visual node
        NodeGraph* visualGraph;
        GraphNode* graphNode;
        if (animGraphPlugin->FindGraphAndNode(emfxNode->GetParentNode(), nodeID, &visualGraph, &graphNode, animGraph, false) == false)
        {
            if (emfxNode->GetType() == EMotionFX::BlendTreeParameterNode::TYPE_ID)
            {
                return true;
            }
            else
            {
                MCore::LogError("AnimGraphPlugin::FindGraphAndNode() - There is no visual node associated with EMotion FX node '%s' in graph of node '%s'", MCore::GetStringIDGenerator().GetName(nodeID).AsChar(), emfxNode->GetParentNode() ? emfxNode->GetParentNode()->GetName() : "");
                return false;
            }
        }

        // sync the ports in case we change the parameter mask
        if (commandLine.CheckIfHasParameter("parameterMask"))
        {
            animGraphPlugin->SyncVisualNode(emfxNode);
        }

        // adjust the graph node
        graphNode->SetName(emfxNode->GetName());
        graphNode->SetID(emfxNode->GetID());
        graphNode->SetIsEnabled(emfxNode->GetIsEnabled());
        graphNode->SetIsVisualized(emfxNode->GetIsVisualizationEnabled());
        graphNode->MoveAbsolute(QPoint(emfxNode->GetVisualPosX(), emfxNode->GetVisualPosY()));

        return true;
    }


    // callback when undoing creation
    bool AnimGraphPlugin::CommandAdjustBlendNodeCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        CommandSystem::CommandAnimGraphAdjustNode* blendCommand = (CommandSystem::CommandAnimGraphAdjustNode*)command;

        // get the anim graph
        const uint32 animGraphID = blendCommand->mAnimGraphID;
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (animGraph == nullptr)
        {
            MCore::LogError("The anim graph with id '%i' does not exist anymore.", animGraphID);
            return false;
        }

        // get the new name
        MCore::String newName;
        commandLine.GetValue("newName", command, &newName);

        // get the emfx node
        EMotionFX::AnimGraphNode* emfxNode = animGraph->RecursiveFindNodeByID(blendCommand->GetNodeID());
        if (emfxNode == nullptr)
        {
            return true;
        }

        // sync the ports in case we change the parameter mask
        if (commandLine.CheckIfHasParameter("parameterMask"))
        {
            animGraphPlugin->SyncVisualNode(emfxNode);
        }

        // rename the graph (change its ID)
        if (newName.GetLength() > 0)
        {
            animGraphPlugin->RenameGraph(MCore::GetStringIDGenerator().GenerateIDForString(newName.AsChar()), emfxNode->GetID(), animGraph);
        }

        // get the EMotion FX parent node
        EMotionFX::AnimGraphNode* parentNode = emfxNode->GetParentNode();

        NodeGraph* visualGraph;
        if (parentNode)
        {
            visualGraph = animGraphPlugin->FindGraphForNode(parentNode->GetID(), animGraph);
        }
        else
        {
            visualGraph = animGraphPlugin->FindGraphForNode(MCORE_INVALIDINDEX32, animGraph);
        }

        // find the graph node associated with this emfx node
        GraphNode* graphNode;
        if (newName.GetLength() > 0)
        {
            graphNode = animGraphPlugin->FindGraphNode(visualGraph, MCore::GetStringIDGenerator().GenerateIDForString(newName.AsChar()), animGraph);
        }
        else
        {
            graphNode = animGraphPlugin->FindGraphNode(visualGraph, emfxNode->GetID(), animGraph);
        }

        // find the graph node associated with this emfx node
        if (graphNode == nullptr)
        {
            MCore::LogError("AnimGraphPlugin::CommandAdjustBlendNodeCallback::Execute() - Cannot locate visual graph node for emfx node '%s'", emfxNode->GetName());
            return false;
        }

        // adjust the graph
        graphNode->SetName(emfxNode->GetName());
        graphNode->SetID(emfxNode->GetID());
        graphNode->SetIsEnabled(emfxNode->GetIsEnabled());
        graphNode->SetIsVisualized(emfxNode->GetIsVisualizationEnabled());
        graphNode->MoveAbsolute(QPoint(emfxNode->GetVisualPosX(), emfxNode->GetVisualPosY()));

        return true;
    }



    //----------------------------------------------------------------------------------------------------------------------------------
    // CreateConnection callback
    //----------------------------------------------------------------------------------------------------------------------------------

    // shared create connection callback helper
    bool CreateConnectionCallbackHelper(MCore::Command* command, const MCore::CommandLine& commandLine, uint32 sourceNodeID, uint32 targetNodeID, uint32 sourcePort, uint32 targetPort, uint32 connectionID, QPoint startOffset, QPoint endOffset)
    {
        // find the anim graph plugin
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            MCore::LogError("CreateConnectionCallbackHelper: Cannot get anim graph plugin.");
            return false;
        }

        // get the anim graph to work on
        MCore::String outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        CommandSystem::CommandAnimGraphCreateConnection* blendCommand = (CommandSystem::CommandAnimGraphCreateConnection*)command;

        // when pasting connections to a parameter node where the parameter is not created yet, still return true so that the command group won't fail entirly
        MCore::String sourcePortName;
        commandLine.GetValue("sourcePortName", command, &sourcePortName);
        if (sourcePortName.GetIsEmpty() == false && blendCommand->GetConnectionID() == MCORE_INVALIDINDEX32)
        {
            return true;
        }

        // get the emfx anim graph nodes
        EMotionFX::AnimGraphNode* sourceEMFXNode = animGraph->RecursiveFindNodeByID(sourceNodeID);
        EMotionFX::AnimGraphNode* targetEMFXNode = animGraph->RecursiveFindNodeByID(targetNodeID);

        // check if we are dealing with a wildcard transition
        bool isWildcardTransition = false;
        if (sourceEMFXNode == nullptr)
        {
            isWildcardTransition = true;
        }

        // get the EMotion FX parent node
        EMotionFX::AnimGraphNode* parentNode = targetEMFXNode->GetParentNode();

        // try to find the visual graph related to this node
        NodeGraph* visualGraph;
        if (parentNode)
        {
            visualGraph = animGraphPlugin->FindGraphForNode(parentNode->GetID(), animGraph);
        }
        else
        {
            visualGraph = animGraphPlugin->FindGraphForNode(MCORE_INVALIDINDEX32, animGraph);
        }

        // find the graph node associated with this emfx node
        GraphNode* targetGraphNode = animGraphPlugin->FindGraphNode(visualGraph, targetEMFXNode->GetID(), animGraph);
        if (targetGraphNode == nullptr)
        {
            MCore::LogError("CreateConnectionCallbackHelper:: Cannot locate visual graph node for emfx node '%s'", targetEMFXNode->GetName());
            return false;
        }

        // find the graph node associated with this emfx node
        GraphNode* sourceGraphNode = nullptr;
        if (sourceEMFXNode)
        {
            sourceGraphNode = animGraphPlugin->FindGraphNode(visualGraph, sourceEMFXNode->GetID(), animGraph);
        }

        StateConnection* visualTransition = nullptr;
        if (sourceGraphNode)
        {
            // if we're connecting two state nodes
            if (targetGraphNode->GetType() == StateGraphNode::TYPE_ID && sourceGraphNode->GetType() == StateGraphNode::TYPE_ID)
            {
                EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(animGraph->RecursiveFindNodeByID(parentNode->GetID()));
                visualTransition = new StateConnection(stateMachine, sourceGraphNode, targetGraphNode, startOffset, endOffset, isWildcardTransition, connectionID);
                targetGraphNode->AddConnection(visualTransition);
                //MCore::LogInfo("Adding visual transition between state '%s' and '%s'.", sourceGraphNode->GetName(), targetGraphNode->GetName());
            }
            else
            {
                if (visualGraph->CheckIfHasConnection(sourceGraphNode, sourcePort, targetGraphNode, targetPort) == false)
                {
                    visualGraph->AddConnection(sourceGraphNode, sourcePort, targetGraphNode, targetPort);
                }
            }
        }
        else // wildcard transitions
        {
            // check if the target node is a state machine node
            if (targetGraphNode->GetType() == StateGraphNode::TYPE_ID)
            {
                //if (visualGraph->HasConnection(sourceGraphNode, sourcePort, targetGraphNode, targetPort) == false)
                {
                    EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(animGraph->RecursiveFindNodeByID(parentNode->GetID()));
                    visualTransition = new StateConnection(stateMachine, sourceGraphNode, targetGraphNode, startOffset, endOffset, isWildcardTransition, connectionID);
                    targetGraphNode->AddConnection(visualTransition);
                    //MCore::LogInfo("Adding visual wildcard transition  to '%s'.", targetGraphNode->GetName());
                }
                //else
                //  QMessageBox::critical(animGraphPlugin->GetGraphWidget(), "Error", "How could this happen? Two wild card transitions? Better contact the support directly.");
            }
            else
            {
                return false;
            }
        }

        // sync the visual with the emfx connection
        if (visualTransition)
        {
            if (parentNode && parentNode->GetType() == EMotionFX::AnimGraphStateMachine::TYPE_ID)
            {
                EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);
                EMotionFX::AnimGraphStateTransition* transition = stateMachine->FindTransitionByID(connectionID);

                if (transition)
                {
                    animGraphPlugin->SyncTransition(transition, visualTransition);
                }
            }
        }

        // refresh viewports
        if (isWildcardTransition)
        {
            animGraphPlugin->GetViewWidget()->Update();
        }

        return true;
    }


    // callback when creating a connection
    bool AnimGraphPlugin::CommandCreateConnectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        GetOutlinerManager()->FireItemModifiedEvent();

        // call the create connection callback helper function
        CommandSystem::CommandAnimGraphCreateConnection* blendCommand = (CommandSystem::CommandAnimGraphCreateConnection*)command;
        const QPoint startOffset(blendCommand->GetStartOffsetX(), blendCommand->GetStartOffsetY());
        const QPoint endOffset(blendCommand->GetEndOffsetX(), blendCommand->GetEndOffsetY());
        return CreateConnectionCallbackHelper(command,
            commandLine,
            blendCommand->GetSourceNodeID(),
            blendCommand->GetTargetNodeID(),
            blendCommand->GetSourcePort(),
            blendCommand->GetTargetPort(),
            blendCommand->GetConnectionID(),
            startOffset,
            endOffset);
    }


    // callback when undoing creation
    bool AnimGraphPlugin::CommandCreateConnectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        // the create connection undo internally calls the remove connection command so everything is updated in there already
        return true;
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    // RemoveConnection callback
    //----------------------------------------------------------------------------------------------------------------------------------

    // shared remove connection callback helper
    bool RemoveConnectionCallbackHelper(MCore::Command* command, const MCore::CommandLine& commandLine, uint32 sourceNodeID, uint32 targetNodeID, uint32 sourcePort, uint32 targetPort, uint32 connectionID)
    {
        // get the anim graph plugin
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            MCore::LogError("RemoveConnectionCallbackHelper: Cannot get anim graph plugin.");
            return false;
        }

        // get the anim graph to work on
        MCore::String outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // get the emfx anim graph nodes
        EMotionFX::AnimGraphNode* sourceEMFXNode = animGraph->RecursiveFindNodeByID(sourceNodeID);
        EMotionFX::AnimGraphNode* targetEMFXNode = animGraph->RecursiveFindNodeByID(targetNodeID);
        if (targetEMFXNode == nullptr)
        {
            MCore::LogError("RemoveConnectionCallbackHelper: EMotion FX target anim graph node is invalid.");
            return false;
        }

        // get the EMotion FX and the visual graph parent node of the target node
        EMotionFX::AnimGraphNode* parentNode = targetEMFXNode->GetParentNode();
        NodeGraph* visualGraph;
        if (parentNode)
        {
            visualGraph = animGraphPlugin->FindGraphForNode(parentNode->GetID(), animGraph);
        }
        else
        {
            visualGraph = animGraphPlugin->FindGraphForNode(MCORE_INVALIDINDEX32, animGraph);
        }

        // find the graph node associated with the target emfx node
        GraphNode* targetGraphNode = animGraphPlugin->FindGraphNode(visualGraph, targetEMFXNode->GetID(), animGraph);
        if (targetGraphNode == nullptr)
        {
            MCore::LogError("AnimGraphPlugin::RemoveConnectionCallbackHelper(): Cannot locate visual graph node for emfx target anim graph node '%s'", targetEMFXNode->GetName());
            return false;
        }

        // find the graph node associated with the source emfx node
        GraphNode* sourceGraphNode = nullptr;
        if (sourceEMFXNode)
        {
            sourceGraphNode = animGraphPlugin->FindGraphNode(visualGraph, sourceEMFXNode->GetID(), animGraph);
            if (sourceGraphNode == nullptr)
            {
                MCore::LogError("AnimGraphPlugin::RemoveConnectionCallbackHelper(): Cannot locate visual graph node for emfx source anim graph node '%s'", sourceEMFXNode->GetName());
                return false;
            }
        }

        // remove the visual graph connection
        if (targetGraphNode->RemoveConnection(targetPort, sourceGraphNode, sourcePort, connectionID) == false)
        {
            MCore::LogError("AnimGraphPlugin::CommandRemoveConnectionCallback::Execute() - Removing connection failed.");
            return false;
        }

        // refresh viewports
        if (sourceEMFXNode == nullptr)
        {
            animGraphPlugin->GetViewWidget()->Update();
        }

        // reset the attribute widget else it could still show the removed transition in the attribute window and that will crash when editing the values
        AttributesWindow* attributeWindow = animGraphPlugin->GetAttributesWindow();
        attributeWindow->InitForAnimGraphObject(nullptr);
        return true;
    }


    // callback when removing a connection
    bool AnimGraphPlugin::CommandRemoveConnectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        GetOutlinerManager()->FireItemModifiedEvent();

        // call the remove connection callback helper function
        CommandSystem::CommandAnimGraphRemoveConnection* blendCommand = (CommandSystem::CommandAnimGraphRemoveConnection*)command;
        return RemoveConnectionCallbackHelper(command, commandLine, blendCommand->GetSourceNodeID(), blendCommand->GetTargetNodeID(), blendCommand->GetSourcePort(), blendCommand->GetTargetPort(), blendCommand->GetConnectionID());
    }


    // callback when undoing the remove connection command
    bool AnimGraphPlugin::CommandRemoveConnectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        // the remove connection undo internally calls the create connection command so everything is updated in there already
        return true;
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    // AdjustConnection callback
    //----------------------------------------------------------------------------------------------------------------------------------

    // callback when removing a connection
    bool AnimGraphPlugin::CommandAdjustConnectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCore::String outResult;

        // get the anim graph plugin
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            MCore::LogError("CommandAdjustConnectionCallback: Cannot get anim graph plugin.");
            return false;
        }

        // get the anim graph to work on
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        MCore::String stateMachineName;
        commandLine.GetValue("stateMachine", "", &stateMachineName);

        // find the node in the anim graph
        EMotionFX::AnimGraphNode* stateMachineNode = animGraph->RecursiveFindNode(stateMachineName.AsChar());
        if (stateMachineNode == nullptr)
        {
            outResult.Format("CommandAdjustConnectionCallback: Cannot find state machine node with name '%s' in anim graph '%s'", stateMachineName.AsChar(), animGraph->GetName());
            return false;
        }

        // type cast it to a state machine node
        if (stateMachineNode->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            outResult.Format("CommandAdjustConnectionCallback: Anim graph node named '%s' is not a state machine.", stateMachineName.AsChar());
            return false;
        }
        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(stateMachineNode);

        // get the transition id and find the transition index based on the id
        const uint32 transitionID    = commandLine.GetValueAsInt("transitionID", command);
        const uint32 transitionIndex = stateMachine->FindTransitionIndexByID(transitionID);
        if (transitionIndex == MCORE_INVALIDINDEX32)
        {
            outResult.Format("CommandAdjustConnectionCallback: The state transition you are trying to adjust cannot be found.");
            return false;
        }

        // save the transition information for undo
        EMotionFX::AnimGraphStateTransition* transition = stateMachine->GetTransition(transitionIndex);

        // get the emfx anim graph nodes
        EMotionFX::AnimGraphNode* sourceEMFXNode = transition->GetSourceNode();
        EMotionFX::AnimGraphNode* targetEMFXNode = transition->GetTargetNode();

        // get the EMotion FX and the visual graph parent node of the target node
        NodeGraph* stateMachineNodeGraph = animGraphPlugin->FindGraphForNode(stateMachineNode->GetID(), animGraph);
        if (stateMachineNodeGraph == nullptr)
        {
            outResult.Format("CommandAdjustConnectionCallback: Cannot find visual state machine node for id %i.", stateMachineNode->GetID());
            return false;
        }

        NodeConnection* visualConnection = stateMachineNodeGraph->FindConnectionByID(transitionID);
        if (visualConnection && visualConnection->GetType() != StateConnection::TYPE_ID)
        {
            outResult.Format("CommandAdjustConnectionCallback: Visual connection is not a transition.");
            return false;
        }

        // find the graph node associated with the target emfx node
        GraphNode* targetGraphNode = animGraphPlugin->FindGraphNode(stateMachineNodeGraph, targetEMFXNode->GetID(), animGraph);
        if (targetGraphNode == nullptr)
        {
            MCore::LogError("AnimGraphPlugin::CommandAdjustConnectionCallback(): Cannot locate visual graph node for emfx target anim graph node '%s'", targetEMFXNode->GetName());
            return false;
        }

        // find the graph node associated with the source emfx node
        GraphNode* sourceGraphNode = nullptr;
        if (sourceEMFXNode)
        {
            sourceGraphNode = animGraphPlugin->FindGraphNode(stateMachineNodeGraph, sourceEMFXNode->GetID(), animGraph);
            if (sourceGraphNode == nullptr)
            {
                MCore::LogError("AnimGraphPlugin::CommandAdjustConnectionCallback(): Cannot locate visual graph node for emfx source anim graph node '%s'", sourceEMFXNode->GetName());
                return false;
            }
        }

        StateConnection* visualTransition = static_cast<StateConnection*>(visualConnection);
        visualTransition->SetStartOffset(QPoint(transition->GetVisualStartOffsetX(), transition->GetVisualStartOffsetY()));
        visualTransition->SetEndOffset(QPoint(transition->GetVisualEndOffsetX(), transition->GetVisualEndOffsetY()));

        if (sourceGraphNode)
        {
            visualTransition->SetSourceNode(sourceGraphNode);
        }
        if (targetGraphNode)
        {
            visualTransition->SetTargetNode(targetGraphNode);
        }

        // remove the connection from the old target state
        {
            bool connectionRemovedSuccess = false;
            const uint32 numStates = stateMachineNodeGraph->GetNumNodes();
            for (uint32 i = 0; i < numStates; ++i)
            {
                GraphNode* state = stateMachineNodeGraph->GetNode(i);

                if (state->RemoveConnection(visualConnection, false))
                {
                    connectionRemovedSuccess = true;
                }
            }

            if (connectionRemovedSuccess == false)
            {
                MCore::LogError("AnimGraphPlugin::CommandAdjustConnectionCallback(): Cannot remove relinked visual transition from the old target state.");
                return false;
            }
        }

        // and add it to the new target state
        targetGraphNode->AddConnection(visualTransition);

        // sync the visual with the emfx connection
        animGraphPlugin->SyncTransition(transition, visualTransition);

        // refresh viewports
        if (sourceEMFXNode == nullptr)
        {
            animGraphPlugin->GetViewWidget()->Update();
        }

        // reset the attribute widget else it could still show the removed transition in the attribute window and that will crash when editing the values
        AttributesWindow* attributeWindow = animGraphPlugin->GetAttributesWindow();
        attributeWindow->InitForAnimGraphObject(nullptr);
        return true;
    }


    // callback when undoing the remove connection command
    bool AnimGraphPlugin::CommandAdjustConnectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        // the remove connection undo internally calls the create connection command so everything is updated in there already
        return true;
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    // RemoveNode POST callback
    //----------------------------------------------------------------------------------------------------------------------------------
    bool AnimGraphPlugin::CommandRemoveNodePostCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        GetOutlinerManager()->FireItemModifiedEvent();

        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        // get the emfx node
        CommandSystem::CommandAnimGraphRemoveNode* blendCommand = (CommandSystem::CommandAnimGraphRemoveNode*)command;

        // get the anim graph to work on
        MCore::String outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // get the EMotion FX parent node
        EMotionFX::AnimGraphNode* parentNode = nullptr;
        if (blendCommand->GetParentID() != MCORE_INVALIDINDEX32)
        {
            parentNode = animGraph->RecursiveFindNodeByID(blendCommand->GetParentID());
        }

        // try to find the visual graph related to this node
        NodeGraph* visualGraph;
        if (parentNode)
        {
            visualGraph = animGraphPlugin->FindGraphForNode(parentNode->GetID(), animGraph);
        }
        else
        {
            visualGraph = animGraphPlugin->FindGraphForNode(MCORE_INVALIDINDEX32, animGraph);
        }

        // find the graph node associated with this emfx node
        GraphNode* graphNode = visualGraph->FindNodeByID(blendCommand->GetNodeID());
        if (graphNode == nullptr)
        {
            //MCore::LogError("AnimGraphPlugin::CommandRemoveConnectionCallback::Execute() - Cannot locate visual graph node for emfx node.");
            return true;
        }

        //("#### Visual Node '%s' removed.", graphNode->GetName());

        // remove the node
        visualGraph->RemoveNode(graphNode);

        // remove the visual graph of the removed node
        animGraphPlugin->RemoveGraphForNode(blendCommand->GetNodeID(), animGraph);
        animGraphPlugin->GetNavigateWidget()->RemoveTreeRow(MCore::GetStringIDGenerator().GetName(blendCommand->GetNodeID()).AsChar());
        //animGraphPlugin->GetNavigateWidget()->UpdateTreeWidget( animGraph );

        animGraphPlugin->GetNavigateWidget()->update();

        return true;
    }


    // undo
    bool AnimGraphPlugin::CommandRemoveNodePostCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        // the remove connection undo internally calls the create node command so everything is updated in there already
        return true;
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    // RemoveNode PRE callback
    //----------------------------------------------------------------------------------------------------------------------------------
    bool AnimGraphPlugin::CommandRemoveNodePreCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        // get the emfx node
        //EMotionFX::CommandAnimGraphRemoveNode* blendCommand = (EMotionFX::CommandAnimGraphRemoveNode*)command;

        // get the anim graph to work on
        MCore::String outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // in case we removed a parent of the currently active graph
        // get the EMotion FX node
        MCore::String name;
        commandLine.GetValue("name", "", &name);
        EMotionFX::AnimGraphNode* emfxNode = animGraph->RecursiveFindNode(name.AsChar());
        EMotionFX::AnimGraphNode* currentAnimGraphNode = animGraphPlugin->GetGraphWidget()->GetCurrentNode();
        if (animGraphPlugin->CheckIfIsParentNodeRecursively(currentAnimGraphNode, emfxNode) ||
            emfxNode == currentAnimGraphNode)
        {
            EMotionFX::AnimGraphNode* node = nullptr;
            animGraphPlugin->GetNavigateWidget()->ShowGraph(node, true);
        }

        return true;
    }


    // undo
    bool AnimGraphPlugin::CommandRemoveNodePreCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return true;
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    // CommandSetAsEntryStateCallback callback
    //----------------------------------------------------------------------------------------------------------------------------------
    bool AnimGraphPlugin::CommandSetAsEntryStateCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        // get the plugin and check if it is valid
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        // get the anim graph to work on
        MCore::String outResult;
        EMotionFX::AnimGraph* animGraph = CommandSystem::CommandsGetAnimGraph(commandLine, command, outResult);
        if (animGraph == nullptr)
        {
            return false;
        }

        // find the entry anim graph node
        MCore::String entryNodeName;
        commandLine.GetValue("entryNodeName", "", &entryNodeName);
        EMotionFX::AnimGraphNode* entryNode = animGraph->RecursiveFindNode(entryNodeName.AsChar());
        if (entryNode == nullptr)
        {
            return false;
        }

        // check if the parent node is a state machine
        EMotionFX::AnimGraphNode* stateMachineNode = entryNode->GetParentNode();
        if (stateMachineNode == nullptr || stateMachineNode->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            return false;
        }

        // try to find the visual graph related to this node
        NodeGraph* visualGraph;
        if (stateMachineNode)
        {
            visualGraph = animGraphPlugin->FindGraphForNode(stateMachineNode->GetID(), animGraph);
        }
        else
        {
            visualGraph = animGraphPlugin->FindGraphForNode(MCORE_INVALIDINDEX32, animGraph);
        }

        // find the graph node associated with this emfx node
        GraphNode* graphNode = visualGraph->FindNodeByID(entryNode->GetID());
        if (graphNode == nullptr)
        {
            MCore::LogError("AnimGraphPlugin::CommandSetAsEntryStateCallback::Execute() - Cannot locate visual graph node for emfx node");
            return false;
        }

        // finally set the graph node as entry node
        visualGraph->SetEntryNode(graphNode);
        return true;
    }


    // undo
    bool AnimGraphPlugin::CommandSetAsEntryStateCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);

        // get the plugin and check if it is valid
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        // get the emfx node
        CommandSystem::CommandAnimGraphSetEntryState* blendCommand = (CommandSystem::CommandAnimGraphSetEntryState*)command;

        // get the anim graph
        const uint32 animGraphID = blendCommand->mAnimGraphID;
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (animGraph == nullptr)
        {
            MCore::LogError("The anim graph with id '%i' does not exist anymore.", animGraphID);
            return false;
        }

        // try to find the visual graph related to this node
        NodeGraph* visualGraph = animGraphPlugin->FindGraphForNode(blendCommand->mOldStateMachineNodeID, animGraph);
        if (visualGraph == nullptr)
        {
            return false;
        }

        if (blendCommand->mOldEntryStateNodeID == MCORE_INVALIDINDEX32)
        {
            visualGraph->SetEntryNode(nullptr);
            return true;
        }

        // find the graph node associated with this emfx node
        GraphNode* graphNode = visualGraph->FindNodeByID(blendCommand->mOldEntryStateNodeID);
        if (graphNode == nullptr)
        {
            return false;
        }

        // finally set the graph node as entry node
        visualGraph->SetEntryNode(graphNode);
        return true;
    }

    //----------------------------------------------------------------------------------------------------------------------------------
    // CommandAnimGraphAddConditionCallback and CommandAnimGraphRemoveConditionCallback callback
    //----------------------------------------------------------------------------------------------------------------------------------
    bool UpdateAttributesWindow()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        AttributesWindow* attributeWindow = animGraphPlugin->GetAttributesWindow();
        attributeWindow->InitForAnimGraphObject(attributeWindow->GetObject());
        return true;
    }

    bool AnimGraphPlugin::CommandAnimGraphAddConditionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAttributesWindow(); }
    bool AnimGraphPlugin::CommandAnimGraphAddConditionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)          { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAttributesWindow(); }
    bool AnimGraphPlugin::CommandAnimGraphRemoveConditionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAttributesWindow(); }
    bool AnimGraphPlugin::CommandAnimGraphRemoveConditionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAttributesWindow(); }


    //----------------------------------------------------------------------------------------------------------------------------------
    // Parameters
    //----------------------------------------------------------------------------------------------------------------------------------
    bool AnimGraphPlugin::CommandAnimGraphCreateParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAttributesWindow(); }
    bool AnimGraphPlugin::CommandAnimGraphCreateParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAttributesWindow(); }
    bool AnimGraphPlugin::CommandAnimGraphAdjustParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAttributesWindow(); }
    bool AnimGraphPlugin::CommandAnimGraphAdjustParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAttributesWindow(); }
    bool AnimGraphPlugin::CommandAnimGraphRemoveParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAttributesWindow(); }
    bool AnimGraphPlugin::CommandAnimGraphRemoveParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAttributesWindow(); }


    //----------------------------------------------------------------------------------------------------------------------------------
    // CommandPlayMotionCallback callback
    //----------------------------------------------------------------------------------------------------------------------------------
    bool AnimGraphPlugin::CommandPlayMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)   { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); MCore::String outResult; return GetCommandManager()->ExecuteCommand("Unselect -animGraphIndex SELECT_ALL", outResult, false); }
    bool AnimGraphPlugin::CommandPlayMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)      { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); MCore::String outResult; return GetCommandManager()->ExecuteCommand("Unselect -animGraphIndex SELECT_ALL", outResult, false); }

    //----------------------------------------------------------------------------------------------------------------------------------
    // CommandAnimGraphWapParametersCallback callback
    //----------------------------------------------------------------------------------------------------------------------------------
    bool AnimGraphPlugin::CommandAnimGraphWapParametersCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        EMotionFX::AnimGraph* activeAnimGraph = animGraphPlugin->GetActiveAnimGraph();
        EMotionFX::AnimGraphNode* lastShownNode = animGraphPlugin->GetGraphWidget()->GetCurrentNode();
        animGraphPlugin->GetParameterWindow()->Init();
        animGraphPlugin->ReInitAllGraphs();
        animGraphPlugin->ShowGraph(lastShownNode, activeAnimGraph, false);

        return true;
    }


    bool AnimGraphPlugin::CommandAnimGraphWapParametersCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        EMotionFX::AnimGraph* activeAnimGraph = animGraphPlugin->GetActiveAnimGraph();
        EMotionFX::AnimGraphNode* lastShownNode = animGraphPlugin->GetGraphWidget()->GetCurrentNode();
        animGraphPlugin->GetParameterWindow()->Init();
        animGraphPlugin->ReInitAllGraphs();
        animGraphPlugin->ShowGraph(lastShownNode, activeAnimGraph, false);

        return true;
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    // CommandRecorderClearCallback callback
    //----------------------------------------------------------------------------------------------------------------------------------
    bool AnimGraphPlugin::CommandRecorderClearCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        animGraphPlugin->GetRecorderWidget()->OnClearButton();
        animGraphPlugin->GetParameterWindow()->Init();

        return true;
    }


    bool AnimGraphPlugin::CommandRecorderClearCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return true;
    }


    //----------------------------------------------------------------------------------------------------------------------------------
    // CommandMotionSetAdjustMotionCallback callback
    //----------------------------------------------------------------------------------------------------------------------------------
    bool AnimGraphPlugin::CommandMotionSetAdjustMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        // check if the node is remapped
        if (commandLine.CheckIfHasParameter("newIDString") == false)
        {
            return false;
        }

        // find the anim graph plugin
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        // cast the plugin to anim graph plugin
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;

        // get the new ID
        MCore::String newIDString;
        commandLine.GetValue("newIDString", command, &newIDString);

        // update the attribute window if one motion node is selected using this new ID
        EMotionFX::AnimGraphObject* object = animGraphPlugin->GetAttributesWindow()->GetObject();
        if (object && object->GetType() == EMotionFX::AnimGraphMotionNode::TYPE_ID)
        {
            EMotionFX::AnimGraphMotionNode* motionNode = (EMotionFX::AnimGraphMotionNode*)object;
            MCore::AttributeArray* motionArray = motionNode->GetAttributeArray(EMotionFX::AnimGraphMotionNode::ATTRIB_MOTION);
            const uint32 numMotions = motionArray->GetNumAttributes();
            for (uint32 i = 0; i < numMotions; ++i)
            {
                MCore::Attribute* attribute = motionArray->GetAttribute(i);
                MCORE_ASSERT(attribute->GetType() == MCore::AttributeString::TYPE_ID);
                MCore::AttributeString* motionString = (MCore::AttributeString*)attribute;
                if (newIDString == motionString->GetValue())
                {
                    animGraphPlugin->GetAttributesWindow()->Reinit();
                    break;
                }
            }
        }

        // update all motion nodes of the active graph using this new ID
        NodeGraph* nodeGraph = animGraphPlugin->GetGraphWidget()->GetActiveGraph();
        if (nodeGraph)
        {
            MCore::Array<GraphNode*> graphNodes = nodeGraph->GetNodes();
            const uint32 numNodes = graphNodes.GetLength();
            for (uint32 n = 0; n < numNodes; ++n)
            {
                EMotionFX::AnimGraphNode* node = ((BlendTreeVisualNode*)graphNodes[n])->GetEMFXNode();
                if (node->GetType() != EMotionFX::AnimGraphMotionNode::TYPE_ID)
                {
                    continue;
                }

                EMotionFX::AnimGraphMotionNode* motionNode = (EMotionFX::AnimGraphMotionNode*)node;
                MCore::AttributeArray* motionArray = motionNode->GetAttributeArray(EMotionFX::AnimGraphMotionNode::ATTRIB_MOTION);
                const uint32 numMotions = motionArray->GetNumAttributes();
                for (uint32 i = 0; i < numMotions; ++i)
                {
                    MCore::Attribute* attribute = motionArray->GetAttribute(i);
                    MCORE_ASSERT(attribute->GetType() == MCore::AttributeString::TYPE_ID);
                    MCore::AttributeString* motionString = (MCore::AttributeString*)attribute;
                    if (newIDString == motionString->GetValue())
                    {
                        graphNodes[n]->Sync();
                        break;
                    }
                }
            }
        }

        return true;
    }


    bool AnimGraphPlugin::CommandMotionSetAdjustMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return true;
    }
} // namespace EMStudio