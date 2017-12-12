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
#include "CollisionMeshesNodeHierarchyWidget.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
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
    CollisionMeshesNodeHierarchyWidget::CollisionMeshesNodeHierarchyWidget(QWidget* parent)
        : QWidget(parent)
    {
        mRootSelected = false;

        mMeshIcon       = new QIcon(MCore::String(MysticQt::GetDataDir() + "Images/Icons/Mesh.png").AsChar());
        mCharacterIcon  = new QIcon(MCore::String(MysticQt::GetDataDir() + "Images/Icons/Character.png").AsChar());

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);

        // create the display layout
        QHBoxLayout* displayLayout = new QHBoxLayout();

        displayLayout->addWidget(new QLabel("LOD:"));

        mLODSpinBox = new MysticQt::IntSpinBox();
        connect(mLODSpinBox, SIGNAL(valueChanged(int)), this, SLOT(LODSpinBoxValueChanged(int)));
        displayLayout->addWidget(mLODSpinBox);

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
        mHierarchy->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mHierarchy->setMinimumWidth(670);
        mHierarchy->setMinimumHeight(500);
        mHierarchy->setAlternatingRowColors(true);
        mHierarchy->setExpandsOnDoubleClick(true);
        mHierarchy->setAnimated(true);

        // disable the move of section to have column order fixed
        mHierarchy->header()->setSectionsMovable(false);

        // connect the context menu
        mHierarchy->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(mHierarchy, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(TreeContextMenu(const QPoint&)));

        // connect when the selection changed
        connect(mHierarchy, SIGNAL(itemSelectionChanged()), this, SLOT(UpdateSelection()));

        layout->addLayout(displayLayout);
        layout->addWidget(mHierarchy);
        setLayout(layout);
    }


    // destructor
    CollisionMeshesNodeHierarchyWidget::~CollisionMeshesNodeHierarchyWidget()
    {
        delete mMeshIcon;
        delete mCharacterIcon;
    }


    void CollisionMeshesNodeHierarchyWidget::Update(uint32 actorInstanceID)
    {
        // keep the actor instance ID
        mActorInstanceID = actorInstanceID;

        // set the root not selected
        mRootSelected = false;

        // clear the LOD node list
        mLODNodeList.Clear();

        // get the actor instance and check if it's invalid
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(mActorInstanceID);
        if (actorInstance == nullptr)
        {
            Update();
            return;
        }

        // store the actual LOD node list
        EMotionFX::Actor* actor = actorInstance->GetActor();
        const uint32 numNodes = actor->GetNumNodes();
        const uint32 numLODLevels = actor->GetNumLODLevels();
        mLODNodeList.Resize(numLODLevels);
        for (uint32 i = 0; i < numLODLevels; ++i)
        {
            for (uint32 j = 0; j < numNodes; ++j)
            {
                EMotionFX::Mesh* mesh = actor->GetMesh(i, j);
                if (mesh && mesh->GetIsCollisionMesh())
                {
                    mLODNodeList[i].Add(actor->GetSkeleton()->GetNode(j)->GetNameString());
                }
            }
        }

        // set the spinbox range and set it to 0
        mLODSpinBox->blockSignals(true);
        mLODSpinBox->setRange(0, numLODLevels - 1);
        mLODSpinBox->setValue(0);
        mLODSpinBox->blockSignals(false);

        // update the widget
        Update();
    }


    void CollisionMeshesNodeHierarchyWidget::Update()
    {
        mHierarchy->blockSignals(true);

        mHierarchy->clear();

        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(mActorInstanceID);
        if (actorInstance)
        {
            AddActorInstance(actorInstance);
        }

        mHierarchy->blockSignals(false);
    }


    void CollisionMeshesNodeHierarchyWidget::AddActorInstance(EMotionFX::ActorInstance* actorInstance)
    {
        EMotionFX::Actor*   actor       = actorInstance->GetActor();
        MCore::String       actorName   = actor->GetFileNameString().ExtractFileName();
        const uint32        numNodes    = actor->GetNumNodes();

        // calculate the number of polygons and indices
        uint32 numPolygons, numVertices, numIndices;
        actor->CalcMeshTotals(mLODSpinBox->value(), &numPolygons, &numVertices, &numIndices);

        QTreeWidgetItem* rootItem = new QTreeWidgetItem(mHierarchy);
        rootItem->setSelected(mRootSelected);

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


    bool CollisionMeshesNodeHierarchyWidget::CheckIfNodeVisible(const MCore::String& nodeName, bool isMeshNode)
    {
        if ((isMeshNode) && (mFindString.GetIsEmpty() || nodeName.Lowered().Contains(mFindString.AsChar())))
        {
            return true;
        }

        return false;
    }


    void CollisionMeshesNodeHierarchyWidget::RecursiveRemoveUnselectedItems(QTreeWidgetItem* item)
    {
        // check if this item is unselected, if yes remove it from the selected nodes
        if (item->isSelected() == false)
        {
            const uint32 index = mLODNodeList[mLODSpinBox->value()].Find(FromQtString(item->text(0)));
            if (index != MCORE_INVALIDINDEX32)
            {
                mLODNodeList[mLODSpinBox->value()].Remove(index);
            }
        }

        // get the number of children and iterate through them
        const uint32 numChilds = item->childCount();
        for (uint32 i = 0; i < numChilds; ++i)
        {
            RecursiveRemoveUnselectedItems(item->child(i));
        }
    }


    void CollisionMeshesNodeHierarchyWidget::RecursivelyAddChilds(QTreeWidgetItem* parent, EMotionFX::Actor* actor, EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node)
    {
        const uint32        nodeIndex   = node->GetNodeIndex();
        MCore::String       nodeName    = node->GetNameString();
        const uint32        numChildren = node->GetNumChildNodes();
        EMotionFX::Mesh*    mesh        = actor->GetMesh(mLODSpinBox->value(), nodeIndex);
        const bool          isMeshNode  = (mesh);

        if (CheckIfNodeVisible(nodeName, isMeshNode))
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(parent);
            item->setSelected(mLODNodeList[mLODSpinBox->value()].Contains(nodeName));

            item->setText(0, node->GetName());
            mTempString.Format("%d", numChildren);
            item->setText(2, mTempString.AsChar());
            item->setExpanded(true);
            mTempString.Format("%d", actorInstance->GetID());
            item->setWhatsThis(0, mTempString.AsChar());

            // set the correct icon and the type
            item->setIcon(0, *mMeshIcon);
            item->setText(1, "Mesh");
            mTempString.Format("%d", mesh->GetNumIndices() / 3);
            item->setText(3, mTempString.AsChar());

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


    void CollisionMeshesNodeHierarchyWidget::UpdateSelection()
    {
        const uint32 numTopLevelItems = mHierarchy->topLevelItemCount();
        for (uint32 i = 0; i < numTopLevelItems; ++i)
        {
            RecursiveRemoveUnselectedItems(mHierarchy->topLevelItem(i));
        }

        const QList<QTreeWidgetItem*> selectedItems = mHierarchy->selectedItems();
        const int numSelectedItems = selectedItems.length();
        const int currentLOD = mLODSpinBox->value();

        mRootSelected = false;
        for (int i = 0; i < numSelectedItems; ++i)
        {
            QTreeWidgetItem* item = selectedItems[i];
            if (item->parent() == nullptr)
            {
                mRootSelected = true;
                continue;
            }

            const MCore::String nodeName = FromQtString(item->text(0));
            if (mLODNodeList[currentLOD].Contains(nodeName) == false)
            {
                mLODNodeList[currentLOD].Add(nodeName);
            }
        }
    }


    void CollisionMeshesNodeHierarchyWidget::TreeContextMenu(const QPoint& pos)
    {
        // show the menu if at least one selected
        const QList<QTreeWidgetItem*> selectedItems = mHierarchy->selectedItems();
        const uint32 numSelectedNodes = selectedItems.length();
        if (numSelectedNodes == 0)
        {
            return;
        }

        // show the menu
        QMenu menu(this);
        menu.addAction("Add all towards root to selection");
        if (menu.exec(mHierarchy->mapToGlobal(pos)))
        {
            // disable signal to not have the update selection called each time
            mHierarchy->blockSignals(true);

            // get the current LOD
            const int currentLOD = mLODSpinBox->value();

            // select all parents
            for (uint32 i = 0; i < numSelectedNodes; ++i)
            {
                QTreeWidgetItem* item = selectedItems[i];
                const MCore::String nodeName = FromQtString(item->text(0));
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(mActorInstanceID);
                EMotionFX::Node* node = actorInstance->GetActor()->GetSkeleton()->FindNodeByName(nodeName);
                EMotionFX::Node* parentNode = node->GetParentNode();
                while (parentNode)
                {
                    if (mLODNodeList[currentLOD].Contains(parentNode->GetNameString()) == false)
                    {
                        mLODNodeList[currentLOD].Add(parentNode->GetNameString());
                    }
                    parentNode = parentNode->GetParentNode();
                }
            }

            // enable signals
            mHierarchy->blockSignals(false);

            // update the selection only one time
            UpdateSelection();
        }
    }


    void CollisionMeshesNodeHierarchyWidget::TextChanged(const QString& text)
    {
        FromQtString(text, &mFindString);
        mFindString.ToLower();
        Update();
    }


    void CollisionMeshesNodeHierarchyWidget::LODSpinBoxValueChanged(int value)
    {
        MCORE_UNUSED(value);
        mRootSelected = false;
        Update();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/SceneManager/CollisionMeshesNodeHierarchyWidget.moc>