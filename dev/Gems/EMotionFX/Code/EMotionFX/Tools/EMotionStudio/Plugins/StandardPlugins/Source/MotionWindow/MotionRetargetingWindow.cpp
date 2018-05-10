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
#include "MotionRetargetingWindow.h"
#include "MotionWindowPlugin.h"
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QIcon>
#include <MysticQt/Source/ButtonGroup.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <MCore/Source/Compare.h>


namespace EMStudio
{
    // constructor
    MotionRetargetingWindow::MotionRetargetingWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin)
        : QWidget(parent)
    {
        mMotionWindowPlugin         = motionWindowPlugin;
        mMotionRetargetingButton    = nullptr;
        //mRetargetingNode          = nullptr;
        //mRetargetRootOffsetSlider = nullptr;
        //mRetargetRootOffsetLabel  = nullptr;
        //mOffsetValueEdit          = nullptr;
        //mOffsetMinSpinBox         = nullptr;
        //mOffsetMaxSpinBox         = nullptr;
        //mNodeSelectionWindow      = nullptr;
        mRenderMotionBindPose       = nullptr;

        //  mSelectCallback             = nullptr;
        //  mUnselectCallback           = nullptr;
        //  mClearSelectionCallback     = nullptr;
    }


    // destructor
    MotionRetargetingWindow::~MotionRetargetingWindow()
    {
        /*  GetCommandManager()->RemoveCommandCallback( mSelectCallback, false );
            GetCommandManager()->RemoveCommandCallback( mUnselectCallback, false );
            GetCommandManager()->RemoveCommandCallback( mClearSelectionCallback, false );
            delete mSelectCallback;
            delete mUnselectCallback;
            delete mClearSelectionCallback;*/
    }


    // init after the parent dock window has been created
    void MotionRetargetingWindow::Init()
    {
        // create and register the command callbacks
        /*  mSelectCallback                 = new CommandSelectCallback(false);
            mUnselectCallback               = new CommandUnselectCallback(false);
            mClearSelectionCallback         = new CommandClearSelectionCallback(false);
            GetCommandManager()->RegisterCommandCallback( "Select", mSelectCallback );
            GetCommandManager()->RegisterCommandCallback( "Unselect", mUnselectCallback );
            GetCommandManager()->RegisterCommandCallback( "ClearSelection", mClearSelectionCallback );*/

        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);
        setLayout(layout);

        mMotionRetargetingButton = new QCheckBox("Use Motion Retargeting");
        layout->addWidget(mMotionRetargetingButton);
        connect(mMotionRetargetingButton, SIGNAL(clicked()), this, SLOT(UpdateMotions()));

        mRenderMotionBindPose = new QCheckBox("Render Motion Bind Pose");
        mRenderMotionBindPose->setToolTip("Render motion bind pose of the currently selected motion for the selected actor instances");
        mRenderMotionBindPose->setChecked(false);
        layout->addWidget(mRenderMotionBindPose);

        /*mRetargetRootOffsetSlider = new MysticQt::Slider(Qt::Horizontal, this);
        mRetargetRootOffsetSlider->setRange(0, 1000);
        mRetargetRootOffsetSlider->setValue(666); // this is needed so that reseting works below
        mRetargetRootOffsetSlider->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        mRetargetRootOffsetLabel    = new QLabel(tr("Retarget Root Offset"), this);
        mOffsetValueEdit            = new QLineEdit(this);
        mOffsetValueEdit->setReadOnly(true);
        mOffsetValueEdit->setMaximumWidth(50);
        mOffsetMinSpinBox           = new MysticQt::DoubleSpinBox(this);
        mOffsetMaxSpinBox           = new MysticQt::DoubleSpinBox(this);
        mOffsetMinSpinBox->setRange(-9999.9, 9999.9);
        mOffsetMaxSpinBox->setRange(-9999.9, 9999.9);
        mOffsetMinSpinBox->setValue(-10.0);
        mOffsetMaxSpinBox->setValue(10.0);*/

        // add the retarget root node selection
        /*QHBoxLayout* retargetNodeLayout = new QHBoxLayout();
        retargetNodeLayout->setMargin(0);

        QLabel* rootNodeLabel = new QLabel("Retarget Root Node:");
        retargetNodeLayout->addWidget( rootNodeLabel, 0, Qt::AlignLeft );

        mRetargetingNode = new MysticQt::LinkWidget( mMotionWindowPlugin->GetDefaultNodeSelectionLabelText() );
        retargetNodeLayout->addWidget(mRetargetingNode, 0, Qt::AlignLeft);

        QWidget* dummyWidget = new QWidget();
        dummyWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        retargetNodeLayout->addWidget(dummyWidget, 0, Qt::AlignLeft);

        layout->addLayout(retargetNodeLayout);*/

        // create the retarget root offset slider helper layout and widget
        /*QVBoxLayout* offsetGroupLayout = new QVBoxLayout();
        offsetGroupLayout->setMargin(0);
        offsetGroupLayout->setSpacing(0);

        QHBoxLayout* offsetSliderLayout = new QHBoxLayout();
        offsetSliderLayout->setMargin(2);

        offsetSliderLayout->addWidget(mOffsetMinSpinBox);
        offsetSliderLayout->addWidget(mRetargetRootOffsetSlider);
        offsetSliderLayout->addWidget(mOffsetMaxSpinBox);
        offsetSliderLayout->addWidget(mOffsetValueEdit, Qt::AlignRight);

        offsetGroupLayout->addWidget(mRetargetRootOffsetLabel);
        offsetGroupLayout->addLayout(offsetSliderLayout);

        layout->addLayout(offsetGroupLayout);

        // create the node selection window
        mNodeSelectionWindow = new NodeSelectionWindow(this, true);*/

        //connect( mRetargetRootOffsetSlider,                                                       SIGNAL(valueChanged(int)),                              this, SLOT(RetargetRootOffsetSliderChanged(int)) );
        //connect( mRetargetRootOffsetSlider,                                                       SIGNAL(sliderReleased()),                               this, SLOT(UpdateMotions()) );
        //connect( mOffsetMinSpinBox,                                                               SIGNAL(valueChanged(double)),                           this, SLOT(OffsetMinValueChanged(double)) );
        //connect( mOffsetMaxSpinBox,                                                               SIGNAL(valueChanged(double)),                           this, SLOT(OffsetMaxValueChanged(double)) );
        //connect( mRetargetingNode,                                                                SIGNAL(clicked()),                                      this, SLOT(OnSelectNode()) );
        //connect( (const QObject*)mNodeSelectionWindow,                                            SIGNAL(rejected()),                                     this, SLOT(OnCancelNodeSelection()) );
        //connect( (const QObject*)mNodeSelectionWindow->GetNodeHierarchyWidget()->GetTreeWidget(), SIGNAL(itemSelectionChanged()),                         this, SLOT(OnNodeChanged()) );
        //connect( (const QObject*)mNodeSelectionWindow->GetNodeHierarchyWidget(),                  SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)),   this, SLOT(OnNodeSelected(MCore::Array<SelectionItem>)) );

        //ResetRetargetRootOffset();
        //UpdateSelection();
    }

    /*
    void MotionRetargetingWindow::OnCancelNodeSelection()
    {
        //Node* oldRetargetNode = mActor->FindNodeByName( mNodeBeforeSelectionWindow.c_str() );
        //mActor->SetRepositioningNode( oldMotionExtractionNode );
    }
    */

    void MotionRetargetingWindow::UpdateMotions()
    {
        const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();

        // create our command group
        MCore::CommandGroup commandGroup("Adjust default motion instances");

        // get the number of selected motions and iterate through them
        const uint32 numMotions = selection.GetNumSelectedMotions();
        for (uint32 i = 0; i < numMotions; ++i)
        {
            MotionWindowPlugin::MotionTableEntry* entry = mMotionWindowPlugin->FindMotionEntryByID(selection.GetMotion(i)->GetID());
            if (entry == nullptr)
            {
                MCore::LogError("Cannot find motion table entry for the given motion.");
                continue;
            }

            EMotionFX::Motion*          motion              = entry->mMotion;
            EMotionFX::PlayBackInfo*    playbackInfo        = motion->GetDefaultPlayBackInfo();

            AZStd::string commandParameters;

            //if (Compare<float>::CheckIfIsClose(playbackInfo->mRetargetRootOffset, GetRetargetRootOffset(), 0.001f) == false)      commandParameters += AZStd::string::format("-retargetRootOffset %f ", GetRetargetRootOffset());
            //if (playbackInfo->mRetargetRootIndex != compareTo->mRetargetRootIndex)                                                commandParameters += AZStd::string::format("-retargetRootIndex %i ", playbackInfo->mRetargetRootIndex);
            if (playbackInfo->mRetarget != mMotionRetargetingButton->isChecked())
            {
                commandParameters += AZStd::string::format("-retarget %s ", AZStd::to_string(mMotionRetargetingButton->isChecked()).c_str());
            }

            // debug output, remove me later on!
            //LogDebug("commandParameters: %s", commandParameters.c_str());

            // in case the command parameters are empty it means nothing changed, so we can skip this command
            if (commandParameters.empty() == false)
            {
                commandGroup.AddCommandString(AZStd::string::format("AdjustDefaultPlayBackInfo -filename \"%s\" %s", motion->GetFileName(), commandParameters.c_str()).c_str());
            }
        }

        // execute the group command
        AZStd::string outResult;
        if (CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    void MotionRetargetingWindow::UpdateInterface()
    {
        const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        //MCore::Array<EMotionFX::MotionInstance*>& motionInstances = mMotionWindowPlugin->GetSelectedMotionInstances();

        // check if there actually is any motion selected
        const uint32 numSelectedMotions = selection.GetNumSelectedMotions();
        const bool isEnabled = (numSelectedMotions != 0);

        mMotionRetargetingButton->setEnabled(isEnabled);
        mRenderMotionBindPose->setEnabled(isEnabled);
        //mRetargetingNode->setEnabled(isEnabled);
        //mRetargetRootOffsetSlider->setEnabled(isEnabled);
        //mRetargetRootOffsetLabel->setEnabled(isEnabled);
        //mOffsetValueEdit->setEnabled(isEnabled);
        //mOffsetMinSpinBox->setEnabled(isEnabled);
        //mOffsetMaxSpinBox->setEnabled(isEnabled);

        // adjust the retargeting node label
        /*ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            mRetargetingNode->setText( QString::fromWCharArray(mMotionWindowPlugin->GetDefaultNodeSelectionLabelText()) );
            mRetargetingNode->setEnabled(false);
        }
        else
        {
            mRetargetingNode->setEnabled(true);
            if (numSelectedMotions == 1)
                SetRetargetingNodeLabel( actorInstance, selection.GetMotion(0) );
            else
                mRetargetingNode->setText( QString::fromWCharArray(mMotionWindowPlugin->GetDefaultNodeSelectionLabelText()) );
        }*/

        if (isEnabled == false)
        {
            return;
        }

        // iterate through the selected motions
        for (uint32 i = 0; i < numSelectedMotions; ++i)
        {
            MotionWindowPlugin::MotionTableEntry* entry = mMotionWindowPlugin->FindMotionEntryByID(selection.GetMotion(i)->GetID());
            if (entry == nullptr)
            {
                MCore::LogError("Cannot find motion table entry for the given motion.");
                continue;
            }

            EMotionFX::Motion*          motion              = entry->mMotion;
            EMotionFX::PlayBackInfo*    defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();

            mMotionRetargetingButton->setChecked(defaultPlayBackInfo->mRetarget);
            //SetRetargetRootOffset( defaultPlayBackInfo->mRetargetRootOffset );
        }
    }


    /*
    void MotionRetargetingWindow::SetRetargetingNodeLabel(EMotionFX::ActorInstance* actorInstance, EMotionFX::Motion* motion)
    {
        MotionWindowPlugin::MotionTableEntry* entry = mMotionWindowPlugin->FindMotionByID( motion->GetID() );
        if (entry == nullptr)
        {
            LogError("Cannot find motion table entry for the given motion.");
            return;
        }

        PlayBackInfo*   defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();
        const uint32    retargetRootIndex   = defaultPlayBackInfo->mRetargetRootIndex;

        if (retargetRootIndex == MCORE_INVALIDINDEX32)
        {
            mRetargetingNode->setText( QString::fromWCharArray(mMotionWindowPlugin->GetDefaultNodeSelectionLabelText()) );
        }
        else
        {
            Actor* actor = actorInstance->GetActor();
            Node* node = actor->GetNode(retargetRootIndex);
            mRetargetingNode->setText( QString::fromWCharArray(node->GetName()) );
        }
    }
    */

    /*void MotionRetargetingWindow::UpdateSelection()
    {
        // get the selected actorinstance
        SelectionList& selection    = GetCommandManager()->GetCurrentSelection();
        mSelectedActorInstance      = selection.GetSingleActorInstance();

        bool interfaceEnabled = true;
        if (mSelectedActorInstance == nullptr)
            interfaceEnabled = false;

        mRetargetRootOffsetSlider->setEnabled(interfaceEnabled);
        mRetargetRootOffsetLabel->setEnabled(interfaceEnabled);
        mOffsetValueEdit->setEnabled(interfaceEnabled);
        mOffsetMinSpinBox->setEnabled(interfaceEnabled);
        mOffsetMaxSpinBox->setEnabled(interfaceEnabled);
    }*/

    /*
    void MotionRetargetingWindow::RetargetRootOffsetSliderChanged(int value)
    {
        const float offset = GetRetargetRootOffset();
        MCore::Array<EMotionFX::MotionInstance*>& motionInstances = mMotionWindowPlugin->GetSelectedMotionInstances();
        const uint32 numMotionInstances = motionInstances.GetLength();
        for (uint32 i=0; i<numMotionInstances; ++i)
        {
            MotionInstance* motionInstance = motionInstances[i];
            //motionInstance->SetRetargetRootOffset(offset);
            MCore::LogInfo("MotionRetargetingWindow::RetargetRootOffsetSliderChanged() - Remove this function, move it to the Actor or ActorInstance settings");
        }

        mOffsetValueEdit->setText( QString::fromWCharArray(AZStd::string::format("%.2f", offset).c_str()) );
    }


    float MotionRetargetingWindow::GetRetargetRootOffset() const
    {
        const float min             = mOffsetMinSpinBox->value();
        const float max             = mOffsetMaxSpinBox->value();
        const float range           = max - min;
        const float normalizedValue = mRetargetRootOffsetSlider->value() * 0.001f;

        return min + normalizedValue * range;
    }


    void MotionRetargetingWindow::SetRetargetRootOffset(float value)
    {
        const float min             = mOffsetMinSpinBox->value();
        const float max             = mOffsetMaxSpinBox->value();
        const float range           = max - min;

        // in case the min and max are the same, prevent a division by zero
        if (range < Math::epsilon)
            mRetargetRootOffsetSlider->setValue(1000);

        const float normalizedValue = (value - min) / range;

        mRetargetRootOffsetSlider->setValue(normalizedValue * 1000);
    }


    void MotionRetargetingWindow::OnSelectNode()
    {
        ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            LogWarning("Cannot open node selection window. Please select an actor instance first.");
            return;
        }

        // get the old motion extraction node name
        mNodeBeforeSelectionWindow.Clear();
        Actor* actor = actorInstance->GetActor();
        mActor = actor;
        //Node* retargetNode = ???;
        //if (retargetNode)
        //  mNodeBeforeSelectionWindow = retargetNode->GetName();

        // show the node selection window
        mSelectionList.Clear();
        //if (retargetNode)
        //  mSelectionList.AddNode(retargetNode);
        mNodeSelectionWindow->Update(actorInstance->GetID(), &mSelectionList);
        mNodeSelectionWindow->show();
    }
    */
    /*
    void MotionRetargetingWindow::OnNodeSelected(MCore::Array<SelectionItem> selection)
    {
        // check if selection is valid
        if (selection.GetLength() != 1 || selection[0].GetNodeNameString().GetIsEmpty())
        {
            LogWarning("Cannot adjust retargeting root node. No valid node selected.");
            return;
        }

        const uint32    actorInstanceID     = selection[0].mActorInstanceID;
        const char* nodeName            = selection[0].GetNodeName();
        ActorInstance*  actorInstance       = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
            return;

        mRetargetingNode->setText( QString::fromWCharArray(nodeName) );

        Actor* actor = actorInstance->GetActor();
        Node* rootNode = actor->FindNodeByName(nodeName);
        if (rootNode == nullptr)
            return;

        uint32 rootNodeIndex = rootNode->GetNodeIndex();


        SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();

        // create our command group
        CommandGroup commandGroup("Adjust default motion playback infos");

        // get the number of selected motions and iterate through them
        const uint32 numMotions = selectionList.GetNumSelectedMotions();
        for (uint32 i=0; i<numMotions; ++i)
        {
            MotionWindowPlugin::MotionTableEntry* entry = mMotionWindowPlugin->FindMotionByID( selectionList.GetMotion(i)->GetID() );
            if (entry == nullptr)
            {
                LogError("Cannot find motion table entry for the given motion.");
                continue;
            }

            Motion*         motion              = entry->mMotion;
            PlayBackInfo*   playbackInfo        = motion->GetDefaultPlayBackInfo();

            commandGroup.AddCommandString( String() = AZStd::string::format("AdjustDefaultPlayBackInfo -filename \"%s\" -retargetRootIndex %i", motion->GetFileName(), rootNodeIndex).c_str() );
        }

        // execute the group command
        String outResult;
        if (GetCommandManager()->ExecuteCommandGroup( commandGroup, outResult ) == false)
            LogError(outResult.c_str());
    }*/

    //-----------------------------------------------------------------------------------------
    // command callbacks
    //-----------------------------------------------------------------------------------------
    /*
    bool UpdateInterfaceMotionRetargetingWindow()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin( MotionWindowPlugin::CLASS_ID );
        if (plugin == nullptr)
            return false;

        MotionWindowPlugin* motionWindowPlugin = (MotionWindowPlugin*)plugin;
        MotionRetargetingWindow* retargetingWindow = motionWindowPlugin->GetMotionRetargetingWindow();

        // is the plugin visible? only update it if it is visible
        if (retargetingWindow->visibleRegion().isEmpty() == false)
            retargetingWindow->UpdateSelection();

        return true;
    }

    bool MotionRetargetingWindow::CommandSelectCallback::Execute(Command* command, const CommandLine& commandLine)                      { return UpdateInterfaceMotionRetargetingWindow(); }
    bool MotionRetargetingWindow::CommandSelectCallback::Undo(Command* command, const CommandLine& commandLine)                         { return UpdateInterfaceMotionRetargetingWindow(); }
    bool MotionRetargetingWindow::CommandUnselectCallback::Execute(Command* command, const CommandLine& commandLine)                    { return UpdateInterfaceMotionRetargetingWindow(); }
    bool MotionRetargetingWindow::CommandUnselectCallback::Undo(Command* command, const CommandLine& commandLine)                       { return UpdateInterfaceMotionRetargetingWindow(); }
    bool MotionRetargetingWindow::CommandClearSelectionCallback::Execute(Command* command, const CommandLine& commandLine)              { return UpdateInterfaceMotionRetargetingWindow(); }
    bool MotionRetargetingWindow::CommandClearSelectionCallback::Undo(Command* command, const CommandLine& commandLine)                 { return UpdateInterfaceMotionRetargetingWindow(); }
    */
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionRetargetingWindow.moc>