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
#include "MeshBuilder.h"
#include "MeshBuilderSkinningInfo.h"
#include "MeshBuilderSubMesh.h"
#include "Mesh.h"
#include "VertexAttributeLayer.h"
#include "VertexAttributeLayerAbstractData.h"
#include "SubMesh.h"
#include "SkinningInfoVertexAttributeLayer.h"
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/MultiThreadManager.h>

#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobContext.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshBuilder, MeshAllocator, 0)

    
    // constructor
    MeshBuilder::MeshBuilder(uint32 nodeNumber, uint32 numOrgVerts, uint32 maxBonesPerSubMesh, uint32 maxSubMeshVertices, bool isCollisionMesh, bool optimizeDuplicates)
        : BaseObject()
    {
        mLastSubMeshHit     = nullptr;
        mIsCollisionMesh    = isCollisionMesh;
        mNodeNumber         = nodeNumber;
        mNumOrgVerts        = numOrgVerts;
        mSkinningInfo       = nullptr;
        mMaterialNr         = 0;
        mMaxBonesPerSubMesh = maxBonesPerSubMesh; // default: 65 bones maximum per submesh (195 constants when using 4x3 matrices)
        mMaxSubMeshVertices = maxSubMeshVertices; // default: 65535 (max 16 bit value)
        m_optimizeDuplicates = optimizeDuplicates;

        mPolyBoneList.SetMemoryCategory(EMFX_MEMCATEGORY_MESHBUILDER);
        mSubMeshes.SetMemoryCategory(EMFX_MEMCATEGORY_MESHBUILDER);
        mLayers.SetMemoryCategory(EMFX_MEMCATEGORY_MESHBUILDER);

        mPolyIndices.SetMemoryCategory(EMFX_MEMCATEGORY_MESHBUILDER);
        mPolyOrgVertexNumbers.SetMemoryCategory(EMFX_MEMCATEGORY_MESHBUILDER);
        mPolyVertexCounts.SetMemoryCategory(EMFX_MEMCATEGORY_MESHBUILDER);

        mVertices.SetMemoryCategory(EMFX_MEMCATEGORY_MESHBUILDER);
        mVertices.Resize(numOrgVerts);
        for (uint32 i = 0; i < numOrgVerts; ++i)
        {
            mVertices[i].SetMemoryCategory(EMFX_MEMCATEGORY_MESHBUILDER);
        }
    }


    // destructor
    MeshBuilder::~MeshBuilder()
    {
        uint32 i;

        // get rid of all vertex attribute layers
        const uint32 numLayers = mLayers.GetLength();
        for (i = 0; i < numLayers; ++i)
        {
            mLayers[i]->Destroy();
        }
        mLayers.Clear();

        // get rid of all sub meshes
        const uint32 numSubMeshes = mSubMeshes.GetLength();
        for (i = 0; i < numSubMeshes; ++i)
        {
            mSubMeshes[i]->Destroy();
        }
        mSubMeshes.Clear();

        // get rid of the skinning info
        if (mSkinningInfo)
        {
            mSkinningInfo->Destroy();
        }

        mVertices.Clear();
    }


    // create
    MeshBuilder* MeshBuilder::Create(uint32 nodeNumber, uint32 numOrgVerts, uint32 maxBonesPerSubMesh, uint32 maxSubMeshVertices, bool isCollisionMesh, bool optimizeDuplicates)
    {
        return aznew MeshBuilder(nodeNumber, numOrgVerts, maxBonesPerSubMesh, maxSubMeshVertices, isCollisionMesh, optimizeDuplicates);
    }

    // init the default value to renderer's limit.
    const int MeshBuilder::s_defaultMaxBonesPerSubMesh = 512;
    const int MeshBuilder::s_defaultMaxSubMeshVertices = 65535;

    MeshBuilder* MeshBuilder::Create(uint32 nodeNumber, uint32 numOrgVerts, bool isCollisionMesh, bool optimizeDuplicates)
    {
        return Create(nodeNumber, numOrgVerts, s_defaultMaxBonesPerSubMesh, s_defaultMaxSubMeshVertices, isCollisionMesh, optimizeDuplicates);
    }

    // try to find the layer
    MeshBuilderVertexAttributeLayer* MeshBuilder::FindLayer(uint32 layerID, uint32 occurrence) const
    {
        uint32 count = 0;
        const uint32 numLayers = mLayers.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            if (mLayers[i]->GetTypeID() == layerID)
            {
                if (occurrence == count)
                {
                    return mLayers[i];
                }
                else
                {
                    count++;
                }
            }
        }

        return nullptr;
    }


    // calculate the number of layers for the given type
    uint32 MeshBuilder::GetNumLayers(uint32 layerID) const
    {
        uint32 count = 0;
        const uint32 numLayers = mLayers.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            if (mLayers[i]->GetTypeID() == layerID)
            {
                count++;
            }
        }

        return count;
    }


    // add a layer
    void MeshBuilder::AddLayer(MeshBuilderVertexAttributeLayer* layer)
    {
        mLayers.Add(layer);
    }


    MeshBuilderVertexLookup MeshBuilder::FindMatchingDuplicate(uint32 orgVertexNr)
    {
        // check with all vertex duplicates
        const uint32 numDuplicates = mLayers[0]->GetNumDuplicates(orgVertexNr);
        for (uint32 d = 0; d < numDuplicates; ++d)
        {
            // check if the submitted vertex data is equal in all layers for the current duplicate
            bool allDataEqual = true;
            const uint32 numLayers = mLayers.GetLength();
            for (uint32 layer = 0; layer < numLayers && allDataEqual; ++layer)
            {
                if (mLayers[layer]->CheckIfIsVertexEqual(orgVertexNr, d) == false)
                {
                    allDataEqual = false;
                }
            }

            // if so, we have found a matching vertex!
            if (allDataEqual)
            {
                return MeshBuilderVertexLookup(orgVertexNr, d);
            }
        }

        // no matching vertex found
        return MeshBuilderVertexLookup();
    }


    MeshBuilderVertexLookup MeshBuilder::AddVertex(uint32 orgVertexNr)
    {
        // when there are no layers, there is nothing to do
        const uint32 numLayers = mLayers.GetLength();
        if (numLayers == 0)
        {
            return MeshBuilderVertexLookup();
        }

        // try to find a matching duplicate number for the current vertex
        // if there isn't a similar vertex, we have to submit it to all layers
        MeshBuilderVertexLookup index;       
        if (m_optimizeDuplicates)
        {
            index = FindMatchingDuplicate(orgVertexNr);
        }

        if (index.mOrgVtx == MCORE_INVALIDINDEX32)
        {
            for (uint32 i = 0; i < numLayers; ++i)
            {
                mLayers[i]->AddVertex(orgVertexNr);
                index.mOrgVtx = orgVertexNr;
                index.mDuplicateNr = mLayers[i]->GetNumDuplicates(orgVertexNr) - 1;
            }
        }

        return index;
    }


    // find the index value for the current set vertex
    MeshBuilderVertexLookup MeshBuilder::FindVertexIndex(uint32 orgVertexNr)
    {
        // if there are no layers, we can't find a valid index
        if (mLayers.GetLength() == 0)
        {
            return MeshBuilderVertexLookup();
        }

        // try to locate a matching duplicate
        return FindMatchingDuplicate(orgVertexNr);
    }



    void MeshBuilder::BeginPolygon(uint32 materialNr)
    {
        mMaterialNr     = materialNr;
        mPolyIndices.Clear();
        mPolyOrgVertexNumbers.Clear();
    }


    void MeshBuilder::AddPolygonVertex(uint32 orgVertexNr)
    {
        mPolyIndices.Add(AddVertex(orgVertexNr));
        mPolyOrgVertexNumbers.Add(orgVertexNr);
    }


    void MeshBuilder::EndPolygon()
    {
        MCORE_ASSERT(mPolyIndices.GetLength() >= 3);

        // add the triangle
        AddPolygon(mPolyIndices, mPolyOrgVertexNumbers, mMaterialNr);
    }


    MeshBuilderSubMesh* MeshBuilder::FindSubMeshForPolygon(const MCore::Array<uint32>& orgVertexNumbers, uint32 materialNr)
    {
        // collect all bones that influence the given polygon
        MCore::Array<uint32> polyBoneList;
        ExtractBonesForPolygon(orgVertexNumbers, &polyBoneList);

        // create our list of possible submeshes, start value are all submeshes available
        MCore::Array<MeshBuilderSubMesh*> possibleSubMeshes;
        possibleSubMeshes = mSubMeshes;

        while (possibleSubMeshes.GetLength() > 0)
        {
            uint32          maxMatchings            = 0;
            uint32          foundSubMeshNr          = MCORE_INVALIDINDEX32;
            const uint32    numPossibleSubMeshes    = possibleSubMeshes.GetLength();

            // iterate over all submeshes and find the one with the most similar bones
            for (uint32 i = 0; i < numPossibleSubMeshes; ++i)
            {
                // get the number of matching bones from the current submesh and the given polygon
                const uint32 currentNumMatches = possibleSubMeshes[i]->CalcNumSimilarBones(polyBoneList);

                // if the current submesh has more similar bones than the current maximum we found a better one
                if (currentNumMatches > maxMatchings)
                {
                    // check if this submesh already our perfect match
                    if (currentNumMatches == polyBoneList.GetLength())
                    {
                        // check if the submesh which has the most common bones with the given polygon can handle it
                        if (possibleSubMeshes[i]->CanHandlePolygon(orgVertexNumbers, materialNr, &mPolyBoneList))
                        {
                            return possibleSubMeshes[i];
                        }
                    }

                    maxMatchings    = currentNumMatches;
                    foundSubMeshNr  = i;
                }
            }

            // if we cannot find a submesh return nullptr and create a new one
            if (foundSubMeshNr == MCORE_INVALIDINDEX32)
            {
                for (uint32 i = 0; i < possibleSubMeshes.GetLength(); ++i)
                {
                    // check if the submesh which has the most common bones with the given polygon can handle it
                    if (possibleSubMeshes[i]->CanHandlePolygon(orgVertexNumbers, materialNr, &mPolyBoneList))
                    {
                        return possibleSubMeshes[i];
                    }
                }

                return nullptr;
            }

            // check if the submesh which has the most common bones with the given polygon can handle it
            if (possibleSubMeshes[foundSubMeshNr]->CanHandlePolygon(orgVertexNumbers, materialNr, &mPolyBoneList))
            {
                return possibleSubMeshes[foundSubMeshNr];
            }

            // remove the found submesh from the possible submeshes directly so that we can don't find the same one in the next iteration again
            possibleSubMeshes.Remove(foundSubMeshNr);
        }

        return nullptr;
    }


    // add a polygon
    void MeshBuilder::AddPolygon(const MCore::Array<MeshBuilderVertexLookup>& indices, const MCore::Array<uint32>& orgVertexNumbers, uint32 materialNr)
    {
        // add the polygon to the list of poly vertex counts
        mPolyVertexCounts.Add(indices.GetLength());

        // try to find a submesh where we can add it
        MeshBuilderSubMesh* subMesh = FindSubMeshForPolygon(orgVertexNumbers, materialNr);

        // if there is none where we can add it to, create a new one
        if (subMesh == nullptr)
        {
            subMesh = MeshBuilderSubMesh::Create(materialNr, this);
            mSubMeshes.Add(subMesh);
            mLastSubMeshHit = subMesh;
        }

        // add the polygon to the submesh
        ExtractBonesForPolygon(orgVertexNumbers, &mPolyBoneList);
        subMesh->AddPolygon(indices, mPolyBoneList);
    }


    // optimize the submeshes
    void MeshBuilder::OptimizeTriangleList()
    {
        if (CheckIfIsTriangleMesh() == false)
        {
            return;
        }

        // optimize all submeshes
        const uint32 numSubMeshes = mSubMeshes.GetLength();
        for (uint32 i = 0; i < numSubMeshes; ++i)
        {
            mSubMeshes[i]->Optimize();
        }
    }


    void MeshBuilder::LogContents()
    {
        MCore::LogInfo("---------------------------------------------------------------------------");
        MCore::LogInfo("Mesh for node nr %d (colMesh=%d)", mNodeNumber, mIsCollisionMesh);
        const uint32 numLayers = mLayers.GetLength();
        MCore::LogInfo("Num org verts = %d", mNumOrgVerts);
        MCore::LogInfo("Num layers    = %d", numLayers);
        MCore::LogInfo("Num polys     = %d", GetNumPolygons());
        MCore::LogInfo("IsTriMesh     = %d", CheckIfIsTriangleMesh());
        MCore::LogInfo("IsQuaddMesh   = %d", CheckIfIsQuadMesh());

        for (uint32 i = 0; i < numLayers; ++i)
        {
            MeshBuilderVertexAttributeLayer* layer = mLayers[i];
            MCore::LogInfo("Layer #%d:", i);
            MCore::LogInfo("  - Type type ID   = %d", layer->GetTypeID());
            MCore::LogInfo("  - Num vertices   = %d", layer->CalcNumVertices());
            MCore::LogInfo("  - Attribute size = %d bytes", layer->GetAttributeSizeInBytes());
            MCore::LogInfo("  - Layer size     = %d bytes", layer->CalcLayerSizeInBytes());
            MCore::LogInfo("  - Name           = %s", layer->GetName());
        }
        MCore::LogInfo("");

        const uint32 numSubMeshes = mSubMeshes.GetLength();
        MCore::LogInfo("Num Submeshes = %d", numSubMeshes);
        for (uint32 i = 0; i < numSubMeshes; ++i)
        {
            MeshBuilderSubMesh* subMesh = mSubMeshes[i];
            MCore::LogInfo("Submesh #%d:", i);
            MCore::LogInfo("  - Material    = %d", subMesh->GetMaterialIndex());
            MCore::LogInfo("  - Num verts   = %d", subMesh->GetNumVertices());
            MCore::LogInfo("  - Num indices = %d (%d polys)", subMesh->GetNumIndices(), subMesh->GetNumPolygons());
            MCore::LogInfo("  - Num bones   = %d", subMesh->GetNumBones());
        }
    }



    void MeshBuilder::ExtractBonesForPolygon(const MCore::Array<uint32>& orgVertexNumbers, MCore::Array<uint32>* outPolyBoneList) const
    {
        // get rid of existing data
        outPolyBoneList->Clear(false);

        // get the skinning info, if there is any
        MeshBuilderSkinningInfo* skinningInfo = GetSkinningInfo();
        if (skinningInfo == nullptr)
        {
            return;
        }

        // for all 3 vertices of the polygon
        const uint32 numPolyVerts = orgVertexNumbers.GetLength();
        for (uint32 i = 0; i < numPolyVerts; ++i)
        {
            const uint32 orgVtxNr = orgVertexNumbers[i];

            // traverse all influences for this vertex
            const uint32 numInfluences = skinningInfo->GetNumInfluences(orgVtxNr);
            for (uint32 n = 0; n < numInfluences; ++n)
            {
                // get the bone of the influence
                uint32 nodeNr = skinningInfo->GetInfluence(orgVtxNr, n).mNodeNr;

                // if it isn't yet in the output array with bones, add it
                if (outPolyBoneList->Find(nodeNr) == MCORE_INVALIDINDEX32)
                {
                    outPolyBoneList->Add(nodeNr);
                }
            }
        }
    }


    // optimize memory usage (minimize it)
    void MeshBuilder::OptimizeMemoryUsage()
    {
        const uint32 numLayers = mLayers.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            mLayers[i]->OptimizeMemoryUsage();
        }
    }


    // convert the endian type of all layers in the mesh
    void MeshBuilder::ConvertEndian(MCore::Endian::EEndianType sourceEndian, MCore::Endian::EEndianType destEndian)
    {
        const uint32 numLayers = mLayers.GetLength();
        for (uint32 i = 0; i < numLayers; ++i)
        {
            mLayers[i]->ConvertEndian(sourceEndian, destEndian);
        }
    }


    // calculate the number of indices
    uint32 MeshBuilder::CalcNumIndices() const
    {
        uint32 totalIndices = 0;

        // add all indices of the submeshes
        const uint32 numSubMeshes = mSubMeshes.GetLength();
        for (uint32 i = 0; i < numSubMeshes; ++i)
        {
            totalIndices += mSubMeshes[i]->GetNumIndices();
        }

        return totalIndices;
    }


    // calculate the number of vertices
    uint32 MeshBuilder::CalcNumVertices() const
    {
        uint32 total = 0;

        // add all indices of the submeshes
        const uint32 numSubMeshes = mSubMeshes.GetLength();
        for (uint32 i = 0; i < numSubMeshes; ++i)
        {
            total += mSubMeshes[i]->GetNumVertices();
        }

        return total;
    }


    void AddZeroVectors(MCore::Array<AZ::Vector3>& tangents, MCore::Array<AZ::Vector3>& bitangents, MCore::Array<AZ::Vector2>& tangentsSpherical, MCore::Array<AZ::Vector2>& bitangentsSpherical, uint32 numDuplicates)
    {
        // check if we have already added the zero vectors
        if (tangents.GetIsEmpty())
        {
            tangents.Resize(numDuplicates);
            bitangents.Resize(numDuplicates);
            tangentsSpherical.Resize(numDuplicates);
            bitangentsSpherical.Resize(numDuplicates);

            for (uint32 dupeNr = 0; dupeNr < numDuplicates; ++dupeNr)
            {
                tangents[dupeNr] = AZ::Vector3::CreateZero();
                bitangents[dupeNr] = AZ::Vector3::CreateZero();
                tangentsSpherical[dupeNr]  = AZ::Vector2::CreateZero();
                bitangentsSpherical[dupeNr] = AZ::Vector2::CreateZero();
            }
        }
    }


    // convert the mesh into an emotion fx mesh
    Mesh* MeshBuilder::ConvertToEMotionFXMesh()
    {
        // multi threaded vertex order generation for all sub meshes in advance
        GenerateSubMeshVertexOrders();

        // create the emfx mesh object
        const uint32 numVerts       = CalcNumVertices();
        const uint32 numIndices     = CalcNumIndices();
        const uint32 numOrgVerts    = GetNumOrgVerts();
        const uint32 numPolygons    = GetNumPolygons();
        Mesh* result = Mesh::Create(numVerts, numIndices, numPolygons, numOrgVerts, mIsCollisionMesh);

        // convert all layers
        const uint32 numLayers = mLayers.GetLength();
        result->ReserveVertexAttributeLayerSpace(numLayers);
        for (uint32 layerNr = 0; layerNr < numLayers; ++layerNr)
        {
            // create the layer
            MeshBuilderVertexAttributeLayer* builderLayer = GetLayer(layerNr);
            VertexAttributeLayerAbstractData* layer = VertexAttributeLayerAbstractData::Create(numVerts, builderLayer->GetTypeID(), builderLayer->GetAttributeSizeInBytes(), builderLayer->GetIsDeformable());
            layer->SetName(builderLayer->GetName());
            result->AddVertexAttributeLayer(layer);

            // read the data from disk into the layer
            MCORE_ASSERT(layer->CalcTotalDataSizeInBytes(false) == (builderLayer->GetAttributeSizeInBytes() * numVerts));

            uint32 vertexIndex = 0;
            const uint32 numSubMeshes = GetNumSubMeshes();
            for (uint32 s = 0; s < numSubMeshes; ++s)
            {
                MeshBuilderSubMesh* subMesh = GetSubMesh(s);
                const uint32 numSubVerts = subMesh->GetNumVertices();
                for (uint32 v = 0; v < numSubVerts; ++v)
                {
                    MeshBuilderVertexLookup& vertexLookup = subMesh->GetVertex(v);
                    MCore::MemCopy(layer->GetOriginalData(vertexIndex++), builderLayer->GetVertexValue(vertexLookup.mOrgVtx, vertexLookup.mDuplicateNr), builderLayer->GetAttributeSizeInBytes());
                }
            }

            // copy the original data over the current data values
            layer->ResetToOriginalData();
        }

        // submesh offsets
        uint32 indexOffset  = 0;
        uint32 vertexOffset = 0;
        uint32 startPolygon = 0;
        uint32* indices     = result->GetIndices();
        uint8* polyVertexCounts = result->GetPolygonVertexCounts();

        // convert submeshes
        const uint32 numSubMeshes = GetNumSubMeshes();
        result->SetNumSubMeshes(numSubMeshes);
        for (uint32 s = 0; s < numSubMeshes; ++s)
        {
            // create the submesh
            MeshBuilderSubMesh* builderSubMesh = GetSubMesh(s);
            SubMesh* subMesh = SubMesh::Create(result, vertexOffset, indexOffset, startPolygon, builderSubMesh->GetNumVertices(), builderSubMesh->GetNumIndices(), builderSubMesh->GetNumPolygons(), builderSubMesh->GetMaterialIndex(), builderSubMesh->GetNumBones());
            result->SetSubMesh(s, subMesh);

            // convert the indices
            const uint32 numSubMeshIndices = builderSubMesh->GetNumIndices();
            for (uint32 i = 0; i < numSubMeshIndices; ++i)
            {
                indices[indexOffset + i] = builderSubMesh->GetIndex(i) + vertexOffset;
            }

            // convert the poly vertex counts
            const uint32 numSubMeshPolygons = builderSubMesh->GetNumPolygons();
            for (uint32 i = 0; i < numSubMeshPolygons; ++i)
            {
                polyVertexCounts[startPolygon + i] = builderSubMesh->GetPolygonVertexCount(i);
            }

            // read the bone/node numbers
            const uint32 numSubMeshBones = builderSubMesh->GetNumBones();
            for (uint32 i = 0; i < numSubMeshBones; ++i)
            {
                subMesh->SetBone(i, builderSubMesh->GetBone(i));
            }

            // increase the offsets
            indexOffset  += builderSubMesh->GetNumIndices();
            vertexOffset += builderSubMesh->GetNumVertices();
            startPolygon += builderSubMesh->GetNumPolygons();
        }

        //-------------------

        if (mSkinningInfo)
        {
            // add the skinning info to the mesh
            SkinningInfoVertexAttributeLayer* skinningLayer = SkinningInfoVertexAttributeLayer::Create(numOrgVerts);
            result->AddSharedVertexAttributeLayer(skinningLayer);

            // get the Array2D from the skinning layer
            for (uint32 v = 0; v < numOrgVerts; ++v)
            {
                const uint32 numInfluences = mSkinningInfo->GetNumInfluences(v);
                for (uint32 i = 0; i < numInfluences; ++i)
                {
                    const MeshBuilderSkinningInfo::Influence& influence = mSkinningInfo->GetInfluence(v, i);
                    skinningLayer->AddInfluence(v, influence.mNodeNr, influence.mWeight, 0);
                }
            }

            skinningLayer->OptimizeMemoryUsage(); // TODO: make this optional?
        }

        return result;
    }


    void MeshBuilder::GenerateSubMeshVertexOrders()
    {
        const uint32 numSubMeshes = mSubMeshes.GetLength();

        AZ::JobCompletion jobCompletion;           

        for (uint32 i = 0; i < numSubMeshes; ++i)
        {
            AZ::JobContext* jobContext = nullptr;
            AZ::Job* job = AZ::CreateJobFunction([this, i]()
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Animation, "MeshBuilder::GenerateSubMeshVertexOrders::SubMeshJob");
                mSubMeshes[i]->GenerateVertexOrder();
            }, true, jobContext);

            job->SetDependent(&jobCompletion);               
            job->Start();
        }

        jobCompletion.StartAndWaitForCompletion();
    }


    // check if we are a pure triangle mesh
    bool MeshBuilder::CheckIfIsTriangleMesh() const
    {
        const uint32 numPolys = mPolyVertexCounts.GetLength();
        for (uint32 i = 0; i < numPolys; ++i)
        {
            if (mPolyVertexCounts[i] != 3)
            {
                return false;
            }
        }

        return true;
    }


    // check if we are a pure quad mesh
    bool MeshBuilder::CheckIfIsQuadMesh() const
    {
        const uint32 numPolys = mPolyVertexCounts.GetLength();
        for (uint32 i = 0; i < numPolys; ++i)
        {
            if (mPolyVertexCounts[i] != 4)
            {
                return false;
            }
        }

        return true;
    }


    uint32 MeshBuilder::FindRealVertexNr(MeshBuilderSubMesh* subMesh, uint32 orgVtx, uint32 dupeNr)
    {
        const uint32 numIndices = mVertices[orgVtx].GetLength();
        for (uint32 i = 0; i < numIndices; ++i)
        {
            SubMeshVertex* subMeshVertex = &mVertices[orgVtx][i];
            if (subMeshVertex->mSubMesh == subMesh && subMeshVertex->mDupeNr == dupeNr)
            {
                return subMeshVertex->mRealVertexNr;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    MeshBuilder::SubMeshVertex* MeshBuilder::FindSubMeshVertex(MeshBuilderSubMesh* subMesh, uint32 orgVtx, uint32 dupeNr)
    {
        const uint32 numIndices = mVertices[orgVtx].GetLength();
        for (uint32 i = 0; i < numIndices; ++i)
        {
            SubMeshVertex* subMeshVertex = &mVertices[orgVtx][i];
            if (subMeshVertex->mSubMesh == subMesh && subMeshVertex->mDupeNr == dupeNr)
            {
                return subMeshVertex;
            }
        }

        return nullptr;
    }


    uint32 MeshBuilder::CalcNumVertexDuplicates(MeshBuilderSubMesh* subMesh, uint32 orgVtx) const
    {
        uint32 numDupes = 0;
        const uint32 numIndices = mVertices[orgVtx].GetLength();
        for (uint32 i = 0; i < numIndices; ++i)
        {
            const SubMeshVertex* submeshVertex = &mVertices[orgVtx][i];
            if (submeshVertex->mSubMesh == subMesh)
            {
                numDupes++;
            }
        }

        return numDupes;
    }


    uint32 MeshBuilder::GetNumSubMeshVertices(uint32 orgVtx)
    {
        return mVertices[orgVtx].GetLength();
    }


    void MeshBuilder::AddSubMeshVertex(uint32 orgVtx, const SubMeshVertex& vtx)
    {
        mVertices[orgVtx].Add(vtx);
    }
}   // namespace EMotionFX
