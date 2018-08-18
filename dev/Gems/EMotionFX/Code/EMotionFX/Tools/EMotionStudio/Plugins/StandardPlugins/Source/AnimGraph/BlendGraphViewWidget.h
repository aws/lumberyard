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
#include <Editor/ActorEditorBus.h>
#include "NavigationLinkWidget.h"
#include "../StandardPluginsConfig.h"
#include "BlendGraphWidget.h"
#include <QAction>
#include <QWidget>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/functional.h>

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QMenuBar)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QSettings)
QT_FORWARD_DECLARE_CLASS(QSplitter)
QT_FORWARD_DECLARE_CLASS(QLayout)


namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;
    class AnimGraphNodeWidget;

    //
    class BlendGraphViewWidget
        : public QWidget
        , public EMotionFX::ActorEditorRequestBus::Handler
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendGraphViewWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        enum EOptionFlag
        {
            SELECTION_ALIGNLEFT                 = 0,
            SELECTION_ALIGNRIGHT                = 1,
            SELECTION_ALIGNTOP                  = 2,
            SELECTION_ALIGNBOTTOM               = 3,
            FILE_NEW                            = 4,
            FILE_OPENFILE                       = 5,
            FILE_OPEN                           = 6,
            FILE_SAVE                           = 7,
            FILE_SAVEAS                         = 8,
            NAVIGATION_FORWARD                  = 9,
            NAVIGATION_BACK                     = 10,
            NAVIGATION_ROOT                     = 11,
            NAVIGATION_PARENT                   = 12,
            NAVIGATION_OPENSELECTEDNODE         = 13,
            NAVIGATION_NAVPANETOGGLE            = 14,
            SELECTION_ZOOMSELECTION             = 15,
            SELECTION_ACTIVATESTATE             = 16,
            SELECTION_DELETENODES               = 17,
            SELECTION_ADDWILDCARDTRANSITION     = 18,
            WINDOWS_NODEGROUPWINDOW             = 19,
            VISUALIZATION_PLAYSPEEDS            = 20,
            VISUALIZATION_GLOBALWEIGHTS         = 21,
            VISUALIZATION_SYNCSTATUS            = 22,
            VISUALIZATION_PLAYPOSITIONS         = 23,

            NUM_OPTIONS //automatically gets the next number assigned
        };

        BlendGraphViewWidget(AnimGraphPlugin* plugin, QWidget* parentWidget);
        ~BlendGraphViewWidget();

        MCORE_INLINE bool GetOptionFlag(EOptionFlag option)         { return mActions[(uint32)option]->isChecked(); }
        void SetOptionFlag(EOptionFlag option, bool isEnabled);
        void SetOptionEnabled(EOptionFlag option, bool isEnabled);

        void Init(BlendGraphWidget* blendGraphWidget);
        void Update();
        void UpdateOpenMenu();

        // If there is a specific widget to handle this node returns that.
        // Else, returns nullptr.
        AnimGraphNodeWidget* GetWidgetForNode(const EMotionFX::AnimGraphNode* node);

        void SetCurrentNode(EMotionFX::AnimGraphNode* node);
        void SetSelectedAnimGraph(uint32 id) { mSelectedAnimGraphID = id; }
        void OpenAnimGraph();
        EMotionFX::AnimGraph* GetSelectedAnimGraph();

        // ActorEditorRequests
        EMotionFX::ActorInstance* GetSelectedActorInstance() override;

        void ActivateAnimGraphToSelection(EMotionFX::AnimGraph* animGraph, EMotionFX::MotionSet* motionSet = nullptr);

    public slots:
        void OnCreateAnimGraph();

        void AlignLeft();
        void AlignRight();
        void AlignTop();
        void AlignBottom();

        void NavigateForward();
        void NavigateBackward();
        void NavigateToRoot();
        void NavigateToNode();
        void NavigateToParent();
        void ToggleNavigationPane();

        void ZoomSelected();

        void OnActivateState();

        void OnDisplayPlaySpeeds();
        void OnDisplayGlobalWeights();
        void OnDisplaySyncStatus();
        void OnDisplayPlayPositions();

    protected:
        void SaveOptions(QSettings* settings);
        void LoadOptions(QSettings* settings);

        void Reset();
        void CreateEntry(QMenu* menu, QHBoxLayout* toolbarLayout, const char* entryName, const char* toolbarIconFileName, bool addToToolbar, bool checkable, int32 actionIndex, const QKeySequence& shortcut = 0, bool border = true, bool addToMenu = true);
        void BuildOpenMenu();
        void AddSeparator(QLayout *layout = nullptr);

        void AlignNodes(uint32 mode);

        void keyReleaseEvent(QKeyEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private:
        MCORE_DEFINECOMMANDCALLBACK(CommandSaveAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveAnimGraphCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSaveMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandLoadMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandActivateAnimGraphCallback);
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

        CommandSaveAnimGraphCallback*                  mSaveAnimGraphCallback;
        CommandCreateAnimGraphCallback*                mCreateAnimGraphCallback;
        CommandRemoveAnimGraphCallback*                mRemoveAnimGraphCallback;
        CommandSelectCallback*                          mSelectCallback;
        CommandUnselectCallback*                        mUnselectCallback;
        CommandClearSelectionCallback*                  mClearSelectionCallback;
        CommandCreateMotionSetCallback*                 mCreateMotionSetCallback;
        CommandRemoveMotionSetCallback*                 mRemoveMotionSetCallback;
        CommandAdjustMotionSetCallback*                 mAdjustMotionSetCallback;
        CommandSaveMotionSetCallback*                   mSaveMotionSetCallback;
        CommandLoadMotionSetCallback*                   mLoadMotionSetCallback;
        CommandActivateAnimGraphCallback*              mActivateAnimGraphCallback;
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

        QMenuBar*               mMenu;
        QMenu*                  mOpenMenu;
        QHBoxLayout*            mToolbarLayout;
        QAction*                mActions[NUM_OPTIONS];
        QPushButton*            mToolbarButtons[NUM_OPTIONS];
        AnimGraphPlugin*        mParentPlugin;
        NavigationLinkWidget*   mNavigationLink;
        QStackedWidget          mViewportStack;
        QSplitter*              mViewportSplitter;
        AZStd::string           m_tempString;
        uint32                  mSelectedAnimGraphID;

        // This maps a node's UUID to a widget that will be used to
        // display the "contents" of that node type.  If no entry for a given
        // node type is found, then a BlendGraphWidget is used by default.
        // For normal blend trees and state machines, the BlendGraphWidget is
        // shown to draw the nodes inside the tree.  For special types like a
        // blendspace, a separate widget is registered to handle the drawing for that
        // node.
        AZStd::unordered_map<AZ::TypeId, AnimGraphNodeWidget*> mNodeTypeToWidgetMap;
    };
}   // namespace EMStudio
