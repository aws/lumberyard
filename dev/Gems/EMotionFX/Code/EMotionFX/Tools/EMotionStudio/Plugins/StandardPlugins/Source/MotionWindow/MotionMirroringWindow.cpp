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
#include "MotionMirroringWindow.h"
#include "MotionWindowPlugin.h"
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QCheckBox>
#include <MysticQt/Source/ButtonGroup.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>


namespace EMStudio
{
    // constructor
    MotionMirroringWindow::MotionMirroringWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin)
        : QWidget(parent)
    {
        mMotionWindowPlugin     = motionWindowPlugin;
        mMotionMirroringButton  = nullptr;
        mVerifyButton           = nullptr;
    }


    // destructor
    MotionMirroringWindow::~MotionMirroringWindow()
    {
    }


    // init after the parent dock window has been created
    void MotionMirroringWindow::Init()
    {
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);
        setLayout(layout);

        mMotionMirroringButton = new QCheckBox("Use Motion Mirroring");
        layout->addWidget(mMotionMirroringButton);
        connect(mMotionMirroringButton, &QCheckBox::clicked, this, &MotionMirroringWindow::UpdateMotions);

        mVerifyButton = new QPushButton();
        mVerifyButton->setText("Verify Motion Bind Pose vs. Actor Bind pose");
        connect(mVerifyButton, &QPushButton::pressed, this, &MotionMirroringWindow::OnVerifyButtonPressed);
        layout->addWidget(mVerifyButton);
    }


    void MotionMirroringWindow::UpdateMotions()
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

            if (motion->GetIsOwnedByRuntime())
            {
                continue;
            }

            AZStd::string commandParameters;
            if (playbackInfo->mMirrorMotion != mMotionMirroringButton->isChecked())
            {
                commandParameters += AZStd::string::format("-mirrorMotion %s ", AZStd::to_string(mMotionMirroringButton->isChecked()).c_str());
            }

            // debug output, remove me later on!
            //LogDebug("commandParameters: %s", commandParameters.c_str());

            // in case the command parameters are empty it means nothing changed, so we can skip this command
            if (commandParameters.empty() == false)
            {
                commandGroup.AddCommandString(AZStd::string::format("AdjustDefaultPlayBackInfo -filename \"%s\" %s", motion->GetFileName(), commandParameters.c_str()).c_str());
            }

            // adjust all motion instances if we modified the mirroring settings
            const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
            for (uint32 a = 0; a < numActorInstances; ++a)
            {
                // get the actor instance
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(a);

                if (actorInstance->GetIsOwnedByRuntime())
                {
                    continue;
                }

                // check if we have a motion system
                // if not, it is using the anim graph system
                EMotionFX::MotionSystem* motionSystem = actorInstance->GetMotionSystem();
                if (motionSystem == nullptr) // not using motion layer system
                {
                    continue;
                }

                // iterate over all currently playing motion instances
                const uint32 numMotionInstances = motionSystem->GetNumMotionInstances();
                for (uint32 j = 0; j < numMotionInstances; ++j)
                {
                    EMotionFX::MotionInstance* motionInstance = motionSystem->GetMotionInstance(j);

                    // recreate the motion links if it uses this motion
                    if (motionInstance->GetMotion() == motion)
                    {
                        motionInstance->SetMirrorMotion(mMotionMirroringButton->isChecked());
                        //motion->CreateMotionLinks( motionInstance->GetActorInstance()->GetActor(), motionInstance );
                        motionInstance->InitForSampling();
                    }
                }
            }
        }

        // execute the group command
        AZStd::string outResult;
        if (CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    void MotionMirroringWindow::UpdateInterface()
    {
        const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        //MCore::Array<EMotionFX::MotionInstance*>& motionInstances = mMotionWindowPlugin->GetSelectedMotionInstances();

        // check if there actually is any motion selected
        const uint32 numSelectedMotions = selection.GetNumSelectedMotions();
        const bool isEnabled = (numSelectedMotions != 0);

        mMotionMirroringButton->setEnabled(isEnabled);
        mVerifyButton->setEnabled(isEnabled);

        if (isEnabled == false)
        {
            return;
        }

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
            EMotionFX::PlayBackInfo*    defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();

            mMotionMirroringButton->setChecked(defaultPlayBackInfo->mMirrorMotion);
        }
    }


    // verify motion bind pose against actor bind pose
    void MotionMirroringWindow::OnVerifyButtonPressed()
    {
        const CommandSystem::SelectionList& selectionList               = CommandSystem::GetCommandManager()->GetCurrentSelection();
        const uint32                        numSelectedActorInstances   = selectionList.GetNumSelectedActorInstances();
        const uint32                        numSelectedMotions          = selectionList.GetNumSelectedMotions();

        if (numSelectedActorInstances != 1 || numSelectedMotions != 1)
        {
            MCore::LogWarning("No or multiple actor instances/motions selected. Please select only one actor instance and one motion.");
            return;
        }

        EMotionFX::ActorInstance*   actorInstance   = selectionList.GetActorInstance(0);
        EMotionFX::Motion*          motion          = selectionList.GetMotion(0);

        // check if it is safe to start playing a mirrored motion
        if (motion->GetType() == EMotionFX::SkeletalMotion::TYPE_ID)
        {
            EMotionFX::Actor*               actor           = actorInstance->GetActor();
            EMotionFX::SkeletalMotion*      skeletalMotion  = (EMotionFX::SkeletalMotion*)motion;

            if (skeletalMotion->CheckIfIsMatchingActorBindPose(actor) == false)
            {
                MCore::LogError("The bind pose from the skeletal motion does not match the one from the actor. Skinning errors might occur when using motion mirroring. Please read the motion mirroring chapter in the ArtistGuide guide for more information or contact the support team.");
            }
            else
            {
                MCore::LogInfo("Everything OK. The bind pose from the skeletal motion matches the one from the actor.");
            }
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionMirroringWindow.moc>