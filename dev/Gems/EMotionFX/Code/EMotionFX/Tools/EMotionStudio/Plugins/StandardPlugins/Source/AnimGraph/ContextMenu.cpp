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

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFx/Source/AnimGraphReferenceNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodePaletteWidget.h>
#include <QMenu>


namespace EMStudio
{
    // Fill the given menu with anim graph objects that can be created inside the current given object and category.
    void BlendGraphWidget::RegisterItems(AnimGraphPlugin* plugin, QMenu* menu, EMotionFX::AnimGraphObject* object, EMotionFX::AnimGraphObject::ECategory category)
    {
        const AZStd::vector<EMotionFX::AnimGraphObject*>& objectPrototypes = plugin->GetAnimGraphObjectFactory()->GetUiObjectPrototypes();
        for (EMotionFX::AnimGraphObject* objectPrototype : objectPrototypes)
        {
            if (mPlugin->CheckIfCanCreateObject(object, objectPrototype, category))
            {
                // Add the item
                EMotionFX::AnimGraphNode* curNode = static_cast<EMotionFX::AnimGraphNode*>(objectPrototype);
                QIcon icon(NodePaletteWidget::GetNodeIconFileName(curNode).c_str());
                QAction* action = menu->addAction(icon, curNode->GetPaletteName());
                action->setWhatsThis(azrtti_typeid(curNode).ToString<QString>());
                action->setData(QVariant(curNode->GetPaletteName()));
                connect(action, &QAction::triggered, plugin->GetGraphWidget(), &BlendGraphWidget::OnContextMenuCreateNode);
            }
        }
    }


    void BlendGraphWidget::AddNodeGroupSubmenu(QMenu* menu, EMotionFX::AnimGraph* animGraph, const AZStd::vector<EMotionFX::AnimGraphNode*>& selectedNodes)
    {
        // node group sub menu
        QMenu* nodeGroupMenu = new QMenu("Node Group", menu);
        bool isNodeInNoneGroup = true;
        QAction* noneNodeGroupAction = nodeGroupMenu->addAction("None");
        noneNodeGroupAction->setData(0); // this index is there to know it's the real none action in case one node group is also called like that
        connect(noneNodeGroupAction, &QAction::triggered, this, &BlendGraphWidget::OnNodeGroupSelected);
        noneNodeGroupAction->setCheckable(true);

        const uint32 numNodeGroups = animGraph->GetNumNodeGroups();
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(i);

            QAction* nodeGroupAction = nodeGroupMenu->addAction(nodeGroup->GetName());
            nodeGroupAction->setData(i + 1); // index of the menu added, not used
            connect(nodeGroupAction, &QAction::triggered, this, &BlendGraphWidget::OnNodeGroupSelected);
            nodeGroupAction->setCheckable(true);

            if (selectedNodes.size() == 1)
            {
                const EMotionFX::AnimGraphNode* animGraphNode = selectedNodes[0];
                if (nodeGroup->Contains(animGraphNode->GetId()))
                {
                    nodeGroupAction->setChecked(true);
                    isNodeInNoneGroup = false;
                }
                else
                {
                    nodeGroupAction->setChecked(false);
                }
            }
        }

        if (selectedNodes.size() == 1)
        {
            if (isNodeInNoneGroup)
            {
                noneNodeGroupAction->setChecked(true);
            }
            else
            {
                noneNodeGroupAction->setChecked(false);
            }
        }

        menu->addMenu(nodeGroupMenu);
    }


    void BlendGraphWidget::OnContextMenuEvent(QWidget* parentWidget, QPoint localMousePos, QPoint globalMousePos, AnimGraphPlugin* plugin,
        const AZStd::vector<EMotionFX::AnimGraphNode*>& selectedNodes, bool graphWidgetOnlyMenusEnabled)
    {
        BlendGraphWidget*       blendGraphWidget= plugin->GetGraphWidget();
        BlendGraphViewWidget*   viewWidget      = plugin->GetViewWidget();
        NodeGraph*              nodeGraph       = blendGraphWidget->GetActiveGraph();
        AnimGraphActionManager& actionManager   = plugin->GetActionManager();

        // only show the paste and the create node menu entries in case the function got called from the graph widget
        if (!nodeGraph->IsInReferencedGraph() && graphWidgetOnlyMenusEnabled)
        {
            // check if we have actually clicked a node and if it is selected, only show the menu if both cases are true
            GraphNode* graphNode = nodeGraph->FindNode(localMousePos);
            if (graphNode == nullptr)
            {
                QMenu menu(parentWidget);

                if (actionManager.GetIsReadyForPaste())
                {
                    const QModelIndex modelIndex = nodeGraph->GetModelIndex();
                    if (modelIndex.isValid())
                    {
                        localMousePos = SnapLocalToGrid(LocalToGlobal(localMousePos));

                        QAction* pasteAction = menu.addAction("Paste");
                        pasteAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Paste.png"));
                        connect(pasteAction, &QAction::triggered, [&actionManager, modelIndex, localMousePos]() { actionManager.Paste(modelIndex, localMousePos); });
                        menu.addSeparator();
                    }
                }

                QMenu createNodeMenu("Create Node", parentWidget);
                QMenu sourcesMenu("Sources", &menu);
                QMenu blendingMenu("Blending", &menu);
                QMenu controllersMenu("Controllers", &menu);
                QMenu physicsMenu("Physics", &menu);
                QMenu logicMenu("Logic", &menu);
                QMenu mathMenu("Math", &menu);
                QMenu miscMenu("Misc", &menu);

                createNodeMenu.setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));

                EMotionFX::AnimGraphNode* currentNode = nodeGraph->GetModelIndex().data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();

                RegisterItems(plugin, &sourcesMenu,    currentNode, EMotionFX::AnimGraphNode::CATEGORY_SOURCES);
                RegisterItems(plugin, &blendingMenu,   currentNode, EMotionFX::AnimGraphNode::CATEGORY_BLENDING);
                RegisterItems(plugin, &controllersMenu,currentNode, EMotionFX::AnimGraphNode::CATEGORY_CONTROLLERS);
                RegisterItems(plugin, &physicsMenu,    currentNode, EMotionFX::AnimGraphNode::CATEGORY_PHYSICS);
                RegisterItems(plugin, &logicMenu,      currentNode, EMotionFX::AnimGraphNode::CATEGORY_LOGIC);
                RegisterItems(plugin, &mathMenu,       currentNode, EMotionFX::AnimGraphNode::CATEGORY_MATH);
                RegisterItems(plugin, &miscMenu,       currentNode, EMotionFX::AnimGraphNode::CATEGORY_MISC);

                if (!sourcesMenu.isEmpty())
                {
                    createNodeMenu.addMenu(&sourcesMenu);
                }
                if (!blendingMenu.isEmpty())
                {
                    createNodeMenu.addMenu(&blendingMenu);
                }
                if (!controllersMenu.isEmpty())
                {
                    createNodeMenu.addMenu(&controllersMenu);
                }
                if (!physicsMenu.isEmpty())
                {
                    createNodeMenu.addMenu(&physicsMenu);
                }
                if (!logicMenu.isEmpty())
                {
                    createNodeMenu.addMenu(&logicMenu);
                }
                if (!mathMenu.isEmpty())
                {
                    createNodeMenu.addMenu(&mathMenu);
                }
                if (!miscMenu.isEmpty())
                {
                    createNodeMenu.addMenu(&miscMenu);
                }
                if (!createNodeMenu.isEmpty())
                {
                    menu.addMenu(&createNodeMenu);
                }

                if (!menu.isEmpty())
                {
                    menu.exec(globalMousePos);
                    return;
                }
            }
        }

        const size_t numSelectedNodes = selectedNodes.size();

        // Is only one anim graph node selected?
        if (numSelectedNodes == 1 && selectedNodes[0])
        {
            QMenu menu(parentWidget);
            EMotionFX::AnimGraphNode* animGraphNode = selectedNodes[0];

            if (animGraphNode->GetParentNode())
            {
                // if the parent is a state machine
                if (azrtti_typeid(animGraphNode->GetParentNode()) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
                {
                    QAction* activateNodeAction = menu.addAction("Activate State");
                    activateNodeAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/Run.png"));
                    connect(activateNodeAction, &QAction::triggered, viewWidget, &BlendGraphViewWidget::OnActivateState);

                    if (!nodeGraph->IsInReferencedGraph())
                    {
                        EMotionFX::AnimGraphStateMachine* stateMachine = (EMotionFX::AnimGraphStateMachine*)animGraphNode->GetParentNode();
                        if (stateMachine->GetEntryState() != animGraphNode)
                        {
                            QAction* setAsEntryStateAction = menu.addAction("Set As Entry State");
                            setAsEntryStateAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/EntryState.png"));
                            connect(setAsEntryStateAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::SetEntryState);
                        }

                        // action for adding a wildcard transition
                        QAction* addWildcardAction = menu.addAction("Add Wildcard Transition");
                        addWildcardAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));
                        connect(addWildcardAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::AddWildCardTransition);
                    }
                } // parent is a state machine

                // if the parent is a state blend tree
                if (azrtti_typeid(animGraphNode->GetParentNode()) == azrtti_typeid<EMotionFX::BlendTree>())
                {
                    if (animGraphNode->GetSupportsDisable())
                    {
                        // enable or disable the node
                        if (animGraphNode->GetIsEnabled() == false)
                        {
                            QAction* enableAction = menu.addAction("Enable Node");
                            enableAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/EnableNodes.png"));
                            connect(enableAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::EnableSelected);
                        }
                        else
                        {
                            QAction* disableAction = menu.addAction("Disable Node");
                            disableAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/DisableNodes.png"));
                            connect(disableAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::DisableSelected);
                        }
                    }

                    if (animGraphNode->GetSupportsVisualization())
                    {
                        menu.addSeparator();
                        QAction* action = menu.addAction("Adjust Visualization Color");
                        action->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Rendering/Texturing.png"));
                        connect(action, &QAction::triggered, [this, animGraphNode](bool) { mPlugin->GetActionManager().ShowNodeColorPicker(animGraphNode); });
                    }
                }
            }

            if (!menu.isEmpty())
            {
                menu.addSeparator();
            }

            if (azrtti_typeid(animGraphNode) == azrtti_typeid<EMotionFX::AnimGraphReferenceNode>())
            {
                EMotionFX::AnimGraphReferenceNode* referenceNode = static_cast<EMotionFX::AnimGraphReferenceNode*>(animGraphNode);
                EMotionFX::AnimGraph* referencedGraph = referenceNode->GetReferencedAnimGraph();
                if (referencedGraph)
                {
                    AZStd::string filename;
                    AzFramework::StringFunc::Path::GetFullFileName(referencedGraph->GetFileName(), filename);
                    QAction* openAnimGraphAction = menu.addAction(QString("Open '%1' file").arg(filename.c_str()));
                    connect(openAnimGraphAction, &QAction::triggered, [&actionManager, referenceNode]() { actionManager.OpenReferencedAnimGraph(referenceNode); });
                    menu.addSeparator();
                }
            }

            // we can only go to the selected node in case the selected node has a visual graph (state machine / blend tree)
            if (animGraphNode->GetHasVisualGraph())
            {
                QAction* goToNodeAction = menu.addAction("Open Selected Node");
                goToNodeAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/OpenNode.png"));
                connect(goToNodeAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::NavigateToNode);
                menu.addSeparator();
            }

            // make the node a virtual final node
            if (animGraphNode->GetHasOutputPose())
            {
                if (animGraphNode->GetParentNode() && azrtti_typeid(animGraphNode->GetParentNode()) == azrtti_typeid<EMotionFX::BlendTree>())
                {
                    EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(animGraphNode->GetParentNode());
                    if (blendTree->GetVirtualFinalNode() != animGraphNode)
                    {
                        QAction* virtualAction = menu.addAction("Make Final Output");
                        virtualAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/Run.png"));
                        connect(virtualAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::MakeVirtualFinalNode);
                        menu.addSeparator();
                    }
                    else
                    {
                        if (blendTree->GetFinalNode() != animGraphNode)
                        {
                            QAction* virtualAction = menu.addAction("Restore Final Output");
                            virtualAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/Run.png"));
                            connect(virtualAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::RestoreVirtualFinalNode);
                            menu.addSeparator();
                        }
                    }
                }
            }

            if (animGraphNode->GetIsDeletable())
            {
                if (!nodeGraph->IsInReferencedGraph())
                {
                    // cut and copy actions
                    QAction* cutAction = menu.addAction("Cut");
                    cutAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Cut.png"));
                    connect(cutAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::Cut);
                }

                QAction* ccopyAction = menu.addAction("Copy");
                ccopyAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Copy.png"));
                connect(ccopyAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::Copy);
                menu.addSeparator();

                if (!nodeGraph->IsInReferencedGraph())
                {
                    QAction* removeNodeAction = menu.addAction("Delete Node");
                    removeNodeAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Remove.png"));
                    connect(removeNodeAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::DeleteSelectedNodes);
                    menu.addSeparator();
                }
            }

            // add the node group sub menu
            if (!nodeGraph->IsInReferencedGraph() && animGraphNode->GetParentNode())
            {
                AddNodeGroupSubmenu(&menu, animGraphNode->GetAnimGraph(), selectedNodes);
            }

            // show the menu at the given position
            if (menu.isEmpty() == false)
            {
                menu.exec(globalMousePos);
            }
        }

        // check if we are dealing with multiple selected nodes
        if (numSelectedNodes > 1)
        {
            // create the context menu
            QMenu menu(parentWidget);

            if (!nodeGraph->IsInReferencedGraph())
            {
                QAction* alignLeftAction = menu.addAction("Align Left");
                QAction* alignRightAction = menu.addAction("Align Right");
                QAction* alignTopAction = menu.addAction("Align Top");
                QAction* alignBottomAction = menu.addAction("Align Bottom");

                alignLeftAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/AlignLeft.png"));
                alignRightAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/AlignRight.png"));
                alignTopAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/AlignTop.png"));
                alignBottomAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/AlignBottom.png"));

                menu.addSeparator();
            }

            QAction* zoomSelectionAction = menu.addAction("Zoom Selection");
            zoomSelectionAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/FitSelected.png"));
            connect(zoomSelectionAction, &QAction::triggered, viewWidget, &BlendGraphViewWidget::ZoomSelected);

            menu.addSeparator();

            // check if all nodes selected have a blend tree as parent and if they all support disabling/enabling
            bool allBlendTreeNodes = true;
            for (const EMotionFX::AnimGraphNode* selectedNode : selectedNodes)
            {
                // make sure its a node inside a blend tree and that it supports disabling
                if (!selectedNode->GetParentNode() ||
                    (azrtti_typeid(selectedNode->GetParentNode()) != azrtti_typeid<EMotionFX::BlendTree>()))
                {
                    allBlendTreeNodes = false;
                    break;
                }
            }

            // if we only deal with blend tree nodes
            if (allBlendTreeNodes)
            {
                // check if we have at least one enabled or disabled node
                bool oneDisabled = false;
                bool oneEnabled = false;
                bool oneSupportsDelete = false;
                for (const EMotionFX::AnimGraphNode* selectedNode : selectedNodes)
                {
                    if (selectedNode->GetSupportsDisable())
                    {
                        oneSupportsDelete = true;
                        if (selectedNode->GetIsEnabled())
                        {
                            oneEnabled = true;
                        }
                        else
                        {
                            oneDisabled = true;
                        }
                    }
                }

                // enable all nodes
                if (oneDisabled && oneSupportsDelete)
                {
                    QAction* enableAction = menu.addAction("Enable Nodes");
                    enableAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/EnableNodes.png"));
                    connect(enableAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::EnableSelected);
                }

                // disable all nodes
                if (oneEnabled && oneSupportsDelete)
                {
                    QAction* disableAction = menu.addAction("Disable Nodes");
                    disableAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/DisableNodes.png"));
                    connect(disableAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::DisableSelected);
                }

                menu.addSeparator();
            }

            // check if we need to disable the delete nodes option as an undeletable node is selected
            bool canDelete = false;
            const AZStd::vector<GraphNode*> selectedGraphNodes = nodeGraph->GetSelectedGraphNodes();
            for (GraphNode* graphNode : selectedGraphNodes)
            {
                if (graphNode->GetIsDeletable())
                {
                    canDelete = true;
                    break;
                }
            }

            if (canDelete)
            {
                menu.addSeparator();

                if (!nodeGraph->IsInReferencedGraph())
                {
                    QAction* cutAction = menu.addAction("Cut");
                    cutAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Cut.png"));
                    connect(cutAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::Cut);
                }

                QAction* ccopyAction = menu.addAction("Copy");
                ccopyAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Copy.png"));
                connect(ccopyAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::Copy);

                menu.addSeparator();

                if (!nodeGraph->IsInReferencedGraph())
                {
                    QAction* removeNodesAction = menu.addAction("Delete Nodes");
                    removeNodesAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Remove.png"));
                    connect(removeNodesAction, &QAction::triggered, &actionManager, &AnimGraphActionManager::DeleteSelectedNodes);

                    menu.addSeparator();
                }
            }

            if (!nodeGraph->IsInReferencedGraph())
            {
                AddNodeGroupSubmenu(&menu, selectedNodes[0]->GetAnimGraph(), selectedNodes);
            }

            if (!menu.isEmpty())
            {
                menu.exec(globalMousePos);
            }
        }
    }
}
