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
#include "AnimGraphNodeGroup.h"
#include <MCore/Source/StringIDGenerator.h>


namespace EMotionFX
{
    // default constructor
    AnimGraphNodeGroup::AnimGraphNodeGroup()
    {
        mNodes.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_NODEGROUP);
        mColor      = MCore::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f);
        mIsVisible  = true;
        SetName("");
    }


    // extended constructor
    AnimGraphNodeGroup::AnimGraphNodeGroup(const char* groupName)
    {
        mNodes.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_NODEGROUP);
        SetName(groupName);
        mIsVisible = true;
    }


    // another extended constructor
    AnimGraphNodeGroup::AnimGraphNodeGroup(const char* groupName, uint32 numNodes)
    {
        mNodes.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_NODEGROUP);
        SetName(groupName);
        SetNumNodes(numNodes);
        mIsVisible = true;
    }


    // destructor
    AnimGraphNodeGroup::~AnimGraphNodeGroup()
    {
        RemoveAllNodes();
    }


    // default create
    AnimGraphNodeGroup* AnimGraphNodeGroup::Create()
    {
        return new AnimGraphNodeGroup();
    }


    // extended create
    AnimGraphNodeGroup* AnimGraphNodeGroup::Create(const char* groupName)
    {
        return new AnimGraphNodeGroup(groupName);
    }


    // extended create
    AnimGraphNodeGroup* AnimGraphNodeGroup::Create(const char* groupName, uint32 numNodes)
    {
        return new AnimGraphNodeGroup(groupName, numNodes);
    }


    // clear node group
    void AnimGraphNodeGroup::RemoveAllNodes()
    {
        mNodes.Clear();
    }


    // set the name of the group
    void AnimGraphNodeGroup::SetName(const char* groupName)
    {
        if (groupName)
        {
            mNameID = MCore::GetStringIDGenerator().GenerateIDForString(groupName);
        }
        else
        {
            mNameID = MCORE_INVALIDINDEX32;
        }
    }


    // get the name of the group as character buffer
    const char* AnimGraphNodeGroup::GetName() const
    {
        return MCore::GetStringIDGenerator().GetName(mNameID).AsChar();
    }


    // get the name of the string as mcore string object
    const MCore::String& AnimGraphNodeGroup::GetNameString() const
    {
        return MCore::GetStringIDGenerator().GetName(mNameID);
    }


    // set the color of the group
    void AnimGraphNodeGroup::SetColor(const MCore::RGBAColor& color)
    {
        mColor = color;
    }


    // get the color of the group
    const MCore::RGBAColor& AnimGraphNodeGroup::GetColor() const
    {
        return mColor;
    }


    // set the visibility flag
    void AnimGraphNodeGroup::SetIsVisible(bool isVisible)
    {
        mIsVisible = isVisible;
    }


    // set the number of nodes
    void AnimGraphNodeGroup::SetNumNodes(uint32 numNodes)
    {
        mNodes.Resize(numNodes);
    }


    // get the number of nodes
    uint32 AnimGraphNodeGroup::GetNumNodes() const
    {
        return mNodes.GetLength();
    }


    // set a given node to a given node number
    void AnimGraphNodeGroup::SetNode(uint32 index, uint32 nodeID)
    {
        mNodes[index] = nodeID;
    }


    // get the node number of a given index
    uint32 AnimGraphNodeGroup::GetNode(uint32 index) const
    {
        return mNodes[index];
    }


    // add a given node to the group (performs a realloc internally)
    void AnimGraphNodeGroup::AddNode(uint32 nodeID)
    {
        // add the node in case it is not in yet
        if (Contains(nodeID) == false)
        {
            mNodes.Add(nodeID);
        }
    }


    // remove a given node by its node id
    void AnimGraphNodeGroup::RemoveNodeByID(uint32 nodeID)
    {
        mNodes.RemoveByValue(nodeID);
    }


    // remove a given array element from the list of nodes
    void AnimGraphNodeGroup::RemoveNodeByGroupIndex(uint32 index)
    {
        mNodes.Remove(index);
    }


    // get the node array directly
    MCore::Array<uint32>& AnimGraphNodeGroup::GetNodeArray()
    {
        return mNodes;
    }


    // check if the given node id
    bool AnimGraphNodeGroup::Contains(uint32 nodeID) const
    {
        return (mNodes.Find(nodeID) != MCORE_INVALIDINDEX32);
    }


    // init from another group
    void AnimGraphNodeGroup::InitFrom(const AnimGraphNodeGroup& other)
    {
        mNodes          = other.mNodes;
        mColor          = other.mColor;
        mNameID         = other.mNameID;
        mIsVisible      = other.mIsVisible;
    }


    bool AnimGraphNodeGroup::GetIsVisible() const
    {
        return mIsVisible;
    }


    uint32 AnimGraphNodeGroup::GetNameID() const
    {
        return mNameID;
    }
} // namespace EMotionFX

