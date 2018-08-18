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

#pragma once

#include <MCore/Source/MemoryCategoriesCore.h>
#include <AzCore/std/containers/vector.h>
#include "../StandardPluginsConfig.h"
#include "MotionWindowPlugin.h"
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/WaveletSkeletalMotion.h>
#include <EMotionFX/Source/PlayBackInfo.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <QWidget>
#include <QTableWidget>
#include <QDialog>


// forward declarations
QT_FORWARD_DECLARE_CLASS(QContextMenuEvent)
QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

namespace EMStudio
{
    // forward declarations
    class MotionWindowPlugin;


    class MotionListRemoveMotionsFailedWindow
        : public QDialog
    {
        MCORE_MEMORYOBJECTCATEGORY(MotionListRemoveMotionsFailedWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionListRemoveMotionsFailedWindow(QWidget* parent, const AZStd::vector<EMotionFX::Motion*>& motions);
    };


    class MotionTableWidget
        : public QTableWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionTableWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionTableWidget(MotionWindowPlugin* parentPlugin, QWidget* parent);
        virtual ~MotionTableWidget();

    protected:
        // used for drag and drop support for the blend tree
        QMimeData* mimeData(const QList<QTableWidgetItem*> items) const override;
        QStringList mimeTypes() const override;
        Qt::DropActions supportedDropActions() const override;

        MotionWindowPlugin* mPlugin;
    };


    class MotionListWindow
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(MotionListWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionListWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin);
        ~MotionListWindow();

        void Init();
        void ReInit();
        void UpdateInterface();

        QTableWidget* GetMotionTable() { return mMotionTable; }

        bool AddMotionByID(uint32 motionID);
        bool RemoveMotionByID(uint32 motionID);
        uint32 GetMotionID(uint32 rowIndex);
        uint32 FindRowByMotionID(uint32 motionID);
        bool CheckIfIsMotionVisible(MotionWindowPlugin::MotionTableEntry* entry);
    signals:
        void MotionSelectionChanged();

    private slots:
        void cellDoubleClicked(int row, int column);
        void itemSelectionChanged();
        void OnAddMotionsButtonPressed();
        void OnRemoveMotionsButtonPressed();
        void OnClearMotionsButtonPressed();
        void OnStopSelectedMotionsButton();
        void OnStopAllMotionsButton();
        void OnSave();
        void OnTextFilterChanged(const QString& text);
        void OnAddMotionsInSelectedMotionSets();

    private:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void contextMenuEvent(QContextMenuEvent* event) override;
        void UpdateSelection(const CommandSystem::SelectionList& selectionList);

    private:
        AZStd::vector<uint32>                                   mSelectedMotionIDs;
        AZStd::vector<MotionWindowPlugin::MotionTableEntry*>    mShownMotionEntries;
        QVBoxLayout*                                            mVLayout;
        MotionTableWidget*                                      mMotionTable;
        QPushButton*                                            mAddMotionsButton;
        QPushButton*                                            mRemoveMotionsButton;
        QPushButton*                                            mClearMotionsButton;
        QPushButton*                                            mSaveButton;
        MotionWindowPlugin*                                     mMotionWindowPlugin;
        AzQtComponents::FilteredSearchWidget*                   m_searchWidget;
        AZStd::string                                           m_searchWidgetText;
    };

} // namespace EMStudio