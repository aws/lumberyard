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
#include "RetargetMappingWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QTableWidget>
#include <QLabel>
#include <QSplitter>
#include <QLineEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/NodeMap.h>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <MysticQt/Source/MysticQtManager.h>
#include "../../../../EMStudioSDK/Source/NotificationWindow.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"


namespace EMStudio
{
    // constructor
    RetargetMappingWindow::RetargetMappingWindow(QWidget* parent, ActorSetupPlugin* plugin)
        : QWidget(parent)
    {
        mPlugin         = plugin;
        mSourceActor    = nullptr;

        // load some icons
        mBoneIcon       = new QIcon(MCore::String(MysticQt::GetDataDir() + "Images/Icons/Bone.png").AsChar());
        mNodeIcon       = new QIcon(MCore::String(MysticQt::GetDataDir() + "Images/Icons/Node.png").AsChar());
        mMeshIcon       = new QIcon(MCore::String(MysticQt::GetDataDir() + "Images/Icons/Mesh.png").AsChar());
        mMappedIcon     = new QIcon(MCore::String(MysticQt::GetDataDir() + "Images/Icons/Confirm.png").AsChar());

        // create the main layout
        QVBoxLayout* mainLayout = new QVBoxLayout();
        setLayout(mainLayout);

        // add a label
        mainLayout->setMargin(0);

        // create the layout that contains the two listboxes next to each other, and in the middle the link button
        QHBoxLayout* topPartLayout = new QHBoxLayout();
        topPartLayout->setMargin(0);

        QHBoxLayout* toolBarLayout = new QHBoxLayout();
        toolBarLayout->setMargin(0);
        toolBarLayout->setSpacing(1);
        mainLayout->addLayout(toolBarLayout);
        mButtonOpen = new QPushButton();
        EMStudioManager::MakeTransparentButton(mButtonOpen,    "/Images/Icons/Open.png",       "Load and apply a mapping template.");
        connect(mButtonOpen, SIGNAL(clicked()), this, SLOT(OnLoadMapping()));
        mButtonSave = new QPushButton();
        EMStudioManager::MakeTransparentButton(mButtonSave,    "/Images/Menu/FileSave.png",    "Save the currently setup mapping as template.");
        connect(mButtonSave, SIGNAL(clicked()), this, SLOT(OnSaveMapping()));
        mButtonClear = new QPushButton();
        EMStudioManager::MakeTransparentButton(mButtonClear,   "/Images/Icons/Clear.png",      "Clear the currently setup mapping entirely.");
        connect(mButtonClear, SIGNAL(clicked()), this, SLOT(OnClearMapping()));

        toolBarLayout->addWidget(mButtonOpen, 0, Qt::AlignLeft);
        toolBarLayout->addWidget(mButtonSave, 0, Qt::AlignLeft);
        toolBarLayout->addWidget(mButtonClear, 0, Qt::AlignLeft);
        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        toolBarLayout->addWidget(spacerWidget);

        // create the two widgets used to create a splitter in between
        QWidget* upperWidget = new QWidget();
        QWidget* lowerWidget = new QWidget();
        QSplitter* splitter = new QSplitter(Qt::Vertical);
        splitter->addWidget(upperWidget);
        splitter->addWidget(lowerWidget);
        mainLayout->addWidget(splitter);
        upperWidget->setLayout(topPartLayout);

        // left listbox part
        QVBoxLayout* leftListLayout = new QVBoxLayout();
        leftListLayout->setMargin(0);
        leftListLayout->setSpacing(1);
        topPartLayout->addLayout(leftListLayout);

        // add the search filter button
        QHBoxLayout* curSearchLayout = new QHBoxLayout();
        leftListLayout->addLayout(curSearchLayout);
        QLabel* curLabel = new QLabel("<b>Current Actor</b>");
        curLabel->setTextFormat(Qt::RichText);
        curSearchLayout->addWidget(curLabel);
        spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        curSearchLayout->addWidget(spacerWidget);
        curSearchLayout->addWidget(new QLabel("Find:"), 0, Qt::AlignRight);
        mCurrentSearchButton = new MysticQt::SearchButton(this, MysticQt::GetMysticQt()->FindIcon("Images/Icons/SearchClearButton2.png"));
        connect(mCurrentSearchButton->GetSearchEdit(), SIGNAL(textChanged(const QString&)), this, SLOT(CurrentFilterStringChanged(const QString&)));
        curSearchLayout->addWidget(mCurrentSearchButton);
        curSearchLayout->setSpacing(6);
        curSearchLayout->setMargin(0);

        mCurrentList = new QTableWidget();
        mCurrentList->setAlternatingRowColors(true);
        mCurrentList->setGridStyle(Qt::SolidLine);
        mCurrentList->setSelectionBehavior(QAbstractItemView::SelectRows);
        mCurrentList->setSelectionMode(QAbstractItemView::SingleSelection);
        //mCurrentList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        mCurrentList->setCornerButtonEnabled(false);
        mCurrentList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mCurrentList->setContextMenuPolicy(Qt::DefaultContextMenu);
        mCurrentList->setColumnCount(3);
        mCurrentList->setColumnWidth(0, 20);
        mCurrentList->setColumnWidth(1, 20);
        mCurrentList->setSortingEnabled(true);
        QHeaderView* verticalHeader = mCurrentList->verticalHeader();
        verticalHeader->setVisible(false);
        QTableWidgetItem* headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mCurrentList->setHorizontalHeaderItem(0, headerItem);
        headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mCurrentList->setHorizontalHeaderItem(1, headerItem);
        headerItem = new QTableWidgetItem("Name");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mCurrentList->setHorizontalHeaderItem(2, headerItem);
        mCurrentList->horizontalHeader()->setStretchLastSection(true);
        mCurrentList->horizontalHeader()->setSortIndicatorShown(false);
        mCurrentList->horizontalHeader()->setSectionsClickable(false);
        connect(mCurrentList, SIGNAL(itemSelectionChanged()), this, SLOT(OnCurrentListSelectionChanged()));
        connect(mCurrentList, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(OnCurrentListDoubleClicked(QTableWidgetItem*)));
        leftListLayout->addWidget(mCurrentList);

        // add link button middle part
        QVBoxLayout* middleLayout = new QVBoxLayout();
        middleLayout->setMargin(0);
        topPartLayout->addLayout(middleLayout);
        QPushButton* linkButton = new QPushButton("link");
        connect(linkButton, SIGNAL(clicked()), this, SLOT(OnLinkPressed()));
        middleLayout->addWidget(linkButton);

        // right listbox part
        QVBoxLayout* rightListLayout = new QVBoxLayout();
        rightListLayout->setMargin(0);
        rightListLayout->setSpacing(1);
        topPartLayout->addLayout(rightListLayout);

        // add the search filter button
        QHBoxLayout* sourceSearchLayout = new QHBoxLayout();
        rightListLayout->addLayout(sourceSearchLayout);
        QLabel* sourceLabel = new QLabel("<b>Source Actor</b>");
        sourceLabel->setTextFormat(Qt::RichText);
        sourceSearchLayout->addWidget(sourceLabel);
        QPushButton* loadSourceButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(loadSourceButton,   "/Images/Icons/Open.png",   "Load a source actor");
        connect(loadSourceButton, SIGNAL(clicked()), this, SLOT(OnLoadSourceActor()));
        sourceSearchLayout->addWidget(loadSourceButton, 0, Qt::AlignLeft);
        spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sourceSearchLayout->addWidget(spacerWidget);
        sourceSearchLayout->addWidget(new QLabel("Find:"), 0, Qt::AlignRight);
        mSourceSearchButton = new MysticQt::SearchButton(this, MysticQt::GetMysticQt()->FindIcon("Images/Icons/SearchClearButton2.png"));
        connect(mSourceSearchButton->GetSearchEdit(), SIGNAL(textChanged(const QString&)), this, SLOT(SourceFilterStringChanged(const QString&)));
        sourceSearchLayout->addWidget(mSourceSearchButton);
        sourceSearchLayout->setSpacing(6);
        sourceSearchLayout->setMargin(0);


        mSourceList = new QTableWidget();
        mSourceList->setAlternatingRowColors(true);
        mSourceList->setGridStyle(Qt::SolidLine);
        mSourceList->setSelectionBehavior(QAbstractItemView::SelectRows);
        mSourceList->setSelectionMode(QAbstractItemView::SingleSelection);
        //mSourceList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        mSourceList->setCornerButtonEnabled(false);
        mSourceList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mSourceList->setContextMenuPolicy(Qt::DefaultContextMenu);
        mSourceList->setColumnCount(3);
        mSourceList->setColumnWidth(0, 20);
        mSourceList->setColumnWidth(1, 20);
        mSourceList->setSortingEnabled(true);
        verticalHeader = mSourceList->verticalHeader();
        verticalHeader->setVisible(false);
        headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mSourceList->setHorizontalHeaderItem(0, headerItem);
        headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mSourceList->setHorizontalHeaderItem(1, headerItem);
        headerItem = new QTableWidgetItem("Name");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mSourceList->setHorizontalHeaderItem(2, headerItem);
        mSourceList->horizontalHeader()->setStretchLastSection(true);
        mSourceList->horizontalHeader()->setSortIndicatorShown(false);
        mSourceList->horizontalHeader()->setSectionsClickable(false);
        connect(mSourceList, SIGNAL(itemSelectionChanged()), this, SLOT(OnSourceListSelectionChanged()));
        rightListLayout->addWidget(mSourceList);

        // create the mapping table
        QVBoxLayout* lowerLayout = new QVBoxLayout();
        lowerLayout->setMargin(0);
        lowerWidget->setLayout(lowerLayout);

        QHBoxLayout* mappingLayout = new QHBoxLayout();
        mappingLayout->setMargin(0);
        lowerLayout->addLayout(mappingLayout);
        mappingLayout->addWidget(new QLabel("Mapping:"), 0, Qt::AlignLeft);
        mappingLayout->setSpacing(1);
        mButtonGuess = new QPushButton();
        EMStudioManager::MakeTransparentButton(mButtonGuess,   "/Images/Icons/Character.png",  "Best guess mapping");
        connect(mButtonGuess, SIGNAL(clicked()), this, SLOT(OnBestGuess()));
        mappingLayout->addWidget(mButtonGuess, 0, Qt::AlignLeft);
        spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        mappingLayout->addWidget(spacerWidget);

        mMappingTable = new QTableWidget();
        lowerLayout->addWidget(mMappingTable);
        mMappingTable->setAlternatingRowColors(true);
        mMappingTable->setGridStyle(Qt::SolidLine);
        mMappingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        mMappingTable->setSelectionMode(QAbstractItemView::SingleSelection);
        //mMappingTable->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        mMappingTable->setCornerButtonEnabled(false);
        mMappingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mMappingTable->setContextMenuPolicy(Qt::DefaultContextMenu);
        mMappingTable->setContentsMargins(3, 1, 3, 1);
        mMappingTable->setColumnCount(2);
        mMappingTable->setColumnWidth(0, mMappingTable->width() / 2);
        mMappingTable->setColumnWidth(1, mMappingTable->width() / 2);
        verticalHeader = mMappingTable->verticalHeader();
        verticalHeader->setVisible(false);

        // add the table headers
        headerItem = new QTableWidgetItem("Current Actor Node");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mMappingTable->setHorizontalHeaderItem(0, headerItem);
        headerItem = new QTableWidgetItem("Source Actor Node");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mMappingTable->setHorizontalHeaderItem(1, headerItem);
        mMappingTable->horizontalHeader()->setStretchLastSection(true);
        mMappingTable->horizontalHeader()->setSortIndicatorShown(false);
        mMappingTable->horizontalHeader()->setSectionsClickable(false);
        connect(mMappingTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(OnMappingTableDoubleClicked(QTableWidgetItem*)));
        connect(mMappingTable, SIGNAL(itemSelectionChanged()), this, SLOT(OnMappingTableSelectionChanged()));
    }


    // destructor
    RetargetMappingWindow::~RetargetMappingWindow()
    {
        if (mSourceActor)
        {
            mSourceActor->Destroy();
        }
        delete mBoneIcon;
        delete mNodeIcon;
        delete mMeshIcon;
        delete mMappedIcon;
    }


    // double clicked on an item in the table
    void RetargetMappingWindow::OnMappingTableDoubleClicked(QTableWidgetItem* item)
    {
        MCORE_UNUSED(item);
        // TODO: open a node hierarchy widget, where we can select a node from the mSourceActor
        // the problem is that the node hierarchy widget works with actor instances, which we don't have and do not really want to create either
        // I think the node hierarchy widget shouldn't use actor instances only, but should support actors as well
    }


    // when we double click on an item in the current node list
    void RetargetMappingWindow::OnCurrentListDoubleClicked(QTableWidgetItem* item)
    {
        // get the currently selected actor
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* currentActor = selection.GetSingleActor();
        if (currentActor == nullptr)
        {
            return;
        }

        if (item->column() != 0)
        {
            return;
        }

        // get the node name
        const uint32 rowIndex = item->row();
        MCore::String nodeName = FromQtString(mCurrentList->item(rowIndex, 2)->text());

        // find its index in the current actor, and remove its mapping
        EMotionFX::Node* node = currentActor->GetSkeleton()->FindNodeByName(nodeName.AsChar());
        MCORE_ASSERT(node);

        // remove the mapping for this node
        PerformMapping(node->GetNodeIndex(), MCORE_INVALIDINDEX32);
    }


    // current list selection changed
    void RetargetMappingWindow::OnCurrentListSelectionChanged()
    {
        QList<QTableWidgetItem*> items = mCurrentList->selectedItems();
        if (items.count() > 0)
        {
            //const uint32 currentListRow = items[0]->row();
            QTableWidgetItem* nameItem = mCurrentList->item(items[0]->row(), 2);
            QList<QTableWidgetItem*> mappingTableItems = mMappingTable->findItems(nameItem->text(), Qt::MatchExactly);

            for (int32 i = 0; i < mappingTableItems.count(); ++i)
            {
                if (mappingTableItems[i]->column() != 0)
                {
                    continue;
                }

                const uint32 rowIndex = mappingTableItems[i]->row();

                mMappingTable->selectRow(rowIndex);
                mMappingTable->setCurrentItem(mappingTableItems[i]);
            }
        }
    }


    // source list selection changed
    void RetargetMappingWindow::OnSourceListSelectionChanged()
    {
        QList<QTableWidgetItem*> items = mSourceList->selectedItems();
        if (items.count() > 0)
        {
            //const uint32 currentListRow = items[0]->row();
            QTableWidgetItem* nameItem = mSourceList->item(items[0]->row(), 2);
            QList<QTableWidgetItem*> mappingTableItems = mMappingTable->findItems(nameItem->text(), Qt::MatchExactly);

            for (int32 i = 0; i < mappingTableItems.count(); ++i)
            {
                if (mappingTableItems[i]->column() != 1)
                {
                    continue;
                }

                const uint32 rowIndex = mappingTableItems[i]->row();

                mMappingTable->selectRow(rowIndex);
                mMappingTable->setCurrentItem(mappingTableItems[i]);
            }
        }
    }


    // selection changed in the table
    void RetargetMappingWindow::OnMappingTableSelectionChanged()
    {
        // select both items in the list widgets as well
        QList<QTableWidgetItem*> items = mMappingTable->selectedItems();
        if (items.count() > 0)
        {
            const uint32 rowIndex = items[0]->row();

            QTableWidgetItem* item = mMappingTable->item(rowIndex, 0);
            if (item)
            {
                QList<QTableWidgetItem*> listItems = mCurrentList->findItems(item->text(), Qt::MatchExactly);
                if (listItems.count() > 0)
                {
                    mCurrentList->selectRow(listItems[0]->row());
                    mCurrentList->setCurrentItem(listItems[0]);
                }
            }

            item = mMappingTable->item(rowIndex, 1);
            if (item)
            {
                QList<QTableWidgetItem*> listItems = mSourceList->findItems(item->text(), Qt::MatchExactly);
                if (listItems.count() > 0)
                {
                    mSourceList->selectRow(listItems[0]->row());
                    mSourceList->setCurrentItem(listItems[0]);
                }
            }
        }
    }


    // reinitialize
    void RetargetMappingWindow::Reinit(bool reInitMap)
    {
        // clear the filter strings
        mCurrentSearchButton->GetSearchEdit()->setText("");
        mSourceSearchButton->GetSearchEdit()->setText("");

        // get the currently selected actor
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* currentActor = selection.GetSingleActor();

        // extract the list of bones
        if (currentActor)
        {
            currentActor->ExtractBoneList(0, &mCurrentBoneList);
        }

        if (mSourceActor)
        {
            mSourceActor->ExtractBoneList(0, &mSourceBoneList);
        }

        // clear the node map
        if (reInitMap)
        {
            mMap.Clear(false);
            if (currentActor)
            {
                const uint32 numNodes = currentActor->GetNumNodes();
                mMap.Resize(numNodes);
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    mMap[i] = MCORE_INVALIDINDEX32;
                }
            }
        }

        // fill the contents
        FillCurrentListWidget(currentActor, "");
        FillSourceListWidget(mSourceActor, "");
        FillMappingTable(currentActor, mSourceActor);

        // enable or disable the filter fields
        mCurrentSearchButton->setEnabled((currentActor));
        mSourceSearchButton->setEnabled((mSourceActor));

        //
        UpdateToolBar();
    }


    // current actor filter change
    void RetargetMappingWindow::CurrentFilterStringChanged(const QString& text)
    {
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* currentActor = selection.GetSingleActor();
        FillCurrentListWidget(currentActor, text);
    }


    // source actor filter change
    void RetargetMappingWindow::SourceFilterStringChanged(const QString& text)
    {
        FillSourceListWidget(mSourceActor, text);
    }


    // fill the current list widget
    void RetargetMappingWindow::FillCurrentListWidget(EMotionFX::Actor* actor, const QString& filterString)
    {
        if (actor == nullptr)
        {
            mCurrentList->setRowCount(0);
            return;
        }

        // fill the left list widget
        QString currentName;
        const uint32 numNodes = actor->GetNumNodes();

        // count the number of rows
        uint32 numRows = 0;
        for (uint32 i = 0; i < numNodes; ++i)
        {
            currentName = actor->GetSkeleton()->GetNode(i)->GetName();
            if (currentName.contains(filterString, Qt::CaseInsensitive) || filterString.isEmpty())
            {
                numRows++;
            }
        }
        mCurrentList->setRowCount(numRows);

        // fill the rows
        uint32 rowIndex = 0;
        for (uint32 i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            currentName = node->GetName();
            if (currentName.contains(filterString, Qt::CaseInsensitive) || filterString.isEmpty())
            {
                // mark if there is a mapping or not
                const bool mapped = (mMap[node->GetNodeIndex()] != MCORE_INVALIDINDEX32);
                if (mapped)
                {
                    QTableWidgetItem* mappedItem = new QTableWidgetItem();
                    mappedItem->setIcon(*mMappedIcon);
                    mCurrentList->setItem(rowIndex, 0, mappedItem);
                }

                // pick the right icon for the type column
                QTableWidgetItem* typeItem = new QTableWidgetItem();
                if (actor->GetMesh(0, node->GetNodeIndex()))
                {
                    typeItem->setIcon(*mMeshIcon);
                }
                else
                if (mCurrentBoneList.Contains(node->GetNodeIndex()))
                {
                    typeItem->setIcon(*mBoneIcon);
                }
                else
                {
                    typeItem->setIcon(*mNodeIcon);
                }

                mCurrentList->setItem(rowIndex, 1, typeItem);

                // set the name
                QTableWidgetItem* currentTableItem = new QTableWidgetItem(currentName);
                mCurrentList->setItem(rowIndex, 2, currentTableItem);

                // set the row height and add one index
                mCurrentList->setRowHeight(rowIndex, 21);
                ++rowIndex;
            }
        }
    }


    // fill the source list widget
    void RetargetMappingWindow::FillSourceListWidget(EMotionFX::Actor* actor, const QString& filterString)
    {
        if (actor == nullptr)
        {
            mSourceList->setRowCount(0);
            return;
        }

        // fill the left list widget
        QString name;
        const uint32 numNodes = actor->GetNumNodes();

        // count the number of rows
        uint32 numRows = 0;
        for (uint32 i = 0; i < numNodes; ++i)
        {
            name = actor->GetSkeleton()->GetNode(i)->GetName();
            if (name.contains(filterString, Qt::CaseInsensitive) || filterString.isEmpty())
            {
                numRows++;
            }
        }
        mSourceList->setRowCount(numRows);

        // fill the rows
        uint32 rowIndex = 0;
        for (uint32 i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            name = node->GetName();
            if (name.contains(filterString, Qt::CaseInsensitive) || filterString.isEmpty())
            {
                // mark if there is a mapping or not
                const bool mapped = mMap.Contains(i);
                if (mapped)
                {
                    QTableWidgetItem* mappedItem = new QTableWidgetItem();
                    mappedItem->setIcon(*mMappedIcon);
                    mSourceList->setItem(rowIndex, 0, mappedItem);
                }

                // pick the right icon for the type column
                QTableWidgetItem* typeItem = new QTableWidgetItem();
                if (actor->GetMesh(0, node->GetNodeIndex()))
                {
                    typeItem->setIcon(*mMeshIcon);
                }
                else
                if (mSourceBoneList.Contains(node->GetNodeIndex()))
                {
                    typeItem->setIcon(*mBoneIcon);
                }
                else
                {
                    typeItem->setIcon(*mNodeIcon);
                }

                mSourceList->setItem(rowIndex, 1, typeItem);

                // set the name
                QTableWidgetItem* currentTableItem = new QTableWidgetItem(name);
                mSourceList->setItem(rowIndex, 2, currentTableItem);

                // set the row height and add one index
                mSourceList->setRowHeight(rowIndex, 21);
                ++rowIndex;
            }
        }
    }


    // fill the mapping table
    void RetargetMappingWindow::FillMappingTable(EMotionFX::Actor* currentActor, EMotionFX::Actor* sourceActor)
    {
        MCORE_UNUSED(sourceActor);

        if (currentActor == nullptr)
        {
            mMappingTable->setRowCount(0);
            return;
        }

        // fill the table
        QString currentName;
        QString sourceName;
        const uint32 numNodes = currentActor->GetNumNodes();
        mMappingTable->setRowCount(numNodes);
        for (uint32 i = 0; i < numNodes; ++i)
        {
            currentName = currentActor->GetSkeleton()->GetNode(i)->GetName();

            QTableWidgetItem* currentTableItem = new QTableWidgetItem(currentName);
            mMappingTable->setItem(i, 0, currentTableItem);
            mMappingTable->setRowHeight(i, 21);

            if (mMap[i] != MCORE_INVALIDINDEX32)
            {
                sourceName = mSourceActor->GetSkeleton()->GetNode(mMap[i])->GetName();
                currentTableItem = new QTableWidgetItem(sourceName);
                mMappingTable->setItem(i, 1, currentTableItem);
            }
        }
        //mMappingTable->resizeColumnsToContents();
        //mMappingTable->setColumnWidth(0, mMappingTable->columnWidth(0) + 25);
    }


    // load a source actor
    void RetargetMappingWindow::OnLoadSourceActor()
    {
        // load the actor
        const AZStd::string filename = GetMainWindow()->GetFileManager()->LoadActorFileDialog(this);
        if (filename.empty())
        {
            return;
        }

        // load the new actor
        if (mSourceActor)
        {
            mSourceActor->Destroy();
        }
        EMotionFX::Importer::ActorSettings settings; // we don't need geometry LODs and morph targets
        settings.mLoadGeometryLODs      = false;
        settings.mLoadMorphTargets      = false;
        settings.mLoadMeshes            = false;
        settings.mLoadSkinningInfo      = false;
        settings.mLoadCollisionMeshes   = false;
        settings.mLoadStandardMaterialLayers = false;
        mSourceActor = EMotionFX::GetImporter().LoadActor(filename.c_str(), &settings);

        if (mSourceActor)
        {
            mSourceActor->SetFileName(filename.c_str());
        }

        // reinitialize the window contents
        Reinit();
    }


    // pressing the link button
    void RetargetMappingWindow::OnLinkPressed()
    {
        if (mCurrentList->currentRow() == -1 || mSourceList->currentRow() == -1)
        {
            return;
        }

        // get the names
        QTableWidgetItem* curItem = mCurrentList->currentItem();
        QTableWidgetItem* sourceItem = mSourceList->currentItem();
        if (curItem == nullptr || sourceItem == nullptr)
        {
            return;
        }

        curItem = mCurrentList->item(curItem->row(), 2);
        sourceItem = mSourceList->item(sourceItem->row(), 2);

        const MCore::String currentNodeName = FromQtString(curItem->text());
        const MCore::String sourceNodeName  = FromQtString(mSourceList->currentItem()->text());
        if (sourceNodeName.GetLength() == 0 || currentNodeName == 0)
        {
            return;
        }

        // get the currently selected actor
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* currentActor = selection.GetSingleActor();
        MCORE_ASSERT(currentActor && mSourceActor);

        EMotionFX::Node* currentNode = currentActor->GetSkeleton()->FindNodeByName(currentNodeName.AsChar());
        EMotionFX::Node* sourceNode  = mSourceActor->GetSkeleton()->FindNodeByName(sourceNodeName.AsChar());
        MCORE_ASSERT(currentNode && sourceNode);

        //MCore::LogInfo("%s (%d) -> %s (%d)", currentNode->GetName(), currentNode->GetNodeIndex(), sourceNode->GetName(), sourceNode->GetNodeIndex());
        PerformMapping(currentNode->GetNodeIndex(), sourceNode->GetNodeIndex());
    }


    // perform the mapping
    void RetargetMappingWindow::PerformMapping(uint32 currentNodeIndex, uint32 sourceNodeIndex)
    {
        const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* currentActor = selection.GetSingleActor();

        // update the map
        const uint32 oldSourceIndex = mMap[currentNodeIndex];
        mMap[currentNodeIndex] = sourceNodeIndex;

        // update the current table
        const QString curName = currentActor->GetSkeleton()->GetNode(currentNodeIndex)->GetName();
        const QList<QTableWidgetItem*> currentListItems = mCurrentList->findItems(curName, Qt::MatchExactly);
        for (int32 i = 0; i < currentListItems.count(); ++i)
        {
            const uint32 rowIndex = currentListItems[i]->row();
            //if (rowIndex != mCurrentList->currentRow())
            //  continue;

            QTableWidgetItem* mappedItem = mCurrentList->item(rowIndex, 0);
            if (mappedItem == nullptr)
            {
                mappedItem = new QTableWidgetItem();
                mCurrentList->setItem(rowIndex, 0, mappedItem);
            }

            if (sourceNodeIndex == MCORE_INVALIDINDEX32)
            {
                mappedItem->setIcon(QIcon());
            }
            else
            {
                mappedItem->setIcon(*mMappedIcon);
            }
        }

        // update source table
        if (sourceNodeIndex != MCORE_INVALIDINDEX32)
        {
            const bool stillUsed = mMap.Contains(sourceNodeIndex);
            const QString sourceName = mSourceActor->GetSkeleton()->GetNode(sourceNodeIndex)->GetName();
            const QList<QTableWidgetItem*> sourceListItems = mSourceList->findItems(sourceName, Qt::MatchExactly);
            for (int32 i = 0; i < sourceListItems.count(); ++i)
            {
                const uint32 rowIndex = sourceListItems[i]->row();
                //if (rowIndex != mSourceList->currentRow())
                //  continue;

                QTableWidgetItem* mappedItem = mSourceList->item(rowIndex, 0);
                if (mappedItem == nullptr)
                {
                    mappedItem = new QTableWidgetItem();
                    mSourceList->setItem(rowIndex, 0, mappedItem);
                }

                if (stillUsed == false)
                {
                    mappedItem->setIcon(QIcon());
                }
                else
                {
                    mappedItem->setIcon(*mMappedIcon);
                }
            }
        }
        else // we're clearing it
        {
            if (oldSourceIndex != MCORE_INVALIDINDEX32)
            {
                const bool stillUsed = mMap.Contains(oldSourceIndex);
                const QString sourceName = mSourceActor->GetSkeleton()->GetNode(oldSourceIndex)->GetName();
                const QList<QTableWidgetItem*> sourceListItems = mSourceList->findItems(sourceName, Qt::MatchExactly);
                for (int32 i = 0; i < sourceListItems.count(); ++i)
                {
                    const uint32 rowIndex = sourceListItems[i]->row();
                    //if (rowIndex != mSourceList->currentRow())
                    //  continue;

                    QTableWidgetItem* mappedItem = mSourceList->item(rowIndex, 0);
                    if (mappedItem == nullptr)
                    {
                        mappedItem = new QTableWidgetItem();
                        mSourceList->setItem(rowIndex, 0, mappedItem);
                    }

                    if (stillUsed == false)
                    {
                        mappedItem->setIcon(QIcon());
                    }
                    else
                    {
                        mappedItem->setIcon(*mMappedIcon);
                    }
                }
            }
        }

        // update the mapping table
        QTableWidgetItem* item = mMappingTable->item(currentNodeIndex, 1);
        if (item == nullptr && sourceNodeIndex != MCORE_INVALIDINDEX32)
        {
            item = new QTableWidgetItem();
            mMappingTable->setItem(currentNodeIndex, 1, item);
        }

        if (sourceNodeIndex == MCORE_INVALIDINDEX32)
        {
            if (item)
            {
                item->setText("");
            }
        }
        else
        {
            item->setText(mSourceActor->GetSkeleton()->GetNode(sourceNodeIndex)->GetName());
        }

        // update toolbar icons
        UpdateToolBar();
    }


    // press a key
    void RetargetMappingWindow::keyPressEvent(QKeyEvent* event)
    {
        event->ignore();
    }


    // releasing a key
    void RetargetMappingWindow::keyReleaseEvent(QKeyEvent* event)
    {
        // if we press delete, remove the mapping
        if (event->key() == Qt::Key_Delete)
        {
            RemoveCurrentSelectedMapping();
            event->accept();
            return;
        }

        event->ignore();
    }


    // remove the currently selected mapping
    void RetargetMappingWindow::RemoveCurrentSelectedMapping()
    {
        QList<QTableWidgetItem*> items = mCurrentList->selectedItems();
        if (items.count() > 0)
        {
            QTableWidgetItem* item = mCurrentList->item(items[0]->row(), 0);
            if (item)
            {
                OnCurrentListDoubleClicked(item);
            }
        }
    }


    // load and apply a mapping
    void RetargetMappingWindow::OnLoadMapping()
    {
        // make sure we have both a current actor and source actor
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* currentActor = selection.GetSingleActor();
        if (currentActor == nullptr)
        {
            MCore::LogWarning("There is no current actor set, a mapping cannot be loaded, please select an actor first!");
            QMessageBox::critical(this, "Cannot Save!", "You need to select a current actor before you can load and apply this node map!", QMessageBox::Ok);
            return;
        }

        // get the filename to save as
        const AZStd::string filename = GetMainWindow()->GetFileManager()->LoadNodeMapFileDialog(this);
        if (filename.empty())
        {
            return;
        }

        // load the node map file from disk
        MCore::LogInfo("Loading node map from file '%s'", filename.c_str());
        EMotionFX::NodeMap* nodeMap = EMotionFX::GetImporter().LoadNodeMap(filename.c_str());
        if (nodeMap == nullptr)
        {
            MCore::LogWarning("Failed to load the node map!");
            QMessageBox::warning(this, "Failed Loading", "Loading of the node map file failed.", QMessageBox::Ok);
        }
        else
        {
            MCore::LogInfo("Loading of node map is successful, applying now...");
        }

        // check if the node map has a source actor
        if (nodeMap->GetSourceActor())
        {
            mSourceActor = nodeMap->GetSourceActor();
            nodeMap->SetAutoDeleteSourceActor(false);
        }
        else
        {
            // check if we already loaded one manually
            if (mSourceActor == nullptr)
            {
                MCore::LogWarning("EMStudio::RetargetMappingWindow::OnLoadMapping() - There is no source actor to use, please manually load one first.");
                QMessageBox::warning(this, "No Source Actor", "Loading of the source actor inside the node map failed (or didn't contain one) and there is currently none set. Please manually load a source actor first and try again.", QMessageBox::Ok);
                return;
            }
        }

        // now update our mapping data
        const uint32 numNodes = currentActor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mMap[i] = MCORE_INVALIDINDEX32;
        }

        // now apply the map we loaded to the data we have here
        const uint32 numEntries = nodeMap->GetNumEntries();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            // find the current node
            EMotionFX::Node* currentNode = currentActor->GetSkeleton()->FindNodeByName(nodeMap->GetFirstName(i));
            if (currentNode == nullptr)
            {
                continue;
            }

            // find the source node
            EMotionFX::Node* sourceNode = mSourceActor->GetSkeleton()->FindNodeByName(nodeMap->GetSecondName(i));
            if (sourceNode == nullptr)
            {
                continue;
            }

            // create the mapping
            mMap[currentNode->GetNodeIndex()] = sourceNode->GetNodeIndex();
        }

        // reinit the interface parts
        Reinit(false);

        // get rid of the nodeMap object as we don't need it anymore
        nodeMap->Destroy();
    }


    // save the mapping
    void RetargetMappingWindow::OnSaveMapping()
    {
        // get the currently selected actor
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* currentActor = selection.GetSingleActor();
        if (currentActor == nullptr || mSourceActor == nullptr)
        {
            MCore::LogWarning("There is no current and/or source actor set, there is nothing to save!");
            QMessageBox::warning(this, "Nothing To Save!", "You need to select both a current actor and load a source actor before you can save a map!", QMessageBox::Ok);
            return;
        }

        // check if we got something to save at all
        if (CheckIfIsMapEmpty())
        {
            MCore::LogWarning("The node map is empty, there is nothing to save!");
            QMessageBox::warning(this, "Nothing To Save!", "The node map is empty, there is nothing to save!", QMessageBox::Ok);
            return;
        }

        // read the filename to save as
        const AZStd::string filename = GetMainWindow()->GetFileManager()->SaveNodeMapFileDialog(this);
        if (filename.empty())
        {
            return;
        }

        MCore::LogInfo("Saving node map as '%s'", filename.c_str());

        // create an emfx node map object
        EMotionFX::NodeMap* map = EMotionFX::NodeMap::Create();
        const uint32 numNodes = currentActor->GetNumNodes();
        map->Reserve(numNodes);
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // skip unmapped entries
            if (mMap[i] == MCORE_INVALIDINDEX32)
            {
                continue;
            }

            // add the entry to the map if it doesn't yet exist
            if (map->GetHasEntry(currentActor->GetSkeleton()->GetNode(i)->GetName()) == false)
            {
                map->AddEntry(currentActor->GetSkeleton()->GetNode(i)->GetName(), mSourceActor->GetSkeleton()->GetNode(mMap[i])->GetName());
            }
        }

        // set the filename, in case we do something with it while saving later on
        map->SetFileName(filename.c_str());

        // save as little endian
        if (map->Save(filename.c_str(), MCore::Endian::ENDIAN_LITTLE) == false)
        {
            MCore::LogWarning("Failed to save node map file '%s', is it maybe in use or is the location read only?", filename.c_str());
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, "Node map <font color=red>failed</font> to save");
        }
        else
        {
            MCore::LogInfo("Saving of node map successfully completed");
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, "Node map <font color=green>successfully</font> saved");
        }

        map->Destroy();
    }


    // clear the mapping
    void RetargetMappingWindow::OnClearMapping()
    {
        if (QMessageBox::warning(this, "Clear Current Mapping?", "Are you sure you want to clear the current mapping?\nAll mapping information will be lost.", QMessageBox::Cancel | QMessageBox::Yes) != QMessageBox::Yes)
        {
            return;
        }

        // reinitialize, which also clears the map
        Reinit();
    }


    // check if the current map is empty
    bool RetargetMappingWindow::CheckIfIsMapEmpty() const
    {
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* currentActor = selection.GetSingleActor();

        if (currentActor == nullptr)
        {
            return true;
        }

        const uint32 numNodes = currentActor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            if (mMap[i] != MCORE_INVALIDINDEX32)
            {
                return false;
            }
        }

        return true;
    }


    // update the toolbar icons
    void RetargetMappingWindow::UpdateToolBar()
    {
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* currentActor = selection.GetSingleActor();

        // check which buttons have to be enabled/disabled
        const bool canOpen  = (currentActor);
        const bool canSave  = (CheckIfIsMapEmpty() == false && currentActor && mSourceActor);
        const bool canClear = (CheckIfIsMapEmpty() == false);
        const bool canGuess = (currentActor && mSourceActor);

        // enable or disable them
        mButtonOpen->setEnabled(canOpen);
        mButtonSave->setEnabled(canSave);
        mButtonClear->setEnabled(canClear);
        mButtonGuess->setEnabled(canGuess);
    }


    // perform best guess mapping
    void RetargetMappingWindow::OnBestGuess()
    {
        // get the current and source actor
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* currentActor = selection.GetSingleActor();
        if (currentActor == nullptr || mSourceActor == nullptr)
        {
            return;
        }

        // show a warning that we will overwrite the table entries
        if (QMessageBox::warning(this, "Overwrite Mapping?", "Are you sure you want to possibly overwrite items in the mapping?\nAll or some existing mapping information might be lost.", QMessageBox::Cancel | QMessageBox::Yes) != QMessageBox::Yes)
        {
            return;
        }

        // for all nodes in the current actor
        uint32 numGuessed = 0;
        const uint32 numNodes = currentActor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // skip already setup mappings
            if (mMap[i] != MCORE_INVALIDINDEX32)
            {
                continue;
            }

            const EMotionFX::Node* currentNode = currentActor->GetSkeleton()->GetNode(i);

            // try to see if we have a node that matches the name
            const EMotionFX::Node* sourceNode = mSourceActor->GetSkeleton()->FindNodeByName(currentNode->GetName());
            if (sourceNode)
            {
                mMap[i] = sourceNode->GetNodeIndex();
                numGuessed++;
            }
        }

        Reinit(false);

        // show some results
        MCore::String resultString;
        resultString.Format("We guessed a mapping for %d nodes.", numGuessed);
        QMessageBox::information(this, "Guess Mapping Results", resultString.AsChar());
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/SceneManager/RetargetMappingWindow.moc>