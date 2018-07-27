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
#include "NavigateWidget.h"
#include "NodePaletteWidget.h"
#include "BlendGraphWidget.h"
#include "BlendGraphViewWidget.h"
#include "BlendTreeVisualNode.h"
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>

#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>


namespace EMStudio
{
    // Fill the given menu with anim graph objects that can be created inside the current given object and category.
    void BlendGraphWidget::RegisterItems(AnimGraphPlugin* plugin, QMenu* menu, EMotionFX::AnimGraphObject* object, EMotionFX::AnimGraphObject::ECategory category)
    {
        const AZStd::unordered_set<AZ::TypeId>& nodeObjectTypes = EMotionFX::AnimGraphObjectFactory::GetUITypes();
        for (const AZ::TypeId& nodeObjectType : nodeObjectTypes)
        {
            EMotionFX::AnimGraphObject* curObject = EMotionFX::AnimGraphObjectFactory::Create(nodeObjectType);

            if (mPlugin->CheckIfCanCreateObject(object, curObject, category))
            {
                // Add the item
                EMotionFX::AnimGraphNode* curNode = static_cast<EMotionFX::AnimGraphNode*>(curObject);
                QIcon icon(NodePaletteWidget::GetNodeIconFileName(curNode).c_str());
                QAction* action = menu->addAction(icon, curNode->GetPaletteName());
                action->setWhatsThis(azrtti_typeid(curNode).ToString<QString>());
                action->setData(QVariant(curNode->GetPaletteName()));
                connect(action, SIGNAL(triggered()), plugin->GetGraphWidget(), SLOT(OnContextMenuCreateNode()));
            }

            if (curObject)
            {
                delete curObject;
            }
        }
    }


    void BlendGraphWidget::AddNodeGroupSubmenu(QMenu* menu, EMotionFX::AnimGraph* animGraph, const MCore::Array<EMotionFX::AnimGraphNode*>& selectedNodes)
    {
        // node group sub menu
        QMenu* nodeGroupMenu = new QMenu("Node Group", menu);
        bool isNodeInNoneGroup = true;
        QAction* noneNodeGroupAction = nodeGroupMenu->addAction("None");
        noneNodeGroupAction->setData(0); // this index is there to know it's the real none action in case one node group is also called like that
        connect(noneNodeGroupAction, SIGNAL(triggered()), this, SLOT(OnNodeGroupSelected()));
        noneNodeGroupAction->setCheckable(true);

        const uint32 numNodeGroups = animGraph->GetNumNodeGroups();
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            // get the node group
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(i);

            QAction* nodeGroupAction = nodeGroupMenu->addAction(nodeGroup->GetName());
            nodeGroupAction->setData(i + 1); // index of the menu added, not used
            connect(nodeGroupAction, SIGNAL(triggered()), this, SLOT(OnNodeGroupSelected()));
            nodeGroupAction->setCheckable(true);

            if (selectedNodes.GetLength() == 1)
            {
                EMotionFX::AnimGraphNode* animGraphNode = selectedNodes[0];
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

        if (selectedNodes.GetLength() == 1)
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


    void BlendGraphWidget::OnContextMenuEvent(QWidget* parentWidget, QPoint localMousePos, QPoint globalMousePos, AnimGraphPlugin* plugin, const MCore::Array<EMotionFX::AnimGraphNode*> selectedNodes, bool graphWidgetOnlyMenusEnabled)
    {
        NavigateWidget*         navigateWidget  = plugin->GetNavigateWidget();
        BlendGraphWidget*       blendGraphWidget= plugin->GetGraphWidget();
        BlendGraphViewWidget*   viewWidget      = plugin->GetViewWidget();
        NodeGraph*              nodeGraph       = blendGraphWidget->GetActiveGraph();

        // only show the paste and the create node menu entries in case the function got called from the graph widget
        if (graphWidgetOnlyMenusEnabled)
        {
            // check if we have actually clicked a node and if it is selected, only show the menu if both cases are true
            GraphNode* graphNode = nodeGraph->FindNode(localMousePos);
            if (graphNode == nullptr)
            {
                // create the context menu
                QMenu menu(parentWidget);

                if (navigateWidget->GetIsReadyForPaste())
                {
                    QAction* pasteAction = menu.addAction("Paste");
                    pasteAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Paste.png"));
                    connect(pasteAction, SIGNAL(triggered()), navigateWidget, SLOT(Paste()));
                    menu.addSeparator();
                }

                // add the create node category menus
                QMenu createNodeMenu("Create Node", parentWidget);
                QMenu sourcesMenu("Sources", &menu);
                QMenu blendingMenu("Blending", &menu);
                QMenu controllersMenu("Controllers", &menu);
                QMenu logicMenu("Logic", &menu);
                QMenu mathMenu("Math", &menu);
                QMenu miscMenu("Misc", &menu);

                createNodeMenu.setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));

                EMotionFX::AnimGraphNode* currentNode = blendGraphWidget->GetCurrentNode();

                RegisterItems(plugin, &sourcesMenu,    currentNode, EMotionFX::AnimGraphNode::CATEGORY_SOURCES);
                RegisterItems(plugin, &blendingMenu,   currentNode, EMotionFX::AnimGraphNode::CATEGORY_BLENDING);
                RegisterItems(plugin, &controllersMenu, currentNode, EMotionFX::AnimGraphNode::CATEGORY_CONTROLLERS);
                RegisterItems(plugin, &logicMenu,      currentNode, EMotionFX::AnimGraphNode::CATEGORY_LOGIC);
                RegisterItems(plugin, &mathMenu,       currentNode, EMotionFX::AnimGraphNode::CATEGORY_MATH);
                RegisterItems(plugin, &miscMenu,       currentNode, EMotionFX::AnimGraphNode::CATEGORY_MISC);

                if (sourcesMenu.isEmpty() == false)
                {
                    createNodeMenu.addMenu(&sourcesMenu);
                }
                if (blendingMenu.isEmpty() == false)
                {
                    createNodeMenu.addMenu(&blendingMenu);
                }
                if (controllersMenu.isEmpty() == false)
                {
                    createNodeMenu.addMenu(&controllersMenu);
                }
                if (logicMenu.isEmpty() == false)
                {
                    createNodeMenu.addMenu(&logicMenu);
                }
                if (mathMenu.isEmpty() == false)
                {
                    createNodeMenu.addMenu(&mathMenu);
                }
                if (miscMenu.isEmpty() == false)
                {
                    createNodeMenu.addMenu(&miscMenu);
                }
                if (createNodeMenu.isEmpty() == false)
                {
                    menu.addMenu(&createNodeMenu);
                }

                // show the menu at the given position
                if (menu.isEmpty() == false)
                {
                    if (menu.exec(globalMousePos))
                    {
                        // update the view widget
                        viewWidget->Update();
                    }
                    return;
                }
            }
        }

        // get the number of selected nodes
        const uint32 numSelectedNodes = selectedNodes.GetLength();

        // check if we have only one selected node
        if (numSelectedNodes == 1 && selectedNodes[0])
        {
            // get our selected node
            EMotionFX::AnimGraphNode* animGraphNode = selectedNodes[0];

            // create the context menu
            QMenu menu(parentWidget);

            if (animGraphNode->GetParentNode())
            {
                // if the parent is a state machine
                if (azrtti_typeid(animGraphNode->GetParentNode()) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
                {
                    QAction* activateNodeAction = menu.addAction("Activate State");
                    activateNodeAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/Run.png"));
                    connect(activateNodeAction, &QAction::triggered, viewWidget, &BlendGraphViewWidget::OnActivateState);

                    // get the parent state machine
                    EMotionFX::AnimGraphStateMachine* stateMachine = (EMotionFX::AnimGraphStateMachine*)animGraphNode->GetParentNode();
                    if (stateMachine->GetEntryState() != animGraphNode)
                    {
                        QAction* setAsEntryStateAction = menu.addAction("Set As Entry State");
                        setAsEntryStateAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/EntryState.png"));
                        connect(setAsEntryStateAction, &QAction::triggered, navigateWidget, &NavigateWidget::OnSetAsEntryState);
                    }

                    // action for adding a wildcard transition
                    QAction* addWildcardAction = menu.addAction("Add Wildcard Transition");
                    addWildcardAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));
                    connect(addWildcardAction, SIGNAL(triggered()), navigateWidget, SLOT(OnAddWildCardTransition()));
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
                            connect(enableAction, SIGNAL(triggered()), (QObject*)navigateWidget, SLOT(EnableSelected()));
                        }
                        else
                        {
                            QAction* disableAction = menu.addAction("Disable Node");
                            disableAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/DisableNodes.png"));
                            connect(disableAction, SIGNAL(triggered()), (QObject*)navigateWidget, SLOT(DisableSelected()));
                        }
                    }
                }
            }

            // if the parent is a state blend tree
            if (animGraphNode->GetParentNode())
            {
                if (azrtti_typeid(animGraphNode->GetParentNode()) == azrtti_typeid<EMotionFX::BlendTree>())
                {
                    if (animGraphNode->GetSupportsVisualization())
                    {
                        menu.addSeparator();
                        QAction* action = menu.addAction("Adjust Visualization Color");
                        action->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Rendering/Texturing.png"));
                        connect(action, SIGNAL(triggered()), (QObject*)navigateWidget, SLOT(OnVisualizeOptions()));
                    }
                }
            }

            if (menu.isEmpty() == false)
            {
                menu.addSeparator();
            }

            // we can only go to the selected node in case the selected node has a visual graph (state machine / blend tree)
            if (animGraphNode->GetHasVisualGraph())
            {
                QAction* goToNodeAction = menu.addAction("Open Selected Node");
                goToNodeAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/OpenNode.png"));
                connect(goToNodeAction, SIGNAL(triggered()), navigateWidget, SLOT(NavigateToNode()));
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
                        connect(virtualAction, SIGNAL(triggered()), navigateWidget, SLOT(MakeVirtualFinalNode()));
                        menu.addSeparator();
                    }
                    else
                    {
                        if (blendTree->GetFinalNode() != animGraphNode)
                        {
                            QAction* virtualAction = menu.addAction("Restore Final Output");
                            virtualAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/Run.png"));
                            connect(virtualAction, SIGNAL(triggered()), navigateWidget, SLOT(RestoreVirtualFinalNode()));
                            menu.addSeparator();
                        }
                    }
                }
            }

            if (animGraphNode->GetIsDeletable())
            {
                // cut and copy actions
                QAction* cutAction = menu.addAction("Cut");
                cutAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Cut.png"));
                connect(cutAction, SIGNAL(triggered()), navigateWidget, SLOT(Cut()));

                QAction* ccopyAction = menu.addAction("Copy");
                ccopyAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Copy.png"));
                connect(ccopyAction, SIGNAL(triggered()), navigateWidget, SLOT(Copy()));
                menu.addSeparator();

                QAction* removeNodeAction = menu.addAction("Delete Node");
                removeNodeAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Remove.png"));
                connect(removeNodeAction, SIGNAL(triggered()), navigateWidget, SLOT(OnDeleteSelectedNodes()));
                menu.addSeparator();
            }

            // add the node group sub menu
            AddNodeGroupSubmenu(&menu, animGraphNode->GetAnimGraph(), selectedNodes);

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

            QAction* alignLeftAction    = menu.addAction("Align Left");
            QAction* alignRightAction   = menu.addAction("Align Right");
            QAction* alignTopAction     = menu.addAction("Align Top");
            QAction* alignBottomAction  = menu.addAction("Align Bottom");

            alignLeftAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/AlignLeft.png"));
            alignRightAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/AlignRight.png"));
            alignTopAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/AlignTop.png"));
            alignBottomAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/AlignBottom.png"));

            connect(alignLeftAction,    SIGNAL(triggered()), (QObject*)viewWidget, SLOT(AlignLeft()));
            connect(alignRightAction,   SIGNAL(triggered()), (QObject*)viewWidget, SLOT(AlignRight()));
            connect(alignTopAction,     SIGNAL(triggered()), (QObject*)viewWidget, SLOT(AlignTop()));
            connect(alignBottomAction,  SIGNAL(triggered()), (QObject*)viewWidget, SLOT(AlignBottom()));

            menu.addSeparator();

            QAction* zoomSelectionAction = menu.addAction("Zoom Selection");
            zoomSelectionAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/FitSelected.png"));
            connect(zoomSelectionAction, SIGNAL(triggered()), (QObject*)viewWidget, SLOT(ZoomSelected()));

            menu.addSeparator();

            // check if all nodes selected have a blend tree as parent and if they all support disabling/enabling
            bool allBlendTreeNodes = true;
            for (uint32 i = 0; i < numSelectedNodes; ++i)
            {
                // make sure its a node inside a blend tree and that it supports disabling
                if (selectedNodes[i]->GetParentNode() == nullptr || azrtti_typeid(selectedNodes[i]->GetParentNode()) != azrtti_typeid<EMotionFX::BlendTree>())
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
                for (uint32 i = 0; i < numSelectedNodes; ++i)
                {
                    if (selectedNodes[i]->GetSupportsDisable())
                    {
                        oneSupportsDelete = true;
                        if (selectedNodes[i]->GetIsEnabled())
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
                    connect(enableAction, SIGNAL(triggered()), (QObject*)navigateWidget, SLOT(EnableSelected()));
                }

                // disable all nodes
                if (oneEnabled && oneSupportsDelete)
                {
                    QAction* disableAction = menu.addAction("Disable Nodes");
                    disableAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/DisableNodes.png"));
                    connect(disableAction, SIGNAL(triggered()), (QObject*)navigateWidget, SLOT(DisableSelected()));
                }

                menu.addSeparator();
            }

            // check if we need to disable the delete nodes option as an undeletable node is selected
            bool canDelete = false;
            const uint32 numNodes = nodeGraph->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                if (nodeGraph->GetNode(i)->GetIsSelected() && nodeGraph->GetNode(i)->GetIsDeletable())
                {
                    canDelete = true;
                    break;
                }
            }

            if (canDelete)
            {
                menu.addSeparator();

                // cut and copy actions
                QAction* cutAction = menu.addAction("Cut");
                cutAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Cut.png"));
                connect(cutAction, SIGNAL(triggered()), navigateWidget, SLOT(Cut()));

                QAction* ccopyAction = menu.addAction("Copy");
                ccopyAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Copy.png"));
                connect(ccopyAction, SIGNAL(triggered()), navigateWidget, SLOT(Copy()));

                menu.addSeparator();

                QAction* removeNodesAction = menu.addAction("Delete Nodes");
                removeNodesAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Remove.png"));
                connect(removeNodesAction, SIGNAL(triggered()), navigateWidget, SLOT(OnDeleteSelectedNodes()));

                menu.addSeparator();
            }

            // add the node group sub menu
            AddNodeGroupSubmenu(&menu, selectedNodes[0]->GetAnimGraph(), selectedNodes);

            // show the menu at the given position
            if (menu.isEmpty() == false)
            {
                menu.exec(globalMousePos);
            }
        }
    }
}