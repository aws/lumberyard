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

#include "BlendResourceWidget.h"
#include <AzFramework/StringFunc/StringFunc.h>
#include "AnimGraphPlugin.h"
#include "BlendGraphWidget.h"
#include "NodeGroupWindow.h"
#include "NavigateWidget.h"
#include "ParameterWindow.h"
#include <QWidget>
#include <QTableWidgetItem>
#include <QGridLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QMenu>
#include <QContextMenuEvent>
#include <QLabel>
#include <QCheckBox>
#include <QTreeWidgetItem>
#include <QMessageBox>

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>

#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSystem.h>

#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/SaveChangedFilesManager.h"

#include <MCore/Source/LogManager.h>
#include <MCore/Source/CommandGroup.h>

#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>


namespace EMStudio
{
    // constructor
    BlendResourceWidget::BlendResourceWidget(AnimGraphPlugin* plugin, QWidget* parent)
        : QWidget(parent)
    {
        mSelectedAnimGraphID   = MCORE_INVALIDINDEX32;
        mPlugin                 = plugin;

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setMargin(0);
        mainLayout->setAlignment(Qt::AlignTop);
        setLayout(mainLayout);

        QVBoxLayout* tableLayout = new QVBoxLayout();
        tableLayout->setMargin(0);
        tableLayout->setSpacing(2);

        // add the buttons to add, remove and clear the motions
        QHBoxLayout* buttonsLayout = new QHBoxLayout();
        buttonsLayout->setSpacing(0);
        buttonsLayout->setAlignment(Qt::AlignLeft);
        tableLayout->addLayout(buttonsLayout);

        mCreateButton       = new QPushButton();
        mRemoveButton       = new QPushButton();
        mClearButton        = new QPushButton();
        mPropertiesButton   = new QPushButton();
        mImportButton       = new QPushButton();
        mSaveButton         = new QPushButton();
        mSaveAsButton       = new QPushButton();

        EMStudioManager::MakeTransparentButton(mImportButton,      "/Images/Icons/Open.png",       "Load anim graph from file");
        EMStudioManager::MakeTransparentButton(mSaveButton,        "/Images/Menu/FileSave.png",    "Save selected anim graphs");
        EMStudioManager::MakeTransparentButton(mSaveAsButton,      "/Images/Menu/FileSaveAs.png",  "Save selected anim graph as a specified filename");
        EMStudioManager::MakeTransparentButton(mCreateButton,      "/Images/Icons/Plus.png",       "Create new anim graph");
        EMStudioManager::MakeTransparentButton(mRemoveButton,      "/Images/Icons/Minus.png",      "Remove selected anim graphs");
        EMStudioManager::MakeTransparentButton(mClearButton,       "/Images/Icons/Clear.png",      "Remove all anim graphs");
        EMStudioManager::MakeTransparentButton(mPropertiesButton,  "/Images/Icons/Edit.png",       "Open anim graph options window");

        buttonsLayout->addWidget(mCreateButton);
        buttonsLayout->addWidget(mRemoveButton);
        buttonsLayout->addWidget(mClearButton);
        buttonsLayout->addWidget(mImportButton);
        buttonsLayout->addWidget(mSaveButton);
        buttonsLayout->addWidget(mSaveAsButton);
        buttonsLayout->addWidget(mPropertiesButton);

        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        buttonsLayout->addWidget(spacerWidget);

        mFindWidget = new MysticQt::SearchButton(this, MysticQt::GetMysticQt()->FindIcon("Images/Icons/SearchClearButton.png"));
        connect(mFindWidget->GetSearchEdit(), SIGNAL(textChanged(const QString&)), this, SLOT(SearchStringChanged(const QString&)));

        QHBoxLayout* searchLayout = new QHBoxLayout();
        searchLayout->addWidget(new QLabel("Find:"), 0, Qt::AlignRight);
        searchLayout->addWidget(mFindWidget);
        searchLayout->setSpacing(6);

        buttonsLayout->addLayout(searchLayout);

        connect(mCreateButton, SIGNAL(released()), this, SLOT(OnCreateAnimGraph()));
        connect(mRemoveButton, SIGNAL(released()), this, SLOT(OnRemoveAnimGraphs()));
        connect(mClearButton, SIGNAL(released()), this, SLOT(OnClearAnimGraphs()));
        connect(mPropertiesButton, SIGNAL(released()), this, SLOT(OnProperties()));
        connect(mImportButton, SIGNAL(released()), mPlugin, SLOT(OnFileOpen()));
        connect(mSaveButton, SIGNAL(released()), this, SLOT(OnFileSave()));
        connect(mSaveAsButton, SIGNAL(released()), this, SLOT(OnFileSaveAs()));

        mTable = new QTableWidget(this);
        mTable->setAlternatingRowColors(true);
        mTable->setGridStyle(Qt::SolidLine);
        mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        mTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
        connect(mTable, SIGNAL(itemSelectionChanged()), this, SLOT(OnItemSelectionChanged()));
        connect(mTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(OnCellDoubleClicked(int, int)));

        // set the minimum size and the resizing policy
        mTable->setMinimumHeight(120);
        mTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // disable the corner button between the row and column selection thingies
        mTable->setCornerButtonEnabled(false);
        mTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mTable->setContextMenuPolicy(Qt::DefaultContextMenu);

        // init the table headers
        mTable->setColumnCount(2);
        mTable->setHorizontalHeaderItem(0, new QTableWidgetItem("Name"));
        mTable->setHorizontalHeaderItem(1, new QTableWidgetItem("FileName"));
        mTable->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTable->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTable->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
        mTable->horizontalHeader()->setStretchLastSection(true);
        mTable->verticalHeader()->setVisible(false);

        // set the name column width
        mTable->setColumnWidth(0, 150);

        // add the table in the layout
        tableLayout->addWidget(mTable, 0, 0);
        mainLayout->addLayout(tableLayout);

        // update the table
        UpdateTable();

        // motion set combo box
        QHBoxLayout* motionSetLayout = new QHBoxLayout();
        motionSetLayout->setAlignment(Qt::AlignLeft);
        motionSetLayout->addWidget(new QLabel("Motion Set:"));
        mMotionSetComboBox = new MysticQt::ComboBox();
        mMotionSetComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        motionSetLayout->addWidget(mMotionSetComboBox);
        mainLayout->addLayout(motionSetLayout);

        // update the motion set and connect
        UpdateMotionSetComboBox();
        connect(mMotionSetComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &BlendResourceWidget::OnMotionSetChanged);

        // create the command callbacks and register them
        mLoadAnimGraphCallback                 = new CommandLoadAnimGraphCallback(true);
        mSaveAnimGraphCallback                 = new CommandSaveAnimGraphCallback(true);
        mCreateAnimGraphCallback               = new CommandCreateAnimGraphCallback(true);
        mRemoveAnimGraphCallback               = new CommandRemoveAnimGraphCallback(true);
        mCloneAnimGraphCallback                = new CommandCloneAnimGraphCallback(true);
        mAdjustAnimGraphCallback               = new CommandAdjustAnimGraphCallback(false);
        mSelectCallback                        = new CommandSelectCallback(false);
        mUnselectCallback                      = new CommandUnselectCallback(false);
        mClearSelectionCallback                = new CommandClearSelectionCallback(false);
        mCreateMotionSetCallback               = new CommandCreateMotionSetCallback(false);
        mRemoveMotionSetCallback               = new CommandRemoveMotionSetCallback(false);
        mAdjustMotionSetCallback               = new CommandAdjustMotionSetCallback(false);
        mSaveMotionSetCallback                 = new CommandSaveMotionSetCallback(false);
        mLoadMotionSetCallback                 = new CommandLoadMotionSetCallback(false);
        mActivateAnimGraphCallback             = new CommandActivateAnimGraphCallback(false);
        mScaleAnimGraphDataCallback            = new CommandScaleAnimGraphDataCallback(false);
        mAnimGraphAddConditionCallback         = new CommandAnimGraphAddConditionCallback(false);
        mAnimGraphRemoveConditionCallback      = new CommandAnimGraphRemoveConditionCallback(false);
        mAnimGraphCreateConnectionCallback     = new CommandAnimGraphCreateConnectionCallback(false);
        mAnimGraphRemoveConnectionCallback     = new CommandAnimGraphRemoveConnectionCallback(false);
        mAnimGraphAdjustConnectionCallback     = new CommandAnimGraphAdjustConnectionCallback(false);
        mAnimGraphCreateNodeCallback           = new CommandAnimGraphCreateNodeCallback(false);
        mAnimGraphAdjustNodeCallback           = new CommandAnimGraphAdjustNodeCallback(false);
        mAnimGraphRemoveNodeCallback           = new CommandAnimGraphRemoveNodeCallback(false);
        mAnimGraphSetEntryStateCallback        = new CommandAnimGraphSetEntryStateCallback(false);
        mAnimGraphAdjustNodeGroupCallback      = new CommandAnimGraphAdjustNodeGroupCallback(false);
        mAnimGraphAddNodeGroupCallback         = new CommandAnimGraphAddNodeGroupCallback(false);
        mAnimGraphRemoveNodeGroupCallback      = new CommandAnimGraphRemoveNodeGroupCallback(false);
        mAnimGraphCreateParameterCallback      = new CommandAnimGraphCreateParameterCallback(false);
        mAnimGraphRemoveParameterCallback      = new CommandAnimGraphRemoveParameterCallback(false);
        mAnimGraphAdjustParameterCallback      = new CommandAnimGraphAdjustParameterCallback(false);
        mAnimGraphSwapParametersCallback       = new CommandAnimGraphSwapParametersCallback(false);
        mAnimGraphAdjustParameterGroupCallback = new CommandAnimGraphAdjustParameterGroupCallback(false);
        mAnimGraphAddParameterGroupCallback    = new CommandAnimGraphAddParameterGroupCallback(false);
        mAnimGraphRemoveParameterGroupCallback = new CommandAnimGraphRemoveParameterGroupCallback(false);
        GetCommandManager()->RegisterCommandCallback("LoadAnimGraph", mLoadAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("SaveAnimGraph", mSaveAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("CreateAnimGraph", mCreateAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveAnimGraph", mRemoveAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("CloneAnimGraph", mCloneAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustAnimGraph", mAdjustAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", mClearSelectionCallback);
        GetCommandManager()->RegisterCommandCallback("CreateMotionSet", mCreateMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveMotionSet", mRemoveMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustMotionSet", mAdjustMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("SaveMotionSet", mSaveMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("LoadMotionSet", mLoadMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("ActivateAnimGraph", mActivateAnimGraphCallback);
        GetCommandManager()->RegisterCommandCallback("ScaleAnimGraphData", mScaleAnimGraphDataCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAddCondition", mAnimGraphAddConditionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveCondition", mAnimGraphRemoveConditionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphCreateConnection", mAnimGraphCreateConnectionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveConnection", mAnimGraphRemoveConnectionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustConnection", mAnimGraphAdjustConnectionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphCreateNode", mAnimGraphCreateNodeCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustNode", mAnimGraphAdjustNodeCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveNode", mAnimGraphRemoveNodeCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphSetEntryState", mAnimGraphSetEntryStateCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustNodeGroup", mAnimGraphAdjustNodeGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAddNodeGroup", mAnimGraphAddNodeGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveNodeGroup", mAnimGraphRemoveNodeGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphCreateParameter", mAnimGraphCreateParameterCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveParameter", mAnimGraphRemoveParameterCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustParameter", mAnimGraphAdjustParameterCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphSwapParameters", mAnimGraphSwapParametersCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustParameterGroup", mAnimGraphAdjustParameterGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAddParameterGroup", mAnimGraphAddParameterGroupCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveParameterGroup", mAnimGraphRemoveParameterGroupCallback);
    }


    // destructor
    BlendResourceWidget::~BlendResourceWidget()
    {
        // unregister and destroy the command callbacks
        GetCommandManager()->RemoveCommandCallback(mLoadAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSaveAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(mCreateAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(mCloneAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mCreateMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSaveMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mLoadMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mActivateAnimGraphCallback, false);
        GetCommandManager()->RemoveCommandCallback(mScaleAnimGraphDataCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAddConditionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphRemoveConditionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphCreateConnectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphRemoveConnectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAdjustConnectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphCreateNodeCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAdjustNodeCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphRemoveNodeCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphSetEntryStateCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAdjustNodeGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAddNodeGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphRemoveNodeGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphCreateParameterCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphRemoveParameterCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAdjustParameterCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphSwapParametersCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAdjustParameterGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphAddParameterGroupCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAnimGraphRemoveParameterGroupCallback, false);

        // delete callbacks
        delete mLoadAnimGraphCallback;
        delete mSaveAnimGraphCallback;
        delete mCreateAnimGraphCallback;
        delete mRemoveAnimGraphCallback;
        delete mCloneAnimGraphCallback;
        delete mAdjustAnimGraphCallback;
        delete mSelectCallback;
        delete mUnselectCallback;
        delete mClearSelectionCallback;
        delete mCreateMotionSetCallback;
        delete mRemoveMotionSetCallback;
        delete mAdjustMotionSetCallback;
        delete mSaveMotionSetCallback;
        delete mLoadMotionSetCallback;
        delete mActivateAnimGraphCallback;
        delete mScaleAnimGraphDataCallback;
        delete mAnimGraphAddConditionCallback;
        delete mAnimGraphRemoveConditionCallback;
        delete mAnimGraphCreateConnectionCallback;
        delete mAnimGraphRemoveConnectionCallback;
        delete mAnimGraphAdjustConnectionCallback;
        delete mAnimGraphCreateNodeCallback;
        delete mAnimGraphAdjustNodeCallback;
        delete mAnimGraphRemoveNodeCallback;
        delete mAnimGraphSetEntryStateCallback;
        delete mAnimGraphAdjustNodeGroupCallback;
        delete mAnimGraphAddNodeGroupCallback;
        delete mAnimGraphRemoveNodeGroupCallback;
        delete mAnimGraphCreateParameterCallback;
        delete mAnimGraphRemoveParameterCallback;
        delete mAnimGraphAdjustParameterCallback;
        delete mAnimGraphSwapParametersCallback;
        delete mAnimGraphAdjustParameterGroupCallback;
        delete mAnimGraphAddParameterGroupCallback;
        delete mAnimGraphRemoveParameterGroupCallback;
    }


    // update the anim graph table contents
    void BlendResourceWidget::UpdateTable()
    {
        // get the current selection list
        const CommandSystem::SelectionList selectionList = GetCommandManager()->GetCurrentSelection();

        // disable signals
        mTable->blockSignals(true);

        // setup the number of rows
        uint32 numDisplayedAnimGraphs = 0;
        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (uint32 i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);
            if (animGraph->GetIsOwnedByRuntime() == false)
            {
                ++numDisplayedAnimGraphs;
            }
        }
        
        mTable->setRowCount(numDisplayedAnimGraphs);

        // disable the sorting
        mTable->setSortingEnabled(false);

        AZStd::string filename, fullFilename;
        const uint32 numSelectedActorInstances = selectionList.GetNumSelectedActorInstances();
        int row = 0;
        for (uint32 i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);
            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            // check if the anim graph is selected
            const bool itemSelected = selectionList.CheckIfHasAnimGraph(animGraph);

            // set the name
            QTableWidgetItem* tableItem1 = new QTableWidgetItem(animGraph->GetName());

            // set the ID as user data
            tableItem1->setData(Qt::UserRole, animGraph->GetID());

            // set the font bold if the anim graph is used by the selected actor instances
            for (uint32 a = 0; a < numSelectedActorInstances; ++a)
            {
                EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(a);
                EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
                if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
                {
                    QFont font = tableItem1->font();
                    font.setBold(true);
                    tableItem1->setFont(font);
                    break;
                }
            }

            // set the table item
            mTable->setItem(row, 0, tableItem1);

            // set the item selected
            mTable->setItemSelected(tableItem1, itemSelected);

            // set the filename
            QTableWidgetItem* tableItem2;
            fullFilename = animGraph->GetFileName();

            if (fullFilename.empty())
            {
                // set the table item
                tableItem2 = new QTableWidgetItem("<not saved yet>");
                mTable->setItem(row, 1, tableItem2);

                // set the item selected
                mTable->setItemSelected(tableItem2, itemSelected);
            }
            else
            {
                AzFramework::StringFunc::Path::GetFullFileName(fullFilename.c_str(), filename);

                // set the table item
                tableItem2 = new QTableWidgetItem(filename.c_str());
                mTable->setItem(row, 1, tableItem2);

                // set the item selected
                mTable->setItemSelected(tableItem2, itemSelected);
            }

            // set the items italic if the anim graph is dirty
            if (animGraph->GetDirtyFlag())
            {
                // create the font italic, all columns use the same font
                QFont font = tableItem1->font();
                font.setItalic(true);

                // set the font for each item
                tableItem1->setFont(font);
                tableItem2->setFont(font);
            }

            // set the row height
            mTable->setRowHeight(row, 21);

            // check if the current item contains the find text
            if (QString(animGraph->GetName()).contains(mFindWidget->GetSearchEdit()->text(), Qt::CaseInsensitive))
            {
                mTable->showRow(row);
            }
            else
            {
                mTable->hideRow(row);
            }

            row++;
        }

        // enable the sorting
        mTable->setSortingEnabled(true);

        // enable signals
        mTable->blockSignals(false);

        // needed to have the interface updated
        OnItemSelectionChanged();
    }


    // when pressing the create button
    void BlendResourceWidget::OnCreateAnimGraph()
    {
        // get the current selection list and the number of actor instances selected
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selectionList.GetNumSelectedActorInstances();

        // activate anim graph if it's the first added
        // a command group is needed if actor instances are selected to activate the anim graph
        if (EMotionFX::GetAnimGraphManager().GetNumAnimGraphs() == 0 && numActorInstances > 0)
        {
            // create the command group
            MCore::CommandGroup commandGroup("Create a anim graph");

            // add the create anim graph command
            commandGroup.AddCommandString("CreateAnimGraph");

            // get the correct motion set
            // nullptr can only be <no motion set> because it's the first anim graph so no one is activated
            // if no motion set selected but one is possible, use the first possible
            // if no motion set selected and no one created, use no motion set
            // if one already selected, use the already selected
            EMotionFX::MotionSet* motionSet = GetSelectedMotionSet();
            if (motionSet == nullptr)
            {
                if (EMotionFX::GetMotionManager().GetNumMotionSets() > 0)
                {
                    motionSet = EMotionFX::GetMotionManager().GetMotionSet(0);
                }
            }

            // get the motion set ID
            const uint32 motionSetID = (motionSet) ? motionSet->GetID() : MCORE_INVALIDINDEX32;

            // activate on each selected actor instance
            for (uint32 i = 0; i < numActorInstances; ++i)
            {
                EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
                m_tempString = AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %%LASTRESULT%% -motionSetID %d", actorInstance->GetID(), motionSetID);
                commandGroup.AddCommandString(m_tempString);
            }

            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
        else
        {
            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommand("CreateAnimGraph", result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }


    // clone selected anim graphs
    void BlendResourceWidget::OnCloneAnimGraphs()
    {
        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = mTable->selectedItems();

        // get the number of selected items
        const uint32 numSelectedItems = selectedItems.count();
        if (numSelectedItems == 0)
        {
            return;
        }

        AZStd::vector<uint32> rowIndices;
        GetSelectedRowIndices(selectedItems, rowIndices);
        const size_t numRowIndices = rowIndices.size();

        // set the command group name
        AZStd::string commandGroupName;
        if (numRowIndices == 1)
        {
            commandGroupName = "Clone 1 anim graph";
        }
        else
        {
            commandGroupName = AZStd::string::format("Clone %d anim graphs", numRowIndices);
        }

        MCore::CommandGroup internalCommandGroup(commandGroupName.c_str());

        // clone each anim graph
        for (uint32 i = 0; i < numRowIndices; ++i)
        {
            // get the anim graph
            const uint32 animGraphID = mTable->item(rowIndices[i], 0)->data(Qt::UserRole).toUInt();
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);

            // add the command
            m_tempString = AZStd::string::format("CloneAnimGraph -animGraphID %i", animGraph->GetID());
            internalCommandGroup.AddCommandString(m_tempString);
        }

        // get the number of anim graphs before clone
        // select all the anim graph after this number, all clone are added at the end
        const uint32 numAnimGraphBeforeClone = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // select all cloned
        mTable->clearSelection();
        const int numRows = mTable->rowCount();
        const int numColumns = mTable->columnCount();
        const uint32 numAnimGraphAfterClone = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (uint32 i = numAnimGraphBeforeClone; i < numAnimGraphAfterClone; ++i)
        {
            // get the anim graph
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            // find the anim graph ID to select the row
            for (int r = 0; r < numRows; ++r)
            {
                if (mTable->item(r, 0)->data(Qt::UserRole).toUInt() == animGraph->GetID())
                {
                    for (int c = 0; c < numColumns; ++c)
                    {
                        mTable->setItemSelected(mTable->item(r, c), true);
                    }
                    break;
                }
            }
        }
    }


    // when pressing the remove button
    void BlendResourceWidget::OnRemoveAnimGraphs()
    {
        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = mTable->selectedItems();

        // get the number of selected items
        const uint32 numSelectedItems = selectedItems.count();
        if (numSelectedItems == 0)
        {
            return;
        }

        AZStd::vector<uint32> rowIndices;
        GetSelectedRowIndices(selectedItems, rowIndices);
        const size_t numRowIndices = rowIndices.size();

        // set the command group name
        AZStd::string commandGroupName;
        if (numRowIndices == 1)
        {
            commandGroupName = "Remove 1 anim graph";
        }
        else
        {
            commandGroupName = AZStd::string::format("Remove %d anim graphs", numRowIndices);
        }

        MCore::CommandGroup internalCommandGroup(commandGroupName.c_str());

        for (uint32 i = 0; i < numRowIndices; ++i)
        {
            const uint32 animGraphID = mTable->item(rowIndices[i], 0)->data(Qt::UserRole).toUInt();
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);

            // in case we modified the anim graph ask if the user wants to save changes it before removing it
            mPlugin->SaveDirtyAnimGraph(animGraph, nullptr, true, false);

            m_tempString = AZStd::string::format("RemoveAnimGraph -animGraphID %i", animGraph->GetID());
            internalCommandGroup.AddCommandString(m_tempString);
        }

        // find the lowest row selected
        uint32 lowestRowSelected = MCORE_INVALIDINDEX32;
        for (uint32 i = 0; i < numRowIndices; ++i)
        {
            if (rowIndices[i] < lowestRowSelected)
            {
                lowestRowSelected = rowIndices[i];
            }
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // selected the next row
        if (lowestRowSelected > ((uint32)mTable->rowCount() - 1))
        {
            mTable->selectRow(lowestRowSelected - 1);
        }
        else
        {
            mTable->selectRow(lowestRowSelected);
        }
    }


    // activate selected anim graph
    void BlendResourceWidget::OnActivateAnimGraph()
    {
        // get the selected anim graph
        EMotionFX::AnimGraph* animGraph = GetSelectedAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // activate the anim graph to selection
        ActivateAnimGraphToSelection(animGraph, GetSelectedMotionSet());
    }


    // when pressing the clear button
    void BlendResourceWidget::OnClearAnimGraphs()
    {
        // make sure we really want to perform the action
        QMessageBox msgBox(QMessageBox::Warning, "Remove All Anim Graphs?", "Are you sure you want to remove all anim graphs?", QMessageBox::Yes | QMessageBox::No, this);
        msgBox.setTextFormat(Qt::RichText);
        if (msgBox.exec() != QMessageBox::Yes)
        {
            return;
        }

        // show the save dirty files window before
        if (mPlugin->OnSaveDirtyAnimGraphs() == DirtyFileManager::CANCELED)
        {
            return;
        }

        // rmove all anim graphs
        CommandSystem::ClearAnimGraphsCommand();
    }


    /*void BlendResourceWidget::SelectAnimGraph(EMotionFX::AnimGraph* animGraph)
    {
        // if the anim graph is not valid, clear the selection
        if (animGraph == nullptr)
        {
            mTable->clearSelection();
            OnItemSelectionChanged();
            return;
        }

        // get the anim graph ID
        const uint32 animGraphID = animGraph->GetID();

        // find the row with the ID
        int row = -1;
        for (int i=0; i<mTable->rowCount(); ++i)
        {
            if (mTable->item(i, 0)->data(Qt::UserRole).toUInt() == animGraphID)
            {
                row = i;
                break;
            }
        }

        // if the row is not found clear the selection
        if (row == -1)
        {
            mTable->clearSelection();
            OnItemSelectionChanged();
            return;
        }

        // select the row found
        mTable->selectRow(row);
    }*/


    // selection changed
    void BlendResourceWidget::OnItemSelectionChanged()
    {
        // clear button if at least one anim graph
        if (EMotionFX::GetAnimGraphManager().GetNumAnimGraphs() > 0)
        {
            mClearButton->setEnabled(true);
        }
        else
        {
            mClearButton->setEnabled(false);
        }

        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = mTable->selectedItems();

        // get the number of selected items
        const uint32 numSelectedItems = selectedItems.count();

        AZStd::vector<uint32> rowIndices;
        GetSelectedRowIndices(selectedItems, rowIndices);
        const int numSelectedRows = static_cast<int>(rowIndices.size());

        // unselect all anim graphs
        GetCommandManager()->GetCurrentSelection().ClearAnimGraphSelection();

        // add each anim graph in the selection list
        for (int i = 0; i < numSelectedRows; ++i)
        {
            const uint32 animGraphID = mTable->item(rowIndices[i], 0)->data(Qt::UserRole).toUInt();
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
            if (animGraph)
            {
                GetCommandManager()->GetCurrentSelection().AddAnimGraph(animGraph);
            }
        }

        // set the active graph using the first selected anim graph, if nothing is selected it will show nothing
        EMotionFX::AnimGraph* firstSelectedAnimGraph = CommandSystem::GetCommandManager()->GetCurrentSelection().GetFirstAnimGraph();
        mPlugin->SetActiveAnimGraph(firstSelectedAnimGraph);

        // remove button enabled if at least one selected
        const bool atLeastOneAnimGraphSelected = numSelectedRows > 0;
        mRemoveButton->setEnabled(atLeastOneAnimGraphSelected);
        mSaveButton->setEnabled(atLeastOneAnimGraphSelected);

        // more than one selected or no one selected
        if (numSelectedRows != 1)
        {
            mSaveAsButton->setEnabled(false);
            mPropertiesButton->setEnabled(false);
            mSelectedAnimGraphID = MCORE_INVALIDINDEX32;
            return;
        }

        // single selection, on this case get the ID and enable actions
        mSelectedAnimGraphID = mTable->item(rowIndices[0], 0)->data(Qt::UserRole).toUInt();
        mSaveButton->setEnabled(true);
        mSaveAsButton->setEnabled(true);
        mPropertiesButton->setEnabled(true);
    }


    // when double clicked
    void BlendResourceWidget::OnCellDoubleClicked(int row, int column)
    {
        MCORE_UNUSED(column);

        // get the anim graph
        const uint32 animGraphID = mTable->item(row, 0)->data(Qt::UserRole).toUInt();
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);

        // activate the anim graph to selection
        ActivateAnimGraphToSelection(animGraph, GetSelectedMotionSet());
    }


    // get the selected anim graph
    EMotionFX::AnimGraph* BlendResourceWidget::GetSelectedAnimGraph()
    {
        // check if the ID is not valid (nothing selected or multiple selection)
        if (mSelectedAnimGraphID == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        // get the anim graph by the ID
        return EMotionFX::GetAnimGraphManager().FindAnimGraphByID(mSelectedAnimGraphID);
    }


    // get the selected motion set
    EMotionFX::MotionSet* BlendResourceWidget::GetSelectedMotionSet()
    {
        // get the current index of the combobox
        const int motionSetComboBoxIndex = mMotionSetComboBox->currentIndex();

        // if the index is valid, get the motion set by this index
        if (motionSetComboBoxIndex > 0)
        {
            return EMotionFX::GetMotionManager().GetMotionSet(motionSetComboBoxIndex - 1);
        }

        // the index is not valid, on this case the motion set is not valid
        return nullptr;
    }


    // activate anim graph to the selected actor instances
    void BlendResourceWidget::ActivateAnimGraphToSelection(EMotionFX::AnimGraph* animGraph, EMotionFX::MotionSet* motionSet)
    {
        if (!animGraph)
        {
            return;
        }

        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selectionList.GetNumSelectedActorInstances();

        // Anim graph can only be activated in case there are actor instances.
        if (numActorInstances == 0)
        {
            return;
        }

        MCore::CommandGroup commandGroup("Activate anim graph");

        // Activate the anim graph each selected actor instance.
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            // Use the given motion set in case it is valid, elsewise use the one previously set to the actor instance.
            uint32 motionSetId = MCORE_INVALIDINDEX32;
            if (motionSet)
            {
                motionSetId = motionSet->GetID();
            }
            else
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
                if (animGraphInstance)
                {
                    EMotionFX::MotionSet* animGraphInstanceMotionSet = animGraphInstance->GetMotionSet();
                    if (animGraphInstanceMotionSet)
                    {
                        motionSetId = animGraphInstanceMotionSet->GetID();
                    }
                }
            }

            m_tempString = AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %d -motionSetID %d", actorInstance->GetID(), animGraph->GetID(), motionSetId);
            commandGroup.AddCommandString(m_tempString);
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void BlendResourceWidget::OnFileSave()
    {
        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = mTable->selectedItems();

        // get the number of selected items
        const uint32 numSelectedItems = selectedItems.count();
        if (numSelectedItems == 0)
        {
            return;
        }

        AZStd::vector<uint32> rowIndices;
        GetSelectedRowIndices(selectedItems, rowIndices);
        const size_t numRowIndices = rowIndices.size();

        // create the command group
        MCore::CommandGroup commandGroup("Save selected anim graphs");
        commandGroup.SetReturnFalseAfterError(true);

        // Add each command
        for (uint32 i = 0; i < numRowIndices; ++i)
        {
            // get the anim graph
            const uint32 animGraphID = mTable->item(rowIndices[i], 0)->data(Qt::UserRole).toUInt();
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);

            // add the anim graph to save and ask the filename in case the filename is empty
            mPlugin->SaveAnimGraph(animGraph, &commandGroup);
        }

        // execute the command group
        // check the number of commands is needed to avoid notification if nothing needs save
        if (commandGroup.GetNumCommands() > 0)
        {
            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, "AnimGraph <font color=red>failed</font> to save");
            }
            else
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, "AnimGraph <font color=green>successfully</font> saved");
            }
        }
    }


    void BlendResourceWidget::OnFileSaveAs()
    {
        EMotionFX::AnimGraph* animGraph = GetSelectedAnimGraph();
        mPlugin->SaveAnimGraphAs(animGraph);
    }


    // constructor
    AnimGraphPropertiesDialog::AnimGraphPropertiesDialog(QWidget* parent, BlendResourceWidget* blendResourceWidget)
        : QDialog(parent)
    {
        mBlendResourceWidget = blendResourceWidget;

        EMotionFX::AnimGraph* animGraph = mBlendResourceWidget->GetSelectedAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        setWindowTitle("Anim Graph Properties");

        QVBoxLayout* verticalLayout = new QVBoxLayout();
        QGridLayout* gridLayout = new QGridLayout();

        QLabel* label = new QLabel("Name:");
        gridLayout->addWidget(label, 0, 0);
        mNameEdit = new QLineEdit();
        mNameEdit->setText(animGraph->GetName());
        connect(mNameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(NameTextEdited(const QString&)));
        gridLayout->addWidget(mNameEdit, 0, 1);

        label = new QLabel("Description:");
        gridLayout->addWidget(label, 1, 0);
        mDescriptionEdit = new QTextEdit();
        mDescriptionEdit->setText(animGraph->GetDescription());
        gridLayout->addWidget(mDescriptionEdit, 1, 1);

        label = new QLabel("Enable Retargeting:");
        gridLayout->addWidget(label, 2, 0);
        mRetargeting = new QCheckBox();
        mRetargeting->setChecked(animGraph->GetRetargetingEnabled());
        gridLayout->addWidget(mRetargeting, 2, 1);
        connect(mRetargeting, SIGNAL(stateChanged(int)), this, SLOT(OnRetargetingChanged(int)));

        QHBoxLayout* horizontalLayout = new QHBoxLayout();
        mOKButton = new QPushButton("OK");
        QPushButton* cancelButton = new QPushButton("Cancel");
        horizontalLayout->addWidget(mOKButton);
        horizontalLayout->addWidget(cancelButton);
        connect(mOKButton, SIGNAL(clicked()), this, SLOT(accept()));
        connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

        verticalLayout->addLayout(gridLayout);
        verticalLayout->addLayout(horizontalLayout);

        setLayout(verticalLayout);
    }


    // name text edited
    void AnimGraphPropertiesDialog::NameTextEdited(const QString& text)
    {
        EMotionFX::AnimGraph* animGraph = mBlendResourceWidget->GetSelectedAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        const AZStd::string convertedNewName = text.toUtf8().data();

        if (text.isEmpty())
        {
            mOKButton->setEnabled(false);
            GetManager()->SetWidgetAsInvalidInput(mNameEdit);
        }
        else if (animGraph->GetName() == convertedNewName)
        {
            mOKButton->setEnabled(true);
            mNameEdit->setStyleSheet("");
        }
        else
        {
            // find duplicate name in all motion sets other than this motion set
            EMotionFX::AnimGraph* duplicateAnimGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByName(convertedNewName.c_str());
            if (duplicateAnimGraph)
            {
                mOKButton->setEnabled(false);
                GetManager()->SetWidgetAsInvalidInput(mNameEdit);
                return;
            }

            // no duplicate name found
            mOKButton->setEnabled(true);
            mNameEdit->setStyleSheet("");
        }
    }


    // when pressing ok
    void AnimGraphPropertiesDialog::accept()
    {
        EMotionFX::AnimGraph* animGraph = mBlendResourceWidget->GetSelectedAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        AZStd::string   newName         = mNameEdit->text().toUtf8().data();
        AZStd::string   newDescription  = mDescriptionEdit->toPlainText().toUtf8().data();
        const bool      newRetargeting  = mRetargeting->isChecked();
        bool            needToCall      = false;

        AZStd::string command;
        command = AZStd::string::format("AdjustAnimGraph -animGraphID %i ", animGraph->GetID());

        if (!newName.empty() && newName != animGraph->GetName())
        {
            command += AZStd::string::format("-name \"%s\" ", newName.c_str());
            needToCall = true;
        }

        if (!newName.empty() && newDescription != animGraph->GetDescription())
        {
            command += AZStd::string::format("-description \"%s\" ", newDescription.c_str());
            needToCall = true;
        }

        if (newRetargeting != animGraph->GetRetargetingEnabled())
        {
            command += AZStd::string::format("-retargetingEnabled %i ", newRetargeting);
            needToCall = true;
        }

        if (needToCall)
        {
            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }

        close();
    }


    // when the properties button got pressed
    void BlendResourceWidget::OnProperties()
    {
        AnimGraphPropertiesDialog* propertiesDialog = new AnimGraphPropertiesDialog(this, this);
        propertiesDialog->open();
    }


    void BlendResourceWidget::SearchStringChanged(const QString& text)
    {
        MCORE_UNUSED(text);
        UpdateTable();
    }


    void BlendResourceWidget::UpdateMotionSetComboBox()
    {
        // block signal to avoid event when the current selected item changed
        mMotionSetComboBox->blockSignals(true);
        mMotionSetComboBox->setStyleSheet("");

        // get the current selected item to set it back in case no one actor instance is selected
        // if actor instances are selected, the used motion set of the anim graph is set if not multiple used or if no one used
        const QString currentSelectedItem = mMotionSetComboBox->currentText();

        // clear the motion set combobox
        mMotionSetComboBox->clear();

        // add the first element : <no motion set>
        mMotionSetComboBox->addItem("<no motion set>");

        // add each motion set name
        const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);
            if (motionSet->GetIsOwnedByRuntime())
            {
                continue;
            }
            mMotionSetComboBox->addItem(motionSet->GetName());
        }

        // get the current selection list and the number of actor instances selected
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selectionList.GetNumSelectedActorInstances();

        // if actor instances are selected, set the used motion set
        if (numActorInstances > 0)
        {
            // add each used motion set in the array (without duplicate)
            // this is used to check if multiple motion sets are used
            AZStd::vector<EMotionFX::MotionSet*> usedMotionSets;
            AZStd::vector<EMotionFX::AnimGraphInstance*> usedAnimGraphs;
            for (uint32 i = 0; i < numActorInstances; ++i)
            {
                EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
                if (actorInstance->GetIsOwnedByRuntime())
                {
                    continue;
                }

                EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
                if (animGraphInstance)
                {
                    EMotionFX::MotionSet* motionSet = animGraphInstance->GetMotionSet();
                    if (AZStd::find(usedMotionSets.begin(), usedMotionSets.end(), motionSet) == usedMotionSets.end())
                    {
                        usedMotionSets.push_back(motionSet);
                    }

                    if (AZStd::find(usedAnimGraphs.begin(), usedAnimGraphs.end(), animGraphInstance) == usedAnimGraphs.end())
                    {
                        usedAnimGraphs.push_back(animGraphInstance);
                    }
                }
            }

            // set the text based on the used motion sets
            if (usedMotionSets.empty())
            {
                // the used motion set array can be empty if no one anim graph activated on the selected actor instances
                // if the current motion set name can be found use it else use the first item
                // the current motion set name can not be found if "<multiple used>" was used
                if (mMotionSetComboBox->findText(currentSelectedItem) == -1)
                {
                    mMotionSetComboBox->setCurrentIndex(0);
                }
                else
                {
                    mMotionSetComboBox->setCurrentText(currentSelectedItem);
                }

                // enable the combo box in case it was disabled before
                mMotionSetComboBox->setEnabled(true);

                if (usedAnimGraphs.size() > 0)
                    mMotionSetComboBox->setStyleSheet("border: 1px solid orange;");
            }
            else if (usedMotionSets.size() == 1)
            {
                // if the motion set used is not valid use the first item else use the motion set name
                if (usedMotionSets[0])
                {
                    mMotionSetComboBox->setCurrentText(usedMotionSets[0]->GetName());
                }
                else
                {
                    mMotionSetComboBox->setCurrentIndex(0);

                    if (usedAnimGraphs.size() > 0)
                        mMotionSetComboBox->setStyleSheet("border: 1px solid orange;");
                }

                // enable the combo box in case it was disabled before
                mMotionSetComboBox->setEnabled(true);
            }
            else
            {
                // clear all items and add only "<multiple used"
                mMotionSetComboBox->clear();
                mMotionSetComboBox->addItem("<multiple used>");

                // disable the combobox to avoid user action
                mMotionSetComboBox->setDisabled(true);
            }
        }
        else // no one actor instance selected, set the old text
        {
            // if the current motion set name can be found use it else use the first item
            // the current motion set name can not be found if "<multiple used>" was used
            if (mMotionSetComboBox->findText(currentSelectedItem) == -1)
            {
                mMotionSetComboBox->setCurrentIndex(0);
            }
            else
            {
                mMotionSetComboBox->setCurrentText(currentSelectedItem);
            }

            // enable the combo box in case it was disabled before
            mMotionSetComboBox->setEnabled(true);
        }

        // enable signals
        mMotionSetComboBox->blockSignals(false);
    }


    void BlendResourceWidget::ChangeMotionSet(EMotionFX::MotionSet* motionSet)
    {
        const AZ::u32 motionSetIndex = EMotionFX::GetMotionManager().FindMotionSetIndex(motionSet);
        if (motionSetIndex == MCORE_INVALIDINDEX32)
        {
            return;
        }

        mMotionSetComboBox->setCurrentIndex(motionSetIndex + 1);
    }


    void BlendResourceWidget::OnMotionSetChanged(int index)
    {
        // get the current selection list and the number of actor instances selected
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selectionList.GetNumSelectedActorInstances();

        // if no one actor instance is selected, the combo box has no effect
        if (numActorInstances == 0)
        {
            return;
        }

        // get the currently selected motion set
        EMotionFX::MotionSet* motionSet = (index > 0) ? EMotionFX::GetMotionManager().GetMotionSet(index - 1) : nullptr;

        // create the command group
        MCore::CommandGroup commandGroup("Change motion set");

        // update the motion set on each actor instance if one anim graph is activated
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // get the actor instance from the selection list and the anim graph instance
            EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();

            // check if one anim graph instance is set on the actor instance
            if (animGraphInstance)
            {
                // check if the motion set is not the same
                if (animGraphInstance->GetMotionSet() != motionSet)
                {
                    // get the current anim graph
                    EMotionFX::AnimGraph* animGraph = animGraphInstance->GetAnimGraph();

                    // add the command in the command group
                    const uint32 motionSetID = (motionSet) ? motionSet->GetID() : MCORE_INVALIDINDEX32;
                    m_tempString = AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %d -motionSetID %d", actorInstance->GetID(), animGraph->GetID(), motionSetID);
                    commandGroup.AddCommandString(m_tempString);
                }
            }
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void BlendResourceWidget::keyPressEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            OnRemoveAnimGraphs();
            event->accept();
            return;
        }

        // base class
        QWidget::keyPressEvent(event);
    }


    void BlendResourceWidget::keyReleaseEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            event->accept();
            return;
        }

        // base class
        QWidget::keyReleaseEvent(event);
    }


    void BlendResourceWidget::contextMenuEvent(QContextMenuEvent* event)
    {
        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = mTable->selectedItems();

        // get the number of selected items
        const uint32 numSelectedItems = selectedItems.count();
        if (numSelectedItems == 0)
        {
            return;
        }

        AZStd::vector<uint32> rowIndices;
        GetSelectedRowIndices(selectedItems, rowIndices);

        // create the context menu
        QMenu menu(this);

        // only one selected
        if (rowIndices.size() == 1)
        {
            QAction* activateAction = menu.addAction("Activate");
            activateAction->setIcon(MysticQt::GetMysticQt()->FindIcon("/Images/AnimGraphPlugin/ShowProcessed.png"));
            connect(activateAction, SIGNAL(triggered()), this, SLOT(OnActivateAnimGraph()));

            QAction* editAction = menu.addAction("Properties");
            editAction->setIcon(MysticQt::GetMysticQt()->FindIcon("/Images/Icons/Edit.png"));
            connect(editAction, SIGNAL(triggered()), this, SLOT(OnProperties()));

            menu.addSeparator();
        }

        // at least one selected
        if (rowIndices.size() > 0)
        {
            QAction* cloneAction = menu.addAction("Clone Selected Anim Graphs");
            cloneAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));
            connect(cloneAction, SIGNAL(triggered()), this, SLOT(OnCloneAnimGraphs()));

            QAction* removeAction = menu.addAction("Remove Selected Anim Graphs");
            removeAction->setIcon(MysticQt::GetMysticQt()->FindIcon("/Images/Icons/Minus.png"));
            connect(removeAction, SIGNAL(triggered()), this, SLOT(OnRemoveAnimGraphs()));

            menu.addSeparator();

            QAction* saveAction = menu.addAction("Save Selected Anim Graphs");
            saveAction->setIcon(MysticQt::GetMysticQt()->FindIcon("/Images/Menu/FileSave.png"));
            connect(saveAction, SIGNAL(triggered()), this, SLOT(OnFileSave()));

            menu.addSeparator();
        }

        // data scaling
        QAction* scaleAction = menu.addAction("Scale Anim Graph Data");
        scaleAction->setToolTip("<b>Scale Anim Graph Data:</b><br>This will scale all internal data of the selected anim graphs. This adjusts all positional data, such as IK handle positions, etc.");
        scaleAction->setIcon(MysticQt::GetMysticQt()->FindIcon("/Images/Rendering/Scale.png"));
        connect(scaleAction, SIGNAL(triggered()), GetMainWindow(), SLOT(OnScaleSelectedAnimGraphs()));

        // show the menu at the given position
        menu.exec(event->globalPos());
    }


    void BlendResourceWidget::GetSelectedRowIndices(const QList<QTableWidgetItem*>& selectedItems, AZStd::vector<uint32>& outRowIndices) const
    {
        outRowIndices.clear();

        const size_t numSelectedItems = static_cast<size_t>(selectedItems.count());
        outRowIndices.reserve(numSelectedItems);

        for (size_t i = 0; i < numSelectedItems; ++i)
        {
            const int rowIndex = selectedItems[static_cast<int>(i)]->row();
            if (AZStd::find(outRowIndices.begin(), outRowIndices.end(), rowIndex) == outRowIndices.end())
            {
                outRowIndices.push_back(rowIndex);
            }
        }
    }

    //----------------------------------------------------------------------------------------------------------------------------------
    // command callbacks
    //----------------------------------------------------------------------------------------------------------------------------------

    bool UpdateTableBlendResourceWidget()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        AnimGraphPlugin* animGraphPlugin = static_cast<AnimGraphPlugin*>(plugin);
        BlendResourceWidget* resourceWidget = animGraphPlugin->GetResourceWidget();

        animGraphPlugin->RemoveAllUnusedGraphInfos();
        resourceWidget->UpdateTable();
        return true;
    }


    bool UpdateTableAndMotionSetBlendResourceWidget()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        AnimGraphPlugin* animGraphPlugin = static_cast<AnimGraphPlugin*>(plugin);
        BlendResourceWidget* resourceWidget = animGraphPlugin->GetResourceWidget();

        animGraphPlugin->RemoveAllUnusedGraphInfos();
        resourceWidget->UpdateTable();
        resourceWidget->UpdateMotionSetComboBox();
        return true;
    }


    bool UpdateAnimGraphPluginAfterActivate()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        AnimGraphPlugin*                animGraphPlugin     = static_cast<AnimGraphPlugin*>(plugin);
        BlendResourceWidget*            resourceWidget      = animGraphPlugin->GetResourceWidget();
        EMStudio::ParameterWindow*      parameterWindow     = animGraphPlugin->GetParameterWindow();
        EMStudio::AttributesWindow*     attributesWindow    = animGraphPlugin->GetAttributesWindow();
        EMStudio::NodeGroupWindow*      nodeGroupWindow     = animGraphPlugin->GetNodeGroupWidget();

        animGraphPlugin->RemoveAllUnusedGraphInfos();
        resourceWidget->UpdateTable();

        resourceWidget->UpdateMotionSetComboBox();

        animGraphPlugin->UpdateStateMachineColors();

        parameterWindow->Init();
        nodeGroupWindow->Init();
        attributesWindow->Reinit();
        return true;
    }


    bool UpdateAnimGraphPluginAfterScaleData()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        AnimGraphPlugin*                animGraphPlugin     = static_cast<AnimGraphPlugin*>(plugin);
        BlendResourceWidget*            resourceWidget      = animGraphPlugin->GetResourceWidget();
        EMStudio::ParameterWindow*      parameterWindow     = animGraphPlugin->GetParameterWindow();
        EMStudio::AttributesWindow*     attributesWindow    = animGraphPlugin->GetAttributesWindow();

        animGraphPlugin->RemoveAllUnusedGraphInfos();
        resourceWidget->UpdateTable();

        parameterWindow->Init();
        attributesWindow->Reinit();
        return true;
    }


    bool UpdateMotionSetComboBoxResourceWidget()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        AnimGraphPlugin* animGraphPlugin = static_cast<AnimGraphPlugin*>(plugin);
        BlendResourceWidget* resourceWidget = animGraphPlugin->GetResourceWidget();
        
        resourceWidget->UpdateMotionSetComboBox();
        return true;
    }


    bool BlendResourceWidget::CommandLoadAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        CommandSystem::CommandLoadAnimGraph* loadAnimGraphCommand = static_cast<CommandSystem::CommandLoadAnimGraph*>(command);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(loadAnimGraphCommand->mOldAnimGraphID);
        if (animGraph)
        {
            OutlinerCategoryItem* outlinerCategoryItem = new OutlinerCategoryItem();
            outlinerCategoryItem->mID = animGraph->GetID();
            outlinerCategoryItem->mUserData = animGraph;
            GetOutlinerManager()->FindCategoryByName("Anim Graphs")->AddItem(outlinerCategoryItem);
        }

        if (commandLine.GetValueAsBool("autoActivate", command))
        {
            EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
            if (plugin)
            {
                AnimGraphPlugin* animGraphPlugin = static_cast<AnimGraphPlugin*>(plugin);
                BlendResourceWidget* resourceWidget = animGraphPlugin->GetResourceWidget();

                const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
                for (uint32 i = 0; i < numMotionSets; ++i)
                {
                    EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);
                    if (motionSet->GetIsOwnedByRuntime())
                    {
                        continue;
                    }

                    resourceWidget->ChangeMotionSet(motionSet);
                    break;
                }

                return UpdateTableAndMotionSetBlendResourceWidget();
            }
        }
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandLoadAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        if (commandLine.GetValueAsBool("autoActivate", command))
        {
            return UpdateTableAndMotionSetBlendResourceWidget();
        }

        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandSaveAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandSaveAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                   { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateTableBlendResourceWidget(); }


    bool BlendResourceWidget::CommandActivateAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)            { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAnimGraphPluginAfterActivate(); }
    bool BlendResourceWidget::CommandActivateAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)               { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAnimGraphPluginAfterActivate(); }


    bool BlendResourceWidget::CommandScaleAnimGraphDataCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)           { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAnimGraphPluginAfterScaleData(); }
    bool BlendResourceWidget::CommandScaleAnimGraphDataCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)              { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAnimGraphPluginAfterScaleData(); }


    bool BlendResourceWidget::CommandCreateAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);
        CommandSystem::CommandCreateAnimGraph* createAnimGraphCommand = static_cast<CommandSystem::CommandCreateAnimGraph*>(command);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(createAnimGraphCommand->mPreviouslyUsedID);
        OutlinerCategoryItem* outlinerCategoryItem = new OutlinerCategoryItem();
        outlinerCategoryItem->mID = animGraph->GetID();
        outlinerCategoryItem->mUserData = animGraph;
        GetOutlinerManager()->FindCategoryByName("Anim Graphs")->AddItem(outlinerCategoryItem);
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandCreateAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                 { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateTableBlendResourceWidget(); }


    bool BlendResourceWidget::CommandRemoveAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);
        CommandSystem::CommandRemoveAnimGraph* removeAnimGraphCommand = static_cast<CommandSystem::CommandRemoveAnimGraph*>(command);
        GetOutlinerManager()->FindCategoryByName("Anim Graphs")->RemoveItem(removeAnimGraphCommand->mOldID);
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandRemoveAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                 { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateTableBlendResourceWidget(); }


    bool BlendResourceWidget::CommandCloneAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);
        CommandSystem::CommandCloneAnimGraph* cloneAnimGraphCommand = static_cast<CommandSystem::CommandCloneAnimGraph*>(command);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(cloneAnimGraphCommand->mOldAnimGraphID);
        OutlinerCategoryItem* outlinerCategoryItem = new OutlinerCategoryItem();
        outlinerCategoryItem->mID = animGraph->GetID();
        outlinerCategoryItem->mUserData = animGraph;
        GetOutlinerManager()->FindCategoryByName("Anim Graphs")->AddItem(outlinerCategoryItem);
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandCloneAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                  { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateTableBlendResourceWidget(); }


    bool BlendResourceWidget::CommandAdjustAnimGraphCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAdjustAnimGraphCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAddConditionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAddConditionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphRemoveConditionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphRemoveConditionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphCreateConnectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphCreateConnectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphRemoveConnectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphRemoveConnectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAdjustConnectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAdjustConnectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphCreateNodeCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphCreateNodeCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAdjustNodeCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAdjustNodeCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphRemoveNodeCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphRemoveNodeCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphSetEntryStateCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphSetEntryStateCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAdjustNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAdjustNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAddNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAddNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphRemoveNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphRemoveNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphCreateParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphCreateParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphRemoveParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphRemoveParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAdjustParameterCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAdjustParameterCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphSwapParametersCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphSwapParametersCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAdjustParameterGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAdjustParameterGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAddParameterGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphAddParameterGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphRemoveParameterGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandAnimGraphRemoveParameterGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return UpdateTableBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false && CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return UpdateTableAndMotionSetBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false && CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return UpdateTableAndMotionSetBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false && CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return UpdateTableAndMotionSetBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasAnimGraphSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return UpdateTableAndMotionSetBlendResourceWidget();
    }


    bool BlendResourceWidget::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)                { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateTableAndMotionSetBlendResourceWidget(); }
    bool BlendResourceWidget::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                   { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateTableAndMotionSetBlendResourceWidget(); }


    //////////////////////////////////////////////////////////////////////////////////////////////////
    // Motion set related callbacks
    //////////////////////////////////////////////////////////////////////////////////////////////////


    bool BlendResourceWidget::CommandAdjustMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (commandLine.CheckIfHasParameter("newName"))
        {
            return UpdateMotionSetComboBoxResourceWidget();
        }
        return true;
    }
    bool BlendResourceWidget::CommandAdjustMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (commandLine.CheckIfHasParameter("newName"))
        {
            return UpdateMotionSetComboBoxResourceWidget();
        }
        return true;
    }
    bool BlendResourceWidget::CommandCreateMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)               { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
    bool BlendResourceWidget::CommandCreateMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                  { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
    bool BlendResourceWidget::CommandRemoveMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)               { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
    bool BlendResourceWidget::CommandRemoveMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                  { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
    bool BlendResourceWidget::CommandSaveMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)                 { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
    bool BlendResourceWidget::CommandSaveMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
    bool BlendResourceWidget::CommandLoadMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)                 { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
    bool BlendResourceWidget::CommandLoadMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateMotionSetComboBoxResourceWidget(); }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendResourceWidget.moc>