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
#include <ROP_Blast.h>

#include <BlastHelpers.h>

#include <GA/GA_Detail.h>
#include <GA/GA_Primitive.h>
#include <GU/GU_Detail.h>
#include <GU/GU_DetailHandle.h>
#include <OP/OP_Director.h>
#include <OP/OP_Network.h>
#include <OP/OP_NodeInfoParms.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <ROP/ROP_Error.h>
#include <ROP/ROP_Templates.h>
#include <SOP/SOP_Node.h>
#include <UT/UT_InfoTree.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_StringHolder.h>

#include <NvBlast.h>
#include <NvBlastExtAuthoring.h>
#include <NvBlastExtAuthoringBondGenerator.h>
#include <NvBlastExtPxAsset.h>
#include <NvBlastExtPxCollisionBuilder.h>
#include <NvBlastExtPxManager.h>
#include <NvBlastExtSerialization.h>
#include <NvBlastExtLlSerialization.h>
#include <NvBlastExtTkSerialization.h>
#include <NvBlastExtPxSerialization.h>
#include <NvBlastIndexFns.h>
#include <NvBlastPxCallbacks.h>
#include <NvBlastTkAsset.h>
#include <NvBlastTypes.h>

#include <PxPhysicsAPI.h>

#include <iostream>
#include <fstream>
#include <unordered_map>

static const char* outputFileKey = "outputFile";
static const char* makePathKey = "mkpath";
static const char* rootNameKey = "rootName";
static const char* rootIsStaticKey = "rootIsStatic";
static const char* blastAssetFormatKey = "blastAssetFormat";
static const char* maxSeparationKey = "maxSeparation";
static const char* maxConvexHullsKey = "maxConvexHulls";

PRM_Template* ROP_Blast::getTemplates()
{
    static std::vector<PRM_Template> templates;
    if (templates.empty())
    {
        static PRM_Name outputFile(outputFileKey, "Output File");
        static PRM_Default outputFileDefault(0, "$HIP/fracture.blast");
        static PRM_Name ropDoRender("execute", "Save to Disk");

        static PRM_Name rootName(rootNameKey, "Root Name");
        static PRM_Default rootNameDefault(0, "root");

        static PRM_Name rootIsStatic(rootIsStaticKey, "Static root");
        static PRM_Default rootIsStaticDefault(1);
        
        static PRM_Name blastAssetFormatName(blastAssetFormatKey, "Blast Asset Format");
        static PRM_Name	blastAssetFormats[] =
        {
            PRM_Name("lowLevelAsset", "Blast low-level asset"),
            PRM_Name("tkAsset", "Blast Tk asset"),
            PRM_Name("pxAsset", "Blast PhysX asset"),
            PRM_Name(0),
        };
        static PRM_ChoiceList blastAssetFormatMenu((PRM_ChoiceListType)(PRM_CHOICELIST_EXCLUSIVE | PRM_CHOICELIST_REPLACE), blastAssetFormats);

        static PRM_Name maxSeparationName(maxSeparationKey, "Max separation between chunks");
        static PRM_Default maxSeparationDefault(0, "0.001");

        static PRM_Name maxConvexHullsName(maxConvexHullsKey, "Maximum number of convex hulls per chunk");
        static PRM_Default maxConvexHullsDefault(1);


        templates.emplace_back(PRM_FILE, 1, &outputFile, &outputFileDefault);
        templates.emplace_back(theRopTemplates[ROP_MKPATH_TPLATE]);
        templates.emplace_back(PRM_STRING, 1, &rootName, &rootNameDefault);
        templates.emplace_back(PRM_TOGGLE, 1, &rootIsStatic, &rootIsStaticDefault);
        templates.emplace_back(PRM_ORD, PRM_Template::PRM_EXPORT_TBX, 1, &blastAssetFormatName, nullptr, &blastAssetFormatMenu);
        templates.emplace_back(PRM_FLT_E, 1, &maxSeparationName, &maxSeparationDefault);
        templates.emplace_back(PRM_INT_E, 1, &maxConvexHullsName, &maxConvexHullsDefault);
        templates.emplace_back(theRopTemplates[ROP_TPRERENDER_TPLATE]);
        templates.emplace_back(theRopTemplates[ROP_PRERENDER_TPLATE]);
        templates.emplace_back(theRopTemplates[ROP_LPRERENDER_TPLATE]);
        templates.emplace_back(theRopTemplates[ROP_TPREFRAME_TPLATE]);
        templates.emplace_back(theRopTemplates[ROP_PREFRAME_TPLATE]);
        templates.emplace_back(theRopTemplates[ROP_LPREFRAME_TPLATE]);
        templates.emplace_back(theRopTemplates[ROP_TPOSTFRAME_TPLATE]);
        templates.emplace_back(theRopTemplates[ROP_POSTFRAME_TPLATE]);
        templates.emplace_back(theRopTemplates[ROP_LPOSTFRAME_TPLATE]);
        templates.emplace_back(theRopTemplates[ROP_TPOSTRENDER_TPLATE]);
        templates.emplace_back(theRopTemplates[ROP_POSTRENDER_TPLATE]);
        templates.emplace_back(theRopTemplates[ROP_LPOSTRENDER_TPLATE]);
        templates.emplace_back();
    }

    return templates.data();
}

OP_TemplatePair* ROP_Blast::getTemplatePair()
{
    static OP_TemplatePair* ropPair = nullptr;
    if (ropPair == nullptr)
    {
        OP_TemplatePair* base;

        base = new OP_TemplatePair(getTemplates());
        ropPair = new OP_TemplatePair(ROP_Node::getROPbaseTemplate(), base);
    }
    return ropPair;
}

OP_VariablePair* ROP_Blast::getVariablePair()
{
    static OP_VariablePair* pair = nullptr;
    if (pair == nullptr)
    {
        pair = new OP_VariablePair(ROP_Node::myVariableList);
    }
    return pair;
}

OP_Node* ROP_Blast::myConstructor(OP_Network *net, const char *name, OP_Operator *op)
{
    return new ROP_Blast(net, name, op);
}

ROP_Blast::ROP_Blast(OP_Network *net, const char *name, OP_Operator *entry)
    : ROP_Node(net, name, entry)
{

}

ROP_Blast::~ROP_Blast()
{
}

bool ROP_Blast::initPhysX()
{
    m_foundation.reset(PxCreateFoundation(PX_PHYSICS_VERSION, NvBlastGetPxAllocatorCallback(), NvBlastGetPxErrorCallback()));
    if (m_foundation == nullptr)
    {
        addError(ROP_MESSAGE, "Can't init PxFoundation");
        return false;
    }

    physx::PxTolerancesScale scale;
    m_physics.reset(PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, scale, true));
    if (m_physics == nullptr)
    {
        addError(ROP_MESSAGE, "Can't create PxPhysics");
        return false;
    }

    physx::PxCookingParams cookingParams(scale);
    cookingParams.buildGPUData = true;
    m_cooking.reset(PxCreateCooking(PX_PHYSICS_VERSION, m_physics->getFoundation(), cookingParams));
    if (m_cooking == nullptr)
    {
        addError(ROP_MESSAGE, "Can't create PxCooking");
        return false;
    }

    m_collisionBuilder.reset(Nv::Blast::ExtPxManager::createCollisionBuilder(*m_physics, *m_cooking));
    if (m_collisionBuilder == nullptr)
    {
        addError(ROP_MESSAGE, "Can't create ExtPxCollisionBuilder");
        return false;
    }

    return true;
}

int ROP_Blast::startRender(int frameCount, fpreal startTime, fpreal endTime)
{
    m_endTime = endTime;
    if (error() < UT_ERROR_ABORT)
    {
        executePreRenderScript(startTime);
    }

    if (!initPhysX())
    {
        return ROP_ABORT_RENDER;
    }

    return 1;
}

// Comments in other HDK code seem to indicate this is required to properly cook subnets
class OPAutoCookRender
{
public:
    OPAutoCookRender(OP_Node* opNode)
        : m_node(opNode)
    {
        if (m_node != nullptr)
        {
            m_prevStatus = m_node->isCookingRender();
            m_node->setCookingRender(1);
        }
    }
    ~OPAutoCookRender()
    {
        if (m_node != nullptr)
        {
            m_node->setCookingRender(m_prevStatus);
        }
    }
private:
    OP_Node* m_node;
    int m_prevStatus;
};

ROP_RENDER_CODE ROP_Blast::renderFrame(fpreal time, UT_Interrupt* boss)
{
    // Execute the pre-render script.
    executePreFrameScript(time);

    if (boss == nullptr)
    {
        boss = UTgetInterrupt();
    }

    // Get our geometry detail
    SOP_Node* sopNode = CAST_SOPNODE(getInput(0));
    OP_Context context(time);

    if (sopNode == nullptr)
    {
        addError(ROP_MESSAGE, "No SOP node connected");
        return ROP_ABORT_RENDER;
    }

    OPAutoCookRender autoCook(sopNode);
    GU_DetailHandle detailHandle = sopNode->getCookedGeoHandle(context);

    if (detailHandle.isNull())
    {
        addError(ROP_MESSAGE, "Unable to acquire detail handle for SOP node");
        return ROP_ABORT_RENDER;
    }

    // Get a non-const GU_Detail so we can triangulate it for generating multiple convex hulls
    detailHandle.makeUnique();
    GU_Detail* detail = detailHandle.gdpNC();

    if (detail == nullptr)
    {
        addError(ROP_MESSAGE, "Null detail node from parent SOP node");
        return ROP_ABORT_RENDER;
    }

    // Create the chunk infos we'll use for the asset
    if (!boss->opStart("Creating chunk information"))
    {
        addError(ROP_MESSAGE, "Interrupted blast asset generation");
        return ROP_ABORT_RENDER;
    }

    UT_String rootName;
    evalString(rootName, rootNameKey, 0, time);

    BlastHelpers::ChunkInfos chunkInfos;
    std::vector<std::string> errors;
    if (!BlastHelpers::createChunkInfos(detail, context, rootName.toStdString(), chunkInfos, errors))
    {
        for (const auto& error : errors)
        {
            addError(ROP_MESSAGE, error.c_str());
        }
        addError(ROP_MESSAGE, "Unable to create chunk infos");
        return ROP_ABORT_RENDER;
    }

    boss->opEnd();

    // Generate collision data for bond generation & chunk volume data
    if (evalInt(rootIsStaticKey, 0, time))
    {
        chunkInfos.chunkIsStatic[0] = true;
    }

    int maxConvexHulls = evalInt(maxConvexHullsKey, 0, time);

    ChunkCollisionData chunkCollisionData(*m_collisionBuilder);
    if (!createCollisionData(boss, chunkInfos, maxConvexHulls, chunkCollisionData))
    {
        addError(ROP_MESSAGE, "Unable to create collision data for chunks");
        return ROP_ABORT_RENDER;
    }

    // Generate bonds
    if (!boss->opStart("Generating bonds"))
    {
        addError(ROP_MESSAGE, "Interrupted blast asset generation");
        return ROP_ABORT_RENDER;
    }

    fpreal maxSeparation = evalFloat(maxSeparationKey, 0, time);

    NvBlastBondDesc* bondDescs = nullptr;
    uint32_t bondCount = createBonds(chunkInfos, chunkCollisionData, maxSeparation, bondDescs);
    Nv::Blast::unique_ptr<NvBlastBondDesc> bondDescsRelease(bondDescs);
    if (bondCount == 0)
    {
        addError(ROP_MESSAGE, "Failed to create bond descriptions");
        return ROP_ABORT_RENDER;
    }

    boss->opEnd();

    // Put the chunks into the proper order for a NvBlastAsset
    if (!boss->opStart("Reordering chunks"))
    {
        addError(ROP_MESSAGE, "Interrupted blast asset generation");
        return ROP_ABORT_RENDER;
    }

    std::vector<uint32_t> chunkReorderMap = reorderChunks(chunkInfos, bondDescs, bondCount);

    boss->opEnd();

    // Create physics data after chunk reorder so it's in the correct order
    if (!boss->opStart("Creating physics chunk data"))
    {
        addError(ROP_MESSAGE, "Interrupted blast asset generation");
        return ROP_ABORT_RENDER;
    }

    ChunkPhysicsData chunkPhysicsData;
    if (!createPhysicsData(chunkInfos, chunkCollisionData, chunkReorderMap, chunkPhysicsData))
    {
        addError(ROP_MESSAGE, "Unable to create physics data for chunks");
        return ROP_ABORT_RENDER;
    }

    boss->opEnd();

    // Save out the appropriate type of blast asset - currently low level, tk (has joints), or with full physx meshes
    if (!boss->opStart("Saving blast asset"))
    {
        addError(ROP_MESSAGE, "Interrupted blast asset generation");
        return ROP_ABORT_RENDER;
    }

    UT_String fileName;
    getOutputOverrideEx(fileName, time, outputFileKey, makePathKey);

    BlastAssetFormat blastAssetFormat = static_cast<BlastAssetFormat>(evalInt(blastAssetFormatKey, 0, time));

    if (!saveAsset(fileName.c_str(), chunkInfos, chunkPhysicsData, bondDescs, bondCount, blastAssetFormat))
    {
        addError(ROP_MESSAGE, "Failed to save asset");
        return ROP_ABORT_RENDER;
    }

    boss->opEnd();

    // Execute the post-render script.
    if (error() < UT_ERROR_ABORT)
    {
        executePostFrameScript(time);
    }

    return ROP_CONTINUE_RENDER;
}

ROP_RENDER_CODE ROP_Blast::endRender()
{
    m_cooking = nullptr;
    m_physics = nullptr;
    m_foundation = nullptr;

    if (error() < UT_ERROR_ABORT)
    {
        executePostRenderScript(m_endTime);
    }
    return ROP_CONTINUE_RENDER;
}

void ROP_Blast::getNodeSpecificInfoText(OP_Context& context, OP_NodeInfoParms& parms)
{
    ROP_Node::getNodeSpecificInfoText(context, parms);

    SOP_Node* sopNode = CAST_SOPNODE(getInput(0));
    UT_String sopPath;

    if (sopNode)
    {
        sopNode->getFullPath(sopPath);
    }

    if(sopPath.isstring())
    {
        parms.append("Render SOP        ");
        parms.append(sopPath);
        parms.append("\n");
    }

    UT_String outputFile;
    evalStringRaw(outputFile, outputFileKey, 0, 0.0f);
    parms.append("Write to          ");
    parms.append(outputFile);
}

void ROP_Blast::fillInfoTreeNodeSpecific(UT_InfoTree &tree, const OP_NodeInfoTreeParms& parms)
{
    ROP_Node::fillInfoTreeNodeSpecific(tree, parms);

    UT_InfoTree* branch = tree.addChildMap("Blast ROP Info");
    
    SOP_Node* sopNode = CAST_SOPNODE(getInput(0));
    UT_String sopPath;

    if (sopNode)
    {
        sopNode->getFullPath(sopPath);
    }

    if (sopPath.isstring())
    {
        branch->addProperties("Render SOP", sopPath);
    }

    UT_String outputFile;
    evalStringRaw(outputFile, outputFileKey, 0, 0.0f);
    branch->addProperties("Writes to", outputFile);
};

bool ROP_Blast::createCollisionData(UT_Interrupt* boss, BlastHelpers::ChunkInfos& chunkInfos, int maxConvexHulls, ChunkCollisionData& outCollisionData)
{
    Nv::Blast::ConvexDecompositionParams convexDecompositionParameters;
    convexDecompositionParameters.maximumNumberOfHulls = maxConvexHulls > 0 ? maxConvexHulls : 1;
    convexDecompositionParameters.voxelGridResolution = 0;

    size_t chunkCount = chunkInfos.chunkDescs.size();

    if (boss->opStart("Creating chunk collision data"))
    {
        for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
        {
            int percent = static_cast<int>((100.0f * chunkIndex) / chunkCount);
            if (boss->opInterrupt(percent))
            {
                addError(ROP_MESSAGE, "Interrupted collision generation");
                return false;
            }

            Nv::Blast::CollisionHull** hulls = nullptr;
            uint32_t hullCount = 0;

            // Helps safely release the array of collision hulls created by NvBlastExtAuthoringBuildMeshConvexDecomposition
            Nv::Blast::unique_ptr<Nv::Blast::CollisionHull*> hullArrayRelease;

            if (convexDecompositionParameters.maximumNumberOfHulls == 1)
            {
                std::vector<NvcVec3> vertices;
                for (const auto& triangle : chunkInfos.chunkTriangles[chunkIndex])
                {
                    vertices.push_back(triangle.a.p);
                    vertices.push_back(triangle.b.p);
                    vertices.push_back(triangle.c.p);
                }

                Nv::Blast::CollisionHull* hull = m_collisionBuilder->buildCollisionGeometry(static_cast<uint32_t>(vertices.size()), vertices.data());
                if (hull != nullptr)
                {
                    hulls = &hull;
                    hullCount = 1;
                }
            }
            else
            {
                const std::vector<Nv::Blast::Triangle>& triangleMesh = chunkInfos.chunkTriangles[chunkIndex];
                hullCount = NvBlastExtAuthoringBuildMeshConvexDecomposition(
                    m_collisionBuilder.get(), triangleMesh.data(), static_cast<uint32_t>(triangleMesh.size()), convexDecompositionParameters, hulls);
                hullArrayRelease.reset(hulls);
            }

            if (hullCount == 0 || hulls == nullptr)
            {
                std::string errorMessage = "Failed to build collision meshes for chunk ";
                errorMessage += chunkInfos.chunkNames[chunkIndex];
                addError(ROP_MESSAGE, errorMessage.c_str());
                return false;
            }

            uint32_t hullOffset = outCollisionData.collisionHulls.size();

            outCollisionData.collisionHullOffsets.push_back(outCollisionData.collisionHulls.size());

            float totalVolume = 0.f;
            for (uint32_t hullIndex = 0; hullIndex < hullCount; ++hullIndex)
            {
                Nv::Blast::CollisionHull* hull = hulls[hullIndex];
                if (hull == nullptr)
                {
                    std::string errorMessage = "Found null collision hull for chunk ";
                    errorMessage += chunkInfos.chunkNames[chunkIndex];
                    addError(ROP_MESSAGE, errorMessage.c_str());
                    return false;
                }
                outCollisionData.collisionHulls.emplace_back(hull);

                // Note that the authoring tool now uses 'calculateCollisionHullVolume'
                // from its internal implementation (see NvBlastExtAuthoring.cpp)
                // Still create our convex mesh here and use its mass information to avoid
                // the dependency on a 'source' file that isn't officially exposed
                // Convex meshes were also created with ExtPxCollisionBuilder::buildPhysicsChunks
                // but we want greater control over creating those chunks (like their static flag)
                physx::PxConvexMesh* convexMesh = m_collisionBuilder->buildConvexMesh(*hull);
                if (convexMesh == nullptr)
                {
                    std::string errorMessage = "Failed to build convex mesh from decomposition for chunk ";
                    errorMessage += chunkInfos.chunkNames[chunkIndex];
                    addError(ROP_MESSAGE, errorMessage.c_str());
                    return false;
                }
                outCollisionData.convexMeshes.emplace_back(convexMesh);

                physx::PxVec3 localCenterOfMass;
                physx::PxMat33 inertia;
                float mass;
                convexMesh->getMassInformation(mass, inertia, localCenterOfMass);
                totalVolume += mass;
            }

            chunkInfos.chunkDescs[chunkIndex].volume = totalVolume;
        }

        // Need to output end hull offset in the last position for bond creation
        outCollisionData.collisionHullOffsets.push_back(outCollisionData.collisionHulls.size());
    }

    boss->opEnd();
    return true;
}

uint32_t ROP_Blast::createBonds(const BlastHelpers::ChunkInfos& chunkInfos, const ChunkCollisionData& collisionData, float maxSeparation, NvBlastBondDesc*& outBondDescs)
{
    physx::unique_ptr<Nv::Blast::BlastBondGenerator> bondGenerator(
        NvBlastExtAuthoringCreateBondGenerator(m_collisionBuilder.get()));

    if (bondGenerator == nullptr)
    {
        addError(ROP_MESSAGE, "Unable to create bond generator");
        return 0;
    }

    uint32_t chunkCount = chunkInfos.chunkDescs.size();

    // Create C-style array since vector<bool> is optimized and doesn't return a bool*
    std::unique_ptr<bool[]> isSupportChunk(new bool[chunkCount]);
    for (int chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
    {
        isSupportChunk[chunkIndex] = static_cast<bool>(chunkInfos.chunkDescs[chunkIndex].flags & NvBlastChunkDesc::Flags::SupportFlag);
    }

    // Const-cast to workaround Blast API and still have automatic memory management - C++ doesn't allow this conversion automatically due to the second level of indirection
    const Nv::Blast::CollisionHull** hulls = const_cast<const Nv::Blast::CollisionHull**>(collisionData.collisionHulls.data());
    return bondGenerator->bondsFromPrefractured(chunkCount, collisionData.collisionHullOffsets.data(), hulls, isSupportChunk.get(), nullptr, outBondDescs, maxSeparation);
}

std::vector<uint32_t> ROP_Blast::reorderChunks(BlastHelpers::ChunkInfos& chunkInfos, NvBlastBondDesc* bondDescs, uint32_t bondCount)
{
    size_t chunkCount = chunkInfos.chunkDescs.size();
    auto chunkDescs = chunkInfos.chunkDescs.data();

    // order chunks, build map
    std::vector<uint32_t> chunkReorderMap(chunkCount);
    std::vector<char> scratch(chunkCount * sizeof(NvBlastChunkDesc));
    NvBlastEnsureAssetExactSupportCoverage(chunkDescs, chunkCount, scratch.data(), Nv::Blast::logLL);
    NvBlastBuildAssetDescChunkReorderMap(chunkReorderMap.data(), chunkDescs, chunkCount, scratch.data(), Nv::Blast::logLL);
    NvBlastApplyAssetDescChunkReorderMapInPlace(chunkDescs, chunkCount, bondDescs, bondCount, chunkReorderMap.data(), true, scratch.data(), Nv::Blast::logLL);

    // return the reordered chunk map
    std::vector<uint32_t> chunkReorderInvMap(chunkReorderMap.size());
    Nv::Blast::invertMap(chunkReorderInvMap.data(), chunkReorderMap.data(), static_cast<unsigned int>(chunkReorderMap.size()));
    return chunkReorderInvMap;
}

bool ROP_Blast::createPhysicsData(const BlastHelpers::ChunkInfos& chunkInfos, const ChunkCollisionData& collisionData, const std::vector<uint32_t>& chunkReorderMap, ChunkPhysicsData& outPhysicsData)
{
    size_t chunkCount = chunkInfos.chunkDescs.size();

    if (chunkCount != chunkReorderMap.size())
    {
        addError(ROP_MESSAGE, "Chunk info & reordering map sizes do not match");
        return false;
    }

    // Assumption, shared with Blast, that the collision hull offset array ends with the total hull count, so per chunk counts work
    if (chunkCount + 1 != collisionData.collisionHullOffsets.size())
    {
        addError(ROP_MESSAGE, "Collision hull offsets does not contain the correct number of entries (number of chunks plus a last entry with total hull count)");
        return false;
    }

    if (collisionData.collisionHullOffsets.back() != static_cast<uint32_t>(collisionData.collisionHulls.size()))
    {
        addError(ROP_MESSAGE, "Collision offset total count & collision hull vector sizes do not match");
        return false;
    }

    for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
    {
        // Add to our output arrays
        outPhysicsData.physicsChunks.emplace_back();
        auto& physicsChunk = outPhysicsData.physicsChunks.back();

        // Look up the original chunk index used for collision hull generation.
        const uint32_t originalIndex = chunkReorderMap[chunkIndex];

        if (originalIndex > chunkCount)
        {
            addError(ROP_MESSAGE, "Invalid chunk reordering map entry found");
            return false;
        }

        const uint32_t hullOffset = collisionData.collisionHullOffsets[originalIndex];
        const uint32_t hullCount = collisionData.collisionHullOffsets[originalIndex + 1] - hullOffset;
        const uint32_t firstSubchunkIndex = static_cast<uint32_t>(outPhysicsData.physicsSubchunks.size());

        // Fill in the subchunks
        for (uint32_t hullOffsetIndex = 0; hullOffsetIndex < hullCount; ++hullOffsetIndex)
        {
            outPhysicsData.physicsSubchunks.emplace_back();
            auto& physicsSubchunk = outPhysicsData.physicsSubchunks.back();

            physicsSubchunk.transform = physx::PxTransform(physx::PxIdentity);
            physicsSubchunk.geometry = physx::PxConvexMeshGeometry(collisionData.convexMeshes[hullOffset + hullOffsetIndex]);
        }


        physicsChunk.isStatic = chunkInfos.chunkIsStatic[originalIndex];
        physicsChunk.subchunkCount = hullCount;
        physicsChunk.firstSubchunkIndex = chunkIndex;
    }

    return true;
}

bool ROP_Blast::saveAsset(const char* fileName, const BlastHelpers::ChunkInfos& chunkInfos, ChunkPhysicsData& chunkPhysicsData, NvBlastBondDesc* bondDescs, uint32_t bondCount, BlastAssetFormat assetFormat)
{
    // Following example output from BlastDataExporter.cpp
    const void* outputAsset = nullptr;
    uint32_t objectTypeID = 0; 

    auto chunkDescs = chunkInfos.chunkDescs.data();
    uint32_t chunkCount = chunkInfos.chunkDescs.size();

    physx::unique_ptr<Nv::Blast::TkFramework> tkFramework(NvBlastTkFrameworkCreate());
    Nv::Blast::unique_ptr<NvBlastAsset> lowLevelAsset;
    physx::unique_ptr<Nv::Blast::ExtPxAsset> physicsAsset;

    if (tkFramework == nullptr)
    {
        addError(ROP_MESSAGE, "Unable to create TkFramework");
        return false;
    }

    if (assetFormat == BlastAssetFormat::LowLevelAsset)
    {
        NvBlastAssetDesc descriptor;
        descriptor.bondCount = bondCount;
        descriptor.bondDescs = bondDescs;
        descriptor.chunkCount = chunkCount;
        descriptor.chunkDescs = chunkDescs;

        std::vector<uint8_t> scratch(static_cast<unsigned int>(NvBlastGetRequiredScratchForCreateAsset(&descriptor, Nv::Blast::logLL)));
        void* mem = NVBLAST_ALLOC(NvBlastGetAssetMemorySize(&descriptor, Nv::Blast::logLL));
        lowLevelAsset.reset(NvBlastCreateAsset(mem, &descriptor, scratch.data(), Nv::Blast::logLL));
        outputAsset = lowLevelAsset.get();
        objectTypeID = Nv::Blast::LlObjectTypeID::Asset;
    }
    else
    {
        Nv::Blast::TkAssetDesc descriptor;
        descriptor.bondCount = bondCount;
        descriptor.bondDescs = bondDescs;
        descriptor.chunkCount = chunkCount;
        descriptor.chunkDescs = chunkDescs;
        std::vector<uint8_t> bondFlags(bondCount);
        for (auto i = 0u; i < bondCount; ++i)
        {
            bondFlags[i] = Nv::Blast::TkAssetDesc::BondJointed;
        }
        descriptor.bondFlags = bondFlags.data();
        physicsAsset.reset(Nv::Blast::ExtPxAsset::create(descriptor, chunkPhysicsData.physicsChunks.data(), chunkPhysicsData.physicsSubchunks.data(), *tkFramework));

        if (assetFormat == BlastAssetFormat::TkAsset)
        {
            outputAsset = &physicsAsset->getTkAsset();
            objectTypeID = Nv::Blast::TkObjectTypeID::Asset;
        }
        else if (assetFormat == BlastAssetFormat::PhysXAsset)
        {
            outputAsset = physicsAsset.get();
            objectTypeID = Nv::Blast::ExtPxObjectTypeID::Asset;
        }
    }

    if (outputAsset == nullptr)
    {
        addError(ROP_MESSAGE, "Unable to create blast asset");
        return false;
    }

    physx::unique_ptr<Nv::Blast::ExtSerialization> serialization(NvBlastExtSerializationCreate());
    if (serialization == nullptr)
    {
        addError(ROP_MESSAGE, "Can't create serialization class");
        return false;
    }

    bool success = true;
    success = success && NvBlastExtTkSerializerLoadSet(*tkFramework, *serialization) > 0;
    success = success && NvBlastExtPxSerializerLoadSet(*tkFramework, *m_physics, *m_cooking, *serialization) > 0;
    success = success && serialization->setSerializationEncoding(NVBLAST_FOURCC('C', 'P', 'N', 'B'));

    if (!success)
    {
        addError(ROP_MESSAGE, "Failed to load serialization data");
        return false;
    }

    void* outputBuffer = nullptr;
    const size_t bufferSize = static_cast<size_t>(serialization->serializeIntoBuffer(outputBuffer, outputAsset, objectTypeID));
    Nv::Blast::unique_ptr<void> outputBufferRelease(outputBuffer);

    if (bufferSize == 0)
    {
        addError(ROP_MESSAGE, "Unable to serialize asset into buffer");
        return false;
    }

    std::ofstream file(fileName, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!file.is_open())
    {
        addError(ROP_MESSAGE, "Can't open output file");
        return false;
    }

    file.write(static_cast<const char*>(outputBuffer), bufferSize);
    size_t endPos = file.tellp();

    if (bufferSize != endPos)
    {
        addError(ROP_MESSAGE, "Buffer write failed");
        return false;
    }

    return true;
}

ROP_Blast::ChunkCollisionData::ChunkCollisionData(const Nv::Blast::ExtPxCollisionBuilder& inCollisionBuilder)
    : collisionBuilder(inCollisionBuilder)
{
}

ROP_Blast::ChunkCollisionData::~ChunkCollisionData()
{
    // Clean up allocated collision hulls & convex meshes
    for (auto hull : collisionHulls)
    {
        collisionBuilder.releaseCollisionHull(hull);
    }

    for (auto convexMesh : convexMeshes)
    {
        convexMesh->release();
    }
}
