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
#include "StateFilterSelectionWindow.h"
#include "BlendGraphWidget.h"
#include "AnimGraphPlugin.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QPushButton>

#include <MysticQt/Source/ColorLabel.h>

#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>


namespace EMStudio
{
    // constructor
    StateFilterSelectionWindow::StateFilterSelectionWindow(AnimGraphPlugin* plugin, QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle("Select States");

        mPlugin = plugin;
        mAnimGraph = nullptr;

        QVBoxLayout* mainLayout = new QVBoxLayout();
        setLayout(mainLayout);
        mainLayout->setAlignment(Qt::AlignTop);

        mTableWidget = new QTableWidget();
        mTableWidget->setAlternatingRowColors(true);
        mTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        mTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mTableWidget->horizontalHeader()->setStretchLastSection(true);
        mTableWidget->setCornerButtonEnabled(false);
        mTableWidget->setContextMenuPolicy(Qt::DefaultContextMenu);
        connect(mTableWidget, SIGNAL(itemSelectionChanged()), this, SLOT(OnSelectionChanged()));

        mainLayout->addWidget(mTableWidget);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        mainLayout->addLayout(buttonLayout);

        QPushButton* okButton = new QPushButton("OK");
        okButton->setDefault(true);
        buttonLayout->addWidget(okButton);
        connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));

        QPushButton* cancelButton = new QPushButton("Cancel");
        buttonLayout->addWidget(cancelButton);
        connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    }


    // destructor
    StateFilterSelectionWindow::~StateFilterSelectionWindow()
    {
    }


    // called to init for a new anim graph
    void StateFilterSelectionWindow::ReInit(EMotionFX::AnimGraph* animGraph, const MCore::Array<MCore::String>& oldNodeSelection, const MCore::Array<MCore::String>& oldGroupSelection)
    {
        mAnimGraph             = animGraph;
        mSelectedGroupNames     = oldGroupSelection;
        mSelectedNodeNames      = oldNodeSelection;

        // clear the table widget
        mWidgetTable.Clear();
        mTableWidget->clear();
        mTableWidget->setColumnCount(2);

        // set header items for the table
        QTableWidgetItem* headerItem = new QTableWidgetItem("Name");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTableWidget->setHorizontalHeaderItem(0, headerItem);

        headerItem = new QTableWidgetItem("Type");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTableWidget->setHorizontalHeaderItem(1, headerItem);

        mTableWidget->resizeColumnsToContents();
        QHeaderView* horizontalHeader = mTableWidget->horizontalHeader();
        horizontalHeader->setStretchLastSection(true);

        // get the active graph and find the corresponding emfx node
        EMotionFX::AnimGraphNode* activeNode = mPlugin->GetGraphWidget()->GetCurrentNode();
        if (activeNode == nullptr)
        {
            return;
        }

        // get the number of nodes inside the active node, the number node groups and set table size and add header items
        const uint32 numNodeGroups  = mAnimGraph->GetNumNodeGroups();
        const uint32 numNodes       = activeNode->GetNumChildNodes();
        const uint32 numRows        = numNodeGroups + numNodes;
        mTableWidget->setRowCount(numRows);

        // iterate the nodes and add them all
        uint32 currentRowIndex = 0;
        for (uint32 i = 0; i < numNodes; ++i)
        {
            EMotionFX::AnimGraphNode* childNode = activeNode->GetChildNode(i);

            // check if we need to select the node
            bool isSelected = false;
            if (oldNodeSelection.Find(childNode->GetName()) != MCORE_INVALIDINDEX32)
            {
                isSelected = true;
            }

            AddRow(currentRowIndex, childNode->GetName(), false, isSelected);
            currentRowIndex++;
        }

        // iterate the node groups and add them all
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            // get a pointer to the node group
            EMotionFX::AnimGraphNodeGroup* nodeGroup = mAnimGraph->GetNodeGroup(i);

            // check if any of the nodes from the node group actually is visible in the current state machine
            bool addGroup = false;
            for (uint32 n = 0; n < numNodes; ++n)
            {
                EMotionFX::AnimGraphNode* childNode = activeNode->GetChildNode(n);
                if (nodeGroup->Contains(childNode->GetID()))
                {
                    addGroup = true;
                    break;
                }
            }

            if (addGroup)
            {
                // check if we need to select the node group
                bool isSelected = false;
                if (oldGroupSelection.Find(nodeGroup->GetName()) != MCORE_INVALIDINDEX32)
                {
                    isSelected = true;
                }

                AddRow(currentRowIndex, nodeGroup->GetName(), true, isSelected, nodeGroup->GetColor());
                currentRowIndex++;
            }
        }

        // resize to contents and adjust header
        QHeaderView* verticalHeader = mTableWidget->verticalHeader();
        verticalHeader->setVisible(false);
        mTableWidget->resizeColumnsToContents();
        horizontalHeader->setStretchLastSection(true);
    }


    void StateFilterSelectionWindow::AddRow(uint32 rowIndex, const char* name, bool isGroup, bool isSelected, const MCore::RGBAColor& color)
    {
        // create the name item
        QTableWidgetItem* nameItem = nullptr;
        if (isGroup)
        {
            nameItem = new QTableWidgetItem("[" + QString(name) + "]");
        }
        else
        {
            nameItem = new QTableWidgetItem(name);
        }

        // set the name item params
        nameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        // add the name item in the table
        mTableWidget->setItem(rowIndex, 0, nameItem);

        // add a lookup
        mWidgetTable.Add(WidgetLookup(nameItem, name, isGroup));

        // create the type item
        QTableWidgetItem* typeItem = nullptr;
        if (isGroup)
        {
            typeItem = new QTableWidgetItem("Node Group");
        }
        else
        {
            typeItem = new QTableWidgetItem("Node");
        }

        // set the type item params
        typeItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        // add the type item in the table
        mTableWidget->setItem(rowIndex, 1, typeItem);

        // add a lookup
        mWidgetTable.Add(WidgetLookup(typeItem, name, isGroup));

        // set backgroundcolor of the row
        if (isGroup)
        {
            QColor backgroundColor(color.r * 255, color.g * 255, color.b * 255, 50);
            nameItem->setBackgroundColor(backgroundColor);
            typeItem->setBackgroundColor(backgroundColor);
        }

        // handle selection
        if (isSelected)
        {
            nameItem->setSelected(true);
            typeItem->setSelected(true);
        }

        // set the row height
        mTableWidget->setRowHeight(rowIndex, 21);
    }


    // find the index for the given widget
    EMotionFX::AnimGraphNodeGroup* StateFilterSelectionWindow::FindGroupByWidget(QTableWidgetItem* widget) const
    {
        // return directly if the anim graph is not valid
        if (mAnimGraph == nullptr)
        {
            return nullptr;
        }

        // for all table entries
        const uint32 numWidgets = mWidgetTable.GetLength();
        for (uint32 i = 0; i < numWidgets; ++i)
        {
            if (mWidgetTable[i].mIsGroup && mWidgetTable[i].mWidget == widget)
            {
                return mAnimGraph->FindNodeGroupByName(mWidgetTable[i].mName.AsChar());
            }
        }

        // the widget is not in the list, return failure
        return nullptr;
    }


    // find the node for the given widget
    EMotionFX::AnimGraphNode* StateFilterSelectionWindow::FindNodeByWidget(QTableWidgetItem* widget) const
    {
        // return directly if the anim graph is not valid
        if (mAnimGraph == nullptr)
        {
            return nullptr;
        }

        // for all table entries
        const uint32 numWidgets = mWidgetTable.GetLength();
        for (uint32 i = 0; i < numWidgets; ++i)
        {
            if (mWidgetTable[i].mIsGroup == false && mWidgetTable[i].mWidget == widget)
            {
                return mAnimGraph->RecursiveFindNode(mWidgetTable[i].mName.AsChar());
            }
        }

        // the widget is not in the list, return failure
        return nullptr;
    }


    // called when the selection got changed
    void StateFilterSelectionWindow::OnSelectionChanged()
    {
        // reset the selection arrays
        mSelectedGroupNames.Clear();
        mSelectedNodeNames.Clear();

        // get the selected items and the number of them
        QList<QTableWidgetItem*> selectedItems = mTableWidget->selectedItems();
        const int numSelectedItems = selectedItems.count();

        // iterate through the selected items
        for (int32 i = 0; i < numSelectedItems; ++i)
        {
            // handle nodes
            EMotionFX::AnimGraphNode* node = FindNodeByWidget(selectedItems[i]);
            if (node)
            {
                // add the node name in case it is not in yet
                if (mSelectedNodeNames.Find(node->GetName()) == MCORE_INVALIDINDEX32)
                {
                    mSelectedNodeNames.Add(node->GetName());
                }
            }

            // handle node groups
            EMotionFX::AnimGraphNodeGroup* nodeGroup = FindGroupByWidget(selectedItems[i]);
            if (nodeGroup)
            {
                // add the node group name in case it is not in yet
                if (mSelectedGroupNames.Find(nodeGroup->GetName()) == MCORE_INVALIDINDEX32)
                {
                    mSelectedGroupNames.Add(nodeGroup->GetName());
                }
            }
        }
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/StateFilterSelectionWindow.moc>
