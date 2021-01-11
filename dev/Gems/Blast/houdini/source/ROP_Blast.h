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
#pragma once

#include <ROP/ROP_Node.h>
#include <NvBlastGlobals.h>

struct NvBlastBondDesc;
struct NvBlastChunkDesc;

class OP_Network;
class OP_Node;
class OP_NodeInfoParms;
class OP_NodeInfoTreeParms;
class OP_Operator;
class OP_TemplatePair;
class OP_VariablePair;
class PRM_Template;
class SOP_Node;
class UT_InfoTree;

namespace BlastHelpers
{
    struct ChunkInfos;
}

namespace physx
{
    template <typename T>
    struct PxPtrReleaser 
    {
        void operator()(T* ptr) 
        {
            if (ptr)
            {
                ptr->release();
            }
        }
    };

    template <typename T>
    using unique_ptr = std::unique_ptr<T, PxPtrReleaser<T>>;

    class PxConvexMesh;
    class PxCooking;
    class PxFoundation;
    class PxPhysics;
    class PxVec3;
}

namespace Nv
{
    namespace Blast
    {
        template <typename T>
        struct BlastPtrReleaser
        {
            void operator()(T* ptr)
            {
                if (ptr)
                {
                    NVBLAST_FREE(ptr);
                }
            }
        };

        template <typename T>
        using unique_ptr = std::unique_ptr<T, BlastPtrReleaser<T>>;

        struct CollisionHull;
        class ExtPxCollisionBuilder;
        struct ExtPxChunk;
        struct ExtPxSubchunk;
    }
}


class ROP_Blast : public ROP_Node
{
public:
    static OP_TemplatePair* getTemplatePair();
    static OP_VariablePair* getVariablePair();
    static OP_Node* myConstructor(OP_Network* net, const char* name, OP_Operator* op);

    void getNodeSpecificInfoText(OP_Context& context, OP_NodeInfoParms& iparms) override;
    void fillInfoTreeNodeSpecific(UT_InfoTree& tree, const OP_NodeInfoTreeParms& parms) override;

protected:
    ROP_Blast(OP_Network* net, const char* name, OP_Operator* op);
    ~ROP_Blast() override;

    int startRender(int frameCount, fpreal startTime, fpreal endTime) override;
    ROP_RENDER_CODE renderFrame(fpreal time, UT_Interrupt* boss) override;
    ROP_RENDER_CODE endRender() override;

    static PRM_Template* getTemplates();
    bool initPhysX();

    // Struct of arrays to easily pass data to helper functions & NvBlast's C API
    struct ChunkCollisionData
    {
        ChunkCollisionData(const Nv::Blast::ExtPxCollisionBuilder& inCollisionBuilder);
        ~ChunkCollisionData();
        // Contains the offset for each chunk into the collisionHulls & convexMeshes arrays, and has a final entry of the total number of hulls
        std::vector<uint32_t> collisionHullOffsets;
        // One-to-one mapping of collision hulls and convex meshes - there may be multiple per chunk
        std::vector<Nv::Blast::CollisionHull*> collisionHulls;
        std::vector<physx::PxConvexMesh*> convexMeshes;
        // Used to release collision hulls without knowing their internals
        const Nv::Blast::ExtPxCollisionBuilder& collisionBuilder;
    };

    struct ChunkPhysicsData
    {
        std::vector<Nv::Blast::ExtPxSubchunk> physicsSubchunks;
        std::vector<Nv::Blast::ExtPxChunk> physicsChunks;
    };

    enum class BlastAssetFormat
    {
        LowLevelAsset,
        TkAsset,
        PhysXAsset,
    };;

    bool createCollisionData(UT_Interrupt* boss, BlastHelpers::ChunkInfos& chunkInfos, int maxConvexHulls, ChunkCollisionData& outCollisionData);
    uint32_t createBonds(const BlastHelpers::ChunkInfos& chunkInfos, const ChunkCollisionData& collisionData, float maxSeparation, NvBlastBondDesc*& outBondDescs);
    std::vector<uint32_t> reorderChunks(BlastHelpers::ChunkInfos& chunkInfos, NvBlastBondDesc* bondDescs, uint32_t bondCount);
    bool createPhysicsData(const BlastHelpers::ChunkInfos& chunkInfos, const ChunkCollisionData& collisionData, const std::vector<uint32_t>& chunkReorderMap, ChunkPhysicsData& outPhysicsData);
    bool saveAsset(const char* fileName, const BlastHelpers::ChunkInfos& chunkInfos, ChunkPhysicsData& chunkPhysicsData, NvBlastBondDesc* bondDescs, uint32_t bondCount, BlastAssetFormat assetFormat);

private:
    fpreal m_endTime;

    physx::unique_ptr<physx::PxFoundation> m_foundation;
    physx::unique_ptr<physx::PxPhysics> m_physics;
    physx::unique_ptr<physx::PxCooking> m_cooking;
    physx::unique_ptr<Nv::Blast::ExtPxCollisionBuilder> m_collisionBuilder;
};
