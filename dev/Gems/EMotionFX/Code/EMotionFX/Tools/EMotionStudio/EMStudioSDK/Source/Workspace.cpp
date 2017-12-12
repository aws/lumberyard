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

#include "Workspace.h"
#include "EMStudioManager.h"
#include "FileManager.h"
#include "MainWindow.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/FileSystem.h>
#include <MCore/Source/UnicodeString.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AttachmentNode.h>
#include <EMotionFX/Source/AttachmentSkin.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/MotionSetCommands.h>
#include <EMotionFX/Source/ActorManager.h>
#include "PreferencesWindow.h"
#include <AzFramework/StringFunc/StringFunc.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MetricsEventSender.h>

#include <QApplication>
#include <QMessageBox>
#include <QSettings>
#include <QFile>


namespace EMStudio
{
    Workspace::Workspace()
    {
        mDirtyFlag = false;
    }


    Workspace::~Workspace()
    {
    }


    void Workspace::AddFile(AZStd::string* inOutCommands, const char* command, const AZStd::string& filename, const char* additionalParameters) const
    {
        if (filename.empty())
        {
            return;
        }

        AZStd::string commandString;

        // Always load products from the cache folder.
        AZStd::string cacheFilename = filename;
        GetMainWindow()->GetFileManager()->RelocateToAssetCacheFolder(cacheFilename);

        // Retrieve relative filename.
        MCore::String relativeFilename = cacheFilename.c_str();
        EMotionFX::GetEMotionFX().GetFilenameRelativeTo(&relativeFilename, EMotionFX::GetEMotionFX().GetAssetCacheFolder().c_str());

        MCore::String finalFilename = "@assets@/";
        finalFilename += relativeFilename;

        commandString = AZStd::string::format("%s -filename \"%s\"", command, finalFilename.AsChar());

        if (additionalParameters)
        {
            commandString += " ";
            commandString += additionalParameters;
        }

        commandString += "\n";

        (*inOutCommands) += commandString;
    }


    bool Workspace::SaveToFile(const char* filename) const
    {
        QSettings settings(filename, QSettings::IniFormat, GetManager()->GetMainWindow());

        AZStd::string commandString;
        AZStd::string commands;

        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();

        // actors
        const uint32 numActors = EMotionFX::GetActorManager().GetNumActors();
        for (uint32 i = 0; i < numActors; ++i)
        {
            EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

            for (uint32 j = 0; j < numActorInstances; ++j)
            {
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(j);
                if (actorInstance->GetActor() != actor)
                {
                    continue;
                }

                if (actorInstance->GetIsOwnedByRuntime())
                {
                    continue;
                }

                const MCore::Vector3&       pos     = actorInstance->GetLocalPosition();
                const MCore::Vector3&       scale   = actorInstance->GetLocalScale();
                const MCore::Quaternion&    rot     = actorInstance->GetLocalRotation();

                // We need to add it here because if we dont do the last result will be the id of the actor instance and then it won't work anymore.
                AddFile(&commands, "ImportActor", actor->GetFileName());
                commandString = AZStd::string::format("CreateActorInstance -actorID %%LASTRESULT%% -xPos %f -yPos %f -zPos %f -xScale %f -yScale %f -zScale %f -rot %s\n", pos.x, pos.y, pos.z, scale.x, scale.y, scale.z, MCore::String(AZ::Vector4(rot.x, rot.y, rot.z, rot.w)).AsChar());
                commands += commandString;
            }
        }

        // attachments
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            if (actorInstance->GetIsAttachment())
            {
                EMotionFX::Attachment*      attachment                  = actorInstance->GetSelfAttachment();
                EMotionFX::ActorInstance*   attachedToActorInstance     = attachment->GetAttachToActorInstance();
                const uint32                attachedToInstanceIndex     = EMotionFX::GetActorManager().FindActorInstanceIndex(attachedToActorInstance);
                const uint32                attachtmentInstanceIndex    = EMotionFX::GetActorManager().FindActorInstanceIndex(actorInstance);

                if (actorInstance->GetIsSkinAttachment())
                {
                    commandString = AZStd::string::format("AddDeformableAttachment -attachmentIndex %d -attachToIndex %d\n", attachtmentInstanceIndex, attachedToInstanceIndex);
                    commands += commandString;
                }
                else
                {
                    EMotionFX::AttachmentNode*  attachmentSingleNode    = static_cast<EMotionFX::AttachmentNode*>(attachment);
                    const uint32                attachedToNodeIndex     = attachmentSingleNode->GetAttachToNodeIndex();
                    EMotionFX::Actor*           attachedToActor         = attachedToActorInstance->GetActor();
                    EMotionFX::Node*            attachedToNode          = attachedToActor->GetSkeleton()->GetNode(attachedToNodeIndex);

                    commandString = AZStd::string::format("AddAttachment -attachmentIndex %d -attachToIndex %d -attachToNode \"%s\"\n", attachtmentInstanceIndex, attachedToInstanceIndex, attachedToNode->GetName());
                    commands += commandString;
                }
            }
        }

        // motions
        const uint32 numMotions = EMotionFX::GetMotionManager().GetNumMotions();
        for (uint32 i = 0; i < numMotions; ++i)
        {
            EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);

            if (motion->GetIsOwnedByRuntime())
            {
                continue;
            }

            AddFile(&commands, "ImportMotion", motion->GetFileName());
        }

        // motion sets
        const uint32 numRootMotionSets = EMotionFX::GetMotionManager().CalcNumRootMotionSets();
        for (uint32 i = 0; i < numRootMotionSets; ++i)
        {
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindRootMotionSet(i);

            if (motionSet->GetIsOwnedByRuntime())
            {
                continue;
            }

            AddFile(&commands, "LoadMotionSet", motionSet->GetFilename());
        }

        // anim graphs
        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (uint32 i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            AddFile(&commands, "LoadAnimGraph", animGraph->GetFileName());
        }

        // activate anim graph for each actor instance
        const uint32 numActorInstance = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstance; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
            if (!animGraphInstance)
            {
                continue;
            }

            EMotionFX::AnimGraph* animGraph = animGraphInstance->GetAnimGraph();
            EMotionFX::MotionSet* motionSet = animGraphInstance->GetMotionSet();

            const uint32 animGraphIndex = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph);
            const uint32 motionSetIndex = EMotionFX::GetMotionManager().FindMotionSetIndex(motionSet);

            // only activate the saved anim graph
            if (!animGraph->GetFileNameString().GetIsEmpty())
            {
                commandString = AZStd::string::format("ActivateAnimGraph -actorInstanceIndex %d -animGraphIndex %d -motionSetIndex %d -visualizeScale %f\n", i, animGraphIndex, motionSetIndex, animGraphInstance->GetVisualizeScale());
                commands += commandString;
            }
        }

        // set workspace values
        settings.setValue("version", 1);
        settings.setValue("startScript", commands.c_str());

        // sync to ensure the status is correct because qt delays the write to file
        settings.sync();

        // check the status for no error
        return settings.status() == QSettings::NoError;
    }


    bool Workspace::Save(const char* filename, bool updateFileName, bool updateDirtyFlag)
    {
        // save to file secured by a backup file
        if (MCore::FileSystem::SaveToFileSecured(filename, [this, filename] { return SaveToFile(filename); }, GetCommandManager()))
        {
            // update the workspace filename
            if (updateFileName)
            {
                mFilename = filename;
            }

            // update the workspace dirty flag
            if (updateDirtyFlag)
            {
                GetCommandManager()->SetWorkspaceDirtyFlag(false);
                mDirtyFlag = false;
            }

            // Send LyMetrics event.
            //MetricsEventSender::SendSaveWorkspaceEvent(this);

            // save succeeded
            return true;
        }
        else
        {
            // show the error report
            GetCommandManager()->ShowErrorReport();

            // save failed
            return false;
        }
    }


    bool Workspace::Load(const char* filename, MCore::CommandGroup* commandGroup)
    {
        if (!QFile::exists(filename))
        {
            return false;
        }

        QSettings settings(filename, QSettings::IniFormat, (QWidget*)GetManager()->GetMainWindow());

        int32 version   = settings.value("version", -1).toInt();
        mFilename       = filename;

        MCore::String commandsString = FromQtString(settings.value("startScript", "").toString());
        MCore::Array<MCore::String> commands = commandsString.Split(MCore::UnicodeCharacter::endLine);

        const AZStd::string& assetCacheFolder = EMotionFX::GetEMotionFX().GetAssetCacheFolder();

        const uint32 numCommands = commands.GetLength();
        for (uint32 i = 0; i < numCommands; ++i)
        {
            // check if the string is empty and skip it in this case
            if (commands[i].GetIsEmpty())
            {
                continue;
            }

            // if we are dealing with a comment skip it as well
            if (commands[i].Find("//") == 0)
            {
                continue;
            }

            commands[i].Replace("@assets@/", assetCacheFolder.c_str());

            // add the command to the command group
            commandGroup->AddCommandString(commands[i]);
        }

        GetCommandManager()->SetWorkspaceDirtyFlag(false);
        mDirtyFlag = false;
        return true;
    }


    bool Workspace::GetDirtyFlag() const
    {
        if (mDirtyFlag || GetCommandManager()->GetWorkspaceDirtyFlag())
        {
            return true;
        }

        return false;
    }


    void Workspace::SetDirtyFlag(bool dirty)
    {
        mDirtyFlag = dirty;
        GetCommandManager()->SetWorkspaceDirtyFlag(dirty);
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/Workspace.moc>