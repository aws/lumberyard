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

// include the required headers
#include "LODCommands.h"
#include "CommandManager.h"
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/LODGenerator.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/ActorManager.h>
#include <AzFramework/API/ApplicationAPI.h>


namespace CommandSystem
{
    // 22 parameters
#define SYNTAX_LODCOMMANDS                                                                                                                                                                                                                                  \
    GetSyntax().AddRequiredParameter("actorID",        "The actor identification number of the actor to work on.", MCore::CommandSyntax::PARAMTYPE_INT);                                                                                                    \
    GetSyntax().AddParameter("curvatureFactor",        "",                                                         MCore::CommandSyntax::PARAMTYPE_FLOAT,      MCore::String(EMotionFX::LODGenerator::InitSettings().mCurvatureFactor).AsChar());           \
    GetSyntax().AddParameter("edgeLengthFactor",       "",                                                         MCore::CommandSyntax::PARAMTYPE_FLOAT,      MCore::String(EMotionFX::LODGenerator::InitSettings().mEdgeLengthFactor).AsChar());          \
    GetSyntax().AddParameter("skinningFactor",         "",                                                         MCore::CommandSyntax::PARAMTYPE_FLOAT,      MCore::String(EMotionFX::LODGenerator::InitSettings().mSkinningFactor).AsChar());            \
    GetSyntax().AddParameter("aabbThreshold",          "",                                                         MCore::CommandSyntax::PARAMTYPE_FLOAT,      MCore::String(EMotionFX::LODGenerator::InitSettings().mAABBThreshold).AsChar());             \
    GetSyntax().AddParameter("colorStrength",          "",                                                         MCore::CommandSyntax::PARAMTYPE_FLOAT,      MCore::String(EMotionFX::LODGenerator::InitSettings().mColorStrength).AsChar());             \
    GetSyntax().AddParameter("inputLOD",               "",                                                         MCore::CommandSyntax::PARAMTYPE_INT,        MCore::String(EMotionFX::LODGenerator::InitSettings().mInputLOD).AsChar());                  \
    GetSyntax().AddParameter("colorLayerSetIndex",     "",                                                         MCore::CommandSyntax::PARAMTYPE_INT,        MCore::String(EMotionFX::LODGenerator::InitSettings().mColorLayerSetIndex).AsChar());        \
    GetSyntax().AddParameter("useVertexColors",        "",                                                         MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    MCore::String(EMotionFX::LODGenerator::InitSettings().mUseVertexColors).AsChar());           \
    GetSyntax().AddParameter("preserveBorderEdges",    "",                                                         MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    MCore::String(EMotionFX::LODGenerator::InitSettings().mPreserveBorderEdges).AsChar());       \
    GetSyntax().AddParameter("randomizeSameCostVerts", "",                                                         MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    MCore::String(EMotionFX::LODGenerator::InitSettings().mRandomizeSameCostVerts).AsChar());    \
    GetSyntax().AddParameter("detailPercentage",       "",                                                         MCore::CommandSyntax::PARAMTYPE_FLOAT,      MCore::String(EMotionFX::LODGenerator::GenerateSettings().mDetailPercentage).AsChar());      \
    GetSyntax().AddParameter("maxBonesPerSubMesh",     "",                                                         MCore::CommandSyntax::PARAMTYPE_INT,        MCore::String(EMotionFX::LODGenerator::GenerateSettings().mMaxBonesPerSubMesh).AsChar());    \
    GetSyntax().AddParameter("maxVertsPerSubMesh",     "",                                                         MCore::CommandSyntax::PARAMTYPE_INT,        MCore::String(EMotionFX::LODGenerator::GenerateSettings().mMaxVertsPerSubMesh).AsChar());    \
    GetSyntax().AddParameter("maxSkinInfluences",      "",                                                         MCore::CommandSyntax::PARAMTYPE_INT,        MCore::String(EMotionFX::LODGenerator::GenerateSettings().mMaxSkinInfluences).AsChar());     \
    GetSyntax().AddParameter("weightRemoveThreshold",  "",                                                         MCore::CommandSyntax::PARAMTYPE_FLOAT,      MCore::String(EMotionFX::LODGenerator::GenerateSettings().mWeightRemoveThreshold).AsChar()); \
    GetSyntax().AddParameter("generateNormals",        "",                                                         MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    MCore::String(EMotionFX::LODGenerator::GenerateSettings().mGenerateNormals).AsChar());       \
    GetSyntax().AddParameter("smoothNormals",          "",                                                         MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    MCore::String(EMotionFX::LODGenerator::GenerateSettings().mSmoothNormals).AsChar());         \
    GetSyntax().AddParameter("generateTangents",       "",                                                         MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    MCore::String(EMotionFX::LODGenerator::GenerateSettings().mGenerateTangents).AsChar());      \
    GetSyntax().AddParameter("optimizeTriList",        "",                                                         MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    MCore::String(EMotionFX::LODGenerator::GenerateSettings().mOptimizeTriList).AsChar());       \
    GetSyntax().AddParameter("dualQuatSkinning",       "",                                                         MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    MCore::String(EMotionFX::LODGenerator::GenerateSettings().mDualQuatSkinning).AsChar());      \
    GetSyntax().AddParameter("excludedMorphTargets",   "",                                                         MCore::CommandSyntax::PARAMTYPE_STRING,     "");



    //--------------------------------------------------------------------------------
    // CommandAddLOD
    //--------------------------------------------------------------------------------

    // constructor
    CommandAddLOD::CommandAddLOD(MCore::Command* orgCommand)
        : MCore::Command("AddLOD", orgCommand)
    {
    }


    // destructor
    CommandAddLOD::~CommandAddLOD()
    {
    }


    // execute
    bool CommandAddLOD::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // find the actor based on the given id
        const uint32 actorID = parameters.GetValueAsInt("actorID", this);
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult.Format("Cannot execute LOD command. Actor ID %i is not valid.", actorID);
            return false;
        }

        // get the LOD level to insert at
        const uint32 lodLevel = MCore::Clamp<uint32>(parameters.GetValueAsInt("lodLevel", this), 1, actor->GetNumLODLevels());

        EMotionFX::Actor::LODCreationData*  lodData = nullptr;

        // manual LOD mode?
        if (parameters.CheckIfHasParameter("actorFileName"))
        {
            // get the filename of the LOD actor
            MCore::String lodFileName;
            parameters.GetValue("actorFileName", this, &lodFileName);

            // load the LOD actor
            EMotionFX::Actor* lodActor = EMotionFX::GetImporter().LoadActor(lodFileName.AsChar());
            if (lodActor == nullptr)
            {
                outResult.Format("Cannot execute LOD command. Loading LOD actor from file '%s' failed.", lodFileName.AsChar());
                return false;
            }

            // replace the given LOD level with the lod actor and remove the LOD actor object from memory afterwards
            actor->CopyLODLevel(lodActor, 0, lodLevel, false);

            // set the mode to manual LOD
            lodData                 = actor->GetLODCreationData(lodLevel);
            lodData->mActorFileName = lodFileName;
            lodData->mMode          = EMotionFX::Actor::LODCreationData::PREVIEWMODE_MANUALLOD;
            lodData->mPreviewMode   = EMotionFX::Actor::LODCreationData::PREVIEWMODE_NONE;
        }
        // add a copy of the last LOD level to the end?
        else if (parameters.CheckIfHasParameter("addLastLODLevel"))
        {
            const bool copyAddLODLevel = parameters.GetValueAsBool("addLastLODLevel", this);
            if (copyAddLODLevel)
            {
                actor->AddLODLevel();
            }

            // set the mode to none as we added an empty or a copy of the last LOD
            lodData                 = actor->GetLODCreationData(actor->GetNumLODLevels() - 1);
            lodData->mMode          = EMotionFX::Actor::LODCreationData::PREVIEWMODE_NONE;
            lodData->mPreviewMode   = EMotionFX::Actor::LODCreationData::PREVIEWMODE_NONE;
        }
        // move/insert/copy a LOD level
        else if (parameters.CheckIfHasParameter("insertAt"))
        {
            int32 insertAt = parameters.GetValueAsInt("insertAt", this);
            int32 copyFrom = parameters.GetValueAsInt("copyFrom", this);

            actor->InsertLODLevel(insertAt);

            // in case we inserted our new LOD level
            if (insertAt < copyFrom)
            {
                copyFrom++;
            }

            actor->CopyLODLevel(actor, copyFrom, insertAt, true, false, true);

            // enable or disable nodes based on the skeletal LOD flags
            const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
            for (uint32 j = 0; j < numActorInstances; ++j)
            {
                EMotionFX::GetActorManager().GetActorInstance(j)->UpdateSkeletalLODFlags();
            }
        }
        // automatic LOD generation
        else
        {
            // get the init settings from the parameters
            EMotionFX::LODGenerator::InitSettings initSettings = ConstructInitSettings(this, parameters);

            // get the morph setup based on the input LOD level set in the init settings
            EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(initSettings.mInputLOD);

            // get the generate settings from the parameters
            EMotionFX::LODGenerator::GenerateSettings generateSettings = ConstructGenerateSettings(this, parameters, morphSetup);

            // always optimize the triangle list when baking the LOD to the actor
            generateSettings.mOptimizeTriList = true;

            // initialize the LOD generator
            EMotionFX::LODGenerator* lodGenerator = EMotionFX::LODGenerator::Create();
            lodGenerator->Init(actor, initSettings);

            // generate the meshes
            lodGenerator->GenerateMeshes(generateSettings);

            // replace the lod Level with the newly generated one
            lodGenerator->ReplaceLODLevelWithLastGeneratedActor(actor, lodLevel);

            // set the mode to automatic LOD
            lodData                     = actor->GetLODCreationData(lodLevel);
            lodData->mInitSettings      = initSettings;
            lodData->mGenerateSettings  = generateSettings;
            lodData->mMode              = EMotionFX::Actor::LODCreationData::PREVIEWMODE_AUTOMATICLOD;
            lodData->mPreviewMode       = EMotionFX::Actor::LODCreationData::PREVIEWMODE_NONE;

            // get the enabled morph targets and add them to the LOD data
            if (morphSetup)
            {
                // get the number of morph targets
                const uint32 numMorphTargets = morphSetup->GetNumMorphTargets();

                // clear the enabled morph targets array
                lodData->mEnabledMorphTargets.Clear();
                lodData->mEnabledMorphTargets.Reserve(numMorphTargets);

                // iterate through the morph targets and add the enabled ones to the LOD data
                for (uint32 i = 0; i < numMorphTargets; ++i)
                {
                    // get the current morph target and its id
                    EMotionFX::MorphTarget* morphTarget = morphSetup->GetMorphTarget(i);
                    const uint32    morphTargetID       = morphTarget->GetID();

                    // in case it is not in the exluded array, add it to the enabled morph targets array
                    if (lodData->mGenerateSettings.mMorphTargetIDsToRemove.Find(morphTargetID) == MCORE_INVALIDINDEX32)
                    {
                        lodData->mEnabledMorphTargets.Add(morphTargetID);
                    }
                }
            }

            lodGenerator->Destroy();
        }

        // check if parametes skeletal LOD node names is set
        if (parameters.CheckIfHasParameter("skeletalLOD"))
        {
            const uint32 numNodes = actor->GetNumNodes();
            // get the node names of the currently enabled nodes for the skeletal LOD
            mOldSkeletalLOD.Clear();
            mOldSkeletalLOD.Reserve(16448);
            for (uint32 i = 0; i < numNodes; ++i)
            {
                EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);

                // check if the current node is enabled in the given skeletal LOD level and add it to the undo information
                if (node->GetSkeletalLODStatus(lodLevel))
                {
                    mOldSkeletalLOD += node->GetName();
                    mOldSkeletalLOD += ";";
                }
            }
            mOldSkeletalLOD.TrimRight(MCore::UnicodeCharacter(';'));

            // get the node names for the skeletal LOD and split the string
            MCore::String skeletalLODString;
            skeletalLODString.Reserve(16448);
            parameters.GetValue("skeletalLOD", this, &skeletalLODString);

            // get the individual node names
            MCore::Array<MCore::String> nodeNames = skeletalLODString.Split(MCore::UnicodeCharacter(';'));

            // reset the enabled nodes in the LOD data
            if (lodData)
            {
                lodData->mEnabledNodes.Clear();
                lodData->mEnabledNodes.Reserve(numNodes);
            }

            // iterate through the nodes and set the skeletal LOD flag based on the input parameter
            for (uint32 i = 0; i < numNodes; ++i)
            {
                EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);

                // check if this node is enabled in the parent skeletal LOD level and only allow the node to be enabled in the current skeletal LOD level in case it is also enabled in the parent LOD level
                /*bool parendLODEnabled = true;
                if (lodLevel > 0)
                    parendLODEnabled = node->GetSkeletalLODStatus(lodLevel-1);*/

                // check if the given node is in the parameter where the enabled nodes are listed, if yes enable the skeletal LOD flag, disable if not as well as
                // check if the node is enabled in the parent LOD level and only allow enabling
                if (/*parendLODEnabled && */ nodeNames.Find(node->GetName()) != MCORE_INVALIDINDEX32)
                {
                    node->SetSkeletalLODStatus(lodLevel, true);

                    // add the enabled node to the LOD data
                    if (lodData)
                    {
                        lodData->mEnabledNodes.Add(node->GetID());
                    }
                }
                else
                {
                    node->SetSkeletalLODStatus(lodLevel, false);
                }
            }

            // enable or disable nodes based on the skeletal LOD flags
            const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
            for (uint32 j = 0; j < numActorInstances; ++j)
            {
                EMotionFX::GetActorManager().GetActorInstance(j)->UpdateSkeletalLODFlags();
            }

            // adjust the skins and remove all weights to nodes
            actor->MakeGeomLODsCompatibleWithSkeletalLODs();
        }

        // reinit our render actors
        MCore::String reinitRenderActorsResult;
        GetCommandManager()->ExecuteCommandInsideCommand("ReInitRenderActors -resetViewCloseup false", reinitRenderActorsResult);

        // save the current dirty flag and tell the actor that something got changed
        mOldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);

        return true;
    }


    // undo the command
    bool CommandAddLOD::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        /*String commandString;
        //commandString.Format("RemoveLOD -actorID %i -lodLevel %i", );
        if (GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult, false) == false)
        {
            if (outResult.GetIsEmpty() == false)
                LogError( outResult.AsChar() );

            return false;
        }

        // set the dirty flag back to the old value
        actor->SetDirtyFlag(mOldDirtyFlag);*/
        return true;
    }


    // init the syntax of the command
    void CommandAddLOD::InitSyntax()
    {
        GetSyntax().ReserveParameters(22 + 4);
        SYNTAX_LODCOMMANDS
            GetSyntax().AddParameter("actorFileName",   "",                                                                 MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("lodLevel",        "",                                                                 MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("insertAt",        "",                                                                 MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("copyFrom",        "",                                                                 MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("addLastLODLevel", "",                                                                 MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
        GetSyntax().AddParameter("skeletalLOD",     "A list of nodes that will be used to adjust the skeletal LOD.",    MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    // get the description
    const char* CommandAddLOD::GetDescription() const
    {
        return "This command can be used to add a new LOD level.";
    }


    //--------------------------------------------------------------------------------
    // CommandRemoveGeometryLOD
    //--------------------------------------------------------------------------------

    // constructor
    CommandRemoveLOD::CommandRemoveLOD(MCore::Command* orgCommand)
        : MCore::Command("RemoveLOD", orgCommand)
    {
    }


    // destructor
    CommandRemoveLOD::~CommandRemoveLOD()
    {
    }


    // execute
    bool CommandRemoveLOD::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCore::String commandResult;

        // find the actor based on the given id
        const uint32 actorID = parameters.GetValueAsInt("actorID", this);
        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult.Format("Cannot execute LOD command. Actor ID %i is not valid.", actorID);
            return false;
        }

        // get the LOD level to work on
        const uint32 lodLevel = parameters.GetValueAsInt("lodLevel", this);
        if (lodLevel >= actor->GetNumLODLevels())
        {
            outResult.Format("Cannot execute LOD command. Actor only has %d LOD levels (valid indices are [0, %d]) while the operation wanted to work on LOD level %d.", actor->GetNumLODLevels(), actor->GetNumLODLevels() - 1, lodLevel);
            return false;
        }

        // check if there is a LOD level to remove
        if (actor->GetNumLODLevels() <= 1)
        {
            outResult = "Cannot remove LOD level 0.";
            return false;
        }

        // remove the LOD level from the actor
        actor->RemoveLODLevel(lodLevel);

        // iterate over all actor instances from the given actor and make sure they have a valid LOD level set
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // get the actor instance and check if it belongs to the given actor
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);
            if (actorInstance->GetActor() != actor)
            {
                continue;
            }

            // make sure the LOD level is valid
            if (actorInstance->GetLODLevel() >= actor->GetNumLODLevels())
            {
                actorInstance->SetLODLevel(actor->GetNumLODLevels() - 1);
            }
        }

        // reinit our render actors
        GetCommandManager()->ExecuteCommandInsideCommand("ReInitRenderActors -resetViewCloseup false", commandResult);

        // save the current dirty flag and tell the actor that something got changed
        mOldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }


    // undo the command
    bool CommandRemoveLOD::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // init the syntax of the command
    void CommandRemoveLOD::InitSyntax()
    {
        GetSyntax().ReserveParameters(2);
        GetSyntax().AddRequiredParameter("actorID",     "The actor identification number of the actor to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("lodLevel",    "",                                                         MCore::CommandSyntax::PARAMTYPE_INT);
    }


    // get the description
    const char* CommandRemoveLOD::GetDescription() const
    {
        return "This command can be used to remove the given LOD level.";
    }


    //--------------------------------------------------------------------------------
    // Helper functions
    //--------------------------------------------------------------------------------

    // get the init settings from the command parameters
    EMotionFX::LODGenerator::InitSettings ConstructInitSettings(MCore::Command* command, const MCore::CommandLine& parameters)
    {
        EMotionFX::LODGenerator::InitSettings initSettings;

        if (parameters.CheckIfHasParameter("curvatureFactor"))
        {
            initSettings.mCurvatureFactor = parameters.GetValueAsFloat("curvatureFactor", command);
        }
        if (parameters.CheckIfHasParameter("edgeLengthFactor"))
        {
            initSettings.mEdgeLengthFactor = parameters.GetValueAsFloat("edgeLengthFactor", command);
        }
        if (parameters.CheckIfHasParameter("skinningFactor"))
        {
            initSettings.mSkinningFactor = parameters.GetValueAsFloat("skinningFactor", command);
        }
        if (parameters.CheckIfHasParameter("aabbThreshold"))
        {
            initSettings.mAABBThreshold = parameters.GetValueAsFloat("aabbThreshold", command);
        }
        if (parameters.CheckIfHasParameter("colorStrength"))
        {
            initSettings.mColorStrength = parameters.GetValueAsFloat("colorStrength", command);
        }
        if (parameters.CheckIfHasParameter("inputLOD"))
        {
            initSettings.mInputLOD = parameters.GetValueAsInt("inputLOD", command);
        }
        if (parameters.CheckIfHasParameter("colorLayerSetIndex"))
        {
            initSettings.mColorLayerSetIndex = parameters.GetValueAsInt("colorLayerSetIndex", command);
        }
        if (parameters.CheckIfHasParameter("useVertexColors"))
        {
            initSettings.mUseVertexColors = parameters.GetValueAsBool("useVertexColors", command);
        }
        if (parameters.CheckIfHasParameter("preserveBorderEdges"))
        {
            initSettings.mPreserveBorderEdges = parameters.GetValueAsBool("preserveBorderEdges", command);
        }
        if (parameters.CheckIfHasParameter("randomizeSameCostVerts"))
        {
            initSettings.mRandomizeSameCostVerts = parameters.GetValueAsBool("randomizeSameCostVerts", command);
        }

        return initSettings;
    }


    // get the generate settings from the command parameters
    EMotionFX::LODGenerator::GenerateSettings ConstructGenerateSettings(MCore::Command* command, const MCore::CommandLine& parameters, EMotionFX::MorphSetup* morphSetup)
    {
        EMotionFX::LODGenerator::GenerateSettings generateSettings;

        if (parameters.CheckIfHasParameter("detailPercentage"))
        {
            generateSettings.mDetailPercentage = parameters.GetValueAsFloat("detailPercentage", command);
        }
        if (parameters.CheckIfHasParameter("maxBonesPerSubMesh"))
        {
            generateSettings.mMaxBonesPerSubMesh = parameters.GetValueAsInt("maxBonesPerSubMesh", command);
        }
        if (parameters.CheckIfHasParameter("maxVertsPerSubMesh"))
        {
            generateSettings.mMaxVertsPerSubMesh = parameters.GetValueAsInt("maxVertsPerSubMesh", command);
        }
        if (parameters.CheckIfHasParameter("maxSkinInfluences"))
        {
            generateSettings.mMaxSkinInfluences = parameters.GetValueAsInt("maxSkinInfluences", command);
        }
        if (parameters.CheckIfHasParameter("weightRemoveThreshold"))
        {
            generateSettings.mWeightRemoveThreshold = parameters.GetValueAsFloat("weightRemoveThreshold", command);
        }
        if (parameters.CheckIfHasParameter("generateNormals"))
        {
            generateSettings.mGenerateNormals = parameters.GetValueAsBool("generateNormals", command);
        }
        if (parameters.CheckIfHasParameter("smoothNormals"))
        {
            generateSettings.mSmoothNormals = parameters.GetValueAsBool("smoothNormals", command);
        }
        if (parameters.CheckIfHasParameter("generateTangents"))
        {
            generateSettings.mGenerateTangents = parameters.GetValueAsBool("generateTangents", command);
        }
        if (parameters.CheckIfHasParameter("optimizeTriList"))
        {
            generateSettings.mOptimizeTriList = parameters.GetValueAsBool("optimizeTriList", command);
        }
        if (parameters.CheckIfHasParameter("dualQuatSkinning"))
        {
            generateSettings.mDualQuatSkinning = parameters.GetValueAsBool("dualQuatSkinning", command);
        }

        // special handling for the excluded morph targets
        if (parameters.CheckIfHasParameter("excludedMorphTargets"))
        {
            // get the parameter as string
            MCore::String excludedMTString;
            parameters.GetValue("excludedMorphTargets", command, &excludedMTString);

            // split the morph target names
            MCore::Array<MCore::String> excludedMTNames = excludedMTString.Split(MCore::UnicodeCharacter(';'));

            // iterate the morph target names and convert them to ids
            const uint32 numMorphTargets = excludedMTNames.GetLength();
            for (uint32 i = 0; i < numMorphTargets; ++i)
            {
                EMotionFX::MorphTarget* morphTarget = morphSetup->FindMorphTargetByName(excludedMTNames[i].AsChar());
                if (morphTarget == nullptr)
                {
                    MCore::LogWarning("Cannot get generate settings from command parameter. Morph target '%s' not found in the given morph setup.", excludedMTNames[i].AsChar());
                    continue;
                }

                // add the morph target to the excluded morph targets array in the generate settings
                generateSettings.mMorphTargetIDsToRemove.Add(morphTarget->GetID());
            }
        }

        return generateSettings;
    }


    // construct command parameters from the given init settings
    void ConstructInitSettingsCommandParameters(const EMotionFX::LODGenerator::InitSettings& initSettings, MCore::String* outString)
    {
        outString->Reserve(16384);
        outString->FormatAdd(" -curvatureFactor %f",    initSettings.mCurvatureFactor);
        outString->FormatAdd(" -edgeLengthFactor %f",   initSettings.mEdgeLengthFactor);
        outString->FormatAdd(" -skinningFactor %f",     initSettings.mSkinningFactor);
        outString->FormatAdd(" -aabbThreshold %f",      initSettings.mAABBThreshold);
        outString->FormatAdd(" -colorStrength %f",      initSettings.mColorStrength);
        outString->FormatAdd(" -inputLOD %d",           initSettings.mInputLOD);
        outString->FormatAdd(" -colorLayerSetIndex %d", initSettings.mColorLayerSetIndex);
        outString->FormatAdd(" -useVertexColors %s",    initSettings.mUseVertexColors ? "true" : "false");
        outString->FormatAdd(" -preserveBorderEdges %s", initSettings.mPreserveBorderEdges ? "true" : "false");
        outString->FormatAdd(" -randomizeSameCostVerts %s", initSettings.mRandomizeSameCostVerts ? "true" : "false");
    }


    // construct command parameters from the given generate settings
    void ConstructGenerateSettingsCommandParameters(const EMotionFX::LODGenerator::GenerateSettings& generateSettings, MCore::String* outString, EMotionFX::MorphSetup* morphSetup)
    {
        outString->Reserve(16384);
        outString->FormatAdd(" -detailPercentage %f",       generateSettings.mDetailPercentage);
        outString->FormatAdd(" -maxBonesPerSubMesh %d",     generateSettings.mMaxBonesPerSubMesh);
        outString->FormatAdd(" -maxVertsPerSubMesh %d",     generateSettings.mMaxVertsPerSubMesh);
        outString->FormatAdd(" -maxSkinInfluences %d",      generateSettings.mMaxSkinInfluences);
        outString->FormatAdd(" -weightRemoveThreshold %f",  generateSettings.mWeightRemoveThreshold);
        outString->FormatAdd(" -generateNormals %s",        generateSettings.mGenerateNormals ? "true" : "false");
        outString->FormatAdd(" -smoothNormals %s",          generateSettings.mSmoothNormals ? "true" : "false");
        outString->FormatAdd(" -generateTangents %s",       generateSettings.mGenerateTangents ? "true" : "false");
        outString->FormatAdd(" -optimizeTriList %s",        generateSettings.mOptimizeTriList ? "true" : "false");
        outString->FormatAdd(" -dualQuatSkinning %s",       generateSettings.mDualQuatSkinning ? "true" : "false");

        // only add the excluded morph targets parameter in case the array is not empty
        if (generateSettings.mMorphTargetIDsToRemove.GetIsEmpty() == false)
        {
            outString->FormatAdd(" -excludedMorphTargets \"");

            // get the number of excluded morph targets and iterate through them
            const uint32 numExcludedMorphTargets = generateSettings.mMorphTargetIDsToRemove.GetLength();
            for (uint32 i = 0; i < numExcludedMorphTargets; ++i)
            {
                const uint32 morphTargetID = generateSettings.mMorphTargetIDsToRemove[i];
                EMotionFX::MorphTarget* morphTarget = morphSetup->FindMorphTargetByID(morphTargetID);
                if (morphTarget)
                {
                    outString->FormatAdd("%s;", morphTarget->GetName());
                }
                else
                {
                    MCore::LogWarning("Cannot construct parameter for LOD command. Morph target with id %d not found in the given morph setup.", morphTargetID);
                }
            }

            outString->TrimRight(MCore::UnicodeCharacter(';'));
            outString->FormatAdd("\"");
        }
    }


    // remove all LOD levels from an actor
    void ClearLODLevels(EMotionFX::Actor* actor, MCore::CommandGroup* commandGroup)
    {
        // get the number of LOD levels and return directly in case there only is the original
        const uint32 numLODLevels = actor->GetNumLODLevels();
        if (numLODLevels == 1)
        {
            return;
        }

        // create the command group
        MCore::String command;
        MCore::CommandGroup internalCommandGroup("Clear LOD levels");

        // iterate through the LOD levels and remove them back to front
        for (uint32 lodLevel = numLODLevels - 1; lodLevel > 0; --lodLevel)
        {
            // construct the command to remove the given LOD level
            command.Format("RemoveLOD -actorID %d -lodLevel %d", actor->GetID(), lodLevel);

            // add the command to the command group
            if (commandGroup == nullptr)
            {
                internalCommandGroup.AddCommandString(command.AsChar());
            }
            else
            {
                commandGroup->AddCommandString(command.AsChar());
            }
        }

        // execute the command or add it to the given command group
        if (commandGroup == nullptr)
        {
            MCore::String resultString;
            if (GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, resultString) == false)
            {
                MCore::LogError(resultString.AsChar());
            }
        }
    }


    void PrepareSkeletalLODParameter(EMotionFX::Actor* actor, uint32 lodLevel, const MCore::Array<uint32>& enabledNodeIDs, MCore::String* outString)
    {
        MCORE_UNUSED(lodLevel);

        // skeletal LOD
        outString->FormatAdd(" -skeletalLOD \"");
        const uint32 numEnabledNodes = enabledNodeIDs.GetLength();
        for (uint32 n = 0; n < numEnabledNodes; ++n)
        {
            // find the node by id
            EMotionFX::Node* node = actor->GetSkeleton()->FindNodeByID(enabledNodeIDs[n]);
            if (node == nullptr)
            {
                continue;
            }

            outString->FormatAdd("%s;", node->GetName());
        }
        outString->FormatAdd("\"");
    }


    // construct the replace LOD command using the automatic LOD generation
    void ConstructReplaceAutomaticLODCommand(EMotionFX::Actor* actor, uint32 lodLevel, const EMotionFX::LODGenerator::InitSettings& initSettings, const EMotionFX::LODGenerator::GenerateSettings& generateSettings, const MCore::Array<uint32>& enabledNodeIDs, MCore::String* outString, bool useForMetaData)
    {
        // clear the command string, reserve space and start filling it
        outString->Clear();
        outString->Reserve(16384);

        if (useForMetaData == false)
        {
            outString->Format("AddLOD -actorID %i -lodLevel %i", actor->GetID(), lodLevel);
        }
        else
        {
            outString->Format("AddLOD -actorID $(ACTORID) -lodLevel %i", lodLevel);
        }

        // construct the init settings command parameters
        ConstructInitSettingsCommandParameters(initSettings, outString);

        // get the morph setup based on the input LOD level set in the init settings
        EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(initSettings.mInputLOD);

        // construct the init settings command parameters
        ConstructGenerateSettingsCommandParameters(generateSettings, outString, morphSetup);

        // skeletal LOD
        PrepareSkeletalLODParameter(actor, lodLevel, enabledNodeIDs, outString);

        // add a new line for meta data usage
        if (useForMetaData)
        {
            outString->FormatAdd("\n");
        }
    }


    // construct the replace LOD command using the manual LOD method
    void ConstructReplaceManualLODCommand(EMotionFX::Actor* actor, uint32 lodLevel, const char* lodActorFileName, const MCore::Array<uint32>& enabledNodeIDs, MCore::String* outString, bool useForMetaData)
    {
        // clear the command string, reserve space and start filling it
        outString->Clear();
        outString->Reserve(16384);

        AZStd::string nativeFileName = lodActorFileName;
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, nativeFileName);

        if (useForMetaData == false)
        {
            outString->Format("AddLOD -actorID %i -lodLevel %i -actorFileName \"%s\"", actor->GetID(), lodLevel, nativeFileName.c_str());
        }
        else
        {
            outString->Format("AddLOD -actorID $(ACTORID) -lodLevel %i -actorFileName \"%s\"", lodLevel, nativeFileName.c_str());
        }

        // skeletal LOD
        PrepareSkeletalLODParameter(actor, lodLevel, enabledNodeIDs, outString);

        // add a new line for meta data usage
        if (useForMetaData)
        {
            outString->FormatAdd("\n");
        }
    }
} // namespace CommandSystem
