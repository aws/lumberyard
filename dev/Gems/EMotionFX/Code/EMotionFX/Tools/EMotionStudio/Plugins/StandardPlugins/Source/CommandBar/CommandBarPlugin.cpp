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
#include "CommandBarPlugin.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MCore/Source/LogManager.h>
#include "../MotionWindow/MotionWindowPlugin.h"
#include "../MotionWindow/MotionListWindow.h"
#include <QLineEdit>
#include <QPushButton>
#include <QApplication>
#include <QAction>
#include <QLabel>
#include <QHBoxLayout>


namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(CommandBarPlugin::ProgressHandler, EMotionFX::EventHandlerAllocator, 0)


    // constructor
    CommandBarPlugin::CommandBarPlugin()
        : EMStudio::ToolBarPlugin()
    {
        mCommandEdit                    = nullptr;
        mResultEdit                     = nullptr;
        mPauseButton                    = nullptr;
        mPlayForwardButton              = nullptr;
        mStopButton                     = nullptr;
        mSeekForwardButton              = nullptr;
        mSeekBackwardButton             = nullptr;
        mLockSelectionButton            = nullptr;
        mLockEnabledIcon                = nullptr;
        mLockDisabledIcon               = nullptr;
        mToggleLockSelectionCallback    = nullptr;
        mProgressHandler                = nullptr;
        mSpeedSlider                    = nullptr;
        mSpeedResetButton               = nullptr;
    }


    // destructor
    CommandBarPlugin::~CommandBarPlugin()
    {
        delete mLockEnabledIcon;
        delete mLockDisabledIcon;

        GetCommandManager()->RemoveCommandCallback(mToggleLockSelectionCallback, false);
        delete mToggleLockSelectionCallback;

        // remove the event handler again
        EMotionFX::GetEventManager().RemoveEventHandler(mProgressHandler);
        delete mProgressHandler;
    }


    // get the compile date
    const char* CommandBarPlugin::GetCompileDate() const
    {
        return MCORE_DATE;
    }


    // get the name
    const char* CommandBarPlugin::GetName() const
    {
        return "Command Bar";
    }


    // get the plugin type id
    uint32 CommandBarPlugin::GetClassID() const
    {
        return CommandBarPlugin::CLASS_ID;
    }


    // get the creator name
    const char* CommandBarPlugin::GetCreatorName() const
    {
        return "MysticGD";
    }


    // get the version
    float CommandBarPlugin::GetVersion() const
    {
        return 1.0f;
    }


    // clone the log window
    EMStudioPlugin* CommandBarPlugin::Clone()
    {
        CommandBarPlugin* newPlugin = new CommandBarPlugin();
        return newPlugin;
    }


    // init after the parent dock window has been created
    bool CommandBarPlugin::Init()
    {
        mToggleLockSelectionCallback = new CommandToggleLockSelectionCallback(false);
        GetCommandManager()->RegisterCommandCallback("ToggleLockSelection", mToggleLockSelectionCallback);

        mLockEnabledIcon    = new QIcon(AZStd::string(MysticQt::GetDataDir() + "/Images/Icons/LockEnabled.png").c_str());
        mLockDisabledIcon   = new QIcon(AZStd::string(MysticQt::GetDataDir() + "/Images/Icons/LockDisabled.png").c_str());

        mCommandEdit = new QLineEdit();
        mCommandEdit->setPlaceholderText("Enter command");
        connect(mCommandEdit, &QLineEdit::returnPressed, this, &CommandBarPlugin::OnEnter);
        mCommandEditAction = mBar->addWidget(mCommandEdit);

        mResultEdit = new QLineEdit();
        mResultEdit->setReadOnly(true);
        mCommandResultAction = mBar->addWidget(mResultEdit);

        mSpeedSlider = new MysticQt::Slider(Qt::Horizontal);
        mSpeedSlider->setMaximumWidth(80);
        mSpeedSlider->setMinimumWidth(30);
        mSpeedSlider->setRange(0, 100);
        mSpeedSlider->setValue(50);
        mSpeedSlider->setToolTip("The global simulation speed factor.\nA value of 1.0 means the normal speed, which is when the slider handle is in the center.\nPress the button on the right of this slider to reset to the normal speed.");
        connect(mSpeedSlider, &MysticQt::Slider::valueChanged, this, &CommandBarPlugin::OnSpeedSliderValueChanged);
        mBar->addWidget(mSpeedSlider);

        mSpeedResetButton = CreateButton("/Images/Icons/Reset.png");
        mSpeedResetButton->setToolTip("Reset the global simulation speed factor to its normal speed");
        connect(mSpeedResetButton, &QPushButton::clicked, this, &CommandBarPlugin::ResetGlobalSpeed);

        mProgressText = new QLabel();
        mProgressText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        mProgressText->setAlignment(Qt::AlignRight);
        mProgressText->setStyleSheet("padding-right: 1px; color: rgb(140, 140, 140);");
        mProgressTextAction = mBar->addWidget(mProgressText);
        mProgressTextAction->setVisible(false);

        mProgressBar = new QProgressBar();
        mProgressBar->setRange(0, 100);
        mProgressBar->setValue(0);
        mProgressBar->setMaximumWidth(300);
        mProgressBar->setStyleSheet("padding-right: 2px;");
        mProgressBarAction = mBar->addWidget(mProgressBar);
        mProgressBarAction->setVisible(false);

        mLockSelectionButton = CreateButton();
        mLockSelectionButton->setToolTip("Lock or unlock the selection of actor instances");
        connect(mLockSelectionButton, &QPushButton::clicked, this, &CommandBarPlugin::OnLockSelectionButton);

        mSeekBackwardButton = CreateButton("/Images/Icons/SkipBackward.png");
        mSeekBackwardButton->setToolTip("Seek backward the selected motions");
        connect(mSeekBackwardButton, &QPushButton::clicked, this, &CommandBarPlugin::OnSeekBackwardButton);

        mPauseButton = CreateButton("/Images/Icons/Pause.png");
        mPauseButton->setToolTip("Pause the selected motions");
        connect(mPauseButton, &QPushButton::clicked, this, &CommandBarPlugin::OnPauseButton);

        mStopButton = CreateButton("/Images/Icons/Stop.png");
        mStopButton->setToolTip("Stop all motions");
        connect(mStopButton, &QPushButton::clicked, this, &CommandBarPlugin::OnStopButton);

        mPlayForwardButton = CreateButton("/Images/Icons/PlayForward.png");
        mPlayForwardButton->setToolTip("Play the selected motions");
        connect(mPlayForwardButton, &QPushButton::clicked, this, &CommandBarPlugin::OnPlayForwardButton);

        mSeekForwardButton = CreateButton("/Images/Icons/SkipForward.png");
        mSeekForwardButton->setToolTip("Seek forward the selected motions");
        connect(mSeekForwardButton, &QPushButton::clicked, this, &CommandBarPlugin::OnSeekForwardButton);

        UpdateLockSelectionIcon();

        mProgressHandler = aznew ProgressHandler(this);
        EMotionFX::GetEventManager().AddEventHandler(mProgressHandler);

        return true;
    }


    QPushButton* CommandBarPlugin::CreateButton(const char* iconFileName)
    {
        QPushButton* button = new QPushButton();
        button->setStyleSheet("border-radius: 0px;");

        const QSize buttonSize = QSize(18, 18);
        button->setMinimumSize(buttonSize);
        button->setMaximumSize(buttonSize);
        button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        if (iconFileName)
        {
            button->setIcon(MysticQt::GetMysticQt()->FindIcon(iconFileName));
        }

        button->setIconSize(buttonSize - QSize(2, 2));

        mBar->addWidget(button);
        return button;
    }


    void CommandBarPlugin::OnPlayForwardButton()
    {
        // get the selected motion instances, get the number of them and iterate through them
        const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
        const size_t numMotionInstances = motionInstances.size();
        for (size_t i = 0; i < numMotionInstances; ++i)
        {
            EMotionFX::MotionInstance* motionInstance = motionInstances[i];
            motionInstance->UnPause();
        }

        // get the current selected motions
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numSelectedMotions = selectionList.GetNumSelectedMotions();

        AZStd::vector<EMotionFX::Motion*> motionsToPlay;
        motionsToPlay.reserve(numSelectedMotions);

        // iterate through the motions
        for (uint32 i = 0; i < numSelectedMotions; ++i)
        {
            EMotionFX::Motion* motion = selectionList.GetMotion(i);

            // check if the given motion is already playing
            bool isPlaying = false;
            for (uint32 j = 0; j < numMotionInstances; ++j)
            {
                if (motion == motionInstances[j]->GetMotion())
                {
                    isPlaying = true;
                    break;
                }
            }

            // only start playing the motion in case it is not being played already
            if (isPlaying == false)
            {
                motionsToPlay.push_back(motion);
            }
        }

        // finally start playing the motions
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return;
        }

        MotionWindowPlugin* motionWindowPlugin = (MotionWindowPlugin*)plugin;
        motionWindowPlugin->PlayMotions(motionsToPlay);
    }


    void CommandBarPlugin::OnStopButton()
    {
        AZStd::string outResult;
        GetCommandManager()->ExecuteCommand("StopAllMotionInstances", outResult);
    }


    void CommandBarPlugin::OnPauseButton()
    {
        const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
        const size_t numMotionInstances = motionInstances.size();
        for (size_t i = 0; i < numMotionInstances; ++i)
        {
            EMotionFX::MotionInstance* motionInstance = motionInstances[i];
            motionInstance->Pause();
        }
    }


    void CommandBarPlugin::OnSeekForwardButton()
    {
        const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
        const size_t numMotionInstances = motionInstances.size();
        for (size_t i = 0; i < numMotionInstances; ++i)
        {
            EMotionFX::MotionInstance* motionInstance = motionInstances[i];
            motionInstance->SetCurrentTime(motionInstance->GetMaxTime());
        }
    }


    void CommandBarPlugin::OnSeekBackwardButton()
    {
        const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
        const size_t numMotionInstances = motionInstances.size();
        for (size_t i = 0; i < numMotionInstances; ++i)
        {
            EMotionFX::MotionInstance* motionInstance = motionInstances[i];
            motionInstance->Rewind();
        }
    }


    void CommandBarPlugin::OnLockSelectionButton()
    {
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand("ToggleLockSelection", result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        UpdateLockSelectionIcon();
    }


    void CommandBarPlugin::UpdateLockSelectionIcon()
    {
        if (EMStudio::GetCommandManager()->GetLockSelection())
        {
            mLockSelectionButton->setIcon(*mLockEnabledIcon);
        }
        else
        {
            mLockSelectionButton->setIcon(*mLockDisabledIcon);
        }
    }


    // execute the command when enter is pressed
    void CommandBarPlugin::OnEnter()
    {
        QLineEdit* edit = qobject_cast<QLineEdit*>(sender());
        assert(edit == mCommandEdit);

        // get the command string trimmed
        AZStd::string command = FromQtString(edit->text());
        AzFramework::StringFunc::TrimWhiteSpace(command, true, true);

        // don't do anything on an empty command
        if (command.size() == 0)
        {
            edit->clear();
            return;
        }

        // execute the command
        AZStd::string resultString;
        const bool success = EMStudio::GetCommandManager()->ExecuteCommand(command.c_str(), resultString);

        // there was an error
        if (success == false)
        {
            MCore::LogError(resultString.c_str());
            mResultEdit->setStyleSheet("color: red;");
            mResultEdit->setText(resultString.c_str());
            mResultEdit->setToolTip(resultString.c_str());
        }
        else // no error
        {
            mResultEdit->setStyleSheet("color: rgb(0,255,0);");
            mResultEdit->setText(resultString.c_str());
        }

        // clear the text of the edit box
        edit->clear();
    }


    void CommandBarPlugin::OnProgressStart()
    {
        mProgressBarAction->setVisible(true);
        mProgressTextAction->setVisible(true);

        mCommandEditAction->setVisible(false);
        mCommandResultAction->setVisible(false);

        // setVisible has no effect, hack to have them not visible
        mSpeedSlider->setMaximumWidth(0);
        mSpeedSlider->setMinimumWidth(0);
        mSpeedResetButton->setMinimumWidth(0);
        mSpeedResetButton->setMaximumWidth(0);

        EMStudio::GetApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    }


    void CommandBarPlugin::OnProgressEnd()
    {
        mProgressBarAction->setVisible(false);
        mProgressTextAction->setVisible(false);

        mCommandEditAction->setVisible(true);
        mCommandResultAction->setVisible(true);

        // setVisible has no effect, hack to have them visible again
        mSpeedSlider->setMaximumWidth(80);
        mSpeedSlider->setMinimumWidth(30);
        mSpeedResetButton->setMinimumWidth(18);
        mSpeedResetButton->setMaximumWidth(18);

        EMStudio::GetApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    }


    // adjust the main progress text
    void CommandBarPlugin::OnProgressText(const char* text)
    {
        mProgressText->setText(text);
        EMStudio::GetApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    }


    // adjust the main progress bar
    void CommandBarPlugin::OnProgressValue(float percentage)
    {
        mProgressBar->setValue(percentage);
        EMStudio::GetApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    }


    // reset the global simulation speed
    void CommandBarPlugin::ResetGlobalSpeed()
    {
        mSpeedSlider->setValue(50);
        EMotionFX::GetEMotionFX().SetGlobalSimulationSpeed(1.0f);
    }


    // the value of the speed slider changed
    void CommandBarPlugin::OnSpeedSliderValueChanged(int value)
    {
        const float newSpeed = (value / 100.0f) * 2.0f;
        EMotionFX::GetEMotionFX().SetGlobalSimulationSpeed(newSpeed);
    }


    //-----------------------------------------------------------------------------------------
    // command callbacks
    //-----------------------------------------------------------------------------------------

    bool UpdateInterfaceCommandBarPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(CommandBarPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        CommandBarPlugin* commandBarPlugin = (CommandBarPlugin*)plugin;
        commandBarPlugin->UpdateLockSelectionIcon();
        return true;
    }

    bool CommandBarPlugin::CommandToggleLockSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)  { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateInterfaceCommandBarPlugin(); }
    bool CommandBarPlugin::CommandToggleLockSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateInterfaceCommandBarPlugin(); }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/CommandBar/CommandBarPlugin.moc>