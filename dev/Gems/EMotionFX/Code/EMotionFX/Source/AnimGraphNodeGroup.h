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

#pragma once

// include required files
#include "EMotionFXConfig.h"
#include "BaseObject.h"

#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/Array.h>
#include <MCore/Source/Color.h>


namespace EMotionFX
{
    class EMFX_API AnimGraphNodeGroup
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphNodeGroup, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH);

    public:
        /**
         * Default creation.
         * This does not assign a name and there will be nodes inside this group on default.
         */
        static AnimGraphNodeGroup* Create();

        /**
         * Extended creation.
         * @param groupName The name of the group. Please keep in mind that it is not allowed to have two groups with the same name inside a anim graph.
         */
        static AnimGraphNodeGroup* Create(const char* groupName);

        /**
         * Extended creation.
         * @param groupName The name of the group. Please keep in mind that it is not allowed to have two groups with the same name inside a anim graph.
         * @param numNodes The number of nodes to create inside the group. This will have all uninitialized values for the node ids in the group, so be sure that you
         *                 set them all to some valid node index using the AnimGraphNodeGroup::SetNode(...) method. This constructor automatically calls the SetNumNodes(...) method.
         */
        static AnimGraphNodeGroup* Create(const char* groupName, uint32 numNodes);

        /**
         * Set the name of the group. Please keep in mind that group names must be unique inside the anim graph objects. So you should not have two or more groups with the same name.
         * @param groupName The name of the group.
         */
        void SetName(const char* groupName);

        /**
         * Get the name of the group as null terminated character buffer.
         * @result The name of the group.
         */
        const char* GetName() const;

        /**
         * Get the name of the group, in form of a MCore::String object.
         * @result The name as a reference to a MCore::String object.
         */
        const MCore::String& GetNameString() const;

        /**
         * Get the unique identification number returned by the MCore::GetStringIDGenerator() for the given node group name.
         * @return The unique name identification number.
         */
        uint32 GetNameID() const;

        /**
         * Set the color of the group.
         * @param color The color the nodes of the group will be filled with.
         */
        void SetColor(const MCore::RGBAColor& color);

        /**
         * Get the color of the group.
         * @result The color the nodes of the group will be filled with.
         */
        const MCore::RGBAColor& GetColor() const;

        /**
         * Check the visibility flag.
         * This flag has been set by the user and identifies if this node group is visible or not.
         * @result True when the node group is marked as visible, otherwise false is returned.
         */
        bool GetIsVisible() const;

        /**
         * Change the visibility state.
         * @param isVisible Set to true when the node group is visible, otherwise set it to false.
         */
        void SetIsVisible(bool isVisible);

        /**
         * Set the number of nodes that remain inside this group.
         * This will resize the array of node ids. Don't forget to initialize the node values after increasing the number of nodes.
         * @param numNodes The number of nodes that are inside this group.
         */
        void SetNumNodes(uint32 numNodes);

        /**
         * Get the number of nodes that remain inside this group.
         * @result The number of nodes inside this group.
         */
        uint32 GetNumNodes() const;

        /**
         * Set the value of a given node.
         * @param index The node number inside this group, which must be in range of [0..GetNumNodes()-1].
         * @param nodeID The value for the given node. This is the node id where this group will belong to.
         *               To get access to the actual node object use AnimGraph::RecursiveFindNodeByID( nodeID ).
         */
        void SetNode(uint32 index, uint32 nodeID);

        /**
         * Get the node id for a given node inside the group.
         * @param index The node number inside this group, which must be in range of [0..GetNumNodes()-1].
         * @result The node id, which points inside the Actor object. Use AnimGraph::RecursiveFindNodeByID( nodeID ) to get access to the node information.
         */
        uint32 GetNode(uint32 index) const;

        /**
         * Check if the node with the given id is inside the node group.
         * @param nodeID The node id to check.
         * @result True in case the node with the given id is inside the node group, false if not.
         */
        bool Contains(uint32 nodeID) const;

        /**
         * Add a given node to this group.
         * Please keep in mind that performing an AddNode will result in a reallocation being done.
         * It is much better to use SetNumNodes(...) in combination with SetNode(...) upfront if the total number of nodes is known upfront.
         * @param nodeID The value for the given node. This is the node id where this group will belong to. To get access to the actual node object use AnimGraph::RecursiveFindNodeByID( nodeID ).
         */
        void AddNode(uint32 nodeID);

        /**
         * Remove a given node from the group by its node id (the value returned by AnimGraphNode::GetID().
         * If you wish to remove for example the 3rd node inside this group, then use RemoveNodeByGroupIndex(...) instead.
         * Removing a node which is not part of this group will do nothing, except that it wastes performance as it will perform a search inside the list of nodes inside this group.
         * @param nodeID The value for the given node. This is the node id where this group will belong to. To get access to the actual node object use AnimGraph::RecursiveFindNodeByID( nodeID ).
         */
        void RemoveNodeByID(uint32 nodeID);

        /**
         * Remove a given node from the group by the array element index.
         * If for example you wish to remove the 3rd node from the group, you can call RemoveNodeByGroupIndex( 2 ).
         * If you wish to remove a node by its node id, then use RemoveNodeByID(...) instead.
         * @param index The node index in the group. So for example an index value of 5 will remove the sixth node from the group.
         *              The index value must be in range of [0..GetNumNodes() - 1].
         */
        void RemoveNodeByGroupIndex(uint32 index);

        /**
         * Clear the node group. This removes all nodes.
         */
        void RemoveAllNodes();

        /**
         * Get direct access to the array of node ids that are part of this group.
         * @result A reference to the array of nodes inside this group. Please use this with care.
         */
        MCore::Array<uint32>& GetNodeArray();

        /**
         * Initialize the node group based on another group. Please note that the name of this group will also be copied and it is not allowed to have multiple groups with the same name in the same animgraph.
         * @param other The node group to initialize from.
         */
        void InitFrom(const AnimGraphNodeGroup& other);

    protected:
        MCore::Array<uint32>        mNodes;             /**< The node ids that are inside this group. */
        MCore::RGBAColor            mColor;             /**< The color the nodes of the group will be filled with. */
        uint32                      mNameID;            /**< The unique identification number for the node group name. */
        bool                        mIsVisible;

        /**
         * The default constructor.
         * This does not assign a name and there will be nodes inside this group on default.
         */
        AnimGraphNodeGroup();

        /**
         * Extended constructor.
         * @param groupName The name of the group. Please keep in mind that it is not allowed to have two groups with the same name inside a anim graph.
         */
        AnimGraphNodeGroup(const char* groupName);

        /**
         * Another extended constructor.
         * @param groupName The name of the group. Please keep in mind that it is not allowed to have two groups with the same name inside a anim graph.
         * @param numNodes The number of nodes to create inside the group. This will have all uninitialized values for the node ids in the group, so be sure that you
         *                 set them all to some valid node index using the AnimGraphNodeGroup::SetNode(...) method. This constructor automatically calls the SetNumNodes(...) method.
         */
        AnimGraphNodeGroup(const char* groupName, uint32 numNodes);

        /**
         * The destructor.
         */
        virtual ~AnimGraphNodeGroup();
    };
}   // namespace EMotionFX
