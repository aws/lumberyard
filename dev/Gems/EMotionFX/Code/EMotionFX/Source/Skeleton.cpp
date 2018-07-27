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
#include "Skeleton.h"
#include "Node.h"
#include <MCore/Source/StringConversions.h>
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Skeleton, SkeletonAllocator, 0)


    // constructor
    Skeleton::Skeleton()
    {
        mNodes.SetMemoryCategory(EMFX_MEMCATEGORY_SKELETON);
        mRootNodes.SetMemoryCategory(EMFX_MEMCATEGORY_SKELETON);
    }


    // destructor
    Skeleton::~Skeleton()
    {
        RemoveAllNodes();
    }


    // create the skeleton
    Skeleton* Skeleton::Create()
    {
        return aznew Skeleton();
    }


    // clone the skeleton
    Skeleton* Skeleton::Clone()
    {
        Skeleton* result = Skeleton::Create();

        const uint32 numNodes = mNodes.GetLength();
        result->ReserveNodes(numNodes);
        result->mRootNodes = mRootNodes;

        // clone the nodes
        for (uint32 i = 0; i < numNodes; ++i)
        {
            result->AddNode(mNodes[i]->Clone(result));
        }

        result->mBindPose = mBindPose;

        return result;
    }


    // reserve memory
    void Skeleton::ReserveNodes(uint32 numNodes)
    {
        mNodes.Reserve(numNodes);
    }


    // add a node
    void Skeleton::AddNode(Node* node)
    {
        mNodes.Add(node);
    }


    // remove a node
    void Skeleton::RemoveNode(uint32 nodeIndex, bool delFromMem)
    {
        if (delFromMem)
        {
            mNodes[nodeIndex]->Destroy();
        }

        mNodes.Remove(nodeIndex);
    }


    // remove all nodes
    void Skeleton::RemoveAllNodes(bool delFromMem)
    {
        if (delFromMem)
        {
            const uint32 numNodes = mNodes.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                mNodes[i]->Destroy();
            }
        }

        mNodes.Clear();
        mBindPose.Clear();
    }


    // search for a node on a specific name, and return a pointer to it
    Node* Skeleton::FindNodeByName(const char* name) const
    {
        // check the name of all nodes
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            if (mNodes[i]->GetNameString() == name)
            {
                return mNodes[i];
            }
        }

        return nullptr;
    }


    // search on name, non case sensitive
    Node* Skeleton::FindNodeByNameNoCase(const char* name) const
    {
        // check the names for all nodes
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            if (AzFramework::StringFunc::Equal(mNodes[i]->GetNameString().c_str(), name, false /* no case */))
            {
                return mNodes[i];
            }
        }

        return nullptr;
    }


    // search for a node on ID
    Node* Skeleton::FindNodeByID(uint32 id) const
    {
        // check the ID's for all nodes
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            if (mNodes[i]->GetID() == id)
            {
                return mNodes[i];
            }
        }

        return nullptr;
    }


    // set a given node
    void Skeleton::SetNode(uint32 index, Node* node)
    {
        mNodes[index] = node;
    }


    // set the number of nodes
    void Skeleton::SetNumNodes(uint32 numNodes)
    {
        mNodes.Resize(numNodes);
        mBindPose.SetNumTransforms(numNodes);
    }


    // update the node indices
    void Skeleton::UpdateNodeIndexValues(uint32 startNode)
    {
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = startNode; i < numNodes; ++i)
        {
            mNodes[i]->SetNodeIndex(i);
        }
    }


    // reserve memory for the root nodes array
    void Skeleton::ReserveRootNodes(uint32 numNodes)
    {
        mRootNodes.Reserve(numNodes);
    }


    // add a root node
    void Skeleton::AddRootNode(uint32 nodeIndex)
    {
        mRootNodes.Add(nodeIndex);
    }


    // remove a given root node
    void Skeleton::RemoveRootNode(uint32 nr)
    {
        mRootNodes.Remove(nr);
    }


    // remove all root nodes
    void Skeleton::RemoveAllRootNodes()
    {
        mRootNodes.Clear();
    }


    // calc the local space matrix for a given node
    void Skeleton::CalcLocalSpaceMatrix(uint32 nodeIndex, const Transform* localTransforms, MCore::Matrix* outMatrix)
    {
    #ifdef EMFX_SCALE_DISABLED
        outMatrix->InitFromPosRot(localTransforms[nodeIndex].mPosition, localTransforms[nodeIndex].mRotation);
    #else
        // check if there is a parent
        const uint32 parentIndex = GetNode(nodeIndex)->GetParentIndex();
        if (parentIndex != MCORE_INVALIDINDEX32)    // if there is a parent and it has scale
        {
            // calculate the inverse parent scale
            AZ::Vector3 invParentScale(1.0f, 1.0f, 1.0f);
            invParentScale /= localTransforms[parentIndex].mScale;  // TODO: unsafe, can get division by zero

            outMatrix->InitFromNoScaleInherit(localTransforms[nodeIndex].mPosition, localTransforms[nodeIndex].mRotation, localTransforms[nodeIndex].mScale, invParentScale);
        }
        else // there is NO parent, so its a root node
        {
            outMatrix->InitFromPosRotScale(localTransforms[nodeIndex].mPosition, localTransforms[nodeIndex].mRotation, localTransforms[nodeIndex].mScale);
        }
    #endif
    }


    // calculate the local space matrices in bind pose
    void Skeleton::CalcBindPoseLocalMatrices(MCore::Array<MCore::Matrix>& outLocalMatrices)
    {
        // resize the array to the right number
        const uint32 numNodes = mNodes.GetLength();
        outLocalMatrices.Resize(numNodes);

        // for all nodes
        for (uint32 n = 0; n < numNodes; ++n)
        {
            CalcLocalSpaceMatrix(n, mBindPose.GetLocalTransforms(), &outLocalMatrices[n]); // calculate the local TM from the bind pose transform
        }
    }


    // calculate the global space matrices from a given set of local space matrices
    void Skeleton::CalcGlobalMatrices(const MCore::Array<MCore::Matrix>& localMatrices, MCore::Array<MCore::Matrix>& outGlobalMatrices)
    {
        // resize the array to the right number
        const uint32 numNodes = GetNumNodes();
        outGlobalMatrices.Resize(numNodes);

        // process all nodes
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const Node* node = mNodes[i];

            // if this is a root node
            const uint32 parentIndex = node->GetParentIndex();
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                outGlobalMatrices[i].MultMatrix4x3(localMatrices[i], outGlobalMatrices[parentIndex]);
            }
            else
            {
                outGlobalMatrices[i] = localMatrices[i];
            }
        }
    }


    // calculate the global space matrices of the bind pose
    void Skeleton::CalcBindPoseGlobalMatrices(MCore::Array<MCore::Matrix>& outGlobalMatrices)
    {
        // calculate the local space matrices of the bind pose first
        MCore::Array<MCore::Matrix> localMatrices;
        CalcBindPoseLocalMatrices(localMatrices);

        // now calculate the global space matrices using those local space matrices
        CalcGlobalMatrices(localMatrices, outGlobalMatrices);
    }


    // log all node names
    void Skeleton::LogNodes()
    {
        const uint32 numNodes = mNodes.GetLength();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            MCore::LogInfo("%d = '%s'", i, mNodes[i]->GetName());
        }
    }


    // calculate the hierarchy depth for a given node
    uint32 Skeleton::CalcHierarchyDepthForNode(uint32 nodeIndex) const
    {
        uint32 result = 0;
        Node* curNode = mNodes[nodeIndex];
        while (curNode->GetParentNode())
        {
            result++;
            curNode = curNode->GetParentNode();
        }

        return result;
    }
}   // namespace EMotionFX
