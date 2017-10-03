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

#include "ParameterWidget.h"
#include <AzCore/std/string/conversions.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/AnimGraphParameterGroup.h>
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
    // constructor
    ParameterWidget::ParameterWidget(QWidget* parent, bool useSingleSelection)
        : QWidget(parent)
    {
        // create the display button group
        QHBoxLayout* displayLayout = new QHBoxLayout();

        displayLayout->addWidget(new QLabel("Find:"), 0, Qt::AlignRight);
        mFindWidget = new MysticQt::SearchButton(this, MysticQt::GetMysticQt()->FindIcon("Images/Icons/SearchClearButton.png"));
        displayLayout->addWidget(mFindWidget);

        connect(mFindWidget->GetSearchEdit(), SIGNAL(textChanged(const QString&)), this, SLOT(TextChanged(const QString&)));

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);

        // create the tree widget
        mTreeWidget = new QTreeWidget();

        // create header items
        mTreeWidget->setColumnCount(1);
        QStringList headerList;
        headerList.append("Name");
        mTreeWidget->setHeaderLabels(headerList);

        // set optical stuff for the tree
        //mTreeWidget->setColumnWidth(0, 400);
        //mTreeWidget->setColumnWidth(1, 60);
        //mTreeWidget->setColumnWidth(2, 80);
        //mTreeWidget->setColumnWidth(3, 60);
        mTreeWidget->setSortingEnabled(false);
        mTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        mTreeWidget->setMinimumWidth(620);
        mTreeWidget->setMinimumHeight(500);
        mTreeWidget->setAlternatingRowColors(true);
        mTreeWidget->setExpandsOnDoubleClick(true);
        mTreeWidget->setAnimated(true);

        // disable the move of section to have column order fixed
        mTreeWidget->header()->setSectionsMovable(false);

        layout->addLayout(displayLayout);
        layout->addWidget(mTreeWidget);
        setLayout(layout);

        connect(mTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(UpdateSelection()));
        connect(mTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(ItemDoubleClicked(QTreeWidgetItem*, int)));

        // set the selection mode
        SetSelectionMode(useSingleSelection);
    }


    ParameterWidget::~ParameterWidget()
    {
    }


    void ParameterWidget::Update(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& selectedParameters)
    {
        mAnimGraph                = animGraph;
        mSelectedParameters       = selectedParameters;
        mOldSelectedParameters    = selectedParameters;

        Update();
    }


    void ParameterWidget::AddParameterToInterface(EMotionFX::AnimGraph* animGraph, uint32 parameterIndex, QTreeWidgetItem* parameterGroupItem)
    {
        MCORE_ASSERT(parameterIndex < animGraph->GetNumParameters());

        MCore::AttributeSettings* parameter = animGraph->GetParameter(parameterIndex);

        // make sure we only show the parameters that are wanted after the name are filtering
        if (mFilterString.empty() || parameter->GetNameString().Lowered().Contains(mFilterString.c_str()))
        {
            // add the parameter to the tree widget
            QTreeWidgetItem* item = new QTreeWidgetItem(parameterGroupItem);
            item->setText(0, parameter->GetName());
            item->setExpanded(true);
            parameterGroupItem->addChild(item);

            // check if the given parameter is selected
            if (AZStd::find(mOldSelectedParameters.begin(), mOldSelectedParameters.end(), parameter->GetName()) != mOldSelectedParameters.end())
            {
                item->setSelected(true);
            }
        }
    }


    void ParameterWidget::Update()
    {
        mTreeWidget->clear();
        mTreeWidget->blockSignals(true);

        // default parameter group
        QTreeWidgetItem* defaultGroupItem = new QTreeWidgetItem(mTreeWidget);
        defaultGroupItem->setText(0, "Default");
        defaultGroupItem->setText(1, "");
        defaultGroupItem->setExpanded(true);
        mTreeWidget->addTopLevelItem(defaultGroupItem);

        // add all parameters that belong to no parameter group
        const uint32 numParameters = mAnimGraph->GetNumParameters();
        for (uint32 i = 0; i < numParameters; ++i)
        {
            if (mAnimGraph->FindParameterGroupForParameter(i))
            {
                continue;
            }

            AddParameterToInterface(mAnimGraph, i, defaultGroupItem);
        }

        // get the number of parameter groups and iterate through them
        MCore::String tempString;
        const uint32 numGroups = mAnimGraph->GetNumParameterGroups();
        for (uint32 g = 0; g < numGroups; ++g)
        {
            EMotionFX::AnimGraphParameterGroup* group = mAnimGraph->GetParameterGroup(g);

            // add the group item to the tree widget
            QTreeWidgetItem* groupItem = new QTreeWidgetItem(mTreeWidget);
            groupItem->setText(0, group->GetName());
            //tempString.Format("%d", group->GetNumParameters());
            //groupItem->setText( 2, tempString.AsChar() );

            groupItem->setExpanded(true);
            tempString.Format("%d Parameters", group->GetNumParameters());
            groupItem->setToolTip(1, tempString.AsChar());
            mTreeWidget->addTopLevelItem(groupItem);

            // add all parameters that belong to the given group
            bool groupSelected = true;
            for (uint32 i = 0; i < numParameters; ++i)
            {
                if (mAnimGraph->FindParameterGroupForParameter(i) != group)
                {
                    continue;
                }

                AddParameterToInterface(mAnimGraph, i, groupItem);

                // check if the given parameter is selected
                if (AZStd::find(mOldSelectedParameters.begin(), mOldSelectedParameters.end(), mAnimGraph->GetParameter(i)->GetName()) == mOldSelectedParameters.end())
                {
                    groupSelected = false;
                }
            }

            // select the group
            if (group->GetNumParameters() == 0)
            {
                groupSelected = false;
            }
            groupItem->setSelected(groupSelected);
        }

        mTreeWidget->blockSignals(false);
        UpdateSelection();
    }


    void ParameterWidget::UpdateSelection()
    {
        // get the selected items and the number of them
        QList<QTreeWidgetItem*> selectedItems = mTreeWidget->selectedItems();
        const uint32 numSelectedItems = selectedItems.count();

        // reset the old selection
        mSelectedParameters.clear();
        mSelectedParameters.reserve(numSelectedItems);

        // iterate through all selected items
        MCore::String itemName;
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            QTreeWidgetItem* item = selectedItems[i];

            // get the item name
            FromQtString(item->text(0), &itemName);
            EMotionFX::AnimGraphParameterGroup* parameterGroup = mAnimGraph->FindParameterGroupByName(itemName.AsChar());

            // check if the selected item is a parameter
            if (mAnimGraph->FindParameter(itemName.AsChar()))
            {
                if (AZStd::find(mSelectedParameters.begin(), mSelectedParameters.end(), itemName.AsChar()) == mSelectedParameters.end())
                {
                    mSelectedParameters.push_back(itemName.AsChar());
                }
            }
            // check if the selected item is a group
            else if (parameterGroup)
            {
                // get the number of parameters in the group and iterate through them
                const uint32 numParameters = parameterGroup->GetNumParameters();
                for (uint32 p = 0; p < numParameters; ++p)
                {
                    const uint32                parameterIndex  = parameterGroup->GetParameter(p);
                    MCore::AttributeSettings*   attributeSettings = mAnimGraph->GetParameter(parameterIndex);

                    if (AZStd::find(mSelectedParameters.begin(), mSelectedParameters.end(), attributeSettings->GetName()) == mSelectedParameters.end())
                    {
                        mSelectedParameters.push_back(attributeSettings->GetName());
                    }
                }
            }
        }
    }


    void ParameterWidget::SetSelectionMode(bool useSingleSelection)
    {
        if (useSingleSelection)
        {
            mTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        }
        else
        {
            mTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        }

        mUseSingleSelection = useSingleSelection;
    }


    void ParameterWidget::ItemDoubleClicked(QTreeWidgetItem* item, int column)
    {
        MCORE_UNUSED(item);
        MCORE_UNUSED(column);

        UpdateSelection();
        if (!mSelectedParameters.empty())
        {
            emit OnDoubleClicked(mSelectedParameters[0]);
        }
    }


    void ParameterWidget::TextChanged(const QString& text)
    {
        mFilterString = text.toUtf8().data();
        AZStd::to_lower(mFilterString.begin(), mFilterString.end());
        Update();
    }


    void ParameterWidget::FireSelectionDoneSignal()
    {
        emit OnSelectionDone(mSelectedParameters);
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWidget.moc>