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

#ifndef __EMSTUDIO_ANIMGRAPHPLUGIN_H
#define __EMSTUDIO_ANIMGRAPHPLUGIN_H

// include MCore
#include "../StandardPluginsConfig.h"

#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MysticQt/Source/DockWidget.h>

#include <MCore/Source/Random.h>
#include <MCore/Source/Array.h>

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphBus.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphNodeId.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Source/EventHandler.h>

#include <QDockWidget>
#include <QStackedWidget>

#include "AnimGraphOptions.h"
#include "NodeGraph.h"
#include "StateGraphNode.h"

QT_FORWARD_DECLARE_CLASS(QSettings)
QT_FORWARD_DECLARE_CLASS(QMenu)




namespace EMStudio
{
    // forward declarations
    class BlendGraphWidget;
    class NodeGraph;
    class GraphNode;
    class NodePaletteWidget;
    class NavigateWidget;
    class BlendTreeVisualNode;
    class AttributesWindow;
    class GameControllerWindow;
    class GraphNodeFactory;
    class ParameterWindow;
    class NodeGroupWindow;
    class BlendGraphViewWidget;
    class AnimGraphPlugin;
    class RecorderWidget;
    class TimeViewPlugin;
    class SaveDirtyAnimGraphFilesCallback;
    class OutlinerCategoryCallback;

    // our anim graph event handler
    class AnimGraphEventHandler
        : public EMotionFX::EventHandler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphEventHandler(AnimGraphPlugin* plugin);

        void OnDeleteAnimGraph(EMotionFX::AnimGraph* animGraph) override;

        void Update(EMotionFX::AnimGraphStateMachine* stateMachine, EMotionFX::AnimGraphInstance* animGraphInstance, NodeGraph* visualGraph);

        void Sync(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* animGraphNode) override;
        void OnSetVisualManipulatorOffset(EMotionFX::AnimGraphInstance* animGraphInstance, uint32 paramIndex, const AZ::Vector3& offset) override;
        void OnParameterNodeMaskChanged(EMotionFX::BlendTreeParameterNode* parameterNode, const AZStd::vector<AZStd::string>& newParameterMask) override;
        bool OnRayIntersectionTest(const AZ::Vector3& start, const AZ::Vector3& end, EMotionFX::IntersectionInfo* outIntersectInfo) override;
        void OnCreatedNode(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node) override;

    private:
        AnimGraphPlugin* mPlugin;
    };

    class AnimGraphPerFrameCallback
    {
    public:
        virtual void ProcessFrame(float timePassedInSeconds) = 0;
    };

    /**
     *
     *
     */
    class AnimGraphPlugin
        : public EMStudio::DockWidgetPlugin
        , public EMotionFX::AnimGraphNotificationBus::Handler
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT

    public:
        enum
        {
            CLASS_ID = 0x06def5df
        };

        enum
        {
            DISPLAYFLAG_PLAYSPEED       = 1 << 0,
            DISPLAYFLAG_GLOBALWEIGHT    = 1 << 1,
            DISPLAYFLAG_SYNCSTATUS      = 1 << 2,
            DISPLAYFLAG_PLAYPOSITION    = 1 << 3
        };

        struct AnimGraphWithNode
        {
            EMotionFX::AnimGraph*      mAnimGraph;
            EMotionFX::AnimGraphNode*  mNode;
        };

        struct GraphInfo
        {
            MCORE_MEMORYOBJECTCATEGORY(AnimGraphPlugin::GraphInfo, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
            NodeGraph*                  mVisualGraph;
            EMotionFX::AnimGraph*       mAnimGraph;
            EMotionFX::AnimGraphNodeId  mNodeId;

            GraphInfo()
                : mVisualGraph(nullptr)
                , mAnimGraph(nullptr)
            {}
            ~GraphInfo() { delete mVisualGraph; }
        };

        struct HistoryItem
        {
            MCORE_MEMORYOBJECTCATEGORY(AnimGraphPlugin::HistoryItem, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

            AZStd::string                   mNodeName;
            EMotionFX::AnimGraph*          mAnimGraph;

            HistoryItem()
            {
                mAnimGraph = nullptr;
            }

            EMotionFX::AnimGraphNode* FindNode()
            {
                if (mAnimGraph == nullptr)
                {
                    return nullptr;
                }

                return mAnimGraph->RecursiveFindNodeByName(mNodeName.c_str());
            }

            HistoryItem(const char* nodeName, EMotionFX::AnimGraph* animGraph)
            {
                mNodeName   = nodeName;
                mAnimGraph = animGraph;
            }
        };

        struct AnimGraphHistory
        {
            AnimGraphHistory()
            {
                mAnimGraph = nullptr;
                mHistoryIndex = 0;
            }

            EMotionFX::AnimGraph*      mAnimGraph;
            MCore::Array<HistoryItem>   mHistory;
            uint32                      mHistoryIndex;
        };

        AnimGraphPlugin();
        ~AnimGraphPlugin();

        // overloaded
        const char* GetCompileDate() const override;
        const char* GetName() const override;
        uint32 GetClassID() const override;
        const char* GetCreatorName() const override;
        float GetVersion() const override;
        bool GetIsClosable() const override             { return true; }
        bool GetIsFloatable() const override            { return true; }
        bool GetIsVertical() const override             { return false; }
        uint32 GetProcessFramePriority() const override { return 200; }
        void AddWindowMenuEntries(QMenu* parent) override;

        void SetActiveAnimGraph(EMotionFX::AnimGraph* animGraph);
        MCORE_INLINE EMotionFX::AnimGraph* GetActiveAnimGraph()           { return mActiveAnimGraph; }
        bool IsAnimGraphRunning() const { return mAnimGraphRunning; }
        void SetAnimGraphRunning(bool isRunning) { mAnimGraphRunning = isRunning; }

        void SelectAll();
        void UnselectAll();
        void FitActiveGraphOnScreen();
        void FitActiveSelectionOnScreen();
        void OnUpdateUniqueData();

        void SaveAnimGraph(const char* filename, uint32 animGraphIndex, MCore::CommandGroup* commandGroup = nullptr);
        void SaveAnimGraph(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup = nullptr);
        void SaveAnimGraphAs(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup = nullptr);
        int SaveDirtyAnimGraph(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton = true);
        int OnSaveDirtyAnimGraphs();

        void SetVirtualFinalNode(EMotionFX::AnimGraphNode* node);
        void RecursiveSetOpacity(NodeGraph* graph, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* startNode, float opacity);

        PluginOptions* GetOptions() override { return &mOptions; }

        void LoadOptions();
        void SaveOptions();

        void RegisterKeyboardShortcuts() override;

        void RemoveAllUnusedGraphInfos();

        bool CheckIfCanCreateObject(EMotionFX::AnimGraphObject* parentObject, EMotionFX::AnimGraphObject* object, EMotionFX::AnimGraphObject::ECategory category) const;

        void ProcessFrame(float timePassedInSeconds) override;

        void OnAnimGraphNodeInputPortsChanged(EMotionFX::AnimGraphNode* node, const MCore::Array<EMotionFX::AnimGraphNode::Port>& newPorts);

        TimeViewPlugin* FindTimeViewPlugin() const;

        void RegisterPerFrameCallback(AnimGraphPerFrameCallback* callback);
        void UnregisterPerFrameCallback(AnimGraphPerFrameCallback* callback);

        void OnMainWindowClosed() override;
    public slots:
        // save/load the active anim graph
        void OnFileOpen();
        void OnFileSave();
        void OnFileSaveAs();
        void OnDoubleClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, EMotionFX::Recorder::NodeHistoryItem* historyItem);
        void OnClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, EMotionFX::Recorder::NodeHistoryItem* historyItem);

        void OnUpdateAttributesWindow();
        void UpdateWindowVisibility();

    public:
        void RemoveAllGraphs();
        void ReInitAllGraphs();
        void ShowGraph(EMotionFX::AnimGraphNode* node, EMotionFX::AnimGraph* animGraph, bool addToHistory = true);

        void CleanHistory();
        void HistoryStepBack();
        void HistoryStepForward();
        void ShowGraphFromHistory(uint32 historyIndex);

        MCORE_INLINE BlendGraphWidget* GetGraphWidget()                     { return mGraphWidget; }
        MCORE_INLINE NavigateWidget* GetNavigateWidget()                    { return mNavigateWidget; }
        MCORE_INLINE NodePaletteWidget* GetPaletteWidget()                  { return mPaletteWidget; }
        MCORE_INLINE AttributesWindow* GetAttributesWindow()                { return mAttributesWindow; }
        MCORE_INLINE ParameterWindow* GetParameterWindow()                  { return mParameterWindow; }
        MCORE_INLINE NodeGroupWindow* GetNodeGroupWidget()                  { return mNodeGroupWindow; }
        MCORE_INLINE BlendGraphViewWidget* GetViewWidget()                  { return mViewWidget; }
        MCORE_INLINE RecorderWidget* GetRecorderWidget()                    { return mRecorderWidget; }

        MCORE_INLINE MysticQt::DockWidget* GetAttributeDock()               { return mAttributeDock; }
        MCORE_INLINE MysticQt::DockWidget* GetNodePaletteDock()             { return mNodePaletteDock; }
        MCORE_INLINE MysticQt::DockWidget* GetRecorderDock()                { return mRecorderDock; }
        MCORE_INLINE MysticQt::DockWidget* GetAnimGraphAssetManagerDock()   { return mAnimGraphAssetManagerDock; }
        MCORE_INLINE MysticQt::DockWidget* GetParameterDock()               { return mParameterDock; }
        MCORE_INLINE MysticQt::DockWidget* GetNodeGroupDock()               { return mNodeGroupDock; }

        #ifdef HAS_GAME_CONTROLLER
        MCORE_INLINE GameControllerWindow* GetGameControllerWindow()        { return mGameControllerWindow; }
        MCORE_INLINE MysticQt::DockWidget* GetGameControllerDock()          { return mGameControllerDock; }
        #endif

        void ClearAllProcessedFlags(bool updateView = true);

        void SetDisplayFlagEnabled(uint32 flags, bool enabled)
        {
            if (enabled)
            {
                mDisplayFlags |= flags;
            }
            else
            {
                mDisplayFlags &= ~flags;
            }
        }
        bool GetIsDisplayFlagEnabled(uint32 flags) const                    { return (mDisplayFlags & flags); }
        uint32 GetDisplayFlags() const                                      { return mDisplayFlags; }

        MCORE_INLINE uint32 GetNumGraphInfos() const                        { return mGraphInfos.GetLength(); }
        MCORE_INLINE GraphInfo* GetGraphInfo(uint32 index)                  { return mGraphInfos[index]; }
        MCORE_INLINE void AddGraphInfo(GraphInfo* info)                     { mGraphInfos.Add(info); }

        uint32 FindGraphInfo(EMotionFX::AnimGraphNodeId nodeId, EMotionFX::AnimGraph* animGraph);
        uint32 FindGraphInfo(NodeGraph* graph, EMotionFX::AnimGraph* animGraph);
        GraphNode* FindGraphNode(NodeGraph* graph, EMotionFX::AnimGraphNodeId nodeId, EMotionFX::AnimGraph* animGraph);
        NodeGraph* FindGraphForNode(EMotionFX::AnimGraphNodeId nodeId, EMotionFX::AnimGraph* animGraph);
        StateConnection* FindStateConnection(EMotionFX::AnimGraphStateTransition* transition);

        GraphNodeFactory* GetGraphNodeFactory()                             { return mGraphNodeFactory; }

        bool CheckIfIsParentNodeRecursively(EMotionFX::AnimGraphNode* startNode, EMotionFX::AnimGraphNode* mightBeAParent);

        void RemoveGraphForNode(EMotionFX::AnimGraphNodeId nodeId, EMotionFX::AnimGraph* animGraph);

        // overloaded main init function
        void Reflect(AZ::ReflectContext* serializeContext) override;
        bool Init() override;
        void OnAfterLoadLayout() override;
        EMStudioPlugin* Clone() override;

        bool FindGraphAndNode(EMotionFX::AnimGraphNode* parentNode, EMotionFX::AnimGraphNodeId nodeId, NodeGraph** outVisualGraph, GraphNode** outGraphNode, EMotionFX::AnimGraph* animGraph, bool logErrorWhenNotFound = true);
        void SyncVisualNode(EMotionFX::AnimGraphNode* node);
        void SyncVisualNodes();
        void SyncVisualTransitions();
        void SyncTransition(EMotionFX::AnimGraphStateTransition* transition);
        void SyncTransition(EMotionFX::AnimGraphStateTransition* transition, StateConnection* visualTransition);

        bool CanPopHistory()                                                                            { return (mCurrentAnimGraphHistory) ? mCurrentAnimGraphHistory->mHistoryIndex > 0 : false; }
        bool CanStepForwardInHistory()                                                                  { return (mCurrentAnimGraphHistory) ? mCurrentAnimGraphHistory->mHistoryIndex + 1 < mCurrentAnimGraphHistory->mHistory.GetLength() : false; }

        HistoryItem GetHistoryItem(uint32 index)                                                        { assert(mCurrentAnimGraphHistory); return mCurrentAnimGraphHistory->mHistory[index]; }
        uint32 GetNumHistoryItems() const                                                               { return (mCurrentAnimGraphHistory) ? mCurrentAnimGraphHistory->mHistory.GetLength() : 0; }

        MCORE_INLINE const AnimGraphOptions& GetAnimGraphOptions() const                                { return mOptions; }

        void UpdateStateMachineColors();

        MCORE_INLINE void SetDisableRendering(bool flag)                                                { mDisableRendering = flag; }
        MCORE_INLINE bool GetDisableRendering() const                                                   { return mDisableRendering; }

        // AnimGraphNotificationBus
        void OnSyncVisualObject(EMotionFX::AnimGraphObject* object) override;

    private:
        enum EDockWindowOptionFlag
        {
            WINDOWS_PARAMETERWINDOW = 1,
            WINDOWS_ATTRIBUTEWINDOW = 2,
            WINDOWS_NODEGROUPWINDOW = 3,
            WINDOWS_PALETTEWINDOW = 4,
            WINDOWS_GAMECONTROLLERWINDOW = 5,
            WINDOWS_RECORDER = 6,

            NUM_DOCKWINDOW_OPTIONS //automatically gets the next number assigned
        };

        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateBlendNodeCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustBlendNodeCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateConnectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveConnectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustConnectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveNodePreCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveNodePostCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSetAsEntryStateCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAddConditionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphRemoveConditionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandPlayMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphMoveParameterCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRecorderClearCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandMotionSetAdjustMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphCreateParameterCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAdjustParameterCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphRemoveParameterCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAddGroupParameter);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphRemoveGroupParameter);
        MCORE_DEFINECOMMANDCALLBACK(CommandAnimGraphAdjustGroupParameter);

        CommandSelectCallback*                      mSelectCallback;
        CommandUnselectCallback*                    mUnselectCallback;
        CommandClearSelectionCallback*              mClearSelectionCallback;
        CommandCreateBlendNodeCallback*             mCreateBlendNodeCallback;
        CommandAdjustBlendNodeCallback*             mAdjustBlendNodeCallback;
        CommandCreateConnectionCallback*            mCreateConnectionCallback;
        CommandRemoveConnectionCallback*            mRemoveConnectionCallback;
        CommandAdjustConnectionCallback*            mAdjustConnectionCallback;
        CommandRemoveNodePostCallback*              mRemoveNodePostCallback;
        CommandRemoveNodePreCallback*               mRemoveNodePreCallback;
        CommandSetAsEntryStateCallback*             mSetAsEntryStateCallback;
        CommandAnimGraphAddConditionCallback*       mAddConditionCallback;
        CommandAnimGraphRemoveConditionCallback*    mRemoveConditionCallback;
        CommandPlayMotionCallback*                  mPlayMotionCallback;
        CommandAnimGraphMoveParameterCallback*      mMoveParameterCallback;
        CommandRecorderClearCallback*               mRecorderClearCallback;
        CommandMotionSetAdjustMotionCallback*       mMotionSetAdjustMotionCallback;
        CommandAnimGraphCreateParameterCallback*    mCreateParameterCallback;
        CommandAnimGraphAdjustParameterCallback*    mAdjustParameterCallback;
        CommandAnimGraphRemoveParameterCallback*    mRemoveParameterCallback;
        CommandAnimGraphAddGroupParameter*          mAddGroupParameterCallback;
        CommandAnimGraphRemoveGroupParameter*       mRemoveGroupParameterCallback;
        CommandAnimGraphAdjustGroupParameter*       mAdjustGroupParameterCallback;

        AZStd::vector<AnimGraphPerFrameCallback*>   mPerFrameCallbacks;

        bool                                        mDisableRendering;

        MCore::Array<GraphInfo*>                    mGraphInfos;
        MCore::Array<AnimGraphHistory*>             mAnimGraphHistories;
        MCore::Array<AnimGraphWithNode*>            mCurrentVisibleNodeOnAnimGraphs;
        AnimGraphHistory*                           mCurrentAnimGraphHistory;

        AnimGraphEventHandler*                      mEventHandler;

        BlendGraphWidget*                           mGraphWidget;
        NavigateWidget*                             mNavigateWidget;
        NodePaletteWidget*                          mPaletteWidget;
        AttributesWindow*                           mAttributesWindow;
        ParameterWindow*                            mParameterWindow;
        NodeGroupWindow*                            mNodeGroupWindow;
        BlendGraphViewWidget*                       mViewWidget;
        RecorderWidget*                             mRecorderWidget;

        QStackedWidget                              mViewportStack;

        SaveDirtyAnimGraphFilesCallback*            mDirtyFilesCallback;
        OutlinerCategoryCallback*                   mOutlinerCategoryCallback;

        MysticQt::DockWidget*                       mAttributeDock;
        MysticQt::DockWidget*                       mNodePaletteDock;
        MysticQt::DockWidget*                       mRecorderDock;
        MysticQt::DockWidget*                       mAnimGraphAssetManagerDock;
        MysticQt::DockWidget*                       mParameterDock;
        MysticQt::DockWidget*                       mNodeGroupDock;
        QAction*                                    mDockWindowActions[NUM_DOCKWINDOW_OPTIONS];
        EMotionFX::AnimGraph*                       mActiveAnimGraph;
        bool                                        mAnimGraphRunning;

#ifdef HAS_GAME_CONTROLLER
        GameControllerWindow*                       mGameControllerWindow;
        QPointer<MysticQt::DockWidget>              mGameControllerDock;
#endif

        float                                       mLastPlayTime;
        float                                       mTotalTime;

        uint32                                      mDisplayFlags;

        AnimGraphOptions                            mOptions;
        
        GraphNodeFactory*                           mGraphNodeFactory;
        uint32                                      mMaxHistoryEntries;

        MCORE_INLINE void PopHistory()                                                                  { mCurrentAnimGraphHistory->mHistory.RemoveFirst(); }
        void PushHistory(EMotionFX::AnimGraphNode* node, EMotionFX::AnimGraph* animGraph);

        void InitForAnimGraph(EMotionFX::AnimGraph* setup);
        NodeGraph* CreateGraph(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node);
        void ReInitGraph(NodeGraph* graph, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node);
        MCORE_INLINE bool GetOptionFlag(EDockWindowOptionFlag option) { return mDockWindowActions[(uint32)option]->isChecked(); }
        void SetOptionFlag(EDockWindowOptionFlag option, bool isEnabled);
        void SetOptionEnabled(EDockWindowOptionFlag option, bool isEnabled);

    };

}   // namespace EMStudio


#endif
