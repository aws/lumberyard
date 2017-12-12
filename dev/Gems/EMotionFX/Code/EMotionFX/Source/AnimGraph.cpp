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
#include "AnimGraph.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphManager.h"
#include "AnimGraphInstance.h"
#include "AnimGraphNodeGroup.h"
#include "AnimGraphParameterGroup.h"
#include "AnimGraphObject.h"
#include "AnimGraphMotionNode.h"
#include "Pose.h"
#include "ActorInstance.h"
#include "Recorder.h"
#include "EventManager.h"
#include "AnimGraphGameControllerSettings.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/AttributeSet.h>
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    AnimGraph::AnimGraph(const char* name)
        : BaseObject()
    {
        mObjects.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH);
        mNodes.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH);
        mNodeGroups.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH);
        mParameterGroups.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH);
        mAnimGraphInstances.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH);
        mMotionNodes.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH);

        mName                       = name;
        mID                         = MCore::GetIDGenerator().GenerateID();
        mDirtyFlag                  = false;
        mAutoUnregister             = true;
        mRetarget                   = false;
        mParameterSettings          = MCore::AttributeSettingsSet::Create();
        mGameControllerSettings     = AnimGraphGameControllerSettings::Create();
        mRootStateMachine           = nullptr;
        mAttributeSet               = MCore::AttributeSet::Create();
        mUnitType                   = GetEMotionFX().GetUnitType();
        mFileUnitType               = mUnitType;

#if defined(EMFX_DEVELOPMENT_BUILD)
        mIsOwnedByRuntime           = false;
#endif // EMFX_DEVELOPMENT_BUILD

        // reserve some memory
        mObjects.Reserve(2048);
        mNodes.Reserve(1024);
        mAnimGraphInstances.Reserve(1024);
        mNodeGroups.Reserve(8);
        mParameterGroups.Reserve(8);
        mMotionNodes.Reserve(128);

        // automatically register the anim graph
        GetAnimGraphManager().AddAnimGraph(this);

        GetEventManager().OnCreateAnimGraph(this);
    }


    // destructor
    AnimGraph::~AnimGraph()
    {
        GetEventManager().OnDeleteAnimGraph(this);
        GetRecorder().RemoveAnimGraphFromRecording(this);

        RemoveAllParameters(true);
        RemoveAllNodeGroups();
        RemoveAllParameterGroups();

        if (mRootStateMachine) 
        {
            mRootStateMachine->Destroy();
        }
        mAttributeSet->Destroy();

        if (mGameControllerSettings)
        {
            mGameControllerSettings->Destroy();
        }

        if (mParameterSettings)
        {
            mParameterSettings->Destroy();
        }

        // automatically unregister the anim graph
        if (mAutoUnregister)
        {
            GetAnimGraphManager().RemoveAnimGraph(this, false);
        }
    }


    // static create method
    AnimGraph* AnimGraph::Create(const char* name)
    {
        return new AnimGraph(name);
    }


    // recursive call update data for all nodes
    void AnimGraph::RecursiveUpdateAttributes()
    {
        mRootStateMachine->RecursiveUpdateAttributes();
    }


    // add a parameter
    void AnimGraph::AddParameter(MCore::AttributeSettings* paramInfo)
    {
        mParameterSettings->AddAttribute(paramInfo);
    }


    // insert a parameter
    void AnimGraph::InsertParameter(MCore::AttributeSettings* paramInfo, uint32 index)
    {
        mParameterSettings->InsertAttribute(index, paramInfo);
    }


    // get the number of parameters
    uint32 AnimGraph::GetNumParameters() const
    {
        return mParameterSettings->GetNumAttributes();
    }


    // get a given parameter
    MCore::AttributeSettings* AnimGraph::GetParameter(uint32 index) const
    {
        return mParameterSettings->GetAttribute(index);
    }


    // remove a given parameter
    void AnimGraph::RemoveParameter(uint32 index, bool delFromMem)
    {
        mParameterSettings->RemoveAttributeByIndex(index, delFromMem);
    }


    // remove the parameter by name
    bool AnimGraph::RemoveParameter(const char* paramName, bool delFromMem)
    {
        return mParameterSettings->RemoveAttributeByName(paramName, delFromMem);
    }


    // remove all params
    void AnimGraph::RemoveAllParameters(bool delFromMem)
    {
        mParameterSettings->RemoveAllAttributes(delFromMem);
    }


    // recursively find a given node
    AnimGraphNode* AnimGraph::RecursiveFindNode(const char* name) const
    {
        // get the ID for a given node name we search for
        uint32 id = MCore::GetStringIDGenerator().GenerateIDForString(name);

        // search based on the id
        return mRootStateMachine->RecursiveFindNodeByID(id);
    }


    // recursively find a given node
    AnimGraphNode* AnimGraph::RecursiveFindNodeByID(uint32 id) const
    {
        return mRootStateMachine->RecursiveFindNodeByID(id);
    }


    // recursively find a given node
    AnimGraphNode* AnimGraph::RecursiveFindNodeByUniqueID(uint32 id) const
    {
        return mRootStateMachine->RecursiveFindNodeByUniqueID(id);
    }


    // generate a state name that isn't in use yet
    MCore::String AnimGraph::GenerateNodeName(const MCore::Array<MCore::String>& nameReserveList, const char* prefix) const
    {
        MCore::String result;
        uint32 number = 0;
        bool found = false;
        while (found == false)
        {
            // build the string
            result.Format("%s%d", prefix, number++);

            // if there is no such state machine yet
            if (RecursiveFindNode(result.AsChar()) == nullptr && nameReserveList.Find(result.AsChar()) == MCORE_INVALIDINDEX32)
            {
                found = true;
            }
        }

        return result;
    }


    uint32 AnimGraph::RecursiveCalcNumNodes() const
    {
        return mRootStateMachine->RecursiveCalcNumNodes();
    }


    AnimGraph::Statistics::Statistics() :
        m_maxHierarchyDepth(0),
        m_numStateMachines(0),
        m_numStates(1),
        m_numTransitions(0),
        m_numWildcardTransitions(0),
        m_numTransitionConditions(0)
    {
    }


    void AnimGraph::RecursiveCalcStatistics(Statistics& outStatistics) const
    {
        RecursiveCalcStatistics(outStatistics, mRootStateMachine);
    }


    void AnimGraph::RecursiveCalcStatistics(Statistics& outStatistics, AnimGraphNode* animGraphNode, uint32 currentHierarchyDepth) const
    {
        outStatistics.m_maxHierarchyDepth = MCore::Max<uint32>(currentHierarchyDepth, outStatistics.m_maxHierarchyDepth);

        // Are we dealing with a state machine? If yes, increase the number of transitions, states etc. in the statistics.
        if (animGraphNode->GetType() == AnimGraphStateMachine::TYPE_ID)
        {
            AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(animGraphNode);
            outStatistics.m_numStateMachines++;

            const uint32 numTransitions = stateMachine->GetNumTransitions();
            outStatistics.m_numTransitions += numTransitions;

            outStatistics.m_numStates += stateMachine->GetNumChildNodes();

            for (uint32 i=0; i<numTransitions; ++i)
            {
                AnimGraphStateTransition* transition = stateMachine->GetTransition(i);

                if (transition->GetIsWildcardTransition())
                {
                    outStatistics.m_numWildcardTransitions++;
                }

                outStatistics.m_numTransitionConditions += transition->GetNumConditions();
            }
        }

        const uint32 numChildNodes = animGraphNode->GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            RecursiveCalcStatistics(outStatistics, animGraphNode->GetChildNode(i), currentHierarchyDepth+1);
        }
    }


    // recursively calculate the number of node connections
    uint32 AnimGraph::RecursiveCalcNumNodeConnections() const
    {
        return mRootStateMachine->RecursiveCalcNumNodeConnections();
    }


    // resize the number of parameters
    void AnimGraph::SetNumParameters(uint32 numParams)
    {
        mParameterSettings->Resize(numParams);
    }


    // set the parameter pointer
    void AnimGraph::SetParameter(uint32 index, MCore::AttributeSettings* paramInfo)
    {
        mParameterSettings->SetAttribute(index, paramInfo);
    }


    // adjust the dirty flag
    void AnimGraph::SetDirtyFlag(bool dirty)
    {
        mDirtyFlag = dirty;
    }

    // adjust the auto unregistering from the anim graph manager on delete
    void AnimGraph::SetAutoUnregister(bool enabled)
    {
        mAutoUnregister = enabled;
    }


    // do we auto unregister from the anim graph manager on delete?
    bool AnimGraph::GetAutoUnregister() const
    {
        return mAutoUnregister;
    }


    void AnimGraph::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        mIsOwnedByRuntime = isOwnedByRuntime;
#endif
    }
    

    bool AnimGraph::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return mIsOwnedByRuntime;
#else
        return true;
#endif
    }


    // get a pointer to the given node group
    AnimGraphNodeGroup* AnimGraph::GetNodeGroup(uint32 index) const
    {
        return mNodeGroups[index];
    }


    // find the node group by name
    AnimGraphNodeGroup* AnimGraph::FindNodeGroupByName(const char* groupName) const
    {
        // get the number of node groups and iterate through them
        const uint32 numNodeGroups = mNodeGroups.GetLength();
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            // compare the node names and return a pointer in case they are equal
            if (mNodeGroups[i]->GetNameString().CheckIfIsEqual(groupName))
            {
                return mNodeGroups[i];
            }
        }

        // failure
        return nullptr;
    }


    // find the node group index by name
    uint32 AnimGraph::FindNodeGroupIndexByName(const char* groupName) const
    {
        // get the number of node groups and iterate through them
        const uint32 numNodeGroups = mNodeGroups.GetLength();
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            // compare the node names and return the index in case they are equal
            if (mNodeGroups[i]->GetNameString().CheckIfIsEqual(groupName))
            {
                return i;
            }
        }

        // failure
        return MCORE_INVALIDINDEX32;
    }


    // find the node group by name identification number
    AnimGraphNodeGroup* AnimGraph::FindNodeGroupByNameID(uint32 groupNameID) const
    {
        // get the number of node groups and iterate through them
        const uint32 numNodeGroups = mNodeGroups.GetLength();
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            // compare the node ids and return a pointer in case they are equal
            if (mNodeGroups[i]->GetNameID() == groupNameID)
            {
                return mNodeGroups[i];
            }
        }

        // failure
        return nullptr;
    }


    // find the node group index by name identification number
    uint32 AnimGraph::FindNodeGroupIndexByNameID(uint32 groupNameID) const
    {
        // get the number of node groups and iterate through them
        const uint32 numNodeGroups = mNodeGroups.GetLength();
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            // compare the ids and return the index in case they are equal
            if (mNodeGroups[i]->GetNameID() == groupNameID)
            {
                return i;
            }
        }

        // failure
        return MCORE_INVALIDINDEX32;
    }


    // add the given node group to the anim graph
    void AnimGraph::AddNodeGroup(AnimGraphNodeGroup* nodeGroup)
    {
        mNodeGroups.Add(nodeGroup);
    }


    // remove the node group at the given index from the anim graph
    void AnimGraph::RemoveNodeGroup(uint32 index, bool delFromMem)
    {
        // destroy the object
        if (delFromMem)
        {
            mNodeGroups[index]->Destroy();
        }

        // remove the node group from the array
        mNodeGroups.Remove(index);
    }


    // remove all node groups
    void AnimGraph::RemoveAllNodeGroups(bool delFromMem)
    {
        // destroy the node groups
        if (delFromMem)
        {
            const uint32 numNodeGroups = mNodeGroups.GetLength();
            for (uint32 i = 0; i < numNodeGroups; ++i)
            {
                mNodeGroups[i]->Destroy();
            }
        }

        // remove all node groups
        mNodeGroups.Clear();
    }


    // get the number of node groups
    uint32 AnimGraph::GetNumNodeGroups() const
    {
        return mNodeGroups.GetLength();
    }


    // find the node group �n which the given anim graph node is in and return a pointer to it
    AnimGraphNodeGroup* AnimGraph::FindNodeGroupForNode(AnimGraphNode* animGraphNode) const
    {
        // get the number of node groups and iterate through them
        const uint32 numNodeGroups = GetNumNodeGroups();
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            // get a pointer to the currently iterated node group
            AnimGraphNodeGroup* nodeGroup = GetNodeGroup(i);

            // check if the given node is part of the currently iterated node group
            if (nodeGroup->Contains(animGraphNode->GetID()))
            {
                return nodeGroup;
            }
        }

        // failure, the node is not part of a node group
        return nullptr;
    }


    // get the nodes of a given type, recursively
    void AnimGraph::RecursiveCollectNodesOfType(uint32 nodeTypeID, MCore::Array<AnimGraphNode*>* outNodes) const
    {
        mRootStateMachine->RecursiveCollectNodesOfType(nodeTypeID, outNodes);
    }


    // get the transition conditions of a given type, recursively
    void AnimGraph::RecursiveCollectTransitionConditionsOfType(uint32 conditionTypeID, MCore::Array<AnimGraphTransitionCondition*>* outConditions) const
    {
        mRootStateMachine->RecursiveCollectTransitionConditionsOfType(conditionTypeID, outConditions);
    }


    // swap parameters
    void AnimGraph::SwapParameters(uint32 whatIndex, uint32 withIndex)
    {
        // swap the parameter pointers
        mParameterSettings->Swap(whatIndex, withIndex);

        //MCore::AttributeSettings* temp = mParameters[whatIndex];
        //mParameters[whatIndex] = mParameters[withIndex];
        //mParameters[withIndex] = temp;

        // find the parameter groups in which the parameters were and switch them
        /*AnimGraphParameterGroup* whatGroup = animGraph->FindParameterGroupForParameter(withIndex);
        if (whatGroup)
        {
            whatGroup->RemoveParameter(whatIndex);
            whatGroup->AddParameter(withIndex);
        }

        AnimGraphParameterGroup* withGroup = animGraph->FindParameterGroupForParameter(whatIndex);
        if (withGroup)
        {
            withGroup->RemoveParameter(withIndex);
            withGroup->AddParameter(whatIndex);
        }*/
    }


    // get a pointer to the given parameter group
    AnimGraphParameterGroup* AnimGraph::GetParameterGroup(uint32 index) const
    {
        return mParameterGroups[index];
    }


    // find the parameter group by name
    AnimGraphParameterGroup* AnimGraph::FindParameterGroupByName(const char* groupName) const
    {
        // get the number of parameter groups and iterate through them
        const uint32 numParameterGroups = mParameterGroups.GetLength();
        for (uint32 i = 0; i < numParameterGroups; ++i)
        {
            // compare the parameter names and return a pointer in case they are equal
            if (mParameterGroups[i]->GetNameString() == groupName)
            {
                return mParameterGroups[i];
            }
        }

        // failure
        return nullptr;
    }


    // find the parameter group index by name
    uint32 AnimGraph::FindParameterGroupIndexByName(const char* groupName) const
    {
        // get the number of parameter groups and iterate through them
        const uint32 numParameterGroups = mParameterGroups.GetLength();
        for (uint32 i = 0; i < numParameterGroups; ++i)
        {
            // compare the parameter names and return the index in case they are equal
            if (mParameterGroups[i]->GetNameString() == groupName)
            {
                return i;
            }
        }

        // failure
        return MCORE_INVALIDINDEX32;
    }


    // find the index for a parameter group
    uint32 AnimGraph::FindParameterGroupIndex(AnimGraphParameterGroup* parameterGroup) const
    {
        return mParameterGroups.Find(parameterGroup);
    }


    // add the given parameter group to the anim graph
    void AnimGraph::AddParameterGroup(AnimGraphParameterGroup* parameterGroup)
    {
        mParameterGroups.Add(parameterGroup);
    }


    // insert a parameter group at the given location
    void AnimGraph::InsertParameterGroup(AnimGraphParameterGroup* parameterGroup, uint32 index)
    {
        mParameterGroups.Insert(index, parameterGroup);
    }


    // remove the parameter group at the given index from the anim graph
    void AnimGraph::RemoveParameterGroup(uint32 index, bool delFromMem)
    {
        // destroy the object
        if (delFromMem)
        {
            mParameterGroups[index]->Destroy();
        }

        // remove the parameter group from the array
        mParameterGroups.Remove(index);
    }


    // remove all parameter groups
    void AnimGraph::RemoveAllParameterGroups(bool delFromMem)
    {
        // destroy the parameter groups
        if (delFromMem)
        {
            const uint32 numParameterGroups = mParameterGroups.GetLength();
            for (uint32 i = 0; i < numParameterGroups; ++i)
            {
                mParameterGroups[i]->Destroy();
            }
        }

        // remove all parameter groups
        mParameterGroups.Clear();
    }


    // get the number of parameter groups
    uint32 AnimGraph::GetNumParameterGroups() const
    {
        return mParameterGroups.GetLength();
    }


    // find the parameter group �n which the given anim graph parameter is in and return a pointer to it
    AnimGraphParameterGroup* AnimGraph::FindParameterGroupForParameter(uint32 parameterIndex) const
    {
        // get the number of parameter groups and iterate through them
        const uint32 numParameterGroups = GetNumParameterGroups();
        for (uint32 i = 0; i < numParameterGroups; ++i)
        {
            // get a pointer to the currently iterated parameter group
            AnimGraphParameterGroup* group = GetParameterGroup(i);

            // check if the given parameter is part of the currently iterated parameter group
            if (group->Contains(parameterIndex))
            {
                return group;
            }
        }

        // failure, the parameter is not part of a parameter group
        return nullptr;
    }


    // remove the parameter from all groups so that it is not part of any group
    void AnimGraph::RemoveParameterFromAllGroups(uint32 parameterIndex)
    {
        // get the number of parameter groups and iterate through them
        const uint32 numParameterGroups = GetNumParameterGroups();
        for (uint32 i = 0; i < numParameterGroups; ++i)
        {
            // get a pointer to the currently iterated parameter group and remove the given parameter from the group
            AnimGraphParameterGroup* group = GetParameterGroup(i);
            group->RemoveParameter(parameterIndex);
        }
    }


    // find a given parameter
    MCore::AttributeSettings* AnimGraph::FindParameter(const char* paramName) const
    {
        const uint32 index = mParameterSettings->FindAttributeIndexByName(paramName);
        if (index != MCORE_INVALIDINDEX32)
        {
            return mParameterSettings->GetAttribute(index);
        }

        // not found
        return nullptr;
    }


    // find a given parameter
    uint32 AnimGraph::FindParameterIndex(const char* paramName) const
    {
        return mParameterSettings->FindAttributeIndexByName(paramName);
    }


    // delete all unique datas for a given object
    void AnimGraph::RemoveAllObjectData(AnimGraphObject* object, bool delFromMem)
    {
        MCore::LockGuard lock(mLock);

        // get the number of anim graph instances and iterate through them
        const uint32 numInstances = mAnimGraphInstances.GetLength();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            mAnimGraphInstances[i]->RemoveUniqueObjectData(object->GetObjectIndex(), delFromMem);  // remove all unique datas that belong to the given object
        }
    }


    // set the root state machine
    void AnimGraph::SetRootStateMachine(AnimGraphStateMachine* stateMachine)
    {
        mRootStateMachine = stateMachine;

        // make sure the name is always the same for the root state machine
        mRootStateMachine->SetName("Root");
    }


    // add an object
    void AnimGraph::AddObject(AnimGraphObject* object)
    {
        MCore::LockGuard lock(mLock);

        // assign the index and add it to the objects array
        object->SetObjectIndex(mObjects.GetLength());
        mObjects.Add(object);

        // if it's a node, add it to the nodes array as well
        if (object->GetBaseType() == AnimGraphNode::BASETYPE_ID)
        {
            AnimGraphNode* node = static_cast<AnimGraphNode*>(object);
            node->SetNodeIndex(mNodes.GetLength());
            mNodes.Add(node);

            // keep a list of motion nodes
            if (node->GetType() == AnimGraphMotionNode::TYPE_ID)
            {
                mMotionNodes.Add(static_cast<AnimGraphMotionNode*>(node));
            }
        }

        // create a unique data for this added object in the animgraph instances as well
        const uint32 numInstances = mAnimGraphInstances.GetLength();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            mAnimGraphInstances[i]->AddUniqueObjectData();
        }
    }


    // remove an object
    void AnimGraph::RemoveObject(AnimGraphObject* object)
    {
        MCore::LockGuard lock(mLock);

        const uint32 objectIndex = object->GetObjectIndex();

        // remove all internal attributes for this object
        object->RemoveInternalAttributesForAllInstances();

        // decrease the indices of all objects that have an index after this node
        const uint32 numObjects = mObjects.GetLength();
        for (uint32 i = objectIndex + 1; i < numObjects; ++i)
        {
            AnimGraphObject* curObject = mObjects[i];
            MCORE_ASSERT(i == curObject->GetObjectIndex());
            curObject->SetObjectIndex(i - 1);
        }

        // remove the object from the array
        mObjects.Remove(objectIndex);

        // remove it from the nodes array if it is a node
        if (object->GetBaseType() == AnimGraphNode::BASETYPE_ID)
        {
            AnimGraphNode* node = static_cast<AnimGraphNode*>(object);
            const uint32 nodeIndex = node->GetNodeIndex();

            const uint32 numNodes = mNodes.GetLength();
            for (uint32 i = nodeIndex + 1; i < numNodes; ++i)
            {
                AnimGraphNode* curNode = mNodes[i];
                MCORE_ASSERT(i == curNode->GetNodeIndex());
                curNode->SetNodeIndex(i - 1);
            }

            // remove the object from the array
            mNodes.Remove(nodeIndex);

            if (node->GetType() == AnimGraphMotionNode::TYPE_ID)
            {
                mMotionNodes.RemoveByValue(static_cast<AnimGraphMotionNode*>(node));
            }
        }
    }


    // reserve space for a given amount of objects
    void AnimGraph::ReserveNumObjects(uint32 numObjects)
    {
        mObjects.Reserve(numObjects);
    }


    // reserve space for a given amount of nodes
    void AnimGraph::ReserveNumNodes(uint32 numNodes)
    {
        mNodes.Reserve(numNodes);
    }


    // reserve memory for the animgraph instance array
    void AnimGraph::ReserveNumAnimGraphInstances(uint32 numInstances)
    {
        mAnimGraphInstances.Reserve(numInstances);
    }


    // register an animgraph instance
    void AnimGraph::AddAnimGraphInstance(AnimGraphInstance* animGraphInstance)
    {
        MCore::LockGuard lock(mLock);
        mAnimGraphInstances.Add(animGraphInstance);
    }


    // remove an animgraph instance
    void AnimGraph::RemoveAnimGraphInstance(AnimGraphInstance* animGraphInstance)
    {
        MCore::LockGuard lock(mLock);
        mAnimGraphInstances.RemoveByValue(animGraphInstance);
    }


    // clone this animgraph
    AnimGraph* AnimGraph::Clone(const char* name)
    {
        AnimGraph* result = new AnimGraph(name);

        // copy simple values
        result->mRetarget           = mRetarget;
        result->mAutoUnregister     = mAutoUnregister;
        result->mFileName           = mFileName;
        result->mDescription        = mDescription;

        // clone the parameter settings
        result->mParameterSettings->CopyFrom(*mParameterSettings);

        // clone all objects
        if (mRootStateMachine)
        {
            mRootStateMachine->RecursiveClone(result, nullptr);
            mRootStateMachine->RecursiveClonePostProcess(result->GetRootStateMachine());
            //MCore::LogInfo("orgObj=%d  clonedObj=%d", mObjects.GetLength(), result->mObjects.GetLength());
        }

        // clone node groups
        const uint32 numNodeGroups = mNodeGroups.GetLength();
        result->mNodeGroups.Resize(numNodeGroups);
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            AnimGraphNodeGroup* resultGroup = AnimGraphNodeGroup::Create();
            resultGroup->InitFrom(*mNodeGroups[i]);
            result->mNodeGroups[i] = resultGroup;
        }

        // clone parameter groups
        const uint32 numParameterGroups = mParameterGroups.GetLength();
        result->mParameterGroups.Resize(numParameterGroups);
        for (uint32 i = 0; i < numParameterGroups; ++i)
        {
            AnimGraphParameterGroup* resultGroup = AnimGraphParameterGroup::Create("");
            resultGroup->InitFrom(*mParameterGroups[i]);
            result->mParameterGroups[i] = resultGroup;
        }

        // clone game controller settings
        if (mGameControllerSettings)
        {
            if (result->mGameControllerSettings)
            {
                result->mGameControllerSettings->Destroy();
            }

            result->mGameControllerSettings = mGameControllerSettings->Clone();
        }

        // we modified the parameter values, handle that
        result->RecursiveUpdateAttributes();

        return result;
    }


    // decrease internal attribute indices by one, for values higher than the given parameter
    void AnimGraph::DecreaseInternalAttributeIndices(uint32 decreaseEverythingHigherThan)
    {
        const uint32 numObjects = mObjects.GetLength();
        for (uint32 i = 0; i < numObjects; ++i)
        {
            mObjects[i]->DecreaseInternalAttributeIndices(decreaseEverythingHigherThan);
        }
    }


    const char* AnimGraph::GetName() const
    {
        return mName.AsChar();
    }


    const char* AnimGraph::GetFileName() const
    {
        return mFileName.AsChar();
    }


    const MCore::String& AnimGraph::GetNameString() const
    {
        return mName;
    }


    const MCore::String& AnimGraph::GetFileNameString() const
    {
        return mFileName;
    }


    void AnimGraph::SetName(const char* name)
    {
        mName = name;
    }


    void AnimGraph::SetFileName(const char* fileName)
    {
        mFileName = fileName;
    }


    AnimGraphStateMachine* AnimGraph::GetRootStateMachine() const
    {
        return mRootStateMachine;
    }


    void AnimGraph::SetDescription(const char* description)
    {
        mDescription = description;
    }


    const char* AnimGraph::GetDescription() const
    {
        return mDescription.AsChar();
    }


    uint32 AnimGraph::GetID() const
    {
        return mID;
    }


    void AnimGraph::SetID(uint32 id)
    {
        mID = id;
    }


    bool AnimGraph::GetDirtyFlag() const
    {
        return mDirtyFlag;
    }


    AnimGraphGameControllerSettings* AnimGraph::GetGameControllerSettings() const
    {
        return mGameControllerSettings;
    }


    bool AnimGraph::GetRetargetingEnabled() const
    {
        return mRetarget;
    }


    void AnimGraph::SetRetargetingEnabled(bool enabled)
    {
        mRetarget = enabled;
    }


    MCore::AttributeSet* AnimGraph::GetAttributeSet() const
    {
        return mAttributeSet;
    }


    void AnimGraph::Lock()
    {
        mLock.Lock();
    }


    void AnimGraph::Unlock()
    {
        mLock.Unlock();
    }


    void AnimGraph::SetUnitType(MCore::Distance::EUnitType unitType)
    {
        mUnitType = unitType;
    }


    MCore::Distance::EUnitType AnimGraph::GetUnitType() const
    {
        return mUnitType;
    }



    void AnimGraph::SetFileUnitType(MCore::Distance::EUnitType unitType)
    {
        mFileUnitType = unitType;
    }


    MCore::Distance::EUnitType AnimGraph::GetFileUnitType() const
    {
        return mFileUnitType;
    }


    // scale everything to the given unit type
    void AnimGraph::ScaleToUnitType(MCore::Distance::EUnitType targetUnitType)
    {
        if (mUnitType == targetUnitType)
        {
            return;
        }

        // calculate the scale factor and scale
        const float scaleFactor = static_cast<float>(MCore::Distance::GetConversionFactor(mUnitType, targetUnitType));
        Scale(scaleFactor);

        // update the unit type
        mUnitType = targetUnitType;
    }


    // scale
    void AnimGraph::Scale(float scaleFactor)
    {
        // convert parameters
        const uint32 numParameters = GetNumParameters();
        for (uint32 i = 0; i < numParameters; ++i)
        {
            MCore::AttributeSettings* param = GetParameter(i);
            if (param->GetCanScale())
            {
                param->Scale(scaleFactor);

                // scale all animgraph instance values
                const uint32 numInstances = GetNumAnimGraphInstances();
                for (uint32 b = 0; b < numInstances; ++b)
                {
                    AnimGraphInstance* animGraphInstance = GetAnimGraphInstance(b);
                    animGraphInstance->GetParameterValue(i)->Scale(scaleFactor);
                }
            }
        }

        // convert all object internals if needed
        const uint32 numObjects = GetNumObjects();
        for (uint32 i = 0; i < numObjects; ++i)
        {
            GetObject(i)->Scale(scaleFactor);
        }

        // trigger the event
        GetEventManager().OnScaleAnimGraphData(this, scaleFactor);
    }
} // namespace EMotionFX

