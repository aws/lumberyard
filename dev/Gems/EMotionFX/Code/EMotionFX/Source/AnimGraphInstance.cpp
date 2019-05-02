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
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphPosePool.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/Attachment.h>
#include <EMotionFX/Source/BlendTreeMotionFrameNode.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <EMotionFX/Source/ThreadData.h>
#include <EMotionFX/Source/TransformData.h>
#include <AzCore/std/smart_ptr/make_shared.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphInstance, AnimGraphInstanceAllocator, 0)

    // constructor
    AnimGraphInstance::AnimGraphInstance(AnimGraph* animGraph, ActorInstance* actorInstance, MotionSet* motionSet, const InitSettings* initSettings)
        : BaseObject()
    {
        // register at the animgraph
        animGraph->AddAnimGraphInstance(this);
        animGraph->Lock();

        mAnimGraph              = animGraph;
        mActorInstance          = actorInstance;
        m_parentAnimGraphInstance = nullptr;
        mMotionSet              = motionSet;
        mAutoUnregister         = true;
        mEnableVisualization    = true;
        mRetarget               = animGraph->GetRetargetingEnabled();
        mVisualizeScale         = 1.0f;

#if defined(EMFX_DEVELOPMENT_BUILD)
        mIsOwnedByRuntime       = false;
#endif // EMFX_DEVELOPMENT_BUILD

        if (initSettings)
        {
            mInitSettings = *initSettings;
        }

        mParamValues.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_INSTANCE);
        mObjectFlags.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_INSTANCE);
        m_eventHandlersByEventType.resize(EVENT_TYPE_ANIM_GRAPH_INSTANCE_LAST_EVENT - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT + 1);

        // init the internal attributes (create them)
        InitInternalAttributes();

        // prealloc the unique data array (doesn't create the actual unique data objects yet though)
        InitUniqueDatas();

        // automatically register the anim graph instance
        GetAnimGraphManager().AddAnimGraphInstance(this);

        // create the parameter value objects
        CreateParameterValues();

        // recursively create the unique datas for all nodes
        mAnimGraph->GetRootStateMachine()->RecursiveOnUpdateUniqueData(this);

        // start the state machines at the entry state
        Start();

        mAnimGraph->Unlock();
        GetEventManager().OnCreateAnimGraphInstance(this);
    }


    // destructor
    AnimGraphInstance::~AnimGraphInstance()
    {
        for (AnimGraphInstance* childInstance : m_childAnimGraphInstances)
        {
            childInstance->m_parentAnimGraphInstance = nullptr;
        }
        m_childAnimGraphInstances.clear();
        SetParentAnimGraphInstance(nullptr);

        if (m_parentAnimGraphInstance)
        {
            m_parentAnimGraphInstance->RemoveChildAnimGraphInstance(this);
        }

        GetEventManager().OnDeleteAnimGraphInstance(this);

        // automatically unregister the anim graph instance
        if (mAutoUnregister)
        {
            GetAnimGraphManager().RemoveAnimGraphInstance(this, false);
        }

        // Get rid of the unique data for all anim graph objects.
        for (AnimGraphObjectData* uniqueData : m_uniqueDatas)
        {
            if (uniqueData)
            {
                uniqueData->Destroy();
            }
        }
        m_uniqueDatas.clear();

        RemoveAllParameters(true);
        RemoveAllEventHandlers();

        // remove all the internal attributes (from node ports etc)
        RemoveAllInternalAttributes();

        // Remove all master graph
        for (AnimGraphInstance* servant : m_servantGraphs)
        {
            servant->RemoveMasterGraph(this);
        }
        m_servantGraphs.clear();

        for (AnimGraphInstance* master : m_masterGraphs)
        {
            master->RemoveServantGraph(this, false);
        }
        m_masterGraphs.clear();

        // unregister from the animgraph
        mAnimGraph->RemoveAnimGraphInstance(this);
    }


    // create an animgraph instance
    AnimGraphInstance* AnimGraphInstance::Create(AnimGraph* animGraph, ActorInstance* actorInstance, MotionSet* motionSet, const InitSettings* initSettings)
    {
        return aznew AnimGraphInstance(animGraph, actorInstance, motionSet, initSettings);
    }


    // remove all parameter values
    void AnimGraphInstance::RemoveAllParameters(bool delFromMem)
    {
        if (delFromMem)
        {
            const uint32 numParams = mParamValues.GetLength();
            for (uint32 i = 0; i < numParams; ++i)
            {
                if (mParamValues[i])
                {
                    delete mParamValues[i];
                }
            }
        }

        mParamValues.Clear();
    }


    // remove all internal attributes
    void AnimGraphInstance::RemoveAllInternalAttributes()
    {
        MCore::LockGuard lock(mMutex);

        for (MCore::Attribute* internalAttribute : m_internalAttributes)
        {
            if (internalAttribute)
            {
                delete internalAttribute;
            }
        }
    }


    uint32 AnimGraphInstance::AddInternalAttribute(MCore::Attribute* attribute)
    {
        MCore::LockGuard lock(mMutex);

        m_internalAttributes.emplace_back(attribute);
        return static_cast<uint32>(m_internalAttributes.size() - 1);
    }


    size_t AnimGraphInstance::GetNumInternalAttributes() const
    {
        return m_internalAttributes.size();
    }


    MCore::Attribute* AnimGraphInstance::GetInternalAttribute(size_t attribIndex) const
    {
        return m_internalAttributes[attribIndex];
    }


    void AnimGraphInstance::ReserveInternalAttributes(size_t totalNumInternalAttributes)
    {
        MCore::LockGuard lock(mMutex);
        m_internalAttributes.reserve(totalNumInternalAttributes);
    }


    void AnimGraphInstance::RemoveInternalAttribute(size_t index, bool delFromMem)
    {
        MCore::LockGuard lock(mMutex);
        if (delFromMem)
        {
            MCore::Attribute* internalAttribute = m_internalAttributes[index];
            if (internalAttribute)
            {
                delete internalAttribute;
            }
        }

        m_internalAttributes.erase(m_internalAttributes.begin() + index);
    }


    // output the results into the internal pose object
    void AnimGraphInstance::Output(Pose* outputPose, bool autoFreeAllPoses)
    {
        // reset max used
        const uint32 threadIndex = mActorInstance->GetThreadIndex();
        AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(threadIndex)->GetPosePool();
        posePool.ResetMaxUsedPoses();

        // calculate the anim graph output
        AnimGraphNode* rootNode = GetRootNode();

        // calculate the output of the state machine
        rootNode->PerformOutput(this);

        // update the output pose
        if (outputPose)
        {
            *outputPose = rootNode->GetMainOutputPose(this)->GetPose();
        }

        // decrease pose ref count for the root
        rootNode->DecreaseRef(this);

        //MCore::LogInfo("------poses   used = %d/%d (max=%d)----------", GetEMotionFX().GetThreadData(0)->GetPosePool().GetNumUsedPoses(), GetEMotionFX().GetThreadData(0)->GetPosePool().GetNumPoses(), GetEMotionFX().GetThreadData(0)->GetPosePool().GetNumMaxUsedPoses());
        //MCore::LogInfo("------refData used = %d/%d (max=%d)----------", GetEMotionFX().GetThreadData(0)->GetRefCountedDataPool().GetNumUsedItems(), GetEMotionFX().GetThreadData(0)->GetRefCountedDataPool().GetNumItems(), GetEMotionFX().GetThreadData(0)->GetRefCountedDataPool().GetNumMaxUsedItems());
        //MCORE_ASSERT(GetEMotionFX().GetThreadData(0).GetPosePool().GetNumUsedPoses() == 0);
        if (autoFreeAllPoses)
        {
            // Temp solution: In the AnimGraphStateMachine, there's a possibility that certain nodes get ref count increased, but never got decreased. This would result in some dangling
            // poses in those node's output ports. If somehow we are accessing them later (this could be another bug as well - in blendNNode e.g, we are freeing all the in comming port
            // regardless of whether they has gone through the output step), we will release such dangling pointer, which would cause random issue / crash later.
            // For now, we free all poses and clean all the ports.
            for (MCore::Attribute* attribute : m_internalAttributes)
            {
                if (attribute->GetType() == AttributePose::TYPE_ID)
                {
                    AttributePose* attributePose = static_cast<AttributePose*>(attribute);
                    attributePose->SetValue(nullptr);
                }
            }
            posePool.FreeAllPoses();
        }

        // Gather active state. Must be done in output function.
        if (mSnapshot && mSnapshot->IsNetworkAuthoritative())
        {
            mSnapshot->CollectActiveNodes(*this);
            mSnapshot->CollectMotionNodePlaytimes(*this);
        }
    }



    // resize the number of parameters
    void AnimGraphInstance::CreateParameterValues()
    {
        RemoveAllParameters(true);

        const ValueParameterVector& valueParameters = mAnimGraph->RecursivelyGetValueParameters();
        mParamValues.Resize(static_cast<uint32>(valueParameters.size()));

        // init the values
        const uint32 numParams = mParamValues.GetLength();
        for (uint32 i = 0; i < numParams; ++i)
        {
            mParamValues[i] = valueParameters[i]->ConstructDefaultValueAsAttribute();
        }
    }


    // add the missing parameters that the anim graph has to this anim graph instance
    void AnimGraphInstance::AddMissingParameterValues()
    {
        // check how many parameters we need to add
        const ValueParameterVector& valueParameters = mAnimGraph->RecursivelyGetValueParameters();
        const int32 numToAdd = static_cast<uint32>(valueParameters.size()) - mParamValues.GetLength();
        if (numToAdd <= 0)
        {
            return;
        }

        // make sure we have the right space pre-allocated
        mParamValues.Reserve(static_cast<uint32>(valueParameters.size()));

        // add the remaining parameters
        const uint32 startIndex = mParamValues.GetLength();
        for (int32 i = 0; i < numToAdd; ++i)
        {
            const uint32 index = startIndex + i;
            mParamValues.AddEmpty();
            mParamValues.GetLast() = valueParameters[index]->ConstructDefaultValueAsAttribute();
        }
    }


    // remove a parameter value
    void AnimGraphInstance::RemoveParameterValue(uint32 index, bool delFromMem)
    {
        if (delFromMem)
        {
            if (mParamValues[index])
            {
                delete mParamValues[index];
            }
        }

        mParamValues.Remove(index);
    }


    // reinitialize the parameter
    void AnimGraphInstance::ReInitParameterValue(uint32 index)
    {
        if (mParamValues[index])
        {
            delete mParamValues[index];
        }

        mParamValues[index] = mAnimGraph->FindValueParameter(index)->ConstructDefaultValueAsAttribute();
    }


    void AnimGraphInstance::ReInitParameterValues()
    {
        const AZ::u32 parameterValueCount = mParamValues.GetLength();
        for (AZ::u32 i = 0; i < parameterValueCount; ++i)
        {
            ReInitParameterValue(i);
        }
    }


    // switch to another state using a state name
    bool AnimGraphInstance::SwitchToState(const char* stateName)
    {
        // now try to find the state
        AnimGraphNode* state = mAnimGraph->RecursiveFindNodeByName(stateName);
        if (state == nullptr)
        {
            return false;
        }

        // check if the parent node is a state machine or not
        AnimGraphNode* parentNode = state->GetParentNode();
        if (parentNode == nullptr) // in this case the stateName node is a state machine itself
        {
            return false;
        }

        // if it's not a state machine, then our node is not a state we can switch to
        if (azrtti_typeid(parentNode) != azrtti_typeid<AnimGraphStateMachine>())
        {
            return false;
        }

        // get the state machine object
        AnimGraphStateMachine* machine = static_cast<AnimGraphStateMachine*>(parentNode);

        // only allow switching to a new state when we are currently not transitioning
        if (machine->GetIsTransitioning(this))
        {
            return false;
        }

        // recurisvely make sure the parent state machines are currently active as well
        SwitchToState(parentNode->GetName());

        // now switch to the new state
        machine->SwitchToState(this, state);
        return true;
    }


    // checks if there is a transition from the current to the target node and starts a transition towards it, in case there is no transition between them the target node just gets activated
    bool AnimGraphInstance::TransitionToState(const char* stateName)
    {
        // now try to find the state
        AnimGraphNode* state = mAnimGraph->RecursiveFindNodeByName(stateName);
        if (state == nullptr)
        {
            return false;
        }

        // check if the parent node is a state machine or not
        AnimGraphNode* parentNode = state->GetParentNode();
        if (parentNode == nullptr) // in this case the stateName node is a state machine itself
        {
            return false;
        }

        // if it's not a state machine, then our node is not a state we can switch to
        if (azrtti_typeid(parentNode) != azrtti_typeid<AnimGraphStateMachine>())
        {
            return false;
        }

        // get the state machine object
        AnimGraphStateMachine* machine = static_cast<AnimGraphStateMachine*>(parentNode);

        // only allow switching to a new state when we are currently not transitioning
        if (machine->GetIsTransitioning(this))
        {
            return false;
        }

        // recurisvely make sure the parent state machines are currently active as well
        TransitionToState(parentNode->GetName());

        // now transit to the new state
        machine->TransitionToState(this, state);
        return true;
    }


    void AnimGraphInstance::RecursiveSwitchToEntryState(AnimGraphNode* node)
    {
        // check if the given node is a state machine
        if (azrtti_typeid(node) == azrtti_typeid<AnimGraphStateMachine>())
        {
            // type cast the node to a state machine
            AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(node);

            // switch to the entry state
            AnimGraphNode* entryState = stateMachine->GetEntryState();
            if (entryState)
            {
                stateMachine->SwitchToState(this, entryState);
                RecursiveSwitchToEntryState(entryState);
            }
        }
        else
        {
            // get the number of child nodes, iterate through them and call the function recursively in case we are dealing with a blend tree or another node
            const uint32 numChildNodes = node->GetNumChildNodes();
            for (uint32 i = 0; i < numChildNodes; ++i)
            {
                RecursiveSwitchToEntryState(node->GetChildNode(i));
            }
        }
    }


    // start the state machines at the entry state
    void AnimGraphInstance::Start()
    {
        RecursiveSwitchToEntryState(GetRootNode());
    }


    // reset all current states of all state machines recursively
    void AnimGraphInstance::RecursiveResetCurrentState(AnimGraphNode* node)
    {
        // check if the given node is a state machine
        if (azrtti_typeid(node) == azrtti_typeid<AnimGraphStateMachine>())
        {
            // type cast the node to a state machine
            AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(node);

            // reset the current state
            stateMachine->SwitchToState(this, nullptr);
        }

        // get the number of child nodes, iterate through them and call the function recursively
        const uint32 numChildNodes = node->GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            RecursiveResetCurrentState(node->GetChildNode(i));
        }
    }


    // stop the state machines and reset the current state to nullptr
    void AnimGraphInstance::Stop()
    {
        RecursiveResetCurrentState(GetRootNode());
    }


    // find the parameter value for a parameter with a given name
    MCore::Attribute* AnimGraphInstance::FindParameter(const AZStd::string& name) const
    {
        const AZ::Outcome<size_t> paramIndex = mAnimGraph->FindValueParameterIndexByName(name);
        if (!paramIndex.IsSuccess())
        {
            return nullptr;
        }

        return mParamValues[static_cast<uint32>(paramIndex.GetValue())];
    }


    // add the last anim graph parameter to this instance
    void AnimGraphInstance::AddParameterValue()
    {
        mParamValues.Add(nullptr);
        ReInitParameterValue(mParamValues.GetLength() - 1);
    }


    // add the parameter of the animgraph, at a given index
    void AnimGraphInstance::InsertParameterValue(uint32 index)
    {
        mParamValues.Insert(index, nullptr);
        ReInitParameterValue(index);
    }


    void AnimGraphInstance::ResetUniqueData()
    {
        GetRootNode()->RecursiveResetUniqueData(this);
    }


    void AnimGraphInstance::UpdateUniqueData()
    {
        GetRootNode()->RecursiveOnUpdateUniqueData(this);
    }


    // set a new motion set to the anim graph instance
    void AnimGraphInstance::SetMotionSet(MotionSet* motionSet)
    {
        // update the local motion set pointer
        mMotionSet = motionSet;

        // get the number of state machines, iterate through them and recursively call the callback
        GetRootNode()->RecursiveOnChangeMotionSet(this, motionSet);
    }


    // adjust the auto unregistering from the anim graph manager on delete
    void AnimGraphInstance::SetAutoUnregisterEnabled(bool enabled)
    {
        mAutoUnregister = enabled;
    }


    // do we auto unregister from the anim graph manager on delete?
    bool AnimGraphInstance::GetAutoUnregisterEnabled() const
    {
        return mAutoUnregister;
    }

    void AnimGraphInstance::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        mIsOwnedByRuntime = isOwnedByRuntime;
#endif
    }


    bool AnimGraphInstance::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return mIsOwnedByRuntime;
#else
        return true;
#endif
    }


    // find an actor instance based on a parent depth value
    ActorInstance* AnimGraphInstance::FindActorInstanceFromParentDepth(uint32 parentDepth) const
    {
        // start with the actor instance this anim graph instance is working on
        ActorInstance* curInstance = mActorInstance;
        if (parentDepth == 0)
        {
            return curInstance;
        }

        // repeat until we are at the root
        uint32 depth = 1;
        while (curInstance)
        {
            // get the attachment object
            Attachment* attachment = curInstance->GetSelfAttachment();

            // if this is the depth we are looking for
            if (depth == parentDepth)
            {
                if (attachment)
                {
                    return attachment->GetAttachToActorInstance();
                }
                else
                {
                    return nullptr;
                }
            }

            // traverse up the hierarchy
            if (attachment)
            {
                depth++;
                curInstance = attachment->GetAttachToActorInstance();
            }
            else
            {
                return nullptr;
            }
        }

        return nullptr;
    }


    void AnimGraphInstance::RegisterUniqueObjectData(AnimGraphObjectData* data)
    {
        m_uniqueDatas[data->GetObject()->GetObjectIndex()] = data;
    }


    void AnimGraphInstance::AddUniqueObjectData()
    {
        m_uniqueDatas.emplace_back(nullptr);
        mObjectFlags.Add(0);
    }


    // remove the given unique data object
    void AnimGraphInstance::RemoveUniqueObjectData(AnimGraphObjectData* uniqueData, bool delFromMem)
    {
        if (uniqueData == nullptr)
        {
            return;
        }

        const uint32 index = uniqueData->GetObject()->GetObjectIndex();
        if (delFromMem && m_uniqueDatas[index])
        {
            m_uniqueDatas[index]->Destroy();
        }

        m_uniqueDatas.erase(m_uniqueDatas.begin() + index);
        mObjectFlags.Remove(index);
    }


    void AnimGraphInstance::RemoveUniqueObjectData(size_t index, bool delFromMem)
    {
        AnimGraphObjectData* data = m_uniqueDatas[index];
        m_uniqueDatas.erase(m_uniqueDatas.begin() + index);
        mObjectFlags.Remove(static_cast<uint32>(index));
        if (delFromMem && data)
        {
            data->Destroy();
        }
    }


    // remove all object data
    void AnimGraphInstance::RemoveAllObjectData(bool delFromMem)
    {
        if (delFromMem)
        {
            for (AnimGraphObjectData* uniqueData : m_uniqueDatas)
            {
                if (uniqueData)
                {
                    uniqueData->Destroy();
                }
            }
        }

        m_uniqueDatas.clear();
        mObjectFlags.Clear();
    }


    // register event handler
    void AnimGraphInstance::AddEventHandler(AnimGraphInstanceEventHandler* eventHandler)
    {
        AZ_Assert(eventHandler, "Expected non-null event handler");
        for (const EventTypes eventType : eventHandler->GetHandledEventTypes())
        {
            AZ_Assert(AZStd::find(m_eventHandlersByEventType[eventType - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT].begin(), m_eventHandlersByEventType[eventType - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT].end(), eventHandler) == m_eventHandlersByEventType[eventType - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT].end(),
                "Event handler already added to manager");
            m_eventHandlersByEventType[eventType - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT].emplace_back(eventHandler);
        }
    }


    // unregister event handler
    void AnimGraphInstance::RemoveEventHandler(AnimGraphInstanceEventHandler* eventHandler)
    {
        for (const EventTypes eventType : eventHandler->GetHandledEventTypes())
        {
            EventHandlerVector& eventHandlers = m_eventHandlersByEventType[eventType - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT];
            eventHandlers.erase(AZStd::remove(eventHandlers.begin(), eventHandlers.end(), eventHandler), eventHandlers.end());
        }
    }


    // remove all event handlers
    void AnimGraphInstance::RemoveAllEventHandlers()
    {
#ifdef DEBUG
        for (const EventHandlerVector& eventHandlers : m_eventHandlersByEventType)
        {
            AZ_Assert(eventHandlers.empty(), "Expected all events to be removed");
        }
#endif
        m_eventHandlersByEventType.clear();
    }


    void AnimGraphInstance::OnStateEnter(AnimGraphNode* state)
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STATE_ENTER - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT];
        for (AnimGraphInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStateEnter(this, state);
        }
    }


    void AnimGraphInstance::OnStateEntering(AnimGraphNode* state)
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STATE_ENTERING - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT];
        for (AnimGraphInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStateEntering(this, state);
        }
    }


    void AnimGraphInstance::OnStateExit(AnimGraphNode* state)
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STATE_EXIT - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT];
        for (AnimGraphInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStateExit(this, state);
        }
    }


    void AnimGraphInstance::OnStateEnd(AnimGraphNode* state)
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STATE_END - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT];
        for (AnimGraphInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStateEnd(this, state);
        }
    }


    void AnimGraphInstance::OnStartTransition(AnimGraphStateTransition* transition)
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_START_TRANSITION - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT];
        for (AnimGraphInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStartTransition(this, transition);
        }
    }


    void AnimGraphInstance::OnEndTransition(AnimGraphStateTransition* transition)
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_END_TRANSITION - EVENT_TYPE_ANIM_GRAPH_INSTANCE_FIRST_EVENT];
        for (AnimGraphInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnEndTransition(this, transition);
        }
    }


    // init the hashmap
    void AnimGraphInstance::InitUniqueDatas()
    {
        const uint32 numObjects = mAnimGraph->GetNumObjects();
        m_uniqueDatas.resize(numObjects);
        mObjectFlags.Resize(numObjects);
        for (uint32 i = 0; i < numObjects; ++i)
        {
            m_uniqueDatas[i] = nullptr;
            mObjectFlags[i] = 0;
        }
    }


    // get the root node
    AnimGraphNode* AnimGraphInstance::GetRootNode() const
    {
        return mAnimGraph->GetRootStateMachine();
    }


    // apply motion extraction
    void AnimGraphInstance::ApplyMotionExtraction()
    {
        // perform motion extraction
        Transform trajectoryDelta;

        // get the motion extraction node, and if it hasn't been set, we can already quit
        Node* motionExtractNode = mActorInstance->GetActor()->GetMotionExtractionNode();
        if (motionExtractNode == nullptr)
        {
            trajectoryDelta.ZeroWithIdentityQuaternion();
            mActorInstance->SetTrajectoryDeltaTransform(trajectoryDelta);
            return;
        }

        // get the root node's trajectory delta
        AnimGraphRefCountedData* rootData = mAnimGraph->GetRootStateMachine()->FindUniqueNodeData(this)->GetRefCountedData();
        trajectoryDelta = rootData->GetTrajectoryDelta();

        // update the actor instance with the delta movement already
        mActorInstance->SetTrajectoryDeltaTransform(trajectoryDelta);
        mActorInstance->ApplyMotionExtractionDelta();
    }


    // synchronize all nodes, based on sync tracks etc
    void AnimGraphInstance::Update(float timePassedInSeconds)
    {
        // pass 0: (Optional, networking only) When this instance is shared between network, restore the instance using an animgraph snapshot.
        if (mSnapshot)
        {
            mSnapshot->Restore(*this);
        }

        // pass 1: update (bottom up), update motion timers etc
        // pass 2: topdown update (top down), syncing happens here (adjusts motion/node timers again)
        // pass 3: postupdate (bottom up), processing the motion events events and update motion extraction deltas
        // pass 4: output (bottom up), calculate all new bone transforms (the heavy thing to process) <--- not performed by this function but in AnimGraphInstance::Output()

        // reset the output is ready flags, so we return cached copies of the outputs, but refresh/recalculate them
        AnimGraphNode* rootNode = GetRootNode();

        ResetFlagsForAllObjects();

        if (GetEMotionFX().GetIsInEditorMode())
        {
            rootNode->RecursiveResetFlags(this, 0xffffffff);    // the 0xffffffff clears all flags
        }

        // reset all node pose ref counts
        const uint32 threadIndex = mActorInstance->GetThreadIndex();
        ResetPoseRefCountsForAllNodes();
        ResetRefDataRefCountsForAllNodes();
        GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool().ResetMaxUsedItems();

        // perform a bottom-up update, which updates the nodes, and sets their sync tracks, and play time etc
        rootNode->IncreasePoseRefCount(this);
        rootNode->IncreaseRefDataRefCount(this);
        rootNode->PerformUpdate(this, timePassedInSeconds);

        // perform a top-down update, starting from the root and going downwards to the leaf nodes
        AnimGraphNodeData* rootNodeUniqueData = rootNode->FindUniqueNodeData(this);
        rootNodeUniqueData->SetGlobalWeight(1.0f); // start with a global weight of 1 at the root
        rootNodeUniqueData->SetLocalWeight(1.0f); // start with a local weight of 1 at the root
        rootNode->PerformTopDownUpdate(this, timePassedInSeconds);

        // bottom up pass event buffers and update motion extraction deltas
        rootNode->PerformPostUpdate(this, timePassedInSeconds);

        //-------------------------------------

        // apply motion extraction
        ApplyMotionExtraction();

        // store a copy of the root's event buffer
        mEventBuffer = rootNodeUniqueData->GetRefCountedData()->GetEventBuffer();

        // trigger the events inside the root node's buffer
        OutputEvents();

        rootNode->DecreaseRefDataRef(this);

        //MCore::LogInfo("------bef refData used = %d/%d (max=%d)----------", GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool().GetNumUsedItems(), GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool().GetNumItems(), GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool().GetNumMaxUsedItems());

        // release any left over ref data
        AnimGraphRefCountedDataPool& refDataPool = GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool();
        const uint32 numNodes = mAnimGraph->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            AnimGraphNodeData* nodeData = static_cast<AnimGraphNodeData*>(m_uniqueDatas[mAnimGraph->GetNode(i)->GetObjectIndex()]);
            AnimGraphRefCountedData* refData = nodeData->GetRefCountedData();
            if (refData)
            {
                refDataPool.Free(refData);
                nodeData->SetRefCountedData(nullptr);
            }
        }

        //MCore::LogInfo("------refData used = %d/%d (max=%d)----------", GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool().GetNumUsedItems(), GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool().GetNumItems(), GetEMotionFX().GetThreadData(threadIndex)->GetRefCountedDataPool().GetNumMaxUsedItems());
    }


    // recursively reset flags
    void AnimGraphInstance::RecursiveResetFlags(uint32 flagsToDisable)
    {
        mAnimGraph->GetRootStateMachine()->RecursiveResetFlags(this, flagsToDisable);
    }


    // reset all node flags
    void AnimGraphInstance::ResetFlagsForAllObjects(uint32 flagsToDisable)
    {
        const uint32 numObjects = mObjectFlags.GetLength();
        for (uint32 i = 0; i < numObjects; ++i)
        {
            mObjectFlags[i] &= ~flagsToDisable;
        }
    }


    // reset all node pose ref counts
    void AnimGraphInstance::ResetPoseRefCountsForAllNodes()
    {
        const uint32 numNodes = mAnimGraph->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mAnimGraph->GetNode(i)->ResetPoseRefCount(this);
        }
    }


    // reset all node pose ref counts
    void AnimGraphInstance::ResetRefDataRefCountsForAllNodes()
    {
        const uint32 numNodes = mAnimGraph->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mAnimGraph->GetNode(i)->ResetRefDataRefCount(this);
        }
    }


    // reset all node flags
    void AnimGraphInstance::ResetFlagsForAllObjects()
    {
        MCore::MemSet(mObjectFlags.GetPtr(), 0, sizeof(uint32) * mObjectFlags.GetLength());

        for (AnimGraphInstance* childInstance : m_childAnimGraphInstances)
        {
            childInstance->ResetFlagsForAllObjects();
        }
    }


    // reset flags for all nodes
    void AnimGraphInstance::ResetFlagsForAllNodes(uint32 flagsToDisable)
    {
        const uint32 numNodes = mAnimGraph->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            AnimGraphNode* node = mAnimGraph->GetNode(i);
            mObjectFlags[node->GetObjectIndex()] &= ~flagsToDisable;

            if (GetEMotionFX().GetIsInEditorMode())
            {
                // reset all connections
                const uint32 numConnections = node->GetNumConnections();
                for (uint32 c = 0; c < numConnections; ++c)
                {
                    node->GetConnection(c)->SetIsVisited(false);
                }
            }
        }
    }


    // output the events
    void AnimGraphInstance::OutputEvents()
    {
        AnimGraphNode* rootNode = GetRootNode();
        AnimGraphRefCountedData* rootData = rootNode->FindUniqueNodeData(this)->GetRefCountedData();
        AnimGraphEventBuffer& eventBuffer = rootData->GetEventBuffer();
        eventBuffer.UpdateWeights(this);
        eventBuffer.TriggerEvents();
        //eventBuffer.Log();
    }


    // recursively collect all active anim graph nodes
    void AnimGraphInstance::CollectActiveAnimGraphNodes(MCore::Array<AnimGraphNode*>* outNodes, const AZ::TypeId& nodeType)
    {
        outNodes->Clear(false);
        mAnimGraph->GetRootStateMachine()->RecursiveCollectActiveNodes(this, outNodes, nodeType);
    }


    void AnimGraphInstance::CollectActiveNetTimeSyncNodes(AZStd::vector<AnimGraphNode*>* outNodes)
    {
        outNodes->clear();
        mAnimGraph->GetRootStateMachine()->RecursiveCollectActiveNetTimeSyncNodes(this, outNodes);
    }


    // find the unique node data
    AnimGraphNodeData* AnimGraphInstance::FindUniqueNodeData(const AnimGraphNode* node) const
    {
        AnimGraphObjectData* result = m_uniqueDatas[node->GetObjectIndex()];
        //MCORE_ASSERT(result->GetObject()->GetBaseType() == AnimGraphNode::BASETYPE_ID);
        return reinterpret_cast<AnimGraphNodeData*>(result);
    }

    // find the parameter index
    AZ::Outcome<size_t> AnimGraphInstance::FindParameterIndex(const AZStd::string& name) const
    {
        return mAnimGraph->FindValueParameterIndexByName(name);
    }


    // init all internal attributes
    void AnimGraphInstance::InitInternalAttributes()
    {
        const uint32 numNodes = mAnimGraph->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mAnimGraph->GetNode(i)->InitInternalAttributes(this);
        }
    }


    void AnimGraphInstance::SetVisualizeScale(float scale)
    {
        mVisualizeScale = scale;
    }


    float AnimGraphInstance::GetVisualizeScale() const
    {
        return mVisualizeScale;
    }


    void AnimGraphInstance::SetVisualizationEnabled(bool enabled)
    {
        mEnableVisualization = enabled;
    }


    bool AnimGraphInstance::GetVisualizationEnabled() const
    {
        return mEnableVisualization;
    }


    bool AnimGraphInstance::GetRetargetingEnabled() const
    {
        return mRetarget;
    }


    void AnimGraphInstance::SetRetargetingEnabled(bool enabled)
    {
        mRetarget = enabled;
    }


    void AnimGraphInstance::SetUniqueObjectData(size_t index, AnimGraphObjectData* data)
    {
        m_uniqueDatas[index] = data;
    }


    const AnimGraphInstance::InitSettings& AnimGraphInstance::GetInitSettings() const
    {
        return mInitSettings;
    }


    const AnimGraphEventBuffer& AnimGraphInstance::GetEventBuffer() const
    {
        return mEventBuffer;
    }
   

    void AnimGraphInstance::AddServantGraph(AnimGraphInstance* servant, bool registerMasterInsideServant)
    {
        const auto& it = AZStd::find(m_servantGraphs.begin(), m_servantGraphs.end(), servant);
        if (it == m_servantGraphs.end())
        {
            m_servantGraphs.emplace_back(servant);
        }

        if (registerMasterInsideServant)
        {
            servant->AddMasterGraph(this);
        }
    }

    void AnimGraphInstance::RemoveServantGraph(AnimGraphInstance* servant, bool removeMasterFromServant)
    {
        const auto& it = AZStd::find(m_servantGraphs.begin(), m_servantGraphs.end(), servant);
        if (it != m_servantGraphs.end())
        {
            m_servantGraphs.erase(it);
        }

        if (removeMasterFromServant)
        {
            servant->RemoveMasterGraph(this);
        }
    }

    AZStd::vector<AnimGraphInstance*>& AnimGraphInstance::GetServantGraphs()
    {
        return m_servantGraphs;
    }

    void AnimGraphInstance::AddMasterGraph(AnimGraphInstance* master)
    {
        const auto& it = AZStd::find(m_masterGraphs.begin(), m_masterGraphs.end(), master);
        if (it == m_masterGraphs.end())
        {
            m_masterGraphs.emplace_back(master);
        }
    }

    void AnimGraphInstance::RemoveMasterGraph(AnimGraphInstance* master)
    {
        const auto& it = AZStd::find(m_masterGraphs.begin(), m_masterGraphs.end(), master);
        if (it != m_masterGraphs.end())
        {
            m_masterGraphs.erase(it);
        }
    }

    AZStd::vector<AnimGraphInstance*>& AnimGraphInstance::GetMasterGraphs()
    {
        return m_masterGraphs;
    }

    void AnimGraphInstance::CreateSnapshot(bool authoritative)
    {
        if (mSnapshot)
        {
            AZ_Error("EMotionFX", false, "Snapshot already created for this animgraph instance.");
            return;
        }
        mSnapshot = AZStd::make_shared<AnimGraphSnapshot>(*this, authoritative);
    }

    void AnimGraphInstance::SetSnapshotSerializer(AZStd::shared_ptr<Network::AnimGraphSnapshotSerializer> serializer)
    {
        if (!mSnapshot)
        {
            AZ_Error("EMotionFX", false, "Snapshot should be created first.");
            return;
        }
        mSnapshot->SetSnapshotSerializer(serializer);
    }

    void AnimGraphInstance::SetSnapshotChunkSerializer(AZStd::shared_ptr<Network::AnimGraphSnapshotChunkSerializer> serializer)
    {
        if (!mSnapshot)
        {
            AZ_Error("EMotionFX", false, "Snapshot should be created first.");
            return;
        }
        mSnapshot->SetSnapshotChunkSerializer(serializer);
    }

    void AnimGraphInstance::OnNetworkConnected()
    {
        if (!mSnapshot)
        {
            AZ_Error("EMotionFX", false, "Snapshot should be created first.");
            return;
        }
         mSnapshot->OnNetworkConnected(*this);
    }

    void AnimGraphInstance::OnNetworkParamUpdate(const AttributeContainer& parameters)
    {
        if (!mSnapshot)
        {
            AZ_Error("EMotionFX", false, "Snapshot should be created first.");
            return;
        }
        mSnapshot->SetParameters(parameters);
    }

    void AnimGraphInstance::OnNetworkActiveNodesUpdate(const AZStd::vector<AZ::u32>& activeNodes)
    {
        if (!mSnapshot)
        {
            AZ_Error("EMotionFX", false, "Snapshot should be created first.");
            return;
        }
        mSnapshot->SetActiveNodes(activeNodes);
    }

    void AnimGraphInstance::OnNetworkMotionNodePlaytimesUpdate(const MotionNodePlaytimeContainer& motionNodePlaytimes)
    {
        if (!mSnapshot)
        {
            AZ_Error("EMotionFX", false, "Snapshot should be created first.");
            return;
        }
        mSnapshot->SetMotionNodePlaytimes(motionNodePlaytimes);
    }

    bool AnimGraphInstance::GetParameterValueAsFloat(uint32 paramIndex, float* outValue)
    {
        MCore::AttributeFloat* floatAttribute = GetParameterValueChecked<MCore::AttributeFloat>(paramIndex);
        if (floatAttribute)
        {
            *outValue = floatAttribute->GetValue();
            return true;
        }

        MCore::AttributeInt32* intAttribute = GetParameterValueChecked<MCore::AttributeInt32>(paramIndex);
        if (intAttribute)
        {
            *outValue = static_cast<float>(intAttribute->GetValue());
            return true;
        }

        MCore::AttributeBool* boolAttribute = GetParameterValueChecked<MCore::AttributeBool>(paramIndex);
        if (boolAttribute)
        {
            *outValue = static_cast<float>(boolAttribute->GetValue());
            return true;
        }

        return false;
    }

    bool AnimGraphInstance::GetParameterValueAsBool(uint32 paramIndex, bool* outValue)
    {
        float floatValue;
        if (GetParameterValueAsFloat(paramIndex, &floatValue))
        {
            *outValue = (MCore::Math::IsFloatZero(floatValue) == false);
            return true;
        }

        return false;
    }


    bool AnimGraphInstance::GetParameterValueAsInt(uint32 paramIndex, int32* outValue)
    {
        float floatValue;
        if (GetParameterValueAsFloat(paramIndex, &floatValue))
        {
            *outValue = static_cast<int32>(floatValue);
            return true;
        }

        return false;
    }


    bool AnimGraphInstance::GetVector2ParameterValue(uint32 paramIndex, AZ::Vector2* outValue)
    {
        MCore::AttributeVector2* param = GetParameterValueChecked<MCore::AttributeVector2>(paramIndex);
        if (param)
        {
            *outValue = param->GetValue();
            return true;
        }

        return false;
    }


    bool AnimGraphInstance::GetVector3ParameterValue(uint32 paramIndex, AZ::Vector3* outValue)
    {
        MCore::AttributeVector3* param = GetParameterValueChecked<MCore::AttributeVector3>(paramIndex);
        if (param)
        {
            *outValue = AZ::Vector3(param->GetValue());
            return true;
        }

        return false;
    }


    bool AnimGraphInstance::GetVector4ParameterValue(uint32 paramIndex, AZ::Vector4* outValue)
    {
        MCore::AttributeVector4* param = GetParameterValueChecked<MCore::AttributeVector4>(paramIndex);
        if (param)
        {
            *outValue = param->GetValue();
            return true;
        }

        return false;
    }


    bool AnimGraphInstance::GetRotationParameterValue(uint32 paramIndex, MCore::Quaternion* outRotation)
    {
        MCore::AttributeQuaternion* param = GetParameterValueChecked<MCore::AttributeQuaternion>(paramIndex);
        if (param)
        {
            *outRotation = param->GetValue();
            return true;
        }

        return false;
    }

    void AnimGraphInstance::SetParentAnimGraphInstance(AnimGraphInstance* parentAnimGraphInstance)
    { 
        if (m_parentAnimGraphInstance)
        {
            m_parentAnimGraphInstance->m_childAnimGraphInstances.erase(
                AZStd::remove(m_parentAnimGraphInstance->m_childAnimGraphInstances.begin(), m_parentAnimGraphInstance->m_childAnimGraphInstances.end(), this),
                m_parentAnimGraphInstance->m_childAnimGraphInstances.end());
        }

        m_parentAnimGraphInstance = parentAnimGraphInstance; 
        
        // Add myself to the parent
        if (parentAnimGraphInstance)
        {
            parentAnimGraphInstance->m_childAnimGraphInstances.emplace_back(this);
        }
    }


    void AnimGraphInstance::RemoveChildAnimGraphInstance(AnimGraphInstance* childAnimGraphInstance)
    {
        m_childAnimGraphInstances.erase(AZStd::remove(m_childAnimGraphInstances.begin(), m_childAnimGraphInstances.end(), childAnimGraphInstance), m_childAnimGraphInstances.end());
    }


    bool AnimGraphInstance::GetParameterValueAsFloat(const char* paramName, float* outValue)
    {
        const AZ::Outcome<size_t> index = FindParameterIndex(paramName);
        if (!index.IsSuccess())
        {
            return false;
        }

        return GetParameterValueAsFloat(static_cast<uint32>(index.GetValue()), outValue);
    }


    bool AnimGraphInstance::GetParameterValueAsBool(const char* paramName, bool* outValue)
    {
        const AZ::Outcome<size_t> index = FindParameterIndex(paramName);
        if (!index.IsSuccess())
        {
            return false;
        }

        return GetParameterValueAsBool(static_cast<uint32>(index.GetValue()), outValue);
    }


    bool AnimGraphInstance::GetParameterValueAsInt(const char* paramName, int32* outValue)
    {
        const AZ::Outcome<size_t> index = FindParameterIndex(paramName);
        if (!index.IsSuccess())
        {
            return false;
        }

        return GetParameterValueAsInt(static_cast<uint32>(index.GetValue()), outValue);
    }


    bool AnimGraphInstance::GetVector2ParameterValue(const char* paramName, AZ::Vector2* outValue)
    {
        const AZ::Outcome<size_t> index = FindParameterIndex(paramName);
        if (!index.IsSuccess())
        {
            return false;
        }

        return GetVector2ParameterValue(static_cast<uint32>(index.GetValue()), outValue);
    }


    bool AnimGraphInstance::GetVector3ParameterValue(const char* paramName, AZ::Vector3* outValue)
    {
        const AZ::Outcome<size_t> index = FindParameterIndex(paramName);
        if (!index.IsSuccess())
        {
            return false;
        }

        return GetVector3ParameterValue(static_cast<uint32>(index.GetValue()), outValue);
    }


    bool AnimGraphInstance::GetVector4ParameterValue(const char* paramName, AZ::Vector4* outValue)
    {
        const AZ::Outcome<size_t> index = FindParameterIndex(paramName);
        if (!index.IsSuccess())
        {
            return false;
        }

        return GetVector4ParameterValue(static_cast<uint32>(index.GetValue()), outValue);
    }


    bool AnimGraphInstance::GetRotationParameterValue(const char* paramName, MCore::Quaternion* outRotation)
    {
        const AZ::Outcome<size_t> index = FindParameterIndex(paramName);
        if (!index.IsSuccess())
        {
            return false;
        }

        return GetRotationParameterValue(static_cast<uint32>(index.GetValue()), outRotation);
    }
} // namespace EMotionFX

