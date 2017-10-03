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

// include the required headers
#include "EMotionFXConfig.h"
#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/Distance.h>
#include "BaseObject.h"

MCORE_FORWARD_DECLARE(AttributeSettingsSet);
MCORE_FORWARD_DECLARE(AttributeSettings);
MCORE_FORWARD_DECLARE(AttributeSet);


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;
    class BlendTree;
    class AnimGraphStateMachine;
    class AnimGraphNode;
    class AnimGraphNodeGroup;
    class AnimGraphParameterGroup;
    class AnimGraphTransitionCondition;
    class AnimGraphGameControllerSettings;
    class AnimGraphMotionNode;
    class AnimGraphObject;

    //
    class EMFX_API AnimGraph
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraph, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH);

    public:
        static AnimGraph* Create(const char* name);

        AnimGraph* Clone(const char* name);

        void RecursiveUpdateAttributes();
        void RecursiveResetIsOutputReadyFlag(AnimGraphInstance* animGraphInstance);

        const char* GetName() const;
        const char* GetFileName() const;
        const MCore::String& GetNameString() const;
        const MCore::String& GetFileNameString() const;
        void SetName(const char* name);
        void SetFileName(const char* fileName);

        AnimGraphStateMachine* GetRootStateMachine() const;
        void SetRootStateMachine(AnimGraphStateMachine* stateMachine);

        AnimGraphNode* RecursiveFindNode(const char* name) const;
        AnimGraphNode* RecursiveFindNodeByID(uint32 id) const;         // name ID
        AnimGraphNode* RecursiveFindNodeByUniqueID(uint32 id) const;   // unique ID

        void RecursiveCollectNodesOfType(uint32 nodeTypeID, MCore::Array<AnimGraphNode*>* outNodes) const; // note: outNodes is NOT cleared internally, nodes are added to the array
        void RecursiveCollectTransitionConditionsOfType(uint32 conditionTypeID, MCore::Array<AnimGraphTransitionCondition*>* outConditions) const; // note: outNodes is NOT cleared internally, nodes are added to the array

        uint32 RecursiveCalcNumNodes() const;

        struct Statistics
        {
            AZ::u32 m_maxHierarchyDepth;
            AZ::u32 m_numStateMachines;
            AZ::u32 m_numStates;
            AZ::u32 m_numTransitions;
            AZ::u32 m_numWildcardTransitions;
            AZ::u32 m_numTransitionConditions;

            Statistics();
        };

        void RecursiveCalcStatistics(Statistics& outStatistics) const;
        uint32 RecursiveCalcNumNodeConnections() const;

        void DecreaseInternalAttributeIndices(uint32 decreaseEverythingHigherThan);

        MCore::String GenerateNodeName(const MCore::Array<MCore::String>& nameReserveList, const char* prefix = "Node") const;

        void SetNumParameters(uint32 numParams);

        void SetDescription(const char* description);
        const char* GetDescription() const;

        //-------------------------------------------------------------------------------------------------------------

        /**
         * Get the total number of parameters inside the anim graph. This will be the number of parameters from all groups as well as the ones
         * that are not part of any parameter group.
         * @result The number of parameters.
         */
        uint32 GetNumParameters() const;

        /**
         * Get a pointer to the given parameter.
         * @param[in] index The index of the parameter to return.
         * @result A pointer to the attribute info which represents the parameter.
         */
        MCore::AttributeSettings* GetParameter(uint32 index) const;

        /**
         * Find parameter by name.
         * @param[in] paramName The name of the parameter to search for.
         * @result A pointer to the attribute info with the given name. nullptr will be returned in case it has not been found.
         */
        MCore::AttributeSettings* FindParameter(const char* paramName) const;

        /**
         * Find parameter index by name.
         * @param[in] paramName The name of the parameter to search for.
         * @result The index of the attribute info with the given name. MCORE_INVALIDINDEX32 will be returned in case it has not been found.
         */
        uint32 FindParameterIndex(const char* paramName) const;

        /**
         * Add the given attribute info as a parameter.
         * The attribute info will be fully mananged and destroyed by the anim graph.
         * @param[in] paramInfo A pointer to the attribute info to add.
         */
        void AddParameter(MCore::AttributeSettings* paramInfo);

        /**
         * Insert the given attribute info as a parameter.
         * The attribute info will be fully mananged and destroyed by the anim graph.
         * @param[in] paramInfo A pointer to the attribute info to insert.
         * @param[in] index The index at which position the new parameter shall be inserted.
         */
        void InsertParameter(MCore::AttributeSettings* paramInfo, uint32 index);

        void SetParameter(uint32 index, MCore::AttributeSettings* paramInfo);

        void RemoveParameter(uint32 index, bool delFromMem = true);
        bool RemoveParameter(const char* paramName, bool delFromMem = true);
        void RemoveAllParameters(bool delFromMem = true);

        void SwapParameters(uint32 whatIndex, uint32 withIndex);

        //-------------------------------------------------------------------------------------------------------------

        /**
         * Get the unique identification number for the anim graph.
         * @return The unique identification number.
         */
        uint32 GetID() const;

        /**
         * Set the unique identification number for the anim graph.
         * @param[in] id The unique identification number.
         */
        void SetID(uint32 id);

        /**
         * Set the dirty flag which indicates whether the user has made changes to the anim graph. This indicator is set to true
         * when the user changed something like adding a node. When the user saves the anim graph, the indicator is usually set to false.
         * @param dirty The dirty flag.
         */
        void SetDirtyFlag(bool dirty);

        /**
         * Get the dirty flag which indicates whether the user has made changes to the anim graph. This indicator is set to true
         * when the user changed something like adding a node. When the user saves the anim graph, the indicator is usually set to false.
         * @return The dirty flag.
         */
        bool GetDirtyFlag() const;

        /**
         * Set if we want to automatically unregister the anim graph from the anim graph manager when we delete the anim graph.
         * On default this is set to true.
         * @param enabled Set to true when you wish to automatically have the anim graph unregistered, otherwise set it to false.
         */
        void SetAutoUnregister(bool enabled);

        /**
         * Check if the anim graph is automatically being unregistered from the anim graph manager when the anim graph motion gets deleted or not.
         * @result Returns true when it will get automatically deleted, otherwise false is returned.
         */
        bool GetAutoUnregister() const;

        /**
         * Marks the actor as used by the engine runtime, as opposed to the tool suite.
         */
        void SetIsOwnedByRuntime(bool isOwnedByRuntime);
        bool GetIsOwnedByRuntime() const;

        //-------------------------------------------------------------------------------------------------------------

        /**
         * Get the number of node groups.
         * @result The number of node groups.
         */
        uint32 GetNumNodeGroups() const;

        /**
         * Get a pointer to the given node group.
         * @param index The node group index, which must be in range of [0..GetNumNodeGroups()-1].
         */
        AnimGraphNodeGroup* GetNodeGroup(uint32 index) const;

        /**
         * Find a node group based on the name and return a pointer.
         * @param groupName The group name to search for.
         * @result A pointer to the node group with the given name. nullptr will be returned in case there is no node group in the anim graph with the given name.
         */
        AnimGraphNodeGroup* FindNodeGroupByName(const char* groupName) const;

        /**
         * Find a node group index based on the name and return its index.
         * @param groupName The group name to search for.
         * @result The index of the node group inside the anim graph, MCORE_INVALIDINDEX32 in case the node group wasn't found.
         */
        uint32 FindNodeGroupIndexByName(const char* groupName) const;

        /**
         * Find a node group based on the name id and return a pointer.
         * @param groupNameID The identification number returned by AnimGraphNodeGroup::GetNameID().
         * @result A pointer to the node group with the given name. nullptr will be returned in case there is no node group in the anim graph with the given id.
         */
        AnimGraphNodeGroup* FindNodeGroupByNameID(uint32 groupNameID) const;

        /**
         * Find a node group index based on the name id and return its index.
         * @param groupNameID The identification number returned by AnimGraphNodeGroup::GetNameID().
         * @result The index of the node group inside the anim graph, MCORE_INVALIDINDEX32 in case the node group wasn't found.
         */
        uint32 FindNodeGroupIndexByNameID(uint32 groupNameID) const;

        /**
         * Add the given node group.
         * @param nodeGroup A pointer to the new node group to add.
         */
        void AddNodeGroup(AnimGraphNodeGroup* nodeGroup);

        /**
         * Remove the node group at the given index.
         * @param index The node group index to remove. This value must be in range of [0..GetNumNodeGroups()-1].
         * @param delFromMem Set to true (default) when you wish to also delete the specified group from memory.
         */
        void RemoveNodeGroup(uint32 index, bool delFromMem = true);

        /**
         * Remove all node groups.
         * @param delFromMem Set to true (default) when you wish to also delete the node groups from memory.
         */
        void RemoveAllNodeGroups(bool delFromMem = true);

        /**
         * Find the node group the given node is part of and return a pointer to it.
         * @param[in] animGraphNode The node to search the node group for.
         * @result A pointer to the node group in which the given node is in. nullptr will be returned in case the node is not part of a node group.
         */
        AnimGraphNodeGroup* FindNodeGroupForNode(AnimGraphNode* animGraphNode) const;

        //-------------------------------------------------------------------------------------------------------------

        /**
         * Get the number of parameter groups.
         * @result The number of parameter groups.
         */
        uint32 GetNumParameterGroups() const;

        /**
         * Get a pointer to the given parameter group.
         * @param index The parameter group index, which must be in range of [0..GetNumParameterGroups()-1].
         */
        AnimGraphParameterGroup* GetParameterGroup(uint32 index) const;

        /**
         * Find a parameter group based on the name and return a pointer.
         * @param groupName The group name to search for.
         * @result A pointer to the parameter group with the given name. nullptr will be returned in case there is no parameter group in the anim graph with the given name.
         */
        AnimGraphParameterGroup* FindParameterGroupByName(const char* groupName) const;

        /**
         * Find a parameter group index based on the name and return its index.
         * @param groupName The group name to search for.
         * @result The index of the parameter group inside the anim graph, MCORE_INVALIDINDEX32 in case the parameter group wasn't found.
         */
        uint32 FindParameterGroupIndexByName(const char* groupName) const;

        /**
         * Add the given parameter group.
         * @param[in] parameterGroup A pointer to the new parameter group to add.
         */
        void AddParameterGroup(AnimGraphParameterGroup* parameterGroup);

        /**
         * Insert a parameter group at the given location.
         * @param[in] parameterGroup A pointer to the new parameter group to add.
         * @param[in] index The index position where to add the new parameter group.
         */
        void InsertParameterGroup(AnimGraphParameterGroup* parameterGroup, uint32 index);

        /**
         * Remove the parameter group at the given index.
         * @param[in] index The parameter group index to remove. This value must be in range of [0..GetNumParameterGroups()-1].
         * @param[in] delFromMem Set to true (default) when you wish to also delete the specified group from memory.
         */
        void RemoveParameterGroup(uint32 index, bool delFromMem = true);

        /**
         * Remove all parameter groups.
         * @param[in] delFromMem Set to true (default) when you wish to also delete the parameter groups from memory.
         */
        void RemoveAllParameterGroups(bool delFromMem = true);

        /**
         * Find the parameter group the given parameter is part of and return a pointer to it.
         * @param[in] parameterIndex The index of the parameter to search the parameter group for.
         * @result A pointer to the parameter group in which the given parameter is in. nullptr will be returned in case the parameter is not part of a parameter group.
         */
        AnimGraphParameterGroup* FindParameterGroupForParameter(uint32 parameterIndex) const;

        /**
         * Find the index for the given parameter group.
         * @param[in] parameterGroup A pointer to the parameter group of which we want to find the index inside the anim graph.
         * @result The index to the given parameter group in range [0..GetNumParameterGroups()-1]. MCORE_INVALIDINDEX32 will be returned in case the parameter group is not part of the anim graph.
         */
        uint32 FindParameterGroupIndex(AnimGraphParameterGroup* parameterGroup) const;

        /**
         * Iterate over all parameter groups and make sure the given parameter is not part of any of the groups anymore.
         * @param[in] parameterIndex The index of the parameter to be removed from all parameter groups.
         */
        void RemoveParameterFromAllGroups(uint32 parameterIndex);

        //-------------------------------------------------------------------------------------------------------------

        AnimGraphGameControllerSettings* GetGameControllerSettings() const;
        bool GetRetargetingEnabled() const;
        void SetRetargetingEnabled(bool enabled);

        void RemoveAllObjectData(AnimGraphObject* object, bool delFromMem);    // remove all unique object datas
        void AddObject(AnimGraphObject* object);       // registers the object in the array and modifies the object's object index value
        void RemoveObject(AnimGraphObject* object);    // doesn't actually remove it from memory, just removes it from the list

        MCORE_INLINE uint32 GetNumObjects() const                                                           { return mObjects.GetLength(); }
        MCORE_INLINE AnimGraphObject* GetObject(uint32 index) const                                        { return mObjects[index]; }
        void ReserveNumObjects(uint32 numObjects);

        MCORE_INLINE uint32 GetNumNodes() const                                                             { return mNodes.GetLength(); }
        MCORE_INLINE AnimGraphNode* GetNode(uint32 index) const                                            { return mNodes[index]; }
        void ReserveNumNodes(uint32 numNodes);

        MCORE_INLINE uint32 GetNumMotionNodes() const                                                       { return mMotionNodes.GetLength(); }
        MCORE_INLINE AnimGraphMotionNode* GetMotionNode(uint32 index) const                                { return mMotionNodes[index]; }

        MCORE_INLINE uint32 GetNumAnimGraphInstances() const                                               { return mAnimGraphInstances.GetLength(); }
        MCORE_INLINE AnimGraphInstance* GetAnimGraphInstance(uint32 index) const                          { return mAnimGraphInstances[index]; }
        void ReserveNumAnimGraphInstances(uint32 numInstances);
        void AddAnimGraphInstance(AnimGraphInstance* animGraphInstance);
        void RemoveAnimGraphInstance(AnimGraphInstance* animGraphInstance);

        MCore::AttributeSet* GetAttributeSet() const;

        void Lock();
        void Unlock();

        void SetUnitType(MCore::Distance::EUnitType unitType);
        MCore::Distance::EUnitType GetUnitType() const;

        void SetFileUnitType(MCore::Distance::EUnitType unitType);
        MCore::Distance::EUnitType GetFileUnitType() const;

        /**
         * Scale all data.
         * This is a very slow operation and is used to convert between different unit systems (cm, meters, etc).
         * @param scaleFactor The scale factor to scale the current data by.
         */
        void Scale(float scaleFactor);

        /**
         * Scale to a given unit type.
         * This method does nothing if the motion is already in this unit type.
         * You can check what the current unit type is with the GetUnitType() method.
         * @param targetUnitType The unit type to scale into (meters, centimeters, etc).
         */
        void ScaleToUnitType(MCore::Distance::EUnitType targetUnitType);


    private:
        void RecursiveCalcStatistics(Statistics& outStatistics, AnimGraphNode* animGraphNode, uint32 currentHierarchyDepth=0) const;

        AnimGraph(const char* name);
        ~AnimGraph();

        MCore::AttributeSettingsSet*                    mParameterSettings;
        MCore::AttributeSet*                            mAttributeSet;
        MCore::Array<AnimGraphNodeGroup*>              mNodeGroups;
        MCore::Array<AnimGraphParameterGroup*>         mParameterGroups;
        MCore::Array<AnimGraphObject*>                 mObjects;
        MCore::Array<AnimGraphNode*>                   mNodes;
        MCore::Array<AnimGraphMotionNode*>             mMotionNodes;
        MCore::Array<AnimGraphInstance*>               mAnimGraphInstances;
        MCore::String                                   mName;
        MCore::String                                   mFileName;
        MCore::String                                   mDescription;
        AnimGraphStateMachine*                         mRootStateMachine;
        AnimGraphGameControllerSettings*               mGameControllerSettings;
        MCore::Mutex                                    mLock;
        MCore::Distance::EUnitType                      mUnitType;
        MCore::Distance::EUnitType                      mFileUnitType;
        uint32                                          mID;                    /**< The unique identification number for the anim graph. */
        bool                                            mAutoUnregister;        /**< Specifies whether we will automatically unregister this anim graph set from the anim graph manager or not, when deleting this object. */
        bool                                            mRetarget;              /**< Is retargeting enabled on default? */
        bool                                            mDirtyFlag;             /**< The dirty flag which indicates whether the user has made changes to the anim graph since the last file save operation. */

#if defined(EMFX_DEVELOPMENT_BUILD)
        bool                                            mIsOwnedByRuntime;      /**< Set if the anim graph is used/owned by the engine runtime. */
#endif // EMFX_DEVELOPMENT_BUILD
    };
}   // namespace EMotionFX
