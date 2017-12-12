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
#include "LODGenerator.h"
#include "LODGenMesh.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "Mesh.h"
#include "LODGenVertex.h"
#include "LODGenTriangle.h"
#include "SoftSkinDeformer.h"
#include "DualQuatSkinDeformer.h"
#include "SoftSkinManager.h"
#include "EventManager.h"
#include "SkinningInfoVertexAttributeLayer.h"
#include "MeshDeformerStack.h"
#include "MorphMeshDeformer.h"
#include "MorphSetup.h"
#include "MorphTargetStandard.h"


namespace EMotionFX
{
    // default constructor
    LODGenerator::LODGenerator()
    {
        mClone          = nullptr;
        mActor          = nullptr;
        mTotalNumVerts  = 0;
        mNumVerts       = 0;
    }


    // destructor
    LODGenerator::~LODGenerator()
    {
        Release();
    }


    // create
    LODGenerator* LODGenerator::Create()
    {
        return new LODGenerator();
    }


    // release memory
    void LODGenerator::Release()
    {
        // delete all meshes
        const uint32 numMeshes = mMeshes.GetLength();
        for (uint32 m = 0; m < numMeshes; ++m)
        {
            mMeshes[m]->Destroy();
        }
        mMeshes.Clear();

        // delete the cloned actor
        if (mClone)
        {
            mClone->Destroy();
            mClone = nullptr;
        }

        // delete the actor
        if (mActor)
        {
            mActor->Destroy();
            mActor = nullptr;
        }

        mTotalNumVerts  = 0;
        mNumVerts       = 0;
    }


    // init the generator for a given actor
    void LODGenerator::Init(Actor* sourceActor, const InitSettings& initSettings)
    {
        GetEventManager().OnProgressStart();
        MCore::LogDetailedInfo("LODGen - Initializing LOD Generator...");
        GetEventManager().OnProgressText("Initializing LOD Generator");

        // release any existing data
        Release();

        // create the clones
        mActor = sourceActor->Clone();
        mClone = sourceActor->Clone();

        mActor->SetIsUsedForVisualization(true);
        mClone->SetIsUsedForVisualization(true);

        // calc the bind pose matrices
        sourceActor->GetSkeleton()->CalcBindPoseGlobalMatrices(mBindPoseMatrices);

        // make sure the input LOD is within a valid range
        const uint32 inputLOD = MCore::Clamp<uint32>(initSettings.mInputLOD, 0, mClone->GetNumLODLevels() - 1);

        // for all nodes
        const uint32 numNodes = mClone->GetNumNodes();
        mMeshes.Reserve(numNodes);
        uint32 startVertex = 0;
        for (uint32 n = 0; n < numNodes; ++n)
        {
            Node* node = mClone->GetSkeleton()->GetNode(n);

            // skip to the next node if there is no mesh
            Mesh* mesh = mClone->GetMesh(inputLOD, n);
            if (mesh == nullptr)
            {
                continue;
            }

            if (mesh->CheckIfIsTriangleMesh() == false)
            {
                MCore::LogWarning("EMotionFX::LODGenerator::Init() - The mesh for node '%s' is not a triangle mesh, while currently only triangle meshes are supported.", node->GetName());
            }

            // get the per node settings
            const NodeSettings* settings = FindNodeSettings(initSettings, n);
            bool removeMesh = (settings) ? settings->mForceRemoveMesh : false;

            // calculate the AABB
            MCore::AABB aabb;
            aabb.Init();
            mesh->CalcAABB(&aabb, mBindPoseMatrices[n]);
            const float length = aabb.CalcExtents().SafeLength();
            if (removeMesh || length < initSettings.mAABBThreshold)
            {
                MCore::LogDetailedInfo("LODGen - Removing mesh for node '%s' (AABB extent length=%f)", node->GetName(), length);
                continue;
            }

            // setup the mesh importance factor
            float importanceFactor = 1.0f;
            if (settings)
            {
                importanceFactor = settings->mImportanceFactor;
            }

            // register the mesh to the generator
            LODGenMesh* lodGenMesh = LODGenMesh::Create(this, mClone, mesh, n, startVertex, importanceFactor);
            lodGenMesh->SetGlobalMatrix(mBindPoseMatrices[n]);
            mMeshes.Add(lodGenMesh);

            startVertex += mesh->GetNumVertices();
        }

        // initialize the meshes
        Reinit(initSettings);

        MCore::LogInfo("LODGen - Initialization successfully completed...");
        GetEventManager().OnProgressEnd();
    }


    // reinitialize all generated meshes
    void LODGenerator::Reinit(const InitSettings& initSettings)
    {
        // init the mesh data
        const uint32 numMeshes = mMeshes.GetLength();
        for (uint32 m = 0; m < numMeshes; ++m)
        {
            mMeshes[m]->Init(initSettings);
        }

        // get the total number of vertices
        mTotalNumVerts = CalcTotalNumVertices();
        mNumVerts = mTotalNumVerts;

        // precompute the edge collapse costs
        CalcEdgeCollapseCosts(initSettings);

        // get the total number of vertices and set it in all meshes
        //  GetEventManager().OnSubProgressText("Generating cells");
        const uint32 totalNumVertices = CalcTotalNumVertices();
        uint32 numVertices = totalNumVertices;
        for (uint32 m = 0; m < numMeshes; ++m)
        {
            mMeshes[m]->SetTotalNumVertices(numVertices);
            mMeshes[m]->GenerateCells();
            //      GetEventManager().OnSubProgressValue( (m  / (float)(numMeshes-1)) * 100.0f );
        }

        if (initSettings.mRandomizeSameCostVerts == false)
        {
            // reduce the object down to nothing
            //      GetEventManager().OnSubProgressText("Collapsing edges");
            while (mNumVerts > 0)
            {
                // get the next vertex to collapse
                LODGenVertex* collapseFrom  = FindMinimumCostVertex();

                // collapse this edge
                Collapse(collapseFrom, collapseFrom->GetCollapseVertex(), initSettings);
                mNumVerts--;

                // update the progress
                if (mNumVerts % 2000 == 0)
                {
                    const float progressPercentage = ((mTotalNumVerts - mNumVerts) / (float)mTotalNumVerts) * 100.0f;
                    GetEventManager().OnProgressValue(progressPercentage);
                }
            }
        }
        else // include randomization
        {
            //GetEventManager().OnSubProgressText("Collapsing edges");
            while (mNumVerts > 0)
            {
                // update the lowest cost array (slow) and get a random vertex with the same cost
                LODGenVertex* collapseFrom  = FindMinimumCostVertex();
                collapseFrom->GetMesh()->BuildSameCostVertexArray(collapseFrom->GetCollapseCost());
                collapseFrom = collapseFrom->GetMesh()->GetRandomSameCostVertex();

                // collapse this edge
                Collapse(collapseFrom, collapseFrom->GetCollapseVertex(), initSettings);
                mNumVerts--;

                // update the progress
                if (mNumVerts % 2000 == 0)
                {
                    const float progressPercentage = ((mTotalNumVerts - mNumVerts) / (float)mTotalNumVerts) * 100.0f;
                    GetEventManager().OnProgressValue(progressPercentage);
                }
            }
        }

        // reorder the map lists based on the collapse ordering
        //GetEventManager().OnSubProgressText("Finalizing lookup tables");
        for (uint32 m = 0; m < numMeshes; ++m)
        {
            mMeshes[m]->PrepareMap();
            //GetEventManager().OnSubProgressValue( (m  / (float)(numMeshes-1)) * 100.0f );
        }
    }


    // precalculate the edge collapse costs of all vertices in all meshes
    void LODGenerator::CalcEdgeCollapseCosts(const InitSettings& settings)
    {
        //GetEventManager().OnSubProgressText("Calculating edge collapse costs");
        const uint32 numMeshes = mMeshes.GetLength();
        for (uint32 m = 0; m < numMeshes; ++m)
        {
            LODGenMesh* mesh = mMeshes[m];

            // calculate the collapse cost for all vertices
            const uint32 numVertices = mesh->GetNumVertices();
            for (uint32 v = 0; v < numVertices; ++v)
            {
                mesh->GetStartVertex(v)->CalcEdgeCollapseCosts(settings);
            }

            //GetEventManager().OnSubProgressValue( (m  / (float)(numMeshes-1)) * 100.0f );
        }
    }


    // calculate the total number of vertices
    uint32 LODGenerator::CalcTotalNumVertices() const
    {
        uint32 sum = 0;

        const uint32 numMeshes = mMeshes.GetLength();
        for (uint32 m = 0; m < numMeshes; ++m)
        {
            sum += mMeshes[m]->GetNumStartVertices();
        }

        return sum;
    }


    // find the vertex with minimum collapse costs
    LODGenVertex* LODGenerator::FindMinimumCostVertex() const
    {
        LODGenVertex* result = nullptr;

        // find an initial lowest vertex
        const uint32 numMeshes = mMeshes.GetLength();
        for (uint32 m = 0; m < numMeshes && result == nullptr; ++m)
        {
            LODGenMesh* mesh = mMeshes[m];
            result = mesh->FindLowestCostVertex();
        }

        // check if there are meshes with even lower cost
        for (uint32 m = 0; m < numMeshes; ++m)
        {
            LODGenVertex* meshLowest = mMeshes[m]->FindLowestCostVertex();
            if (meshLowest == nullptr || result->GetMesh() == mMeshes[m])
            {
                continue;
            }

            if (meshLowest->GetCollapseCost() < result->GetCollapseCost())
            {
                result  = meshLowest;
            }
        }

        return result;

        /*
            // use the first vertex on default
            LODGenVertex* result2 = nullptr;
            const uint32 numMeshes = mMeshes.GetLength();
            for (uint32 m=0; m<numMeshes && result2==nullptr; ++m)
            {
                LODGenMesh* mesh = mMeshes[m];
                result2 = mesh->FindLowestCostVertexLinear();
            }

            for (uint32 m=0; m<numMeshes; ++m)
            {
                LODGenVertex* meshLowest = mMeshes[m]->FindLowestCostVertexLinear();
                if (meshLowest == nullptr)
                    continue;

                if (meshLowest->GetCollapseCost() < result2->GetCollapseCost())
                    result2 = meshLowest;
            }

            return result2;
        */

        /*
            if (result == result2)
            {
                //MCore::LogInfo("Same vertex");
            }
            else
                MCore::LogInfo("Different vertex %f vs %f", result->GetCollapseCost(), result2->GetCollapseCost());

            return result2;
        */
    }



    // collapse this vertex onto vertex 'u' onto 'v'
    void LODGenerator::Collapse(LODGenVertex* u, LODGenVertex* v, const InitSettings& settings)
    {
        // keep track of vertex to which we collapse to
        const uint32 targetVertex = (v) ? v->GetIndex() : MCORE_INVALIDINDEX32;
        LODGenMesh* vertexMesh = u->GetMesh();
        vertexMesh->SetPermutationValue(u->GetIndex(), vertexMesh->GetNumVertices() - 1);
        vertexMesh->SetMapValue(vertexMesh->GetNumVertices() - 1, targetVertex);
        const uint32 numMeshes = mMeshes.GetLength();
        for (uint32 m = 0; m < numMeshes; ++m)
        {
            mMeshes[m]->SetVertexTotals(mNumVerts - 1, mMeshes[m]->GetNumVertices());
        }

        // if this vertex is all by itself, there is nothing to collapse
        if (v == nullptr)
        {
            u->Delete();
            return;
        }

        // backup the list of neighbors
        mTemp.Clear(false);
        mTemp = u->GetNeighbors();

        // delete triangles on edge uv
        for (int32 i = u->GetNumTriangles() - 1; i >= 0; --i)
        {
            LODGenTriangle* triangle = u->GetTriangle(i);
            if (triangle->GetHasVertex(v))
            {
                triangle->Delete();
            }
        }

        // update remaining triangles to have v instead of this vertex
        for (int32 i = u->GetNumTriangles() - 1; i >= 0; --i)
        {
            u->GetTriangle(i)->ReplaceVertex(u, v);
        }

        u->Delete();

        // recompute the edge collapse costs for neighboring vertices
        const uint32 numNeighbors = mTemp.GetLength();
        for (uint32 i = 0; i < numNeighbors; ++i)
        {
            mTemp[i]->CalcEdgeCollapseCosts(settings);
            mTemp[i]->GetCell()->Invalidate();
        }

        vertexMesh->UpdateCells();
    }


    // debug render
    void LODGenerator::DebugRender(ActorInstance* instance, uint32 maxTotalVerts, const MCore::Vector3& visualOffset, const MCore::Vector3& camPos, bool colorBorders)
    {
    #ifndef EMFX_SCALE_DISABLED
        const MCore::Vector3& scale = instance->GetLocalScale();
    #else
        const MCore::Vector3 scale(1.0f, 1.0f, 1.0f);
    #endif

        const uint32 numMeshes = mMeshes.GetLength();
        for (uint32 m = 0; m < numMeshes; ++m)
        {
            mMeshes[m]->DebugRender(instance, maxTotalVerts, visualOffset, scale, camPos, colorBorders);
        }
    }


    // generate the meshes
    void LODGenerator::GenerateMeshes(const GenerateSettings& settings)
    {
        const uint32 lodLevel = 0;

        // calculate the maximum number of vertices
        const float percentage = MCore::Clamp<float>(settings.mDetailPercentage, 0.0f, 100.0f);
        const uint32 maxTotalVerts = (uint32)(mTotalNumVerts * (percentage / 100.0f));

        //GetEventManager().OnProgressText("Generating Meshes");
        //GetEventManager().OnProgressValue( 0.0f );

        // remove meshes that have to be removed
        Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 numMeshes = mMeshes.GetLength();
        const uint32 numNodes = skeleton->GetNumNodes();
        for (uint32 n = 0; n < numNodes; ++n)
        {
            Mesh* mesh = mActor->GetMesh(lodLevel, n);
            if (mesh == nullptr)
            {
                continue;
            }

            // try to see if the LOD generator has a mesh for this node, or if it removed the entire mesh
            bool found = false;
            for (uint32 m = 0; m < numMeshes; ++m)
            {
                if (mMeshes[m]->GetNodeIndex() == n)
                {
                    found = true;
                    break;
                }
            }

            // the mesh still exists, we don't have to remove it
            if (found)
            {
                continue;
            }

            // remove the mesh and deformer stack
            if (mesh)
            {
                mesh->Destroy();
            }

            MeshDeformerStack* stack = mActor->GetMeshDeformerStack(lodLevel, n);
            if (stack)
            {
                stack->Destroy();
            }

            mActor->SetMesh(lodLevel, n, nullptr);
            mActor->SetMeshDeformerStack(lodLevel, n, nullptr);
        }

        // for all LOD meshes
        for (uint32 m = 0; m < numMeshes; ++m)
        {
            Node* node = skeleton->GetNode(mMeshes[m]->GetNodeIndex());

            // create the mesh
            Mesh* newMesh = mMeshes[m]->GenerateMesh(maxTotalVerts, settings);
            Mesh* mesh = newMesh;

            // remove the vertex lookup attribute layer, used to map the new vertices to the ones at full LOD
            //const uint32 layerIndex = newMesh->FindVertexAttributeLayerNumber( 0xFADE );  // 0xFADE is the vertex layer ID we used
            //if (layerIndex != MCORE_INVALIDINDEX32)
            //          newMesh->RemoveVertexAttributeLayer( layerIndex );

            // remove the mesh and deformer stack
            if (mesh->GetNumVertices() == 0)
            {
                if (mActor->GetMesh(lodLevel, node->GetNodeIndex()))
                {
                    mActor->GetMesh(lodLevel, node->GetNodeIndex())->Destroy();
                }
                if (mActor->GetMeshDeformerStack(lodLevel, node->GetNodeIndex()))
                {
                    mActor->GetMeshDeformerStack(lodLevel, node->GetNodeIndex())->Destroy();
                }
                mActor->SetMesh(lodLevel, node->GetNodeIndex(), nullptr);
                mActor->SetMeshDeformerStack(lodLevel, node->GetNodeIndex(), nullptr);
                mesh->Destroy();
                continue;
            }

            // calculate the normals
            if (settings.mGenerateNormals)
            {
                mesh->CalcNormals(!settings.mSmoothNormals);
            }

            // calculate the tangents
            if (settings.mGenerateTangents)
            {
                mesh->CalcTangents();
            }

            // update the mesh
            //Mesh* oldMesh = node->GetMesh( lodLevel );
            if (mActor->GetMesh(lodLevel, node->GetNodeIndex()))
            {
                mActor->GetMesh(lodLevel, node->GetNodeIndex())->Destroy();
                mActor->SetMesh(lodLevel, node->GetNodeIndex(), mesh);
            }

            MeshDeformerStack* stack = mActor->GetMeshDeformerStack(lodLevel, node->GetNodeIndex());
            if (stack)
            {
                // create the morph deformer if there is one
                MorphMeshDeformer* morphDeformer = (MorphMeshDeformer*)stack->FindDeformerByType(MorphMeshDeformer::TYPE_ID, 0);
                MeshDeformer* newMorphDeformer = nullptr;
                if (morphDeformer)
                {
                    newMorphDeformer = morphDeformer->Clone(mesh);
                }

                // create the new stack
                MeshDeformerStack* newStack = MeshDeformerStack::Create(mesh);
                if (mActor->GetMeshDeformerStack(lodLevel, node->GetNodeIndex()))
                {
                    mActor->GetMeshDeformerStack(lodLevel, node->GetNodeIndex())->Destroy();
                }
                mActor->SetMeshDeformerStack(lodLevel, node->GetNodeIndex(), newStack);
                stack = newStack;

                // add the new morph deformer if there is one
                if (newMorphDeformer)
                {
                    stack->AddDeformer(newMorphDeformer);
                }

                // get the skinning info attribute layer from the mesh and only add the skinning deformers to the stack if there actually is a skinning layer
                SkinningInfoVertexAttributeLayer* layer = (SkinningInfoVertexAttributeLayer*)mesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
                if (layer)
                {
                    // add the skinning deformer to the stack
                    if (settings.mDualQuatSkinning)
                    {
                        DualQuatSkinDeformer* skinDeformer = DualQuatSkinDeformer::Create(mActor->GetMesh(lodLevel, node->GetNodeIndex()));
                        skinDeformer->ReserveLocalBones(mActor->GetNumNodes()); // pre-alloc data to prevent reallocs
                        stack->AddDeformer(skinDeformer);
                    }
                    else
                    {
                        SoftSkinDeformer* skinDeformer = GetSoftSkinManager().CreateDeformer(mActor->GetMesh(lodLevel, node->GetNodeIndex()));
                        skinDeformer->ReserveLocalBones(mActor->GetNumNodes()); // pre-alloc data to prevent reallocs
                        stack->AddDeformer(skinDeformer);
                    }
                }

                stack->ReinitializeDeformers(mActor, node, lodLevel);
            }

            // remove the old mesh
            //oldMesh = nullptr;

            //      GetEventManager().OnProgressValue( (((m+1) / (float)numMeshes)) * 100.0f );
        }

        // update the morph setup
        UpdateMorphSetup(settings);

        // remove all vertex lookup layers, used to fix the morph target lods
        for (uint32 m = 0; m < numMeshes; ++m)
        {
            Mesh* mesh = mActor->GetMesh(lodLevel, mMeshes[m]->GetNodeIndex());
            if (mesh == nullptr)
            {
                continue;
            }

            // remove the vertex lookup attribute layer, used to map the new vertices to the ones at full LOD
            const uint32 layerIndex = mesh->FindVertexAttributeLayerNumber(0xFADE); // 0xFADE is the vertex layer ID we used
            if (layerIndex != MCORE_INVALIDINDEX32)
            {
                mesh->RemoveVertexAttributeLayer(layerIndex);
            }
        }
    }


    // add the last generated LOD level to this actor
    void LODGenerator::AddLastGeneratedLODToActor(Actor* actor)
    {
        actor->AddLODLevel(false);
        actor->CopyLODLevel(mActor, 0, actor->GetNumLODLevels() - 1, false, false);
    }


    // replace the given lod level for the given actor with the last generated LOD level
    void LODGenerator::ReplaceLODLevelWithLastGeneratedActor(Actor* actor, uint32 lodLevel)
    {
        actor->CopyLODLevel(mActor, 0, lodLevel, false, false);
    }


    // find the settings for a given node
    const LODGenerator::NodeSettings* LODGenerator::FindNodeSettings(const LODGenerator::InitSettings& settings, uint32 nodeIndex) const
    {
        const uint32 numSettings = settings.mNodeSettings.GetLength();
        for (uint32 i = 0; i < numSettings; ++i)
        {
            if (settings.mNodeSettings[i].mNodeIndex == nodeIndex)
            {
                return &settings.mNodeSettings[i];
            }
        }

        return nullptr;
    }


    // update the morph setup
    void LODGenerator::UpdateMorphSetup(const GenerateSettings& settings)
    {
        const uint32 lodLevel = 0;
        Actor* sourceActor = mClone;
        Actor* targetActor = mActor;

        // copy the morph setup
        CopyMorphSetup(sourceActor, targetActor, lodLevel, 0);

        // now remove any morph targets we dont want to be included
        MorphSetup* morphSetup = mActor->GetMorphSetup(0);
        if (morphSetup)
        {
            // remove all morph targets we don't want
            const uint32 numToRemove = settings.mMorphTargetIDsToRemove.GetLength();
            for (uint32 d = 0; d < numToRemove; ++d)
            {
                const uint32 index = morphSetup->FindMorphTargetNumberByID(settings.mMorphTargetIDsToRemove[d]);
                if (index != MCORE_INVALIDINDEX32)
                {
                    morphSetup->RemoveMorphTarget(index);
                }
            }
        }

        // if there is no morph setup in the source actor, there is nothing to do
        MorphSetup* sourceSetup = sourceActor->GetMorphSetup(lodLevel);
        if (sourceSetup == nullptr)
        {
            return;
        }

        // clone the morph setup
        MorphSetup* targetSetup = targetActor->GetMorphSetup(0);

        // for all morph targets
        uint32 i;
        const uint32 numMorphTargets = targetSetup->GetNumMorphTargets();
        for (uint32 m = 0; m < numMorphTargets; ++m)
        {
            // get the target morph target and make sure it's of the type we want
            MorphTarget* targetMorph = targetSetup->GetMorphTarget(m);
            if (targetMorph->GetType() != MorphTargetStandard::TYPE_ID)
            {
                continue;
            }

            // find the morph target with the same name inside source setup
            MorphTarget* sourceMorph = sourceSetup->FindMorphTargetByID(targetMorph->GetID());
            if (sourceMorph == nullptr)
            {
                // remove all deform datas when we can't find the source morph target, because that would be bad
                MorphTargetStandard* stdMorphTarget = (MorphTargetStandard*)targetMorph;
                stdMorphTarget->RemoveAllDeformDatas();
                continue;
            }

            // also make sure it's of the type we want
            if (sourceMorph->GetType() != MorphTargetStandard::TYPE_ID)
            {
                continue;
            }

            // get the standard morph target
            MorphTargetStandard* stdMorphTargetSource = (MorphTargetStandard*)sourceMorph;
            MorphTargetStandard* stdMorphTargetTarget = (MorphTargetStandard*)targetMorph;

            // for all deform datas
            const uint32 numDeformDatas = stdMorphTargetSource->GetNumDeformDatas();
            for (i = 0; i < numDeformDatas; ++i)
            {
                // get the deform data object
                MorphTargetStandard::DeformData* deformDataSource = stdMorphTargetSource->GetDeformData(i);
                MorphTargetStandard::DeformData* deformDataTarget = stdMorphTargetTarget->GetDeformData(i);

                // get the target mesh
                Mesh* targetMesh = targetActor->GetMesh(0, deformDataSource->mNodeIndex);
                if (targetMesh == nullptr)
                {
                    // NOTE: we don't actually delete the deform data, because then the indices to the deform data numbers
                    // become invalid inside the MorphMeshDeformer's deform passes
                    // so instead of that we just remove the deltas from the deform data
                    MCore::Free(deformDataTarget->mDeltas);
                    deformDataTarget->mDeltas   = nullptr;
                    deformDataTarget->mNumVerts = 0;
                    continue;
                }

                // the new number of deltas
                uint32 numTargetDeltas = 0;

                // find the vertex numbers
                const uint32 numTargetVerts = targetMesh->GetNumVertices();
                uint32* numbers = (uint32*)targetMesh->FindVertexData(0xFADE);
                //MCORE_ASSERT( numbers );
                if (numbers == nullptr)
                {
                    continue;
                }

                // make space in case we add more detail than before
                deformDataTarget->mDeltas = (MorphTargetStandard::DeformData::VertexDelta*)MCore::Realloc(deformDataTarget->mDeltas, sizeof(MorphTargetStandard::DeformData::VertexDelta) * deformDataSource->mNumVerts, EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS, MorphTargetStandard::MEMORYBLOCK_ID);

                // for all vertices in the deform data
                for (uint32 d = 0; d < deformDataSource->mNumVerts; ++d)
                {
                    // try to find the vertex number in the lower detail mesh
                    for (uint32 v = 0; v < numTargetVerts; ++v)
                    {
                        // if this delta vertex still exists in the lower detail mesh
                        if (numbers[v] == deformDataSource->mDeltas[d].mVertexNr)
                        {
                            deformDataTarget->mDeltas[numTargetDeltas] = deformDataSource->mDeltas[d];
                            deformDataTarget->mDeltas[numTargetDeltas].mVertexNr = v;
                            numTargetDeltas++;
                            break; // end the search and process the next vertex delta
                        }
                    }
                }

                //MCore::LogInfo("************* numTargetDeltas = %d / %d / %d", numTargetDeltas, deformDataSource->mNumVerts, targetMesh->GetNumVertices());

                // update the number of vertices, and shrink the delta array
                deformDataTarget->mNumVerts = numTargetDeltas;
                deformDataTarget->mDeltas   = (MorphTargetStandard::DeformData::VertexDelta*)MCore::Realloc(deformDataTarget->mDeltas, sizeof(MorphTargetStandard::DeformData::VertexDelta) * numTargetDeltas, EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS, MorphTargetStandard::MEMORYBLOCK_ID);
            }
        }
    }


    // copy the morph setup from one to the other actor
    // automatically update the morph mesh deformers as well
    void LODGenerator::CopyMorphSetup(Actor* sourceActor, Actor* targetActor, uint32 sourceLOD, uint32 targetLOD) const
    {
        // if there is no morph setup, there is nothing to do
        MorphSetup* sourceSetup = sourceActor->GetMorphSetup(sourceLOD);
        if (sourceSetup == nullptr)
        {
            return;
        }

        // clone the source setup
        MorphSetup* clonedSource(sourceSetup->Clone());
        if (targetActor->GetMorphSetup(targetLOD))
        {
            targetActor->GetMorphSetup(targetLOD)->Destroy();
        }
        targetActor->SetMorphSetup(targetLOD, clonedSource);

        // update all morph mesh deformer morph target pointers
        // as we need to point them to the new cloned ones
        const uint32 numNodes = targetActor->GetNumNodes();
        for (uint32 n = 0; n < numNodes; ++n)
        {
            // check if there is
            MeshDeformerStack* stack = targetActor->GetMeshDeformerStack(targetLOD, n);
            if (stack == nullptr)
            {
                continue;
            }

            // try to locate the mesh deformer if it has any
            MorphMeshDeformer* morphDeformer = (MorphMeshDeformer*)stack->FindDeformerByType(MorphMeshDeformer::TYPE_ID);
            if (morphDeformer == nullptr)
            {
                continue;
            }

            // adjust the deform passes
            morphDeformer->Reinitialize(targetActor, targetActor->GetSkeleton()->GetNode(n), targetLOD);
        }
    }
}   // namespace EMotionFX
