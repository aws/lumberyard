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


    Node* Skeleton::FindNodeByName(const char* name) const
    {
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


    Node* Skeleton::FindNodeByName(const AZStd::string& name) const
    {
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


    Node* Skeleton::FindNodeAndIndexByName(const AZStd::string& name, AZ::u32& outIndex) const
    {
        if (name.empty())
        {
            outIndex = MCORE_INVALIDINDEX32;
            return nullptr;
        }

        Node* joint = FindNodeByNameNoCase(name.c_str());
        if (!joint)
        {
            outIndex = MCORE_INVALIDINDEX32;
            return nullptr;
        }

        outIndex = joint->GetNodeIndex();
        return joint;
    }
}   // namespace EMotionFX
