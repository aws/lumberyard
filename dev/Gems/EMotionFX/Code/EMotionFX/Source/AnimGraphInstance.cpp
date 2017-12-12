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
#include "EMotionFXConfig.h"
#include "AnimGraphInstance.h"
#include "AnimGraph.h"
#include "BlendTreeMotionFrameNode.h"
#include "AnimGraphManager.h"
#include "AnimGraphPosePool.h"
#include "ThreadData.h"
#include "Attachment.h"
#include "ActorInstance.h"
#include "AnimGraphStateMachine.h"
#include "MotionSet.h"
#include "TransformData.h"
#include "EventHandler.h"
#include "EventManager.h"
#include "EMotionFXManager.h"
#include "AnimGraphReferenceNode.h"
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    AnimGraphInstance::AnimGraphInstance(AnimGraph* animGraph, ActorInstance* actorInstance, MotionSet* motionSet, const InitSettings* initSettings, ReferenceInfo* referenceInfo)
        : BaseObject()
    {
        // register at the animgraph
        animGraph->AddAnimGraphInstance(this);
        animGraph->Lock();

        mAnimGraph             = animGraph;
        mActorInstance          = actorInstance;
        mMotionSet              = motionSet;
        mAutoUnregister         = true;
        mEnableVisualization    = true;
        mRetarget               = animGraph->GetRetargetingEnabled();
        mVisualizeScale         = 1.0f;

#if defined(EMFX_DEVELOPMENT_BUILD)
        mIsOwnedByRuntime       = false;
#endif // EMFX_DEVELOPMENT_BUILD

        // copy the reference info settings
        if (referenceInfo)
        {
            mReferenceInfo = *referenceInfo;
        }

        if (initSettings)
        {
            mInitSettings = *initSettings;
        }

        mParamValues.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_INSTANCE);
        mUniqueDatas.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTUNIQUEDATA);
        mObjectFlags.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_INSTANCE);
        mInternalAttributes.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_INSTANCE);

        // init the internal attributes (create them)
        InitInternalAttributes();

        // prealloc the unique data array (doesn't create the actual unique data objects yet though)
        InitUniqueDatas();

        // automatically register the anim graph instance
        GetAnimGraphManager().AddAnimGraphInstance(this);

        // create the parameter value objects
        CreateParameterValues();

        // recursively create the unique datas for all nodes
        if (referenceInfo == nullptr)
        {
            mAnimGraph->GetRootStateMachine()->RecursiveOnUpdateUniqueData(this);
            mAnimGraph->GetRootStateMachine()->Init(this);
        }
        else
        {
            referenceInfo->mSourceNode->RecursiveOnUpdateUniqueData(this);
            referenceInfo->mSourceNode->Init(this);

            // we have to use the parent here, because in some special cases with passthrough nodes, we also need unique data objects of the parent
            AnimGraphNode* sourceParent = referenceInfo->mSourceNode->GetParentNode();
            if (sourceParent)
            {
                const uint32 numChildNodes = sourceParent->GetNumChildNodes();
                for (uint32 i = 0; i < numChildNodes; ++i)
                {
                    AnimGraphNode* child = sourceParent->GetChildNode(i);
                    if (child != referenceInfo->mSourceNode && child != referenceInfo->mReferenceNode) // skip the source node as we already initialized that, and skip the reference node as that creates infinite recursion
                    {
                        child->RecursiveOnUpdateUniqueData(this);
                        child->Init(this);
                    }
                }
            }
        }

        // initialize the anim graph instance
        Init();

        // start the state machines at the entry state
        Start();

        mAnimGraph->Unlock();
        GetEventManager().OnCreateAnimGraphInstance(this);
    }


    // destructor
    AnimGraphInstance::~AnimGraphInstance()
    {
        GetEventManager().OnDeleteAnimGraphInstance(this);

        // automatically unregister the anim graph instance
        if (mAutoUnregister)
        {
            GetAnimGraphManager().RemoveAnimGraphInstance(this, false);
        }

        // get rid of the unique data objects
        AnimGraphObjectDataPool& objectDataPool = GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool();
        objectDataPool.Lock();
        const uint32 numUniqueDatas = mUniqueDatas.GetLength();
        for (uint32 i = 0; i < numUniqueDatas; ++i)
        {
            objectDataPool.FreeWithoutLock(mUniqueDatas[i]);
        }
        mUniqueDatas.Clear();
        objectDataPool.Unlock();

        // remove the parameters and the remaining event handlers
        // this needs to be called at the very end of the destructor!
        if (mReferenceInfo.mParameterSource == nullptr)
        {
            RemoveAllParameters(true);
        }
        else
        {
            RemoveAllParameters(false);
        }

        RemoveAllEventHandlers(true);

        // remove all the internal attributes (from node ports etc)
        RemoveAllInternalAttributes();

        // unregister from the animgraph
        mAnimGraph->RemoveAnimGraphInstance(this);
    }


    // create an animgraph instance
    AnimGraphInstance* AnimGraphInstance::Create(AnimGraph* animGraph, ActorInstance* actorInstance, MotionSet* motionSet, const InitSettings* initSettings, ReferenceInfo* referenceInfo)
    {
        return new AnimGraphInstance(animGraph, actorInstance, motionSet, initSettings, referenceInfo);
    }


    // init
    void AnimGraphInstance::Init()
    {
        // get the root node
        AnimGraphNode* rootNode = GetRootNode();
        if (rootNode == nullptr)
        {
            return;
        }

        // recursively init the root node
        rootNode->RecursiveInit(this);
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
                    mParamValues[i]->Destroy();
                }
            }
        }

        mParamValues.Clear();
    }


    // remove all internal attributes
    void AnimGraphInstance::RemoveAllInternalAttributes()
    {
        MCore::LockGuard lock(mMutex);
        const uint32 numAttribs = mInternalAttributes.GetLength();
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            if (mInternalAttributes[i])
            {
                mInternalAttributes[i]->Destroy();
            }
        }
    }


    // add a new internal attribute
    uint32 AnimGraphInstance::AddInternalAttribute(MCore::Attribute* attribute)
    {
        MCore::LockGuard lock(mMutex);

        // grow the array if we reached its max capacity, to prevent a lot of reallocs
        if (mInternalAttributes.GetLength() == mInternalAttributes.GetMaxLength())
        {
            mInternalAttributes.Reserve(mInternalAttributes.GetLength() + 100);
        }

        mInternalAttributes.Add(attribute);

        return mInternalAttributes.GetLength() - 1;
    }


    // reserve memory for the internal attributes
    void AnimGraphInstance::ReserveInternalAttributes(uint32 totalNumInternalAttributes)
    {
        MCore::LockGuard lock(mMutex);
        mInternalAttributes.Reserve(totalNumInternalAttributes);
    }


    // removes an internal attribute
    void AnimGraphInstance::RemoveInternalAttribute(uint32 index, bool delFromMem)
    {
        MCore::LockGuard lock(mMutex);
        if (delFromMem)
        {
            if (mInternalAttributes[index])
            {
                mInternalAttributes[index]->Destroy();
            }
        }

        mInternalAttributes.Remove(index);
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
            posePool.FreeAllPoses();
        }
    }



    // resize the number of parameters
    void AnimGraphInstance::CreateParameterValues()
    {
        // if we are not dealing with a reference, or we want unique parameters
        if (mReferenceInfo.mParameterSource == nullptr)
        {
            RemoveAllParameters(true);
            mParamValues.Resize(mAnimGraph->GetNumParameters());

            // init the values
            const uint32 numParams = mParamValues.GetLength();
            for (uint32 i = 0; i < numParams; ++i)
            {
                mParamValues[i] = mAnimGraph->GetParameter(i)->GetDefaultValue()->Clone();
            }
        }
        else
        {
            RemoveAllParameters(false); // don't delete them from memory
            mParamValues.Resize(mReferenceInfo.mParameterSource->GetAnimGraph()->GetNumParameters());

            // init the values
            const uint32 numParams = mParamValues.GetLength();
            for (uint32 i = 0; i < numParams; ++i)
            {
                mParamValues[i] = mReferenceInfo.mParameterSource->GetParameterValue(i);
            }
        }
    }


    // add the missing parameters that the anim graph has to this anim graph instance
    void AnimGraphInstance::AddMissingParameterValues()
    {
        // check how many parameters we need to add
        const int32 numToAdd = mAnimGraph->GetNumParameters() - mParamValues.GetLength();
        if (numToAdd <= 0)
        {
            return;
        }

        // make sure we have the right space pre-allocated
        mParamValues.Reserve(mAnimGraph->GetNumParameters());

        // add the remaining parameters
        const uint32 startIndex = mParamValues.GetLength();
        for (int32 i = 0; i < numToAdd; ++i)
        {
            const uint32 index = startIndex + i;
            mParamValues.AddEmpty();
            mParamValues.GetLast() = mAnimGraph->GetParameter(index)->GetDefaultValue()->Clone();
        }
    }


    // remove a parameter value
    void AnimGraphInstance::RemoveParameterValue(uint32 index, bool delFromMem)
    {
        if (delFromMem)
        {
            if (mParamValues[index])
            {
                mParamValues[index]->Destroy();
            }
        }

        mParamValues.Remove(index);
    }


    // reinitialize the parameter
    void AnimGraphInstance::ReInitParameterValue(uint32 index)
    {
        //delete mParamValues[index];
        if (mParamValues[index])
        {
            mParamValues[index]->Destroy();
        }

        mParamValues[index] = mAnimGraph->GetParameter(index)->GetDefaultValue()->Clone();
    }


    // switch to another state using a state name
    bool AnimGraphInstance::SwitchToState(const char* stateName)
    {
        // now try to find the state
        AnimGraphNode* state = mAnimGraph->RecursiveFindNode(stateName);
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
        if (parentNode->GetType() != AnimGraphStateMachine::TYPE_ID)
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
        AnimGraphNode* state = mAnimGraph->RecursiveFindNode(stateName);
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
        if (parentNode->GetType() != AnimGraphStateMachine::TYPE_ID)
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
        if (node->GetType() == AnimGraphStateMachine::TYPE_ID)
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
        if (node->GetType() == AnimGraphStateMachine::TYPE_ID)
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
    MCore::Attribute* AnimGraphInstance::FindParameter(const char* name) const
    {
        // find the parameter index
        const uint32 paramIndex = mAnimGraph->FindParameterIndex(name);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        return mParamValues[paramIndex];
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


    // update all attribute values, like reinit the combobox possible values
    void AnimGraphInstance::OnUpdateUniqueData()
    {
        GetRootNode()->RecursiveOnUpdateUniqueData(this);
    }


    // recursively prepare the node and all its child nodes
    void AnimGraphInstance::RecursivePrepareNode(AnimGraphNode* node)
    {
        // prepare the given node
        node->Prepare(this);

        // get the number of child nodes, iterate through them and call the function recursively
        const uint32 numChildNodes = node->GetNumChildNodes();
        for (uint32 j = 0; j < numChildNodes; ++j)
        {
            RecursivePrepareNode(node->GetChildNode(j));
        }
    }


    // recursively call prepare for all nodes in the anim graph
    void AnimGraphInstance::RecursivePrepareNodes()
    {
        RecursivePrepareNode(GetRootNode());
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


    //
    void AnimGraphInstance::RegisterUniqueObjectData(AnimGraphObjectData* data)
    {
        mUniqueDatas[data->GetObject()->GetObjectIndex()] = data;
    }


    // add a unique object data
    void AnimGraphInstance::AddUniqueObjectData()
    {
        mUniqueDatas.Add(nullptr);
        mObjectFlags.Add(0);
    }


    // remove the given unique data object
    void AnimGraphInstance::RemoveUniqueObjectData(AnimGraphObjectData* uniqueData, bool delFromMem)
    {
        if (uniqueData == nullptr)
        {
            return;
        }

        AnimGraphObjectDataPool& objectDataPool = GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool();
        const uint32 index = uniqueData->GetObject()->GetObjectIndex();
        if (delFromMem)
        {
            objectDataPool.Free(mUniqueDatas[index]);
        }

        mUniqueDatas.Remove(index);
        mObjectFlags.Remove(index);
    }



    void AnimGraphInstance::RemoveUniqueObjectData(uint32 index, bool delFromMem)
    {
        AnimGraphObjectData* data = mUniqueDatas[index];
        mUniqueDatas.Remove(index);
        mObjectFlags.Remove(index);
        if (delFromMem)
        {
            AnimGraphObjectDataPool& objectDataPool = GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool();
            objectDataPool.Free(data);
        }
    }


    // remove all object data
    void AnimGraphInstance::RemoveAllObjectData(bool delFromMem)
    {
        AnimGraphObjectDataPool& objectDataPool = GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool();
        objectDataPool.Lock();
        const uint32 numUniqueDatas = mUniqueDatas.GetLength();
        for (uint32 i = 0; i < numUniqueDatas; ++i)
        {
            if (delFromMem)
            {
                objectDataPool.FreeWithoutLock(mUniqueDatas[i]);
            }
        }
        objectDataPool.Unlock();

        mUniqueDatas.Clear();
        mObjectFlags.Clear();
    }


    // switch parameter values
    void AnimGraphInstance::SwapParameterValues(uint32 whatIndex, uint32 withIndex)
    {
        MCore::Attribute* temp = mParamValues[whatIndex];
        mParamValues[whatIndex] = mParamValues[withIndex];
        mParamValues[withIndex] = temp;
    }


    // register event handler
    void AnimGraphInstance::AddEventHandler(AnimGraphInstanceEventHandler* eventHandler)
    {
        mEventHandlers.Add(eventHandler);
    }


    // find the index of the given event handler
    uint32 AnimGraphInstance::FindEventHandlerIndex(AnimGraphInstanceEventHandler* eventHandler) const
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            // compare the event handlers and return the index in case they are equal
            if (eventHandler == mEventHandlers[i])
            {
                return i;
            }
        }

        // failure, the event handler hasn't been found
        return MCORE_INVALIDINDEX32;
    }


    // unregister event handler
    bool AnimGraphInstance::RemoveEventHandler(AnimGraphInstanceEventHandler* eventHandler, bool delFromMem)
    {
        // get the index of the event handler
        const uint32 index = FindEventHandlerIndex(eventHandler);
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        // remove the given event handler
        RemoveEventHandler(index, delFromMem);
        return true;
    }


    // unregister event handler
    void AnimGraphInstance::RemoveEventHandler(uint32 index, bool delFromMem)
    {
        if (delFromMem)
        {
            mEventHandlers[index]->Destroy();
        }

        mEventHandlers.Remove(index);
    }


    // remove all event handlers
    void AnimGraphInstance::RemoveAllEventHandlers(bool delFromMem)
    {
        // destroy all event handlers
        if (delFromMem)
        {
            const uint32 numEventHandlers = mEventHandlers.GetLength();
            for (uint32 i = 0; i < numEventHandlers; ++i)
            {
                mEventHandlers[i]->Destroy();
            }
        }

        mEventHandlers.Clear();
    }


    void AnimGraphInstance::OnStateEnter(AnimGraphNode* state)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnStateEnter(this, state);
        }
    }


    void AnimGraphInstance::OnStateEntering(AnimGraphNode* state)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnStateEntering(this, state);
        }
    }


    void AnimGraphInstance::OnStateExit(AnimGraphNode* state)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnStateExit(this, state);
        }
    }


    void AnimGraphInstance::OnStateEnd(AnimGraphNode* state)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnStateEnd(this, state);
        }
    }


    void AnimGraphInstance::OnStartTransition(AnimGraphStateTransition* transition)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnStartTransition(this, transition);
        }
    }


    void AnimGraphInstance::OnEndTransition(AnimGraphStateTransition* transition)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnEndTransition(this, transition);
        }
    }


    // init the hashmap
    void AnimGraphInstance::InitUniqueDatas()
    {
        const uint32 numObjects = mAnimGraph->GetNumObjects();
        mUniqueDatas.Resize(numObjects);
        mObjectFlags.Resize(numObjects);
        for (uint32 i = 0; i < numObjects; ++i)
        {
            mUniqueDatas[i] = nullptr;
            mObjectFlags[i] = 0;
        }
    }


    // get the root node
    AnimGraphNode* AnimGraphInstance::GetRootNode() const
    {
        return (mReferenceInfo.mSourceNode == nullptr) ? mAnimGraph->GetRootStateMachine() : mReferenceInfo.mSourceNode;
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
        // pass 1: update (bottom up), update motion timers etc
        // pass 2: topdown update (top down), syncing happens here (adjusts motion/node timers again)
        // pass 3: postupdate (bottom up), processing the motion events events and update motion extraction deltas
        // pass 4: output (bottom up), calculate all new bone transforms (the heavy thing to process) <--- not performed by this function but in AnimGraphInstance::Output()

        // reset the output is ready flags, so we return cached copies of the outputs, but refresh/recalculate them
        AnimGraphNode* rootNode = GetRootNode();

        // TODO: just use the ResetFlagsForAllNodes later on, but fix the issue with overwriting connection isProcessed/isVisited flags first when showing the graphs in EMStudio
    #ifdef EMFX_EMSTUDIOBUILD
        rootNode->RecursiveResetFlags(this, 0xffffffff);    // the 0xffffffff clears all flags
    #else
        //ResetFlagsForAllNodes(0xffffffff);
        ResetFlagsForAllObjects();
    #endif

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
            AnimGraphNodeData* nodeData = static_cast<AnimGraphNodeData*>(mUniqueDatas[mAnimGraph->GetNode(i)->GetObjectIndex()]);
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
    }


    // reset flags for all nodes
    void AnimGraphInstance::ResetFlagsForAllNodes(uint32 flagsToDisable)
    {
        const uint32 numNodes = mAnimGraph->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            AnimGraphNode* node = mAnimGraph->GetNode(i);
            mObjectFlags[node->GetObjectIndex()] &= ~flagsToDisable;

        #ifdef EMFX_EMSTUDIOBUILD
            // reset all connections
            const uint32 numConnections = node->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                node->GetConnection(c)->SetIsVisited(false);
            }
        #endif
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
    void AnimGraphInstance::CollectActiveAnimGraphNodes(MCore::Array<AnimGraphNode*>* outNodes, uint32 nodeTypeID)
    {
        outNodes->Clear(false);
        mAnimGraph->GetRootStateMachine()->RecursiveCollectActiveNodes(this, outNodes, nodeTypeID);
    }


    // find the unique node data
    AnimGraphNodeData* AnimGraphInstance::FindUniqueNodeData(const AnimGraphNode* node) const
    {
        AnimGraphObjectData* result = mUniqueDatas[node->GetObjectIndex()];
        //MCORE_ASSERT(result->GetObject()->GetBaseType() == AnimGraphNode::BASETYPE_ID);
        return reinterpret_cast<AnimGraphNodeData*>(result);
    }

    // find the parameter index
    uint32 AnimGraphInstance::FindParameterIndex(const char* name) const
    {
        return mAnimGraph->FindParameterIndex(name);
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


    bool AnimGraphInstance::GetIsReference() const
    {
        return (mReferenceInfo.mSourceNode != nullptr);
    }


    const AnimGraphInstance::ReferenceInfo& AnimGraphInstance::GetReferenceInfo() const
    {
        return mReferenceInfo;
    }


    void AnimGraphInstance::SetUniqueObjectData(uint32 index, AnimGraphObjectData* data)
    {
        mUniqueDatas[index] = data;
    }


    const AnimGraphInstance::InitSettings& AnimGraphInstance::GetInitSettings() const
    {
        return mInitSettings;
    }


    const AnimGraphEventBuffer& AnimGraphInstance::GetEventBuffer() const
    {
        return mEventBuffer;
    }



    bool AnimGraphInstance::GetFloatParameterValue(uint32 paramIndex, float* outValue)
    {
        MCore::AttributeFloat* param = GetParameterValueChecked<MCore::AttributeFloat>(paramIndex);
        if (param)
        {
            *outValue = param->GetValue();
            return true;
        }

        return false;
    }


    bool AnimGraphInstance::GetFloatParameterValueAsBool(uint32 paramIndex, bool* outValue)
    {
        MCore::AttributeFloat* param = GetParameterValueChecked<MCore::AttributeFloat>(paramIndex);
        if (param)
        {
            *outValue = (MCore::Math::IsFloatZero(param->GetValue()) == false);
            return true;
        }

        return false;
    }


    bool AnimGraphInstance::GetFloatParameterValueAsInt(uint32 paramIndex, int32* outValue)
    {
        MCore::AttributeFloat* param = GetParameterValueChecked<MCore::AttributeFloat>(paramIndex);
        if (param)
        {
            *outValue = static_cast<int32>(param->GetValue());
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


    bool AnimGraphInstance::GetVector3ParameterValue(uint32 paramIndex, MCore::Vector3* outValue)
    {
        MCore::AttributeVector3* param = GetParameterValueChecked<MCore::AttributeVector3>(paramIndex);
        if (param)
        {
            *outValue = param->GetValue();
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


    // get the rotation
    bool AnimGraphInstance::GetRotationParameterValue(uint32 paramIndex, MCore::Quaternion* outRotation)
    {
        AttributeRotation* param = GetParameterValueChecked<AttributeRotation>(paramIndex);
        if (param)
        {
            *outRotation = param->GetRotationQuaternion();
            return true;
        }

        return false;
    }


    bool AnimGraphInstance::GetFloatParameterValue(const char* paramName, float* outValue)
    {
        const uint32 index = FindParameterIndex(paramName);
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        MCore::AttributeFloat* attrib = GetParameterValueChecked<MCore::AttributeFloat>(index);
        if (attrib == nullptr)
        {
            return false;
        }

        *outValue = attrib->GetValue();
        return true;
    }


    bool AnimGraphInstance::GetFloatParameterValueAsBool(const char* paramName, bool* outValue)
    {
        const uint32 index = FindParameterIndex(paramName);
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        MCore::AttributeFloat* attrib = GetParameterValueChecked<MCore::AttributeFloat>(index);
        if (attrib == nullptr)
        {
            return false;
        }

        *outValue = (MCore::Math::IsFloatZero(attrib->GetValue()) == false);
        return true;
    }


    bool AnimGraphInstance::GetFloatParameterValueAsInt(const char* paramName, int32* outValue)
    {
        const uint32 index = FindParameterIndex(paramName);
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        MCore::AttributeFloat* attrib = GetParameterValueChecked<MCore::AttributeFloat>(index);
        if (attrib == nullptr)
        {
            return false;
        }

        *outValue = static_cast<int32>(attrib->GetValue());
        return true;
    }


    bool AnimGraphInstance::GetVector2ParameterValue(const char* paramName, AZ::Vector2* outValue)
    {
        const uint32 index = FindParameterIndex(paramName);
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        MCore::AttributeVector2* attrib = GetParameterValueChecked<MCore::AttributeVector2>(index);
        if (attrib == nullptr)
        {
            return false;
        }

        *outValue = attrib->GetValue();
        return true;
    }


    bool AnimGraphInstance::GetVector3ParameterValue(const char* paramName, MCore::Vector3* outValue)
    {
        const uint32 index = FindParameterIndex(paramName);
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        MCore::AttributeVector3* attrib = GetParameterValueChecked<MCore::AttributeVector3>(index);
        if (attrib == nullptr)
        {
            return false;
        }

        *outValue = attrib->GetValue();
        return true;
    }


    bool AnimGraphInstance::GetVector4ParameterValue(const char* paramName, AZ::Vector4* outValue)
    {
        const uint32 index = FindParameterIndex(paramName);
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        MCore::AttributeVector4* attrib = GetParameterValueChecked<MCore::AttributeVector4>(index);
        if (attrib == nullptr)
        {
            return false;
        }

        *outValue = attrib->GetValue();
        return true;
    }


    bool AnimGraphInstance::GetRotationParameterValue(const char* paramName, MCore::Quaternion* outRotation)
    {
        const uint32 index = FindParameterIndex(paramName);
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        AttributeRotation* attrib = GetParameterValueChecked<AttributeRotation>(index);
        if (attrib == nullptr)
        {
            return false;
        }

        *outRotation = attrib->GetRotationQuaternion();
        return true;
    }
} // namespace EMotionFX

