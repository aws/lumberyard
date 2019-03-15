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

#include "MotionEventPresetsWidget.h"
#include "MotionEventsPlugin.h"
#include "MotionEventPresetCreateDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QIcon>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenu>
#include <QSettings>
#include <QShortcut>
#include <QKeySequence>
#include <QDragEnterEvent>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"


namespace EMStudio
{
    MotionEventPresetsWidget::MotionEventPresetsWidget(QWidget* parent, MotionEventsPlugin* plugin)
        : QWidget(parent)
        , mTableWidget(nullptr)
        , mAddButton(nullptr)
        , mRemoveButton(nullptr)
        , mClearButton(nullptr)
        , mPlugin(plugin)
    {
        Init();
    }


    void MotionEventPresetsWidget::Init()
    {
        // create the layouts
        QVBoxLayout* layout = new QVBoxLayout();
        QHBoxLayout* ioButtonsLayout = new QHBoxLayout();
        layout->setMargin(0);
        layout->setSpacing(2);

        // create the table widget
        mTableWidget = new DragTableWidget(0, 2, nullptr);
        mTableWidget->setCornerButtonEnabled(false);
        mTableWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        mTableWidget->setContextMenuPolicy(Qt::DefaultContextMenu);

        // set the table to row single selection
        mTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        mTableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

        QHeaderView* horizontalHeader = mTableWidget->horizontalHeader();
        horizontalHeader->setStretchLastSection(true);

        // create buttons
        mAddButton = new QPushButton();
        mRemoveButton = new QPushButton();
        mClearButton = new QPushButton();
        mLoadButton = new QPushButton();
        mSaveButton = new QPushButton();
        mSaveAsButton = new QPushButton();

        EMStudioManager::MakeTransparentButton(mAddButton,     "/Images/Icons/Plus.png",       "Add new motion event preset");
        EMStudioManager::MakeTransparentButton(mRemoveButton,  "/Images/Icons/Minus.png",      "Remove selected motion event presets");
        EMStudioManager::MakeTransparentButton(mClearButton,   "/Images/Icons/Clear.png",      "Remove all motion event presets");
        EMStudioManager::MakeTransparentButton(mLoadButton,    "/Images/Icons/Open.png",       "Load motion event preset config file");
        EMStudioManager::MakeTransparentButton(mSaveButton,    "/Images/Menu/FileSave.png",    "Save motion event preset config file");
        EMStudioManager::MakeTransparentButton(mSaveAsButton,  "/Images/Menu/FileSaveAs.png",  "Save a copy of motion event preset config file");

        // create the buttons layout
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setAlignment(Qt::AlignLeft);
        buttonLayout->addWidget(mAddButton);
        buttonLayout->addWidget(mRemoveButton);
        buttonLayout->addWidget(mClearButton);
        buttonLayout->addWidget(mLoadButton);
        buttonLayout->addWidget(mSaveButton);
        buttonLayout->addWidget(mSaveAsButton);

        layout->addLayout(buttonLayout);
        layout->addWidget(mTableWidget);
        layout->addLayout(ioButtonsLayout);

        // set the main layout
        setLayout(layout);

        // connect the signals and the slots
        connect(mAddButton, &QPushButton::clicked, this, &MotionEventPresetsWidget::AddMotionEventPreset);
        connect(mRemoveButton, &QPushButton::clicked, this, &MotionEventPresetsWidget::RemoveMotionEventPresets);
        connect(mClearButton, &QPushButton::clicked, this, &MotionEventPresetsWidget::ClearMotionEventPresetsButton);
        connect(mLoadButton, &QPushButton::clicked, [this]() { this->LoadPresets(); /* use lambda so that we get the default value for the showDialog parameter */ });
        connect(mSaveButton, &QPushButton::clicked, [this]() { this->SavePresets(); });
        connect(mSaveAsButton, &QPushButton::clicked, this, &MotionEventPresetsWidget::SaveWithDialog);
        connect(mTableWidget, &MotionEventPresetsWidget::DragTableWidget::itemSelectionChanged, this, &MotionEventPresetsWidget::SelectionChanged);
        connect(mTableWidget, &QTableWidget::cellDoubleClicked, this, [this](int row, int column)
        {
            AZ_UNUSED(column);
            MotionEventPreset* preset = GetEventPresetManager()->GetPreset(row);
            MotionEventPresetCreateDialog createDialog(*preset, this);
            if (createDialog.exec() == QDialog::Accepted)
            {
                *preset = createDialog.GetPreset();

                GetEventPresetManager()->SetDirtyFlag(true);
                ReInit();
                mPlugin->FireColorChangedSignal();
            }
        });


        // initialize everything
        ReInit();
        UpdateInterface();
        mPlugin->ReInit();
    }


    void MotionEventPresetsWidget::ReInit()
    {
        // clear the table widget
        mTableWidget->clear();
        mTableWidget->setColumnCount(2);

        const size_t numEventPresets = GetEventPresetManager()->GetNumPresets();
        mTableWidget->setRowCount(static_cast<int>(numEventPresets));

        // set header items for the table
        QTableWidgetItem* colorHeaderItem = new QTableWidgetItem("Color");
        QTableWidgetItem* presetNameHeaderItem = new QTableWidgetItem("Name");
        colorHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        presetNameHeaderItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);

        mTableWidget->setHorizontalHeaderItem(0, colorHeaderItem);
        mTableWidget->setHorizontalHeaderItem(1, presetNameHeaderItem);

        mTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        mTableWidget->setColumnWidth(0, 39);

        for (uint32 i = 0; i < numEventPresets; ++i)
        {
            MotionEventPreset* motionEventPreset = GetEventPresetManager()->GetPreset(i);
            if (motionEventPreset == nullptr)
            {
                continue;
            }

            // create table items
            QPixmap colorPixmap(16, 16);
            const QColor eventColor(motionEventPreset->GetEventColor());
            colorPixmap.fill(eventColor);
            QTableWidgetItem* tableItemColor = new QTableWidgetItem(QIcon(colorPixmap), "");
            QTableWidgetItem* tableItemPresetName = new QTableWidgetItem(motionEventPreset->GetName().c_str());

            AZStd::string whatsThisString = AZStd::string::format("%i", i);
            tableItemColor->setWhatsThis(whatsThisString.c_str());
            tableItemPresetName->setWhatsThis(whatsThisString.c_str());

            // set backgroundcolor of the row
            QColor backgroundColor(eventColor);
            backgroundColor.setAlpha(50);
            tableItemColor->setBackgroundColor(backgroundColor);
            tableItemPresetName->setBackgroundColor(backgroundColor);

            mTableWidget->setItem(i, 0, tableItemColor);
            mTableWidget->setItem(i, 1, tableItemPresetName);

            // Editing will be handled in the double click signal handler
            tableItemPresetName->setFlags(tableItemPresetName->flags() ^ Qt::ItemIsEditable);
        }

        // set the vertical header not visible
        QHeaderView* verticalHeader = mTableWidget->verticalHeader();
        verticalHeader->setVisible(false);

        mTableWidget->resizeColumnToContents(1);
        mTableWidget->resizeColumnToContents(2);

        if (mTableWidget->columnWidth(1) < 36)
        {
            mTableWidget->setColumnWidth(1, 36);
        }

        if (mTableWidget->columnWidth(2) < 70)
        {
            mTableWidget->setColumnWidth(2, 70);
        }

        mTableWidget->horizontalHeader()->setStretchLastSection(true);
        
        // update the interface
        UpdateInterface();
    }


    void MotionEventPresetsWidget::UpdateInterface()
    {
        mClearButton->setEnabled(false);
        mRemoveButton->setEnabled(false);
        mSaveButton->setEnabled(false);

        if (!GetEventPresetManager()->IsEmpty())
        {
            mClearButton->setEnabled(true);
        }

        QList<QTableWidgetItem*> selectedItems = mTableWidget->selectedItems();
        if (selectedItems.count() > 0)
        {
            mRemoveButton->setEnabled(true);
        }

        if (GetEventPresetManager()->GetIsDirty() && !GetEventPresetManager()->GetFileNameString().empty())
        {
            mSaveButton->setEnabled(true);
        }
    }


    void MotionEventPresetsWidget::AddMotionEventPreset()
    {
        MotionEventPresetCreateDialog createDialog(MotionEventPreset(), this);
        if (createDialog.exec() == QDialog::Rejected)
        {
            return;
        }

        // add the preset
        MotionEventPreset* motionEventPreset = aznew MotionEventPreset(AZStd::move(createDialog.GetPreset()));
        GetEventPresetManager()->AddPreset(motionEventPreset);

        // reinit the dialog
        ReInit();
    }


    void MotionEventPresetsWidget::RemoveMotionEventPreset(uint32 index)
    {
        GetEventPresetManager()->RemovePreset(index);

        // reinit the dialog
        ReInit();
    }


    void MotionEventPresetsWidget::RemoveMotionEventPresets()
    {
        // store which indices to delete
        MCore::Array<int> deleteRows;

        // get selected rows
        const uint32 numRows = mTableWidget->rowCount();
        for (uint32 i = 0; i < numRows; ++i)
        {
            QTableWidgetItem* item = mTableWidget->item(i, 0);
            if (item->isSelected())
            {
                deleteRows.Add(item->whatsThis().toInt());
            }
        }

        const uint32 numDeletions = deleteRows.GetLength();
        if (numDeletions == 0)
        {
            return;
        }

        deleteRows.Sort();

        // remove all selected rows
        for (uint32 i = numDeletions; i > 0; --i)
        {
            RemoveMotionEventPreset(deleteRows[i - 1]);
        }

        // reinit the table
        ReInit();

        // selected the next row
        if (deleteRows[0] > (mTableWidget->rowCount() - 1))
        {
            mTableWidget->selectRow(deleteRows[0] - 1);
        }
        else
        {
            mTableWidget->selectRow(deleteRows[0]);
        }
    }


    void MotionEventPresetsWidget::ClearMotionEventPresetsButton()
    {
        // show message box
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Delete All Motion Event Presets?");
        msgBox.setText("Are you sure to really delete all motion event presets?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int result = msgBox.exec();

        // only delete if result was yes
        if (result == QMessageBox::Yes)
        {
            ClearMotionEventPresets();
        }
    }


    void MotionEventPresetsWidget::ClearMotionEventPresets()
    {
        mTableWidget->selectAll();
        RemoveMotionEventPresets();
        UpdateInterface();
    }


    void MotionEventPresetsWidget::LoadPresets(bool showDialog)
    {
        if (showDialog)
        {
            GetManager()->SetAvoidRendering(true);
            const QString filename = QFileDialog::getOpenFileName(this, "Open", GetEventPresetManager()->GetFileName(), "EMStudio Config Files (*.cfg);;All Files (*)");
            GetManager()->SetAvoidRendering(false);

            if (filename.isEmpty() == false)
            {
                GetEventPresetManager()->Load(FromQtString(filename).c_str());
            }
        }
        else
        {
            GetEventPresetManager()->Load();
        }

        ReInit();
        UpdateInterface();
        mPlugin->ReInit();
    }


    void MotionEventPresetsWidget::SavePresets(bool showSaveDialog)
    {
        if (showSaveDialog)
        {
            GetManager()->SetAvoidRendering(true);

            AZStd::string defaultFolder;
            AzFramework::StringFunc::Path::GetFullPath(GetEventPresetManager()->GetFileName(), defaultFolder);

            const QString filename = QFileDialog::getSaveFileName(this, "Save", defaultFolder.c_str(), "EMotionFX Event Preset Files (*.cfg);;All Files (*)");
            GetManager()->SetAvoidRendering(false);

            if (!filename.isEmpty())
            {
                GetEventPresetManager()->SaveAs(filename.toUtf8().data());
            }
        }
        else
        {
            GetEventPresetManager()->Save();
        }
        UpdateInterface();
    }


    void MotionEventPresetsWidget::SaveWithDialog()
    {
        SavePresets(true);
    }


    void MotionEventPresetsWidget::contextMenuEvent(QContextMenuEvent* event)
    {
        MCORE_UNUSED(event);
    }


    void MotionEventPresetsWidget::keyPressEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            RemoveMotionEventPresets();
            event->accept();
            return;
        }

        // base class
        QWidget::keyPressEvent(event);
    }


    void MotionEventPresetsWidget::keyReleaseEvent(QKeyEvent* event)
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
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/MotionEventPresetsWidget.moc>
