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

// include the required headers
#include "NodeHierarchyWidget.h"
#include "EMStudioManager.h"
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/ActorManager.h>
#include <QLabel>
#include <QSizePolicy>
#include <QTreeWidget>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QMenu>
#include <QHeaderView>


namespace EMStudio
{
    // constructor
    NodeHierarchyWidget::NodeHierarchyWidget(QWidget* parent, bool useSingleSelection)
        : QWidget(parent)
    {
        /*  // create and register the command callbacks only
            mSelectCallback             = new CommandSelectCallback(false);
            mUnselectCallback           = new CommandUnselectCallback(false);
            mClearSelectionCallback     = new CommandClearSelectionCallback(false);

            GetCommandManager()->RegisterCommandCallback( "Select", mSelectCallback );
            GetCommandManager()->RegisterCommandCallback( "Unselect", mUnselectCallback );
            GetCommandManager()->RegisterCommandCallback( "ClearSelection", mClearSelectionCallback );*/

        mBoneIcon       = new QIcon(MCore::String(MysticQt::GetDataDir() + "Images/Icons/Bone.png").AsChar());
        mNodeIcon       = new QIcon(MCore::String(MysticQt::GetDataDir() + "Images/Icons/Node.png").AsChar());
        mMeshIcon       = new QIcon(MCore::String(MysticQt::GetDataDir() + "Images/Icons/Mesh.png").AsChar());
        mCharacterIcon  = new QIcon(MCore::String(MysticQt::GetDataDir() + "Images/Icons/Character.png").AsChar());

        mActorInstanceIDs.SetMemoryCategory(MEMCATEGORY_EMSTUDIOSDK);

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);

        // create the display layout
        QHBoxLayout* displayLayout = new QHBoxLayout();

        displayLayout->addWidget(new QLabel("Display:"));

        mDisplayButtonGroup     = new MysticQt::ButtonGroup(this, 1, 3);
        mDisplayMeshesButton    = mDisplayButtonGroup->GetButton(0, 0);
        mDisplayNodesButton     = mDisplayButtonGroup->GetButton(0, 1);
        mDisplayBonesButton     = mDisplayButtonGroup->GetButton(0, 2);

        mDisplayMeshesButton->setText("Meshes");
        mDisplayNodesButton->setText("Nodes");
        mDisplayBonesButton->setText("Bones");

        mDisplayBonesButton->setIcon(*mBoneIcon);
        mDisplayNodesButton->setIcon(*mNodeIcon);
        mDisplayMeshesButton->setIcon(*mMeshIcon);

        mDisplayMeshesButton->setChecked(true);
        mDisplayNodesButton->setChecked(true);
        mDisplayBonesButton->setChecked(true);

        connect(mDisplayMeshesButton, SIGNAL(clicked()), this, SLOT(Update()));
        connect(mDisplayNodesButton, SIGNAL(clicked()), this, SLOT(Update()));
        connect(mDisplayBonesButton, SIGNAL(clicked()), this, SLOT(Update()));

        displayLayout->addWidget(mDisplayButtonGroup);

        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        displayLayout->addWidget(spacerWidget);

        displayLayout->addWidget(new QLabel("Find:"), 0, Qt::AlignRight);
        mFindWidget = new MysticQt::SearchButton(this, MysticQt::GetMysticQt()->FindIcon("Images/Icons/SearchClearButton.png"));
        connect(mFindWidget->GetSearchEdit(), SIGNAL(textChanged(const QString&)), this, SLOT(TextChanged(const QString&)));
        displayLayout->addWidget(mFindWidget);

        // create the tree widget
        mHierarchy = new QTreeWidget();

        // create header items
        mHierarchy->setColumnCount(5);
        QStringList headerList;
        headerList.append("Name");
        headerList.append("Type");
        headerList.append("Child");
        headerList.append("Poly");
        headerList.append("Mirror");
        mHierarchy->setHeaderLabels(headerList);

        // set optical stuff for the tree
        mHierarchy->setColumnWidth(0, 400);
        mHierarchy->setColumnWidth(1, 63);
        mHierarchy->setColumnWidth(2, 37);
        mHierarchy->setColumnWidth(3, 50);
        mHierarchy->setSortingEnabled(false);
        mHierarchy->setSelectionMode(QAbstractItemView::SingleSelection);
        mHierarchy->setMinimumWidth(670);
        mHierarchy->setMinimumHeight(500);
        mHierarchy->setAlternatingRowColors(true);
        mHierarchy->setExpandsOnDoubleClick(true);
        mHierarchy->setAnimated(true);

        // disable the move of section to have column order fixed
        mHierarchy->header()->setSectionsMovable(false);

        if (useSingleSelection == false)
        {
            mHierarchy->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(mHierarchy, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(TreeContextMenu(const QPoint&)));
        }

        layout->addLayout(displayLayout);
        layout->addWidget(mHierarchy);
        setLayout(layout);

        connect(mHierarchy, SIGNAL(itemSelectionChanged()), this, SLOT(UpdateSelection()));
        connect(mHierarchy, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(ItemDoubleClicked(QTreeWidgetItem*, int)));

        // connect the window activation signal to refresh if reactivated
        //connect( this, SIGNAL(visibilityChanged(bool)), this, SLOT(OnVisibilityChanged(bool)) );

        // set the selection mode
        SetSelectionMode(useSingleSelection);
    }


    // destructor
    NodeHierarchyWidget::~NodeHierarchyWidget()
    {
        delete mBoneIcon;
        delete mMeshIcon;
        delete mNodeIcon;
        delete mCharacterIcon;
        /*  // unregister the command callbacks and get rid of the memory
            GetCommandManager()->RemoveCommandCallback( mSelectCallback, false );
            GetCommandManager()->RemoveCommandCallback( mUnselectCallback, false );
            GetCommandManager()->RemoveCommandCallback( mClearSelectionCallback, false );
            delete mSelectCallback;
            delete mUnselectCallback;
            delete mClearSelectionCallback;*/
    }


    void NodeHierarchyWidget::Update(const MCore::Array<uint32>& actorInstanceIDs, CommandSystem::SelectionList* selectionList)
    {
        mActorInstanceIDs = actorInstanceIDs;
        ConvertFromSelectionList(selectionList);

        Update();
    }


    void NodeHierarchyWidget::Update(uint32 actorInstanceID, CommandSystem::SelectionList* selectionList)
    {
        mActorInstanceIDs.Clear();

        if (actorInstanceID == MCORE_INVALIDINDEX32)
        {
            // get the number actor instances and iterate over them
            const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
            for (uint32 i = 0; i < numActorInstances; ++i)
            {
                // add the actor to the node hierarchy widget
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

                if (actorInstance->GetIsOwnedByRuntime())
                {
                    continue;
                }

                mActorInstanceIDs.Add(actorInstance->GetID());
            }
        }
        else
        {
            mActorInstanceIDs.Add(actorInstanceID);
        }

        Update(mActorInstanceIDs, selectionList);
    }


    void NodeHierarchyWidget::Update()
    {
        mHierarchy->blockSignals(true);

        // clear the whole thing (don't put this before blockSignals() else we have a bug in the skeletal LOD choosing, before also doesn't make any sense cause the OnNodesChanged() gets called and resets the selection!)
        mHierarchy->clear();

        // get the number actor instances and iterate over them
        const uint32 numActorInstances = mActorInstanceIDs.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // get the actor instance by its id
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(mActorInstanceIDs[i]);
            if (actorInstance)
            {
                AddActorInstance(actorInstance);
            }
        }

        mHierarchy->blockSignals(false);

        // after we refilled everything, update the selection
        UpdateSelection();
    }


    void NodeHierarchyWidget::AddActorInstance(EMotionFX::ActorInstance* actorInstance)
    {
        EMotionFX::Actor*   actor       = actorInstance->GetActor();
        MCore::String       actorName   = actor->GetFileNameString().ExtractFileName();
        const uint32        numNodes    = actor->GetNumNodes();

        // extract the bones from the actor
        actor->ExtractBoneList(actorInstance->GetLODLevel(), &mBoneList);

        // calculate the number of polygons and indices
        uint32 numPolygons, numVertices, numIndices;
        actor->CalcMeshTotals(actorInstance->GetLODLevel(), &numPolygons, &numVertices, &numIndices);

        QTreeWidgetItem* rootItem = new QTreeWidgetItem(mHierarchy);

        // select the item in case the actor
        if (CheckIfActorInstanceSelected(actorInstance->GetID()))
        {
            rootItem->setSelected(true);
        }

        rootItem->setText(0, actorName.AsChar());
        rootItem->setText(1, "Character");
        rootItem->setText(2, MCore::String(numNodes).AsChar());
        rootItem->setText(3, MCore::String(numIndices / 3).AsChar());
        rootItem->setText(4, "");
        rootItem->setExpanded(true);
        rootItem->setIcon(0, *mCharacterIcon);
        QString whatsthis = MCore::String(actorInstance->GetID()).AsChar();
        rootItem->setWhatsThis(0, whatsthis);

        mHierarchy->addTopLevelItem(rootItem);

        // get the number of root nodes and iterate through them
        const uint32 numRootNodes = actor->GetSkeleton()->GetNumRootNodes();
        for (uint32 i = 0; i < numRootNodes; ++i)
        {
            // get the root node index and the corresponding node
            const uint32        rootNodeIndex   = actor->GetSkeleton()->GetRootNodeIndex(i);
            EMotionFX::Node*    rootNode        = actor->GetSkeleton()->GetNode(rootNodeIndex);

            // recursively add all the nodes to the hierarchy
            RecursivelyAddChilds(rootItem, actor, actorInstance, rootNode);
        }
    }


    bool NodeHierarchyWidget::CheckIfNodeVisible(EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node)
    {
        if (node == nullptr)
        {
            return false;
        }

        const uint32        nodeIndex   = node->GetNodeIndex();
        MCore::String       nodeName    = node->GetNameString().Lowered();
        EMotionFX::Mesh*    mesh        = actorInstance->GetActor()->GetMesh(actorInstance->GetLODLevel(), nodeIndex);
        const bool          isMeshNode  = (mesh);
        const bool          isBone      = (mBoneList.Find(nodeIndex) != MCORE_INVALIDINDEX32);
        const bool          isNode      = (isMeshNode == false && isBone == false);

        return CheckIfNodeVisible(nodeName, isMeshNode, isBone, isNode);
    }


    bool NodeHierarchyWidget::CheckIfNodeVisible(const MCore::String& nodeName, bool isMeshNode, bool isBone, bool isNode)
    {
        if (((mDisplayMeshesButton->isChecked() && isMeshNode) ||
             (mDisplayBonesButton->isChecked() && isBone) ||
             (mDisplayNodesButton->isChecked() && isNode)) &&
            (mFindString.GetIsEmpty() || nodeName.Contains(mFindString.AsChar())))
        {
            return true;
        }

        return false;
    }


    void NodeHierarchyWidget::RecursivelyAddChilds(QTreeWidgetItem* parent, EMotionFX::Actor* actor, EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node)
    {
        const uint32        nodeIndex   = node->GetNodeIndex();
        MCore::String       nodeName    = node->GetNameString().Lowered();
        const uint32        numChildren = node->GetNumChildNodes();
        EMotionFX::Mesh*    mesh        = actor->GetMesh(actorInstance->GetLODLevel(), nodeIndex);
        const bool          isMeshNode  = (mesh);
        const bool          isBone      = (mBoneList.Find(nodeIndex) != MCORE_INVALIDINDEX32);
        const bool          isNode      = (isMeshNode == false && isBone == false);

        if (CheckIfNodeVisible(nodeName, isMeshNode, isBone, isNode))
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(parent);

            // check if the node is selected
            if (CheckIfNodeSelected(node->GetName(), actorInstance->GetID()))
            {
                item->setSelected(true);
            }

            item->setText(0, node->GetName());
            mTempString.Format("%d", numChildren);
            item->setText(2, mTempString.AsChar());
            item->setExpanded(true);
            mTempString.Format("%d", actorInstance->GetID());
            item->setWhatsThis(0, mTempString.AsChar());

            // set the correct icon and the type
            if (isBone)
            {
                item->setIcon(0, *mBoneIcon);
                item->setText(1, "Bone");
            }
            else
            if (isMeshNode)
            {
                item->setIcon(0, *mMeshIcon);
                item->setText(1, "Mesh");
                mTempString.Format("%d", mesh->GetNumIndices() / 3);
                item->setText(3, mTempString.AsChar());
            }
            else if (isNode)
            {
                item->setIcon(0, *mNodeIcon);
                item->setText(1, "Node");
            }
            else
            {
                item->setText(1, "Undefined");
            }


            // the mirrored node
            const bool hasMirrorInfo = actor->GetHasMirrorInfo();
            if (hasMirrorInfo == false || actor->GetNodeMirrorInfo(nodeIndex).mSourceNode == MCORE_INVALIDINDEX16 || actor->GetNodeMirrorInfo(nodeIndex).mSourceNode == nodeIndex)
            {
                item->setText(4, "");
            }
            else
            {
                item->setText(4, actor->GetSkeleton()->GetNode(actor->GetNodeMirrorInfo(nodeIndex).mSourceNode)->GetName());
            }

            parent->addChild(item);

            // iterate through all children
            for (uint32 i = 0; i < numChildren; ++i)
            {
                // get the node index and the corresponding node
                const uint32        childIndex  = node->GetChildIndex(i);
                EMotionFX::Node*    child       = actor->GetSkeleton()->GetNode(childIndex);

                // recursively add all the nodes to the hierarchy
                RecursivelyAddChilds(item, actor, actorInstance, child);
            }
        }
        else
        {
            // iterate through all children
            for (uint32 i = 0; i < numChildren; ++i)
            {
                // get the node index and the corresponding node
                const uint32        childIndex  = node->GetChildIndex(i);
                EMotionFX::Node*    child       = actor->GetSkeleton()->GetNode(childIndex);

                // recursively add all the nodes to the hierarchy
                RecursivelyAddChilds(parent, actor, actorInstance, child);
            }
        }
    }


    // remove the selected item with the given node name from the selected nodes
    void NodeHierarchyWidget::RemoveNodeFromSelectedNodes(const char* nodeName, uint32 actorInstanceID)
    {
        // generate the string id for the given node name
        const uint32 nodeNameID = MCore::GetStringIDGenerator().GenerateIDForString(nodeName);

        // iterate through the selected nodes and remove the given ones
        for (uint32 i = 0; i < mSelectedNodes.GetLength(); )
        {
            // check if this is our node, if yes remove it
            if (nodeNameID == mSelectedNodes[i].mNodeNameID && actorInstanceID == mSelectedNodes[i].mActorInstanceID)
            {
                mSelectedNodes.Remove(i);
                //LOG("Removing: %s", nodeName);
            }
            else
            {
                i++;
            }
        }
    }


    void NodeHierarchyWidget::RemoveActorInstanceFromSelectedNodes(uint32 actorInstanceID)
    {
        const uint32 emptyStringID = MCore::GetStringIDGenerator().GenerateIDForString("");

        // iterate through the selected nodes and remove the given ones
        for (uint32 i = 0; i < mSelectedNodes.GetLength(); )
        {
            // check if this is our node, if yes remove it
            if (emptyStringID == mSelectedNodes[i].mNodeNameID && actorInstanceID == mSelectedNodes[i].mActorInstanceID)
            {
                mSelectedNodes.Remove(i);
                //LOG("Removing: %s", nodeName);
            }
            else
            {
                i++;
            }
        }
    }


    // add the given node from the given actor instance to the selected nodes
    void NodeHierarchyWidget::AddNodeToSelectedNodes(const char* nodeName, uint32 actorInstanceID)
    {
        // generate the string id for the given node name
        const uint32 nodeNameID = MCore::GetStringIDGenerator().GenerateIDForString(nodeName);

        // get the number of nodes and iterate through them
        const uint32 numSelectedNodes = mSelectedNodes.GetLength();
        for (uint32 i = 0; i < numSelectedNodes; ++i)
        {
            const uint32 currentNodeNameID  = mSelectedNodes[i].mNodeNameID;
            const uint32 currentActorID     = mSelectedNodes[i].mActorInstanceID;

            // check if this is our node, if yes remove it
            if (nodeNameID == currentNodeNameID && actorInstanceID == currentActorID)
            {
                return; // this means the item is already in and we don't need to add it anymore
            }
        }

        SelectionItem selectionItem;
        selectionItem.mActorInstanceID = actorInstanceID;
        selectionItem.SetNodeName(nodeName);
        mSelectedNodes.Add(selectionItem);
        //LOG("   Adding: %s", mItemName.AsChar());
    }


    // remove all unselected child items from the currently selected nodes
    void NodeHierarchyWidget::RecursiveRemoveUnselectedItems(QTreeWidgetItem* item)
    {
        // check if this item is unselected, if yes remove it from the selected nodes
        if (item->isSelected() == false)
        {
            // get the actor instance id to which this item belongs to
            mActorInstanceIDString = FromQtString(item->whatsThis(0));
            MCORE_ASSERT(mActorInstanceIDString.CheckIfIsValidInt());
            uint32 actorInstanceID = mActorInstanceIDString.ToInt();

            // remove the node from the selected nodes
            RemoveNodeFromSelectedNodes(FromQtString(item->text(0)).AsChar(), actorInstanceID);

            // remove the selected actor
            QTreeWidgetItem* parent = item->parent();
            if (parent == nullptr)
            {
                RemoveActorInstanceFromSelectedNodes(actorInstanceID);
            }
        }

        // get the number of children and iterate through them
        const uint32 numChilds = item->childCount();
        for (uint32 i = 0; i < numChilds; ++i)
        {
            RecursiveRemoveUnselectedItems(item->child(i));
        }
    }


    void NodeHierarchyWidget::UpdateSelection()
    {
        uint32 i;

        //LOG("================================Update Selection!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        //LOG("NumSelectedNodes=%i", mSelectedNodes.GetLength());
        //String debugString;
        //debugString.Reserve(10000);
        //for (uint32 s=0; s<mSelectedNodes.GetLength(); ++s)
        //  debugString.FormatAdd("%s,", mSelectedNodes[s].GetNodeName());
        //LOG(debugString.AsChar());

        // get the selected items and the number of them
        QList<QTreeWidgetItem*> selectedItems = mHierarchy->selectedItems();
        const uint32 numSelectedItems = selectedItems.count();

        // remove the unselected tree widget items from the selected nodes
        const uint32 numTopLevelItems = mHierarchy->topLevelItemCount();
        for (i = 0; i < numTopLevelItems; ++i)
        {
            RecursiveRemoveUnselectedItems(mHierarchy->topLevelItem(i));
        }

        // iterate through all selected items
        for (i = 0; i < numSelectedItems; ++i)
        {
            QTreeWidgetItem* item = selectedItems[i];

            // get the item name
            FromQtString(item->text(0), &mItemName);
            FromQtString(item->whatsThis(0), &mActorInstanceIDString);

            MCORE_ASSERT(mActorInstanceIDString.CheckIfIsValidInt());
            uint32 actorInstanceID = mActorInstanceIDString.ToInt();

            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
            if (actorInstance == nullptr)
            {
                //LogWarning("Cannot add node '%s' from actor instance %i to the selection.", itemName.AsChar(), actorInstanceID);
                continue;
            }

            // check if the item name is actually a valid node
            EMotionFX::Actor* actor = actorInstance->GetActor();
            if (actor->GetSkeleton()->FindNodeByName(mItemName.AsChar()))
            {
                AddNodeToSelectedNodes(mItemName.AsChar(), actorInstanceID);
            }

            // check if we are dealing with an actor instance
            QTreeWidgetItem* parent = item->parent();
            if (parent == nullptr)
            {
                AddNodeToSelectedNodes("", actorInstanceID);
            }
        }
    }


    void NodeHierarchyWidget::SetSelectionMode(bool useSingleSelection)
    {
        if (useSingleSelection)
        {
            mHierarchy->setSelectionMode(QAbstractItemView::SingleSelection);
        }
        else
        {
            mHierarchy->setSelectionMode(QAbstractItemView::ExtendedSelection);
        }

        mUseSingleSelection = useSingleSelection;
    }


    void NodeHierarchyWidget::ItemDoubleClicked(QTreeWidgetItem* item, int column)
    {
        MCORE_UNUSED(item);
        MCORE_UNUSED(column);

        UpdateSelection();
        emit OnDoubleClicked(mSelectedNodes);
    }


    void NodeHierarchyWidget::TreeContextMenu(const QPoint& pos)
    {
        // show the menu if at least one selected
        const uint32 numSelectedNodes = mSelectedNodes.GetLength();
        if (numSelectedNodes == 0)
        {
            return;
        }

        // show the menu
        QMenu menu(this);
        menu.addAction("Add all towards root to selection");
        if (menu.exec(mHierarchy->mapToGlobal(pos)))
        {
            for (uint32 i = 0; i < numSelectedNodes; ++i)
            {
                SelectionItem selectedItem = mSelectedNodes[i];
                EMotionFX::Node* node = EMotionFX::GetActorManager().FindActorInstanceByID(selectedItem.mActorInstanceID)->GetActor()->GetSkeleton()->FindNodeByName(selectedItem.GetNodeName());
                EMotionFX::Node* parentNode = node->GetParentNode();
                while (parentNode)
                {
                    AddNodeToSelectedNodes(parentNode->GetName(), selectedItem.mActorInstanceID);
                    parentNode = parentNode->GetParentNode();
                }
            }
            Update();
        }
    }


    void NodeHierarchyWidget::TextChanged(const QString& text)
    {
        //mFindString = String(text.toAscii().data()).Lowered();
        FromQtString(text, &mFindString);
        mFindString.ToLower();
        Update();
    }


    void NodeHierarchyWidget::FireSelectionDoneSignal()
    {
        emit OnSelectionDone(mSelectedNodes);
    }


    MCore::Array<SelectionItem>& NodeHierarchyWidget::GetSelectedItems()
    {
        UpdateSelection();
        return mSelectedNodes;
    }


    // check if the node with the given name is selected in the window
    bool NodeHierarchyWidget::CheckIfNodeSelected(const char* nodeName, uint32 actorInstanceID)
    {
        // get the number of nodes and iterate through them
        const uint32 numSelectedNodes = mSelectedNodes.GetLength();
        for (uint32 i = 0; i < numSelectedNodes; ++i)
        {
            if (mSelectedNodes[i].mActorInstanceID == actorInstanceID && mSelectedNodes[i].GetNodeNameString().CheckIfIsEqual(nodeName))
            {
                return true;
            }
        }

        // failure, not found in the selected nodes array
        return false;
    }


    // check if the actor instance with the given id is selected in the window
    bool NodeHierarchyWidget::CheckIfActorInstanceSelected(uint32 actorInstanceID)
    {
        // get the number of nodes and iterate through them
        const uint32 numSelectedNodes = mSelectedNodes.GetLength();
        for (uint32 i = 0; i < numSelectedNodes; ++i)
        {
            if (mSelectedNodes[i].mActorInstanceID == actorInstanceID && mSelectedNodes[i].GetNodeNameString().GetIsEmpty())
            {
                return true;
            }
        }

        // failure, not found in the selected nodes array
        return false;
    }


    // sync the selection list with the selected nodes
    void NodeHierarchyWidget::ConvertFromSelectionList(CommandSystem::SelectionList* selectionList)
    {
        // if the selection list is not valid get the global from emstudio
        if (selectionList == nullptr)
        {
            selectionList = &(GetCommandManager()->GetCurrentSelection());
        }

        mSelectedNodes.Clear();

        // get the number actor instances and iterate over them
        const uint32 numActorInstances = mActorInstanceIDs.GetLength();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // add the actor to the node hierarchy widget
            const uint32 actorInstanceID = mActorInstanceIDs[i];
            //      ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);

            // get the number of selected nodes and iterate through them
            const uint32 numSelectedNodes = selectionList->GetNumSelectedNodes();
            for (uint32 n = 0; n < numSelectedNodes; ++n)
            {
                // add the node to the selected nodes
                SelectionItem selectionItem;
                selectionItem.mActorInstanceID = actorInstanceID;
                selectionItem.SetNodeName(selectionList->GetNode(n)->GetName());
                mSelectedNodes.Add(selectionItem);
            }
        }
    }

    /*
    void NodeHierarchyWidget::OnVisibilityChanged(bool isVisible)
    {
        if (isVisible)
            Update();
    }


    bool UpdateNodeHierarchyWidget()
    {
        NodeHierarchyWidget* window = NodeHierarchyWidget::GetNodeHierarchyWidget();

        // is the window visible? only update it if it is visible
        if (window->visibleRegion().isEmpty() == false)
            window->Update();

        return true;
    }

    bool NodeHierarchyWidget::CommandSelectCallback::Execute(Command* command, const CommandLine& commandLine)                  { return UpdateNodeHierarchyWidget(); }
    bool NodeHierarchyWidget::CommandSelectCallback::Undo(Command* command, const CommandLine& commandLine)                     { return UpdateNodeHierarchyWidget(); }
    bool NodeHierarchyWidget::CommandUnselectCallback::Execute(Command* command, const CommandLine& commandLine)                    { return UpdateNodeHierarchyWidget(); }
    bool NodeHierarchyWidget::CommandUnselectCallback::Undo(Command* command, const CommandLine& commandLine)                       { return UpdateNodeHierarchyWidget(); }
    bool NodeHierarchyWidget::CommandClearSelectionCallback::Execute(Command* command, const CommandLine& commandLine)          { return UpdateNodeHierarchyWidget(); }
    bool NodeHierarchyWidget::CommandClearSelectionCallback::Undo(Command* command, const CommandLine& commandLine)         { return UpdateNodeHierarchyWidget(); }*/
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/NodeHierarchyWidget.moc>