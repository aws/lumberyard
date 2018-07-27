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
        AZStd::string relativeFilename = cacheFilename.c_str();
        EMotionFX::GetEMotionFX().GetFilenameRelativeTo(&relativeFilename, EMotionFX::GetEMotionFX().GetAssetCacheFolder().c_str());

        AZStd::string finalFilename = "@assets@/";
        finalFilename += relativeFilename;

        commandString = AZStd::string::format("%s -filename \"%s\"", command, finalFilename.c_str());

        if (additionalParameters)
        {
            commandString += " ";
            commandString += additionalParameters;
        }

        commandString += "\n";

        (*inOutCommands) += commandString;
    }

    struct ActivationIndices
    {
        int32 m_actorInstanceCommandIndex = -1;
        int32 m_animGraphCommandIndex = -1;
        int32 m_motionSetCommandIndex = -1;
    };

    bool Workspace::SaveToFile(const char* filename) const
    {
        QSettings settings(filename, QSettings::IniFormat, GetManager()->GetMainWindow());

        AZStd::string commandString;
        AZStd::string commands;

        // For each actor we are going to set a different activation command. We need to store the command indices
        // that will produce the results for the actorInstanceIndex, animGraphIndex and motionSetIndex that are
        // currently selected
        typedef AZStd::unordered_map<EMotionFX::ActorInstance*, ActivationIndices> ActivationIndicesByActorInstance;
        ActivationIndicesByActorInstance activationIndicesByActorInstance;
        int32 commandIndex = 0;

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

                const AZ::Vector3&          pos     = actorInstance->GetLocalPosition();
                const AZ::Vector3&          scale   = actorInstance->GetLocalScale();
                const MCore::Quaternion&    rot     = actorInstance->GetLocalRotation();

                // We need to add it here because if we dont do the last result will be the id of the actor instance and then it won't work anymore.
                AddFile(&commands, "ImportActor", actor->GetFileName());
                ++commandIndex;
                commandString = AZStd::string::format("CreateActorInstance -actorID %%LASTRESULT%% -xPos %f -yPos %f -zPos %f -xScale %f -yScale %f -zScale %f -rot %s\n",
                    static_cast<float>(pos.GetX()), static_cast<float>(pos.GetY()), static_cast<float>(pos.GetZ()), static_cast<float>(scale.GetX()), static_cast<float>(scale.GetY()), static_cast<float>(scale.GetZ()), 
                    AZStd::to_string(AZ::Vector4(rot.x, rot.y, rot.z, rot.w)).c_str());
                commands += commandString;

                activationIndicesByActorInstance[actorInstance].m_actorInstanceCommandIndex = commandIndex;
                ++commandIndex;
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
                    ++commandIndex;
                }
                else
                {
                    EMotionFX::AttachmentNode*  attachmentSingleNode    = static_cast<EMotionFX::AttachmentNode*>(attachment);
                    const uint32                attachedToNodeIndex     = attachmentSingleNode->GetAttachToNodeIndex();
                    EMotionFX::Actor*           attachedToActor         = attachedToActorInstance->GetActor();
                    EMotionFX::Node*            attachedToNode          = attachedToActor->GetSkeleton()->GetNode(attachedToNodeIndex);

                    commandString = AZStd::string::format("AddAttachment -attachmentIndex %d -attachToIndex %d -attachToNode \"%s\"\n", attachtmentInstanceIndex, attachedToInstanceIndex, attachedToNode->GetName());
                    commands += commandString;
                    ++commandIndex;
                }
            }
        }

        // motion sets
        const uint32 numRootMotionSets = EMotionFX::GetMotionManager().CalcNumRootMotionSets();
        AZStd::unordered_set<EMotionFX::Motion*> motionsInMotionSets;
        for (uint32 i = 0; i < numRootMotionSets; ++i)
        {
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindRootMotionSet(i);

            if (motionSet->GetIsOwnedByRuntime())
            {
                continue;
            }

            AddFile(&commands, "LoadMotionSet", motionSet->GetFilename());

            for (ActivationIndicesByActorInstance::value_type& indicesByActorInstance : activationIndicesByActorInstance)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = indicesByActorInstance.first->GetAnimGraphInstance();
                if (!animGraphInstance)
                {
                    continue;
                }

                EMotionFX::MotionSet* currentActiveMotionSet = animGraphInstance->GetMotionSet();
                if (currentActiveMotionSet == motionSet)
                {
                    indicesByActorInstance.second.m_motionSetCommandIndex = commandIndex;
                    break;
                }
            }
            ++commandIndex;

            motionSet->RecursiveGetMotions(motionsInMotionSets);
        }
        
        // motions that are not in the above motion sets
        const uint32 numMotions = EMotionFX::GetMotionManager().GetNumMotions();
        for (uint32 i = 0; i < numMotions; ++i)
        {
            EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);

            if (motion->GetIsOwnedByRuntime())
            {
                continue;
            }
            if (motionsInMotionSets.find(motion) != motionsInMotionSets.end())
            {
                continue; // Already saved by a motion set
            }

            AddFile(&commands, "ImportMotion", motion->GetFileName());
            ++commandIndex;
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

            for (ActivationIndicesByActorInstance::value_type& indicesByActorInstance : activationIndicesByActorInstance)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = indicesByActorInstance.first->GetAnimGraphInstance();
                if (!animGraphInstance)
                {
                    continue;
                }

                EMotionFX::AnimGraph* currentActiveAnimGraph = animGraphInstance->GetAnimGraph();
                if (currentActiveAnimGraph == animGraph)
                {
                    indicesByActorInstance.second.m_animGraphCommandIndex = commandIndex;
                    break;
                }
            }

            ++commandIndex;
        }

        // activate anim graph for each actor instance
        for (uint32 i = 0; i < numActorInstances; ++i)
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

            ActivationIndicesByActorInstance::const_iterator itActivationIndices = activationIndicesByActorInstance.find(actorInstance);
            if (itActivationIndices == activationIndicesByActorInstance.end())
            {
                continue;
            }

            // only activate the saved anim graph
            if (itActivationIndices->second.m_animGraphCommandIndex != -1
                && itActivationIndices->second.m_motionSetCommandIndex != -1)
            {
                commandString = AZStd::string::format("ActivateAnimGraph -actorInstanceID %%LASTRESULT%d%% -animGraphID %%LASTRESULT%d%% -motionSetID %%LASTRESULT%d%% -visualizeScale %f\n", 
                    (commandIndex - itActivationIndices->second.m_actorInstanceCommandIndex), 
                    (commandIndex - itActivationIndices->second.m_animGraphCommandIndex),
                    (commandIndex - itActivationIndices->second.m_motionSetCommandIndex),
                    animGraphInstance->GetVisualizeScale());
                commands += commandString;
                ++commandIndex;
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

        AZStd::string commandsString = FromQtString(settings.value("startScript", "").toString());

        AZStd::vector<AZStd::string> commands;
        AzFramework::StringFunc::Tokenize(commandsString.c_str(), commands, MCore::CharacterConstants::endLine, true /* keep empty strings */, true /* keep space strings */);

        const AZStd::string& assetCacheFolder = EMotionFX::GetEMotionFX().GetAssetCacheFolder();

        const size_t numCommands = commands.size();
        for (size_t i = 0; i < numCommands; ++i)
        {
            // check if the string is empty and skip it in this case
            if (commands[i].empty())
            {
                continue;
            }

            // if we are dealing with a comment skip it as well
            if (commands[i].find("//") == 0)
            {
                continue;
            }

            AzFramework::StringFunc::Replace(commands[i], "@assets@/", assetCacheFolder.c_str(), true /* case sensitive */);

            // add the command to the command group
            commandGroup->AddCommandString(commands[i]);
        }

        GetCommandManager()->SetWorkspaceDirtyFlag(false);
        mDirtyFlag = false;
        return true;
    }


    void Workspace::Reset()
    {
        mFilename.clear();

        GetCommandManager()->SetWorkspaceDirtyFlag(false);
        mDirtyFlag = false;
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