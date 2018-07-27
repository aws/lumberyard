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

#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <AzCore/std/containers/vector.h>
#include "NavigateWidget.h"
#include "AnimGraphPlugin.h"
#include "GraphNode.h"
#include "BlendGraphViewWidget.h"
#include "BlendGraphWidget.h"
#include "BlendTreeVisualNode.h"
#include "NodePaletteWidget.h"
#include "AttributesWindow.h"
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MetricsEventSender.h>

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QContextMenuEvent>
#include <QMessageBox>
#include <QColorDialog>
#include <QHeaderView>

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/ActorInstance.h>

#include "../../../../EMStudioSDK/Source/EMStudioManager.h"

#include <MCore/Source/LogManager.h>
#include <MCore/Source/CommandGroup.h>
#include <MCore/Source/StringConversions.h>

#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>


namespace EMStudio
{
    // constructor
    NavigateWidget::NavigateWidget(AnimGraphPlugin* plugin, QWidget* parent)
        : QWidget(parent)
    {
        mPlugin             = plugin;
        mVisibleItem        = nullptr;
        mRootItem           = nullptr;
        mVisualColorNode    = nullptr;
        mVisualOptionsNode  = nullptr;

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setMargin(0);
        //mainLayout->setContentsMargins(0, 0, 0, 10);
        mainLayout->setSpacing(2);
        setLayout(mainLayout);
        mainLayout->setSizeConstraint(QLayout::SetNoConstraint);

        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &NavigateWidget::OnTextFilterChanged);
        mainLayout->addWidget(m_searchWidget);

        // create the tree widget
        mTreeWidget = new QTreeWidget();
        connect(mTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(OnSelectionChanged()));
        connect(mTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(NavigateToNode(QTreeWidgetItem*, int)));
        connect(mTreeWidget, SIGNAL(itemPressed(QTreeWidgetItem*, int)), this, SLOT(OnItemClicked(QTreeWidgetItem*, int)));
        mainLayout->addWidget(mTreeWidget);

        // setup the tree widget
        mTreeWidget->setColumnCount(2);
        QStringList headerList;
        headerList.append("Name");
        headerList.append("Type");
        mTreeWidget->setHeaderLabels(headerList);
        mTreeWidget->setColumnWidth(COLUMN_NAME, 250);
        mTreeWidget->setColumnWidth(COLUMN_TYPE, 50);
        mTreeWidget->setSortingEnabled(false);
        mTreeWidget->setDragEnabled(false);
        //  mTreeWidget->setSelectionMode( QAbstractItemView::SingleSelection );
        mTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mTreeWidget->setMinimumWidth(100);
        mTreeWidget->setMinimumHeight(100);
        mTreeWidget->setMaximumHeight(1000);
        mTreeWidget->setAlternatingRowColors(true);
        mTreeWidget->setAnimated(true);
        mTreeWidget->setAcceptDrops(false);
        mTreeWidget->setContextMenuPolicy(Qt::DefaultContextMenu);
        mTreeWidget->setExpandsOnDoubleClick(false);

        // disable the move of section to have column order fixed
        mTreeWidget->header()->setSectionsMovable(false);

        mItemClicked = nullptr;

        // update things based on default selection
        OnSelectionChanged();

        EMotionFX::AnimGraphNotificationBus::Handler::BusConnect();
    }


    NavigateWidget::~NavigateWidget()
    {
        EMotionFX::AnimGraphNotificationBus::Handler::BusDisconnect();
    }


    void NavigateWidget::OnSyncVisualObject(EMotionFX::AnimGraphObject* object)
    {
        if (!object)
        {
            AZ_Assert(false, "EMotionFX: Cannot sync visual object. Invalid object.");
            return;
        }

        if (azrtti_istypeof<EMotionFX::AnimGraphNode>(object))
        {
            const EMotionFX::AnimGraphNode* node = static_cast<EMotionFX::AnimGraphNode*>(object);
            QTreeWidgetItem* item = FindItemById(node->GetId());
            if (item)
            {
                item->setText(0, node->GetName());
            }
        }
    }


    void NavigateWidget::UpdateTreeWidget(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* nodeToShow)
    {
        mVisibleItem = nullptr;
        mItemClicked = nullptr;

        m_searchWidget->ClearTextFilter();
        m_searchStringText.clear();

        mRootItem = UpdateTreeWidget(mTreeWidget, animGraph);

        ChangeVisibleItem(nullptr);

        if (animGraph)
        {
            mPlugin->ShowGraph(nodeToShow, animGraph, false);
            ChangeVisibleItem(nodeToShow);
        }

        mPlugin->GetAttributesWindow()->InitForAnimGraphObject(nullptr);
    }


    // init for a given anim graph
    QTreeWidgetItem* NavigateWidget::UpdateTreeWidget(QTreeWidget* treeWidget, EMotionFX::AnimGraph* animGraph, const AZ::TypeId& visibilityFilterNodeType, bool showStatesOnly, const char* searchFilterString)
    {
        // first clear all contents
        treeWidget->clear();
        if (animGraph == nullptr)
        {
            return nullptr;
        }

        // add the root state machine
        return InsertNode(treeWidget, nullptr, animGraph->GetRootStateMachine(), true, visibilityFilterNodeType, showStatesOnly, searchFilterString);
    }


    // recursively insert a node
    QTreeWidgetItem* NavigateWidget::InsertNode(QTreeWidget* treeWidget, QTreeWidgetItem* parentItem, EMotionFX::AnimGraphNode* node, bool recursive, const AZ::TypeId& visibilityFilterNodeType, bool showStatesOnly, const char* searchFilterString)
    {
        QTreeWidgetItem* newItem = parentItem;

        bool allowItem = true;
        if (!visibilityFilterNodeType.IsNull())
        {
            if (node)
            {
                if (visibilityFilterNodeType != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
                {
                    if (azrtti_typeid(node) != visibilityFilterNodeType)
                    {
                        allowItem = false;
                    }
                }
                else
                {
                    EMotionFX::AnimGraphNode* parentNode = node->GetParentNode();
                    if (parentNode == nullptr || (parentNode && azrtti_typeid(parentNode) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>()))
                    {
                        allowItem = false;
                    }
                }
            }
            else
            {
                allowItem = false;
            }
        }

        if (showStatesOnly && node)
        {
            if (node->GetCanActAsState() == false)
            {
                allowItem = false;
            }
        }

        AZStd::string loweredNodeName = node->GetNameString();
        AZStd::to_lower(loweredNodeName.begin(), loweredNodeName.end());
        if (allowItem && (searchFilterString == nullptr || (node && searchFilterString && loweredNodeName.find(searchFilterString) != AZStd::string::npos)))
        {
            newItem = new QTreeWidgetItem(parentItem, 0);
            newItem->setData(0, Qt::UserRole, node->GetId().ToString().c_str());
            
            if (node == nullptr)
            {
                newItem->setText(COLUMN_NAME, "Root");
                newItem->setText(COLUMN_TYPE, "");
            }
            else
            {
                newItem->setText(COLUMN_NAME, node->GetName());
                newItem->setText(COLUMN_TYPE, node->GetPaletteName());

                QIcon icon(NodePaletteWidget::GetNodeIconFileName(node).c_str());
                newItem->setIcon(COLUMN_NAME, icon);
            }

            newItem->setExpanded(true);
            
            if (parentItem == nullptr)
            {
                treeWidget->addTopLevelItem(newItem);
            }
            else
            {
                parentItem->setExpanded(true);
            }
        }

        // add all child nodes
        if (recursive && node)
        {
            const uint32 numChildNodes = node->GetNumChildNodes();
            for (uint32 i = 0; i < numChildNodes; ++i)
            {
                InsertNode(treeWidget, newItem, node->GetChildNode(i), true, visibilityFilterNodeType, showStatesOnly, searchFilterString);
            }

            if (newItem)
            {
                if (azrtti_typeid(node) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
                {
                    newItem->setExpanded(true);
                }
                else
                {
                    newItem->setExpanded(false);
                }
            }
        }

        return newItem;
    }


    // remove a given row from the tree
    void NavigateWidget::RemoveTreeRow(const QString& name)
    {
        QTreeWidgetItem* item = FindItem(name);
        if (item == nullptr)
        {
            return;
        }

        if (item == mVisibleItem)
        {
            mVisibleItem = nullptr;
        }

        if (item == mItemClicked)
        {
            mItemClicked = nullptr;
        }

        delete item;
        /*
            if (item->parent())
            {
                MCore::LogDebug("Removing child from node '%s'", item->parent()->text(0).toAscii().data());
                item->parent()->removeChild( item );
            }
        */
    }


    // find the item with a given name
    QTreeWidgetItem* NavigateWidget::FindItem(const QString& name)
    {
        QList<QTreeWidgetItem*> items = mTreeWidget->findItems(name, Qt::MatchExactly | Qt::MatchCaseSensitive | Qt::MatchRecursive, COLUMN_NAME);
        AZ_Assert(items.count() <= 1, "Expected only one element with the same name");
        if (items.count() == 0)
        {
            return nullptr;
        }

        return items.at(0);
    }


    QTreeWidgetItem* NavigateWidget::FindItemById(const EMotionFX::AnimGraphNodeId& id) const
    {
        QTreeWidgetItemIterator iterator(mTreeWidget);

        QString idString;
        while (*iterator)
        {
            QTreeWidgetItem* item = *iterator;
            idString = item->data(0, Qt::UserRole).toString();

            const EMotionFX::AnimGraphNodeId itemId = EMotionFX::AnimGraphNodeId::FromString(idString.toUtf8().data());
            if (id == itemId)
            {
                return item;
            }

            ++iterator;
        }

        return nullptr;
    }


    // when the selection changes
    void NavigateWidget::OnSelectionChanged()
    {
        BlendGraphWidget* blendGraphWidget = mPlugin->GetGraphWidget();

        // start block signals for the navigate tree widget
        blendGraphWidget->blockSignals(true);
        {
            NodeGraph* nodeGraph = blendGraphWidget->GetActiveGraph();
            if (nodeGraph)
            {
                // iterate over all nodes
                const uint32 numNodes = nodeGraph->GetNumNodes();
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    GraphNode* node = nodeGraph->GetNode(i);
                    QTreeWidgetItem* item = FindItem(node->GetName());
                    if (item)
                    {
                        node->SetIsSelected(item->isSelected());
                    }
                }
            }
        }
        // end block signals for the navigate tree widget
        blendGraphWidget->blockSignals(false);

        OnSelectionChangedWithoutGraphUpdate();
    }


    void NavigateWidget::OnSelectionChangedWithoutGraphUpdate()
    {
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        BlendGraphWidget* graphWidget = mPlugin->GetGraphWidget();
        if (animGraph == nullptr)
        {
            graphWidget->ShowNothing();
            return;
        }

        // if we selected more than one item, or nothing at all, then show nothing
        QList<QTreeWidgetItem*> selectedItems = mTreeWidget->selectedItems();
        const uint32 numSelected = selectedItems.count();

        // update the menu of the blend graph view widget
        mPlugin->GetViewWidget()->Update();

        // init the attribute window
        if (numSelected != 1)
        {
            NodeGraph* nodeGraph = graphWidget->GetActiveGraph();
            if (nodeGraph)
            {
                const uint32 nodeGraphIndex = mPlugin->FindGraphInfo(nodeGraph, animGraph);
                if (nodeGraphIndex != MCORE_INVALIDINDEX32)
                {
                    const AnimGraphPlugin::GraphInfo* graphInfo = mPlugin->GetGraphInfo(nodeGraphIndex);
                    MCORE_ASSERT(graphInfo);
                    EMotionFX::AnimGraphNode* emfxNode = animGraph->RecursiveFindNodeById(graphInfo->mNodeId);
                    if (emfxNode == nullptr || azrtti_typeid(emfxNode) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
                    {
                        mPlugin->GetAttributesWindow()->InitForAnimGraphObject(nullptr);
                        return;
                    }

                    uint32 numSelectedConnections =  nodeGraph->CalcNumSelectedConnections();
                    if (numSelectedConnections != 1)
                    {
                        mPlugin->GetAttributesWindow()->InitForAnimGraphObject(nullptr);
                    }
                    else
                    {
                        NodeConnection* nodeConnection = nodeGraph->FindFirstSelectedConnection();
                        MCORE_ASSERT(nodeConnection);
                        EMotionFX::AnimGraphStateTransition* transition = graphWidget->FindTransitionForConnection(nodeConnection);
                        mPlugin->GetAttributesWindow()->InitForAnimGraphObject((EMotionFX::AnimGraphObject*)transition);
                    }
                }
            }
        }
        else
        {
            EMotionFX::AnimGraphNode* selectedNode = animGraph->RecursiveFindNodeByName(FromQtString(selectedItems[0]->text(COLUMN_NAME)).c_str());
            mPlugin->GetAttributesWindow()->InitForAnimGraphObject(selectedNode);
        }
    }


    // rename a given node in the tree
    void NavigateWidget::Rename(const char* oldName, const char* newName)
    {
        QTreeWidgetItem* item = FindItem(oldName);
        if (item == nullptr)
        {
            return;
        }

        item->setText(COLUMN_NAME, newName);
    }


    // when clicking an item
    void NavigateWidget::OnItemClicked(QTreeWidgetItem* item, int column)
    {
        MCORE_UNUSED(item);
        MCORE_UNUSED(column);
    }

    MCore::Array<EMotionFX::AnimGraphNode*> NavigateWidget::GetSelectedNodes(EMotionFX::AnimGraph* animGraph)
    {
        // get the array of selected anim graph nodes
        MCore::Array<EMotionFX::AnimGraphNode*> selectedNodes;

        // if nothing is selected, don't show any menu
        QList<QTreeWidgetItem*> selectedItems = mTreeWidget->selectedItems();
        const uint32 numSelectedItems = selectedItems.count();
        if (numSelectedItems == 0)
        {
            return selectedNodes;
        }

        selectedNodes.Reserve(numSelectedItems);
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            // don't process the root item
            if (selectedItems[i] == mRootItem)
            {
                continue;
            }

            EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeByName(FromQtString(selectedItems.at(i)->text(COLUMN_NAME)).c_str());
            if (animGraphNode)
            {
                selectedNodes.Add(animGraphNode);
            }
        }

        return selectedNodes;
    }


    // create the context menu
    void NavigateWidget::contextMenuEvent(QContextMenuEvent* event)
    {
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        MCore::Array<EMotionFX::AnimGraphNode*> selectedNodes = GetSelectedNodes(animGraph);
        mPlugin->GetGraphWidget()->OnContextMenuEvent(this, event->pos(), event->globalPos(), mPlugin, selectedNodes, false);
    }


    // when we want to rename items
    void NavigateWidget::OnContextMenuRename()
    {
    }


    void NavigateWidget::keyReleaseEvent(QKeyEvent* event)
    {
        // delete selected items
        switch (event->key())
        {
        // when pressing delete, delete the selected items
        case Qt::Key_Delete:
            OnDeleteSelectedNodes();
            break;
        }
    }


    // when we want to delete items
    void NavigateWidget::OnDeleteSelectedNodes()
    {
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (!animGraph)
        {
            return;
        }

        const QList<QTreeWidgetItem*>& selectedItems = mTreeWidget->selectedItems();
        const uint32 numSelected = selectedItems.count();
        if (numSelected == 0)
        {
            return;
        }

        // Prepare the list of nodes to remove.
        QString nodeName;
        AZStd::vector<AZStd::string> selectedNodeNames;
        for (uint32 i = 0; i < numSelected; ++i)
        {
            nodeName = selectedItems.at(i)->text(COLUMN_NAME);

            // Don't add the root.
            if (nodeName != "Root")
            {
                selectedNodeNames.push_back(nodeName.toUtf8().data());
            }
        }

        CommandSystem::DeleteNodes(animGraph, selectedNodeNames);

        // Send LyMetrics event.
        MetricsEventSender::SendDeleteNodesAndConnectionsEvent(static_cast<AZ::u32>(selectedNodeNames.size()), 0);
    }


    void NavigateWidget::NavigateToNode()
    {
        NavigateToNode(nullptr, COLUMN_NAME);
    }


    // when showing the graph
    void NavigateWidget::NavigateToNode(QTreeWidgetItem* item, int column)
    {
        MCORE_UNUSED(item);
        MCORE_UNUSED(column);

        // get the array of selected items
        QList<QTreeWidgetItem*> selectedItems = mTreeWidget->selectedItems();
        if (selectedItems.count() != 1)
        {
            return;
        }

        // change the graph
        QTreeWidgetItem* selectedItem = selectedItems[0];
        ShowGraphByNodeName(selectedItem->text(COLUMN_NAME).toUtf8().data(), true);
    }


    void NavigateWidget::ShowGraphByNodeName(const AZStd::string& nodeName, bool updateInterface)
    {
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeByName(nodeName.c_str());
        if (!node)
        {
            ShowGraph(nullptr, updateInterface);
        }
        else if (node->GetNumChildNodes() > 0)
        {
            ShowGraph(node, updateInterface);
        }
        else
        {
            ShowGraph(node->GetParentNode(), updateInterface);

            // select the item for this node
            mTreeWidget->clearSelection();
            QTreeWidgetItem* item = FindItem(node->GetName());
            MCORE_ASSERT(item);
            //mTree->scrollToItem( item );
            item->setSelected(true);

            // select it in the visual graph as well and zoom in on the node in the graph
            GraphNode* graphNode;
            NodeGraph* graph;
            if (mPlugin->FindGraphAndNode(node->GetParentNode(), node->GetId(), &graph, &graphNode, animGraph))
            {
                graphNode->SetIsSelected(true);
                graph->ZoomOnRect(graphNode->GetRect(), mPlugin->GetGraphWidget()->geometry().width(), mPlugin->GetGraphWidget()->geometry().height(), true);
            }

            OnSelectionChanged();
        }
    }


    void NavigateWidget::ShowGraph(EMotionFX::AnimGraphNode* node, bool updateInterface)
    {
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            ChangeVisibleItem(nullptr);
            mPlugin->GetGraphWidget()->ShowNothing();
            return;
        }

        // in case the node is null this means we want to show the root node
        if (node == nullptr)
        {
            node = animGraph->GetRootStateMachine();
        }

        // init the attribute window
        //  mPlugin->GetAttributesWindow()->InitForAnimGraphObject( nullptr );
        mPlugin->GetAttributesWindow()->InitForAnimGraphObject(node);

        mPlugin->ShowGraph(node, animGraph);
        ChangeVisibleItem(node);

        if (updateInterface)
        {
            // find the tree widget item we have to select
            QTreeWidgetItem* item = FindItem(node->GetName());
            mTreeWidget->setCurrentItem(item);
        }
    }


    // change to the given item (mark it)
    void NavigateWidget::ChangeVisibleItem(EMotionFX::AnimGraphNode* node)
    {
        // remove the bold from the old item
        if (mVisibleItem)
        {
            //mVisibleItem->setBackgroundColor(COLUMN_NAME, QColor(20, 20, 20));
            //mVisibleItem->setBackgroundColor(COLUMN_TYPE, QColor(20, 20, 20));
            QFont font = mVisibleItem->font(COLUMN_NAME);
            font.setBold(false);
            mVisibleItem->setFont(COLUMN_NAME, font);
            mVisibleItem->setFont(COLUMN_TYPE, font);
        }

        // if nothing is selected, return
        //if (node == nullptr)
        //return;

        // make the new item bold
        QTreeWidgetItem* newItem;
        if (node)
        {
            newItem = FindItem(node->GetName());
        }
        else
        {
            newItem = FindItem("Root");
        }

        if (newItem)
        {
            mVisibleItem = newItem;
            //mVisibleItem->setBackgroundColor(COLUMN_NAME, QColor(100, 0, 100));
            //mVisibleItem->setBackgroundColor(COLUMN_TYPE, QColor(100, 0, 100));
            QFont font = mVisibleItem->font(COLUMN_NAME);
            font.setBold(true);
            mVisibleItem->setFont(COLUMN_NAME, font);
            mVisibleItem->setFont(COLUMN_TYPE, font);
        }
    }


    void NavigateWidget::OnAddWildCardTransition()
    {
        // find the selected node
        EMotionFX::AnimGraphNode* node = GetSingleSelectedNode();
        if (node == nullptr)
        {
            return;
        }

        // get the active anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // get the currently selected transition type
        //uint32 transitionType = mPlugin->GetPaletteWidget()->GetTransitionType();
        AZ::TypeId transitionType = azrtti_typeid<EMotionFX::AnimGraphStateTransition>();

        // space the wildcard transitions in case we're dealing with multiple ones
        uint32 endOffsetX = 0;
        uint32 endOffsetY = 0;
        EMotionFX::AnimGraphNode* parentNode = node->GetParentNode();
        if (parentNode && azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);

            const uint32    numWildcardTransitions  = stateMachine->CalcNumWildcardTransitions(node);
            const bool      isEven                  = (numWildcardTransitions % 2 == 0);
            const uint32    evenTransitions         = numWildcardTransitions / 2;

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
        }

        // build the command string
        AZStd::string commandString;
        AZStd::string commandResult;
        commandString = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"\" -targetNode \"%s\"  -sourcePort 0 -targetPort 0 -startOffsetX 0 -startOffsetY 0 -endOffsetX %i -endOffsetY %i -transitionType \"%s\"", animGraph->GetID(), node->GetName(), endOffsetX, endOffsetY, transitionType.ToString<AZStd::string>().c_str());

        // execute the command
        if (GetCommandManager()->ExecuteCommand(commandString.c_str(), commandResult) == false)
        {
            if (commandResult.empty() == false)
            {
                MCore::LogError(commandResult.c_str());
            }
        }
    }


    void NavigateWidget::OnSetAsEntryState()
    {
        // find the selected node
        EMotionFX::AnimGraphNode* node = GetSingleSelectedNode();
        if (node == nullptr)
        {
            return;
        }

        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // execute the command
        AZStd::string commandResult;
        if (GetCommandManager()->ExecuteCommand(AZStd::string::format("AnimGraphSetEntryState -animGraphID %i -entryNodeName \"%s\"", animGraph->GetID(), node->GetName()).c_str(), commandResult) == false)
        {
            MCore::LogError(commandResult.c_str());
        }
    }


    EMotionFX::AnimGraphNode* NavigateWidget::GetSingleSelectedNode()
    {
        // get the array of selected items
        QList<QTreeWidgetItem*> selectedItems = mTreeWidget->selectedItems();
        if (selectedItems.count() != 1)
        {
            return nullptr;
        }

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        MCORE_ASSERT(animGraph);

        // find the selected node
        EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeByName(FromQtString(selectedItems.at(0)->text(COLUMN_NAME)).c_str());
        return node;
    }


    void NavigateWidget::FillPasteCommandGroup(bool cutMode)
    {
        mNodeIdsToCopy.clear();

        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (!animGraph)
        {
            return;
        }

        NodeGraph* nodeGraph = mPlugin->GetGraphWidget()->GetActiveGraph();
        if (!nodeGraph)
        {
            return;
        }

        mCopyParentNodeType = AZ::TypeId::CreateNull();

        // Get the selected items and return in case there is nothing selected.
        QList<QTreeWidgetItem*> selectedItems = mPlugin->GetNavigateWidget()->GetTreeWidget()->selectedItems();
        const uint32 numSelected = selectedItems.count();

        // Iterate through the selected nodes and delete them.
        for (uint32 i = 0; i < numSelected; ++i)
        {
            QTreeWidgetItem*           item  = selectedItems.at(i);
            EMotionFX::AnimGraphNode*  node  = animGraph->RecursiveFindNodeByName(FromQtString(item->text(COLUMN_NAME)).c_str());

            // Only add it if the node is valid and is not in the list yet.
            if (node)
            {
                mNodeIdsToCopy.emplace(node->GetId());

                // Check if the node has a parent, if yes store the parent node type (so that we know at paste time if we're pasting states or blend tree nodes).
                EMotionFX::AnimGraphNode* parentNode = node->GetParentNode();
                if (parentNode)
                {
                    mCopyParentNodeType = azrtti_typeid(parentNode);
                }
                else
                {
                    MCORE_ASSERT(false);
                }
            }
        }

        mCutMode = cutMode;
    }


    void NavigateWidget::Cut()      { FillPasteCommandGroup(true); }
    void NavigateWidget::Copy()     { FillPasteCommandGroup(false); }


    bool NavigateWidget::GetIsReadyForPaste()
    {
        return !mNodeIdsToCopy.empty();
    }


    void NavigateWidget::Paste()
    {
        if (!GetIsReadyForPaste())
        {
            return;
        }

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // get the current mouse position and check if it is inside the widget at all
        BlendGraphWidget*   blendGraphWidget    = mPlugin->GetGraphWidget();
        QRect               widgetRect          = blendGraphWidget->rect();
        //MCore::LOG("mousePos: (%d, %d), rect: (%d, %d, %d, %d)", mousePos.x(), mousePos.y(), widgetRect.left(), widgetRect.top(), widgetRect.right(), widgetRect.bottom());
        if (widgetRect.contains(blendGraphWidget->mapFromGlobal(QCursor::pos())) == false)
        {
            return;
        }

        NodeGraph* nodeGraph = mPlugin->GetGraphWidget()->GetActiveGraph();
        if (nodeGraph == nullptr)
        {
            return;
        }

        QPoint mousePos = blendGraphWidget->GetMousePos();
        // when enabling this CTRL+V doesn't work good anymore
        //QPoint mousePos = blendGraphWidget->LocalToGlobal( blendGraphWidget->GetContextMenuEventMousePos() );

        // use the currently visible node in the graph widget as parent name
        EMotionFX::AnimGraphNode* currentNode = blendGraphWidget->GetCurrentNode();
        AZStd::string parentName;
        if (currentNode)
        {
            parentName = currentNode->GetName();
        }

        // check if the node we want to paste into is of the same type of where we copied from, if not don't copy along the connections/transitions
        // create a cloned version of the node names to copy, so that we can copy another time without recreating the copy information
        MCore::CommandGroup pasteCommandGroup;
        if (mCutMode)
        {
            pasteCommandGroup.SetGroupName("Cut and paste nodes");
        }
        else
        {
            pasteCommandGroup.SetGroupName("Copy and paste nodes");
        }

        // Convert the node names to copy to pointers
        AZStd::vector<EMotionFX::AnimGraphNode*> nodesTopCopy;
        nodesTopCopy.reserve(mNodeIdsToCopy.size());
        for (const EMotionFX::AnimGraphNodeId nodeId : mNodeIdsToCopy)
        {
            EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeById(nodeId);
            if (node)
            {
                nodesTopCopy.emplace_back(node);
            }
        }
        AZStd::unordered_map<EMotionFX::AnimGraphNode*, AZStd::string> newNamesByCopiedNodes;
        const bool ignoreTopLevelConnections = !(currentNode && (currentNode == animGraph->GetRootStateMachine() || azrtti_typeid(currentNode) == mCopyParentNodeType));

        CommandSystem::ConstructCopyAnimGraphNodesCommandGroup(&pasteCommandGroup, currentNode, nodesTopCopy, mousePos.x(), mousePos.y(), mCutMode, newNamesByCopiedNodes, ignoreTopLevelConnections);
        
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(pasteCommandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // clear old command group so that we can't paste again
        if (mCutMode)
        {
            mNodeIdsToCopy.clear();
        }

        // Send LyMetrics event.
        MetricsEventSender::SendPasteNodesAndConnectionsEvent(static_cast<AZ::u32>(nodesTopCopy.size()), 0);
    }


    void NavigateWidget::dragEnterEvent(QDragEnterEvent* event)
    {
        event->accept();
    }


    // when dropping stuff in our window
    void NavigateWidget::dropEvent(QDropEvent* event)
    {
        MCORE_UNUSED(event);
    }


    // drag leave
    void NavigateWidget::dragLeaveEvent(QDragLeaveEvent* event)
    {
        //MCore::LogDebug("NavigateWidget::dragLeave");
        event->accept();
    }


    // moving around while dragging
    void NavigateWidget::dragMoveEvent(QDragMoveEvent* event)
    {
        MCORE_UNUSED(event);
        // MCore::LogDebug("NavigateWidget::dragMove");
    }


    // make the selected node the virtual final node
    void NavigateWidget::MakeVirtualFinalNode()
    {
        // get the array of selected items
        QList<QTreeWidgetItem*> selectedItems = mTreeWidget->selectedItems();
        if (selectedItems.count() != 1)
        {
            return;
        }

        // find the node inside EMotionFX::GetEMotionFX()
        QTreeWidgetItem* selectedItem = selectedItems[0];
        EMotionFX::AnimGraphNode* virtualFinalNode = mPlugin->GetActiveAnimGraph()->RecursiveFindNodeByName(FromQtString(selectedItem->text(COLUMN_NAME)).c_str());
        MCORE_ASSERT(virtualFinalNode);

        // get the parent blend tree
        MCORE_ASSERT(azrtti_typeid(virtualFinalNode->GetParentNode()) == azrtti_typeid<EMotionFX::BlendTree>());
        EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(virtualFinalNode->GetParentNode());

        // set its new virtual final node
        if (virtualFinalNode == blendTree->GetFinalNode())
        {
            blendTree->SetVirtualFinalNode(nullptr);
        }
        else
        {
            blendTree->SetVirtualFinalNode(virtualFinalNode);
        }

        // update the virtual final node
        mPlugin->SetVirtualFinalNode(virtualFinalNode);
    }


    // make the final node the virtual final node
    void NavigateWidget::RestoreVirtualFinalNode()
    {
        // get the array of selected items
        QList<QTreeWidgetItem*> selectedItems = mTreeWidget->selectedItems();
        if (selectedItems.count() != 1)
        {
            return;
        }

        // find the node inside EMotionFX::GetEMotionFX()
        QTreeWidgetItem* selectedItem = selectedItems[0];
        EMotionFX::AnimGraphNode* virtualFinalNode = mPlugin->GetActiveAnimGraph()->RecursiveFindNodeByName(FromQtString(selectedItem->text(COLUMN_NAME)).c_str());
        MCORE_ASSERT(virtualFinalNode);

        // get the parent blend tree
        MCORE_ASSERT(azrtti_typeid(virtualFinalNode->GetParentNode()) == azrtti_typeid<EMotionFX::BlendTree>());
        EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(virtualFinalNode->GetParentNode());

        // set its new virtual final node
        blendTree->SetVirtualFinalNode(nullptr);

        // update the virtual final node
        mPlugin->SetVirtualFinalNode(blendTree->GetFinalNode());
    }


    // enable all selected nodes
    void NavigateWidget::SetEnabledSelectedNodes(bool flag)
    {
        // get the array of selected items
        QList<QTreeWidgetItem*> selectedItems = mTreeWidget->selectedItems();
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        MCORE_ASSERT(animGraph);

        AZStd::string commandString;
        MCore::CommandGroup commandGroup;

        // for all selected nodes
        const uint32 numSelected = selectedItems.count();
        for (uint32 i = 0; i < numSelected; ++i)
        {
            QTreeWidgetItem* selectedItem = selectedItems[i];
            EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeByName(FromQtString(selectedItem->text(COLUMN_NAME)).c_str());
            MCORE_ASSERT(node);
            if (node->GetSupportsDisable() == false)
            {
                continue;
            }

            // build the command string and add it to the command group
            commandString = AZStd::string::format("AnimGraphAdjustNode -animGraphID %i -name \"%s\" -enabled %s", animGraph->GetID(), node->GetName(), AZStd::to_string(flag).c_str());
            commandGroup.AddCommandString(commandString.c_str());
        }

        // execute the command group
        AZStd::string commandResult;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, commandResult) == false)
        {
            if (commandResult.size() > 0)
            {
                MCore::LogError(commandResult.c_str());
            }
        }
    }


    // change visualization options
    void NavigateWidget::OnVisualizeOptions()
    {
        // get the array of selected items
        QList<QTreeWidgetItem*> selectedItems = mTreeWidget->selectedItems();
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        MCORE_ASSERT(animGraph);

        const uint32 numSelected = selectedItems.count();
        if (numSelected != 1 && mVisualOptionsNode == nullptr)
        {
            return;
        }

        // get the emfx node
        EMotionFX::AnimGraphNode* node;
        if (mVisualOptionsNode == nullptr)
        {
            QTreeWidgetItem* selectedItem = selectedItems[0];
            node = animGraph->RecursiveFindNodeByName(FromQtString(selectedItem->text(COLUMN_NAME)).c_str());
        }
        else
        {
            node = mVisualOptionsNode;
        }

        MCORE_ASSERT(node);
        if (node->GetSupportsVisualization() == false)
        {
            return;
        }

        // set the node we're modifying
        mVisualColorNode = node;

        // show the color dialog
        const uint32 color = node->GetVisualizeColor();
        const uint32 orgColor = color;
        const QColor initialColor(MCore::ExtractRed(color), MCore::ExtractGreen(color), MCore::ExtractBlue(color));
        QColorDialog dialog(initialColor, this);
        connect(&dialog, SIGNAL(currentColorChanged(const QColor&)), this, SLOT(OnVisualizeColorChanged(const QColor&)));
        if (dialog.exec() != QDialog::Accepted)
        {
            node->SetVisualizeColor(orgColor);
            OnVisualizeColorChanged(orgColor);
        }
    }


    void NavigateWidget::OnVisualizeColorChanged(const QColor& color)
    {
        mVisualColorNode->SetVisualizeColor(MCore::RGBA(color.red(), color.green(), color.blue()));

        // sync the visual node with EMotionFX::GetEMotionFX()
        NodeGraph* visualGraph;
        GraphNode* visualNode;
        mPlugin->FindGraphAndNode(mVisualColorNode->GetParentNode(), mVisualColorNode->GetId(), &visualGraph, &visualNode, mPlugin->GetActiveAnimGraph());
        visualNode->Sync();
    }


    // when the filter string changed
    void NavigateWidget::OnTextFilterChanged(const QString& text)
    {
        FromQtString(text, &m_searchStringText);
        AZStd::to_lower(m_searchStringText.begin(), m_searchStringText.end());

        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        mVisibleItem = nullptr;

        if (m_searchStringText.size() > 0)
        {
            mRootItem = UpdateTreeWidget(mTreeWidget, animGraph, AZ::TypeId::CreateNull(), false, m_searchStringText.c_str());
        }
        else
        {
            mRootItem = UpdateTreeWidget(mTreeWidget, animGraph, AZ::TypeId::CreateNull(), false, nullptr);
        }

        //  ChangeVisibleItem(nullptr);
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigateWidget.moc>
