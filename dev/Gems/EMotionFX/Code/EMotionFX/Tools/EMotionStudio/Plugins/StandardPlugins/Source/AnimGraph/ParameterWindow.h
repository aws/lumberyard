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

#include <MCore/Source/StandardHeaders.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/Array.h>
#include <MCore/Source/UnicodeString.h>
#include <EMotionFX/Source/AnimGraph.h>

#include "../StandardPluginsConfig.h"
#include "AttributesWindow.h"
#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QTreeWidget>

#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <MysticQt/Source/SearchButton.h>


namespace EMStudio
{
    class AnimGraphPlugin;

    class ParameterCreateRenameWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ParameterCreateRenameWindow, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH)

    public:
        ParameterCreateRenameWindow(const char* windowTitle, const char* topText, const char* defaultName, const char* oldName, const AZStd::vector<AZStd::string>& invalidNames, QWidget* parent);
        virtual ~ParameterCreateRenameWindow();

        AZStd::string GetName() const
        {
            return mLineEdit->text().toUtf8().data();
        }

    private slots:
        void NameEditChanged(const QString& text);

    private:
        AZStd::string                   mOldName;
        AZStd::vector<AZStd::string>    mInvalidNames;
        QPushButton*                    mOKButton;
        QPushButton*                    mCancelButton;
        QLineEdit*                      mLineEdit;
    };

    class ParameterWindow
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ParameterWindow, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        ParameterWindow(AnimGraphPlugin* plugin);
        ~ParameterWindow();

        void Init();

        // retrieve current selection
        const char* GetSingleSelectedParameterName() const
        {
            if (mSelectedParameterNames.size() != 1 || !mSelectedParameterGroupNames.empty())
            {
                return "";
            }
            return mSelectedParameterNames[0].c_str();
        }
        uint32 GetSingleSelectedParameterIndex() const;
        const char* GetSingleSelectedParameterGroupName() const;
        bool GetIsDefaultParameterGroupSingleSelected();
        bool GetIsParameterSelected(const char* parameterName)
        {
            if (AZStd::find(mSelectedParameterNames.begin(), mSelectedParameterNames.end(), parameterName) == mSelectedParameterNames.end())
            {
                return false;
            }
            return true;
        }
        bool GetIsParameterGroupSelected(const char* groupName)
        {
            if (AZStd::find(mSelectedParameterGroupNames.begin(), mSelectedParameterGroupNames.end(), groupName) == mSelectedParameterGroupNames.end())
            {
                return false;
            }
            return true;
        }

        // select manually
        void SingleSelectParameter(const char* parameterName, bool ensureVisibility = false, bool updateInterface = false);
        void SingleSelectParameterGroup(const char* groupName, bool ensureVisibility = false, bool updateInterface = false);

        MysticQt::AttributeWidget* FindAttributeWidget(MCore::AttributeSettings* attributeSettings);

        void UpdateParameterValues();

        void ResetNameColumnWidth();
        void ResizeNameColumnToContents();

    private slots:
        void UpdateInterface();

        void OnAddParameter();
        void OnEditButton();
        void OnRemoveButton();
        void OnClearButton();

        void OnGroupCollapsed(QTreeWidgetItem* item);
        void OnGroupExpanded(QTreeWidgetItem* item);

        void OnRenameGroup();
        void OnAddGroupButton();
        void OnParameterGroupSelected();

        void OnMoveParameterUp();
        void OnMoveParameterDown();
        void FilterStringChanged(const QString& text);
        void OnSelectionChanged();
        void OnMakeDefaultValue();

        void OnGamepadControlToggle();
        void OnRecorderStateChanged();

    private:
        void contextMenuEvent(QContextMenuEvent* event);
        void CanMove(bool* outMoveUpPossible, bool* outMoveDownPossible);
        uint32 GetNeighborParameterIndex(EMotionFX::AnimGraph* animGraph, uint32 parameterIndex, bool upper);

        void UpdateSelectionArrays();

        void keyPressEvent(QKeyEvent* event);
        void keyReleaseEvent(QKeyEvent* event);

        void AddParameterToInterface(EMotionFX::AnimGraph* animGraph, uint32 parameterIndex, QTreeWidgetItem* parameterGroupItem);

        /**
         * Check if the gamepad control mode is enabled for the given parameter and if its actually being controlled or not.
         */
        void GetGamepadState(EMotionFX::AnimGraph* animGraph, MCore::AttributeSettings* attributeSettings, bool* outIsActuallyControlled, bool* outIsEnabled);
        void SetGamepadState(EMotionFX::AnimGraph* animGraph, MCore::AttributeSettings* attributeSettings, bool isEnabled);
        void SetGamepadButtonTooltip(QPushButton* button);

        // command callbacks
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateBlendParameterCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveBlendParameterCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustBlendParameterCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAdjustParameterGroupCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAddParameterGroupCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphRemoveParameterGroupCallback);
        CommandCreateBlendParameterCallback*            mCreateCallback;
        CommandRemoveBlendParameterCallback*            mRemoveCallback;
        CommandAdjustBlendParameterCallback*            mAdjustCallback;
        CommandAnimGraphAddParameterGroupCallback*     mAddGroupCallback;
        CommandAnimGraphRemoveParameterGroupCallback*  mRemoveGroupCallback;
        CommandAnimGraphAdjustParameterGroupCallback*  mAdjustGroupCallback;


        struct WidgetLookup
        {
            MCORE_MEMORYOBJECTCATEGORY(ParameterWindow::WidgetLookup, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
            QObject*    mWidget;
            uint32      mGroupIndex;

            WidgetLookup(QObject* widget, uint32 index)
            {
                mWidget     = widget;
                mGroupIndex = index;
            }
        };

        EMotionFX::AnimGraph*           mAnimGraph;

        // toolbar buttons
        QPushButton*                    mAddButton;
        QPushButton*                    mRemoveButton;
        QPushButton*                    mClearButton;

        QPushButton*                    mEditButton;
        QPushButton*                    mMoveUpButton;
        QPushButton*                    mMoveDownButton;

        AZStd::vector<AZStd::string>    mSelectedParameterNames;
        AZStd::vector<AZStd::string>    mSelectedParameterGroupNames;
        bool                            mEnsureVisibility;
        bool                            mLockSelection;

        AZStd::string                   mFilterString;
        AnimGraphPlugin*                mPlugin;
        QTreeWidget*                    mTreeWidget;
        MysticQt::SearchButton*         mSearchButton;
        QVBoxLayout*                    mVerticalLayout;
        QScrollArea*                    mScrollArea;
        AZStd::string                   mNameString;
        AZStd::vector<MysticQt::AttributeWidget*> mAttributeWidgets;
        AZStd::vector<WidgetLookup>     mWidgetTable;
    };
} // namespace EMStudio