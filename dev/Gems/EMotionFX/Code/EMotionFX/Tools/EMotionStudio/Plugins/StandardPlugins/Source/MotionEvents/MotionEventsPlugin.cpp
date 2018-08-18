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

#include "MotionEventsPlugin.h"
#include "../TimeView/TrackHeaderWidget.h"
#include <MCore/Source/LogManager.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <QLabel>
#include <QShortcut>
#include <QPushButton>
#include <QVBoxLayout>


namespace EMStudio
{
    MotionEventsPlugin::MotionEventsPlugin()
        : EMStudio::DockWidgetPlugin()
        , mDialogStack(nullptr)
        , mAdjustMotionCallback(nullptr)
        , mSelectCallback(nullptr)
        , mUnselectCallback(nullptr)
        , mClearSelectionCallback(nullptr)
        , mTimeViewPlugin(nullptr)
        , mTrackDataWidget(nullptr)
        , mMotionWindowPlugin(nullptr)
        , mMotionListWindow(nullptr)
        , mMotionTable(nullptr)
        , mMotionEventWidget(nullptr)
        , mTrackHeaderWidget(nullptr)
        , mMotion(nullptr)
    {
    }


    MotionEventsPlugin::~MotionEventsPlugin()
    {
        GetCommandManager()->RemoveCommandCallback(mAdjustMotionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearSelectionCallback, false);
        delete mAdjustMotionCallback;
        delete mSelectCallback;
        delete mUnselectCallback;
        delete mClearSelectionCallback;
    }


    // clone the log window
    EMStudioPlugin* MotionEventsPlugin::Clone()
    {
        return new MotionEventsPlugin();
    }


    // on before remove plugin
    void MotionEventsPlugin::OnBeforeRemovePlugin(uint32 classID)
    {
        if (classID == TimeViewPlugin::CLASS_ID)
        {
            mTimeViewPlugin = nullptr;
        }

        if (classID == MotionWindowPlugin::CLASS_ID)
        {
            mMotionWindowPlugin = nullptr;
        }
    }


    // init after the parent dock window has been created
    bool MotionEventsPlugin::Init()
    {
        // create callbacks
        mAdjustMotionCallback = new CommandAdjustMotionCallback(false);
        mSelectCallback = new CommandSelectCallback(false);
        mUnselectCallback = new CommandUnselectCallback(false);
        mClearSelectionCallback = new CommandClearSelectionCallback(false);
        GetCommandManager()->RegisterCommandCallback("AdjustMotion", mAdjustMotionCallback);
        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", mClearSelectionCallback);

        // create the dialog stack
        assert(mDialogStack == nullptr);
        mDialogStack = new MysticQt::DialogStack(mDock);
        mDock->SetContents(mDialogStack);

        // create the motion event presets widget
        mMotionEventPresetsWidget = new MotionEventPresetsWidget(mDialogStack, this);
        mDialogStack->Add(mMotionEventPresetsWidget, "Motion Event Presets", false, true);
        connect(mDock, SIGNAL(visibilityChanged(bool)), this, SLOT(WindowReInit(bool)));

        // create the motion event properties widget
        mMotionEventWidget = new MotionEventWidget(mDialogStack);
        mDialogStack->Add(mMotionEventWidget, "Motion Event Properties");

        ValidatePluginLinks();
        UpdateMotionEventWidget();

        return true;
    }


    void MotionEventsPlugin::ValidatePluginLinks()
    {
        if (!mTimeViewPlugin)
        {
            EMStudioPlugin* timeViewBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(TimeViewPlugin::CLASS_ID);
            if (timeViewBasePlugin)
            {
                mTimeViewPlugin     = (TimeViewPlugin*)timeViewBasePlugin;
                mTrackDataWidget    = mTimeViewPlugin->GetTrackDataWidget();
                mTrackHeaderWidget  = mTimeViewPlugin->GetTrackHeaderWidget();

                connect((QWidget*)mTrackDataWidget, SIGNAL(MotionEventPresetsDropped(QPoint)), this, SLOT(OnEventPresetDropped(QPoint)));
                connect(mTimeViewPlugin, SIGNAL(SelectionChanged()), this, SLOT(UpdateMotionEventWidget()));
                connect(this, SIGNAL(OnColorChanged()), mTimeViewPlugin, SLOT(ReInit()));
            }
        }

        if (!mMotionWindowPlugin)
        {
            EMStudioPlugin* motionBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID);
            if (motionBasePlugin)
            {
                mMotionWindowPlugin = (MotionWindowPlugin*)motionBasePlugin;
                mMotionListWindow   = mMotionWindowPlugin->GetMotionListWindow();

                connect(mMotionListWindow, SIGNAL(MotionSelectionChanged()), this, SLOT(MotionSelectionChanged()));
            }
        }

        // in case one motion is selected
        MotionSelectionChanged();
    }


    void MotionEventsPlugin::MotionSelectionChanged()
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        if (mMotion != motion)
        {
            mMotion = motion;
            ReInit();
        }
    }


    void MotionEventsPlugin::ReInit()
    {
        ValidatePluginLinks();

        // update the selection array as well as the motion event widget
        UpdateMotionEventWidget();
    }


    // reinit the window when it gets activated
    void MotionEventsPlugin::WindowReInit(bool visible)
    {
        if (visible)
        {
            MotionSelectionChanged();
        }
    }


    bool MotionEventsPlugin::CheckIfIsPresetReadyToDrop()
    {
        // get the motion event presets table
        QTableWidget* eventPresetsTable = mMotionEventPresetsWidget->GetMotionEventPresetsTable();
        if (eventPresetsTable == nullptr)
        {
            return false;
        }

        // get the number of motion event presets and iterate through them
        const uint32 numRows = eventPresetsTable->rowCount();
        for (uint32 i = 0; i < numRows; ++i)
        {
            QTableWidgetItem* itemType = eventPresetsTable->item(i, 1);

            if (itemType->isSelected())
            {
                return true;
            }
        }

        return false;
    }


    void MotionEventsPlugin::OnEventPresetDropped(QPoint position)
    {
        // calculate the start time for the motion event
        double dropTimeInSeconds = mTimeViewPlugin->PixelToTime(position.x());
        //mTimeViewPlugin->CalcTime( position.x(), &dropTimeInSeconds, nullptr, nullptr, nullptr, nullptr );

        // get the time track on which we dropped the preset
        TimeTrack* timeTrack = mTimeViewPlugin->GetTrackAt(position.y());
        if (!timeTrack || !mMotion)
        {
            return;
        }

        // get the corresponding motion event track
        EMotionFX::MotionEventTable* eventTable = mMotion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(timeTrack->GetName());
        if (eventTrack == nullptr)
        {
            return;
        }

        // get the motion event presets table
        QTableWidget* eventPresetsTable = mMotionEventPresetsWidget->GetMotionEventPresetsTable();
        if (eventPresetsTable == nullptr)
        {
            return;
        }

        // get the number of motion event presets and iterate through them
        const uint32 numRows = eventPresetsTable->rowCount();
        for (uint32 i = 0; i < numRows; ++i)
        {
            QTableWidgetItem* itemType      = eventPresetsTable->item(i, 1);
            QTableWidgetItem* itemParameter = eventPresetsTable->item(i, 2);
            QTableWidgetItem* itemMirror    = eventPresetsTable->item(i, 3);

            if (itemType->isSelected())
            {
                const AZStd::string command = AZStd::string::format("CreateMotionEvent -motionID %i -eventTrackName \"%s\" -startTime %f -endTime %f -eventType \"%s\" -parameters \"%s\" -mirrorType \"%s\"", mMotion->GetID(), eventTrack->GetName(), dropTimeInSeconds, dropTimeInSeconds, itemType->text().toUtf8().data(), itemParameter->text().toUtf8().data(), itemMirror->text().toUtf8().data());

                AZStd::string result;
                if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
                {
                    AZ_Error("EMotionFX", false, result.c_str());
                }
            }
        }
    }


    void MotionEventsPlugin::UpdateMotionEventWidget()
    {
        if (!mMotionEventWidget || !mTimeViewPlugin)
        {
            return;
        }

        mTimeViewPlugin->UpdateSelection();
        if (mTimeViewPlugin->GetNumSelectedEvents() != 1)
        {
            mMotionEventWidget->ReInit(nullptr, nullptr, nullptr, MCORE_INVALIDINDEX32);
        }
        else
        {
            EventSelectionItem selectionItem = mTimeViewPlugin->GetSelectedEvent(0);
            mMotionEventWidget->ReInit(selectionItem.mMotion, selectionItem.GetEventTrack(), selectionItem.GetMotionEvent(), selectionItem.mEventNr);
        }
    }


    // callbacks
    bool ReInitMotionEventsPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionEventsPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        MotionEventsPlugin* motionEventsPlugin = (MotionEventsPlugin*)plugin;
        motionEventsPlugin->ReInit();

        return true;
    }


    bool MotionSelectionChangedMotionEventsPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionEventsPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }

        MotionEventsPlugin* motionEventsPlugin = (MotionEventsPlugin*)plugin;
        motionEventsPlugin->MotionSelectionChanged();

        return true;
    }


    bool MotionEventsPlugin::CommandAdjustMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMotionEventsPlugin(); }
    bool MotionEventsPlugin::CommandAdjustMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)          { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMotionEventsPlugin(); }
    bool MotionEventsPlugin::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return MotionSelectionChangedMotionEventsPlugin();
    }
    bool MotionEventsPlugin::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return MotionSelectionChangedMotionEventsPlugin();
    }
    bool MotionEventsPlugin::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return MotionSelectionChangedMotionEventsPlugin();
    }
    bool MotionEventsPlugin::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return MotionSelectionChangedMotionEventsPlugin();
    }
    bool MotionEventsPlugin::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return MotionSelectionChangedMotionEventsPlugin(); }
    bool MotionEventsPlugin::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return MotionSelectionChangedMotionEventsPlugin(); }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/MotionEventsPlugin.moc>