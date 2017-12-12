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

#include <AzFramework/StringFunc/StringFunc.h>
#include <MCore/Source/AttributeSet.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/NodeGroup.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/SkeletalSubMotion.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/CommandSystem/Source/ActorCommands.h>
#include <EMotionFX/CommandSystem/Source/LODCommands.h>
#include <EMotionFX/CommandSystem/Source/MetaData.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>


namespace CommandSystem
{

    AZStd::string MetaData::GenerateMotionMetaData(EMotionFX::Motion* motion)
    {
        if (!motion)
        {
            AZ_Error("EMotionFX", false, "Cannot generate meta data for motion. Motion invalid.");
            return "";
        }

        AZStd::string metaDataString;

        // Save event tracks including motion events.
        EMotionFX::MotionEventTable* motionEventTable = motion->GetEventTable();
        metaDataString += AZStd::string::format("AdjustMotion -motionID $(MOTIONID) -motionExtractionFlags %d\n", motion->GetMotionExtractionFlags());
        metaDataString += "ClearMotionEvents -motionID $(MOTIONID)\n";

        const AZ::u32 numEventTracks = motionEventTable->GetNumTracks();
        for (AZ::u32 i = 0; i < numEventTracks; ++i)
        {
            EMotionFX::MotionEventTrack* eventTrack = motionEventTable->GetTrack(i);

            metaDataString += AZStd::string::format("CreateMotionEventTrack -motionID $(MOTIONID) -eventTrackName \"%s\"\n", eventTrack->GetName());
            metaDataString += AZStd::string::format("AdjustMotionEventTrack -motionID $(MOTIONID) -eventTrackName \"%s\" -enabled %s\n", eventTrack->GetName(), eventTrack->GetIsEnabled() ? "true" : "false");

            const AZ::u32 numMotionEvents = eventTrack->GetNumEvents();
            for (AZ::u32 j = 0; j < numMotionEvents; ++j)
            {
                EMotionFX::MotionEvent& motionEvent = eventTrack->GetEvent(j);
                metaDataString += AZStd::string::format("CreateMotionEvent -motionID $(MOTIONID) -eventTrackName \"%s\" ", eventTrack->GetName());
                metaDataString += AZStd::string::format("-startTime %f -endTime %f ", motionEvent.GetStartTime(), motionEvent.GetEndTime());
                metaDataString += AZStd::string::format("-eventType \"%s\" -parameters \"%s\" ", motionEvent.GetEventTypeString(), motionEvent.GetParameterString(eventTrack).AsChar());
                metaDataString += AZStd::string::format("-mirrorType \"%s\"\n", motionEvent.GetMirrorEventTypeString());
            }
        }

        return metaDataString;
    }


    bool MetaData::ApplyMetaDataOnMotion(EMotionFX::Motion* motion, const AZStd::string& metaDataString)
    {
        return ApplyMetaData(motion->GetID(), "$(MOTIONID)", metaDataString);
    }


    void MetaData::GenerateNodeGroupMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString)
    {
        AZStd::string nodeNameList;
        const AZ::u32 numNodeGroups = actor->GetNumNodeGroups();
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            EMotionFX::NodeGroup* nodeGroup = actor->GetNodeGroup(i);
            outMetaDataString += AZStd::string::format("AddNodeGroup -actorID $(ACTORID) -name \"%s\"\n", nodeGroup->GetName());

            nodeNameList.clear();
            const AZ::u16 numNodes = nodeGroup->GetNumNodes();
            for (AZ::u16 n = 0; n < numNodes; ++n)
            {
                const AZ::u16    nodeIndex = nodeGroup->GetNode(n);
                EMotionFX::Node* node = actor->GetSkeleton()->GetNode(nodeIndex);

                if (n > 0)
                {
                    nodeNameList += ";";
                }

                nodeNameList += node->GetName();
            }

            const bool enabledOnDefault = nodeGroup->GetIsEnabledOnDefault();
            outMetaDataString += AZStd::string::format("AdjustNodeGroup -actorID $(ACTORID) -name \"%s\" -nodeAction \"add\" ", nodeGroup->GetName());
            outMetaDataString += AZStd::string::format("-nodeNames \"%s\" -enabledOnDefault \"%s\"\n", nodeNameList.c_str(), enabledOnDefault ? "true" : "false");
        }
    }


    void MetaData::GenerateLODMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString)
    {
        MCore::String tempString;
        const AZ::u32 numLODLevels = actor->GetNumLODLevels();
        if (numLODLevels > 1)
        {
            for (AZ::u32 lodLevel = 1; lodLevel < numLODLevels; ++lodLevel)
            {
                EMotionFX::Actor::LODCreationData* lodData = actor->GetLODCreationData(lodLevel);
                if (!lodData)
                {
                    // collision meshes
                    PrepareCollisionMeshesNodesString(actor, lodLevel, &tempString);
                    outMetaDataString += AZStd::string::format("ActorSetCollisionMeshes -actorID $(ACTORID) -lod %i -nodeList \"", lodLevel);
                    outMetaDataString += tempString;
                    outMetaDataString += "\"\n";
                    continue;
                }

                outMetaDataString += AZStd::string::format("AddLOD -actorID $(ACTORID) -addLastLODLevel true\n");

                EMotionFX::Actor::LODCreationData::EPreviewMode mode = lodData->mMode;
                switch (mode)
                {
                    case EMotionFX::Actor::LODCreationData::PREVIEWMODE_AUTOMATICLOD:
                    {
                        // construct the replace LOD command using the automatic LOD generation
                        MCore::String replaceLODCommand;
                        ConstructReplaceAutomaticLODCommand(actor, lodLevel, lodData->mInitSettings, lodData->mGenerateSettings, lodData->mEnabledNodes, &replaceLODCommand, true);
                        outMetaDataString += replaceLODCommand.AsChar();
                        break;
                    }

                    case EMotionFX::Actor::LODCreationData::PREVIEWMODE_MANUALLOD:
                    {
                        // construct the replace LOD command using the manual LOD method
                        MCore::String replaceLODCommand;
                        ConstructReplaceManualLODCommand(actor, lodLevel, lodData->mActorFileName.AsChar(), lodData->mEnabledNodes, &replaceLODCommand, true);

                        // replace the absolute path with the media root folder path
                        if (EMotionFX::GetEMotionFX().GetMediaRootFolderString().empty() == false)
                        {
                            replaceLODCommand.Replace(EMotionFX::GetEMotionFX().GetMediaRootFolder(), EMFX_MEDIAROOTFOLDER_STRING);
                        }

                        outMetaDataString += replaceLODCommand.AsChar();
                        break;
                    }

                    case EMotionFX::Actor::LODCreationData::PREVIEWMODE_NONE:
                    {
                        break;
                    }
                } // switch (mode)

                // collision meshes
                PrepareCollisionMeshesNodesString(actor, lodLevel, &tempString);
                outMetaDataString += AZStd::string::format("ActorSetCollisionMeshes -actorID $(ACTORID) -lod %i -nodeList \"", lodLevel);
                outMetaDataString += tempString;
                outMetaDataString += "\"\n";
            }
        }
    }


    void MetaData::GeneratePhonemeMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString)
    {
        const AZ::u32 numLODLevels = actor->GetNumLODLevels();
        for (AZ::u32 lodLevel = 0; lodLevel < numLODLevels; ++lodLevel)
        {
            EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(lodLevel);
            if (!morphSetup)
            {
                continue;
            }

            const uint32 numMorphTargets = morphSetup->GetNumMorphTargets();
            for (uint32 i = 0; i < numMorphTargets; ++i)
            {
                EMotionFX::MorphTarget* morphTarget = morphSetup->GetMorphTarget(i);
                if (!morphTarget)
                {
                    continue;
                }

                outMetaDataString += AZStd::string::format("AdjustMorphTarget -actorID $(ACTORID) -lodLevel %i -name \"%s\" -phonemeAction \"replace\" ", lodLevel, morphTarget->GetName());
                outMetaDataString += AZStd::string::format("-phonemeSets \"%s\" ", morphTarget->GetPhonemeSetString(morphTarget->GetPhonemeSets()).AsChar());
                outMetaDataString += AZStd::string::format("-rangeMin %f -rangeMax %f\n", morphTarget->GetRangeMin(), morphTarget->GetRangeMax());
            }
        }
    }


    void MetaData::GenerateAttachmentMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString)
    {
        AZStd::string attachmentNodeNameList;

        const AZ::u32 numNodes = actor->GetNumNodes();
        for (AZ::u32 i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            if (!node)
            {
                continue;
            }

            // Check if the node is an attachment node, if yes add it to our string node name list.
            if (node->GetIsAttachmentNode())
            {
                attachmentNodeNameList += node->GetName();
                attachmentNodeNameList += ";";
            }
        }

        outMetaDataString += AZStd::string::format("AdjustActor -actorID $(ACTORID) -nodeAction \"replace\" -attachmentNodes \"%s\"\n", attachmentNodeNameList.c_str());
    }


    void MetaData::GenerateMotionExtractionMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString)
    {
        EMotionFX::Node* motionExtractionNode = actor->GetMotionExtractionNode();
        if (motionExtractionNode)
        {
            outMetaDataString += "AdjustActor -actorID $(ACTORID) ";

            // Add the motion extraction node in case its set.
            if (motionExtractionNode)
            {
                outMetaDataString += AZStd::string::format("-motionExtractionNodeName \"%s\" ", motionExtractionNode->GetName());
            }

            outMetaDataString += "\n";
        }
    }


    void MetaData::GenerateMirrorSetupMetaData(EMotionFX::Actor* actor, AZStd::string& outMetaDataString)
    {
        if (actor->GetHasMirrorInfo())
        {
            outMetaDataString += "AdjustActor -actorID $(ACTORID) -mirrorSetup \"";

            for (uint32 i = 0; i < actor->GetNumNodes(); ++i)
            {
                const EMotionFX::Actor::NodeMirrorInfo& mirrorInfo = actor->GetNodeMirrorInfo(i);
                uint16 sourceNode = mirrorInfo.mSourceNode;
                if (sourceNode != MCORE_INVALIDINDEX16 && sourceNode != static_cast<uint16>(i))
                {
                    outMetaDataString += actor->GetSkeleton()->GetNode(i)->GetNameString();
                    outMetaDataString += ",";
                    outMetaDataString += actor->GetSkeleton()->GetNode(sourceNode)->GetNameString();
                    outMetaDataString += ";";
                }
            }

            outMetaDataString += "\"";
            outMetaDataString += "\n";
        }
    }


    AZStd::string MetaData::GenerateActorMetaData(EMotionFX::Actor* actor)
    {
        if (!actor)
        {
            AZ_Error("EMotionFX", false, "Cannot generate meta data for actor. Actor invalid.");
            return "";
        }

        AZStd::string metaDataString;

        // actor name
        metaDataString += AZStd::string::format("AdjustActor -actorID $(ACTORID) -name \"%s\"\n", actor->GetName());

        // collision mesh for LOD 0
        MCore::String tempString;
        PrepareCollisionMeshesNodesString(actor, 0, &tempString);
        metaDataString += AZStd::string::format("ActorSetCollisionMeshes -actorID $(ACTORID) -lod 0 -nodeList \"");
        metaDataString += tempString.AsChar();
        metaDataString += "\"\n";

        // nodes excluded from the bounding volume calculation
        PrepareExcludedNodesString(actor, &tempString);
        metaDataString += "AdjustActor -actorID $(ACTORID) -nodesExcludedFromBounds \"";
        metaDataString += tempString.AsChar();
        metaDataString += "\" -nodeAction \"select\"\n";


        GenerateNodeGroupMetaData(actor, metaDataString);
        GenerateLODMetaData(actor, metaDataString);
        GeneratePhonemeMetaData(actor, metaDataString);
        GenerateAttachmentMetaData(actor, metaDataString);
        GenerateMotionExtractionMetaData(actor, metaDataString);
        GenerateMirrorSetupMetaData(actor, metaDataString);

        return metaDataString;
    }


    bool MetaData::ApplyMetaDataOnActor(EMotionFX::Actor* actor, const AZStd::string& metaDataString)
    {
        return ApplyMetaData(actor->GetID(), "$(ACTORID)", metaDataString);
    }


    bool MetaData::ApplyMetaData(AZ::u32 objectId, const char* objectIdKeyword, AZStd::string metaDataString)
    {
        if (metaDataString.empty())
        {
            return true;
        }

        // Convert the runtime object id to a string.
        AZStd::string idString = AZStd::string::format("%i", objectId);

        // Replace all object id string occurrences (e.g. $(MOTIONID)) with the runtime object id (e.g. "54")
        AzFramework::StringFunc::Replace(metaDataString, objectIdKeyword, idString.c_str());

        // Tokenize by line delimiters.
        AZStd::vector<AZStd::string> tokens;
        AzFramework::StringFunc::Replace(metaDataString, "\r\n", "\n");
        AzFramework::StringFunc::Replace(metaDataString, '\r', '\n');
        AzFramework::StringFunc::Tokenize(metaDataString.c_str(), tokens, '\n');

        // Construct a new command group and fill it with all meta data commands.
        MCore::CommandGroup commandGroup;
        const size_t numTokens = tokens.size();
        for (size_t i = 0; i < numTokens; ++i)
        {
            commandGroup.AddCommandString(tokens[i].c_str());
        }

        // Execute the command group and apply the meta data.
        MCore::String outResult;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            if (outResult.GetIsEmpty() == false)
            {
                MCore::LogError(outResult.AsChar());
            }

            return false;
        }

        return true;
    }

} // namespace CommandSystem