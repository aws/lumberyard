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

#include <AzCore/Math/Color.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include "NodeGroupWindow.h"
#include "AnimGraphPlugin.h"
#include "GraphNode.h"
#include "NodeGraph.h"
#include "BlendTreeVisualNode.h"
#include "BlendGraphWidget.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QHeaderView>
#include <QCheckBox>
#include <QMessageBox>
#include <QPushButton>
#include <QKeyEvent>
#include <QMenu>

#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MysticQt/Source/ColorLabel.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeGroupCommands.h>

#include <AzQtComponents/Utilities/Conversions.h>


namespace EMStudio
{
    NodeGroupRenameWindow::NodeGroupRenameWindow(QWidget* parent, EMotionFX::AnimGraph* animGraph, const AZStd::string& nodeGroup)
        : QDialog(parent)
    {
        // Store the values
        mAnimGraph = animGraph;
        mNodeGroup = nodeGroup;

        // set the window title
        setWindowTitle("Rename Node Group");

        // set the minimum width
        setMinimumWidth(300);

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();

        // add the top text
        layout->addWidget(new QLabel("Please enter the new node group name:"));

        // add the line edit
        mLineEdit = new QLineEdit();
        connect(mLineEdit, &QLineEdit::textEdited, this, &NodeGroupRenameWindow::TextEdited);
        layout->addWidget(mLineEdit);

        // set the current name and select all
        mLineEdit->setText(nodeGroup.c_str());
        mLineEdit->selectAll();

        // create add the error message
        /*mErrorMsg = new QLabel("<font color='red'>Error: Duplicate name found</font>");
        mErrorMsg->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mErrorMsg->setVisible(false);*/

        // create the button layout
        QHBoxLayout* buttonLayout   = new QHBoxLayout();
        mOKButton                   = new QPushButton("OK");
        QPushButton* cancelButton   = new QPushButton("Cancel");
        //buttonLayout->addWidget(mErrorMsg);
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(cancelButton);

        // connect the buttons
        connect(mOKButton, &QPushButton::clicked, this, &NodeGroupRenameWindow::Accepted);
        connect(cancelButton, &QPushButton::clicked, this, &NodeGroupRenameWindow::reject);

        // set the new layout
        layout->addLayout(buttonLayout);
        setLayout(layout);
    }


    void NodeGroupRenameWindow::TextEdited(const QString& text)
    {
        const AZStd::string convertedNewName = FromQtString(text);
        if (text.isEmpty())
        {
            //mErrorMsg->setVisible(false);
            mOKButton->setEnabled(false);
            GetManager()->SetWidgetAsInvalidInput(mLineEdit);
        }
        else if (mNodeGroup == convertedNewName)
        {
            //mErrorMsg->setVisible(false);
            mOKButton->setEnabled(true);
            mLineEdit->setStyleSheet("");
        }
        else
        {
            // find duplicate name in the anim graph other than this node group
            const uint32 numNodeGroups = mAnimGraph->GetNumNodeGroups();
            for (uint32 i = 0; i < numNodeGroups; ++i)
            {
                EMotionFX::AnimGraphNodeGroup* nodeGroup = mAnimGraph->GetNodeGroup(i);
                if (nodeGroup->GetNameString() == convertedNewName)
                {
                    //mErrorMsg->setVisible(true);
                    mOKButton->setEnabled(false);
                    GetManager()->SetWidgetAsInvalidInput(mLineEdit);
                    return;
                }
            }

            // no duplicate name found
            //mErrorMsg->setVisible(false);
            mOKButton->setEnabled(true);
            mLineEdit->setStyleSheet("");
        }
    }


    void NodeGroupRenameWindow::Accepted()
    {
        // Execute the command
        AZStd::string commandString, outResult;
        const AZStd::string convertedNewName = FromQtString(mLineEdit->text());
        commandString = AZStd::string::format("AnimGraphAdjustNodeGroup -animGraphID %i -name \"%s\" -newName \"%s\"", mAnimGraph->GetID(), mNodeGroup.c_str(), convertedNewName.c_str());
        if (GetCommandManager()->ExecuteCommand(commandString.c_str(), outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }

        // accept
        accept();
    }

    // constructor
    NodeGroupWindow::NodeGroupWindow(AnimGraphPlugin* plugin)
        : QWidget()
    {
        mPlugin             = plugin;
        mTableWidget        = nullptr;
        mAddButton          = nullptr;

        mWidgetTable.SetMemoryCategory(MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

        // create and register the command callbacks
        mCreateCallback     = new CommandAnimGraphAddNodeGroupCallback(false);
        mRemoveCallback     = new CommandAnimGraphRemoveNodeGroupCallback(false);
        mAdjustCallback     = new CommandAnimGraphAdjustNodeGroupCallback(false);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAddNodeGroup", mCreateCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveNodeGroup", mRemoveCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustNodeGroup", mAdjustCallback);

        // add the add button
        mAddButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mAddButton, "/Images/Icons/Plus.png", "Add new node group");
        connect(mAddButton, &QPushButton::clicked, this, &NodeGroupWindow::OnAddNodeGroup);

        // add the remove button
        mRemoveButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mRemoveButton, "/Images/Icons/Minus.png", "Remove selected node group");
        connect(mRemoveButton, &QPushButton::clicked, this, &NodeGroupWindow::OnRemoveSelectedGroups);

        // add the clear button
        mClearButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mClearButton, "/Images/Icons/Clear.png", "Remove all node groups");
        connect(mClearButton, &QPushButton::clicked, this, &NodeGroupWindow::OnClearNodeGroups);

        // add the buttons to add, remove and clear the motions
        QHBoxLayout* buttonsLayout = new QHBoxLayout();
        buttonsLayout->setSpacing(0);
        buttonsLayout->setAlignment(Qt::AlignLeft);
        buttonsLayout->addWidget(mAddButton);
        buttonsLayout->addWidget(mRemoveButton);
        buttonsLayout->addWidget(mClearButton);

        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        buttonsLayout->addWidget(spacerWidget);

        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &NodeGroupWindow::OnTextFilterChanged);
        buttonsLayout->addWidget(m_searchWidget);

        // create the table widget
        mTableWidget = new QTableWidget();
        mTableWidget->setAlternatingRowColors(true);
        mTableWidget->setCornerButtonEnabled(false);
        mTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        mTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mTableWidget->setContextMenuPolicy(Qt::DefaultContextMenu);
        connect(mTableWidget, &QTableWidget::itemSelectionChanged, this, &NodeGroupWindow::UpdateInterface);
        //connect( mTableWidget, SIGNAL(cellChanged(int, int)), this, SLOT(OnCellChanged(int, int)) );
        //connect( mTableWidget, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(OnNameEdited(QTableWidgetItem*)) );

        // set the column count
        mTableWidget->setColumnCount(3);

        // set header items for the table
        QTableWidgetItem* headerItem = new QTableWidgetItem("Vis");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTableWidget->setHorizontalHeaderItem(0, headerItem);
        headerItem = new QTableWidgetItem("Color");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTableWidget->setHorizontalHeaderItem(1, headerItem);
        headerItem = new QTableWidgetItem("Name");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mTableWidget->setHorizontalHeaderItem(2, headerItem);

        // set the column params
        mTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        mTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
        mTableWidget->setColumnWidth(0, 25);
        mTableWidget->setColumnWidth(1, 41);

        // ser the horizontal header params
        QHeaderView* horizontalHeader = mTableWidget->horizontalHeader();
        horizontalHeader->setSortIndicator(2, Qt::AscendingOrder);
        horizontalHeader->setStretchLastSection(true);

        // hide the vertical header
        QHeaderView* verticalHeader = mTableWidget->verticalHeader();
        verticalHeader->setVisible(false);

        // create the vertical layout
        mVerticalLayout = new QVBoxLayout();
        mVerticalLayout->setSpacing(2);
        mVerticalLayout->setMargin(3);
        mVerticalLayout->setAlignment(Qt::AlignTop);
        mVerticalLayout->addLayout(buttonsLayout);
        mVerticalLayout->addWidget(mTableWidget);

        // set the object name
        setObjectName("StyledWidget");

        // create the fake widget and layout
        QWidget* fakeWidget = new QWidget();
        fakeWidget->setObjectName("StyledWidget");
        fakeWidget->setLayout(mVerticalLayout);

        QVBoxLayout* fakeLayout = new QVBoxLayout();
        fakeLayout->setMargin(0);
        fakeLayout->setSpacing(0);
        fakeLayout->setObjectName("StyledWidget");
        fakeLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
        fakeLayout->addWidget(fakeWidget);

        // set the layout
        setLayout(fakeLayout);

        // init
        Init();
    }


    // destructor
    NodeGroupWindow::~NodeGroupWindow()
    {
        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(mCreateCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustCallback, false);
        delete mCreateCallback;
        delete mRemoveCallback;
        delete mAdjustCallback;
    }


    // create the list of parameters
    void NodeGroupWindow::Init()
    {
        // selected node groups array
        MCore::Array<AZStd::string> selectedNodeGroups;

        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = mTableWidget->selectedItems();

        // get the number of selected items
        const uint32 numSelectedItems = selectedItems.count();

        // filter the items
        selectedNodeGroups.Reserve(numSelectedItems);
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            const uint32 rowIndex = selectedItems[i]->row();
            const AZStd::string nodeGroupName = FromQtString(mTableWidget->item(rowIndex, 2)->text());
            if (selectedNodeGroups.Find(nodeGroupName) == MCORE_INVALIDINDEX32)
            {
                selectedNodeGroups.Add(nodeGroupName);
            }
        }

        // clear the lookup array
        mWidgetTable.Clear(false);

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            mTableWidget->setRowCount(0);
            UpdateInterface();
            return;
        }

        // disable signals
        mTableWidget->blockSignals(true);

        // get the number of node groups
        const uint32 numNodeGroups = animGraph->GetNumNodeGroups();

        // set table size and add header items
        mTableWidget->setRowCount(numNodeGroups);

        // disable the sorting
        mTableWidget->setSortingEnabled(false);

        // add each node group
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            // get a pointer to the node group
            EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(i);

            // check if the node group is selected
            const bool itemSelected = selectedNodeGroups.Find(nodeGroup->GetNameString().c_str()) != MCORE_INVALIDINDEX32;

            // get the color and convert to Qt color
            AZ::Color color;
            color.FromU32(nodeGroup->GetColor());
            const QColor backgroundColor(static_cast<float>(color.GetR()) * 100, static_cast<float>(color.GetG()) * 100, static_cast<float>(color.GetB()) * 100, 50);

            // create the visibility checkbox item
            QTableWidgetItem* visibilityCheckboxItem = new QTableWidgetItem();
            visibilityCheckboxItem->setBackgroundColor(backgroundColor);

            // add the item, it's needed to have the background color + the widget
            mTableWidget->setItem(i, 0, visibilityCheckboxItem);

            // create the visibility checkbox
            QCheckBox* visibilityCheckbox = new QCheckBox();
            visibilityCheckbox->setStyleSheet("background: transparent; padding-left: 5px; max-width: 13px;");
            visibilityCheckbox->setChecked(nodeGroup->GetIsVisible());
            mWidgetTable.Add(WidgetLookup(visibilityCheckbox, i));
            connect(visibilityCheckbox, &QCheckBox::stateChanged, this, &NodeGroupWindow::OnIsVisible);

            // add the visibility checkbox in the table
            mTableWidget->setCellWidget(i, 0, visibilityCheckbox);

            // create the color item
            QTableWidgetItem* colorItem = new QTableWidgetItem();
            colorItem->setBackgroundColor(backgroundColor);

            // add the item, it's needed to have the background color + the widget
            mTableWidget->setItem(i, 1, colorItem);

            // create the color widget
            MysticQt::ColorLabel* colorWidget = new MysticQt::ColorLabel(MCore::RGBAColor(static_cast<float>(color.GetR()), static_cast<float>(color.GetG()), static_cast<float>(color.GetB()), static_cast<float>(color.GetA())), nodeGroup);

            QWidget* colorLayoutWidget = new QWidget();
            colorLayoutWidget->setObjectName("colorlayoutWidget");
            colorLayoutWidget->setStyleSheet("#colorlayoutWidget{ background: transparent; margin: 1px; }");
            QHBoxLayout* colorLayout = new QHBoxLayout();
            colorLayout->setAlignment(Qt::AlignCenter);
            colorLayout->setMargin(0);
            colorLayout->setSpacing(0);
            colorLayout->addWidget(colorWidget);
            colorLayoutWidget->setLayout(colorLayout);

            mWidgetTable.Add(WidgetLookup(colorWidget, i));
            connect(colorWidget, static_cast<void (MysticQt::ColorLabel::*)(const QColor&)>(&MysticQt::ColorLabel::ColorChangeEvent), this, &NodeGroupWindow::OnColorChanged);

            // add the color label in the table
            mTableWidget->setCellWidget(i, 1, colorLayoutWidget);

            // create the node group name label
            QTableWidgetItem* nameItem = new QTableWidgetItem(nodeGroup->GetName());
            nameItem->setBackgroundColor(backgroundColor);
            mTableWidget->setItem(i, 2, nameItem);

            // set the item selected
            mTableWidget->setItemSelected(visibilityCheckboxItem, itemSelected);
            mTableWidget->setItemSelected(colorItem, itemSelected);
            mTableWidget->setItemSelected(nameItem, itemSelected);

            // set the row height
            mTableWidget->setRowHeight(i, 21);

            // check if the current item contains the find text
            if (QString(nodeGroup->GetName()).contains(m_searchWidgetText.c_str(), Qt::CaseInsensitive))
            {
                mTableWidget->showRow(i);
            }
            else
            {
                mTableWidget->hideRow(i);
            }
        }

        // enable the sorting
        mTableWidget->setSortingEnabled(true);

        // enable signals
        mTableWidget->blockSignals(false);

        // update the interface
        UpdateInterface();
    }


    // add a new node group
    void NodeGroupWindow::OnAddNodeGroup()
    {
        // add the parameter
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            MCore::LogWarning("NodeGroupWindow::OnAddNodeGroup() - No AnimGraph active!");
            return;
        }

        AZStd::string commandString;
        AZStd::string resultString;
        commandString = AZStd::string::format("AnimGraphAddNodeGroup -animGraphID %i", animGraph->GetID());

        // execute the command
        if (GetCommandManager()->ExecuteCommand(commandString.c_str(), resultString) == false)
        {
            if (resultString.size() > 0)
            {
                MCore::LogError(resultString.c_str());
            }
        }
        else
        {
            // select the new node group
            EMotionFX::AnimGraphNodeGroup* lastNodeGroup = animGraph->GetNodeGroup(animGraph->GetNumNodeGroups() - 1);
            const int numRows = mTableWidget->rowCount();
            for (int i = 0; i < numRows; ++i)
            {
                if (mTableWidget->item(i, 2)->text() == QString(lastNodeGroup->GetName()))
                {
                    mTableWidget->selectRow(i);
                    break;
                }
            }
        }
    }


    // find the index for the given widget
    uint32 NodeGroupWindow::FindGroupIndexByWidget(QObject* widget) const
    {
        // for all table entries
        const uint32 numWidgets = mWidgetTable.GetLength();
        for (uint32 i = 0; i < numWidgets; ++i)
        {
            if (mWidgetTable[i].mWidget == widget) // this is button we search for
            {
                return mWidgetTable[i].mGroupIndex;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    void NodeGroupWindow::OnIsVisible(int state)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // get the node group index by checking the widget lookup table
        const uint32 groupIndex = FindGroupIndexByWidget(sender());
        assert(groupIndex != MCORE_INVALIDINDEX32);

        // get a pointer to the node group
        EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(groupIndex);

        bool isVisible = state == Qt::Checked;

        // construct the command
        AZStd::string commandString;
        commandString = AZStd::string::format("AnimGraphAdjustNodeGroup -animGraphID %i -name \"%s\" -isVisible %s", animGraph->GetID(), nodeGroup->GetName(), AZStd::to_string(isVisible).c_str());

        // execute the command
        AZStd::string resultString;
        if (GetCommandManager()->ExecuteCommand(commandString.c_str(), resultString) == false)
        {
            if (resultString.size() > 0)
            {
                MCore::LogError(resultString.c_str());
            }
        }
    }


    // node group color changed
    void NodeGroupWindow::OnColorChanged(const QColor& color)
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // get the node group index by checking the widget lookup table
        const uint32 groupIndex = FindGroupIndexByWidget(sender());
        assert(groupIndex != MCORE_INVALIDINDEX32);

        // get a pointer to the node group
        EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(groupIndex);

        // get the color
        int r, g, b;
        color.getRgb(&r, &g, &b);
        AZ::Vector4 finalColor(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);

        // construct the command
        AZStd::string commandString;
        commandString = AZStd::string::format("AnimGraphAdjustNodeGroup -animGraphID %i -name \"%s\" -color \"%s\"", animGraph->GetID(), nodeGroup->GetName(), AZStd::to_string(finalColor).c_str());

        // execute the command
        AZStd::string resultString;
        if (GetCommandManager()->ExecuteCommand(commandString.c_str(), resultString) == false)
        {
            if (resultString.size() > 0)
            {
                MCore::LogError(resultString.c_str());
            }
        }
    }


    void NodeGroupWindow::UpdateInterface()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            mAddButton->setEnabled(false);
            mRemoveButton->setEnabled(false);
            mClearButton->setEnabled(false);
            return;
        }

        mAddButton->setEnabled(true);

        const QList<QTableWidgetItem*> selectedItems = mTableWidget->selectedItems();
        mRemoveButton->setEnabled(selectedItems.count() > 0);

        mClearButton->setEnabled(mTableWidget->rowCount() > 0);
    }


    // remove a node group
    void NodeGroupWindow::OnRemoveSelectedGroups()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = mTableWidget->selectedItems();

        // get the number of selected items
        const uint32 numSelectedItems = selectedItems.count();
        if (numSelectedItems == 0)
        {
            return;
        }

        // filter the items
        MCore::Array<uint32> rowIndices;
        rowIndices.Reserve(numSelectedItems);
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            const uint32 rowIndex = selectedItems[i]->row();
            if (rowIndices.Find(rowIndex) == MCORE_INVALIDINDEX32)
            {
                rowIndices.Add(rowIndex);
            }
        }

        // sort the rows
        // it's used to select the next row
        rowIndices.Sort();

        // get the number of selected rows
        const uint32 numRowIndices = rowIndices.GetLength();

        // set the command group name
        AZStd::string commandGroupName;
        if (numRowIndices == 1)
        {
            commandGroupName = "Remove 1 node group";
        }
        else
        {
            commandGroupName = AZStd::string::format("Remove %d node groups", numRowIndices);
        }

        // create the command group
        MCore::CommandGroup internalCommandGroup(commandGroupName);

        // Add each command
        AZStd::string tempString;
        for (uint32 i = 0; i < numRowIndices; ++i)
        {
            const AZStd::string nodeGroupName = FromQtString(mTableWidget->item(rowIndices[i], 2)->text());
            tempString = AZStd::string::format("AnimGraphRemoveNodeGroup -animGraphID %i -name \"%s\"", animGraph->GetID(), nodeGroupName.c_str());
            internalCommandGroup.AddCommandString(tempString.c_str());
        }

        // execute the command group
        if (GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, tempString) == false)
        {
            MCore::LogError(tempString.c_str());
        }

        // selected the next row
        if (rowIndices[0] > ((uint32)mTableWidget->rowCount() - 1))
        {
            mTableWidget->selectRow(rowIndices[0] - 1);
        }
        else
        {
            mTableWidget->selectRow(rowIndices[0]);
        }
    }


    // rename a node group
    void NodeGroupWindow::OnRenameSelectedNodeGroup()
    {
        // take the item of the name column
        const QList<QTableWidgetItem*> selectedItems = mTableWidget->selectedItems();
        QTableWidgetItem* item = mTableWidget->item(selectedItems[0]->row(), 2);

        // show the rename window
        NodeGroupRenameWindow nodeGroupRenameWindow(this, mPlugin->GetActiveAnimGraph(), FromQtString(item->text()));
        nodeGroupRenameWindow.exec();
    }


    void NodeGroupWindow::OnClearNodeGroups()
    {
        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // make sure we really want to perform the action
        QMessageBox msgBox(QMessageBox::Warning, "Remove All Node Groups?", "Are you sure you want to remove all node groups from the anim graph?", QMessageBox::Yes | QMessageBox::No, this);
        msgBox.setTextFormat(Qt::RichText);
        if (msgBox.exec() != QMessageBox::Yes)
        {
            return;
        }

        CommandSystem::ClearNodeGroups(animGraph);
    }


    void NodeGroupWindow::OnTextFilterChanged(const QString& text)
    {
        FromQtString(text, &m_searchWidgetText);
        Init();
    }


    void NodeGroupWindow::keyPressEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            OnRemoveSelectedGroups();
            event->accept();
            return;
        }

        // base class
        QWidget::keyPressEvent(event);
    }


    void NodeGroupWindow::keyReleaseEvent(QKeyEvent* event)
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


    void NodeGroupWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = mTableWidget->selectedItems();

        // get the number of selected items
        const uint32 numSelectedItems = selectedItems.count();
        if (numSelectedItems == 0)
        {
            return;
        }

        // filter the items
        MCore::Array<uint32> rowIndices;
        rowIndices.Reserve(numSelectedItems);
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            const uint32 rowIndex = selectedItems[i]->row();
            if (rowIndices.Find(rowIndex) == MCORE_INVALIDINDEX32)
            {
                rowIndices.Add(rowIndex);
            }
        }

        // create the context menu
        QMenu menu(this);

        // at least one selected, remove action is possible
        if (rowIndices.GetLength() > 0)
        {
            QAction* removeAction = menu.addAction("Remove Selected Node Groups");
            removeAction->setIcon(MysticQt::GetMysticQt()->FindIcon("/Images/Icons/Minus.png"));
            connect(removeAction, &QAction::triggered, this, &NodeGroupWindow::OnRemoveSelectedGroups);
        }

        // add rename if only one selected
        if (rowIndices.GetLength() == 1)
        {
            QAction* renameAction = menu.addAction("Rename Selected Node Group");
            connect(renameAction, &QAction::triggered, this, &NodeGroupWindow::OnRenameSelectedNodeGroup);
        }

        // show the menu at the given position
        menu.exec(event->globalPos());
    }


    bool UpdateAnimGraphNodeGroupWindow()
    {
        // get the plugin object
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
        AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
        if (plugin == nullptr)
        {
            return false;
        }

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = animGraphPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return false;
        }

        // re-init the node group window
        animGraphPlugin->GetNodeGroupWidget()->Init();
        return true;
    }

    //----------------------------------------------------------------------------------------------------------------------------------
    // command callbacks
    //----------------------------------------------------------------------------------------------------------------------------------

    bool NodeGroupWindow::CommandAnimGraphAddNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAnimGraphNodeGroupWindow(); }
    bool NodeGroupWindow::CommandAnimGraphAddNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)           { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAnimGraphNodeGroupWindow(); }
    bool NodeGroupWindow::CommandAnimGraphRemoveNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAnimGraphNodeGroupWindow(); }
    bool NodeGroupWindow::CommandAnimGraphRemoveNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAnimGraphNodeGroupWindow(); }
    bool NodeGroupWindow::CommandAnimGraphAdjustNodeGroupCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAnimGraphNodeGroupWindow(); }
    bool NodeGroupWindow::CommandAnimGraphAdjustNodeGroupCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateAnimGraphNodeGroupWindow(); }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGroupWindow.moc>
