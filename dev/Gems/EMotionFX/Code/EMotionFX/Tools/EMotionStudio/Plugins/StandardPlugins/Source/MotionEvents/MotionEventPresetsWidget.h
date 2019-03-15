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

#include "../StandardPluginsConfig.h"
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include "../../../../EMStudioSDK/Source/MotionEventPresetManager.h"
#include <MysticQt/Source/DialogStack.h>
#include <MysticQt/Source/ButtonGroup.h>
#include "../TimeView/TimeViewPlugin.h"
#include "../MotionWindow/MotionWindowPlugin.h"
#include "../MotionWindow/MotionListWindow.h"
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>

QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QShortcut)


namespace EMStudio
{
    // forward declaration
    class MotionEventsPlugin;

    class MotionEventPresetsWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionEventPresetsWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionEventPresetsWidget(QWidget* parent, MotionEventsPlugin* plugin);

        // overloaded main init function
        void Init();
        void UpdateInterface();

        QTableWidget* GetMotionEventPresetsTable()                                                          { return mTableWidget; }

    public slots:
        void ReInit();
        void AddMotionEventPreset();
        void RemoveMotionEventPresets();
        void RemoveMotionEventPreset(uint32 index);
        void ClearMotionEventPresetsButton();
        void SavePresets(bool showSaveDialog = false);
        void SaveWithDialog();
        void LoadPresets(bool showDialog = true);
        void SelectionChanged()                                                                             { UpdateInterface(); }

    private:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;

        void ClearMotionEventPresets();
        void contextMenuEvent(QContextMenuEvent* event) override;

        // widgets
        class DragTableWidget
            : public QTableWidget
        {
        public:
            DragTableWidget(int rows = 0, int columns = 0, QWidget* parent = nullptr)
                : QTableWidget(rows, columns, parent)
            {
                this->setDragEnabled(true);
            }

        protected:
            void dragEnterEvent(QDragEnterEvent* event) override
            {
                event->acceptProposedAction();
            }
            void dragLeaveEvent(QDragLeaveEvent* event) override
            {
                event->accept();
            }

            void dragMoveEvent(QDragMoveEvent* event) override
            {
                event->accept();
            }
        };

        DragTableWidget*    mTableWidget;
        QPushButton*        mAddButton;
        QPushButton*        mRemoveButton;
        QPushButton*        mClearButton;
        QPushButton*        mSaveButton;
        QPushButton*        mSaveAsButton;
        QPushButton*        mLoadButton;

        MotionEventsPlugin* mPlugin;
    };
} // namespace EMStudio
