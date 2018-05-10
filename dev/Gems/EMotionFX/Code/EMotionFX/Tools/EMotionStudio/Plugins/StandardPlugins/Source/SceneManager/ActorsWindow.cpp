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

#include "ActorsWindow.h"
#include "SceneManagerPlugin.h"
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/CommandSystem/Source/ActorCommands.h>
#include <EMotionFX/CommandSystem/Source/ActorInstanceCommands.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include "../../../../EMStudioSDK/Source/UnitScaleWindow.h"
#include <EMotionFX/Source/ActorManager.h>
#include <QContextMenuEvent>
#include <QMessageBox>
#include <QMenu>
#include <QHeaderView>


namespace EMStudio
{
    ActorsWindow::ActorsWindow(SceneManagerPlugin* plugin, QWidget* parent)
        : QWidget(parent)
    {
        mPlugin = plugin;

        // create the layouts
        QVBoxLayout* vLayout = new QVBoxLayout();
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);
        vLayout->setMargin(0);
        hLayout->setSpacing(0);
        vLayout->setSpacing(2);
        hLayout->setAlignment(Qt::AlignLeft);
        vLayout->setAlignment(Qt::AlignTop);

        // create the tree widget
        mTreeWidget = new QTreeWidget();
        mTreeWidget->setObjectName("IsVisibleTreeView");

        // create header items
        mTreeWidget->setColumnCount(2);
        QStringList headerList;
        headerList.append("Name");
        headerList.append("ID");
        mTreeWidget->setHeaderLabels(headerList);

        // set optical stuff for the tree
        mTreeWidget->setColumnWidth(0, 200);
        mTreeWidget->setColumnWidth(1, 20);
        mTreeWidget->setColumnWidth(1, 100);
        mTreeWidget->setSortingEnabled(false);
        mTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mTreeWidget->setMinimumWidth(150);
        mTreeWidget->setMinimumHeight(150);
        mTreeWidget->setAlternatingRowColors(true);
        mTreeWidget->setExpandsOnDoubleClick(true);
        mTreeWidget->setAnimated(true);

        // disable the move of section to have column order fixed
        mTreeWidget->header()->setSectionsMovable(false);

        // create the push buttons
        mLoadActorsButton       = new QPushButton("");
        mCreateInstanceButton   = new QPushButton("");
        mRemoveButton           = new QPushButton("");
        mClearButton            = new QPushButton("");
        mSaveButton             = new QPushButton("");

        EMStudioManager::MakeTransparentButton(mLoadActorsButton,      "/Images/Icons/Open.png",       "Load actors from file");
        EMStudioManager::MakeTransparentButton(mCreateInstanceButton,  "/Images/Icons/Plus.png",       "Create a new instance of the selected actors");
        EMStudioManager::MakeTransparentButton(mRemoveButton,          "/Images/Icons/Minus.png",      "Remove selected actors/actor instances");
        EMStudioManager::MakeTransparentButton(mClearButton,           "/Images/Icons/Clear.png",      "Remove all actors/actor instances");
        EMStudioManager::MakeTransparentButton(mSaveButton,            "/Images/Menu/FileSave.png",    "Save selected actors");

        // add widgets to the layouts
        hLayout->addWidget(mCreateInstanceButton);
        hLayout->addWidget(mRemoveButton);
        hLayout->addWidget(mClearButton);
        hLayout->addWidget(mLoadActorsButton);
        hLayout->addWidget(mSaveButton);
        vLayout->addLayout(hLayout);
        vLayout->addWidget(mTreeWidget);

        // connect
        connect(mLoadActorsButton,     &QPushButton::clicked, GetMainWindow(), &MainWindow::OnFileOpenActor);
        connect(mCreateInstanceButton, SIGNAL(clicked()), this, SLOT(OnCreateInstanceButtonClicked()));
        connect(mRemoveButton,         SIGNAL(clicked()), this, SLOT(OnRemoveButtonClicked()));
        connect(mClearButton,          SIGNAL(clicked()), this, SLOT(OnClearButtonClicked()));
        connect(mSaveButton,           SIGNAL(clicked()), GetMainWindow(), SLOT(OnFileSaveSelectedActors()));
        connect(mTreeWidget,           SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(OnVisibleChanged(QTreeWidgetItem*, int)));
        connect(mTreeWidget,           SIGNAL(itemSelectionChanged()), this, SLOT(OnSelectionChanged()));

        // set the layout
        setLayout(vLayout);
    }


    void ActorsWindow::ReInit()
    {
        // disable signals and clear the tree widget
        mTreeWidget->blockSignals(true);
        mTreeWidget->clear();

        // iterate trough all actors and add them to the tree including their instances
        const uint32 numActors = EMotionFX::GetActorManager().GetNumActors();
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActors; ++i)
        {
            EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

            // ignore visualization actors
            if (actor->GetIsUsedForVisualization())
            {
                continue;
            }

            // ignore engine actors
            if (actor->GetIsOwnedByRuntime())
            {
                continue;
            }

            // create a tree item for the new attachment
            QTreeWidgetItem* newItem = new QTreeWidgetItem(mTreeWidget);

            // add checkboxes to the treeitem
            newItem->setFlags(newItem->flags() | Qt::ItemIsUserCheckable);
            newItem->setCheckState(0, Qt::Checked);

            // adjust text of the treeitem
            AzFramework::StringFunc::Path::GetFileName(actor->GetFileName(), mTempString);
            newItem->setText(0, mTempString.c_str());
            mTempString = AZStd::string::format("%i", actor->GetID());
            newItem->setText(1, mTempString.c_str());
            newItem->setExpanded(true);

            if (actor->GetDirtyFlag())
            {
                QFont font = newItem->font(0);
                font.setItalic(true);
                newItem->setFont(0, font);
            }

            // add as top level item
            mTreeWidget->addTopLevelItem(newItem);

            for (uint32 k = 0; k < numActorInstances; ++k)
            {
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(k);
                if (actorInstance->GetActor() == actor && !actorInstance->GetIsOwnedByRuntime())
                {
                    // create a tree item for the new attachment
                    QTreeWidgetItem* newChildItem = new QTreeWidgetItem(newItem);

                    // add checkboxes to the treeitem
                    newChildItem->setFlags(newChildItem->flags() | Qt::ItemIsUserCheckable);

                    // adjust text of the treeitem
                    newChildItem->setText(0, "Instance");
                    newChildItem->setText(1, AZStd::string::format("%i", actorInstance->GetID()).c_str());
                    newChildItem->setExpanded(true);

                    // add as top level item
                    newItem->addChild(newChildItem);
                }
            }
        }

        // enable signals
        mTreeWidget->blockSignals(false);
    }


    void ActorsWindow::UpdateInterface()
    {
        // get the current selection
        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();

        // disable signals
        mTreeWidget->blockSignals(true);

        const uint32 numTopLevelItems = mTreeWidget->topLevelItemCount();
        for (uint32 i = 0; i < numTopLevelItems; ++i)
        {
            bool allInstancesVisible = true;

            // update the actor items
            QTreeWidgetItem*    item            = mTreeWidget->topLevelItem(i);
            const uint32        actorID         = GetIDFromTreeItem(item);
            EMotionFX::Actor*   actor           = EMotionFX::GetActorManager().FindActorByID(actorID);
            bool                actorSelected   = selection.CheckIfHasActor(actor);

            // set the item selected
            item->setSelected(actorSelected);

            // loop trough all children and adjust selection and checkboxes
            const uint32 numChildren = item->childCount();
            for (uint32 j = 0; j < numChildren; ++j)
            {
                QTreeWidgetItem* child = item->child(j);
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(GetIDFromTreeItem(child));
                if (actorInstance)
                {
                    bool actorInstSelected  = selection.CheckIfHasActorInstance(actorInstance);
                    bool actorInstVisible   = (actorInstance->GetRender());
                    child->setSelected(actorInstSelected);
                    child->setCheckState(0, actorInstVisible ? Qt::Checked : Qt::Unchecked);

                    if (actorInstVisible == false)
                    {
                        allInstancesVisible = false;
                    }
                }
            }

            item->setCheckState(0, allInstancesVisible ? Qt::Checked : Qt::Unchecked);
        }

        // enable signals
        mTreeWidget->blockSignals(false);

        // toggle enabled state of the add/remove buttons
        SetControlsEnabled();
    }


    void ActorsWindow::OnRemoveButtonClicked()
    {
        // get the selected items
        const QList<QTreeWidgetItem*> items = mTreeWidget->selectedItems();

        // create the group
        MCore::CommandGroup commandGroup("Remove Actors/ActorInstances");

        // iterate trough all selected items
        const uint32 numItems = items.length();
        for (uint32 i = 0; i < numItems; ++i)
        {
            // get the item
            QTreeWidgetItem* item = items[i];
            if (item == nullptr)
            {
                continue;
            }

            // get the parent
            QTreeWidgetItem* parent = item->parent();

            // check if the parent is not valid, on this case it's not an instance
            if (parent == nullptr)
            {
                // ask to save if dirty
                const uint32 actorID = GetIDFromTreeItem(item);
                EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
                mPlugin->SaveDirtyActor(actor, &commandGroup, true, false);

                // remove actor instances
                const uint32 numChildren = item->childCount();
                for (uint32 j = 0; j < numChildren; ++j)
                {
                    QTreeWidgetItem* child = item->child(j);

                    mTempString = AZStd::string::format("RemoveActorInstance -actorInstanceID %i", GetIDFromTreeItem(child));
                    commandGroup.AddCommandString(mTempString);
                }

                // remove the actor
                mTempString = AZStd::string::format("RemoveActor -actorID %i", GetIDFromTreeItem(item));
                commandGroup.AddCommandString(mTempString);
            }
            else
            {
                // remove the actor instance
                if (parent->isSelected() == false)
                {
                    mTempString = AZStd::string::format("RemoveActorInstance -actorInstanceID %i", GetIDFromTreeItem(item));
                    commandGroup.AddCommandString(mTempString);
                }
            }
        }

        // execute the group command
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // reinit the window
        mPlugin->ReInit();
    }


    void ActorsWindow::OnClearButtonClicked()
    {
        // ask the user if he really wants to remove all actors and their instances
        if (QMessageBox::question(this, "Remove All Actors And Actor Instances?", "Are you sure you want to remove all actors and all actor instances?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        {
            return;
        }

        // show the save dirty files window before
        if (GetMainWindow()->GetDirtyFileManager()->SaveDirtyFiles(SaveDirtyActorFilesCallback::TYPE_ID) == DirtyFileManager::CANCELED)
        {
            return;
        }

        // clear the scene
        CommandSystem::ClearScene(true, true);
    }


    void ActorsWindow::OnCreateInstanceButtonClicked()
    {
        // create the command group
        MCore::CommandGroup commandGroup("Create actor instances");

        // the actor ID array
        AZStd::vector<uint32> actorIDs;

        // filter the list to keep the actor items only
        const QList<QTreeWidgetItem*> items = mTreeWidget->selectedItems();
        const uint32 numItems = items.length();
        for (uint32 i = 0; i < numItems; ++i)
        {
            // get the item and check if the item is valid
            QTreeWidgetItem* item = items[i];
            if (item == nullptr)
            {
                continue;
            }

            // check if the item is a root
            if (item->parent() == nullptr)
            {
                const uint32 id = GetIDFromTreeItem(item);
                if (id != MCORE_INVALIDINDEX32)
                {
                    actorIDs.push_back(id);
                }
            }
        }

        // clear the current selection
        commandGroup.AddCommandString("ClearSelection");

        // create instances for all selected actors
        AZStd::string command;
        const size_t numIds = actorIDs.size();
        for (size_t i = 0; i < numIds; ++i)
        {
            command = AZStd::string::format("CreateActorInstance -actorID %i", actorIDs[i]);
            commandGroup.AddCommandString(command);
        }

        // execute the group command
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // update the interface
        mPlugin->UpdateInterface();
    }


    void ActorsWindow::OnVisibleChanged(QTreeWidgetItem* item, int column)
    {
        MCORE_UNUSED(column);

        // create the command group
        MCore::CommandGroup commandGroup("Adjust actor instances");

        if (item == nullptr)
        {
            return;
        }

        // adjust visibility
        if (item->parent() == nullptr)
        {
            // loop trough all children and set check state
            const uint32 numChildren = item->childCount();
            for (uint32 i = 0; i < numChildren; ++i)
            {
                QTreeWidgetItem* child = item->child(i);

                mTempString = AZStd::string::format("AdjustActorInstance -actorInstanceId %i -doRender %s", GetIDFromTreeItem(child), AZStd::to_string(item->checkState(0) == Qt::Checked).c_str());
                commandGroup.AddCommandString(mTempString);
            }
        }
        else
        {
            mTempString = AZStd::string::format("AdjustActorInstance -actorInstanceId %i -doRender %s", GetIDFromTreeItem(item), AZStd::to_string(item->checkState(0) == Qt::Checked).c_str());
            commandGroup.AddCommandString(mTempString);
        }

        // execute the command group
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // update the interface
        mPlugin->UpdateInterface();
    }


    void ActorsWindow::OnSelectionChanged()
    {
        // create the command group
        MCore::CommandGroup commandGroup("Select actors/actor instances");

        // make sure selection is unlocked
        bool selectionLocked = GetCommandManager()->GetLockSelection();
        if (selectionLocked)
        {
            commandGroup.AddCommandString("ToggleLockSelection");
        }

        // get the selected items
        const uint32 numTopItems = mTreeWidget->topLevelItemCount();
        for (uint32 i = 0; i < numTopItems; ++i)
        {
            // selection of the topLevelItems
            QTreeWidgetItem* topLevelItem = mTreeWidget->topLevelItem(i);
            if (topLevelItem->isSelected())
            {
                mTempString = AZStd::string::format("Select -actorID %i", GetIDFromTreeItem(topLevelItem));
                commandGroup.AddCommandString(mTempString);
            }
            else
            {
                mTempString = AZStd::string::format("Unselect -actorID %i", GetIDFromTreeItem(topLevelItem));
                commandGroup.AddCommandString(mTempString);
            }

            // loop trough the children and adjust selection there
            uint32 numChilds = topLevelItem->childCount();
            for (uint32 j = 0; j < numChilds; ++j)
            {
                QTreeWidgetItem* child = topLevelItem->child(j);
                if (child->isSelected())
                {
                    mTempString = AZStd::string::format("Select -actorInstanceID %i", GetIDFromTreeItem(child));
                    commandGroup.AddCommandString(mTempString);
                }
                else
                {
                    mTempString = AZStd::string::format("Unselect -actorInstanceID %i", GetIDFromTreeItem(child));
                    commandGroup.AddCommandString(mTempString);
                }
            }
        }

        // execute the command group
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // update enabled flag for the add/remove instance buttons
        SetControlsEnabled();
    }


    void ActorsWindow::OnResetTransformationOfSelectedActorInstances()
    {
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand("ResetToBindPose", result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void ActorsWindow::OnCloneSelected()
    {
        CommandSystem::CloneSelectedActorInstances();
    }


    void ActorsWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        const QList<QTreeWidgetItem*> items = mTreeWidget->selectedItems();

        // get number of selected items and top level items
        const uint32 numSelected = items.size();
        const uint32 numTopItems = mTreeWidget->topLevelItemCount();

        // create the context menu
        QMenu menu(this);
        menu.setToolTipsVisible(true);

        bool actorSelected = false;
        for (uint32 i = 0; i < numSelected; ++i)
        {
            QTreeWidgetItem* item = items[i];
            if (item->parent() == nullptr)
            {
                actorSelected = true;
                break;
            }
        }

        bool instanceSelected = false;
        const int selectedItemCount = items.count();
        for (int i = 0; i < selectedItemCount; ++i)
        {
            if (items[i]->parent())
            {
                instanceSelected = true;
                break;
            }
        }

        if (numSelected > 0)
        {
            if (instanceSelected)
            {
                QAction* resetTransformationAction = menu.addAction("Reset Transforms");
                connect(resetTransformationAction, SIGNAL(triggered()), this, SLOT(OnResetTransformationOfSelectedActorInstances()));

                menu.addSeparator();

                QAction* hideAction = menu.addAction("Hide Selected Actor Instances");
                connect(hideAction, SIGNAL(triggered()), this, SLOT(OnHideSelected()));

                QAction* unhideAction = menu.addAction("Unhide Selected Actor Instances");
                connect(unhideAction, SIGNAL(triggered()), this, SLOT(OnUnhideSelected()));

                menu.addSeparator();
            }

            if (actorSelected)
            {
                QAction* addAction = menu.addAction("Create New Actor Instances");
                addAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));
                connect(addAction, SIGNAL(triggered()), this, SLOT(OnCreateInstanceButtonClicked()));
            }

            if (instanceSelected)
            {
                QAction* cloneAction = menu.addAction("Clone Selected Actor Instances");
                cloneAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));
                connect(cloneAction, SIGNAL(triggered()), this, SLOT(OnCloneSelected()));
            }

            QAction* removeAction = menu.addAction("Remove Selected Actors/Actor Instances");
            removeAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Minus.png"));
            connect(removeAction, SIGNAL(triggered()), this, SLOT(OnRemoveButtonClicked()));
        }

        if (numTopItems > 0)
        {
            QAction* clearAction = menu.addAction("Remove All Actors/Actor Instances");
            clearAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Clear.png"));
            connect(clearAction, SIGNAL(triggered()), this, SLOT(OnClearButtonClicked()));

            menu.addSeparator();
        }

        QAction* loadAction = menu.addAction("Load Actors From File");
        loadAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Open.png"));
        connect(loadAction, &QAction::triggered, GetMainWindow(), &MainWindow::OnFileOpenActor);

        if (numSelected > 0)
        {
            QAction* saveAction = menu.addAction("Save Selected Actors");
            saveAction->setIcon(MysticQt::GetMysticQt()->FindIcon("/Images/Menu/FileSave.png"));
            connect(saveAction, SIGNAL(triggered()), GetMainWindow(), SLOT(OnFileSaveSelectedActors()));
        }

        // show the menu at the given position
        menu.exec(event->globalPos());
    }


    void ActorsWindow::keyPressEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            OnRemoveButtonClicked();
            event->accept();
            return;
        }

        // base class
        QWidget::keyPressEvent(event);
    }


    void ActorsWindow::keyReleaseEvent(QKeyEvent* event)
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


    void ActorsWindow::SetControlsEnabled()
    {
        // check if table widget was set
        if (mTreeWidget == nullptr)
        {
            return;
        }

        // get number of selected items and top level items
        const QList<QTreeWidgetItem*> items = mTreeWidget->selectedItems();
        const uint32 numSelected = items.size();
        const uint32 numTopItems = mTreeWidget->topLevelItemCount();

        // check if at least one actor selected
        bool actorSelected = false;
        for (uint32 i = 0; i < numSelected; ++i)
        {
            QTreeWidgetItem* item = items[i];
            if (item->parent() == nullptr)
            {
                actorSelected = true;
                break;
            }
        }

        // set the enabled state of the buttons
        mCreateInstanceButton->setEnabled(actorSelected);
        mRemoveButton->setEnabled(numSelected != 0);
        mSaveButton->setEnabled(numSelected != 0);
        mClearButton->setEnabled(numTopItems > 0);
    }


    void ActorsWindow::SetVisibilityFlags(bool isVisible)
    {
        // create the command group
        MCore::CommandGroup commandGroup(isVisible ? "Unhide actor instances" : "Hide actor instances");

        // create the instances of the selected actors
        const QList<QTreeWidgetItem*> items = mTreeWidget->selectedItems();
        const uint32 numItems = items.length();
        for (uint32 i = 0; i < numItems; ++i)
        {
            // check if parent or child item
            QTreeWidgetItem* item = items[i];
            if (item == nullptr || item->parent() == nullptr)
            {
                continue;
            }

            // extract the id from the given item
            const uint32 id = GetIDFromTreeItem(item);

            mTempString = AZStd::string::format("AdjustActorInstance -actorInstanceId %i -doRender %s", id, AZStd::to_string(isVisible).c_str());
            commandGroup.AddCommandString(mTempString);
        }

        // execute the command group
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // update the interface
        mPlugin->UpdateInterface();
    }


    uint32 ActorsWindow::GetIDFromTreeItem(QTreeWidgetItem* item)
    {
        if (item == nullptr)
        {
            return MCORE_INVALIDINDEX32;
        }
        return AzFramework::StringFunc::ToInt(FromQtString(item->text(1)).c_str());
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/SceneManager/ActorsWindow.moc>