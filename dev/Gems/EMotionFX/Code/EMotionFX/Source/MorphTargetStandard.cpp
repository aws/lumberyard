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
#include "Node.h"
#include "Actor.h"
#include "MorphTargetStandard.h"
#include "Mesh.h"
#include "ActorInstance.h"
#include "MorphMeshDeformer.h"
#include "MeshDeformerStack.h"
#include "TransformData.h"
#include <MCore/Source/Compare.h>
#include <MCore/Source/Vector.h>
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MorphTargetStandard, DeformerAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(MorphTargetStandard::DeformData, DeformerAllocator, 0)


    // basic constructor
    MorphTargetStandard::MorphTargetStandard(const char* name)
        : MorphTarget(name)
    {
        mDeformDatas.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS);
    }


    // extended constructor
    MorphTargetStandard::MorphTargetStandard(bool captureTransforms, bool captureMeshDeforms, Actor* neutralPose, Actor* targetPose, const char* name, bool delPoseFromMem)
        : MorphTarget(name)
    {
        InitFromPose(captureTransforms, captureMeshDeforms, neutralPose, targetPose, delPoseFromMem);
    }


    // destructor
    MorphTargetStandard::~MorphTargetStandard()
    {
        RemoveAllDeformDatas();
    }


    // create
    MorphTargetStandard* MorphTargetStandard::Create(const char* name)
    {
        return aznew MorphTargetStandard(name);
    }


    // extended create
    MorphTargetStandard* MorphTargetStandard::Create(bool captureTransforms, bool captureMeshDeforms, Actor* neutralPose, Actor* targetPose, const char* name, bool delPoseFromMem)
    {
        return aznew MorphTargetStandard(captureTransforms, captureMeshDeforms, neutralPose, targetPose, name, delPoseFromMem);
    }


    // get the expressionpart type
    uint32 MorphTargetStandard::GetType() const
    {
        return TYPE_ID;
    }



    void MorphTargetStandard::BuildWorkMesh(Mesh* mesh, MCore::Array< MCore::Array<uint32> >& output)
    {
        // resize the array
        const uint32 numOrgVerts = mesh->GetNumOrgVertices();
        output.Resize(numOrgVerts);

        // get the original vertex numbers
        const uint32* orgVerts = (uint32*)mesh->FindOriginalVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);

        // iterate over all vertices and add them to the worker mesh
        const uint32 numVerts = mesh->GetNumVertices();
        for (uint32 v = 0; v < numVerts; ++v)
        {
            const uint32 orgVertex = orgVerts[v];
            output[orgVertex].Add(v);
        }
    }


    // initialize the morph target from a given actor pose
    void MorphTargetStandard::InitFromPose(bool captureTransforms, bool captureMeshDeforms, Actor* neutralPose, Actor* targetPose, bool delPoseFromMem)
    {
        MCORE_ASSERT(neutralPose);
        MCORE_ASSERT(targetPose);

        // filter all changed meshes
        if (captureMeshDeforms)
        {
            Skeleton* targetSkeleton    = targetPose->GetSkeleton();
            Skeleton* neutralSkeleton   = neutralPose->GetSkeleton();
            const uint32 numPoseNodes = targetSkeleton->GetNumNodes();
            for (uint32 i = 0; i < numPoseNodes; ++i)
            {
                // find if the pose node also exists in the actor where we apply this to
                Node* targetNode  = targetSkeleton->GetNode(i);
                //this code is assuming no duplicate node names.
                Node* neutralNode = neutralSkeleton->FindNodeByID(targetNode->GetID());

                // skip this node if it doesn't exist in the actor we apply this to
                if (neutralNode == nullptr)
                {
                    continue;
                }

                // get the meshes
                Mesh* neutralMesh = neutralPose->GetMesh(0, neutralNode->GetNodeIndex());
                Mesh* targetMesh  = targetPose->GetMesh(0, targetNode->GetNodeIndex());

                // if one of the nodes has no mesh, we can skip this node
                if (neutralMesh == nullptr || targetMesh == nullptr)
                {
                    continue;
                }

                // both nodes have a mesh, lets check if they have the same number of vertices as well
                const uint32 numNeutralVerts = neutralMesh->GetNumVertices();
                const uint32 numTargetVerts  = targetMesh->GetNumVertices();

                // if the number of original vertices is not equal to the number of current vertices, we can't use this mesh
                if (neutralMesh->GetNumOrgVertices() != targetMesh->GetNumOrgVertices())
                {
                    MCore::LogWarning("EMotionFX::MorphTargetStandard::InitFromPose() - Number of original vertices from base mesh (%d org verts) differs from morph target num org vertices (%d verts)", neutralMesh->GetNumOrgVertices(), targetMesh->GetNumOrgVertices());
                    continue;
                }

                if (numNeutralVerts != numTargetVerts)
                {
                    MCore::LogInfo("EMotionFX::MorphTargetStandard::InitFromPose() - Number of vertices of base mesh (%d verts) is different from the number of morph target vertices (%d verts). This can lead to some visual shading errors.", numNeutralVerts, numTargetVerts);
                }


                // check if the mesh has differences
                uint32 numDifferent = 0;
                const AZ::PackedVector3f*   neutralPositions = (AZ::PackedVector3f*)neutralMesh->FindOriginalVertexData(Mesh::ATTRIB_POSITIONS);
                const AZ::PackedVector3f*   neutralNormals   = (AZ::PackedVector3f*)neutralMesh->FindOriginalVertexData(Mesh::ATTRIB_NORMALS);
                const uint32*               neutralOrgVerts  = (uint32*)neutralMesh->FindOriginalVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);
                const AZ::Vector4*          neutralTangents  = static_cast<AZ::Vector4*>(neutralMesh->FindOriginalVertexData(Mesh::ATTRIB_TANGENTS));
                const AZ::PackedVector3f*   targetPositions  = (AZ::PackedVector3f*)targetMesh->FindOriginalVertexData(Mesh::ATTRIB_POSITIONS);
                const AZ::PackedVector3f*   targetNormals    = (AZ::PackedVector3f*)targetMesh->FindOriginalVertexData(Mesh::ATTRIB_NORMALS);
                const AZ::Vector4*          targetTangents   = (AZ::Vector4*)targetMesh->FindOriginalVertexData(Mesh::ATTRIB_TANGENTS);

                //--------------------------------------------------

                // first calculate the neutral and target bounds
                MCore::AABB neutralAABB;
                MCore::AABB targetAABB;
                MCore::Matrix identity;
                identity.Identity();
                neutralMesh->CalcAABB(&neutralAABB, identity, 1);
                targetMesh->CalcAABB(&targetAABB, identity, 1);
                neutralAABB.Encapsulate(targetAABB); // join the two boxes, so that we have the box around those two boxes

                // now get the extent length
                float epsilon = neutralAABB.CalcRadius() * 0.0025f; // TODO: tweak this parameter
                if (epsilon < MCore::Math::epsilon)
                {
                    epsilon = MCore::Math::epsilon;
                }

                //------------------------------------------------
                // build the worker meshes
                MCore::Array< MCore::Array<uint32> > neutralVerts;
                MCore::Array< MCore::Array<uint32> > morphVerts;
                BuildWorkMesh(neutralMesh, neutralVerts);
                BuildWorkMesh(targetMesh, morphVerts);

                // calculate the bounding box
                MCore::AABB box;
                uint32 v;
                for (v = 0; v < numNeutralVerts; ++v)
                {
                    const uint32 orgVertex = neutralOrgVerts[v];

                    // calculate the delta vector between the two positions
                    const AZ::Vector3 deltaVec = AZ::Vector3(targetPositions[ morphVerts[orgVertex][0] ]) - AZ::Vector3(neutralPositions[v]);

                    // check if the vertex positions are different, so if there are mesh changes
                    if (MCore::SafeLength(deltaVec) > /*0.0001f*/ epsilon)
                    {
                        box.Encapsulate(deltaVec);
                        numDifferent++;
                    }
                }
                //------------------------------------------------

                // go to the next node and mesh if there is no difference
                if (numDifferent == 0)
                {
                    continue;
                }

                // calculate the minimum and maximum value
                const AZ::Vector3& boxMin = box.GetMin();
                const AZ::Vector3& boxMax = box.GetMax();
                float minValue = MCore::Min3<float>(boxMin.GetX(), boxMin.GetY(), boxMin.GetZ());
                float maxValue = MCore::Max3<float>(boxMax.GetX(), boxMax.GetY(), boxMax.GetZ());

                // make sure the values won't be too small
                if (maxValue - minValue < 1.0f)
                {
                    minValue -= 0.5f;
                    maxValue += 0.5f;
                }

                // check if we have tangents
                const bool hasTangents = ((neutralTangents) && (targetTangents));

                // create the deformation data
                DeformData* deformData = DeformData::Create(neutralNode->GetNodeIndex(), numDifferent);
                deformData->mMinValue = minValue;
                deformData->mMaxValue = maxValue;

                // set all deltas for all vertices
                uint32 curVertex = 0;
                for (v = 0; v < numNeutralVerts; ++v)
                {
                    const uint32 orgVertex = neutralOrgVerts[v];

                    // calculate the delta vector between the two positions
                    const AZ::Vector3 deltaVec = AZ::Vector3(targetPositions[ morphVerts[orgVertex][0] ]) - AZ::Vector3(neutralPositions[v]);

                    // check if the vertex positions are different, so if there are mesh changes
                    if (MCore::SafeLength(deltaVec) > /*0.0001f*/ epsilon)
                    {
                        uint32 duplicateIndex = 0;
                        uint32 targetIndex = 0;

                        // if the number of duplicates is equal
                        if (neutralVerts[orgVertex].GetLength() == morphVerts[orgVertex].GetLength())
                        {
                            duplicateIndex  = neutralVerts[orgVertex].Find(v);
                        }

                        // get the target vertex index
                        targetIndex = morphVerts[orgVertex][duplicateIndex];

                        // calculate the delta normal tangent
                        AZ::Vector3 deltaNormal  = AZ::Vector3(targetNormals[targetIndex]) - AZ::Vector3(neutralNormals[v]);
                        AZ::Vector3 deltaTangent(0.0f, 0.0f, 0.0f);
                        if (hasTangents)
                        {
                            deltaTangent.SetX(targetTangents[targetIndex].GetX() - neutralTangents[v].GetX());
                            deltaTangent.SetY(targetTangents[targetIndex].GetY() - neutralTangents[v].GetY());
                            deltaTangent.SetZ(targetTangents[targetIndex].GetZ() - neutralTangents[v].GetZ());
                        }

                        // setup the deform data for this vertex
                        deformData->mDeltas[curVertex].mVertexNr = v;
                        deformData->mDeltas[curVertex].mPosition.FromVector3(deltaVec, minValue, maxValue);
                        deformData->mDeltas[curVertex].mNormal.FromVector3(deltaNormal, -2.0f, 2.0f);
                        deformData->mDeltas[curVertex].mTangent.FromVector3(deltaTangent, -1.0f, 1.0f);
                        curVertex++;
                    } // deltaVec.Length() > 0.0001f
                }

                //LogInfo("Mesh deform data for node '%s' contains %d vertices", neutralNode->GetName(), numDifferent);

                // add the deform data
                mDeformDatas.Add(deformData);

                //-------------------------------
                // create the mesh deformer
                MeshDeformerStack* stack = neutralPose->GetMeshDeformerStack(0, neutralNode->GetNodeIndex());

                // create the stack if it doesn't yet exist
                if (stack == nullptr)
                {
                    stack = MeshDeformerStack::Create(neutralMesh);
                    neutralPose->SetMeshDeformerStack(0, neutralNode->GetNodeIndex(), stack);
                }

                // try to see if there is already some  morph deformer
                MorphMeshDeformer* deformer = nullptr;
                MeshDeformerStack* stackPtr = stack;
                deformer = (MorphMeshDeformer*)stackPtr->FindDeformerByType(MorphMeshDeformer::TYPE_ID);
                if (deformer == nullptr) // there isn't one, so create and add one
                {
                    deformer = MorphMeshDeformer::Create(neutralMesh);
                    stack->InsertDeformer(0, deformer);
                }

                // add the deform pass to the mesh deformer
                MorphMeshDeformer::DeformPass deformPass;
                deformPass.mDeformDataNr = mDeformDatas.GetLength() - 1;
                deformPass.mMorphTarget  = this;
                deformer->AddDeformPass(deformPass);

                //-------------------------------

                // make sure we end up with the same number of different vertices, otherwise the two loops
                // have different detection on if a vertex has changed or not
                MCORE_ASSERT(curVertex == numDifferent);
            }
        }

        //--------------------------------------------------------------------------------------------------

        if (captureTransforms)
        {
            // get a bone list, if we also captured meshes, because in that case we want
            // to disable capturing transforms for the bones since the deforms already have been captured
            // by the mesh capture
            //Array<Node*> boneList;
            //if (mCaptureMeshDeforms)
            //pose->ExtractBoneList(0, &boneList);

            Skeleton* targetSkeleton    = targetPose->GetSkeleton();
            Skeleton* neutralSkeleton   = neutralPose->GetSkeleton();

            const Pose& neutralBindPose = *neutralPose->GetBindPose();
            const Pose& targetBindPose  = *targetPose->GetBindPose();

            //      Transform*  neutralData = neutralPose->GetBindPoseLocalTransforms();
            //      Transform*  targetData  = targetPose->GetBindPoseLocalTransforms();

            // check for transformation changes
            const uint32 numPoseNodes = targetSkeleton->GetNumNodes();
            for (uint32 i = 0; i < numPoseNodes; ++i)
            {
                // get a node id (both nodes will have the same id since they represent their names)
                const uint32 nodeID = targetSkeleton->GetNode(i)->GetID();

                // try to find the node with the same name inside the neutral pose actor
                Node* neutralNode = neutralSkeleton->FindNodeByID(nodeID);
                if (neutralNode == nullptr)
                {
                    continue;
                }

                // get the node indices of both nodes
                const uint32 neutralNodeIndex = neutralNode->GetNodeIndex();
                const uint32 targetNodeIndex  = targetSkeleton->GetNode(i)->GetNodeIndex();

                // skip bones in the bone list
                //if (mCaptureMeshDeforms)
                //if (boneList.Contains( nodeA ))
                //continue;

                const Transform& neutralTransform   = neutralBindPose.GetLocalTransform(neutralNodeIndex);
                const Transform& targetTransform    = targetBindPose.GetLocalTransform(targetNodeIndex);

                AZ::Vector3      neutralPos         = neutralTransform.mPosition;
                AZ::Vector3      targetPos          = targetTransform.mPosition;
                MCore::Quaternion   neutralRot      = neutralTransform.mRotation;
                MCore::Quaternion   targetRot       = targetTransform.mRotation;

                EMFX_SCALECODE
                (
                    AZ::Vector3      neutralScale    = neutralTransform.mScale;
                    AZ::Vector3      targetScale     = targetTransform.mScale;
                    //MCore::Quaternion neutralScaleRot = neutralTransform.mScaleRotation;
                    //MCore::Quaternion targetScaleRot  = targetTransform.mScaleRotation;
                )

                // check if the position changed
                bool changed = (MCore::Compare<AZ::Vector3>::CheckIfIsClose(neutralPos, targetPos, MCore::Math::epsilon) == false);

                // check if the rotation changed
                if (changed == false)
                {
                    changed = (MCore::Compare<MCore::Quaternion>::CheckIfIsClose(neutralRot, targetRot, MCore::Math::epsilon) == false);
                }

                EMFX_SCALECODE
                (
                    // check if the scale changed
                    if (changed == false)
                    {
                        changed = (MCore::Compare<AZ::Vector3>::CheckIfIsClose(neutralScale, targetScale, MCore::Math::epsilon) == false);
                    }

                    // check if the scale rotation changed
                    //              if (changed == false)
                    //              changed = (MCore::Compare<MCore::Quaternion>::CheckIfIsClose(neutralScaleRot, targetScaleRot, MCore::Math::epsilon) == false);
                )

                // if this node changed transformation
                if (changed)
                {
                    // create a transform object form the node in the pose
                    Transformation transform;
                    transform.mPosition     = targetPos - neutralPos;
                    transform.mRotation     = targetRot;

                    EMFX_SCALECODE
                    (
                        //transform.mScaleRotation= targetScaleRot;
                        transform.mScale    = targetScale - neutralScale;
                    )

                    transform.mNodeIndex    = neutralNodeIndex;

                    // add the new transform
                    AddTransformation(transform);
                }
            }

            //LogInfo("Num transforms = %d", mTransforms.GetLength());
        }

        // delete the target pose actor when wanted
        if (delPoseFromMem)
        {
            targetPose->Destroy();
        }
    }

    /*
    // initialize the morph target from a given actor pose
    void MorphTargetStandard::InitFromPose(const bool captureTransforms, const bool captureMeshDeforms, Actor* neutralPose, Actor* targetPose, bool delPoseFromMem)
    {
        MCORE_ASSERT(neutralPose);
        MCORE_ASSERT(targetPose);

        // filter all changed meshes
        if (captureMeshDeforms)
        {
            const uint32 numPoseNodes = targetPose->GetNumNodes();
            for (uint32 i=0; i<numPoseNodes; ++i)
            {
                // find if the pose node also exists in the actor where we apply this to
                Node* targetNode  = targetPose->GetNode(i);
                Node* neutralNode = neutralPose->FindNodeByID( targetNode->GetID() );

                // skip this node if it doesn't exist in the actor we apply this to
                if (neutralNode == nullptr)
                    continue;

                // get the meshes
                Mesh* neutralMesh = neutralNode->GetMesh(0).GetPointer();
                Mesh* targetMesh  = targetNode->GetMesh(0).GetPointer();

                // if one of the nodes has no mesh, we can skip this node
                if (neutralMesh==nullptr || targetMesh==nullptr)
                    continue;

                // both nodes have a mesh, lets check if they have the same number of vertices as well
                const uint32 numNeutralVerts = neutralMesh->GetNumVertices();
                const uint32 numTargetVerts  = targetMesh->GetNumVertices();

                // if the number of original vertices is not equal to the number of current vertices, we can't use this mesh
                if (numNeutralVerts != numTargetVerts)
                {
                    //LogInfo("**** not equal number of vertices (neutral=%d    morphtarget=%d)", numNeutralVerts, numTargetVerts);
                    //LogInfo("orgN=%d   orgT=%d", neutralMesh->GetNumOrgVertices(), targetMesh->GetNumOrgVertices());
                    continue;
                }

                // check if the mesh has differences
                uint32 numDifferent = 0;
                const Vector3* neutralPositions = (Vector3*)neutralMesh->FindOriginalVertexData( Mesh::ATTRIB_POSITIONS );
                const Vector3* neutralNormals   = (Vector3*)neutralMesh->FindOriginalVertexData( Mesh::ATTRIB_NORMALS );
                const Vector3* neutralTangents  = (Vector3*)neutralMesh->FindOriginalVertexData( Mesh::ATTRIB_TANGENTS );
                const Vector3* targetPositions  = (Vector3*)targetMesh->FindOriginalVertexData( Mesh::ATTRIB_POSITIONS );
                const Vector3* targetNormals    = (Vector3*)targetMesh->FindOriginalVertexData( Mesh::ATTRIB_NORMALS );
                const Vector3* targetTangents   = (Vector3*)targetMesh->FindOriginalVertexData( Mesh::ATTRIB_TANGENTS );

                AABB box;
                uint32 v;
                for (v=0; v<numNeutralVerts; ++v)
                {
                    // calculate the delta vector between the two positions
                    const Vector3 deltaVec = targetPositions[v] - neutralPositions[v];

                    // check if the vertex positions are different, so if there are mesh changes
                    if (deltaVec.Length() > 0.0001f)
                    {
                        box.Encapsulate( deltaVec );
                        numDifferent++;
                    }
                }

                // go to the next node and mesh if there is no difference
                if (numDifferent == 0)
                    continue;

                // calculate the minimum value of the box
                float minValue = box.GetMin().x;
                minValue = MCore::Min<float>(minValue, box.GetMin().y);
                minValue = MCore::Min<float>(minValue, box.GetMin().z);

                // calculate the minimum value of the box
                float maxValue = box.GetMax().x;
                maxValue = MCore::Max<float>(maxValue, box.GetMax().y);
                maxValue = MCore::Max<float>(maxValue, box.GetMax().z);

                // make sure the values won't be too small
                if (maxValue - minValue < 1.0f)
                {
                    if (minValue < 0.0f && minValue > -1.0f)
                        minValue = -1.0f;

                    if (maxValue > 0.0f && maxValue < 1.0f)
                        maxValue = 1.0f;
                }

                const bool hasTangents = ((neutralTangents!=nullptr) && (targetTangents));

                // create the deformation data
                DeformData* deformData = new DeformData(neutralNode->GetNodeIndex(), numDifferent);
                deformData->mMinValue = minValue;
                deformData->mMaxValue = maxValue;

                uint32 curVertex = 0;
                for (v=0; v<numNeutralVerts; ++v)
                {
                    // calculate the delta vector between the two positions
                    Vector3 deltaVec = targetPositions[v] - neutralPositions[v];

                    // check if the vertex positions are different, so if there are mesh changes
                    if (deltaVec.Length() > 0.0001f)
                    {
                        Vector3 deltaNormal  = targetNormals[v] - neutralNormals[v];
                        Vector3 deltaTangent(0.0f, 0.0f, 0.0f);

                        if (hasTangents)
                            deltaTangent = targetTangents[v] - neutralTangents[v];

                        deformData->mDeltas[curVertex].mVertexNr = v;
                        deformData->mDeltas[curVertex].mPosition.FromVector3(deltaVec, minValue, maxValue);
                        deformData->mDeltas[curVertex].mNormal.FromVector3(deltaNormal, -1.0f, 1.0f);
                        deformData->mDeltas[curVertex].mTangent.FromVector3(deltaTangent, -1.0f, 1.0f);

                        curVertex++;
                    }
                }

                //LogInfo("Mesh deform data for node '%s' contains %d vertices", neutralNode->GetName(), numDifferent);

                // add the deform data
                mDeformDatas.Add( deformData );

                //-------------------------------
                // create the mesh deformer
                MCore::Pointer<MeshDeformerStack>& stack = neutralNode->GetMeshDeformerStack(0);

                // create the stack if it doesn't yet exist
                if (stack.GetPointer() == nullptr)
                {
                    stack = MCore::Pointer<MeshDeformerStack>( new MeshDeformerStack(neutralMesh) );
                    neutralNode->SetMeshDeformerStack(stack, 0);
                }

                // try to see if there is already some  morph deformer
                MorphMeshDeformer* deformer = nullptr;
                MeshDeformerStack* stackPtr = stack.GetPointer();
                deformer = (MorphMeshDeformer*)stackPtr->FindDeformerByType( MorphMeshDeformer::TYPE_ID );
                if (deformer == nullptr)    // there isn't one, so create and add one
                {
                    deformer = new MorphMeshDeformer( neutralMesh );
                    stack->InsertDeformer(0, deformer);
                }

                // add the deform pass to the mesh deformer
                MorphMeshDeformer::DeformPass deformPass;
                deformPass.mDeformDataNr = mDeformDatas.GetLength() - 1;
                deformPass.mMorphTarget  = this;
                deformer->AddDeformPass( deformPass );

                //-------------------------------

                // make sure we end up with the same number of different vertices, otherwise the two loops
                // have different detection on if a vertex has changed or not
                MCORE_ASSERT(curVertex == numDifferent);
            }
        }

        //--------------------------------------------------------------------------------------------------

        if (captureTransforms)
        {
            // get a bone list, if we also captured meshes, because in that case we want
            // to disable capturing transforms for the bones since the deforms already have been captured
            // by the mesh capture
            //Array<Node*> boneList;
            //if (mCaptureMeshDeforms)
                //pose->ExtractBoneList(0, &boneList);

            TransformData* neutralData = neutralPose->GetTransformData();
            TransformData* targetData  = targetPose->GetTransformData();

            // check for transformation changes
            const uint32 numPoseNodes = targetPose->GetNumNodes();
            for (uint32 i=0; i<numPoseNodes; ++i)
            {
                // get a node id (both nodes will have the same id since they represent their names)
                const uint32 nodeID = targetPose->GetNode(i)->GetID();

                // try to find the node with the same name inside the neutral pose actor
                Node* neutralNode = neutralPose->FindNodeByID( nodeID );
                if (neutralNode == nullptr)
                    continue;

                // get the node indices of both nodes
                const uint32 neutralNodeIndex = neutralNode->GetNodeIndex();
                const uint32 targetNodeIndex  = targetPose->GetNode(i)->GetNodeIndex();

                // skip bones in the bone list
                //if (mCaptureMeshDeforms)
                    //if (boneList.Contains( nodeA ))
                        //continue;

                MCore::Vector3      neutralPos      = neutralData->GetOrgPos( neutralNodeIndex );
                MCore::Vector3      targetPos       = targetData->GetOrgPos ( targetNodeIndex );
                MCore::Quaternion   neutralRot      = neutralData->GetOrgRot( neutralNodeIndex);
                MCore::Quaternion   targetRot       = targetData->GetOrgRot ( targetNodeIndex);

                EMFX_SCALECODE
                (
                    MCore::Vector3      neutralScale    = neutralData->GetOrgScale( neutralNodeIndex );
                    MCore::Vector3      targetScale     = targetData->GetOrgScale ( targetNodeIndex );
                    MCore::Quaternion   neutralScaleRot = neutralData->GetOrgScaleRot( neutralNodeIndex);
                    MCore::Quaternion   targetScaleRot  = targetData->GetOrgScaleRot ( targetNodeIndex);
                )

                // check if the position changed
                bool changed = (Compare<Vector3>::CheckIfIsClose(neutralPos, targetPos, 0.0001f) == false);

                // check if the rotation changed
                if (changed == false)
                    changed = (Compare<MCore::Quaternion>::CheckIfIsClose(neutralRot, targetRot, 0.0001f) == false);

                EMFX_SCALECODE
                (
                    // check if the scale changed
                    if (changed == false)
                        changed = (Compare<Vector3>::CheckIfIsClose(neutralScale, targetScale, 0.0001f) == false);

                    // check if the scale rotation changed
                    if (changed == false)
                        changed = (Compare<MCore::Quaternion>::CheckIfIsClose(neutralScaleRot, targetScaleRot, 0.0001f) == false);
                )

                // if this node changed transformation
                if (changed)
                {
                    // create a transform object form the node in the pose
                    Transformation transform;
                    transform.mPosition     = targetPos - neutralPos;
                    transform.mRotation     = targetRot;

                    EMFX_SCALECODE
                    (
                        transform.mScaleRotation= targetScaleRot;
                        transform.mScale        = targetScale - neutralScale;
                    )

                    transform.mNodeIndex    = neutralNodeIndex;

                    // add the new transform
                    AddTransformation( transform );
                }
            }

            //LogInfo("Num transforms = %d", mTransforms.GetLength());
        }

        // delete the target pose actor when wanted
        if (delPoseFromMem)
            delete targetPose;
    }
    */

    // apply the relative transformation to the specified node
    // store the result in the position, rotation and scale parameters
    void MorphTargetStandard::ApplyTransformation(ActorInstance* actorInstance, uint32 nodeIndex, AZ::Vector3& position, MCore::Quaternion& rotation, AZ::Vector3& scale, float weight)
    {
        // calculate the normalized weight (in range of 0..1)
        const float newWeight = MCore::Clamp<float>(weight, mRangeMin, mRangeMax); // make sure its within the range
        const float normalizedWeight = CalcNormalizedWeight(newWeight); // convert in range of 0..1

        // calculate the new transformations for all nodes of this morph target
        const uint32 numTransforms = mTransforms.GetLength();
        for (uint32 i = 0; i < numTransforms; ++i)
        {
            // if this is the node that gets modified by this transform
            if (mTransforms[i].mNodeIndex != nodeIndex)
            {
                continue;
            }

            // calc new position (delta based targetTransform)
            position += mTransforms[i].mPosition * newWeight;

            // calc the new scale
            scale += mTransforms[i].mScale * newWeight;

            // rotate additively
            MCore::Quaternion orgRot = actorInstance->GetTransformData()->GetBindPose()->GetLocalTransform(nodeIndex).mRotation;
            MCore::Quaternion a = orgRot;
            MCore::Quaternion b = mTransforms[i].mRotation;
            MCore::Quaternion rot = a.Lerp(b, normalizedWeight);
            rot.Normalize();

            MCore::Quaternion invRot = orgRot.Inversed(); // inversed neutral rotation
            rotation = rotation * (invRot * rot);
            rotation.Normalize();

            // all remaining nodes in the transform won't modify this current node
            break;
        }
    }


    // check if this morph target influences the specified node or not
    bool MorphTargetStandard::Influences(uint32 nodeIndex) const
    {
        uint32 i;

        // check if there is a deform data object, which works on the specified node
        const uint32 numDatas = mDeformDatas.GetLength();
        for (i = 0; i < numDatas; ++i)
        {
            if (mDeformDatas[i]->mNodeIndex == nodeIndex)
            {
                return true;
            }
        }

        // check all transforms
        const uint32 numTransforms = mTransforms.GetLength();
        for (i = 0; i < numTransforms; ++i)
        {
            if (mTransforms[i].mNodeIndex == nodeIndex)
            {
                return true;
            }
        }

        // this morph target doesn't influence the given node
        return false;
    }


    // apply the deformations to a given actor
    void MorphTargetStandard::Apply(ActorInstance* actorInstance, float weight)
    {
        // calculate the normalized weight (in range of 0..1)
        const float newWeight = MCore::Clamp<float>(weight, mRangeMin, mRangeMax); // make sure its within the range
        const float normalizedWeight = CalcNormalizedWeight(newWeight); // convert in range of 0..1

        TransformData* transformData = actorInstance->GetTransformData();
        Transform newTransform;

        // calculate the new transformations for all nodes of this morph target
        const uint32 numTransforms = mTransforms.GetLength();
        for (uint32 i = 0; i < numTransforms; ++i)
        {
            // try to find the node
            const uint32 nodeIndex = mTransforms[i].mNodeIndex;

            // init the transform data
            newTransform = transformData->GetCurrentPose()->GetLocalTransform(nodeIndex);

            // calc new position and scale (delta based targetTransform)
            newTransform.mPosition += mTransforms[i].mPosition * newWeight;

            EMFX_SCALECODE
            (
                newTransform.mScale    += mTransforms[i].mScale * newWeight;
                //          newTransform.mScaleRotation.Identity();
            )

            // rotate additively
            MCore::Quaternion orgRot = transformData->GetBindPose()->GetLocalTransform(nodeIndex).mRotation;
            MCore::Quaternion a = orgRot;
            MCore::Quaternion b = mTransforms[i].mRotation;
            MCore::Quaternion rot = a.Lerp(b, normalizedWeight);
            rot.Normalize();
            newTransform.mRotation = newTransform.mRotation * (orgRot.Inversed() * rot);
            newTransform.mRotation.Normalize();
            /*
                    // scale rotate additively
                    orgRot = actorInstance->GetTransformData()->GetOrgScaleRot( nodeIndex );
                    a = orgRot;
                    b = mTransforms[i].mScaleRotation;
                    rot = a.Lerp(b, normalizedWeight);
                    rot.Normalize();
                    newTransform.mScaleRotation = newTransform.mScaleRotation * (orgRot.Inversed() * rot);
                    newTransform.mScaleRotation.Normalize();
            */
            // set the new transformation
            transformData->GetCurrentPose()->SetLocalTransform(nodeIndex, newTransform);
        }
    }


    uint32 MorphTargetStandard::GetNumDeformDatas() const
    {
        return mDeformDatas.GetLength();
    }


    MorphTargetStandard::DeformData* MorphTargetStandard::GetDeformData(uint32 nr) const
    {
        return mDeformDatas[nr];
    }


    void MorphTargetStandard::AddDeformData(DeformData* data)
    {
        mDeformDatas.Add(data);
    }


    // add transformation to the morph target
    void MorphTargetStandard::AddTransformation(const Transformation& transform)
    {
        mTransforms.Add(transform);
    }


    // get the number of transformations in this morph target
    uint32 MorphTargetStandard::GetNumTransformations() const
    {
        return mTransforms.GetLength();
    }


    // get the given transformation
    MorphTargetStandard::Transformation& MorphTargetStandard::GetTransformation(uint32 nr)
    {
        return mTransforms[nr];
    }


    // clone this morph target
    MorphTarget* MorphTargetStandard::Clone()
    {
        // create the clone and copy its base class values
        MorphTargetStandard* clone = aznew MorphTargetStandard(""); // use an empty dummy name, as we will copy over the ID generated from it anyway
        CopyBaseClassMemberValues(clone);

        // now copy over the standard morph target related values
        // first start with the transforms
        clone->mTransforms = mTransforms;

        // now clone the deform datas
        clone->mDeformDatas.Resize(mDeformDatas.GetLength());
        for (uint32 i = 0; i < mDeformDatas.GetLength(); ++i)
        {
            clone->mDeformDatas[i] = mDeformDatas[i]->Clone();
        }

        // return the clone
        return clone;
    }



    //---------------------------------------------------
    // DeformData
    //---------------------------------------------------

    // constructor
    MorphTargetStandard::DeformData::DeformData(uint32 nodeIndex, uint32 numVerts)
    {
        mNodeIndex  = nodeIndex;
        mNumVerts   = numVerts;
        mDeltas     = (VertexDelta*)MCore::Allocate(numVerts * sizeof(VertexDelta), EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS, MorphTargetStandard::MEMORYBLOCK_ID);
        mMinValue   = -10.0f;
        mMaxValue   = +10.0f;
    }


    // destructor
    MorphTargetStandard::DeformData::~DeformData()
    {
        MCore::Free(mDeltas);
    }


    // create
    MorphTargetStandard::DeformData* MorphTargetStandard::DeformData::Create(uint32 nodeIndex, uint32 numVerts)
    {
        return aznew MorphTargetStandard::DeformData(nodeIndex, numVerts);
    }


    // clone a morph target
    MorphTargetStandard::DeformData* MorphTargetStandard::DeformData::Clone()
    {
        MorphTargetStandard::DeformData* clone = aznew MorphTargetStandard::DeformData(mNodeIndex, mNumVerts);

        // copy the data
        clone->mMinValue = mMinValue;
        clone->mMaxValue = mMaxValue;
        MCore::MemCopy((uint8*)clone->mDeltas, (uint8*)mDeltas, mNumVerts * sizeof(VertexDelta));

        return clone;
    }


    // delete all deform datas
    void MorphTargetStandard::RemoveAllDeformDatas()
    {
        // get rid of all deform datas
        const uint32 numDeformDatas = mDeformDatas.GetLength();
        for (uint32 i = 0; i < numDeformDatas; ++i)
        {
            mDeformDatas[i]->Destroy();
        }

        mDeformDatas.Clear();
    }


    // pre-alloc memory for the deform datas
    void MorphTargetStandard::ReserveDeformDatas(uint32 numDeformDatas)
    {
        mDeformDatas.Reserve(numDeformDatas);
    }


    // pre-allocate memory for the transformations
    void MorphTargetStandard::ReserveTransformations(uint32 numTransforms)
    {
        mTransforms.Reserve(numTransforms);
    }


    void MorphTargetStandard::RemoveDeformData(uint32 index, bool delFromMem)
    {
        if (delFromMem)
        {
            delete mDeformDatas[index];
        }
        mDeformDatas.Remove(index);
    }


    void MorphTargetStandard::RemoveTransformation(uint32 index)
    {
        mTransforms.Remove(index);
    }


    // scale the morph data
    void MorphTargetStandard::Scale(float scaleFactor)
    {
        // if we don't need to adjust the scale, do nothing
        if (MCore::Math::IsFloatEqual(scaleFactor, 1.0f))
        {
            return;
        }

        // scale the transformations
        const uint32 numTransformations = mTransforms.GetLength();
        for (uint32 i = 0; i < numTransformations; ++i)
        {
            Transformation& transform = mTransforms[i];
            transform.mPosition *= scaleFactor;
        }

        // scale the deform datas (packed per vertex morph deltas)
        const uint32 numDeformDatas = mDeformDatas.GetLength();
        for (uint32 i = 0; i < numDeformDatas; ++i)
        {
            DeformData* deformData = mDeformDatas[i];
            DeformData::VertexDelta* deltas = deformData->mDeltas;

            float newMinValue = deformData->mMinValue * scaleFactor;
            float newMaxValue = deformData->mMaxValue * scaleFactor;

            // make sure the values won't be too small
            if (newMaxValue - newMinValue < 1.0f)
            {
                if (newMinValue < 0.0f && newMinValue > -1.0f)
                {
                    newMinValue = -1.0f;
                }

                if (newMaxValue > 0.0f && newMaxValue < 1.0f)
                {
                    newMaxValue = 1.0f;
                }
            }


            // iterate over the deltas (per vertex values)
            const uint32 numVerts = deformData->mNumVerts;
            for (uint32 v = 0; v < numVerts; ++v)
            {
                // decompress
                AZ::Vector3 decompressed = deltas[v].mPosition.ToVector3(deformData->mMinValue, deformData->mMaxValue);

                // scale
                decompressed *= scaleFactor;

                // compress again
                deltas[v].mPosition.FromVector3(decompressed, newMinValue, newMaxValue);
            }

            deformData->mMinValue = newMinValue;
            deformData->mMaxValue = newMaxValue;
        }
    }
} // namespace EMotionFX
