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
#include "AnimGraphHierarchyWidget.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include "NavigateWidget.h"
#include <QLabel>
#include <QSizePolicy>
#include <QTreeWidget>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QHeaderView>


namespace EMStudio
{
    AnimGraphHierarchyWidget::AnimGraphHierarchyWidget(QWidget* parent, bool useSingleSelection, CommandSystem::SelectionList* selectionList, const AZ::TypeId& visibilityFilterNodeType, bool showStatesOnly)
        : QWidget(parent)
    {
        mCurrentSelectionList = selectionList;
        if (!selectionList)
        {
            mCurrentSelectionList = &(GetCommandManager()->GetCurrentSelection());
        }

        mShowStatesOnly = showStatesOnly;
        mFilterNodeType = visibilityFilterNodeType;

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);

        // Create the display button group.
        QHBoxLayout* displayLayout = new QHBoxLayout();

        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        displayLayout->addWidget(m_searchWidget);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &AnimGraphHierarchyWidget::OnTextFilterChanged);

        // Create the tree widget.
        mHierarchy = new QTreeWidget();

        mHierarchy->setColumnCount(2);
        QStringList headerList;
        headerList.append("Name");
        headerList.append("Type");
        mHierarchy->setHeaderLabels(headerList);

        mHierarchy->setColumnWidth(0, 400);
        mHierarchy->setColumnWidth(1, 100);
        mHierarchy->setSortingEnabled(false);
        mHierarchy->setMinimumWidth(500);
        mHierarchy->setMinimumHeight(500);
        mHierarchy->setAlternatingRowColors(true);
        mHierarchy->setAnimated(true);

        // Disable moving to have a fixed ordering.
        mHierarchy->header()->setSectionsMovable(false);

        layout->addLayout(displayLayout);
        layout->addWidget(mHierarchy);
        setLayout(layout);

        connect(mHierarchy, SIGNAL(itemSelectionChanged()), this, SLOT(UpdateSelection()));
        connect(mHierarchy, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(ItemDoubleClicked(QTreeWidgetItem*, int)));

        SetSelectionMode(useSingleSelection);
    }


    AnimGraphHierarchyWidget::~AnimGraphHierarchyWidget()
    {
    }


    void AnimGraphHierarchyWidget::Update(uint32 animGraphID, CommandSystem::SelectionList* selectionList)
    {
        mAnimGraphID            = animGraphID;
        mCurrentSelectionList   = selectionList;

        if (!selectionList)
        {
            mCurrentSelectionList = &(GetCommandManager()->GetCurrentSelection());
        }

        Update();
    }


    void AnimGraphHierarchyWidget::Update()
    {
        mHierarchy->clear();

        mHierarchy->blockSignals(true);
        if (mAnimGraphID != MCORE_INVALIDINDEX32)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
            if (animGraph)
            {
                if (m_searchWidgetText.empty())
                {
                    NavigateWidget::UpdateTreeWidget(mHierarchy, animGraph, mFilterNodeType, mShowStatesOnly);
                }
                else
                {
                    NavigateWidget::UpdateTreeWidget(mHierarchy, animGraph, mFilterNodeType, mShowStatesOnly, m_searchWidgetText.c_str());
                }
            }
        }
        mHierarchy->blockSignals(false);

        UpdateSelection();
    }


    void AnimGraphHierarchyWidget::UpdateSelection()
    {
        // Reset the old selection.
        mSelectedNodes.clear();

        const EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mAnimGraphID);
        if (!animGraph)
        {
            return;
        }

        // Get the selected items and the number of them.
        QList<QTreeWidgetItem*> selectedItems = mHierarchy->selectedItems();
        const int numSelectedItems = selectedItems.count();
        mSelectedNodes.reserve(numSelectedItems);

        // Iterate through all selected items in the tree widget.
        AZStd::string itemName;
        for (int i = 0; i < numSelectedItems; ++i)
        {
            const QTreeWidgetItem* item = selectedItems[i];
            itemName = item->text(0).toUtf8().data();

            // Is the item referring to a valid node?
            if (animGraph->RecursiveFindNodeByName(itemName.c_str()))
            {
                AnimGraphSelectionItem selectionItem;
                selectionItem.mAnimGraphID = mAnimGraphID;
                selectionItem.mNodeName    = itemName;

                mSelectedNodes.push_back(selectionItem);
            }
        }
    }


    void AnimGraphHierarchyWidget::SetSelectionMode(bool useSingleSelection)
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


    void AnimGraphHierarchyWidget::ItemDoubleClicked(QTreeWidgetItem* item, int column)
    {
        MCORE_UNUSED(item);
        MCORE_UNUSED(column);

        if (!mUseSingleSelection)
        {
            return;
        }

        UpdateSelection();
        FireSelectionDoneSignal();
    }


    void AnimGraphHierarchyWidget::OnTextFilterChanged(const QString& text)
    {
        FromQtString(text, &m_searchWidgetText);
        AZStd::to_lower(m_searchWidgetText.begin(), m_searchWidgetText.end());
        Update();
    }


    void AnimGraphHierarchyWidget::FireSelectionDoneSignal()
    {
        emit OnSelectionDone(mSelectedNodes);
    }


    AZStd::vector<AnimGraphSelectionItem>& AnimGraphHierarchyWidget::GetSelectedItems()
    {
        UpdateSelection();
        return mSelectedNodes;
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphHierarchyWidget.moc>