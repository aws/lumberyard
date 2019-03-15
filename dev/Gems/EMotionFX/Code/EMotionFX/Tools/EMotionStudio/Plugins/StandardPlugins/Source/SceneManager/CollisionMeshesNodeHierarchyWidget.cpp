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
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include "CollisionMeshesNodeHierarchyWidget.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MCore/Source/StringConversions.h>
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

        mMeshIcon       = new QIcon(AZStd::string(MysticQt::GetDataDir() + "Images/Icons/Mesh.png").c_str());
        mCharacterIcon  = new QIcon(AZStd::string(MysticQt::GetDataDir() + "Images/Icons/Character.png").c_str());

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);

        // create the display layout
        QHBoxLayout* displayLayout = new QHBoxLayout();

        displayLayout->addWidget(new QLabel("LOD:"));

        mLODSpinBox = new MysticQt::IntSpinBox();
        connect(mLODSpinBox, static_cast<void (MysticQt::IntSpinBox::*)(int)>(&MysticQt::IntSpinBox::valueChanged), this, &CollisionMeshesNodeHierarchyWidget::LODSpinBoxValueChanged);
        displayLayout->addWidget(mLODSpinBox);

        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        displayLayout->addWidget(spacerWidget);

        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &CollisionMeshesNodeHierarchyWidget::OnTextFilterChanged);
        displayLayout->addWidget(m_searchWidget);

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
        connect(mHierarchy, &QTreeWidget::customContextMenuRequested, this, &CollisionMeshesNodeHierarchyWidget::TreeContextMenu);

        // connect when the selection changed
        connect(mHierarchy, &QTreeWidget::itemSelectionChanged, this, &CollisionMeshesNodeHierarchyWidget::UpdateSelection);

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
        AZStd::string       actorName;
        AzFramework::StringFunc::Path::GetFileName(actor->GetFileNameString().c_str(), actorName);
        const uint32        numNodes    = actor->GetNumNodes();

        // calculate the number of polygons and indices
        uint32 numPolygons, numVertices, numIndices;
        actor->CalcMeshTotals(mLODSpinBox->value(), &numPolygons, &numVertices, &numIndices);

        QTreeWidgetItem* rootItem = new QTreeWidgetItem(mHierarchy);
        rootItem->setSelected(mRootSelected);

        rootItem->setText(0, actorName.c_str());
        rootItem->setText(1, "Character");
        rootItem->setText(2, AZStd::to_string(numNodes).c_str());
        rootItem->setText(3, AZStd::to_string(numIndices / 3).c_str());
        rootItem->setText(4, "");
        rootItem->setExpanded(true);
        rootItem->setIcon(0, *mCharacterIcon);
        QString whatsthis = AZStd::to_string(actorInstance->GetID()).c_str();
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


    bool CollisionMeshesNodeHierarchyWidget::CheckIfNodeVisible(const AZStd::string& nodeName, bool isMeshNode)
    {
        AZStd::string loweredNodeName = nodeName;
        AZStd::to_lower(loweredNodeName.begin(), loweredNodeName.end());
        if ((isMeshNode) && (m_searchWidgetText.empty() || loweredNodeName.find(m_searchWidgetText) != AZStd::string::npos))
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
        AZStd::string       nodeName    = node->GetNameString();
        const uint32        numChildren = node->GetNumChildNodes();
        EMotionFX::Mesh*    mesh        = actor->GetMesh(mLODSpinBox->value(), nodeIndex);
        const bool          isMeshNode  = (mesh);

        if (CheckIfNodeVisible(nodeName, isMeshNode))
        {
            QTreeWidgetItem* item = new QTreeWidgetItem(parent);
            item->setSelected(mLODNodeList[mLODSpinBox->value()].Contains(nodeName));

            item->setText(0, node->GetName());
            mTempString = AZStd::string::format("%d", numChildren);
            item->setText(2, mTempString.c_str());
            item->setExpanded(true);
            mTempString = AZStd::string::format("%d", actorInstance->GetID());
            item->setWhatsThis(0, mTempString.c_str());

            // set the correct icon and the type
            item->setIcon(0, *mMeshIcon);
            item->setText(1, "Mesh");
            mTempString = AZStd::string::format("%d", mesh->GetNumIndices() / 3);
            item->setText(3, mTempString.c_str());

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

            const AZStd::string nodeName = FromQtString(item->text(0));
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
                const AZStd::string nodeName = FromQtString(item->text(0));
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(mActorInstanceID);
                EMotionFX::Node* node = actorInstance->GetActor()->GetSkeleton()->FindNodeByName(nodeName.c_str());
                if (node)
                {
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
            }

            // enable signals
            mHierarchy->blockSignals(false);

            // update the selection only one time
            UpdateSelection();
        }
    }


    void CollisionMeshesNodeHierarchyWidget::OnTextFilterChanged(const QString& text)
    {
        FromQtString(text, &m_searchWidgetText);
        AZStd::to_lower(m_searchWidgetText.begin(), m_searchWidgetText.end());
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