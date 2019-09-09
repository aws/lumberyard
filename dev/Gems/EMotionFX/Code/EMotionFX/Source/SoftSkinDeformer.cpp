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
#include "EMotionFXConfig.h"
#include "SoftSkinDeformer.h"
#include "Mesh.h"
#include "Node.h"
#include "SubMesh.h"
#include "Actor.h"
#include "SkinningInfoVertexAttributeLayer.h"
#include "TransformData.h"
#include "ActorInstance.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SoftSkinDeformer, DeformerAllocator, 0)

    // constructor
    SoftSkinDeformer::SoftSkinDeformer(Mesh* mesh)
        : MeshDeformer(mesh)
    {
        mNodeNumbers.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_DEFORMERS);
        mBoneMatrices.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_DEFORMERS);
    }


    // destructor
    SoftSkinDeformer::~SoftSkinDeformer()
    {
        mNodeNumbers.Clear();
        mBoneMatrices.Clear();
    }


    // create
    SoftSkinDeformer* SoftSkinDeformer::Create(Mesh* mesh)
    {
        return aznew SoftSkinDeformer(mesh);
    }


    // get the type id
    uint32 SoftSkinDeformer::GetType() const
    {
        return TYPE_ID;
    }


    // get the subtype id
    uint32 SoftSkinDeformer::GetSubType() const
    {
        return SUBTYPE_ID;
    }


    // clone this class
    MeshDeformer* SoftSkinDeformer::Clone(Mesh* mesh)
    {
        // create the new cloned deformer
        SoftSkinDeformer* result = aznew SoftSkinDeformer(mesh);

        // copy the bone info (for precalc/optimization reasons)
        result->mNodeNumbers    = mNodeNumbers;
        result->mBoneMatrices   = mBoneMatrices;

        // return the result
        return result;
    }


    // the main method where all calculations are done
    void SoftSkinDeformer::Update(ActorInstance* actorInstance, Node* node, float timeDelta)
    {
        MCORE_UNUSED(timeDelta);

        // get some vars
        const TransformData* transformData = actorInstance->GetTransformData();
        const MCore::Matrix* skinningMatrices = transformData->GetSkinningMatrices();

        // precalc the skinning matrices
        const uint32 numBones = mBoneMatrices.GetLength();
        for (uint32 i = 0; i < numBones; i++)
        {
            const uint32 nodeIndex = mNodeNumbers[i];
            mBoneMatrices[i] = skinningMatrices[nodeIndex];
        }

        // find the skinning layer
        SkinningInfoVertexAttributeLayer* layer = (SkinningInfoVertexAttributeLayer*)mMesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
        AZ_Assert(layer, "Cannot find skinning info");

        // Perform the skinning.
        AZ::PackedVector3f* __restrict positions    = static_cast<AZ::PackedVector3f*>(mMesh->FindVertexData(Mesh::ATTRIB_POSITIONS));
        AZ::PackedVector3f* __restrict normals      = static_cast<AZ::PackedVector3f*>(mMesh->FindVertexData(Mesh::ATTRIB_NORMALS));
        AZ::Vector4*        __restrict tangents     = static_cast<AZ::Vector4*>(mMesh->FindVertexData(Mesh::ATTRIB_TANGENTS));
        AZ::PackedVector3f* __restrict bitangents   = static_cast<AZ::PackedVector3f*>(mMesh->FindVertexData(Mesh::ATTRIB_BITANGENTS));
        AZ::u32*            __restrict orgVerts     = static_cast<AZ::u32*>(mMesh->FindVertexData(Mesh::ATTRIB_ORGVTXNUMBERS));
        SkinVertexRange(0, mMesh->GetNumVertices(), positions, normals, tangents, bitangents, orgVerts, layer);
    }


    void SoftSkinDeformer::SkinVertexRange(uint32 startVertex, uint32 endVertex, AZ::PackedVector3f* positions, AZ::PackedVector3f* normals, AZ::Vector4* tangents, AZ::PackedVector3f* bitangents, uint32* orgVerts, SkinningInfoVertexAttributeLayer* layer)
    {
        AZ::Vector3 newPos, newNormal;
        AZ::Vector3 vtxPos, normal;
        AZ::Vector4 tangent, newTangent;
        AZ::Vector3 bitangent, newBitangent;

        // if there are tangents and bitangents to skin
        if (tangents && bitangents)
        {
            for (uint32 v = startVertex; v < endVertex; ++v)
            {
                newPos = AZ::Vector3::CreateZero();
                newNormal = AZ::Vector3::CreateZero();
                newTangent = AZ::Vector4::CreateZero();
                newBitangent = AZ::Vector3::CreateZero();

                vtxPos  = AZ::Vector3(positions[v]);
                normal  = AZ::Vector3(normals[v]);
                tangent = tangents[v];
                bitangent = AZ::Vector3(bitangents[v]);

                const uint32 orgVertex = orgVerts[v]; // get the original vertex number
                const uint32 numInfluences = layer->GetNumInfluences(orgVertex);
                for (uint32 i = 0; i < numInfluences; ++i)
                {
                    const SkinInfluence* influence = layer->GetInfluence(orgVertex, i);
                    mBoneMatrices[influence->GetBoneNr()].Skin(&vtxPos, &normal, &tangent, &bitangent, &newPos, &newNormal, &newTangent, &newBitangent, influence->GetWeight());
                }

                newTangent.SetW(tangents[v].GetW());

                // output the skinned values
                positions[v]    = AZ::PackedVector3f(newPos);
                normals[v]      = AZ::PackedVector3f(newNormal);
                tangents[v]     = newTangent;
                bitangents[v]   = AZ::PackedVector3f(newBitangent);
            }
        }
        else if (tangents) // only tangents but no bitangents
        {
            for (uint32 v = startVertex; v < endVertex; ++v)
            {
                newPos = AZ::Vector3::CreateZero();
                newNormal = AZ::Vector3::CreateZero();
                newTangent = AZ::Vector4::CreateZero();

                vtxPos  = AZ::Vector3(positions[v]);
                normal  = AZ::Vector3(normals[v]);
                tangent = tangents[v];

                const uint32 orgVertex = orgVerts[v]; // get the original vertex number
                const uint32 numInfluences = layer->GetNumInfluences(orgVertex);
                for (uint32 i = 0; i < numInfluences; ++i)
                {
                    const SkinInfluence* influence = layer->GetInfluence(orgVertex, i);
                    mBoneMatrices[influence->GetBoneNr()].Skin(&vtxPos, &normal, &tangent, &newPos, &newNormal, &newTangent, influence->GetWeight());
                }

                newTangent.SetW(tangents[v].GetW());

                // output the skinned values
                positions[v]    = AZ::PackedVector3f(newPos);
                normals[v]      = AZ::PackedVector3f(newNormal);
                tangents[v]     = newTangent;
            }
        }
        else // there are no tangents and bitangents to skin
        {
            for (uint32 v = startVertex; v < endVertex; ++v)
            {
                newPos = AZ::Vector3::CreateZero();
                newNormal = AZ::Vector3::CreateZero();

                vtxPos  = AZ::Vector3(positions[v]);
                normal  = AZ::Vector3(normals[v]);

                // skin the vertex
                const uint32 orgVertex = orgVerts[v]; // get the original vertex number
                const uint32 numInfluences = layer->GetNumInfluences(orgVertex);
                for (uint32 i = 0; i < numInfluences; ++i)
                {
                    const SkinInfluence* influence = layer->GetInfluence(orgVertex, i);
                    mBoneMatrices[influence->GetBoneNr()].Skin(&vtxPos, &normal, &newPos, &newNormal, influence->GetWeight());
                }

                // output the skinned values
                positions[v]    = AZ::PackedVector3f(newPos);
                normals[v]      = AZ::PackedVector3f(newNormal);
            }
        }
    }


    // initialize the mesh deformer
    void SoftSkinDeformer::Reinitialize(Actor* actor, Node* node, uint32 lodLevel)
    {
        MCORE_UNUSED(actor);
        MCORE_UNUSED(node);
        MCORE_UNUSED(lodLevel);

        // clear the bone information array, but don't free the currently allocated/reserved memory
        mBoneMatrices.Clear(false);
        mNodeNumbers.Clear(false);

        // if there is no mesh
        if (mMesh == nullptr)
        {
            return;
        }

        // get the attribute number
        SkinningInfoVertexAttributeLayer* skinningLayer = (SkinningInfoVertexAttributeLayer*)mMesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
        MCORE_ASSERT(skinningLayer);

        // reserve space for the bone array
        //mBones.Reserve( actor->GetNumNodes() );

        // find out what bones this mesh uses
        const uint32 numOrgVerts = mMesh->GetNumOrgVertices();
        for (uint32 i = 0; i < numOrgVerts; i++)
        {
            // now we have located the skinning information for this vertex, we can see if our bones array
            // already contains the bone it uses by traversing all influences for this vertex, and checking
            // if the bone of that influence already is in the array with used bones
            MCore::Matrix mat;
            mat.Identity();
            const uint32 numInfluences = skinningLayer->GetNumInfluences(i);
            for (uint32 a = 0; a < numInfluences; ++a)
            {
                SkinInfluence* influence = skinningLayer->GetInfluence(i, a);

                // get the bone index in the array
                uint32 boneIndex = FindLocalBoneIndex(influence->GetNodeNr());

                // if the bone is not found in our array
                if (boneIndex == MCORE_INVALIDINDEX32)
                {
                    // add the bone to the array of bones in this deformer
                    mNodeNumbers.Add(influence->GetNodeNr());
                    mBoneMatrices.Add(mat);
                    boneIndex = mBoneMatrices.GetLength() - 1;
                }

                // set the bone number in the influence
                influence->SetBoneNr(static_cast<uint16>(boneIndex));
                //MCore::LogInfo("influence %d/%d = %s with weight %f [nodeIndex=%d] [boneIndex=%d]", a+1, numInfluences, actor->GetNode(influence->GetNodeNr())->GetName(), influence->GetWeight(), influence->GetNodeNr(), boneIndex);
            }
        }
        // get rid of all items in the used bones array
        //  mBones.Shrink();
    }
} // namespace EMotionFX
