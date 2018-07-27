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
#include <MCore/Source/JobManager.h>
#include <MCore/Source/Job.h>
#include <MCore/Source/JobList.h>
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
        TransformData* transformData        = actorInstance->GetTransformData();
        const MCore::Matrix* invBindPoseMatrices    = actorInstance->GetActor()->GetInverseBindPoseGlobalMatrices().GetReadPtr();
        const MCore::Matrix* globalMatrices     = transformData->GetGlobalInclusiveMatrices();
        uint32 i;

        // precalc the skinning matrices
        MCore::Matrix invNodeTM = globalMatrices[ node->GetNodeIndex() ];
        invNodeTM.Inverse();
        const uint32 numBones = mBoneMatrices.GetLength();
        for (i = 0; i < numBones; i++)
        {
            const uint32 nodeIndex = mNodeNumbers[i];
            mBoneMatrices[i].MultMatrix4x3(invBindPoseMatrices[nodeIndex], globalMatrices[nodeIndex]);
            mBoneMatrices[i].MultMatrix4x3(invNodeTM);
        }

        // find the skinning layer
        SkinningInfoVertexAttributeLayer* layer = (SkinningInfoVertexAttributeLayer*)mMesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
        MCORE_ASSERT(layer);

        AZ::Vector3   newPos, newNormal;
        AZ::Vector3   vtxPos, normal;
        AZ::Vector4      tangent, newTangent;
        AZ::PackedVector3f* __restrict  positions   = (AZ::PackedVector3f*)mMesh->FindVertexData(Mesh::ATTRIB_POSITIONS);
        AZ::PackedVector3f* __restrict  normals     = (AZ::PackedVector3f*)mMesh->FindVertexData(Mesh::ATTRIB_NORMALS);
        AZ::Vector4*    __restrict  tangents    = (AZ::Vector4*)mMesh->FindVertexData(Mesh::ATTRIB_TANGENTS);
        uint32*  __restrict orgVerts    = (uint32*) mMesh->FindVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);

        //if (mMesh->GetNumVertices() <= 40000)
        SkinVertexRange(0, mMesh->GetNumVertices(), positions, normals, tangents, orgVerts, layer);
        /*  else
            {
                // split the vertex data into different sections, processing each section in another job
                MCore::JobList* jobList = MCore::JobList::Create();
                const uint32 numVertices = mMesh->GetNumVertices();
                MCore::GetJobManager().GetJobPool().Lock();
                uint32 curStartVertex = 0;
                uint32 curEndVertex = 0;
                while (curStartVertex < numVertices)
                {
                    curEndVertex += 20000;  // max vertices per job
                    if (curEndVertex > numVertices)
                        curEndVertex = numVertices;

                    MCore::Job* job = MCore::Job::CreateWithoutLock(    [this, curStartVertex, curEndVertex, positions, normals, tangents, orgVerts, layer](const MCore::Job* job)
                                                                        {
                                                                            MCORE_UNUSED(job);
                                                                            SkinVertexRange(curStartVertex, curEndVertex, positions, normals, tangents, orgVerts, layer);
                                                                        } );

                    curStartVertex = curEndVertex;
                    jobList->AddJob( job );
                }
                MCore::GetJobManager().GetJobPool().Unlock();
                MCore::ExecuteJobList( jobList );
            }*/

        /*
            // if there are tangents and binormals to skin
            if (tangents)
            {
                const uint32 numVertices = mMesh->GetNumVertices();
                for (uint32 v=0; v<numVertices; ++v)
                {
                    // reset the skinned position
                    newPos.Zero();
                    newNormal.Zero();
                    newTangent.Zero();

                    vtxPos  = *positions;
                    normal  = *normals;
                    tangent = *tangents;

                    const uint32 orgVertex = *(orgVerts++); // get the original vertex number
                    const uint32 numInfluences = layer->GetNumInfluences( orgVertex );
                    for (i=0; i<numInfluences; ++i)
                    {
                        const SkinInfluence* influence = layer->GetInfluence(orgVertex, i);
                        mBoneMatrices[ influence->GetBoneNr() ].Skin(&vtxPos, &normal, &tangent, &newPos, &newNormal, &newTangent, influence->GetWeight());
                    }

                    newTangent.w = tangent.w;

                    // output the skinned values
                    *positions  = newPos;       positions++;
                    *normals    = newNormal;    normals++;
                    *tangents   = newTangent;   tangents++;
                }
            }
            else    // there are no tangents and binormals to skin
            {
                const uint32 numVertices = mMesh->GetNumVertices();
                for (uint32 v=0; v<numVertices; ++v)
                {
                    newPos.Zero();
                    newNormal.Zero();

                    vtxPos  = *positions;
                    normal  = *normals;

                    // skin the vertex
                    const uint32 orgVertex = *(orgVerts++); // get the original vertex number
                    const uint32 numInfluences = layer->GetNumInfluences( orgVertex );
                    for (i=0; i<numInfluences; ++i)
                    {
                        const SkinInfluence* influence = layer->GetInfluence(orgVertex, i);
                        mBoneMatrices[ influence->GetBoneNr() ].Skin(&vtxPos, &normal, &newPos, &newNormal, influence->GetWeight());
                    }

                    // output the skinned values
                    *positions  = newPos;       positions++;
                    *normals    = newNormal;    normals++;
                }
            }*/
    }


    void SoftSkinDeformer::SkinVertexRange(uint32 startVertex, uint32 endVertex, AZ::PackedVector3f* positions, AZ::PackedVector3f* normals, AZ::Vector4* tangents, uint32* orgVerts, SkinningInfoVertexAttributeLayer* layer)
    {
        AZ::Vector3 newPos, newNormal;
        AZ::Vector3 vtxPos, normal;
        AZ::Vector4 tangent, newTangent;

        // if there are tangents and binormals to skin
        if (tangents)
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
                    mBoneMatrices[ influence->GetBoneNr() ].Skin(&vtxPos, &normal, &tangent, &newPos, &newNormal, &newTangent, influence->GetWeight());
                }

                newTangent.SetW(tangent.GetW());

                // output the skinned values
                positions[v]    = AZ::PackedVector3f(newPos);
                normals[v]      = AZ::PackedVector3f(newNormal);
                tangents[v]     = newTangent;
            }
        }
        else // there are no tangents and binormals to skin
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
                    mBoneMatrices[ influence->GetBoneNr() ].Skin(&vtxPos, &normal, &newPos, &newNormal, influence->GetWeight());
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
