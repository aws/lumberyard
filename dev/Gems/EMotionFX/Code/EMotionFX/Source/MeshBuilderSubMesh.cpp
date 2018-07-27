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
#include "MeshBuilderSubMesh.h"
#include "MeshBuilderSkinningInfo.h"
#include "MeshBuilder.h"
#include "MeshBuilderVertexAttributeLayers.h"
#include <MCore/Source/TriangleListOptimizer.h>
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshBuilderSubMesh, MeshAllocator, 0)


    // constructor
    MeshBuilderSubMesh::MeshBuilderSubMesh(uint32 materialNr, MeshBuilder* mesh)
        : BaseObject()
    {
        mMaterialIndex  = materialNr;
        mMesh           = mesh;
        mNumVertices    = 0;

        mIndices.SetMemoryCategory(EMFX_MEMCATEGORY_MESHBUILDER_SUBMESH);
        mVertexOrder.SetMemoryCategory(EMFX_MEMCATEGORY_MESHBUILDER_SUBMESH);
        mBoneList.SetMemoryCategory(EMFX_MEMCATEGORY_MESHBUILDER_SUBMESH);
    }


    // destructor
    MeshBuilderSubMesh::~MeshBuilderSubMesh()
    {
    }


    // create
    MeshBuilderSubMesh* MeshBuilderSubMesh::Create(uint32 materialNr, MeshBuilder* mesh)
    {
        return aznew MeshBuilderSubMesh(materialNr, mesh);
    }


    // optimize the indices
    void MeshBuilderSubMesh::Optimize()
    {
        if (mVertexOrder.GetLength() != mNumVertices)
        {
            GenerateVertexOrder();
        }

        // get the number of indices
        const uint32 numIndices = mIndices.GetLength();

        // build an int array
        MCore::Array<uint32> indexArray;
        indexArray.Resize(numIndices);

        // fill the int array
        uint32 i;
        for (i = 0; i < numIndices; ++i)
        {
            indexArray[i] = GetIndex(i);
        }

        // optimize the int array on cache usage
        MCore::TriangleListOptimizer optimizer(8);  // 8 cache entries
        optimizer.OptimizeIndexBuffer(indexArray.GetPtr(), numIndices);

        // propagate the changed int array changes into the real index buffer
        for (i = 0; i < numIndices; ++i)
        {
            mIndices[i] = mVertexOrder[ indexArray[i] ];
        }
    }


    // map the vertices to their original vertex and dupe numbers
    void MeshBuilderSubMesh::GenerateVertexOrder()
    {
        // create our vertex order array and allocate numVertices lookups
        mVertexOrder.Resize(mNumVertices);

        // iterate through all original vertices
        const uint32 numOrgVertices = mMesh->GetNumOrgVerts();
        for (uint32 rowNr = 0; rowNr < numOrgVertices; ++rowNr)
        {
            const uint32 numSubMeshVertices = mMesh->GetNumSubMeshVertices(rowNr);
            for (uint32 i = 0; i < numSubMeshVertices; ++i)
            {
                MeshBuilder::SubMeshVertex* subMeshVertex = mMesh->GetSubMeshVertex(rowNr, i);

                if (subMeshVertex->mSubMesh == this && subMeshVertex->mRealVertexNr !=  MCORE_INVALIDINDEX32)
                {
                    const uint32 realVertexNr = subMeshVertex->mRealVertexNr;
                    mVertexOrder[ realVertexNr ].mOrgVtx        = rowNr;
                    mVertexOrder[ realVertexNr ].mDuplicateNr   = subMeshVertex->mDupeNr;
                }
            }
        }
    }


    // add a polygon to the submesh
    void MeshBuilderSubMesh::AddPolygon(const MCore::Array<MeshBuilderVertexLookup>& indices, const MCore::Array<uint32>& boneList)
    {
        // pre-allocate
        if (mIndices.GetLength() % 10000 == 0)
        {
            mIndices.Reserve(mIndices.GetLength() + 10000);
        }

        // for all vertices in the poly
        uint32 i;
        const uint32 numPolyVerts = indices.GetLength();
        MCORE_ASSERT(numPolyVerts <= 255);
        mPolyVertexCounts.Add(static_cast<uint8>(numPolyVerts));

        for (i = 0; i < numPolyVerts; ++i)
        {
            // add unused vertices to the list
            if (CheckIfHasVertex(indices[i]) == false)
            {
                const uint32 numDupes = mMesh->CalcNumVertexDuplicates(this, indices[i].mOrgVtx);
                const int32 numToAdd = (indices[i].mDuplicateNr - numDupes) + 1;
                for (int32 j = 0; j < numToAdd; ++j)
                {
                    MeshBuilder::SubMeshVertex subMeshVertex;
                    subMeshVertex.mSubMesh      = this;
                    subMeshVertex.mDupeNr       = numDupes + j;
                    subMeshVertex.mRealVertexNr = MCORE_INVALIDINDEX32;
                    mMesh->AddSubMeshVertex(indices[i].mOrgVtx, subMeshVertex);
                }

                MeshBuilder::SubMeshVertex* subMeshVertex = mMesh->FindSubMeshVertex(this, indices[i].mOrgVtx, indices[i].mDuplicateNr);
                subMeshVertex->mRealVertexNr = mNumVertices;
                mNumVertices++;
            }

            // an index in the local vertices array
            mIndices.Add(indices[i]);
        }

        // add the new bones
        const uint32 numBones = boneList.GetLength();
        for (i = 0; i < numBones; ++i)
        {
            if (mBoneList.Find(boneList[i]) == MCORE_INVALIDINDEX32)
            {
                mBoneList.Add(boneList[i]);
            }
        }
    }



    // check if we can handle a given poly inside the submesh
    bool MeshBuilderSubMesh::CanHandlePolygon(const MCore::Array<uint32>& orgVertexNumbers, uint32 materialNr, MCore::Array<uint32>* outBoneList) const
    {
        // if the material isn't the same, we can't handle it
        if (mMaterialIndex != materialNr)
        {
            return false;
        }

        // check if there is still space for the poly vertices (worst case scenario), and if this won't go over the 16 bit index buffer limit
        const uint32 numPolyVerts = orgVertexNumbers.GetLength();
        if (mNumVertices + numPolyVerts > mMesh->mMaxSubMeshVertices)
        {
            return false;
        }

        // if there is skinning info
        MeshBuilderSkinningInfo* skinningInfo = mMesh->GetSkinningInfo();
        if (skinningInfo)
        {
            // get the maximum number of allowed bones per submesh
            const uint32 maxNumBones = mMesh->GetMaxBonesPerSubMesh();

            // extract the list of bones used by this poly
            mMesh->ExtractBonesForPolygon(orgVertexNumbers, outBoneList);

            // check if worst case scenario would be allowed
            // this is when we have to add all triangle bones to the bone list
            if (mBoneList.GetLength() + outBoneList->GetLength() > maxNumBones)
            {
                return false;
            }

            // calculate the real number of extra bones needed
            uint32 numExtraNeeded = 0;
            const uint32 numPolyBones = outBoneList->GetLength();
            for (uint32 i = 0; i < numPolyBones; ++i)
            {
                if (mBoneList.Find(outBoneList->GetItem(i)) == MCORE_INVALIDINDEX32)
                {
                    numExtraNeeded++;
                }
            }

            // if we can't add the extra required bones to the list, because it would result in more than the
            // allowed number of bones, then return that we can't add this triangle to this submesh
            if (mBoneList.GetLength() + numExtraNeeded > maxNumBones)
            {
                return false;
            }
        }

        // yeah, we can add this triangle to the submesh
        return true;
    }


    bool MeshBuilderSubMesh::CheckIfHasVertex(const MeshBuilderVertexLookup& vertex)
    {
        if (mMesh->CalcNumVertexDuplicates(this, vertex.mOrgVtx) <= vertex.mDuplicateNr)
        {
            return false;
        }

        return (mMesh->FindRealVertexNr(this, vertex.mOrgVtx, vertex.mDuplicateNr) != MCORE_INVALIDINDEX32);
    }


    uint32 MeshBuilderSubMesh::GetIndex(uint32 index)
    {
        return mMesh->FindRealVertexNr(this, mIndices[index].mOrgVtx, mIndices[index].mDuplicateNr);
    }


    uint32 MeshBuilderSubMesh::CalcNumSimilarBones(const MCore::Array<uint32>& boneList) const
    {
        // reset our similar bones counter and get the number of bones from the input and
        // submesh bone lists
        uint32          numMatches      = 0;
        const uint32    numSubMeshBones = mBoneList.GetLength();
        const uint32    numInputBones   = boneList.GetLength();

        // iterate through all bones from the input bone list and find the matching bones
        for (uint32 i = 0; i < numInputBones; ++i)
        {
            for (uint32 j = 0; j < numSubMeshBones; ++j)
            {
                if (boneList[i] == mBoneList[j])
                {
                    numMatches++;
                    break;
                }
            }
        }

        return numMatches;
    }
}   // namespace EMotionFX

