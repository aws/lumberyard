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
#include <MCore/Source/Job.h>
#include <MCore/Source/JobList.h>
#include <MCore/Source/JobManager.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshBuilder, MeshAllocator, 0)

    
    // constructor
    MeshBuilder::MeshBuilder(uint32 nodeNumber, uint32 numOrgVerts, uint32 maxBonesPerSubMesh, uint32 maxSubMeshVertices, bool isCollisionMesh)
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
    MeshBuilder* MeshBuilder::Create(uint32 nodeNumber, uint32 numOrgVerts, uint32 maxBonesPerSubMesh, uint32 maxSubMeshVertices, bool isCollisionMesh)
    {
        return aznew MeshBuilder(nodeNumber, numOrgVerts, maxBonesPerSubMesh, maxSubMeshVertices, isCollisionMesh);
    }

    // init the default value to renderer's limit.
    const int MeshBuilder::s_defaultMaxBonesPerSubMesh = 512;
    const int MeshBuilder::s_defaultMaxSubMeshVertices = 65535;

    MeshBuilder* MeshBuilder::Create(uint32 nodeNumber, uint32 numOrgVerts, bool isCollisionMesh)
    {
        return Create(nodeNumber, numOrgVerts, s_defaultMaxBonesPerSubMesh, s_defaultMaxSubMeshVertices, isCollisionMesh);
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
        MeshBuilderVertexLookup index = FindMatchingDuplicate(orgVertexNr);
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


    //////////////////////////////////////////////////////////////////////////

    /*void FixTangentAndBinormal(MCore::Vector3& normal, MCore::Vector3& tangent, MCore::Vector3& bitangent)
    {
        // this is where we can do a final check for zero length vectors
        // and set them to something appropriate
        const float tangentLength   = tangent.Length();
        const float bitangentLength = bitangent.Length();

        // check if the tangent or binormal is not normalized
        if (tangentLength <= 0.001f || bitangentLength <= 0.001f)
        {
            // the tangent space is ill defined at this vertex
            // so we can generate a valid one based on the normal vector,
            // which we are assuming is valid
            if (tangentLength > 0.5f && normal.Dot( tangent ) < 0.999f && normal.Dot( tangent ) > -0.999f)
            {
                //LogWarning( "The tangent is invalid. Trying to fix it. Please contact MysticGD support." );

                // make sure that the normal and tangent are different enough
                if (normal.Dot( tangent ) > 0.999f || normal.Dot( tangent ) < -0.999f)
                    tangent = bitangent.Cross( normal );

                // the tangent is valid, so we can just use that to calculate the binormal
                bitangent = normal.Cross( tangent );
            }
            else if (bitangentLength > 0.5f && normal.Dot( bitangent ) < 0.999f && normal.Dot( bitangent ) > -0.999f)
            {
                //LogWarning( "The bitangent is invalid. Trying to fix it. Please contact MysticGD support.");

                // the bitangent is good and we can use it to calculate the tangent
                tangent = bitangent.Cross( normal );
            }
            else
            {
                //LogWarning( "Tangent and bitangent seem to be invalid. Trying to fix it. Please contact MysticGD support.");

                // both vectors are invalid, so we should create something that is at least valid if not correct
                MCore::Vector3 xAxis( 1.0f , 0.0f , 0.0f );
                MCore::Vector3 yAxis( 0.0f , 1.0f , 0.0f );

                // check two possible axis, because the normal could be one of them,
                // and we want to chose a different one to start making our valid basis
                //  find out which is further away from it by checking the dot product
                MCore::Vector3 startAxis;

                if (xAxis.Dot( normal ) < yAxis.Dot( normal ))
                    // the x axis is more different than the y axis when compared to the normal
                    startAxis = xAxis;
                else
                    //the y axis is more different than the x axis when compared to the normal
                    startAxis = yAxis;

                tangent = normal.Cross( startAxis );
                bitangent = normal.Cross( tangent );
            }
        }

        // make sure that the tangent and binormal are different enough
        if (bitangent.Dot( tangent ) > 0.999f || bitangent.Dot( tangent ) < -0.999f)
        {
            //LogWarning( "Bitangent and tangent seem to be parallel. Trying to fix it. Please contact MysticGD support.");
            bitangent = normal.Cross( tangent );
        }

        // make sure that the normal and bitangent are different enough
        if (normal.Dot( bitangent ) > 0.999f || normal.Dot( bitangent ) < -0.999f)
        {
            //LogWarning( "Normal and bitangent seem to be parallel. Trying to fix it. Please contact MysticGD support.");
            bitangent = normal.Cross( tangent );
        }

        // make sure that the normal and tangent are different enough
        if (normal.Dot( tangent ) > 0.999f || normal.Dot( tangent ) < -0.999f)
        {
            //LogWarning( "Normal and tangent seem to be parallel. Trying to fix it. Please contact MysticGD support.");
            tangent = bitangent.Cross( normal );
        }
    }


    void OrthogonalizeTangentAndBinormal(MCore::Vector3& normal, MCore::Vector3& tangent, MCore::Vector3& bitangent)
    {
        // check for zero lengh normal vectors
        if (normal.Length() < Math::epsilon)
            LogWarning( "Zero length normal. Tangent orthogonalization will fail for the current vertex." );

        // fix the tangent and bitangent before we start the gram schmidt process
        //FixTangentAndBinormal( normal, tangent, bitangent );

        // Gram�Schmidt process
        // orthogonalizes a set of vectors in an inner product space
        // now with T and B and N we can get from tangent space to object space
        // we can use the Gram-Schmidt algorithm to find the newTangent and the newBinormal
        // newT = T - (N dot T)N
        // newB = B - (N dot B)N - (newT dot B)newT
        tangent = tangent - (normal.Dot(tangent)) * normal;
        tangent.SafeNormalize();
        bitangent = bitangent - (normal.Dot(bitangent)) * normal - (tangent.Dot(bitangent)) * tangent;
        bitangent.SafeNormalize();

        // fix the tangent and bitangent after the gram schmidt process as results are not always correct
        //FixTangentAndBinormal( normal, tangent, bitangent );
    }*/


    void AddZeroVectors(MCore::Array<AZ::Vector3>& tangents, MCore::Array<AZ::Vector3>& binormals, MCore::Array<AZ::Vector2>& tangentsSpherical, MCore::Array<AZ::Vector2>& binormalsSpherical, uint32 numDuplicates)
    {
        // check if we have already added the zero vectors
        if (tangents.GetIsEmpty())
        {
            tangents.Resize(numDuplicates);
            binormals.Resize(numDuplicates);
            tangentsSpherical.Resize(numDuplicates);
            binormalsSpherical.Resize(numDuplicates);

            for (uint32 dupeNr = 0; dupeNr < numDuplicates; ++dupeNr)
            {
                tangents[dupeNr] = AZ::Vector3::CreateZero();
                binormals[dupeNr] = AZ::Vector3::CreateZero();
                tangentsSpherical[dupeNr]  = AZ::Vector2::CreateZero();
                binormalsSpherical[dupeNr] = AZ::Vector2::CreateZero();
            }
        }
    }

    /*
    // calculate the tangents and bitangents of the given mesh and average them if wanted
    MeshBuilder* CalculateTangents(MeshBuilder* mesh, const bool onlyExportForFirstUVLayer)
    {
        if (mesh == nullptr)
            return nullptr;

        MCore::Timer calculateTangentsTimer;

        // get the position layer
        MeshBuilderVertexAttributeLayer* positionLayer  = mesh->FindLayer( EMotionFX::Mesh::ATTRIB_POSITIONS );
        MeshBuilderVertexAttributeLayer* normalLayer    = mesh->FindLayer( EMotionFX::Mesh::ATTRIB_NORMALS );

        // uv layers
        MCore::Array<MeshBuilderVertexAttributeLayer*> uvLayers;
        uint32 numUVLayers = mesh->GetNumLayers( EMotionFX::Mesh::ATTRIB_UVCOORDS );

        // only export tangents for the first uv layer
        if (onlyExportForFirstUVLayer && numUVLayers > 0)
            numUVLayers = 1;

        uvLayers.Reserve( numUVLayers );
        for (uint32 i=0; i<numUVLayers; ++i)
            uvLayers.Add( mesh->FindLayer(EMotionFX::Mesh::ATTRIB_UVCOORDS, i) );

        // get the number of original vertices
        const uint32 numOrgVertices = mesh->GetNumOrgVerts();

        // iterate through all uv layers and calculate the tangents
        for (uint32 i=0; i<numUVLayers; ++i)
        {
            // get the current uvLayer
            MeshBuilderVertexAttributeLayer* uvLayer = uvLayers[i];

            // this array stores the tangents and bitangents per original vertex
            // the 2d array is the better solution than "Array< Array< T > >"
            // because the Array inside Array will perform many allocations
            // the layout of the array is as following:
            // [orgVtx0]: [tangent0][tangent1][tangent2]
            // [orgVtx1]: [tangent0][tangent1]
            // [orgVtx2]: [tangent0][tangent1][tangent2][tangent3]
            // [orgVtx3]: [tangent0]
            MCore::Array< MCore::Array<Vector3> > tangents;
            MCore::Array< MCore::Array<Vector3> > bitangents;

            MCore::Array< Array<AZ::Vector2> > tangentsSpherical;
            MCore::Array< Array<AZ::Vector2> > binormalsSpherical;

            MCore::Array<uint32> numTangents;

            tangents.Resize             ( numOrgVertices );
            bitangents.Resize           ( numOrgVertices );
            tangentsSpherical.Resize    ( numOrgVertices );
            binormalsSpherical.Resize   ( numOrgVertices );
            numTangents.Resize          ( numOrgVertices );

            for (uint32 ti=0; ti<numOrgVertices; ++ti)
            {
                numTangents[ti] = 0;
            }

            // get the number of submeshes in the current mesh and iterate through them
            const uint32 numSubMeshes = mesh->GetNumSubMeshes();
            for (uint32 s=0; s<numSubMeshes; ++s)
            {
                // get the current suso tbmesh
                MeshBuilderSubMesh* subMesh = mesh->GetSubMesh( s );

                // get the number of indices in the current submesh and iterate over the triangles
                const uint32 numIndices = subMesh->GetNumIndices();
                for (uint32 i=0; i<numIndices; i+=3)
                {
                    // get the three indices (positions) for the current triangle
                    uint32 vtxIndex1    = subMesh->GetIndex( i+0 );
                    uint32 vtxIndex2    = subMesh->GetIndex( i+1 );
                    uint32 vtxIndex3    = subMesh->GetIndex( i+2 );

                    // get the original vertex numbers for the triangle vertices
                    uint32 orgVtx1      = subMesh->GetVertex( vtxIndex1 ).mOrgVtx;
                    uint32 orgVtx2      = subMesh->GetVertex( vtxIndex2 ).mOrgVtx;
                    uint32 orgVtx3      = subMesh->GetVertex( vtxIndex3 ).mOrgVtx;

                    // get the duplication vertex numbers for the triangle vertices
                    uint32 duplicateNr1 = subMesh->GetVertex( vtxIndex1 ).mDuplicateNr;
                    uint32 duplicateNr2 = subMesh->GetVertex( vtxIndex2 ).mDuplicateNr;
                    uint32 duplicateNr3 = subMesh->GetVertex( vtxIndex3 ).mDuplicateNr;

                    // get the position vectors for the triangle vertices
                    Vector3* position1  = (Vector3*)positionLayer->GetVertexValue( orgVtx1, duplicateNr1 );
                    Vector3* position2  = (Vector3*)positionLayer->GetVertexValue( orgVtx2, duplicateNr2 );
                    Vector3* position3  = (Vector3*)positionLayer->GetVertexValue( orgVtx3, duplicateNr3 );

                    // get the uvs for the triangle vertices
                    AZ::Vector2* uv1        = static_cast<AZ::Vector2*>(uvLayer->GetVertexValue( orgVtx1, duplicateNr1 ));
                    AZ::Vector2* uv2        = static_cast<AZ::Vector2*>(uvLayer->GetVertexValue( orgVtx2, duplicateNr2 ));
                    AZ::Vector2* uv3        = static_cast<AZ::Vector2*>(uvLayer->GetVertexValue( orgVtx3, duplicateNr3 ));

                    // calculate the tangent and binormal for a given triangle
                    Vector3 tangent, binormal;
                    EMotionFX::Mesh::CalcTangentAndBiNormalForFace( *position1, *position2, *position3, *uv1, *uv2, *uv3, &tangent, &binormal );

                    //LogInfo("=======================================");
                    //LogInfo("CalcTangentAndBiNormalForFace:");
                    //LogInfo("position1(orgVtx=%i, dupeNr=%i): (%f, %f, %f)", orgVtx1, duplicateNr1, position1->x, position1->y, position1->z);
                    //LogInfo("position2(orgVtx=%i, dupeNr=%i): (%f, %f, %f)", orgVtx2, duplicateNr2, position2->x, position2->y, position2->z);
                    //LogInfo("position3(orgVtx=%i, dupeNr=%i): (%f, %f, %f)", orgVtx3, duplicateNr3, position3->x, position3->y, position3->z);
                    //LogInfo("uv1: (%f, %f)", uv1->x, uv1->y);
                    //LogInfo("uv2: (%f, %f)", uv2->x, uv2->y);
                    //LogInfo("uv3: (%f, %f)", uv3->x, uv3->y);
                    //LogInfo("tangent: (%f, %f, %f)", tangent.x, tangent.y, tangent.z);
                    //LogInfo("binormal: (%f, %f, %f)", binormal.x, binormal.y, binormal.z);

                    AddZeroVectors( tangents[orgVtx1], bitangents[orgVtx1], tangentsSpherical[orgVtx1], binormalsSpherical[orgVtx1], positionLayer->GetNumDuplicates( orgVtx1 ) );
                    AddZeroVectors( tangents[orgVtx2], bitangents[orgVtx2], tangentsSpherical[orgVtx2], binormalsSpherical[orgVtx2], positionLayer->GetNumDuplicates( orgVtx2 ) );
                    AddZeroVectors( tangents[orgVtx3], bitangents[orgVtx3], tangentsSpherical[orgVtx3], binormalsSpherical[orgVtx3], positionLayer->GetNumDuplicates( orgVtx3 ) );

                    // add the tangent for each of the three original vertices
                    tangents[orgVtx1][duplicateNr1] += tangent;
                    tangents[orgVtx2][duplicateNr2] += tangent;
                    tangents[orgVtx3][duplicateNr3] += tangent;

                    // add the bitangent for each of the three original vertices
                    bitangents[orgVtx1][duplicateNr1] += binormal;
                    bitangents[orgVtx2][duplicateNr2] += binormal;
                    bitangents[orgVtx3][duplicateNr3] += binormal;

                    // do the same in spherical coordinates
                    AZ::Vector2 tangentSpherical        = MCore::ToSpherical( tangent.SafeNormalized() );
                    AZ::Vector2 binormalSpherical   = MCore::ToSpherical( binormal.SafeNormalized() );

                    tangentsSpherical[orgVtx1][duplicateNr1] += tangentSpherical;
                    tangentsSpherical[orgVtx2][duplicateNr2] += tangentSpherical;
                    tangentsSpherical[orgVtx3][duplicateNr3] += tangentSpherical;

                    binormalsSpherical[orgVtx1][duplicateNr1] += binormalSpherical;
                    binormalsSpherical[orgVtx2][duplicateNr2] += binormalSpherical;
                    binormalsSpherical[orgVtx3][duplicateNr3] += binormalSpherical;

                    numTangents[orgVtx1] = numTangents[orgVtx1] + 1;
                    numTangents[orgVtx2] = numTangents[orgVtx2] + 1;
                    numTangents[orgVtx3] = numTangents[orgVtx3] + 1;
                } // for all indices
            } // for all submeshes


            //////////////////////////////////////////////////////////////////////////
            // create the tangents layers
            //////////////////////////////////////////////////////////////////////////
            MeshBuilderVertexAttributeLayerVector4* newTangentLayer = new MeshBuilderVertexAttributeLayerVector4( numOrgVertices, EMotionFX::Mesh::ATTRIB_TANGENTS, false, true );
            for (uint32 orgVtx=0; orgVtx<numOrgVertices; orgVtx++)
            {
                // get the number of dupes for this vertex
                const uint32 numDuplicates = normalLayer->GetNumDuplicates( orgVtx );

                // iterate over the duplicates
                for (uint32 duplicateNr=0; duplicateNr<numDuplicates; ++duplicateNr)
                {
                    // get the current normal, tangent and bitangent
                    Vector3 normal      = *((Vector3*)normalLayer->GetVertexValue( orgVtx, duplicateNr ));
                    Vector3 tangent     = tangents[orgVtx][duplicateNr];
                    Vector3 binormal    = bitangents[orgVtx][duplicateNr];

                    normal.SafeNormalize();
                    tangent.SafeNormalize();
                    binormal.SafeNormalize();

                    if (tangent.SafeLength() < Math::epsilon)
                    {
                        AZ::Vector2 tangentSpherical = tangentsSpherical[orgVtx][duplicateNr];
                        tangentSpherical /= (float)numTangents[orgVtx];
                        tangent = MCore::FromSpherical( tangentSpherical );
                    }

                    if (binormal.SafeLength() < Math::epsilon)
                    {
                        AZ::Vector2 binormalSpherical = binormalsSpherical[orgVtx][duplicateNr];
                        binormalSpherical /= (float)numTangents[orgVtx];
                        binormal = MCore::FromSpherical( binormalSpherical );
                    }

                    // normalize the vectors
                    normal.SafeNormalize();
                    tangent.SafeNormalize();
                    binormal.SafeNormalize();

                    // Gram-Schmidt orthogonalize
                    Vector3 fixedTangent = tangent - (normal * normal.Dot(tangent));
                    fixedTangent.SafeNormalize();

                    // calculate handedness
                    const Vector3 crossResult = normal.Cross(tangent);
                    const float tangentW = (crossResult.Dot(binormal) < 0.0f) ? -1.0f : 1.0f;

                    //LogInfo("==========================================");
                    //LogInfo("OrgVtx=%i, DupeNr=%i", orgVtx, duplicateNr);
                    //LogInfo("normal: (%f, %f, %f)", normal.x, normal.y, normal.z);
                    //LogInfo("tangent: (%f, %f, %f)", tangent.x, tangent.y, tangent.z);
                    //LogInfo("binormal: (%f, %f, %f)", binormal.x, binormal.y, binormal.z);
                    //LogInfo("fixedTangent: (%f, %f, %f)", fixedTangent.x, fixedTangent.y, fixedTangent.z);
                    //LogInfo("tangentW: %f", tangentW);

                    // store the real final tangents
                    Vector4 finalTangent(fixedTangent.x, fixedTangent.y, fixedTangent.z, tangentW);
                    newTangentLayer->AddVertexValue( orgVtx, &finalTangent );
                }
            }

            mesh->AddLayer( newTangentLayer );
        }

        LogInfo("Calculating tangents: %f seconds", calculateTangentsTimer.GetTime());

        return mesh;
    }
    */


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

        MCore::JobList* jobList = MCore::JobList::Create();
        MCore::GetJobManager().GetJobPool().Lock();
        for (uint32 i = 0; i < numSubMeshes; ++i)
        {
            MCore::Job* newJob = MCore::Job::CreateWithoutLock( [this, i](const MCore::Job* job)
                    {
                        MCORE_UNUSED(job);
                        mSubMeshes[i]->GenerateVertexOrder();
                    });

            jobList->AddJob(newJob);
        }
        MCore::GetJobManager().GetJobPool().Unlock();
        MCore::ExecuteJobList(jobList);
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
