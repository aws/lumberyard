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
#include "OutlinerPlugin.h"
#include <AzCore/std/containers/vector.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/OutlinerManager.h"
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QListWidget>
#include <QSplitter>
#include <QKeyEvent>
#include <QMenu>
#include <QPushButton>


namespace EMStudio
{
    struct OutlinerCategoryWithIds
    {
        OutlinerCategory* mCategory;
        AZStd::vector<uint32> mIDs;
    };


    struct OutlinerCategoryWithItems
    {
        OutlinerCategory* mCategory;
        AZStd::vector<OutlinerCategoryItem*> mItems;
    };


    class OutlinerListWidgetItem
        : public QListWidgetItem
    {
    public:
        bool operator < (const QListWidgetItem& other) const override
        {
            return text().compare(other.text(), Qt::CaseInsensitive) < 0;
        }
    };


    class OutlinerTableWidgetItem
        : public QTableWidgetItem
    {
    public:
        OutlinerTableWidgetItem()
            : QTableWidgetItem()
        {
        }

        OutlinerTableWidgetItem(const QString& text)
            : QTableWidgetItem(text)
        {
        }

        bool operator < (const QTableWidgetItem& other) const override
        {
            return text().compare(other.text(), Qt::CaseInsensitive) < 0;
        }
    };


    class OutlinerTableWidget
        : public QTableWidget
    {
    public:
        OutlinerTableWidget(QWidget* parent = nullptr)
            : QTableWidget(parent)
        {
        }

        void RemoveSelectedItems()
        {
            // get the current selection
            const QList<QTableWidgetItem*> items = selectedItems();

            // get the number of selected items
            const uint32 numSelectedItems = items.count();
            if (numSelectedItems == 0)
            {
                return;
            }

            // filter the items
            AZStd::vector<uint32> rowIndices;
            rowIndices.reserve(numSelectedItems);
            for (size_t i = 0; i < numSelectedItems; ++i)
            {
                const uint32 rowIndex = items[static_cast<int>(i)]->row();
                if (AZStd::find(rowIndices.begin(), rowIndices.end(), rowIndex) == rowIndices.end())
                {
                    rowIndices.push_back(rowIndex);
                }
            }

            // sort the items by category
            AZStd::vector<OutlinerCategoryWithItems> selectedItemsSorted;
            const size_t numRows = rowIndices.size();
            for (size_t i = 0; i < numRows; ++i)
            {
                // get the category item
                OutlinerCategoryItem* categoryItem = (OutlinerCategoryItem*)item(rowIndices[i], 0)->data(Qt::UserRole).value<void*>();

                // add if category found or create a new category with ids item
                bool categoryFound = false;
                const size_t numSelectedItemsSorted = selectedItemsSorted.size();
                for (size_t j = 0; j < numSelectedItemsSorted; ++j)
                {
                    if (selectedItemsSorted[j].mCategory == categoryItem->mCategory)
                    {
                        selectedItemsSorted[j].mItems.push_back(categoryItem);
                        categoryFound = true;
                        break;
                    }
                }
                if (categoryFound == false)
                {
                    OutlinerCategoryWithItems newCategoryWithItems;
                    newCategoryWithItems.mCategory = categoryItem->mCategory;
                    newCategoryWithItems.mItems.push_back(categoryItem);
                    selectedItemsSorted.push_back(newCategoryWithItems);
                }
            }

            // create the command group
            MCore::CommandGroup commandGroup("Remove outliner items");

            // call the remove items callback for each registered category
            const size_t numSelectedItemsSorted = selectedItemsSorted.size();
            AZStd::vector<OutlinerCategoryWithItems> removeFailedSorted;
            for (size_t i = 0; i < numSelectedItemsSorted; ++i)
            {
                selectedItemsSorted[i].mCategory->GetCallback()->OnRemoveItems(this, selectedItemsSorted[i].mItems, &commandGroup);
            }

            // execute the command group
            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }

            // call the post remove items
            for (uint32 i = 0; i < numSelectedItemsSorted; ++i)
            {
                selectedItemsSorted[i].mCategory->GetCallback()->OnPostRemoveItems(this);
            }
        }

    private:
        void keyPressEvent(QKeyEvent* event) override
        {
            // delete key
            if (event->key() == Qt::Key_Delete)
            {
                RemoveSelectedItems();
                event->accept();
                return;
            }

            // base class
            QWidget::keyPressEvent(event);
        }

        void keyReleaseEvent(QKeyEvent* event) override
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
    };


    class OutlinerListWidget
        : public QListWidget
    {
    public:
        OutlinerListWidget(QWidget* parent = nullptr)
            : QListWidget(parent)
        {
        }

        void RemoveSelectedItems()
        {
            // get the current selection
            const QList<QListWidgetItem*> items = selectedItems();

            // get the number of selected items
            const int numSelectedItems = items.size();
            if (numSelectedItems == 0)
            {
                return;
            }

            // sort the items by category
            AZStd::vector<OutlinerCategoryWithItems> selectedItemsSorted;
            for (int i = 0; i < numSelectedItems; ++i)
            {
                // get the category item
                OutlinerCategoryItem* item = (OutlinerCategoryItem*)items[i]->data(Qt::UserRole).value<void*>();

                // add if category found or create a new category with ids item
                bool categoryFound = false;
                const size_t numSelectedItemsSorted = selectedItemsSorted.size();
                for (size_t j = 0; j < numSelectedItemsSorted; ++j)
                {
                    if (selectedItemsSorted[j].mCategory == item->mCategory)
                    {
                        selectedItemsSorted[j].mItems.push_back(item);
                        categoryFound = true;
                        break;
                    }
                }
                if (categoryFound == false)
                {
                    OutlinerCategoryWithItems newCategoryWithItems;
                    newCategoryWithItems.mCategory = item->mCategory;
                    newCategoryWithItems.mItems.push_back(item);
                    selectedItemsSorted.push_back(newCategoryWithItems);
                }
            }

            // create the command group
            MCore::CommandGroup commandGroup("Remove outliner items");

            // call the remove items callback for each registered category
            const size_t numSelectedItemsSorted = selectedItemsSorted.size();
            for (size_t i = 0; i < numSelectedItemsSorted; ++i)
            {
                selectedItemsSorted[i].mCategory->GetCallback()->OnRemoveItems(this, selectedItemsSorted[i].mItems, &commandGroup);
            }

            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }

            // call the post remove items
            for (uint32 i = 0; i < numSelectedItemsSorted; ++i)
            {
                selectedItemsSorted[i].mCategory->GetCallback()->OnPostRemoveItems(this);
            }
        }

    private:
        void keyPressEvent(QKeyEvent* event) override
        {
            // delete key
            if (event->key() == Qt::Key_Delete)
            {
                RemoveSelectedItems();
                event->accept();
                return;
            }

            // base class
            QWidget::keyPressEvent(event);
        }

        void keyReleaseEvent(QKeyEvent* event) override
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
    };


    OutlinerPlugin::OutlinerPlugin()
        : EMStudio::DockWidgetPlugin()
    {
    }


    OutlinerPlugin::~OutlinerPlugin()
    {
        GetOutlinerManager()->UnregisterCallback(this);
    }


    bool OutlinerPlugin::Init()
    {
        // set the default mode
        mIconViewMode = false;

        // create the category tree widget
        mCategoryTreeWidget = new QTreeWidget();
        mCategoryTreeWidget->setAlternatingRowColors(true);
        mCategoryTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mCategoryTreeWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mCategoryTreeWidget->header()->setSortIndicator(0, Qt::AscendingOrder);
        mCategoryTreeWidget->header()->setSectionsMovable(false);
        mCategoryTreeWidget->setSortingEnabled(true);
        mCategoryTreeWidget->setColumnCount(1);
        mCategoryTreeWidget->setHeaderLabel("Name");
        mCategoryTreeWidget->setHeaderHidden(true);
        mCategoryTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(mCategoryTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(OnCategoryItemSelectionChanged()));
        connect(mCategoryTreeWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(OnCategoryTreeContextMenu(const QPoint&)));

        // add the default categories
        mCategoryRootItem = new QTreeWidgetItem(QStringList("All"));
        mCategoryRootItem->setToolTip(0, "All");
        mCategoryTreeWidget->addTopLevelItem(mCategoryRootItem);
        mCategoryRootItem->setExpanded(true);

        // create the categories label
        QLabel* categoriesLabel = new QLabel("Categories:");
        categoriesLabel->setContentsMargins(1, 2, 2, 3);
        categoriesLabel->setAlignment(Qt::AlignLeft);

        // create the category layout
        QVBoxLayout* categoryLayout = new QVBoxLayout();
        categoryLayout->addWidget(categoriesLabel);
        categoryLayout->addWidget(mCategoryTreeWidget);
        categoryLayout->setContentsMargins(0, 0, 3, 0);
        categoryLayout->setSpacing(0);

        // create the category widget
        QWidget* categoryWidget = new QWidget();
        categoryWidget->setLayout(categoryLayout);

        // create the find widget
        m_searchWidget = new AzQtComponents::FilteredSearchWidget(mDock);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &OutlinerPlugin::OnTextFilterChanged);

        // create the viewer list widget
        mViewerListWidget = new OutlinerListWidget();
        mViewerListWidget->setViewMode(QListView::IconMode);
        mViewerListWidget->setDragDropMode(QAbstractItemView::NoDragDrop);
        mViewerListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mViewerListWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mViewerListWidget->setResizeMode(QListView::Adjust);
        mViewerListWidget->setIconSize(QSize(128, 128));
        mViewerListWidget->setUniformItemSizes(true);
        mViewerListWidget->setSortingEnabled(true);
        mViewerListWidget->setWrapping(true);
        mViewerListWidget->setWordWrap(true);
        mViewerListWidget->setTextElideMode(Qt::ElideLeft);
        mViewerListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(mViewerListWidget, SIGNAL(itemSelectionChanged()), this, SLOT(OnItemListSelectionChanged()));
        connect(mViewerListWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(OnCategoryItemListContextMenu(const QPoint&)));

        // set the list widget not visible because the default mode is list
        mViewerListWidget->setVisible(false);

        // create the viewer table widget
        mViewerTableWidget = new OutlinerTableWidget();
        mViewerTableWidget->setGridStyle(Qt::SolidLine);
        mViewerTableWidget->setAlternatingRowColors(true);
        mViewerTableWidget->setDragDropMode(QAbstractItemView::NoDragDrop);
        mViewerTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        mViewerTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mViewerTableWidget->setCornerButtonEnabled(false);
        mViewerTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mViewerTableWidget->setColumnCount(2);
        mViewerTableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem("Name"));
        mViewerTableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem("Category"));
        mViewerTableWidget->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mViewerTableWidget->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mViewerTableWidget->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
        mViewerTableWidget->horizontalHeader()->setStretchLastSection(true);
        mViewerTableWidget->verticalHeader()->setVisible(false);
        mViewerTableWidget->setSortingEnabled(true);
        mViewerTableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(mViewerTableWidget, SIGNAL(itemSelectionChanged()), this, SLOT(OnItemTableSelectionChanged()));
        connect(mViewerTableWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(OnCategoryItemTableContextMenu(const QPoint&)));

        // set the name column width
        mViewerTableWidget->setColumnWidth(0, 234);

        // needed to have the last column stretching correctly
        mViewerTableWidget->setColumnWidth(1, 0);

        // create the num items label
        mNumItemsLabel = new QLabel("0 items");

        // create the list mode button
        QPushButton* listModeButton = new QPushButton("L");
        connect(listModeButton, SIGNAL(released()), this, SLOT(OnListMode()));

        // create the icon mode button
        QPushButton* iconModeButton = new QPushButton("I");
        connect(iconModeButton, SIGNAL(released()), this, SLOT(OnIconMode()));

        // create the view modes layout
        QHBoxLayout* viewModesLayout = new QHBoxLayout();
        viewModesLayout->setAlignment(Qt::AlignRight);
        viewModesLayout->addWidget(listModeButton);
        viewModesLayout->addWidget(iconModeButton);
        viewModesLayout->setSpacing(2);
        viewModesLayout->setMargin(0);

        // create the viewer infos layout
        QHBoxLayout* viewerInfosLayout = new QHBoxLayout();
        viewerInfosLayout->addWidget(mNumItemsLabel, 0, Qt::AlignLeft);
        viewerInfosLayout->addLayout(viewModesLayout);
        viewerInfosLayout->setContentsMargins(4, 2, 4, 2);

        // create the viewer info widget
        QWidget* viewerInfosWidget = new QWidget();
        viewerInfosWidget->setObjectName("OutlinerViewerInfos");
        viewerInfosWidget->setStyleSheet("#OutlinerViewerInfos{ border: 1px solid rgb(40, 40, 40); border-top: 0px; background-color: rgb(50, 50, 50); }");
        viewerInfosWidget->setLayout(viewerInfosLayout);

        // create the viewer layout
        QVBoxLayout* viewerLayout = new QVBoxLayout();
        viewerLayout->addWidget(m_searchWidget);
        viewerLayout->addWidget(mViewerListWidget);
        viewerLayout->addWidget(mViewerTableWidget);
        viewerLayout->addWidget(viewerInfosWidget);
        viewerLayout->setContentsMargins(3, 0, 0, 0);
        viewerLayout->setSpacing(0);

        // create the viewer widget
        QWidget* viewerWidget = new QWidget();
        viewerWidget->setLayout(viewerLayout);

        // create the splitter
        QSplitter* splitter = new QSplitter();
        splitter->addWidget(categoryWidget);
        splitter->addWidget(viewerWidget);
        splitter->setStretchFactor(0, 0);
        splitter->setStretchFactor(1, 1);
        splitter->setChildrenCollapsible(false);
        QList<int> splitterSizes;
        splitterSizes.append(145);
        splitter->setSizes(splitterSizes);

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget(splitter);
        layout->setMargin(3);

        // create the widget
        QWidget* widget = new QWidget();
        widget->setLayout(layout);

        // set the dock content
        mDock->SetContents(widget);

        // add each category from the outliner manager
        const size_t numCategories = GetOutlinerManager()->GetNumCategories();
        for (size_t i = 0; i < numCategories; ++i)
        {
            OutlinerCategory* category = GetOutlinerManager()->GetCategory(i);
            QTreeWidgetItem* categoryItem = new QTreeWidgetItem(QStringList(category->GetName()));
            categoryItem->setData(0, Qt::UserRole, qVariantFromValue((void*)category));
            categoryItem->setToolTip(0, category->GetName());
            mCategoryRootItem->addChild(categoryItem);
        }

        // select the "all" category (the root)
        mCategoryTreeWidget->setItemSelected(mCategoryRootItem, true);

        // register the outliner manager callback
        GetOutlinerManager()->RegisterCallback(this);

        // success
        return true;
    }


    void OutlinerPlugin::OnCategoryItemSelectionChanged()
    {
        UpdateViewerAndRestoreOldSelection();
    }


    void OutlinerPlugin::OnTextFilterChanged(const QString& text)
    {
        FromQtString(text, &m_searchWidgetText);

        if (mIconViewMode)
        {
            // get the number of items in the list
            const int numListItems = mViewerListWidget->count();

            // hide items which doesn't contains the text
            for (int i = 0; i < numListItems; ++i)
            {
                QListWidgetItem* item = mViewerListWidget->item(i);
                const bool itemContainsText = item->text().contains(text, Qt::CaseInsensitive);
                item->setHidden(!itemContainsText);
            }
        }
        else
        {
            // get the number of rows in the table
            const int numTableRows = mViewerTableWidget->rowCount();

            // hide items which doesn't contains the text
            for (int i = 0; i < numTableRows; ++i)
            {
                QTableWidgetItem* item = mViewerTableWidget->item(i, 0);
                const bool itemContainsText = item->text().contains(text, Qt::CaseInsensitive);
                mViewerTableWidget->setRowHidden(i, !itemContainsText);
            }
        }
    }


    void OutlinerPlugin::OnCategoryTreeContextMenu(const QPoint& pos)
    {
        // check if the root is selected
        if (mCategoryRootItem->isSelected())
        {
            return;
        }

        // get the selected items, only one selected accepted to show the menu
        const QList<QTreeWidgetItem*> selectedItems = mCategoryTreeWidget->selectedItems();
        if (selectedItems.size() != 1)
        {
            return;
        }

        // show the menu
        QMenu menu(mDock);
        QAction* loadAction = menu.addAction("Load");
        loadAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.png"));
        if (menu.exec(mCategoryTreeWidget->mapToGlobal(pos)))
        {
            OutlinerCategory* category = (OutlinerCategory*)selectedItems[0]->data(0, Qt::UserRole).value<void*>();
            category->GetCallback()->OnLoadItem(mDock);
        }
    }


    void OutlinerPlugin::OnCategoryItemListContextMenu(const QPoint& pos)
    {
        // get the current selection
        const QList<QListWidgetItem*> selectedItems = mViewerListWidget->selectedItems();

        // get the number of selected items
        const int numSelectedItems = selectedItems.size();
        if (numSelectedItems == 0)
        {
            return;
        }

        // show the menu
        QMenu menu(mDock);
        QAction* removeAction = menu.addAction("Remove");
        removeAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Minus.png"));
        if (menu.exec(mViewerListWidget->mapToGlobal(pos)))
        {
            ((OutlinerListWidget*)mViewerListWidget)->RemoveSelectedItems();
        }
    }


    void OutlinerPlugin::OnCategoryItemTableContextMenu(const QPoint& pos)
    {
        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = mViewerTableWidget->selectedItems();

        // get the number of selected items
        const uint32 numSelectedItems = selectedItems.count();
        if (numSelectedItems == 0)
        {
            return;
        }

        // show the menu
        QMenu menu(mDock);
        QAction* removeAction = menu.addAction("Remove");
        removeAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Minus.png"));
        if (menu.exec(mViewerTableWidget->mapToGlobal(pos)))
        {
            ((OutlinerTableWidget*)mViewerTableWidget)->RemoveSelectedItems();
        }
    }


    void OutlinerPlugin::OnItemTableSelectionChanged()
    {
        // get the current selection
        const QList<QTableWidgetItem*> selectedItems = mViewerTableWidget->selectedItems();

        // get the number of selected items
        const uint32 numSelectedItems = selectedItems.count();
        if (numSelectedItems == 0)
        {
            mNumItemsLabel->setText(QString("%1 items").arg(mViewerTableWidget->rowCount()));
        }
        else
        {
            // filter the items
            AZStd::vector<uint32> rowIndices;
            rowIndices.reserve(numSelectedItems);
            for (uint32 i = 0; i < numSelectedItems; ++i)
            {
                const uint32 rowIndex = selectedItems[i]->row();
                if (AZStd::find(rowIndices.begin(), rowIndices.end(), rowIndex) == rowIndices.end())
                {
                    rowIndices.push_back(rowIndex);
                }
            }

            // set the num items label
            mNumItemsLabel->setText(QString("%1 items (%2 selected)").arg(mViewerTableWidget->rowCount()).arg(rowIndices.size()));
        }
    }


    void OutlinerPlugin::OnItemListSelectionChanged()
    {
        const QList<QListWidgetItem*> selectedItems = mViewerListWidget->selectedItems();
        if (selectedItems.isEmpty())
        {
            mNumItemsLabel->setText(QString("%1 items").arg(mViewerListWidget->count()));
        }
        else
        {
            mNumItemsLabel->setText(QString("%1 items (%2 selected)").arg(mViewerListWidget->count()).arg(selectedItems.size()));
        }
    }


    void OutlinerPlugin::OnListMode()
    {
        // do nothing if the mode is already set
        if (mIconViewMode == false)
        {
            return;
        }

        // get the current selection
        const QList<QListWidgetItem*> listSelectedItems = mViewerListWidget->selectedItems();

        // keep the selected items from the list
        AZStd::vector<OutlinerCategoryWithIds> categorySelectedItems;
        const uint32 numSelectedItems = listSelectedItems.size();
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            OutlinerCategoryItem* item = (OutlinerCategoryItem*)listSelectedItems[i]->data(Qt::UserRole).value<void*>();
            const size_t numCategorySelectedItems = categorySelectedItems.size();
            bool categoryFound = false;
            for (size_t j = 0; j < numCategorySelectedItems; ++j)
            {
                if (categorySelectedItems[j].mCategory == item->mCategory)
                {
                    categorySelectedItems[j].mIDs.push_back(item->mID);
                    categoryFound = true;
                    break;
                }
            }
            if (categoryFound == false)
            {
                OutlinerCategoryWithIds newCategoryWithIds;
                newCategoryWithIds.mCategory = item->mCategory;
                newCategoryWithIds.mIDs.push_back(item->mID);
                categorySelectedItems.push_back(newCategoryWithIds);
            }
        }

        // update the mode
        mIconViewMode = false;

        // set the good widget visible
        mViewerListWidget->setVisible(false);
        mViewerTableWidget->setVisible(true);

        // update the viewer
        UpdateViewer();

        // set the old selection on the table
        const size_t numCategorySelectedItems = categorySelectedItems.size();
        const uint32 numTableRows = mViewerTableWidget->rowCount();
        for (size_t i = 0; i < numCategorySelectedItems; ++i)
        {
            for (uint32 j = 0; j < numTableRows; ++j)
            {
                OutlinerCategoryItem* item = (OutlinerCategoryItem*)mViewerTableWidget->item(j, 0)->data(Qt::UserRole).value<void*>();
                if (item->mCategory == categorySelectedItems[i].mCategory)
                {
                    const AZStd::vector<uint32>& ids = categorySelectedItems[i].mIDs;
                    if (AZStd::find(ids.begin(), ids.end(), item->mID) != ids.end())
                    {
                        mViewerTableWidget->setItemSelected(mViewerTableWidget->item(j, 0), true);
                        mViewerTableWidget->setItemSelected(mViewerTableWidget->item(j, 1), true);
                    }
                }
            }
        }
    }


    void OutlinerPlugin::OnIconMode()
    {
        // do nothing if the mode is already set
        if (mIconViewMode)
        {
            return;
        }

        // get the current selection
        const QList<QTableWidgetItem*> tableSelectedItems = mViewerTableWidget->selectedItems();

        // get the number of selected items
        const uint32 numTableSelectedItems = tableSelectedItems.count();

        // filter the items
        AZStd::vector<uint32> rowIndices;
        rowIndices.reserve(numTableSelectedItems);
        for (uint32 i = 0; i < numTableSelectedItems; ++i)
        {
            const uint32 rowIndex = tableSelectedItems[i]->row();
            if (AZStd::find(rowIndices.begin(), rowIndices.end(), rowIndex) == rowIndices.end())
            {
                rowIndices.push_back(rowIndex);
            }
        }

        // keep the selected items from the table
        AZStd::vector<OutlinerCategoryWithIds> categorySelectedItems;
        const size_t numSelectedRows = rowIndices.size();
        for (size_t i = 0; i < numSelectedRows; ++i)
        {
            OutlinerCategoryItem* item = (OutlinerCategoryItem*)mViewerTableWidget->item(rowIndices[i], 0)->data(Qt::UserRole).value<void*>();
            const size_t numCategorySelectedItems = categorySelectedItems.size();
            bool categoryFound = false;
            for (size_t j = 0; j < numCategorySelectedItems; ++j)
            {
                if (categorySelectedItems[j].mCategory == item->mCategory)
                {
                    categorySelectedItems[j].mIDs.push_back(item->mID);
                    categoryFound = true;
                    break;
                }
            }
            if (categoryFound == false)
            {
                OutlinerCategoryWithIds newCategoryWithIds;
                newCategoryWithIds.mCategory = item->mCategory;
                newCategoryWithIds.mIDs.push_back(item->mID);
                categorySelectedItems.push_back(newCategoryWithIds);
            }
        }

        // update the mode
        mIconViewMode = true;

        // set the good widget visible
        mViewerListWidget->setVisible(true);
        mViewerTableWidget->setVisible(false);

        // update the viewer
        UpdateViewer();

        // set the old selection on the list
        const size_t numCategorySelectedItems = categorySelectedItems.size();
        const size_t numListItems = mViewerListWidget->count();
        for (uint32 i = 0; i < numCategorySelectedItems; ++i)
        {
            for (uint32 j = 0; j < numListItems; ++j)
            {
                OutlinerCategoryItem* item = (OutlinerCategoryItem*)mViewerListWidget->item(j)->data(Qt::UserRole).value<void*>();
                if (item->mCategory == categorySelectedItems[i].mCategory)
                {
                    const AZStd::vector<uint32>& ids = categorySelectedItems[i].mIDs;
                    if (AZStd::find(ids.begin(), ids.end(), item->mID) != ids.end())
                    {
                        mViewerListWidget->setItemSelected(mViewerListWidget->item(j), true);
                    }
                }
            }
        }
    }


    void OutlinerPlugin::OnAddItem(OutlinerCategoryItem* item)
    {
        // find if the category is selected in case root category is not selected
        if (mCategoryRootItem->isSelected() == false)
        {
            bool categorySelected = false;
            const QList<QTreeWidgetItem*> selectedCategories = mCategoryTreeWidget->selectedItems();
            const int numSelectedCategories = selectedCategories.size();
            for (int i = 0; i < numSelectedCategories; ++i)
            {
                if (selectedCategories[i]->text(0) == item->mCategory->GetName())
                {
                    categorySelected = selectedCategories[i]->isSelected();
                    break;
                }
            }
            if (categorySelected == false)
            {
                return;
            }
        }

        // add the item
        if (mIconViewMode)
        {
            // add the list item
            OutlinerListWidgetItem* listWidgetItem = new OutlinerListWidgetItem();
            listWidgetItem->setData(Qt::UserRole, qVariantFromValue((void*)item));
            if (item->mCategory->GetCallback())
            {
                const QString name = item->mCategory->GetCallback()->BuildNameItem(item);
                listWidgetItem->setText(name.isEmpty() ? "<no name>" : name);
                listWidgetItem->setToolTip(item->mCategory->GetCallback()->BuildToolTipItem(item));
                const QIcon icon = item->mCategory->GetCallback()->GetIcon(item);
                if (icon.availableSizes().size() == 0)
                {
                    listWidgetItem->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/OutlinerPlugin/UnknownCategory.png"));
                }
                else
                {
                    listWidgetItem->setIcon(icon);
                }
            }
            else
            {
                listWidgetItem->setText("<no name>");
                listWidgetItem->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/OutlinerPlugin/UnknownCategory.png"));
            }
            mViewerListWidget->addItem(listWidgetItem);
            const bool itemContainsText = listWidgetItem->text().contains(m_searchWidgetText.c_str(), Qt::CaseInsensitive);
            listWidgetItem->setHidden(!itemContainsText);

            // update the num items label
            const QList<QListWidgetItem*> selectedItems = mViewerListWidget->selectedItems();
            if (selectedItems.isEmpty())
            {
                mNumItemsLabel->setText(QString("%1 items").arg(mViewerListWidget->count()));
            }
            else
            {
                mNumItemsLabel->setText(QString("%1 items (%2 selected)").arg(mViewerListWidget->count()).arg(selectedItems.size()));
            }
        }
        else
        {
            // disable the sorting to avoid index issues
            mViewerTableWidget->setSortingEnabled(false);

            // add the table item
            OutlinerTableWidgetItem* nameTableWidgetItem = new OutlinerTableWidgetItem();
            OutlinerTableWidgetItem* categoryTableWidgetItem = new OutlinerTableWidgetItem(item->mCategory->GetName());
            nameTableWidgetItem->setData(Qt::UserRole, qVariantFromValue((void*)item));
            if (item->mCategory->GetCallback())
            {
                const QString name = item->mCategory->GetCallback()->BuildNameItem(item);
                nameTableWidgetItem->setText(name.isEmpty() ? "<no name>" : name);
                nameTableWidgetItem->setToolTip(item->mCategory->GetCallback()->BuildToolTipItem(item));
                categoryTableWidgetItem->setToolTip(item->mCategory->GetCallback()->BuildToolTipItem(item));
            }
            else
            {
                nameTableWidgetItem->setText("<no name");
            }
            const int newRowIndex = mViewerTableWidget->rowCount();
            mViewerTableWidget->insertRow(newRowIndex);
            mViewerTableWidget->setItem(newRowIndex, 0, nameTableWidgetItem);
            mViewerTableWidget->setItem(newRowIndex, 1, categoryTableWidgetItem);
            mViewerTableWidget->setRowHeight(newRowIndex, 21);
            const bool itemContainsText = nameTableWidgetItem->text().contains(m_searchWidgetText.c_str(), Qt::CaseInsensitive);
            mViewerTableWidget->setRowHidden(newRowIndex, !itemContainsText);

            // enable the sorting
            mViewerTableWidget->setSortingEnabled(true);

            // get the current selection
            const QList<QTableWidgetItem*> selectedItems = mViewerTableWidget->selectedItems();

            // get the number of selected items
            const uint32 numSelectedItems = selectedItems.count();
            if (numSelectedItems == 0)
            {
                mNumItemsLabel->setText(QString("%1 items").arg(mViewerTableWidget->rowCount()));
            }
            else
            {
                // filter the items
                AZStd::vector<uint32> rowIndices;
                rowIndices.reserve(numSelectedItems);
                for (uint32 i = 0; i < numSelectedItems; ++i)
                {
                    const uint32 rowIndex = selectedItems[i]->row();
                    if (AZStd::find(rowIndices.begin(), rowIndices.end(), rowIndex) == rowIndices.end())
                    {
                        rowIndices.push_back(rowIndex);
                    }
                }

                // set the num items label
                mNumItemsLabel->setText(QString("%1 items (%2 selected)").arg(mViewerTableWidget->rowCount()).arg(rowIndices.size()));
            }
        }
    }


    void OutlinerPlugin::OnRemoveItem(OutlinerCategoryItem* item)
    {
        // find if the category is selected in case root category is not selected
        if (mCategoryRootItem->isSelected() == false)
        {
            bool categorySelected = false;
            const QList<QTreeWidgetItem*> selectedCategories = mCategoryTreeWidget->selectedItems();
            const int numSelectedCategories = selectedCategories.size();
            for (int i = 0; i < numSelectedCategories; ++i)
            {
                if (selectedCategories[i]->text(0) == item->mCategory->GetName())
                {
                    categorySelected = selectedCategories[i]->isSelected();
                    break;
                }
            }
            if (categorySelected == false)
            {
                return;
            }
        }

        // remove the item
        if (mIconViewMode)
        {
            // remove the list item
            const int numItems = mViewerListWidget->count();
            for (int i = 0; i < numItems; ++i)
            {
                OutlinerCategoryItem* categoryItem = (OutlinerCategoryItem*)mViewerListWidget->item(i)->data(Qt::UserRole).value<void*>();
                if (categoryItem == item)
                {
                    delete mViewerListWidget->item(i);
                    break;
                }
            }

            // update the num items label
            const QList<QListWidgetItem*> selectedItems = mViewerListWidget->selectedItems();
            if (selectedItems.isEmpty())
            {
                mNumItemsLabel->setText(QString("%1 items").arg(mViewerListWidget->count()));
            }
            else
            {
                mNumItemsLabel->setText(QString("%1 items (%2 selected)").arg(mViewerListWidget->count()).arg(selectedItems.size()));
            }
        }
        else
        {
            // remove the table item
            const int numItems = mViewerTableWidget->rowCount();
            for (int i = 0; i < numItems; ++i)
            {
                OutlinerCategoryItem* categoryItem = (OutlinerCategoryItem*)mViewerTableWidget->item(i, 0)->data(Qt::UserRole).value<void*>();
                if (categoryItem == item)
                {
                    mViewerTableWidget->removeRow(i);
                    break;
                }
            }

            // get the current selection
            const QList<QTableWidgetItem*> selectedItems = mViewerTableWidget->selectedItems();

            // get the number of selected items
            const uint32 numSelectedItems = selectedItems.count();
            if (numSelectedItems == 0)
            {
                mNumItemsLabel->setText(QString("%1 items").arg(mViewerTableWidget->rowCount()));
            }
            else
            {
                // filter the items
                AZStd::vector<uint32> rowIndices;
                rowIndices.reserve(numSelectedItems);
                for (uint32 i = 0; i < numSelectedItems; ++i)
                {
                    const uint32 rowIndex = selectedItems[i]->row();
                    if (AZStd::find(rowIndices.begin(), rowIndices.end(), rowIndex) == rowIndices.end())
                    {
                        rowIndices.push_back(rowIndex);
                    }
                }

                // set the num items label
                mNumItemsLabel->setText(QString("%1 items (%2 selected)").arg(mViewerTableWidget->rowCount()).arg(rowIndices.size()));
            }
        }
    }


    void OutlinerPlugin::OnItemModified()
    {
        if (mIconViewMode)
        {
            const int numItems = mViewerListWidget->count();
            for (int i = 0; i < numItems; ++i)
            {
                QListWidgetItem* item = mViewerListWidget->item(i);
                OutlinerCategoryItem* categoryItem = (OutlinerCategoryItem*)item->data(Qt::UserRole).value<void*>();
                if (categoryItem->mCategory->GetCallback())
                {
                    const QString name = categoryItem->mCategory->GetCallback()->BuildNameItem(categoryItem);
                    item->setText(name.isEmpty() ? "<no name>" : name);
                    item->setToolTip(categoryItem->mCategory->GetCallback()->BuildToolTipItem(categoryItem));
                    const QIcon icon = categoryItem->mCategory->GetCallback()->GetIcon(categoryItem);
                    if (icon.availableSizes().size() == 0)
                    {
                        item->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/OutlinerPlugin/UnknownCategory.png"));
                    }
                    else
                    {
                        item->setIcon(icon);
                    }
                }
                else
                {
                    item->setText("<no name>");
                    item->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/OutlinerPlugin/UnknownCategory.png"));
                }
                const bool itemContainsText = item->text().contains(m_searchWidgetText.c_str(), Qt::CaseInsensitive);
                item->setHidden(!itemContainsText);
            }
        }
        else
        {
            mViewerTableWidget->setSortingEnabled(false);
            const int numItems = mViewerTableWidget->rowCount();
            for (int i = 0; i < numItems; ++i)
            {
                QTableWidgetItem* nameTableWidgetItem = mViewerTableWidget->item(i, 0);
                OutlinerCategoryItem* categoryItem = (OutlinerCategoryItem*)nameTableWidgetItem->data(Qt::UserRole).value<void*>();
                if (categoryItem->mCategory->GetCallback())
                {
                    const QString name = categoryItem->mCategory->GetCallback()->BuildNameItem(categoryItem);
                    nameTableWidgetItem->setText(name.isEmpty() ? "<no name>" : name);
                    nameTableWidgetItem->setToolTip(categoryItem->mCategory->GetCallback()->BuildToolTipItem(categoryItem));
                    mViewerTableWidget->item(i, 1)->setToolTip(categoryItem->mCategory->GetCallback()->BuildToolTipItem(categoryItem));
                }
                else
                {
                    nameTableWidgetItem->setText("<no name");
                    nameTableWidgetItem->setToolTip("");
                    mViewerTableWidget->item(i, 1)->setToolTip("");
                }
                const bool itemContainsText = nameTableWidgetItem->text().contains(m_searchWidgetText.c_str(), Qt::CaseInsensitive);
                mViewerTableWidget->setRowHidden(i, !itemContainsText);
            }
            mViewerTableWidget->setSortingEnabled(true);
        }
    }


    void OutlinerPlugin::OnRegisterCategory(OutlinerCategory* category)
    {
        QTreeWidgetItem* categoryItem = new QTreeWidgetItem(QStringList(category->GetName()));
        categoryItem->setData(0, Qt::UserRole, qVariantFromValue(static_cast<void*>(category)));
        categoryItem->setToolTip(0, category->GetName());
        mCategoryRootItem->addChild(categoryItem);
    }


    void OutlinerPlugin::OnUnregisterCategory(const QString& name)
    {
        const int childCount = mCategoryRootItem->childCount();
        for (int i = 0; i < childCount; ++i)
        {
            QTreeWidgetItem* child = mCategoryRootItem->child(i);
            if (child->text(0) == name)
            {
                // keep the selection state before delete to update the viewer if needed
                const bool categoryWasSelected = child->isSelected();
                delete child;

                // update the viewer and restore the old selection
                // only do that if the removed category is selected or the root category is selected
                if (mCategoryRootItem->isSelected() || categoryWasSelected)
                {
                    UpdateViewerAndRestoreOldSelection();
                }

                // stop here because only one category with this name is possible
                return;
            }
        }
    }


    void OutlinerPlugin::UpdateViewerAndRestoreOldSelection()
    {
        if (mIconViewMode)
        {
            // get the current selection
            const QList<QListWidgetItem*> listSelectedItems = mViewerListWidget->selectedItems();

            // keep the selected items from the list
            AZStd::vector<OutlinerCategoryWithIds> categorySelectedItems;
            const uint32 numSelectedItems = listSelectedItems.size();
            for (uint32 i = 0; i < numSelectedItems; ++i)
            {
                OutlinerCategoryItem* item = (OutlinerCategoryItem*)listSelectedItems[i]->data(Qt::UserRole).value<void*>();
                const size_t numCategorySelectedItems = categorySelectedItems.size();
                bool categoryFound = false;
                for (size_t j = 0; j < numCategorySelectedItems; ++j)
                {
                    if (categorySelectedItems[j].mCategory == item->mCategory)
                    {
                        categorySelectedItems[j].mIDs.push_back(item->mID);
                        categoryFound = true;
                        break;
                    }
                }
                if (categoryFound == false)
                {
                    OutlinerCategoryWithIds newCategoryWithIds;
                    newCategoryWithIds.mCategory = item->mCategory;
                    newCategoryWithIds.mIDs.push_back(item->mID);
                    categorySelectedItems.push_back(newCategoryWithIds);
                }
            }

            // update the viewer
            UpdateViewer();

            // set the old selection on the list
            const size_t numCategorySelectedItems = categorySelectedItems.size();
            const uint32 numListItems = mViewerListWidget->count();
            for (size_t i = 0; i < numCategorySelectedItems; ++i)
            {
                for (size_t j = 0; j < numListItems; ++j)
                {
                    OutlinerCategoryItem* item = (OutlinerCategoryItem*)mViewerListWidget->item(static_cast<int>(j))->data(Qt::UserRole).value<void*>();
                    if (item->mCategory == categorySelectedItems[i].mCategory)
                    {
                        const AZStd::vector<uint32>& ids = categorySelectedItems[i].mIDs;
                        if (AZStd::find(ids.begin(), ids.end(), item->mID) != ids.end())
                        {
                            mViewerListWidget->setItemSelected(mViewerListWidget->item(static_cast<int>(j)), true);
                        }
                    }
                }
            }
        }
        else
        {
            // get the current selection
            const QList<QTableWidgetItem*> tableSelectedItems = mViewerTableWidget->selectedItems();

            // get the number of selected items
            const uint32 numTableSelectedItems = tableSelectedItems.count();

            // filter the items
            AZStd::vector<uint32> rowIndices;
            rowIndices.reserve(numTableSelectedItems);
            for (uint32 i = 0; i < numTableSelectedItems; ++i)
            {
                const uint32 rowIndex = tableSelectedItems[i]->row();
                if (AZStd::find(rowIndices.begin(), rowIndices.end(), rowIndex) == rowIndices.end())
                {
                    rowIndices.push_back(rowIndex);
                }
            }

            // keep the selected items from the table
            AZStd::vector<OutlinerCategoryWithIds> categorySelectedItems;
            const size_t numSelectedRows = rowIndices.size();
            for (uint32 i = 0; i < numSelectedRows; ++i)
            {
                OutlinerCategoryItem* item = (OutlinerCategoryItem*)mViewerTableWidget->item(rowIndices[i], 0)->data(Qt::UserRole).value<void*>();
                const size_t numCategorySelectedItems = categorySelectedItems.size();
                bool categoryFound = false;
                for (size_t j = 0; j < numCategorySelectedItems; ++j)
                {
                    if (categorySelectedItems[j].mCategory == item->mCategory)
                    {
                        categorySelectedItems[j].mIDs.push_back(item->mID);
                        categoryFound = true;
                        break;
                    }
                }
                if (categoryFound == false)
                {
                    OutlinerCategoryWithIds newCategoryWithIds;
                    newCategoryWithIds.mCategory = item->mCategory;
                    newCategoryWithIds.mIDs.push_back(item->mID);
                    categorySelectedItems.push_back(newCategoryWithIds);
                }
            }

            // update the viewer
            UpdateViewer();

            // set the old selection on the list
            const size_t numCategorySelectedItems = categorySelectedItems.size();
            const uint32 numTableRows = mViewerTableWidget->rowCount();
            for (size_t i = 0; i < numCategorySelectedItems; ++i)
            {
                for (uint32 j = 0; j < numTableRows; ++j)
                {
                    OutlinerCategoryItem* item = (OutlinerCategoryItem*)mViewerTableWidget->item(j, 0)->data(Qt::UserRole).value<void*>();
                    if (item->mCategory == categorySelectedItems[i].mCategory)
                    {
                        const AZStd::vector<uint32>& ids = categorySelectedItems[i].mIDs;
                        if (AZStd::find(ids.begin(), ids.end(), item->mID) != ids.end())
                        {
                            mViewerTableWidget->setItemSelected(mViewerTableWidget->item(j, 0), true);
                            mViewerTableWidget->setItemSelected(mViewerTableWidget->item(j, 1), true);
                        }
                    }
                }
            }
        }
    }


    void OutlinerPlugin::UpdateViewer()
    {
        // clear the actual viewers
        // the two modes are cleared to avoid memory not needed on one or the other
        // the memory is not needed because only one mode is visible
        mViewerTableWidget->setRowCount(0);
        mViewerListWidget->clear();

        // populate the viewer
        if (mIconViewMode)
        {
            // we check if the all item is selected
            // on this case all categories are visible
            if (mCategoryRootItem->isSelected())
            {
                const size_t numCategories = GetOutlinerManager()->GetNumCategories();
                for (size_t i = 0; i < numCategories; ++i)
                {
                    OutlinerCategory* category = GetOutlinerManager()->GetCategory(i);
                    const size_t numItems = category->GetNumItems();
                    for (size_t j = 0; j < numItems; ++j)
                    {
                        OutlinerCategoryItem* item = category->GetItem(j);
                        OutlinerListWidgetItem* listWidgetItem = new OutlinerListWidgetItem();
                        listWidgetItem->setData(Qt::UserRole, qVariantFromValue((void*)item));
                        if (category->GetCallback())
                        {
                            const QString name = category->GetCallback()->BuildNameItem(item);
                            listWidgetItem->setText(name.isEmpty() ? "<no name>" : name);
                            listWidgetItem->setToolTip(category->GetCallback()->BuildToolTipItem(item));
                            const QIcon icon = category->GetCallback()->GetIcon(item);
                            if (icon.availableSizes().size() == 0)
                            {
                                listWidgetItem->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/OutlinerPlugin/UnknownCategory.png"));
                            }
                            else
                            {
                                listWidgetItem->setIcon(icon);
                            }
                        }
                        else
                        {
                            listWidgetItem->setText("<no name>");
                            listWidgetItem->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/OutlinerPlugin/UnknownCategory.png"));
                        }
                        mViewerListWidget->addItem(listWidgetItem);
                        const bool itemContainsText = listWidgetItem->text().contains(m_searchWidgetText.c_str(), Qt::CaseInsensitive);
                        listWidgetItem->setHidden(!itemContainsText);
                    }
                }
            }
            else
            {
                const QList<QTreeWidgetItem*> selectedItems = mCategoryTreeWidget->selectedItems();
                const uint32 numSelectedItems = selectedItems.length();
                for (uint32 i = 0; i < numSelectedItems; ++i)
                {
                    OutlinerCategory* category = (OutlinerCategory*)selectedItems[i]->data(0, Qt::UserRole).value<void*>();
                    const size_t numItems = category->GetNumItems();
                    for (size_t j = 0; j < numItems; ++j)
                    {
                        OutlinerCategoryItem* item = category->GetItem(j);
                        OutlinerListWidgetItem* listWidgetItem = new OutlinerListWidgetItem();
                        listWidgetItem->setData(Qt::UserRole, qVariantFromValue((void*)item));
                        if (category->GetCallback())
                        {
                            const QString name = category->GetCallback()->BuildNameItem(item);
                            listWidgetItem->setText(name.isEmpty() ? "<no name>" : name);
                            listWidgetItem->setToolTip(category->GetCallback()->BuildToolTipItem(item));
                            const QIcon icon = category->GetCallback()->GetIcon(item);
                            if (icon.availableSizes().size() == 0)
                            {
                                listWidgetItem->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/OutlinerPlugin/UnknownCategory.png"));
                            }
                            else
                            {
                                listWidgetItem->setIcon(icon);
                            }
                        }
                        else
                        {
                            listWidgetItem->setText("<no name>");
                            listWidgetItem->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/OutlinerPlugin/UnknownCategory.png"));
                        }
                        mViewerListWidget->addItem(listWidgetItem);
                        const bool itemContainsText = listWidgetItem->text().contains(m_searchWidgetText.c_str(), Qt::CaseInsensitive);
                        listWidgetItem->setHidden(!itemContainsText);
                    }
                }
            }

            // update the num items label
            mNumItemsLabel->setText(QString("%1 items").arg(mViewerListWidget->count()));
        }
        else
        {
            // disable the sorting to avoid index issues
            mViewerTableWidget->setSortingEnabled(false);

            // we check if the all item is selected
            // on this case all categories are visible
            if (mCategoryRootItem->isSelected())
            {
                const size_t numCategories = GetOutlinerManager()->GetNumCategories();
                for (size_t i = 0; i < numCategories; ++i)
                {
                    OutlinerCategory* category = GetOutlinerManager()->GetCategory(i);
                    const size_t numItems = category->GetNumItems();
                    for (size_t j = 0; j < numItems; ++j)
                    {
                        OutlinerCategoryItem* item = category->GetItem(j);
                        OutlinerTableWidgetItem* nameTableWidgetItem = new OutlinerTableWidgetItem();
                        OutlinerTableWidgetItem* categoryTableWidgetItem = new OutlinerTableWidgetItem(category->GetName());
                        nameTableWidgetItem->setData(Qt::UserRole, qVariantFromValue((void*)item));
                        if (category->GetCallback())
                        {
                            const QString name = category->GetCallback()->BuildNameItem(item);
                            nameTableWidgetItem->setText(name.isEmpty() ? "<no name>" : name);
                            nameTableWidgetItem->setToolTip(category->GetCallback()->BuildToolTipItem(item));
                            categoryTableWidgetItem->setToolTip(category->GetCallback()->BuildToolTipItem(item));
                        }
                        else
                        {
                            nameTableWidgetItem->setText("<no name");
                        }
                        const int newRowIndex = mViewerTableWidget->rowCount();
                        mViewerTableWidget->insertRow(newRowIndex);
                        mViewerTableWidget->setItem(newRowIndex, 0, nameTableWidgetItem);
                        mViewerTableWidget->setItem(newRowIndex, 1, categoryTableWidgetItem);
                        mViewerTableWidget->setRowHeight(newRowIndex, 21);
                        const bool itemContainsText = nameTableWidgetItem->text().contains(m_searchWidgetText.c_str(), Qt::CaseInsensitive);
                        mViewerTableWidget->setRowHidden(newRowIndex, !itemContainsText);
                    }
                }
            }
            else
            {
                const QList<QTreeWidgetItem*> selectedItems = mCategoryTreeWidget->selectedItems();
                const uint32 numSelectedItems = selectedItems.length();
                for (uint32 i = 0; i < numSelectedItems; ++i)
                {
                    OutlinerCategory* category = (OutlinerCategory*)selectedItems[i]->data(0, Qt::UserRole).value<void*>();
                    const size_t numItems = category->GetNumItems();
                    for (size_t j = 0; j < numItems; ++j)
                    {
                        OutlinerCategoryItem* item = category->GetItem(j);
                        OutlinerTableWidgetItem* nameTableWidgetItem = new OutlinerTableWidgetItem();
                        OutlinerTableWidgetItem* categoryTableWidgetItem = new OutlinerTableWidgetItem(category->GetName());
                        nameTableWidgetItem->setData(Qt::UserRole, qVariantFromValue((void*)item));
                        if (category->GetCallback())
                        {
                            const QString name = category->GetCallback()->BuildNameItem(item);
                            nameTableWidgetItem->setText(name.isEmpty() ? "<no name>" : name);
                            nameTableWidgetItem->setToolTip(category->GetCallback()->BuildToolTipItem(item));
                            categoryTableWidgetItem->setToolTip(category->GetCallback()->BuildToolTipItem(item));
                        }
                        else
                        {
                            nameTableWidgetItem->setText("<no name");
                        }
                        const int newRowIndex = mViewerTableWidget->rowCount();
                        mViewerTableWidget->insertRow(newRowIndex);
                        mViewerTableWidget->setItem(newRowIndex, 0, nameTableWidgetItem);
                        mViewerTableWidget->setItem(newRowIndex, 1, categoryTableWidgetItem);
                        mViewerTableWidget->setRowHeight(newRowIndex, 21);
                        const bool itemContainsText = nameTableWidgetItem->text().contains(m_searchWidgetText.c_str(), Qt::CaseInsensitive);
                        mViewerTableWidget->setRowHidden(newRowIndex, !itemContainsText);
                    }
                }
            }

            // enable the sorting
            mViewerTableWidget->setSortingEnabled(true);

            // update the num items label
            mNumItemsLabel->setText(QString("%1 items").arg(mViewerTableWidget->rowCount()));
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/Outliner/OutlinerPlugin.moc>