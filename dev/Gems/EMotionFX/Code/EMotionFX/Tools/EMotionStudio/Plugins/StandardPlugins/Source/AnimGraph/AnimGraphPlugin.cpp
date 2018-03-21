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

#include "AnimGraphPlugin.h"
#include "BlendGraphWidget.h"
#include "NodeGraph.h"
#include "NodePaletteWidget.h"
#include "BlendGraphViewWidget.h"
#include "BlendGraphWidgetCallback.h"
#include "GraphNode.h"
#include "AttributeWidgetCreator.h"
#include "NavigateWidget.h"
#include "AttributesWindow.h"
#include "NodeGroupWindow.h"
#include "BlendResourceWidget.h"
#include "BlendTreeVisualNode.h"
#include "DebugEventHandler.h"
#include "StateGraphNode.h"
#include "ParameterWindow.h"
#include "GraphNodeFactory.h"
#include "RecorderWidget.h"
#include <MysticQt/Source/DockHeader.h>
#include <QDockWidget>
#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QScrollArea>
#include <QFileDialog>
#include <QApplication>
#include <QMessageBox>
#include <QSettings>
#include <QMenu>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringIDGenerator.h>

#ifdef HAS_GAME_CONTROLLER
    #include "GameControllerWindow.h"
#endif

#include "../../../../EMStudioSDK/Source/NotificationWindow.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include <MysticQt/Source/DialogStack.h>
#include "../../../../EMStudioSDK/Source/FileManager.h"

#include "../TimeView/TimeViewPlugin.h"
#include "../TimeView/TimeInfoWidget.h"

#include <QMessageBox>

#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphAttributeTypes.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/Source/BlendTreeConnection.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MotionManager.h>

#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>

#include <MysticQt/Source/PropertyWidget.h>
#include "../../../../EMStudioSDK/Source/PreferencesWindow.h"

#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>


namespace EMStudio
{
    class SaveDirtyAnimGraphFilesCallback
        : public SaveDirtyFilesCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(SaveDirtyFilesCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        SaveDirtyAnimGraphFilesCallback()
            : SaveDirtyFilesCallback()                           {}
        ~SaveDirtyAnimGraphFilesCallback()                                                     {}

        enum
        {
            TYPE_ID = 0x00000004
        };
        uint32 GetType() const override                                                     { return TYPE_ID; }
        uint32 GetPriority() const override                                                 { return 1; }
        bool GetIsPostProcessed() const override                                            { return false; }

        void GetDirtyFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects) override
        {
            // get the number of anim graphs and iterate through them
            const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
            for (uint32 i = 0; i < numAnimGraphs; ++i)
            {
                // return in case we found a dirty file
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

                if (animGraph->GetIsOwnedByRuntime())
                {
                    continue;
                }

                if (animGraph->GetDirtyFlag())
                {
                    // add the filename to the dirty filenames array
                    outFileNames->push_back(animGraph->GetFileName());

                    // add the link to the actual object
                    ObjectPointer objPointer;
                    objPointer.mAnimGraph = animGraph;
                    outObjects->push_back(objPointer);
                }
            }
        }

        int SaveDirtyFiles(const AZStd::vector<AZStd::string>& filenamesToSave, const AZStd::vector<ObjectPointer>& objects, MCore::CommandGroup* commandGroup) override
        {
            MCORE_UNUSED(filenamesToSave);

            EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
            AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
            if (plugin == nullptr)
            {
                return DirtyFileManager::FINISHED;
            }

            const size_t numObjects = objects.size();
            for (size_t i = 0; i < numObjects; ++i)
            {
                // get the current object pointer and skip directly if the type check fails
                ObjectPointer objPointer = objects[i];
                if (objPointer.mAnimGraph == nullptr)
                {
                    continue;
                }

                EMotionFX::AnimGraph* animGraph = objPointer.mAnimGraph;
                if (animGraphPlugin->SaveDirtyAnimGraph(animGraph, commandGroup, false) == DirtyFileManager::CANCELED)
                {
                    return DirtyFileManager::CANCELED;
                }
            }

            return DirtyFileManager::FINISHED;
        }

        const char* GetExtension() const override       { return "animgraph"; }
        const char* GetFileType() const override        { return "anim graph"; }
    };


    class AnimGraphsOutlinerCategoryCallback
        : public OutlinerCategoryCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphsOutlinerCategoryCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS)

    public:
        AnimGraphsOutlinerCategoryCallback(AnimGraphPlugin* plugin)
        {
            mPlugin = plugin;
        }

        ~AnimGraphsOutlinerCategoryCallback()
        {
        }

        QString BuildNameItem(OutlinerCategoryItem* item) const override
        {
            EMotionFX::AnimGraph* animGraph = static_cast<EMotionFX::AnimGraph*>(item->mUserData);
            return animGraph->GetName();
        }

        QString BuildToolTipItem(OutlinerCategoryItem* item) const override
        {
            EMotionFX::AnimGraph* animGraph = static_cast<EMotionFX::AnimGraph*>(item->mUserData);
            const MCore::String relativeFileName = animGraph->GetFileNameString().ExtractPathRelativeTo(EMotionFX::GetEMotionFX().GetMediaRootFolder());
            MCore::Array<EMotionFX::AnimGraphNode*> stateMachines;
            animGraph->RecursiveCollectNodesOfType(EMotionFX::AnimGraphStateMachine::TYPE_ID, &stateMachines);
            MCore::Array<EMotionFX::AnimGraphNode*> blendTrees;
            animGraph->RecursiveCollectNodesOfType(EMotionFX::BlendTree::TYPE_ID, &blendTrees);
            QString toolTip = "<table border=\"0\"";
            toolTip += "<tr><td><p style='white-space:pre'><b>Name: </b></p></td>";
            toolTip += QString("<td><p style='color:rgb(115, 115, 115); white-space:pre'>%1</p></td></tr>").arg(animGraph->GetNameString().GetIsEmpty() ? "&#60;no name&#62;" : animGraph->GetName());
            toolTip += "<tr><td><p style='white-space:pre'><b>FileName: </b></p></td>";
            toolTip += QString("<td><p style='color:rgb(115, 115, 115); white-space:pre'>%1</p></td></tr>").arg(relativeFileName.GetIsEmpty() ? "&#60;not saved yet&#62;" : relativeFileName.AsChar());
            toolTip += "<tr><td><p style='white-space:pre'><b>Num Nodes: </b></p></td>";
            toolTip += QString("<td><p style='color:rgb(115, 115, 115); white-space:pre'>%1</p></td></tr>").arg(animGraph->GetNumNodes());
            toolTip += "<tr><td><p style='white-space:pre'><b>Num Connections: </b></p></td>";
            toolTip += QString("<td><p style='color:rgb(115, 115, 115); white-space:pre'>%1</p></td></tr>").arg(animGraph->RecursiveCalcNumNodeConnections());
            toolTip += "<tr><td><p style='white-space:pre'><b>Num Motion Nodes: </b></p></td>";
            toolTip += QString("<td><p style='color:rgb(115, 115, 115); white-space:pre'>%1</p></td></tr>").arg(animGraph->GetNumMotionNodes());
            toolTip += "<tr><td><p style='white-space:pre'><b>Num State Machines: </b></p></td>";
            toolTip += QString("<td><p style='color:rgb(115, 115, 115); white-space:pre'>%1</p></td></tr>").arg(stateMachines.GetLength());
            toolTip += "<tr><td><p style='white-space:pre'><b>Num Blend Trees: </b></p></td>";
            toolTip += QString("<td><p style='color:rgb(115, 115, 115); white-space:pre'>%1</p></td></tr>").arg(blendTrees.GetLength());
            toolTip += "</table>";
            return toolTip;
        }

        QIcon GetIcon(OutlinerCategoryItem* item) const override
        {
            MCORE_UNUSED(item);
            return MysticQt::GetMysticQt()->FindIcon("Images/OutlinerPlugin/AnimGraphsCategory.png");
        }

        void OnRemoveItems(QWidget* parent, const AZStd::vector<OutlinerCategoryItem*>& items, MCore::CommandGroup* commandGroup) override
        {
            MCORE_UNUSED(parent);

            // remove each item
            const size_t numItems = items.size();
            for (size_t i = 0; i < numItems; ++i)
            {
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(items[i]->mID);
                mPlugin->SaveDirtyAnimGraph(animGraph, nullptr, true, false);
                commandGroup->AddCommandString(MCore::String().Format("RemoveAnimGraph -animGraphID %i", animGraph->GetID()).AsChar());
            }
        }

        void OnPostRemoveItems(QWidget* parent) override
        {
            MCORE_UNUSED(parent);
        }

        void OnLoadItem(QWidget* parent) override
        {
            MCORE_UNUSED(parent);
            mPlugin->OnFileOpen();
        }

    private:
        AnimGraphPlugin* mPlugin;
    };


    // constructor
    AnimGraphPlugin::AnimGraphPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        mGraphInfos.SetMemoryCategory(MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        mAnimGraphHistories.SetMemoryCategory(MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        mCurrentVisibleNodeOnAnimGraphs.SetMemoryCategory(MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

        mGraphWidget                    = nullptr;
        mNavigateWidget                 = nullptr;
        mAttributeDock                  = nullptr;
        mNodeGroupDock                  = nullptr;
        mResourceWidget                 = nullptr;
        mRecorderWidget                 = nullptr;
        mEventHandler                   = nullptr;
        mVizScaleProperty               = nullptr;
        mGraphAnimationProperty         = nullptr;
        mPaletteWidget                  = nullptr;
        mNodePaletteDock                = nullptr;
        mParameterDock                  = nullptr;
        mParameterWindow                = nullptr;
        mNodeGroupWindow                = nullptr;
        mAttributesWindow               = nullptr;
        mNavigationDock                 = nullptr;
        mActiveAnimGraph               = nullptr;
        mSelectCallback                 = nullptr;
        mUnselectCallback               = nullptr;
        mClearSelectionCallback         = nullptr;
        mCreateBlendNodeCallback        = nullptr;
        mAdjustBlendNodeCallback        = nullptr;
        mSwapParametersCallback         = nullptr;
        mRecorderClearCallback          = nullptr;
        mMotionSetAdjustMotionCallback  = nullptr;
        mCreateConnectionCallback       = nullptr;
        mRemoveConnectionCallback       = nullptr;
        mAdjustConnectionCallback       = nullptr;
        mSetAsEntryStateCallback        = nullptr;
        mRemoveNodePreCallback          = nullptr;
        mRemoveNodePostCallback         = nullptr;
        mAddConditionCallback           = nullptr;
        mRemoveConditionCallback        = nullptr;
        mPlayMotionCallback             = nullptr;
        mGraphNodeFactory               = nullptr;
        mViewWidget                     = nullptr;
        mAttributeWidgetCallback        = nullptr;
        mDirtyFilesCallback             = nullptr;
        mOutlinerCategoryCallback       = nullptr;
        mCreateParameterCallback        = nullptr;
        mAdjustParameterCallback        = nullptr;
        mRemoveParameterCallback        = nullptr;

        mCurrentAnimGraphHistory       = nullptr;
        mDisplayFlags                   = 0;
        mMaxHistoryEntries              = 256;
        //  mShowProcessed                  = false;
        mDisableRendering               = false;
        mLastPlayTime                   = -1;
        mTotalTime                      = FLT_MAX;
#ifdef HAS_GAME_CONTROLLER
        mGameControllerWindow           = nullptr;
        mGameControllerDock             = nullptr;
#endif
    }


    // destructor
    AnimGraphPlugin::~AnimGraphPlugin()
    {
        // destroy the event handler
        EMotionFX::GetEventManager().RemoveEventHandler(mEventHandler, true);

        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mCreateBlendNodeCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustBlendNodeCallback, false);
        GetCommandManager()->RemoveCommandCallback(mCreateConnectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveConnectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustConnectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveNodePreCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveNodePostCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSetAsEntryStateCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAddConditionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveConditionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mPlayMotionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSwapParametersCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRecorderClearCallback, false);
        GetCommandManager()->RemoveCommandCallback(mMotionSetAdjustMotionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mCreateParameterCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustParameterCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveParameterCallback, false);
        delete mSelectCallback;
        delete mUnselectCallback;
        delete mClearSelectionCallback;
        delete mPlayMotionCallback;
        delete mCreateBlendNodeCallback;
        delete mAdjustBlendNodeCallback;
        delete mCreateConnectionCallback;
        delete mRemoveConnectionCallback;
        delete mAdjustConnectionCallback;
        delete mRemoveNodePreCallback;
        delete mRemoveNodePostCallback;
        delete mSetAsEntryStateCallback;
        delete mAddConditionCallback;
        delete mRemoveConditionCallback;
        delete mSwapParametersCallback;
        delete mRecorderClearCallback;
        delete mMotionSetAdjustMotionCallback;
        delete mCreateParameterCallback;
        delete mAdjustParameterCallback;
        delete mRemoveParameterCallback;

        // remove the dirty file manager callback
        GetMainWindow()->GetDirtyFileManager()->RemoveCallback(mDirtyFilesCallback, false);
        delete mDirtyFilesCallback;

        // delete the custom attribute creators
        MysticQt::AttributeWidgetFactory* attributeFactory = MysticQt::GetMysticQt()->GetAttributeWidgetFactory();
        attributeFactory->UnregisterCreatorByTypeID(FileBrowserAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(Vector3AttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(RotationAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(NodeNamesAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(MotionPickerAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(ParameterPickerAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(BlendTreeMotionAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(MotionEventTrackAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(NodeSelectionAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(GoalNodeSelectionAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(ParameterNamesAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(AnimGraphStateAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(StateFilterLocalAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(AnimGraphNodeAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(MultipleMotionPickerAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(TagAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(TagPickerAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(BlendSpaceMotionsAttributeWidgetCreator::TYPE_ID);
        attributeFactory->UnregisterCreatorByTypeID(BlendSpaceMotionPickerAttributeWidgetCreator::TYPE_ID);


        // remove the interface callback
        attributeFactory->RemoveCallback(mAttributeWidgetCallback, false);
        delete mAttributeWidgetCallback;

        // delete the graph node factory
        delete mGraphNodeFactory;

        // remove the attribute dock widget
        if (mParameterDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(mParameterDock);
            delete mParameterDock;
        }

        // remove the attribute dock widget
        if (mAttributeDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(mAttributeDock);
            delete mAttributeDock;
        }

        // remove the node group dock widget
        if (mNodeGroupDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(mNodeGroupDock);
            delete mNodeGroupDock;
        }

        // remove the blend node palette
        if (mNodePaletteDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(mNodePaletteDock);
            delete mNodePaletteDock;
        }

        // remove the navigation dock
        if (mNavigationDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(mNavigationDock);
            delete mNavigationDock;
        }

        // remove the game controller dock
    #ifdef HAS_GAME_CONTROLLER
        if (mGameControllerDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(mGameControllerDock);
            delete mGameControllerDock;
        }
    #endif

        // delete all graphs
        RemoveAllGraphs();

        // unregister the outliner category
        GetOutlinerManager()->UnregisterCategory("Anim Graphs");
        delete mOutlinerCategoryCallback;
    }


    // get the compile date
    const char* AnimGraphPlugin::GetCompileDate() const
    {
        return MCORE_DATE;
    }


    // get the name
    const char* AnimGraphPlugin::GetName() const
    {
        return "Anim Graph";
    }


    // get the plugin type id
    uint32 AnimGraphPlugin::GetClassID() const
    {
        return AnimGraphPlugin::CLASS_ID;
    }


    // get the creator name
    const char* AnimGraphPlugin::GetCreatorName() const
    {
        return "MysticGD";
    }


    // get the version
    float AnimGraphPlugin::GetVersion() const
    {
        return 1.0f;
    }


    // clone the log window
    EMStudioPlugin* AnimGraphPlugin::Clone()
    {
        AnimGraphPlugin* newPlugin = new AnimGraphPlugin();
        return newPlugin;
    }


    // try to find the time view plugin
    TimeViewPlugin* AnimGraphPlugin::FindTimeViewPlugin() const
    {
        EMStudioPlugin* timeViewBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(TimeViewPlugin::CLASS_ID);
        if (timeViewBasePlugin)
        {
            return static_cast<TimeViewPlugin*>(timeViewBasePlugin);
        }

        return nullptr;
    }

    void AnimGraphPlugin::RegisterPerFrameCallback(AnimGraphPerFrameCallback* callback)
    {
        if (AZStd::find(mPerFrameCallbacks.begin(), mPerFrameCallbacks.end(), callback) == mPerFrameCallbacks.end())
        {
            mPerFrameCallbacks.push_back(callback);
        }
    }

    void AnimGraphPlugin::UnregisterPerFrameCallback(AnimGraphPerFrameCallback* callback)
    {
        auto it = AZStd::find(mPerFrameCallbacks.begin(), mPerFrameCallbacks.end(), callback);
        if (it != mPerFrameCallbacks.end())
        {
            mPerFrameCallbacks.erase(it);
        }
    }

    void AnimGraphPlugin::OnMainWindowClosed()
    {
        // If the recorder is on, turn it off
        if (EMotionFX::GetEMotionFX().GetRecorder()
            && EMotionFX::GetEMotionFX().GetRecorder()->GetIsRecording())
        {
            EMotionFX::GetEMotionFX().GetRecorder()->StopRecording();
        }

        EMStudio::DockWidgetPlugin::OnMainWindowClosed();
    }

    // init after the parent dock window has been created
    bool AnimGraphPlugin::Init()
    {
        // iterate over all anim graph nodes inside EMotion FX and update their input port change function handlers
        // this is needed because Init is only called when showing interface items
        // if we were a layout that did not show the plugin yet (so Init has not been called yet), then it is possible there are animgraphs
        // that have their nodes not mapped to these new functions yet
        for (uint32 i = 0; i < EMotionFX::GetAnimGraphManager().GetNumAnimGraphs(); ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            const uint32 numNodes = animGraph->GetNumNodes();
            for (uint32 n = 0; n < numNodes; ++n)
            {
                EMotionFX::AnimGraphNode* node = animGraph->GetNode(n);
                using AZStd::placeholders::_1;
                using AZStd::placeholders::_2;
                auto func = AZStd::bind(&EMStudio::AnimGraphPlugin::OnAnimGraphNodeInputPortsChanged, this, _1, _2);
                node->SetInputPortChangeFunction(func);
            }
        }

        MysticQt::AttributeWidgetFactory* attributeFactory = MysticQt::GetMysticQt()->GetAttributeWidgetFactory();

        // register default creators
        attributeFactory->RegisterCreator(new FileBrowserAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new Vector3AttributeWidgetCreator());
        attributeFactory->RegisterCreator(new RotationAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new NodeNamesAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new MotionPickerAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new ParameterPickerAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new BlendTreeMotionAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new MotionEventTrackAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new NodeSelectionAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new GoalNodeSelectionAttributeWidgetCreator());
        //  attributeFactory->RegisterCreator( new MotionExtractionComponentWidgetCreator() );
        attributeFactory->RegisterCreator(new ParameterNamesAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new AnimGraphStateAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new StateFilterLocalAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new AnimGraphNodeAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new MultipleMotionPickerAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new TagAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new TagPickerAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new BlendSpaceMotionsAttributeWidgetCreator());
        attributeFactory->RegisterCreator(new BlendSpaceMotionPickerAttributeWidgetCreator());

        // create a new attribute widget callback and add it to the attribute widget factory
        mAttributeWidgetCallback = new AttributeWidgetCallback(this);
        attributeFactory->AddCallback(mAttributeWidgetCallback);

        // create the command callbacks
        mSelectCallback                 = new CommandSelectCallback(true);
        mUnselectCallback               = new CommandUnselectCallback(true);
        mClearSelectionCallback         = new CommandClearSelectionCallback(true);
        mCreateBlendNodeCallback        = new CommandCreateBlendNodeCallback(true);
        mAdjustBlendNodeCallback        = new CommandAdjustBlendNodeCallback(false);
        mCreateConnectionCallback       = new CommandCreateConnectionCallback(true);
        mRemoveConnectionCallback       = new CommandRemoveConnectionCallback(true);
        mAdjustConnectionCallback       = new CommandAdjustConnectionCallback(true);
        mRemoveNodePreCallback          = new CommandRemoveNodePreCallback(true, true);
        mRemoveNodePostCallback         = new CommandRemoveNodePostCallback(false, false);
        mSetAsEntryStateCallback        = new CommandSetAsEntryStateCallback(false, false);
        mAddConditionCallback           = new CommandAnimGraphAddConditionCallback(false, false);
        mRemoveConditionCallback        = new CommandAnimGraphRemoveConditionCallback(false, false);
        mPlayMotionCallback             = new CommandPlayMotionCallback(true, true);
        mSwapParametersCallback         = new CommandAnimGraphWapParametersCallback(false);
        mRecorderClearCallback          = new CommandRecorderClearCallback(false);
        mMotionSetAdjustMotionCallback  = new CommandMotionSetAdjustMotionCallback(false);
        mCreateParameterCallback        = new CommandAnimGraphCreateParameterCallback(false);
        mAdjustParameterCallback        = new CommandAnimGraphAdjustParameterCallback(false);
        mRemoveParameterCallback        = new CommandAnimGraphRemoveParameterCallback(false);

        // create the graph node factory
        mGraphNodeFactory = new GraphNodeFactory();
        //mGraphNodeFactory->Register( new ParameterNodeCreator() );

        // and register them
        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", mClearSelectionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphCreateNode", mCreateBlendNodeCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustNode", mAdjustBlendNodeCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphCreateConnection", mCreateConnectionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveConnection", mRemoveConnectionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustConnection", mAdjustConnectionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveNode", mRemoveNodePreCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveNode", mRemoveNodePostCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphSetEntryState", mSetAsEntryStateCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAddCondition", mAddConditionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveCondition", mRemoveConditionCallback);
        GetCommandManager()->RegisterCommandCallback("PlayMotion", mPlayMotionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphSwapParameters", mSwapParametersCallback);
        GetCommandManager()->RegisterCommandCallback("RecorderClear", mRecorderClearCallback);
        GetCommandManager()->RegisterCommandCallback("MotionSetAdjustMotion", mMotionSetAdjustMotionCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphCreateParameter", mCreateParameterCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphAdjustParameter", mAdjustParameterCallback);
        GetCommandManager()->RegisterCommandCallback("AnimGraphRemoveParameter", mRemoveParameterCallback);

        // create the corresponding widget that holds the menu and the toolbar
        mViewWidget = new BlendGraphViewWidget(this, mDock);
        mDock->SetContents(mViewWidget);
        //mDock->SetContents( mGraphWidget );
        //mDock->setWidget( mGraphWidget ); // old: without menu and toolbar

        // create the graph widget
        mGraphWidget = new BlendGraphWidget(this, mViewWidget);
        mGraphWidget->SetCallback(new BlendGraphWidgetCallback(mGraphWidget));
        //mGraphWidget->resize(1000, 700);
        //mGraphWidget->move(0,50);
        //mGraphWidget->show();

        // get the main window
        QMainWindow* mainWindow = GetMainWindow();

        // create the attribute dock window
        mAttributeDock = new MysticQt::DockWidget(mainWindow, "Attributes");
        MysticQt::DockHeader* dockHeader = new MysticQt::DockHeader(mAttributeDock);
        mAttributeDock->setTitleBarWidget(dockHeader);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, mAttributeDock);
        QDockWidget::DockWidgetFeatures features = QDockWidget::NoDockWidgetFeatures;
        //features |= QDockWidget::DockWidgetClosable;
        features |= QDockWidget::DockWidgetFloatable;
        features |= QDockWidget::DockWidgetMovable;
        mAttributeDock->setFeatures(features);
        mAttributeDock->setObjectName("AnimGraphPlugin::mAttributeDock");
        mAttributesWindow = new AttributesWindow(this);
        mAttributeDock->SetContents(mAttributesWindow);
        dockHeader->UpdateIcons();

        // create the node group dock window
        mNodeGroupDock = new MysticQt::DockWidget(mainWindow, "Node Groups");
        dockHeader = new MysticQt::DockHeader(mNodeGroupDock);
        mNodeGroupDock->setTitleBarWidget(dockHeader);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, mNodeGroupDock);
        features = QDockWidget::NoDockWidgetFeatures;
        //features |= QDockWidget::DockWidgetClosable;
        features |= QDockWidget::DockWidgetFloatable;
        features |= QDockWidget::DockWidgetMovable;
        mNodeGroupDock->setFeatures(features);
        mNodeGroupDock->setObjectName("AnimGraphPlugin::mNodeGroupDock");
        mNodeGroupWindow = new NodeGroupWindow(this);
        mNodeGroupDock->SetContents(mNodeGroupWindow);
        dockHeader->UpdateIcons();

        // create the node palette dock
        mNodePaletteDock = new MysticQt::DockWidget(mainWindow, "Anim Graph Palette");
        dockHeader = new MysticQt::DockHeader(mNodePaletteDock);
        mNodePaletteDock->setTitleBarWidget(dockHeader);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, mNodePaletteDock);
        features = QDockWidget::NoDockWidgetFeatures;
        //features |= QDockWidget::DockWidgetClosable;
        features |= QDockWidget::DockWidgetFloatable;
        features |= QDockWidget::DockWidgetMovable;
        mNodePaletteDock->setFeatures(features);
        mNodePaletteDock->setObjectName("AnimGraphPlugin::mPaletteDock");
        mPaletteWidget = new NodePaletteWidget();
        mNodePaletteDock->SetContents(mPaletteWidget);
        dockHeader->UpdateIcons();

        // create the parameter dock
        QScrollArea* scrollArea = new QScrollArea();
        mParameterDock = new MysticQt::DockWidget(mainWindow, "Parameters");
        dockHeader = new MysticQt::DockHeader(mParameterDock);
        mParameterDock->setTitleBarWidget(dockHeader);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, mParameterDock);
        features = QDockWidget::NoDockWidgetFeatures;
        //features |= QDockWidget::DockWidgetClosable;
        features |= QDockWidget::DockWidgetFloatable;
        features |= QDockWidget::DockWidgetMovable;
        mParameterDock->setFeatures(features);
        mParameterDock->setObjectName("AnimGraphPlugin::mParameterDock");
        mParameterWindow = new ParameterWindow(this);
        mParameterDock->SetContents(scrollArea);
        scrollArea->setWidget(mParameterWindow);
        scrollArea->setWidgetResizable(true);
        dockHeader->UpdateIcons();

        // create the navigation dock
        MysticQt::DialogStack* stack = new MysticQt::DialogStack();
        mNavigationDock = new MysticQt::DockWidget(mainWindow, "Anim Graphs");
        dockHeader = new MysticQt::DockHeader(mNavigationDock);
        mNavigationDock->setTitleBarWidget(dockHeader);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, mNavigationDock);
        features = QDockWidget::NoDockWidgetFeatures;
        //features |= QDockWidget::DockWidgetClosable;
        features |= QDockWidget::DockWidgetFloatable;
        features |= QDockWidget::DockWidgetMovable;
        mNavigateWidget = new NavigateWidget(this);
        mNavigationDock->setFeatures(features);
        mNavigationDock->setObjectName("AnimGraphPlugin::mManagementDock");
        mNavigationDock->SetContents(stack);
        stack->Add(mNavigateWidget, "Navigation", false, true);
        dockHeader->UpdateIcons();

        // init the display flags
        mDisplayFlags = 0;

        // init the view widget
        // it must be init after navigate widget is created because actions are linked to it
        mViewWidget->Init(mGraphWidget);

    #ifdef HAS_GAME_CONTROLLER
        // create the game controller dock
        mGameControllerDock = new MysticQt::DockWidget(mainWindow, "Game Controller");
        dockHeader = new MysticQt::DockHeader(mGameControllerDock);
        mGameControllerDock->setTitleBarWidget(dockHeader);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, mGameControllerDock);
        features = QDockWidget::NoDockWidgetFeatures;
        //features |= QDockWidget::DockWidgetClosable;
        features |= QDockWidget::DockWidgetFloatable;
        features |= QDockWidget::DockWidgetMovable;
        mGameControllerDock->setFeatures(features);
        mGameControllerDock->setObjectName("AnimGraphPlugin::mGameControllerDock");
        mGameControllerWindow = new GameControllerWindow(this);
        mGameControllerDock->SetContents(mGameControllerWindow);
        dockHeader->UpdateIcons();
    #endif

        // load options
        LoadOptions();

        // create the anim graph resource management
        mResourceWidget = new BlendResourceWidget(this);
        stack->Add(mResourceWidget, "Resource Management", false, true, true, false);

        // create the recorder
        mRecorderWidget = new RecorderWidget(this);
        stack->Add(mRecorderWidget, "Recorder");

        // initialize the dirty files callback
        mDirtyFilesCallback = new SaveDirtyAnimGraphFilesCallback();
        GetMainWindow()->GetDirtyFileManager()->AddCallback(mDirtyFilesCallback);

        // construct the event handler
        mEventHandler = new AnimGraphEventHandler(this);
        EMotionFX::GetEventManager().AddEventHandler(mEventHandler);

        // connect to the timeline recorder data
        TimeViewPlugin* timeViewPlugin = FindTimeViewPlugin();
        if (timeViewPlugin)
        {
            connect(timeViewPlugin, SIGNAL(DoubleClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData*,EMotionFX::Recorder::NodeHistoryItem*)), this, SLOT(OnDoubleClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData*,EMotionFX::Recorder::NodeHistoryItem*)));
            connect(timeViewPlugin, SIGNAL(ClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData*,EMotionFX::Recorder::NodeHistoryItem*)), this, SLOT(OnClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData*,EMotionFX::Recorder::NodeHistoryItem*)));
        }

        // detect changes in the recorder
        connect(mRecorderWidget, SIGNAL(RecorderStateChanged()), mParameterWindow, SLOT(OnRecorderStateChanged()));

        // register the outliner category
        mOutlinerCategoryCallback = new AnimGraphsOutlinerCategoryCallback(this);
        OutlinerCategory* outlinerCategory = GetOutlinerManager()->RegisterCategory("Anim Graphs", mOutlinerCategoryCallback);

        // add each item in the outliner category
        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (uint32 i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            outlinerCategory->AddItem(animGraph->GetID(), animGraph);
        }

        return true;
    }


    // load the options
    void AnimGraphPlugin::LoadOptions()
    {
        QSettings settings(MCore::String(GetManager()->GetAppDataFolder() + "EMStudioRenderOptions.cfg").AsChar(), QSettings::IniFormat, this);

        mOptions.mVisualizeScale = (float)settings.value("visualizeScale",      (double)mOptions.mVisualizeScale).toDouble();
        mOptions.mGraphAnimation = settings.value("useGraphAnimation",          mOptions.mGraphAnimation).toBool();
        mOptions.mShowFPS        = settings.value("showFPS",                    mOptions.mShowFPS).toBool();

        // update all anim graph instances
        const uint32 numAnimGraphInstances = EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances();
        for (uint32 i = 0; i < numAnimGraphInstances; ++i)
        {
            EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i)->SetVisualizeScale(mOptions.mVisualizeScale);
        }

        settings.setValue("useGraphAnimation", mOptions.mGraphAnimation);
        settings.setValue("showFPS", mOptions.mShowFPS);
    }


    // load the options
    void AnimGraphPlugin::SaveOptions()
    {
        QSettings settings(MCore::String(GetManager()->GetAppDataFolder() + "EMStudioRenderOptions.cfg").AsChar(), QSettings::IniFormat, this);
        settings.setValue("visualizeScale", (double)mOptions.mVisualizeScale);
        settings.setValue("useGraphAnimation", mOptions.mGraphAnimation);
        settings.setValue("showFPS", mOptions.mShowFPS);
    }


    // add anim graph settings to the general settings
    void AnimGraphPlugin::AddSettings(PreferencesWindow* preferencesWindow)
    {
        MysticQt::PropertyWidget* generalPropertyWidget = preferencesWindow->FindPropertyWidgetByName("General");
        if (generalPropertyWidget == nullptr)
        {
            generalPropertyWidget = preferencesWindow->AddCategory("General", "Images/Preferences/General.png", false);
        }

        connect(generalPropertyWidget, SIGNAL(ValueChanged(MysticQt::PropertyWidget::Property*)), this, SLOT(OnValueChanged(MysticQt::PropertyWidget::Property*)));

        // visualization scale property
        mVizScaleProperty = generalPropertyWidget->AddFloatSpinnerProperty("Anim Graph Plugin Properties", "Visualization Scale", mOptions.mVisualizeScale, mOptions.mVisualizeScale, 0.0001f, 10000.0f);

        // graph animation property
        mGraphAnimationProperty = generalPropertyWidget->AddBoolProperty("Anim Graph Plugin Properties", "Graph Animation", mOptions.mGraphAnimation);

        // show fps property
        mShowFPSProperty = generalPropertyWidget->AddBoolProperty("Anim Graph Plugin Properties", "Show FPS", mOptions.mShowFPS);

        generalPropertyWidget->SetIsExpanded("Anim Graph Plugin Properties", true);

        const uint32 numGeneralPropertyWidgetColumns = generalPropertyWidget->columnCount();
        for (uint32 i = 0; i < numGeneralPropertyWidgetColumns; ++i)
        {
            generalPropertyWidget->resizeColumnToContents(i);
        }
    }


    // on preferences change
    void AnimGraphPlugin::OnValueChanged(MysticQt::PropertyWidget::Property* property)
    {
        if (property == mVizScaleProperty)
        {
            const float value = property->AsFloat();
            mOptions.mVisualizeScale = value;

            // update all anim graph instances
            const uint32 numAnimGraphInstances = EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances();
            for (uint32 i = 0; i < numAnimGraphInstances; ++i)
            {
                EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i)->SetVisualizeScale(value);
            }
        }

        if (property == mGraphAnimationProperty)
        {
            const bool value = property->AsBool();
            mOptions.mGraphAnimation = value;
        }

        if (property == mShowFPSProperty)
        {
            const bool value = property->AsBool();
            mOptions.mShowFPS = value;
        }

        SaveOptions();
    }


    // triggered after loading a new layout
    void AnimGraphPlugin::OnAfterLoadLayout()
    {
        // fit graph on screen
        if (mGraphWidget->GetActiveGraph())
        {
            mGraphWidget->GetActiveGraph()->FitGraphOnScreen(mGraphWidget->geometry().width(), mGraphWidget->geometry().height(), mGraphWidget->GetMousePos(), true, false);
        }

        // connect to the timeline recorder data
        TimeViewPlugin* timeViewPlugin = FindTimeViewPlugin();
        if (timeViewPlugin)
        {
            connect(timeViewPlugin, SIGNAL(DoubleClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData*,EMotionFX::Recorder::NodeHistoryItem*)), this, SLOT(OnDoubleClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData*,EMotionFX::Recorder::NodeHistoryItem*)));
            connect(timeViewPlugin, SIGNAL(ClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData*,EMotionFX::Recorder::NodeHistoryItem*)), this, SLOT(OnClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData*,EMotionFX::Recorder::NodeHistoryItem*)));
        }
    }


    void AnimGraphPlugin::FitActiveGraphOnScreen()
    {
        if (mGraphWidget->GetActiveGraph())
        {
            mGraphWidget->GetActiveGraph()->FitGraphOnScreen(mGraphWidget->geometry().width(), mGraphWidget->geometry().height(), mGraphWidget->GetMousePos());
        }
    }


    void AnimGraphPlugin::SelectAll()
    {
        NodeGraph* nodeGraph = mGraphWidget->GetActiveGraph();
        if (nodeGraph == nullptr)
        {
            return;
        }

        nodeGraph->SelectAllNodes(true);
    }


    void AnimGraphPlugin::UnselectAll()
    {
        NodeGraph* nodeGraph = mGraphWidget->GetActiveGraph();
        if (nodeGraph == nullptr)
        {
            return;
        }

        nodeGraph->UnselectAllNodes(true);
    }


    void AnimGraphPlugin::FitActiveSelectionOnScreen()
    {
        NodeGraph* nodeGraph = mGraphWidget->GetActiveGraph();
        if (nodeGraph == nullptr)
        {
            return;
        }

        // try zooming on the selection rect
        const QRect selectionRect = nodeGraph->CalcRectFromSelection(true);
        if (selectionRect.isEmpty() == false)
        {
            nodeGraph->ZoomOnRect(selectionRect, mGraphWidget->geometry().width(), mGraphWidget->geometry().height());
        }
        else // zoom on the full scene
        {
            nodeGraph->FitGraphOnScreen(mGraphWidget->geometry().width(), mGraphWidget->geometry().height(), mGraphWidget->GetMousePos());
        }
    }


    // sync the visual node linked to a emfx node
    void AnimGraphPlugin::SyncVisualNode(EMotionFX::AnimGraphNode* node)
    {
        // get the active anim graph
        EMotionFX::AnimGraph* animGraph = GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // find the graph
        EMotionFX::AnimGraphNode* parentNode = node->GetParentNode();
        if (parentNode == nullptr)
        {
            return;
        }

        NodeGraph* graph = FindGraphForNode(node->GetParentNode()->GetID(), animGraph);
        if (graph == nullptr)
        {
            return;
        }

        // find the node
        GraphNode* graphNode = graph->FindNodeByID(node->GetID());
        if (graphNode == nullptr)
        {
            return;
        }

        // if it's a blend graph node, sync it
        graphNode->Sync();
        /*  if (graphNode->GetType() == BlendTreeVisualNode::TYPE_ID)
            {
                BlendTreeVisualNode* blendNode = static_cast<BlendTreeVisualNode*>( graphNode );
                blendNode->Sync();
            }*/
    }


    // sync all visible nodes
    void AnimGraphPlugin::SyncVisualNodes()
    {
        // get the active graph
        NodeGraph* graph = mGraphWidget->GetActiveGraph();
        if (graph == nullptr)
        {
            return;
        }

        // get the number of nodes and iterate through them
        uint32 numNodes = graph->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // sync the node
            GraphNode* graphNode = graph->GetNode(i);
            graphNode->Sync();
        }
    }


    // sync all visible transitions
    void AnimGraphPlugin::SyncVisualTransitions()
    {
        // get the active graph
        NodeGraph* graph = mGraphWidget->GetActiveGraph();
        if (graph == nullptr)
        {
            return;
        }

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // find the graph info for the graph
        const uint32 graphInfoIndex = FindGraphInfo(graph, GetActiveAnimGraph());
        if (graphInfoIndex == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // find the emfx node
        GraphInfo* graphInfo = mGraphInfos[graphInfoIndex];
        EMotionFX::AnimGraphNode* emfxNode = animGraph->RecursiveFindNodeByID(graphInfo->mNodeID);
        if (emfxNode == nullptr)
        {
            return;
        }

        // skip in case this is not a state machine
        if (emfxNode->GetType() != EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            return;
        }

        // type cast it to a state machine
        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(emfxNode);

        // get the number of transitions and iterate through them
        const uint32 numTransitions = stateMachine->GetNumTransitions();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            SyncTransition(stateMachine->GetTransition(i));
        }
    }


    void AnimGraphPlugin::SyncTransition(EMotionFX::AnimGraphStateTransition* transition, StateConnection* visualTransition)
    {
        if (transition == nullptr || visualTransition == nullptr)
        {
            return;
        }

        // sync the transition
        //transition->OnUpdateAttributes();

        // find the visual representation object for the state transition
        if (visualTransition)
        {
            visualTransition->SetIsDisabled(transition->GetIsDisabled());
            visualTransition->SetIsSynced(transition->GetSyncMode() != EMotionFX::AnimGraphObject::SYNCMODE_DISABLED);
        }
    }


    // sync a transition
    void AnimGraphPlugin::SyncTransition(EMotionFX::AnimGraphStateTransition* transition)
    {
        if (transition == nullptr)
        {
            return;
        }

        // find the visual representation object for the state transition
        StateConnection* visualTransition = FindStateConnection(transition);
        SyncTransition(transition, visualTransition);
    }


    // init for a given anim graph
    void AnimGraphPlugin::InitForAnimGraph(EMotionFX::AnimGraph* setup)
    {
        // if the anim graph is nullptr there is nothing left to do
        if (setup == nullptr)
        {
            // set the current anim graph history to nullptr
            // it has to be set firstly to have the history buttons updated correctly
            mCurrentAnimGraphHistory = nullptr;

            // show nothing
            mGraphWidget->ShowNothing();
            mNavigateWidget->UpdateTreeWidget(nullptr);

            // update anim graph widgets
            mPaletteWidget->Init(nullptr, nullptr);
            mParameterWindow->Init();
            mNodeGroupWindow->Init();
            mViewWidget->Update();
    #ifdef HAS_GAME_CONTROLLER
            mGameControllerWindow->ReInit();
    #endif

            // update the parameter window name column
            mParameterWindow->ResetNameColumnWidth();

            // done
            return;
        }

        // find the current anim graph history or create if not found
        mCurrentAnimGraphHistory = nullptr;
        const uint32 numAnimGraphHistories = mAnimGraphHistories.GetLength();
        for (uint32 i = 0; i < numAnimGraphHistories; ++i)
        {
            if (mAnimGraphHistories[i]->mAnimGraph == setup)
            {
                mCurrentAnimGraphHistory = mAnimGraphHistories[i];
                break;
            }
        }
        if (mCurrentAnimGraphHistory == nullptr)
        {
            // add the new anim graph history
            // add the root state machine as first history
            AnimGraphHistory* newAnimGraphHistory = new AnimGraphHistory();
            newAnimGraphHistory->mAnimGraph = setup;
            newAnimGraphHistory->mHistory.Add(HistoryItem(setup->GetRootStateMachine()->GetName(), setup));
            mAnimGraphHistories.Add(newAnimGraphHistory);

            // set the current anim graph history
            mCurrentAnimGraphHistory = newAnimGraphHistory;
        }

        // get the node to show
        // if the anim graph is not found, the root state machine will be shown
        EMotionFX::AnimGraphNode* nodeToShow = nullptr;
        const uint32 numCurrentVisibleNodeOnAnimGraphs = mCurrentVisibleNodeOnAnimGraphs.GetLength();
        for (uint32 i = 0; i < numCurrentVisibleNodeOnAnimGraphs; ++i)
        {
            if (mCurrentVisibleNodeOnAnimGraphs[i]->mAnimGraph == setup)
            {
                nodeToShow = mCurrentVisibleNodeOnAnimGraphs[i]->mNode;
                break;
            }
        }
        if (nodeToShow == nullptr)
        {
            nodeToShow = setup->GetRootStateMachine();
        }

        // init the navigation widget
        mNavigateWidget->UpdateTreeWidget(setup, nodeToShow);
        mNavigateWidget->OnSelectionChanged();

        // update the widgets
        mNavigateWidget->update();

        // Not needed because ShowGraph function already init it
        // mPaletteWidget->Init( setup, nodeToShow );

        // update anim graph widgets
        mParameterWindow->Init();
        mNodeGroupWindow->Init();
        mViewWidget->Update();
        mAttributesWindow->InitForAnimGraphObject(nodeToShow);
#ifdef HAS_GAME_CONTROLLER
        mGameControllerWindow->ReInit();
#endif

        // update the parameter window name column
        mParameterWindow->ResizeNameColumnToContents();
    }


    // constructor
    AnimGraphEventHandler::AnimGraphEventHandler(AnimGraphPlugin* plugin)
        : EMotionFX::EventHandler()
    {
        mPlugin = plugin;
    }


    void AnimGraphEventHandler::OnDeleteAnimGraph(EMotionFX::AnimGraph* animGraph)
    {
        if (animGraph == nullptr)
        {
            return;
        }

        OutlinerManager* manager = GetOutlinerManager();
        if (manager == nullptr)
        {
            return;
        }

        manager->RemoveItemFromCategory("Anim Graphs", animGraph->GetID());
    }

    /*
    // color a given state
    void AnimGraphEventHandler::ColorState(AnimGraphPlugin* plugin, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* state, const QColor& color)
    {
        if (state == nullptr)
            return;

        // find the visual graph and visual node
        NodeGraph* visualGraph;
        GraphNode* graphNode;
        if (plugin->FindGraphAndNode(state->GetParentNode(), state->GetID(), &visualGraph, &graphNode, animGraph))
        {
            graphNode->SetBorderColor( color );
            //plugin->GetGraphWidget()->update();
            //LogError("Coloring node '%s' in RGB(%i, %i, %i).", state->GetName(), color.red(), color.green(), color.blue());
        }
    }


    // color a given state
    void AnimGraphEventHandler::ColorState(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* state, const QColor& color)
    {
        ColorState(mPlugin, animGraph, state, color);
    }
    */
    /*
    // color a transition
    void AnimGraphEventHandler::SetTransitionActive(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphStateTransition* transition, bool active)
    {
        // get the visual transition object and return directly in case we can't find it
        StateConnection* visualTransition = mPlugin->FindStateConnection(transition);
        if (visualTransition == nullptr)
            return;

        visualTransition->SetIsTransitioning(active);
        //mPlugin->GetGraphWidget()->update();
    }


    void AnimGraphEventHandler::ResetStateColors(NodeGraph* visualGraph, const QColor& color)
    {
        const uint32 numStates = visualGraph->GetNumNodes();
        for (uint32 i=0; i<numStates; ++i)
            visualGraph->GetNode(i)->SetBorderColor(color);
    }


    void AnimGraphEventHandler::ResetTransitions(NodeGraph* visualGraph)
    {
        // get the number of nodes and iterate through them
        const uint32 numStates = visualGraph->GetNumNodes();
        for (uint32 i=0; i<numStates; ++i)
        {
            // get the current node
            GraphNode* node = visualGraph->GetNode(i);

            // get the number of connections and iterate through them
            const uint32 numConnections = node->GetNumConnections();
            for (uint32 j=0; j<numConnections; ++j)
            {
                NodeConnection* connection = node->GetConnection(j);
                connection->SetIsTransitioning( false );
            }
        }
    }
    */

    void AnimGraphEventHandler::Update(EMotionFX::AnimGraphStateMachine* stateMachine, EMotionFX::AnimGraphInstance* animGraphInstance, NodeGraph* visualGraph)
    {
        MCORE_UNUSED(visualGraph);

        // make sure the state machine node belongs to the same anim graph than the anim graph instance does
        if (stateMachine->GetAnimGraph() != animGraphInstance->GetAnimGraph())
        {
            return;
        }

        // reset the colors
        //  ResetStateColors( visualGraph, GetNeutralStateColor() );
        //AnimGraphEventHandler::ResetTransitions( visualGraph );

        // sync all transitions
        const uint32 numTransitions = stateMachine->GetNumTransitions();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            mPlugin->SyncTransition(stateMachine->GetTransition(i));
        }
        /*
            // color the currently fully active state
            EMotionFX::AnimGraphNode* currentState = stateMachine->GetCurrentState(animGraphInstance);
            if (currentState)
                ColorState( mPlugin, stateMachine->GetAnimGraph(), currentState, GetActiveColor() );
        */
        /*  // mark the current transition as active
            EMotionFX::AnimGraphStateTransition* currentTransition = stateMachine->GetActiveTransition(animGraphInstance);
            if (currentTransition)
            {
                // color the active transition
                SetTransitionActive( stateMachine->GetAnimGraph(), currentTransition, true );

                // check if the target node is valid and also color that one
                EMotionFX::AnimGraphNode* targetState = currentTransition->GetTargetNode();
                if (targetState)
                    ColorState( mPlugin, stateMachine->GetAnimGraph(), targetState, GetTransitioningColor() );
            }   */
    }

    /*
    void AnimGraphEventHandler::OnStateEnter(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state)                       { ColorState( animGraphInstance->GetAnimGraph(), state, GetActiveColor() ); }
    void AnimGraphEventHandler::OnStateEntering(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state)                    { ColorState( animGraphInstance->GetAnimGraph(), state, GetTransitioningColor() ); }
    void AnimGraphEventHandler::OnStateExit(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state)                        { ColorState( animGraphInstance->GetAnimGraph(), state, GetNeutralStateColor() ); }
    void AnimGraphEventHandler::OnStateEnd(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* state)                         { ColorState( animGraphInstance->GetAnimGraph(), state, GetNeutralStateColor() ); }
    */
    /*
    void AnimGraphEventHandler::OnStartTransition(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphStateTransition* transition)  { SetTransitionActive(animGraphInstance->GetAnimGraph(), transition, true); }
    void AnimGraphEventHandler::OnEndTransition(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphStateTransition* transition)
    {
        //if (transition)
            //ColorState( animGraphInstance->GetAnimGraph(), transition->GetSourceNode(animGraphInstance), GetNeutralStateColor() );

        SetTransitionActive(animGraphInstance->GetAnimGraph(), transition, false);
    }
    */


    // when we create a new node map it to our new port change function
    void AnimGraphEventHandler::OnCreatedNode(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node)
    {
        MCORE_UNUSED(animGraph);
        using AZStd::placeholders::_1;
        using AZStd::placeholders::_2;
        auto func = AZStd::bind(&EMStudio::AnimGraphPlugin::OnAnimGraphNodeInputPortsChanged, mPlugin, _1, _2);
        node->SetInputPortChangeFunction(func);
    }


    //
    bool AnimGraphEventHandler::OnRayIntersectionTest(const AZ::Vector3& start, const AZ::Vector3& end, EMotionFX::IntersectionInfo* outIntersectInfo)
    {
        outIntersectInfo->mIsValid = true;

        AZ::Vector3 pos;
        AZ::Vector3 normal;
        AZ::Vector2 uv(0.0f, 0.0f);
        float baryU;
        float baryV;
        uint32 startIndex;
        bool first = true;
        bool result = false;
        float closestDist = FLT_MAX;

        MCore::Ray ray(start, end);

        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            if (actorInstance == outIntersectInfo->mIgnoreActorInstance)
            {
                continue;
            }

            if (actorInstance->IntersectsMesh(0, ray, &pos, &normal, &uv, &baryU, &baryV, &startIndex) == nullptr)
            {
                continue;
            }

            if (first)
            {
                outIntersectInfo->mPosition     = pos;
                outIntersectInfo->mNormal       = normal;
                outIntersectInfo->mUV           = uv;
                outIntersectInfo->mBaryCentricU = baryU;
                outIntersectInfo->mBaryCentricV = baryU;
                closestDist = MCore::SafeLength(start - pos);
            }
            else
            {
                float dist = MCore::SafeLength(start - pos);
                if (dist < closestDist)
                {
                    outIntersectInfo->mPosition     = pos;
                    outIntersectInfo->mNormal       = normal;
                    outIntersectInfo->mUV           = uv;
                    outIntersectInfo->mBaryCentricU = baryU;
                    outIntersectInfo->mBaryCentricV = baryU;
                    closestDist = MCore::SafeLength(start - pos);
                    closestDist = dist;
                }
            }

            first   = false;
            result  = true;
        }

        /*
            // collide with ground plane
            MCore::Vector3 groundNormal(0.0f, 0.0f, 0.0f);
            groundNormal[MCore::GetCoordinateSystem().GetUpIndex()] = 1.0f;
            MCore::PlaneEq groundPlane( groundNormal, Vector3(0.0f, 0.0f, 0.0f) );
            bool result = MCore::Ray(start, end).Intersects( groundPlane, &(outIntersectInfo->mPosition) );
            outIntersectInfo->mNormal = groundNormal;
        */
        return result;
    }


    void AnimGraphEventHandler::Sync(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphNode* animGraphNode)
    {
        NodeGraph* visualGraph;
        GraphNode* graphNode;
        if (mPlugin->FindGraphAndNode(animGraphNode->GetParentNode(), animGraphNode->GetID(), &visualGraph, &graphNode, animGraphInstance->GetAnimGraph(), false))
        {
            graphNode->Sync();
            //mPlugin->GetGraphWidget()->update();
        }
    }


    // set the gizmo offsets
    void AnimGraphEventHandler::OnSetVisualManipulatorOffset(EMotionFX::AnimGraphInstance* animGraphInstance, uint32 paramIndex, const AZ::Vector3& offset)
    {
        EMStudioManager* manager = GetManager();

        // get the paremeter name
        const MCore::String& paramName = animGraphInstance->GetAnimGraph()->GetParameter(paramIndex)->GetNameString();

        // iterate over all gizmos that are active
        MCore::Array<MCommon::TransformationManipulator*>* gizmos = manager->GetTransformationManipulators();
        const uint32 numGizmos = gizmos->GetLength();
        for (uint32 i = 0; i < numGizmos; ++i)
        {
            MCommon::TransformationManipulator* gizmo = gizmos->GetItem(i);

            // check the gizmo name
            if (paramName == gizmo->GetName())
            {
                gizmo->SetRenderOffset(offset);
                return;
            }
        }
    }


    void AnimGraphEventHandler::OnParameterNodeMaskChanged(EMotionFX::BlendTreeParameterNode* parameterNode)
    {
        uint32 i;
        MCore::String               commandResult;
        MCore::CommandGroup         commandGroup("Adjust parameter node mask");
        EMotionFX::AnimGraph*      animGraph  = parameterNode->GetAnimGraph();
        EMotionFX::AnimGraphNode*  parentNode  = parameterNode->GetParentNode();

        // get the parameter mask attribute and the number of parameters inside it
        EMotionFX::AttributeParameterMask* parameterMaskAttribute = static_cast<EMotionFX::AttributeParameterMask*>(parameterNode->GetAttribute(EMotionFX::BlendTreeParameterNode::ATTRIB_MASK));
        //const uint32 numParameters = parameterMaskAttribute->GetNumParameterNames();

        /////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 1: Remember the connections outgoing the parameter node
        /////////////////////////////////////////////////////////////////////////////////////////////
        AZStd::vector<CommandSystem::ParameterConnectionItem> oldParameterConnections;
        oldParameterConnections.reserve(animGraph->GetNumParameters());

        // iterate through all nodes and check if any of these is connected to our parameter node
        const uint32 numNodes = parentNode->GetNumChildNodes();
        for (i = 0; i < numNodes; ++i)
        {
            // get the child node and skip it in case it is the parameter node itself
            EMotionFX::AnimGraphNode* childNode = parentNode->GetChildNode(i);
            if (childNode == parameterNode)
            {
                continue;
            }

            // get the number of incoming connections and iterate through them
            const uint32 numConnections = childNode->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                // get the connection and check if it is plugged into the parameter node
                EMotionFX::BlendTreeConnection* connection = childNode->GetConnection(c);
                if (connection->GetSourceNode() == parameterNode)
                {
                    const uint16 sourcePort     = connection->GetSourcePort();
                    const uint32 parameterIndex = parameterNode->GetParameterIndex(sourcePort);

                    if (parameterIndex != MCORE_INVALIDINDEX32)
                    {
                        CommandSystem::ParameterConnectionItem connectionItem;
                        connectionItem.SetParameterNodeName(parameterNode->GetName());
                        connectionItem.SetTargetNodeName(childNode->GetName());
                        connectionItem.SetParameterName(animGraph->GetParameter(parameterIndex)->GetName());
                        connectionItem.mTargetNodePort = connection->GetTargetPort();
                        oldParameterConnections.push_back(connectionItem);
                    }
                }
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 2: Removing all connections from the parameter node
        /////////////////////////////////////////////////////////////////////////////////////////////
        AZStd::vector<EMotionFX::BlendTreeConnection*> connectionList;
        CommandSystem::DeleteNodeConnections(&commandGroup, parameterNode, parentNode, connectionList, false);

        /////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 3: Reiniting the parameter node using the new parameter mask
        /////////////////////////////////////////////////////////////////////////////////////////////
        AZStd::string parameterMaskString;
        CommandSystem::ConstructParameterMaskString(parameterMaskString, animGraph, parameterMaskAttribute);
        const AZStd::string commandString = AZStd::string::format("AnimGraphAdjustNode -animGraphID %i -name \"%s\" -parameterMask \"%s\"", animGraph->GetID(), parameterNode->GetName(), parameterMaskString.c_str());
        commandGroup.AddCommandString(commandString);

        /////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 4: Recreate the connections at the new ports
        /////////////////////////////////////////////////////////////////////////////////////////////
        AZStd::vector<AZStd::string> parametersToBeRemoved;
        RecreateOldConnections(animGraph, oldParameterConnections, &commandGroup, parametersToBeRemoved);

        // execute the command group
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, commandResult, false) == false)
        {
            if (commandResult.GetIsEmpty() == false)
            {
                MCore::LogError(commandResult.AsChar());
            }
        }

        mPlugin->GetAttributesWindow()->mObject = parameterNode;
        mPlugin->GetAttributesWindow()->mAttributeLinks.Clear();
    }


    // will be called in case a condition triggers
    void AnimGraphEventHandler::OnConditionTriggered(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphTransitionCondition* condition)
    {
        MCORE_UNUSED(animGraphInstance);
        MCORE_UNUSED(condition);

        // redraw
        //mPlugin->GetGraphWidget()->update();
    }


    // activate a given anim graph
    void AnimGraphPlugin::SetActiveAnimGraph(EMotionFX::AnimGraph* animGraph)
    {
        if (mActiveAnimGraph != animGraph)
        {
            mActiveAnimGraph = animGraph;
            InitForAnimGraph(animGraph);
        }
    }


    // find a graph info by tree
    uint32 AnimGraphPlugin::FindGraphInfo(uint32 nodeID, EMotionFX::AnimGraph* animGraph)
    {
        const uint32 numInfos = mGraphInfos.GetLength();
        for (uint32 i = 0; i < numInfos; ++i)
        {
            if (mGraphInfos[i]->mNodeID == nodeID && mGraphInfos[i]->mAnimGraph == animGraph)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find the graph info for a given visual graph
    uint32 AnimGraphPlugin::FindGraphInfo(NodeGraph* graph, EMotionFX::AnimGraph* animGraph)
    {
        const uint32 numInfos = mGraphInfos.GetLength();
        for (uint32 i = 0; i < numInfos; ++i)
        {
            if (mGraphInfos[i]->mVisualGraph == graph && mGraphInfos[i]->mAnimGraph == animGraph)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // find the graph for a given node
    NodeGraph* AnimGraphPlugin::FindGraphForNode(uint32 nodeID, EMotionFX::AnimGraph* animGraph)
    {
        //  EMotionFX::AnimGraph* animGraph = GetActiveAnimGraph();
        //  assert( animGraph );

        // find the graph
        uint32 graphIndex = FindGraphInfo(nodeID, animGraph);

        // it doesn't exist yet, so create it
        if (graphIndex == MCORE_INVALIDINDEX32)
        {
            // create the graph info
            GraphInfo* newInfo = new GraphInfo();
            newInfo->mNodeID = nodeID;
            newInfo->mAnimGraph = animGraph;
            mGraphInfos.Add(newInfo);
            newInfo->mVisualGraph = CreateGraph(animGraph, animGraph->RecursiveFindNodeByID(nodeID));

            graphIndex = mGraphInfos.GetLength() - 1;
        }

        // show it
        NodeGraph* graph = mGraphInfos[graphIndex]->mVisualGraph;
        if (graph)
        {
            graph->SetScalePivot(QPoint(mGraphWidget->geometry().width() / 2, mGraphWidget->geometry().height() / 2));
        }

        return graph;
    }


    // change the node ID inside a given graph
    void AnimGraphPlugin::RenameGraph(uint32 oldID, uint32 newID, EMotionFX::AnimGraph* animGraph)
    {
        // find the graph
        uint32 graphIndex = FindGraphInfo(oldID, animGraph);

        // rename the item in the tree view
        mNavigateWidget->Rename(MCore::GetStringIDGenerator().GetName(oldID).AsChar(), MCore::GetStringIDGenerator().GetName(newID).AsChar());

        // it doesn't exist yet, so create it
        if (graphIndex == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // update the ID
        mGraphInfos[graphIndex]->mNodeID = newID;
    }


    void AnimGraphPlugin::RemoveAllUnusedGraphInfos()
    {
        // iterate over all graph infos
        for (uint32 i = 0; i < mGraphInfos.GetLength(); )
        {
            // get the anim graph the graph info belongs to and try to find the index in the anim graph manager
            GraphInfo*              graphInfo       = mGraphInfos[i];
            EMotionFX::AnimGraph*  animGraph      = graphInfo->mAnimGraph;
            const uint32            animGraphIndex = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph);

            // if the anim graph is invalid, get rid of it
            if (animGraphIndex == MCORE_INVALIDINDEX32)
            {
                if (mGraphWidget->GetActiveGraph() == graphInfo->mVisualGraph)
                {
                    mGraphWidget->ShowNothing();
                }

                delete graphInfo;
                mGraphInfos.Remove(i);
            }
            else
            {
                ++i;
            }
        }

        // iterate over all current visible node on anim graphs
        for (uint32 i = 0; i < mCurrentVisibleNodeOnAnimGraphs.GetLength(); )
        {
            AnimGraphWithNode*     animGraphWithNode  = mCurrentVisibleNodeOnAnimGraphs[i];
            EMotionFX::AnimGraph*  animGraph          = animGraphWithNode->mAnimGraph;
            const uint32            animGraphIndex     = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph);

            // if the anim graph is invalid, get rid of it
            if (animGraphIndex == MCORE_INVALIDINDEX32)
            {
                delete animGraphWithNode;
                mCurrentVisibleNodeOnAnimGraphs.Remove(i);
            }
            else
            {
                ++i;
            }
        }

        // iterate over all anim graph histories
        for (uint32 i = 0; i < mAnimGraphHistories.GetLength(); )
        {
            AnimGraphHistory*      animGraphHistory   = mAnimGraphHistories[i];
            EMotionFX::AnimGraph*  animGraph          = animGraphHistory->mAnimGraph;
            const uint32            animGraphIndex     = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph);

            // if the anim graph is invalid, get rid of it
            if (animGraphIndex == MCORE_INVALIDINDEX32)
            {
                delete animGraphHistory;
                mAnimGraphHistories.Remove(i);
            }
            else
            {
                ++i;
            }
        }
    }


    // try to find a given blend node in a given visual graph
    GraphNode* AnimGraphPlugin::FindGraphNode(NodeGraph* graph, uint32 nodeID, EMotionFX::AnimGraph* animGraph)
    {
        MCORE_UNUSED(animGraph);

        if (graph == nullptr)
        {
            return nullptr;
        }

        // iterate through all nodes
        const uint32 numNodes = graph->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode* graphNode = graph->GetNode(i);

            if (graphNode->GetID() == nodeID)
            {
                return graphNode;
            }

            /*
                    // check blend nodes
                    if (graphNode->GetType() == BlendTreeVisualNode::TYPE_ID)
                    {
                        BlendTreeVisualNode* blendTreeVisualNode = (BlendTreeVisualNode*)graphNode;
                        if (blendTreeVisualNode->GetEMFXNode()->GetID() == nodeID)
                            return blendTreeVisualNode;
                    }

                    // check state nodes
                    if (graphNode->GetType() == StateGraphNode::TYPE_ID)
                    {
                        StateGraphNode* stateGraphNode = (StateGraphNode*)graphNode;
                        if (stateGraphNode->GetEMFXNode()->GetID() == nodeID)
                            return graphNode;
                    }
                    */
        }

        return nullptr;
    }


    void AnimGraphPlugin::UpdateStateMachineColors()
    {
        NodeGraph*                  activeGraph = mGraphWidget->GetActiveGraph();
        EMotionFX::AnimGraphNode*  currentNode = mGraphWidget->GetCurrentNode();

        // reset the graph node and transition colors
        if (currentNode && activeGraph && currentNode->GetType() == EMotionFX::AnimGraphStateMachine::TYPE_ID)
        {
            EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(currentNode);

            EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
            if (actorInstance)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
                if (mEventHandler && animGraphInstance)
                {
                    mEventHandler->Update(stateMachine, animGraphInstance, activeGraph);
                }
            }
        }
    }


    // show the graph for a given node
    void AnimGraphPlugin::ShowGraph(EMotionFX::AnimGraphNode* node, EMotionFX::AnimGraph* animGraph, bool addToHistory)
    {
        mViewWidget->SetCurrentNode(node);

        // get the node ID
        const uint32 nodeID = (node == nullptr) ? MCORE_INVALIDINDEX32 : node->GetID();

        // find the graph
        uint32 graphIndex = FindGraphInfo(nodeID, animGraph);

        // it doesn't exist yet, so create it
        if (graphIndex == MCORE_INVALIDINDEX32)
        {
            // create the graph info
            GraphInfo* newInfo = new GraphInfo();
            newInfo->mNodeID = nodeID;
            newInfo->mAnimGraph = animGraph;
            newInfo->mVisualGraph = CreateGraph(animGraph, node);
            mGraphInfos.Add(newInfo);
            graphIndex = mGraphInfos.GetLength() - 1;

            // init the virtual final node
            if (node && node->GetType() == EMotionFX::BlendTree::TYPE_ID)
            {
                EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(node);
                if (blendTree->GetVirtualFinalNode())
                {
                    SetVirtualFinalNode(blendTree->GetVirtualFinalNode());
                }
            }
        }

        // add to history if needed
        if (addToHistory)
        {
            PushHistory(node, animGraph);
        }

        // show the graph on the graph widget
        NodeGraph* graph = mGraphInfos[graphIndex]->mVisualGraph;
        if (graph)
        {
            graph->SetScalePivot(QPoint(mGraphWidget->geometry().width() / 2, mGraphWidget->geometry().height() / 2));
            mGraphWidget->SetActiveGraph(graph);
        }

        // set the current node on the graph widget
        mGraphWidget->SetCurrentNode(node);

        // update only if the view widget is valid
        if (mViewWidget)
        {
            mViewWidget->Update();
        }

        // fit it to screen in case this is the first time visualizing it
        if (graph && graph->GetStartFitHappened() == false)
        {
            graph->FitGraphOnScreen(mGraphWidget->geometry().width(), mGraphWidget->geometry().height(), mGraphWidget->GetMousePos(), true, false);
        }

        // clear all processed flags for connections and nodes when showing a blend tree
        ClearAllProcessedFlags(false);

        // sync the visual nodes
        SyncVisualNodes();

        // color the state machine correctly
        UpdateStateMachineColors();

        // update the graph widget
        mGraphWidget->update();

        // init the palette widget
        mPaletteWidget->Init(animGraph, node);

        // keep the current visible node for this anim graph
        uint32 currentVisibleNodeOnAnimGraphsIndex = MCORE_INVALIDINDEX32;
        const uint32 numCurrentVisibleNodeOnAnimGraphs = mCurrentVisibleNodeOnAnimGraphs.GetLength();
        for (uint32 i = 0; i < numCurrentVisibleNodeOnAnimGraphs; ++i)
        {
            if (mCurrentVisibleNodeOnAnimGraphs[i]->mAnimGraph == animGraph)
            {
                currentVisibleNodeOnAnimGraphsIndex = i;
                break;
            }
        }
        if (currentVisibleNodeOnAnimGraphsIndex == MCORE_INVALIDINDEX32)
        {
            AnimGraphWithNode* newAnimGraphWithNode = new AnimGraphWithNode();
            newAnimGraphWithNode->mAnimGraph = animGraph;
            newAnimGraphWithNode->mNode = node;
            mCurrentVisibleNodeOnAnimGraphs.Add(newAnimGraphWithNode);
        }
        else
        {
            mCurrentVisibleNodeOnAnimGraphs[currentVisibleNodeOnAnimGraphsIndex]->mNode = node;
        }
    }


    void AnimGraphPlugin::CleanHistory()
    {
        if (!mCurrentAnimGraphHistory)
        {
            return;
        }
        MCore::Array<HistoryItem>& history = mCurrentAnimGraphHistory->mHistory;

        bool historyModified = false;
        const int32 numHistoryItems = history.GetLength();
        for (int32 i = numHistoryItems - 1; i >= 0; i--)
        {
            const HistoryItem& historyItem = history[i];
            EMotionFX::AnimGraphNode* node = history[i].FindNode();
            const EMotionFX::AnimGraph* animGraph = history[i].mAnimGraph;
            bool deleteHistoryItem = false;

            // Delete the history item in case we are dealing with graphs that already got deleted.
            if (!animGraph || !node)
            {
                deleteHistoryItem = true;
            }
            else
            {
                // Check if the previous or the following history item is a clone of the current one.
                const int32 prevIndex = i - 1;
                if (!deleteHistoryItem && prevIndex >= 0)
                {
                    const EMotionFX::AnimGraphNode* prevNode = history[prevIndex].FindNode();

                    if (prevNode && node == prevNode)
                    {
                        deleteHistoryItem = true;
                    }
                }

                const uint32 nextIndex = i + 1;
                if (!deleteHistoryItem && nextIndex < history.GetLength())
                {
                    const EMotionFX::AnimGraphNode* nextNode = history[nextIndex].FindNode();

                    if (nextNode && node == nextNode)
                    {
                        deleteHistoryItem = true;
                    }
                }
            }

            // Remove the history item in case we are dealing with invalid graphs or duplicated history items.
            if (deleteHistoryItem)
            {
                history.Remove(i);

                // Decrease the current time in history in case we removed a history item before or at the current time.
                if (i <= static_cast<int32>(mCurrentAnimGraphHistory->mHistoryIndex))
                {
                    mCurrentAnimGraphHistory->mHistoryIndex--;
                }

                historyModified = true;
            }
        }

        if (historyModified)
        {
            // Update the step forward and backward buttons in case we modified the history. Maybe they need to be disabled as no history item is left.
            mViewWidget->Update();
        }
    }


    void AnimGraphPlugin::ShowGraphFromHistory(uint32 historyIndex)
    {
        EMotionFX::AnimGraphNode* node = mCurrentAnimGraphHistory->mHistory[historyIndex].FindNode();
        EMotionFX::AnimGraph* animGraph = mCurrentAnimGraphHistory->mHistory[historyIndex].mAnimGraph;

        if (!node || !animGraph)
        {
            return;
        }

        ShowGraph(node, animGraph, false);
    }


    void AnimGraphPlugin::HistoryStepBack()
    {
        // Remove history items of not-existing nodes and update the interface accordingly.
        CleanHistory();

        if (!CanPopHistory())
        {
            return;
        }

        mCurrentAnimGraphHistory->mHistoryIndex--;
        ShowGraphFromHistory(mCurrentAnimGraphHistory->mHistoryIndex);
    }


    void AnimGraphPlugin::HistoryStepForward()
    {
        // Remove history items of not-existing nodes and update the interface accordingly.
        CleanHistory();

        if (!CanStepForwardInHistory())
        {
            return;
        }

        mCurrentAnimGraphHistory->mHistoryIndex++;
        ShowGraphFromHistory(mCurrentAnimGraphHistory->mHistoryIndex);
    }


    void AnimGraphPlugin::PushHistory(EMotionFX::AnimGraphNode* node, EMotionFX::AnimGraph* animGraph)
    {
        //if (node)
        //  MCore::LogInfo("Adding '%s' to anim graph node history.", node->GetName());
        //if (animGraph)
        //  MCore::LogInfo("Adding '%s' to anim graph history.", animGraph->GetName());

        // safety checks
        MCORE_ASSERT(node && animGraph);
        if (node == nullptr || animGraph == nullptr)
        {
            return;
        }

        if (mCurrentAnimGraphHistory->mHistory.GetLength() > 0)
        {
            // check if the last entry is the same as the current one, if yes return
            if (mCurrentAnimGraphHistory->mHistory[mCurrentAnimGraphHistory->mHistory.GetLength() - 1].FindNode() == node && mCurrentAnimGraphHistory->mHistory[mCurrentAnimGraphHistory->mHistory.GetLength() - 1].mAnimGraph == animGraph)
            {
                return;
            }
        }

        // if we reached the maximum number of history entries remove the oldest one
        if (mCurrentAnimGraphHistory->mHistory.GetLength() >= mMaxHistoryEntries)
        {
            PopHistory();
            mCurrentAnimGraphHistory->mHistoryIndex = mCurrentAnimGraphHistory->mHistory.GetLength() - 1;
        }

        if (mCurrentAnimGraphHistory->mHistory.GetLength() > 0)
        {
            // remove unneeded
            uint32 numToRemove = mCurrentAnimGraphHistory->mHistory.GetLength() - mCurrentAnimGraphHistory->mHistoryIndex - 1;
            for (uint32 a = 0; a < numToRemove; ++a)
            {
                mCurrentAnimGraphHistory->mHistory.Remove(mCurrentAnimGraphHistory->mHistoryIndex + 1);
            }
        }

        // add a history entry
        mCurrentAnimGraphHistory->mHistory.Add(HistoryItem(node->GetName(), animGraph));

        // set the history index to the last added entry
        mCurrentAnimGraphHistory->mHistoryIndex = mCurrentAnimGraphHistory->mHistory.GetLength() - 1;
    }


    void AnimGraphPlugin::ReInitGraph(NodeGraph* graph, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node)
    {
        graph->RemoveAllNodes();

        // create nodes that already exist in this graph
        if (node)
        {
            //MCore::LogInfo( "ReInitGraph for node '%s', NumNodes = %i", node->GetName(), node->GetNumChildNodes() );

            uint32 i;
            const uint32 numChildNodes = node->GetNumChildNodes();
            for (i = 0; i < numChildNodes; ++i)
            {
                //MCore::LogInfo( "## Iteration #%i: NumVisualNodes = %i", i, graph->GetNumNodes() );

                EMotionFX::AnimGraphNode* childNode = node->GetChildNode(i);

                // create the new node and add it to the visual graph
                GraphNode* graphNode;
                if (node == nullptr || node->GetType() == EMotionFX::AnimGraphStateMachine::TYPE_ID)
                {
                    graphNode = new StateGraphNode(this, childNode);
                    graph->AddNode(graphNode);
                }
                else
                {
                    graphNode = GetGraphNodeFactory()->CreateGraphNode(childNode->GetType(), childNode->GetName());
                    if (graphNode == nullptr)
                    {
                        BlendTreeVisualNode* blendTreeVisualNode = new BlendTreeVisualNode(this, childNode, false);
                        graph->AddNode(blendTreeVisualNode);
                        blendTreeVisualNode->Sync();
                        blendTreeVisualNode->Update(QRect(std::numeric_limits<int32>::lowest() / 4, std::numeric_limits<int32>::max() / 4, std::numeric_limits<int32>::max() / 2, std::numeric_limits<int32>::max() / 2),  QPoint(std::numeric_limits<int32>::max() / 2, std::numeric_limits<int32>::max() / 2));
                        graphNode = blendTreeVisualNode;
                    }
                    else
                    {
                        graph->AddNode(graphNode);
                    }
                }

                graphNode->SetName(childNode->GetName());
                graphNode->SetDeletable(childNode->GetIsDeletable());
                graphNode->SetBaseColor(childNode->GetVisualColor());
                graphNode->SetHasChildIndicatorColor(childNode->GetHasChildIndicatorColor());
                graphNode->SetIsCollapsed(childNode->GetIsCollapsed());

                const QPoint newPos(childNode->GetVisualPosX(), childNode->GetVisualPosY());
                graphNode->MoveAbsolute(newPos);
                graphNode->SetID(childNode->GetID());
            }

            // create the connections for each child node
            for (i = 0; i < numChildNodes; ++i)
            {
                EMotionFX::AnimGraphNode* childNode = node->GetChildNode(i);

                // for all connections for this node
                const uint32 numConnections = childNode->GetNumConnections();
                for (uint32 c = 0; c < numConnections; ++c)
                {
                    EMotionFX::BlendTreeConnection* connection = childNode->GetConnection(c);
                    GraphNode* source = graph->FindNodeByID(connection->GetSourceNode()->GetID());
                    GraphNode* target = graph->FindNodeByID(childNode->GetID());
                    const uint32 sourcePort = connection->GetSourcePort();
                    const uint32 targetPort = connection->GetTargetPort();

                    NodeConnection* visualConnection = new NodeConnection(target, targetPort, source, sourcePort);
                    target->AddConnection(visualConnection);
                }
            }

            // create visual state transitions in case this graph is for a state machine
            if (node->GetType() == EMotionFX::AnimGraphStateMachine::TYPE_ID)
            {
                EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(node);
                const uint32 numTransitions = stateMachine->GetNumTransitions();
                for (i = 0; i < numTransitions; ++i)
                {
                    EMotionFX::AnimGraphStateTransition* transition = stateMachine->GetTransition(i);

                    // get the source and target nodes
                    GraphNode* source = nullptr;
                    if (transition->GetSourceNode())
                    {
                        source = graph->FindNodeByID(transition->GetSourceNode()->GetID());
                    }
                    GraphNode* target = graph->FindNodeByID(transition->GetTargetNode()->GetID());

                    const QPoint startOffset(transition->GetVisualStartOffsetX(), transition->GetVisualStartOffsetY());
                    const QPoint endOffset(transition->GetVisualEndOffsetX(), transition->GetVisualEndOffsetY());
                    StateConnection* connection = new StateConnection(stateMachine, source, target, startOffset, endOffset, transition->GetIsWildcardTransition(), transition->GetID());
                    target->AddConnection(connection);

                    // sync the visual transition with the emfx one
                    SyncTransition(transition, connection);
                }

                // set the entry state
                EMotionFX::AnimGraphNode* emfxEntryNode = stateMachine->GetEntryState();
                if (emfxEntryNode == nullptr)
                {
                    graph->SetEntryNode(nullptr);
                }
                else
                {
                    GraphNode* entryNode = graph->FindNodeByID(emfxEntryNode->GetID());
                    graph->SetEntryNode(entryNode);
                }
            }
        }
        else
        {
            EMotionFX::AnimGraphStateMachine* rootSM = animGraph->GetRootStateMachine();

            // create the new node and add it to the visual graph
            GraphNode* graphNode = new StateGraphNode(this, rootSM);

            graphNode->SetDeletable(rootSM->GetIsDeletable());
            graphNode->SetBaseColor(rootSM->GetVisualColor());
            graphNode->SetHasChildIndicatorColor(rootSM->GetHasChildIndicatorColor());

            const QPoint newPos(rootSM->GetVisualPosX(), rootSM->GetVisualPosY());
            graphNode->MoveAbsolute(newPos);
            graphNode->SetID(rootSM->GetID());
            graphNode->SetParentGraph(graph);
            graphNode->SetIsCollapsed(rootSM->GetIsCollapsed());
            graph->AddNode(graphNode);
        }

        graph->SetScalePivot(QPoint(mGraphWidget->geometry().width() / 2, mGraphWidget->geometry().height() / 2));
        graph->FitGraphOnScreen(mGraphWidget->geometry().width(), mGraphWidget->geometry().height(), QPoint(0, 0), true, false);
    }


    // create the graph for a given node
    NodeGraph* AnimGraphPlugin::CreateGraph(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node)
    {
        NodeGraph* graph = new NodeGraph(mGraphWidget);
        ReInitGraph(graph, animGraph, node);
        return graph;
    }


    // remove the graph for a given node
    void AnimGraphPlugin::RemoveGraphForNode(uint32 nodeID, EMotionFX::AnimGraph* animGraph)
    {
        MCORE_UNUSED(animGraph);
        const uint32 numInfos = mGraphInfos.GetLength();
        for (uint32 i = 0; i < numInfos; ++i)
        {
            if (mGraphInfos[i]->mNodeID == nodeID)
            {
                // TODO: make sure the active graph isn't the one we're deleting
                if (mGraphWidget->GetActiveGraph() == mGraphInfos[i]->mVisualGraph)
                {
                    mGraphWidget->ShowNothing();
                }

                delete mGraphInfos[i];
                mGraphInfos.Remove(i);
                return;
            }
        }
    }


    StateConnection* AnimGraphPlugin::FindStateConnection(EMotionFX::AnimGraphStateTransition* transition)
    {
        if (transition == nullptr)
        {
            return nullptr;
        }

        // find the visual graph and visual node
        NodeGraph* visualGraph;
        GraphNode* sourceGraphNode;
        EMotionFX::AnimGraphNode* sourceNode = transition->GetSourceNode();
        EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();

        // get the corresponding anim graph
        EMotionFX::AnimGraph* animGraph = targetNode->GetAnimGraph();
        MCORE_ASSERT(animGraph);

        // check if we are dealing with a wildcard transition
        if (transition->GetIsWildcardTransition())
        {
            sourceNode = nullptr;
        }

        if (sourceNode)
        {
            if (FindGraphAndNode(sourceNode->GetParentNode(), sourceNode->GetID(), &visualGraph, &sourceGraphNode, animGraph))
            {
                GraphNode* targetGraphNode = FindGraphNode(visualGraph, targetNode->GetID(), animGraph);
                NodeConnection* connection = targetGraphNode->FindConnectionByID(transition->GetID());
                return static_cast<StateConnection*>(connection);
            }
        }
        else
        {
            if (FindGraphAndNode(targetNode->GetParentNode(), targetNode->GetID(), &visualGraph, &sourceGraphNode, animGraph))
            {
                GraphNode* targetGraphNode = FindGraphNode(visualGraph, transition->GetTargetNode()->GetID(), animGraph);
                NodeConnection* connection = targetGraphNode->FindConnectionByID(transition->GetID());
                return static_cast<StateConnection*>(connection);
            }
        }

        return nullptr;
    }


    // find the visual graph node for a given emfx node
    bool AnimGraphPlugin::FindGraphAndNode(EMotionFX::AnimGraphNode* parentNode, uint32 emfxNodeID, NodeGraph** outVisualGraph, GraphNode** outGraphNode, EMotionFX::AnimGraph* animGraph, bool logErrorWhenNotFound)
    {
        if (animGraph && animGraph->GetIsOwnedByRuntime())
        {
            return false;
        }

        // try to find the visual graph related to this node
        if (parentNode)
        {
            *outVisualGraph = FindGraphForNode(parentNode->GetID(), animGraph);
        }
        else
        {
            *outVisualGraph = FindGraphForNode(MCORE_INVALIDINDEX32, animGraph);
        }

        // now find the node and remove it
        *outGraphNode = FindGraphNode(*outVisualGraph, emfxNodeID, animGraph);
        if (*outGraphNode == nullptr)
        {
            if (logErrorWhenNotFound)
            {
                MCore::LogError("AnimGraphPlugin::FindGraphAndNode() - There is no visual node associated with EMotion FX node '%s' in graph of node '%s'", MCore::GetStringIDGenerator().GetName(emfxNodeID).AsChar(), parentNode ? parentNode->GetName() : "<no parent>");
            }
            //else
            //MCore::LogError("AnimGraphPlugin::FindGraphAndNode() - There is no visual node associated with EMotion FX node '%s' in the root graph.", MCore::GetStringIDGenerator().GetName(emfxNodeID).AsChar());

            return false;
        }

        return true;
    }


    // remove all graphs
    void AnimGraphPlugin::RemoveAllGraphs()
    {
        const uint32 numGraphs = mGraphInfos.GetLength();
        for (uint32 i = 0; i < numGraphs; ++i)
        {
            delete mGraphInfos[i];
        }

        mGraphInfos.Clear();

        const uint32 numCurrentVisibleNodeOnAnimGraphs = mCurrentVisibleNodeOnAnimGraphs.GetLength();
        for (uint32 i = 0; i < numCurrentVisibleNodeOnAnimGraphs; ++i)
        {
            delete mCurrentVisibleNodeOnAnimGraphs[i];
        }

        mCurrentVisibleNodeOnAnimGraphs.Clear();

        const uint32 numAnimGraphHistories = mAnimGraphHistories.GetLength();
        for (uint32 i = 0; i < numAnimGraphHistories; ++i)
        {
            delete mAnimGraphHistories[i];
        }

        mAnimGraphHistories.Clear();
    }


    void AnimGraphPlugin::ReInitAllGraphs()
    {
        // get the number of visual graphs and iterate through them all
        const uint32 numGraphs = mGraphInfos.GetLength();
        for (uint32 i = 0; i < numGraphs; ++i)
        {
            // get the current graph info and check if it is valid
            GraphInfo* graphInfo = mGraphInfos[i];
            if (graphInfo && graphInfo->mVisualGraph)
            {
                // try to find the anim graph node based on the id, in case the id is the invalid index nullptr will be returned, this happens for the root state machine
                EMotionFX::AnimGraphNode* animGraphNode = graphInfo->mAnimGraph->RecursiveFindNodeByID(graphInfo->mNodeID);
                if (animGraphNode)
                {
                    ReInitGraph(mGraphInfos[i]->mVisualGraph, graphInfo->mAnimGraph, animGraphNode);
                }
            }
        }
    }


    void AnimGraphPlugin::SaveAnimGraph(const char* filename, uint32 animGraphIndex, MCore::CommandGroup* commandGroup)
    {
        AZStd::string command = AZStd::string::format("SaveAnimGraph -index %i -filename \"%s\"", animGraphIndex, filename);

        if (commandGroup == nullptr)
        {
            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, "AnimGraph <font color=red>failed</font> to save");
            }
            else
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, "AnimGraph <font color=green>successfully</font> saved");

                // Send LyMetrics event.
                //EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(animGraphIndex);
                //MetricsEventSender::SendSaveAnimGraphEvent(animGraph);
            }
        }
        else
        {
            commandGroup->AddCommandString(command);
        }
    }


    void AnimGraphPlugin::SaveAnimGraph(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup)
    {
        const uint32 animGraphIndex = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph);
        if (animGraphIndex == MCORE_INVALIDINDEX32)
        {
            return;
        }

        AZStd::string filename = animGraph->GetFileName();
        if (filename.empty())
        {
            filename = GetMainWindow()->GetFileManager()->SaveAnimGraphFileDialog(GetResourceWidget());
            if (filename.empty())
            {
                return;
            }
        }

        SaveAnimGraph(filename.c_str(), animGraphIndex, commandGroup);
    }


    void AnimGraphPlugin::SaveAnimGraphAs(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup)
    {
        const AZStd::string filename = GetMainWindow()->GetFileManager()->SaveAnimGraphFileDialog(mViewWidget);
        if (filename.empty())
        {
            return;
        }

        const uint32 animGraphIndex = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph);
        if (animGraphIndex == MCORE_INVALIDINDEX32)
        {
            MCore::LogError("Cannot save anim graph. Anim graph index invalid.");
            return;
        }

        SaveAnimGraph(filename.c_str(), animGraphIndex, commandGroup);
    }


    void AnimGraphPlugin::OnFileOpen()
    {
        AZStd::string filename = GetMainWindow()->GetFileManager()->LoadAnimGraphFileDialog(mViewWidget);
        if (filename.empty())
        {
            return;
        }

        // Auto-relocate to asset source folder.

        if (!GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(filename))
        {
            const AZStd::string errorString = AZStd::string::format("Unable to find Anim Graph -filename \"%s\"", filename.c_str());
            AZ_Error("EMotionFX", false, errorString.c_str());
            return;
        }

        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selectionList.GetNumSelectedActorInstances();

        MCore::CommandGroup commandGroup("Load anim graph");
        AZStd::string command;

        command = AZStd::string::format("LoadAnimGraph -filename \"%s\"", filename.c_str());
        commandGroup.AddCommandString(command);

        // activate anim graph if it's the first added
        // a command group is needed if actor instances are selected to activate the anim graph
        if (EMotionFX::GetAnimGraphManager().GetNumAnimGraphs() == 0 && numActorInstances > 0)
        {
            // get the correct motion set
            // nullptr can only be <no motion set> because it's the first anim graph so no one is activated
            // if no motion set selected but one is possible, use the first possible
            // if no motion set selected and no one created, use no motion set
            // if one already selected, use the already selected
            uint32 motionSetId = MCORE_INVALIDINDEX32;
            EMotionFX::MotionSet* motionSet = mResourceWidget->GetSelectedMotionSet();
            if (motionSet)
            {
                motionSetId = motionSet->GetID();
            }
            else
            {
                const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
                if (numMotionSets > 0)
                {
                    for (uint32 i = 0; i < numMotionSets; ++i)
                    {
                        EMotionFX::MotionSet* candidate = EMotionFX::GetMotionManager().GetMotionSet(i);
                        if (candidate->GetIsOwnedByRuntime())
                        {
                            continue;
                        }

                        motionSet = candidate;
                        motionSetId = motionSet->GetID();
                        break;
                    }
                }
            }

            if (motionSet)
            {
                for (uint32 i = 0; i < numActorInstances; ++i)
                {
                    EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
                    if (actorInstance->GetIsOwnedByRuntime())
                    {
                        continue;
                    }

                    command = AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %%LASTRESULT%% -motionSetID %d", actorInstance->GetID(), motionSetId);
                    commandGroup.AddCommandString(command);
                }
            }
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void AnimGraphPlugin::OnFileSave()
    {
        EMotionFX::AnimGraph* animGraph = GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        const uint32 animGraphIndex = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph);
        assert(animGraphIndex != MCORE_INVALIDINDEX32);

        const MCore::String filename = animGraph->GetFileName();
        if (filename.GetIsEmpty())
        {
            OnFileSaveAs();
        }
        else
        {
            SaveAnimGraph(filename.AsChar(), animGraphIndex);
        }
    }


    void AnimGraphPlugin::OnFileSaveAs()
    {
        SaveAnimGraphAs(GetActiveAnimGraph());
    }


    bool AnimGraphPlugin::CheckIfIsParentNodeRecursively(EMotionFX::AnimGraphNode* startNode, EMotionFX::AnimGraphNode* mightBeAParent)
    {
        if (startNode == nullptr)
        {
            return false;
        }

        EMotionFX::AnimGraphNode* parent = startNode->GetParentNode();
        if (parent == mightBeAParent)
        {
            return true;
        }

        return CheckIfIsParentNodeRecursively(parent, mightBeAParent);
    }


    // toggle showing of processed nodes and connections on or off
    /*void AnimGraphPlugin::SetShowProcessed(bool show)
    {
        mShowProcessed = show;

        if (mShowProcessed)
            mShowProcessedTimer.start( 1000 / 60 , this); // show at 60 fps
        else
        {
            mShowProcessedTimer.stop();
            ClearAllProcessedFlags();
        }
    }*/


    // timer event
    void AnimGraphPlugin::ProcessFrame(float timePassedInSeconds)
    {
        if (GetManager()->GetAvoidRendering() || mGraphWidget->visibleRegion().isEmpty())
            return;

        mTotalTime += timePassedInSeconds;

        for (AnimGraphPerFrameCallback* callback : mPerFrameCallbacks)
        {
            callback->ProcessFrame(timePassedInSeconds);
        }

        bool redraw = false;
    #ifdef MCORE_DEBUG
        if (mTotalTime > 1.0f / 30.0f)
    #else
        if (mTotalTime > 1.0f / 60.0f)
    #endif
        {
            redraw = true;
            mTotalTime = 0.0f;
        }

        if (EMotionFX::GetRecorder().GetIsInPlayMode())
        {
            if (MCore::Compare<float>::CheckIfIsClose(EMotionFX::GetRecorder().GetCurrentPlayTime(), mLastPlayTime, 0.001f) == false)
            {
                mParameterWindow->UpdateParameterValues();
                mLastPlayTime = EMotionFX::GetRecorder().GetCurrentPlayTime();
            }
        }

        // get the active graph
        EMotionFX::AnimGraphNode* parentEMFXNode = mGraphWidget->GetCurrentNode();
        if (parentEMFXNode == nullptr)
        {
            if (redraw)
            {
                mGraphWidget->update();
            }
            return;
        }

        // get the active anim graph
        EMotionFX::AnimGraph* animGraph = GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            if (redraw)
            {
                mGraphWidget->update();
            }
            return;
        }

        // get the node graph
        EMStudio::NodeGraph* graph = mGraphWidget->GetActiveGraph();
        if (graph == nullptr)
        {
            if (redraw)
            {
                mGraphWidget->update();
            }
            return;
        }

        // get the animgraph instance
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();

            if (animGraphInstance)
            {
                // for all nodes in the graph
                const uint32 numNodes = graph->GetNumNodes();
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    GraphNode* graphNode = graph->GetNode(i);

                    // find the EMotion FX node for this graph node
                    EMotionFX::AnimGraphNode* emfxNode = animGraph->RecursiveFindNodeByID(graphNode->GetID());
                    MCORE_ASSERT(emfxNode);

                    if (emfxNode->GetAnimGraph() != animGraphInstance->GetAnimGraph())
                    {
                        continue;
                    }

                    graphNode->SetIsProcessed(animGraphInstance->GetIsOutputReady(emfxNode->GetObjectIndex()));
                    graphNode->SetIsUpdated(animGraphInstance->GetIsUpdateReady(emfxNode->GetObjectIndex()));
                    //      graphNode->SetIsProcessed( emfxNode->IsSynced(animGraphInstance) );
                    if (graphNode->GetType() == StateGraphNode::TYPE_ID)
                    {
                        continue;
                    }

                    // now update all connections
                    const uint32 numConnections = graphNode->GetNumConnections();
                    for (uint32 c = 0; c < numConnections; ++c)
                    {
                        NodeConnection* connection = graphNode->GetConnection(c);
                        EMotionFX::AnimGraphNode* emfxSourceNode = animGraph->RecursiveFindNodeByID(connection->GetSourceNode()->GetID());
                        EMotionFX::AnimGraphNode* emfxTargetNode = animGraph->RecursiveFindNodeByID(connection->GetTargetNode()->GetID());

                        const uint32 emfxConnectionIndex = emfxTargetNode->FindConnectionFromNode(emfxSourceNode, connection->GetOutputPortNr());
                        if (emfxConnectionIndex != MCORE_INVALIDINDEX32)
                        {
                            connection->SetIsProcessed(emfxTargetNode->GetConnection(emfxConnectionIndex)->GetIsVisited());
                        }
                    }
                }
            }
        }

        // redraw the graph
        if (redraw)
        {
            mGraphWidget->update();
        }
    }


    // clear all processed flags
    void AnimGraphPlugin::ClearAllProcessedFlags(bool updateView)
    {
        MCORE_UNUSED(updateView);

        // for all graphs that we already created
        const uint32 numGraphs = mGraphInfos.GetLength();
        for (uint32 i = 0; i < numGraphs; ++i)
        {
            GraphInfo* graphInfo = mGraphInfos[i];
            NodeGraph* graph = graphInfo->mVisualGraph;
            if (graph == nullptr)
            {
                continue;
            }

            // for all nodes in the graph
            const uint32 numNodes = graph->GetNumNodes();
            for (uint32 j = 0; j < numNodes; ++j)
            {
                GraphNode* graphNode = graph->GetNode(j);
                graphNode->SetIsProcessed(false);
                graphNode->SetIsUpdated(false);

                // now update all connections
                const uint32 numConnections = graphNode->GetNumConnections();
                for (uint32 c = 0; c < numConnections; ++c)
                {
                    graphNode->GetConnection(c)->SetIsProcessed(false);
                }
            }
        }

        // redraw the graph
        //if (updateView)
        //  mGraphWidget->update();
    }


    // trigger that attributes have been updated
    void AnimGraphPlugin::OnUpdateUniqueData()
    {
        // get the active anim graph
        EMotionFX::AnimGraph* animGraph = GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // recursively update all attributes of all nodes
        animGraph->RecursiveUpdateAttributes();

        // update all anim graph instances
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetActorManager().GetActorInstance(i)->GetAnimGraphInstance();

            if (animGraphInstance == nullptr)
            {
                continue;
            }

            if (animGraphInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            if (animGraph == animGraphInstance->GetAnimGraph())
            {
                animGraphInstance->OnUpdateUniqueData();
            }
        }

        // now that all attribute states are updated
        mAttributesWindow->UpdateAttributeWidgetStates();
    }


    int AnimGraphPlugin::SaveDirtyAnimGraph(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton)
    {
        // make sure the anim graph is valid
        if (animGraph == nullptr)
        {
            return QMessageBox::Discard;
        }

        // only process changed files
        if (animGraph->GetDirtyFlag() == false)
        {
            return DirtyFileManager::NOFILESTOSAVE;
        }

        if (askBeforeSaving)
        {
            EMStudio::GetApp()->setOverrideCursor(QCursor(Qt::ArrowCursor));

            QMessageBox msgBox(GetMainWindow());
            MCore::String text;

            if (animGraph->GetFileNameString().GetIsEmpty() == false)
            {
                text.Format("Save changes to '%s'?", animGraph->GetFileName());
            }
            else if (animGraph->GetNameString().GetIsEmpty() == false)
            {
                text.Format("Save changes to the anim graph named '%s'?", animGraph->GetName());
            }
            else
            {
                text = "Save changes to untitled anim graph?";
            }

            msgBox.setText(text.AsChar());
            msgBox.setWindowTitle("Save Changes");

            if (showCancelButton)
            {
                msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            }
            else
            {
                msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard);
            }

            msgBox.setDefaultButton(QMessageBox::Save);
            msgBox.setIcon(QMessageBox::Question);

            int messageBoxResult = msgBox.exec();
            switch (messageBoxResult)
            {
            case QMessageBox::Save:
            {
                SaveAnimGraph(animGraph, commandGroup);
                break;
            }
            case QMessageBox::Discard:
            {
                EMStudio::GetApp()->restoreOverrideCursor();
                return DirtyFileManager::FINISHED;
            }
            case QMessageBox::Cancel:
            {
                EMStudio::GetApp()->restoreOverrideCursor();
                return DirtyFileManager::CANCELED;
            }
            }
        }
        else
        {
            // save without asking first
            SaveAnimGraph(animGraph, commandGroup);
        }

        return DirtyFileManager::FINISHED;
    }


    // change the final node (node may not be nullptr)
    void AnimGraphPlugin::SetVirtualFinalNode(EMotionFX::AnimGraphNode* node)
    {
        EMotionFX::AnimGraphNode* parentNode = node->GetParentNode();
        if (parentNode == nullptr || parentNode->GetType() != EMotionFX::BlendTree::TYPE_ID)
        {
            return;
        }

        // get the blend tree
        EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(parentNode);

        // update all graph node opacity values
        NodeGraph* graph = FindGraphForNode(blendTree->GetID(), GetActiveAnimGraph());
        if (graph == nullptr)
        {
            return;
        }

        EMotionFX::AnimGraph* animGraph = GetActiveAnimGraph();
        RecursiveSetOpacity(graph, animGraph, blendTree->GetFinalNode(), 0.065f);
        RecursiveSetOpacity(graph, animGraph, node, 1.0f);

        if (node != blendTree->GetFinalNode())
        {
            GraphNode* graphNode = FindGraphNode(graph, node->GetID(), animGraph);
            MCORE_ASSERT(graphNode);
            graphNode->SetBorderColor(QColor(0, 255, 0));
        }
    }


    void AnimGraphPlugin::RecursiveSetOpacity(NodeGraph* graph, EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* startNode, float opacity)
    {
        GraphNode* graphNode = FindGraphNode(graph, startNode->GetID(), animGraph);
        MCORE_ASSERT(graphNode);
        graphNode->SetOpacity(opacity);
        graphNode->ResetBorderColor();

        // recurse through the inputs
        const uint32 numConnections = startNode->GetNumConnections();
        for (uint32 i = 0; i < numConnections; ++i)
        {
            EMotionFX::BlendTreeConnection* connection = startNode->GetConnection(i);
            RecursiveSetOpacity(graph, animGraph, connection->GetSourceNode(), opacity);
        }
    }


    int AnimGraphPlugin::OnSaveDirtyAnimGraphs()
    {
        return GetMainWindow()->GetDirtyFileManager()->SaveDirtyFiles(SaveDirtyAnimGraphFilesCallback::TYPE_ID);
    }


    // register keyboard shortcuts used for the render plugin
    void AnimGraphPlugin::RegisterKeyboardShortcuts()
    {
        MysticQt::KeyboardShortcutManager* shortcutManager = GetMainWindow()->GetShortcutManager();

        shortcutManager->RegisterKeyboardShortcut("Fit Entire Graph", "Anim Graph Window", Qt::Key_A, false, false, true);
        shortcutManager->RegisterKeyboardShortcut("Zoom On Selected Nodes", "Anim Graph Window", Qt::Key_Z, false, false, true);

        shortcutManager->RegisterKeyboardShortcut("Open Parent Node", "Anim Graph Window", Qt::Key_Up, false, false, true);
        shortcutManager->RegisterKeyboardShortcut("Open Selected Node", "Anim Graph Window", Qt::Key_Down, false, false, true);
        shortcutManager->RegisterKeyboardShortcut("History Back", "Anim Graph Window", Qt::Key_Left, false, false, true);
        shortcutManager->RegisterKeyboardShortcut("History Forward", "Anim Graph Window", Qt::Key_Right, false, false, true);

        shortcutManager->RegisterKeyboardShortcut("Align Left", "Anim Graph Window", Qt::Key_L, true, false, true);
        shortcutManager->RegisterKeyboardShortcut("Align Right", "Anim Graph Window", Qt::Key_R, true, false, true);
        shortcutManager->RegisterKeyboardShortcut("Align Top", "Anim Graph Window", Qt::Key_T, true, false, true);
        shortcutManager->RegisterKeyboardShortcut("Align Bottom", "Anim Graph Window", Qt::Key_B, true, false, true);

        shortcutManager->RegisterKeyboardShortcut("Cut", "Anim Graph Window", Qt::Key_X, true, false, true);
        shortcutManager->RegisterKeyboardShortcut("Copy", "Anim Graph Window", Qt::Key_C, true, false, true);
        shortcutManager->RegisterKeyboardShortcut("Paste", "Anim Graph Window", Qt::Key_V, true, false, true);
        shortcutManager->RegisterKeyboardShortcut("Select All", "Anim Graph Window", Qt::Key_A, true, false, true);
        shortcutManager->RegisterKeyboardShortcut("Unselect All", "Anim Graph Window", Qt::Key_D, true, false, true);
        shortcutManager->RegisterKeyboardShortcut("Delete Selected Nodes", "Anim Graph Window", Qt::Key_Delete, false, false, true);
    }


    void AnimGraphPlugin::OnUpdateAttributesWindow()
    {
        mAttributesWindow->Reinit();
    }


    void AttributeWidgetCallback::OnAttributeChanged()
    {
        /*  EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
            if (animGraph == nullptr)
                return;

            // recursively update attributes of all nodes
            animGraph->RecursiveUpdateAttributes();
            mPlugin->GetAttributesWindow()->UpdateAttributeWidgetStates();*/
    }


    void AttributeWidgetCallback::OnStructureChanged()
    {
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        EMotionFX::AnimGraphObject* object = mPlugin->GetAttributesWindow()->GetObject();
        if (object == nullptr)
        {
            return;
        }

        if (object->GetBaseType() == EMotionFX::AnimGraphNode::BASETYPE_ID)
        {
            mPlugin->SyncVisualNode(static_cast<EMotionFX::AnimGraphNode*>(object));
        }
        else
        if (object->GetBaseType() == EMotionFX::AnimGraphStateTransition::BASETYPE_ID)
        {
            mPlugin->SyncTransition(static_cast<EMotionFX::AnimGraphStateTransition*>(object));
        }

        //mPlugin->SyncVisualNodes();
        //mPlugin->SyncVisualTransitions();
    }


    void AttributeWidgetCallback::OnObjectChanged()
    {
        //mPlugin->OnUpdateUniqueData();
    }


    void AttributeWidgetCallback::OnUpdateAttributeWindow()
    {
        QTimer::singleShot(0, mPlugin, SLOT(OnUpdateAttributesWindow()));
    }


    // double clicked a node history item in the timeview plugin
    void AnimGraphPlugin::OnDoubleClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, EMotionFX::Recorder::NodeHistoryItem* historyItem)
    {
        MCORE_UNUSED(actorInstanceData);

        // try to locate the node based on its unique ID
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(historyItem->mAnimGraphID);
        if (animGraph == nullptr)
        {
            QMessageBox::warning(mDock, "Cannot Find Anim Graph", "The anim graph used by this node cannot be located anymore, did you delete it?", QMessageBox::Ok);
            return;
        }

        EMotionFX::AnimGraphNode* foundNode = animGraph->RecursiveFindNodeByUniqueID(historyItem->mNodeUniqueID);
        if (foundNode == nullptr)
        {
            QMessageBox::warning(mDock, "Cannot Find Node", "The anim graph node cannot be found. Did you perhaps delete the node or change animgraph?", QMessageBox::Ok);
            return;
        }

        EMotionFX::AnimGraphNode* nodeToShow = foundNode->GetParentNode();
        if (nodeToShow)
        {
            // select the item for this node
            mNavigateWidget->GetTreeWidget()->clearSelection();
            QTreeWidgetItem* item = mNavigateWidget->FindItem(foundNode->GetName());
            MCORE_ASSERT(item);
            mNavigateWidget->GetTreeWidget()->scrollToItem(item);
            item->setSelected(true);

            // select it in the visual graph as well and zoom in on the node in the graph
            GraphNode* graphNode;
            NodeGraph* graph;
            if (FindGraphAndNode(nodeToShow, foundNode->GetID(), &graph, &graphNode, animGraph))
            {
                graphNode->SetIsSelected(true);
                graph->ZoomOnRect(graphNode->GetRect(), mGraphWidget->geometry().width(), mGraphWidget->geometry().height(), true);
            }

            // show the graph and notify about the selection change
            mNavigateWidget->OnSelectionChanged();
            ShowGraph(nodeToShow, animGraph, true);
        }
    }



    // clicked a node history item in the timeview plugin
    void AnimGraphPlugin::OnClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, EMotionFX::Recorder::NodeHistoryItem* historyItem)
    {
        MCORE_UNUSED(actorInstanceData);

        // try to locate the node based on its unique ID
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(historyItem->mAnimGraphID);
        if (animGraph == nullptr)
        {
            QMessageBox::warning(mDock, "Cannot Find Anim Graph", "The anim graph used by this node cannot be located anymore, did you delete it?", QMessageBox::Ok);
            return;
        }

        EMotionFX::AnimGraphNode* foundNode = animGraph->RecursiveFindNodeByUniqueID(historyItem->mNodeUniqueID);
        if (foundNode == nullptr)
        {
            QMessageBox::warning(mDock, "Cannot Find Node", "The anim graph node cannot be found. Did you perhaps delete the node or change animgraph?", QMessageBox::Ok);
            return;
        }

        EMotionFX::AnimGraphNode* nodeToShow = foundNode->GetParentNode();
        if (nodeToShow)
        {
            mNavigateWidget->GetTreeWidget()->clearSelection();
            QTreeWidgetItem* item = mNavigateWidget->FindItem(foundNode->GetName());
            MCORE_ASSERT(item);
            mNavigateWidget->GetTreeWidget()->scrollToItem(item);
            item->setSelected(true);
            mNavigateWidget->OnSelectionChanged();
        }
    }


    void AnimGraphPlugin::OnAnimGraphNodeInputPortsChanged(EMotionFX::AnimGraphNode* node, const MCore::Array<EMotionFX::AnimGraphNode::Port>& newPorts)
    {
        const MCore::Array<EMotionFX::AnimGraphNode::Port> oldPorts = node->GetInputPorts();

        // figure out which connection changes are required
        MCore::Array<EMotionFX::BlendTreeConnection*> toRemoveConnections;
        MCore::Array<EMotionFX::BlendTreeConnection*> toUpdateConnections;
        node->GatherRequiredConnectionChangesInputPorts(newPorts, &toRemoveConnections, &toUpdateConnections);

        EMotionFX::AnimGraph* animGraph = node->GetAnimGraph();

        // get the EMotion FX and the visual graph parent node of the target node
        EMotionFX::AnimGraphNode* parentNode = node->GetParentNode();
        NodeGraph* visualGraph;
        if (parentNode)
        {
            visualGraph = FindGraphForNode(parentNode->GetID(), animGraph);
        }
        else
        {
            visualGraph = FindGraphForNode(MCORE_INVALIDINDEX32, animGraph);
        }

        // find the graph node associated with the target emfx node
        GraphNode* targetGraphNode = FindGraphNode(visualGraph, node->GetID(), animGraph);
        if (targetGraphNode == nullptr)
        {
            MCore::LogError("AnimGraphPlugin::OnAnimGraphNodeInputPortsChanged(): Cannot locate visual graph node for emfx target anim graph node '%s'", node->GetName());
        }

        //-------------------------------------------------
        // Remove connections
        //-------------------------------------------------
        for (uint32 i = 0; i < toRemoveConnections.GetLength(); ++i)
        {
            EMotionFX::BlendTreeConnection* connection = toRemoveConnections[i];

            // find the graph node associated with the source emfx node
            GraphNode* sourceGraphNode = nullptr;
            if (connection->GetSourceNode())
            {
                sourceGraphNode = FindGraphNode(visualGraph, connection->GetSourceNode()->GetID(), animGraph);
                if (sourceGraphNode == nullptr)
                {
                    MCore::LogError("AnimGraphPlugin::OnAnimGraphNodeInputPortsChanged(): Cannot locate visual graph node for emfx source anim graph node '%s'", connection->GetSourceNode()->GetName());
                }
            }

            // remove the visual graph connection
            if (sourceGraphNode && targetGraphNode)
            {
                if (targetGraphNode->RemoveConnection(connection->GetTargetPort(), sourceGraphNode, connection->GetSourcePort(), connection->GetID()) == false)
                {
                    MCore::LogError("AnimGraphPlugin::OnAnimGraphNodeInputPortsChanged() - Removing connection failed.");
                }
            }

            node->RemoveConnection(connection);
        }

        //-------------------------------------------------
        // Remove connections
        //-------------------------------------------------
        // update connections by remapping them to the same port names in the new ports list
        for (uint32 i = 0; i < toUpdateConnections.GetLength(); ++i)
        {
            EMotionFX::BlendTreeConnection* connection = toUpdateConnections[i];
            const uint32 orgPortNameID = node->GetInputPort(connection->GetTargetPort()).mNameID;

            // find the graph node associated with the source emfx node
            GraphNode* sourceGraphNode = nullptr;
            if (connection->GetSourceNode())
            {
                sourceGraphNode = FindGraphNode(visualGraph, connection->GetSourceNode()->GetID(), animGraph);
                if (sourceGraphNode == nullptr)
                {
                    MCore::LogError("AnimGraphPlugin::OnAnimGraphNodeInputPortsChanged(): Cannot locate visual graph node for emfx source anim graph node '%s'", connection->GetSourceNode()->GetName());
                }
            }

            NodeConnection* visualConnection = targetGraphNode->FindConnection(connection->GetTargetPort(), sourceGraphNode, connection->GetSourcePort());
            if (visualConnection == nullptr)
            {
                MCore::LogError("AnimGraphPlugin::OnAnimGraphNodeInputPortsChanged(): Cannot locate visual connection for connection from node '%s' to node '%s' on target port %d", connection->GetSourceNode()->GetName(), node->GetName(), connection->GetTargetPort());
            }

            bool found = false;
            for (uint32 p = 0; p < newPorts.GetLength() && found == false; ++p)
            {
                if (newPorts[p].mNameID == orgPortNameID)
                {
                    if (visualConnection)
                    {
                        visualConnection->SetTargetPort(p);
                    }

                    connection->SetTargetPort(static_cast<uint16>(p));
                    found = true;
                }
            }

            MCORE_ASSERT(found);
        }

        //-------------------------------------------------
        // Update ports
        //-------------------------------------------------
        node->SetInputPorts(newPorts);

        // copy unchanged connection pointers
        for (uint32 i = 0; i < oldPorts.GetLength(); ++i)
        {
            if (oldPorts[i].mConnection)
            {
                if (toRemoveConnections.Contains(oldPorts[i].mConnection) == false && toUpdateConnections.Contains(oldPorts[i].mConnection) == false)
                {
                    node->GetInputPort(i).mConnection = oldPorts[i].mConnection;
                }
            }
        }

        // update pointers in the modified ports
        for (uint32 i = 0; i < oldPorts.GetLength(); ++i)
        {
            if (oldPorts[i].mConnection)
            {
                if (toUpdateConnections.Contains(oldPorts[i].mConnection))
                {
                    node->GetInputPort(oldPorts[i].mConnection->GetTargetPort()).mConnection = oldPorts[i].mConnection;
                }
            }
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.moc>
