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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/Array.h>
#include <MCore/Source/Endian.h>
#include "../StandardPluginsConfig.h"
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <MysticQt/Source/ButtonGroup.h>
#include <MysticQt/Source/SearchButton.h>
#include <MysticQt/Source/ComboBox.h>
#include <QWidget>
#include <QDialog>
#include <QGridLayout>
#include <QTableWidget>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>

QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QTableWidget)
QT_FORWARD_DECLARE_CLASS(QTextEdit)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLineEdit)


namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;
    class BlendResourceWidget;


    class AnimGraphPropertiesDialog
        : public QDialog
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(AnimGraphPropertiesDialog, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        AnimGraphPropertiesDialog(QWidget* parent, BlendResourceWidget* blendResourceWidget);
        void accept() override;

    private slots:
        void NameTextEdited(const QString& text);

    private:
        QPushButton*            mOKButton;
        QTextEdit*              mDescriptionEdit;
        QLineEdit*              mNameEdit;
        QCheckBox*              mRetargeting;
        BlendResourceWidget*    mBlendResourceWidget;
    };


    class BlendResourceWidget
        : public QWidget
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(BlendResourceWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        BlendResourceWidget(AnimGraphPlugin* plugin, QWidget* parent = nullptr);
        ~BlendResourceWidget();

        void UpdateTable();
        void UpdateMotionSetComboBox();

        EMotionFX::AnimGraph* GetSelectedAnimGraph();
        EMotionFX::MotionSet* GetSelectedMotionSet();

        //void SelectAnimGraph(EMotionFX::AnimGraph* animGraph);
        void ActivateAnimGraphToSelection(EMotionFX::AnimGraph* animGraph, EMotionFX::MotionSet* motionSet = nullptr);

        void ChangeMotionSet(EMotionFX::MotionSet* motionSet);

    public slots:
        void OnCreateAnimGraph();
        void OnCloneAnimGraphs();
        void OnRemoveAnimGraphs();
        void OnClearAnimGraphs();
        void OnItemSelectionChanged();
        void OnCellDoubleClicked(int row, int column);
        void OnProperties();
        void SearchStringChanged(const QString& text);
        void OnMotionSetChanged(int index);
        void OnActivateAnimGraph();
        void OnFileSave();
        void OnFileSaveAs();

    private:
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;

        void GetSelectedRowIndices(const QList<QTableWidgetItem*>& selectedItems, AZStd::vector<uint32>& outRowIndices) const;

        MCORE_DEFINECOMMANDCALLBACK(CommandLoadAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSaveAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandCloneAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSaveMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandLoadMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandActivateAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandScaleAnimGraphDataCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAddConditionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphRemoveConditionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphCreateConnectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphRemoveConnectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAdjustConnectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphCreateNodeCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAdjustNodeCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphRemoveNodeCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphSetEntryStateCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAdjustNodeGroupCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAddNodeGroupCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphRemoveNodeGroupCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphCreateParameterCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphRemoveParameterCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAdjustParameterCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphSwapParametersCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAdjustParameterGroupCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAddParameterGroupCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphRemoveParameterGroupCallback);

        CommandLoadAnimGraphCallback*                  mLoadAnimGraphCallback;
        CommandSaveAnimGraphCallback*                  mSaveAnimGraphCallback;
        CommandCreateAnimGraphCallback*                mCreateAnimGraphCallback;
        CommandRemoveAnimGraphCallback*                mRemoveAnimGraphCallback;
        CommandCloneAnimGraphCallback*                 mCloneAnimGraphCallback;
        CommandAdjustAnimGraphCallback*                mAdjustAnimGraphCallback;
        CommandSelectCallback*                          mSelectCallback;
        CommandUnselectCallback*                        mUnselectCallback;
        CommandClearSelectionCallback*                  mClearSelectionCallback;
        CommandCreateMotionSetCallback*                 mCreateMotionSetCallback;
        CommandRemoveMotionSetCallback*                 mRemoveMotionSetCallback;
        CommandAdjustMotionSetCallback*                 mAdjustMotionSetCallback;
        CommandSaveMotionSetCallback*                   mSaveMotionSetCallback;
        CommandLoadMotionSetCallback*                   mLoadMotionSetCallback;
        CommandActivateAnimGraphCallback*              mActivateAnimGraphCallback;
        CommandScaleAnimGraphDataCallback*             mScaleAnimGraphDataCallback;
        CommandAnimGraphAddConditionCallback*          mAnimGraphAddConditionCallback;
        CommandAnimGraphRemoveConditionCallback*       mAnimGraphRemoveConditionCallback;
        CommandAnimGraphCreateConnectionCallback*      mAnimGraphCreateConnectionCallback;
        CommandAnimGraphRemoveConnectionCallback*      mAnimGraphRemoveConnectionCallback;
        CommandAnimGraphAdjustConnectionCallback*      mAnimGraphAdjustConnectionCallback;
        CommandAnimGraphCreateNodeCallback*            mAnimGraphCreateNodeCallback;
        CommandAnimGraphAdjustNodeCallback*            mAnimGraphAdjustNodeCallback;
        CommandAnimGraphRemoveNodeCallback*            mAnimGraphRemoveNodeCallback;
        CommandAnimGraphSetEntryStateCallback*         mAnimGraphSetEntryStateCallback;
        CommandAnimGraphAdjustNodeGroupCallback*       mAnimGraphAdjustNodeGroupCallback;
        CommandAnimGraphAddNodeGroupCallback*          mAnimGraphAddNodeGroupCallback;
        CommandAnimGraphRemoveNodeGroupCallback*       mAnimGraphRemoveNodeGroupCallback;
        CommandAnimGraphCreateParameterCallback*       mAnimGraphCreateParameterCallback;
        CommandAnimGraphRemoveParameterCallback*       mAnimGraphRemoveParameterCallback;
        CommandAnimGraphAdjustParameterCallback*       mAnimGraphAdjustParameterCallback;
        CommandAnimGraphSwapParametersCallback*        mAnimGraphSwapParametersCallback;
        CommandAnimGraphAdjustParameterGroupCallback*  mAnimGraphAdjustParameterGroupCallback;
        CommandAnimGraphAddParameterGroupCallback*     mAnimGraphAddParameterGroupCallback;
        CommandAnimGraphRemoveParameterGroupCallback*  mAnimGraphRemoveParameterGroupCallback;

        AnimGraphPlugin*        mPlugin;
        QTableWidget*           mTable;
        uint32                  mSelectedAnimGraphID;
        QPushButton*            mCreateButton;
        QPushButton*            mRemoveButton;
        QPushButton*            mClearButton;
        QPushButton*            mPropertiesButton;
        MysticQt::SearchButton* mFindWidget;
        MysticQt::ComboBox*     mMotionSetComboBox;
        QPushButton*            mImportButton;
        QPushButton*            mSaveButton;
        QPushButton*            mSaveAsButton;
        AZStd::string           m_tempString;
    };
} // namespace EMStudio