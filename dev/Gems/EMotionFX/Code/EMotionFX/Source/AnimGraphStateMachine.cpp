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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "EMotionFXConfig.h"
#include "EventManager.h"
#include "AnimGraphStateMachine.h"
#include "BlendTree.h"
#include "AnimGraph.h"
#include "AnimGraphInstance.h"
#include "ActorInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphStateTransition.h"
#include "AnimGraphTransitionCondition.h"
#include "AnimGraphTriggerAction.h"
#include "AnimGraphExitNode.h"
#include "AnimGraphNodeGroup.h"
#include "AnimGraphEntryNode.h"
#include "AnimGraphManager.h"


#include <AzCore/RTTI/RTTI.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphStateMachine, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphStateMachine::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)

    AnimGraphStateMachine::AnimGraphStateMachine()
        : AnimGraphNode()
        , mEntryState(nullptr)
        , mEntryStateNodeNr(MCORE_INVALIDINDEX32)
        , m_entryStateId(AnimGraphNodeId::InvalidId)
        , m_alwaysStartInEntryState(true)
    {
        // setup the ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    AnimGraphStateMachine::~AnimGraphStateMachine()
    {
        // NOTE: base class automatically removes all child nodes (states)
        RemoveAllTransitions();
    }

    void AnimGraphStateMachine::RecursiveReinit()
    {
        // Re-initialize all child nodes and connections
        AnimGraphNode::RecursiveReinit();

        for (AnimGraphStateTransition* transition : mTransitions)
        {
            transition->RecursiveReinit();
        }
    }


    bool AnimGraphStateMachine::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        for (AnimGraphStateTransition* transition : mTransitions)
        {
            transition->InitAfterLoading(animGraph);
        }

        // Needs to be called after anim graph is set (will iterate through anim graph instances).
        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    // remove all transitions
    void AnimGraphStateMachine::RemoveAllTransitions()
    {
        for (AnimGraphStateTransition* transition : mTransitions)
        {
            delete transition;
        }

        // clear the transition array
        mTransitions.clear();
    }


    // calculate the output
    void AnimGraphStateMachine::Output(AnimGraphInstance* animGraphInstance)
    {
        // make sure we have enough space in our output pose
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();

        // if this node is disabled, output the bind pose
        if (mDisabled)
        {
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            return;
        }

        // reset the error flag
        if (GetEMotionFX().GetIsInEditorMode())
        {
            SetHasError(animGraphInstance, false);
        }

        // in case no state is currently active and evaluated, use the bind pose
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        if (uniqueData->mCurrentState == nullptr)
        {
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
            }
            return;
        }

        AnimGraphPose* outputPose;

        // get the current transition
        AnimGraphStateTransition* transition = uniqueData->mTransition;
        if (!transition)
        {
            // if there is currently no transition going on while there is a current state, just sample that, else return bind pose
            if (uniqueData->mCurrentState)
            {
                uniqueData->mCurrentState->PerformOutput(animGraphInstance);
                RequestPoses(animGraphInstance);
                outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
                *outputPose = *uniqueData->mCurrentState->GetMainOutputPose(animGraphInstance);

                uniqueData->mCurrentState->DecreaseRef(animGraphInstance);
            }
            else
            {
                RequestPoses(animGraphInstance);
                outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
                outputPose->InitFromBindPose(actorInstance);
            }
        }
        else
        {
            // output the source and target state outputs into the temp poses
            OutputIncomingNode(animGraphInstance, uniqueData->mCurrentState);
            OutputIncomingNode(animGraphInstance, uniqueData->mTargetState);

            // processes the transition, calculate and blend the in between pose and use it as output for the state machine
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            if (uniqueData->mCurrentState->GetMainOutputPose(animGraphInstance) && uniqueData->mTargetState->GetMainOutputPose(animGraphInstance))
            {
                transition->CalcTransitionOutput(animGraphInstance, *uniqueData->mCurrentState->GetMainOutputPose(animGraphInstance), *uniqueData->mTargetState->GetMainOutputPose(animGraphInstance), outputPose);
            }
            uniqueData->mCurrentState->DecreaseRef(animGraphInstance);
            uniqueData->mTargetState->DecreaseRef(animGraphInstance);
        }

        // visualize it
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    }


    // check all outgoing transitions for the given node for if they are ready
    void AnimGraphStateMachine::CheckConditions(AnimGraphNode* sourceNode, AnimGraphInstance* animGraphInstance, UniqueData* uniqueData, bool allowTransition)
    {
        // skip calculations in case the source node is not valid
        if (sourceNode == nullptr)
        {
            return;
        }

        // check if there is a state we can transition into, based on the transition conditions
        // variables that will hold the prioritized transition information
        int32                       highestPriority         = -1;
        AnimGraphStateTransition*   prioritizedTransition   = nullptr;
        bool                        requestInterruption     = false;
        const bool                  isTransitioning         = uniqueData->mTransition != nullptr;
        const AnimGraphNodeId       sourceNodeId            = sourceNode->GetId();

        // iterate through the transitions
        for (AnimGraphStateTransition* curTransition : mTransitions)
        {
            // get the current transition and skip it directly if in case it is disabled
            if (curTransition->GetIsDisabled())
            {
                continue;
            }

            // read some information from the transition
            const bool      isWildcardTransition    = curTransition->GetIsWildcardTransition();
            AnimGraphNode* transitionTargetNode    = curTransition->GetTargetNode();

            // skip transitions that don't start from our given start node
            if (isWildcardTransition == false && curTransition->GetSourceNode() != sourceNode)
            {
                continue;
            }
            // Wildcard transitions: The state filter holds the allowed states from which we can enter the wildcard transition.
            // An empty filter means transitioning is allowed from any other state. In case the wildcard transition has a filter specified and
            // the given source node is not part of the selection, we'll skip it and don't allow to transition.
            if (isWildcardTransition && !curTransition->CanWildcardTransitionFrom(sourceNode))
            {
                continue;
            }

            // check if the transition evaluates as valid (if the conditions evaluate to true)
            if (curTransition->CheckIfIsReady(animGraphInstance))
            {
                // compare the priority values and overwrite it in case it is more important
                const int32 transitionPriority = curTransition->GetPriority();
                if (transitionPriority > highestPriority)
                {
                    // is the state machine currently transitioning?
                    if (isTransitioning)
                    {
                        // in case it is transitioning, can this transition interrupt the currently active one?
                        // case 1: only allow an interruption request in case the currently active transition and the one that requested the interruption are different ones and
                        //         in case the new one can interrupt others and the currently active one can be interrupted
                        // case 2: in case the new and the currently active one are the same, check if self interruption is allowed
                        if ((uniqueData->mTransition != curTransition && curTransition->GetCanInterruptOtherTransitions() && uniqueData->mTransition->GetCanBeInterrupted()) ||
                            (uniqueData->mTransition == curTransition && curTransition->GetCanInterruptItself()))
                        {
                            highestPriority         = transitionPriority;
                            prioritizedTransition   = curTransition;
                            requestInterruption     = true;
                        }
                    }
                    else
                    {
                        // skip transitions that end in the currently active state
                        if (sourceNode != transitionTargetNode)
                        {
                            // in case we're not transitioning at the moment, just do normal
                            highestPriority         = transitionPriority;
                            prioritizedTransition   = curTransition;
                        }
                    }
                }
            }
        }

        // check if a transition condition fired and adjust the current state to the target node of the transition with the highest priority
        if (prioritizedTransition && allowTransition)
        {
            // interrupt currently active transition in case of a request
            if (requestInterruption && uniqueData->mTransition)
            {
                // get the source and the target node of the currently active transition
                AnimGraphStateTransition*  transition = uniqueData->mTransition;
                AnimGraphNode*             transitionSourceNode = transition->GetSourceNode(animGraphInstance);
                AnimGraphNode*             transitionTargetNode = transition->GetTargetNode();

                transitionTargetNode->OnStateExit(animGraphInstance, transitionSourceNode, transition);
                transitionTargetNode->OnStateEnd(animGraphInstance, transitionSourceNode, transition);
                uniqueData->mTransition->OnEndTransition(animGraphInstance);

                transitionSourceNode->Rewind(animGraphInstance);

                transitionSourceNode->OnStateEntering(animGraphInstance, transitionTargetNode, transition);
                transitionSourceNode->OnStateEnter(animGraphInstance, transitionTargetNode, transition);

                GetEventManager().OnStateExit(animGraphInstance, transitionTargetNode);
                GetEventManager().OnStateEnd(animGraphInstance, transitionTargetNode);
                GetEventManager().OnEndTransition(animGraphInstance, transition);
                GetEventManager().OnStateEntering(animGraphInstance, transitionSourceNode);
                GetEventManager().OnStateEnter(animGraphInstance, transitionSourceNode);

                uniqueData->mTransition->ResetConditions(animGraphInstance);
                uniqueData->mTransition = nullptr;
            }

            // start transitioning
            StartTransition(animGraphInstance, uniqueData, prioritizedTransition);
        }
    }


    // update conditions for all transitions that start from the given state and all wild card transitions
    void AnimGraphStateMachine::UpdateConditions(AnimGraphInstance* animGraphInstance, AnimGraphNode* animGraphNode, UniqueData* uniqueData, float timePassedInSeconds)
    {
        // skip calculations in case the node is not valid
        if (animGraphNode == nullptr)
        {
            return;
        }

        const bool isTransitioning = (uniqueData->mTransition != nullptr);

        for (const AnimGraphStateTransition* transition : mTransitions)
        {
            // get the current transition and skip it directly if in case it is disabled
            if (transition->GetIsDisabled())
            {
                continue;
            }

            // skip transitions that don't start from our given current node
            if (transition->GetSourceNode() != animGraphNode && transition->GetIsWildcardTransition() == false)
            {
                continue;
            }

            // skip transitions that are not made for interrupting when we are currently transitioning
            if (isTransitioning && transition->GetCanInterruptOtherTransitions() == false)
            {
                continue;
            }

            const size_t numConditions = transition->GetNumConditions();
            for (size_t j = 0; j < numConditions; ++j)
            {
                AnimGraphTransitionCondition* condition = transition->GetCondition(j);
                condition->Update(animGraphInstance, timePassedInSeconds);
            }
        }
    }


    // starts the given transition
    void AnimGraphStateMachine::StartTransition(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData, AnimGraphStateTransition* transition)
    {
        // a new transition has been found, start transitioning
        // get the source and the target node of the transition we need to start
        AnimGraphNode* sourceNode = transition->GetSourceNode();
        AnimGraphNode* targetNode = transition->GetTargetNode();

        // in case we're dealing with a wildcard transition
        if (transition->GetIsWildcardTransition())
        {
            // adjust the source node and adjust the unique data source node of the transition
            sourceNode = uniqueData->mCurrentState;
            transition->SetSourceNode(animGraphInstance, sourceNode);
        }

        // rewind the target state and reset conditions of all outgoing transitions
        targetNode->Rewind(animGraphInstance);
        ResetOutgoingTransitionConditions(animGraphInstance, targetNode);

        sourceNode->OnStateExit(animGraphInstance, targetNode, transition);
        targetNode->OnStateEntering(animGraphInstance, sourceNode, transition);

        transition->OnStartTransition(animGraphInstance);

        GetEventManager().OnStateExit(animGraphInstance, sourceNode);
        GetEventManager().OnStateEntering(animGraphInstance, targetNode);
        GetEventManager().OnStartTransition(animGraphInstance, transition);

        // update the unique data
        uniqueData->mTransition = transition;
        uniqueData->mTargetState = transition->GetTargetNode();
    }

        
    void AnimGraphStateMachine::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));

        // Update the transition that's currently active.
        AnimGraphStateTransition* transition = uniqueData->mTransition;
        if (transition)
        {
            transition->Update(animGraphInstance, timePassedInSeconds);
        }

        // Update the current state, and increase the ref count, as we need its pose when calculating the transition output.
        if (uniqueData->mCurrentState)
        {
            uniqueData->mCurrentState->IncreasePoseRefCount(animGraphInstance);
            uniqueData->mCurrentState->IncreaseRefDataRefCount(animGraphInstance);
            uniqueData->mCurrentState->PerformUpdate(animGraphInstance, timePassedInSeconds);
        }

        // Update the target state if it is pointing to another state than the current one.
        if (uniqueData->mTargetState && (uniqueData->mCurrentState != uniqueData->mTargetState))
        {
            uniqueData->mTargetState->IncreasePoseRefCount(animGraphInstance);
            uniqueData->mTargetState->IncreaseRefDataRefCount(animGraphInstance);
            uniqueData->mTargetState->PerformUpdate(animGraphInstance, timePassedInSeconds);
        }

        // Update the conditions and trigger the right transition based on the conditions and priority levels etc.
        AnimGraphNode* prevTarget = uniqueData->mTargetState;
        UpdateConditions(animGraphInstance, uniqueData->mCurrentState, uniqueData, timePassedInSeconds);
        CheckConditions(uniqueData->mCurrentState, animGraphInstance, uniqueData);
        if (prevTarget != uniqueData->mTargetState && uniqueData->mTargetState != uniqueData->mCurrentState)
        {
            uniqueData->mTargetState->IncreasePoseRefCount(animGraphInstance);
            uniqueData->mTargetState->IncreaseRefDataRefCount(animGraphInstance);
            uniqueData->mTargetState->PerformUpdate(animGraphInstance, 0.0f);   // Use a zero time as we don't want to progress this node yet, as we just want to activate it. But we have to refresh internals so an Update is needed.
        }

        // While our transition ended check for new transitions to be activated directly
        // Do this a maximum of 10 times, which should be more than enough
        AZ::s32 numPasses = 0;
        while (uniqueData->mTransition && uniqueData->mTransition->GetIsDone(animGraphInstance))
        {
            AnimGraphNode* targetState = uniqueData->mTargetState;
            EventManager& eventManager = GetEventManager();

            // Trigger the events in the nodes locally.
            transition->OnEndTransition(animGraphInstance);
            uniqueData->mCurrentState->OnStateEnd(animGraphInstance, targetState, transition);
            targetState->OnStateEnter(animGraphInstance, uniqueData->mCurrentState, transition);

            // Trigger the global events.
            eventManager.OnEndTransition(animGraphInstance, transition);
            eventManager.OnStateEnd(animGraphInstance, uniqueData->mCurrentState);
            eventManager.OnStateEnter(animGraphInstance, targetState);

            // Reset the conditions of the transition that has just ended.
            transition->ResetConditions(animGraphInstance);
            uniqueData->mPreviousState  = uniqueData->mCurrentState;
            uniqueData->mCurrentState   = targetState;
            uniqueData->mTargetState    = nullptr;
            uniqueData->mTransition     = nullptr;

            UpdateConditions(animGraphInstance, uniqueData->mCurrentState, uniqueData, 0.0f);
            CheckConditions(uniqueData->mCurrentState, animGraphInstance, uniqueData);

            // Enable the exit state reached flag when are entering an exit state or if the current state is an exit state.
            UpdateExitStateReachedFlag(uniqueData);

            if (!uniqueData->mTransition)
            {
                break;
            }

            if (uniqueData->mCurrentState)
            {
                uniqueData->mCurrentState->IncreasePoseRefCount(animGraphInstance);
                uniqueData->mCurrentState->IncreaseRefDataRefCount(animGraphInstance);
                uniqueData->mCurrentState->PerformUpdate(animGraphInstance, 0.0f);
            }

            if (uniqueData->mTargetState && (uniqueData->mCurrentState != uniqueData->mTargetState))
            {
                uniqueData->mTargetState->IncreasePoseRefCount(animGraphInstance);
                uniqueData->mTargetState->IncreaseRefDataRefCount(animGraphInstance);
                uniqueData->mTargetState->PerformUpdate(animGraphInstance, 0.0f);
            }

            if (numPasses++ > 10)
            {
                AZ_Assert("EMotionFX", "Breaking out of state transition loop. We keep transitioning for 10 times in a row already.");
                break;
            }
        }

        // Enable the exit state reached flag when are entering an exit state or if the current state is an exit state.
        UpdateExitStateReachedFlag(uniqueData);

        // Get the active states.
        AnimGraphNode* nodeA = nullptr;
        AnimGraphNode* nodeB = nullptr;
        GetActiveStates(animGraphInstance, &nodeA, &nodeB);

        // Perform play speed synchronization when transitioning.
        if (nodeA)
        {
            uniqueData->Init(animGraphInstance, nodeA);
            if (transition && nodeB)
            {
                const float transitionWeight = transition->GetBlendWeight(animGraphInstance);

                // Calculate the corrected play speed.
                float factorA;
                float factorB;
                float playSpeed;
                const ESyncMode syncMode = transition->GetSyncMode();
                AnimGraphNode::CalcSyncFactors(animGraphInstance, nodeA, nodeB, syncMode, transitionWeight, &factorA, &factorB, &playSpeed);
                uniqueData->SetPlaySpeed(playSpeed * factorA);
            }
        }
        else
        {
            uniqueData->Clear();
        }
    }


    void AnimGraphStateMachine::UpdateExitStateReachedFlag(UniqueData* uniqueData)
    {
        if (uniqueData->mCurrentState && (azrtti_typeid(uniqueData->mCurrentState) == azrtti_typeid<AnimGraphExitNode>()) ||
            (uniqueData->mTargetState  && (azrtti_typeid(uniqueData->mTargetState)  == azrtti_typeid<AnimGraphExitNode>())))
        {
            uniqueData->mReachedExitState = true;
        }
        else
        {
            uniqueData->mReachedExitState = false;
        }
    }


    // post update
    void AnimGraphStateMachine::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // get the active states
        AnimGraphNode* nodeA = nullptr;
        AnimGraphNode* nodeB = nullptr;
        GetActiveStates(animGraphInstance, &nodeA, &nodeB);

        if (nodeA)
        {
            nodeA->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        }

        if (nodeB)
        {
            nodeB->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        }

        RequestRefDatas(animGraphInstance);
        AnimGraphNodeData* uniqueData = FindUniqueNodeData(animGraphInstance);
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        if (nodeA == nullptr)
        {
            data->ZeroTrajectoryDelta();
            data->ClearEventBuffer();
        }

        // get the currently active transition
        AnimGraphStateTransition* activeTransition = GetActiveTransition(animGraphInstance);
        if (activeTransition == nullptr)
        {
            // in case no transition is active we're fully blended in a single node
            if (nodeA)
            {
                AnimGraphRefCountedData* nodeAData = nodeA->FindUniqueNodeData(animGraphInstance)->GetRefCountedData();

                if (nodeAData)
                {
                    data->SetEventBuffer(nodeAData->GetEventBuffer());
                    data->SetTrajectoryDelta(nodeAData->GetTrajectoryDelta());
                    data->SetTrajectoryDeltaMirrored(nodeAData->GetTrajectoryDeltaMirrored());
                }
                nodeA->DecreaseRefDataRef(animGraphInstance);
            }
        }
        else
        {
            // get the blend weight
            const float weight = activeTransition->GetBlendWeight(animGraphInstance);

            FilterEvents(animGraphInstance, activeTransition->GetEventFilterMode(), nodeA, nodeB, weight, data);

            // transition motion extraction handling
            Transform delta;
            Transform deltaMirrored;
            activeTransition->ExtractMotion(animGraphInstance, &delta, &deltaMirrored);
            data->SetTrajectoryDelta(delta);
            data->SetTrajectoryDeltaMirrored(deltaMirrored);

            nodeA->DecreaseRefDataRef(animGraphInstance);
            nodeB->DecreaseRefDataRef(animGraphInstance);
        }
    }


    // adjust the current state
    void AnimGraphStateMachine::SwitchToState(AnimGraphInstance* animGraphInstance, AnimGraphNode* targetState)
    {
        // get the unique data for this state machine in a given anim graph instance
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueNodeData(this));

        // rewind the target state and reset all outgoing transitions of it
        if (targetState)
        {
            // rewind the new final state and reset conditions of all outgoing transitions
            targetState->Rewind(animGraphInstance);
            ResetOutgoingTransitionConditions(animGraphInstance, targetState);
        }

        // tell the current node to which node we're exiting
        if (uniqueData->mCurrentState)
        {
            uniqueData->mCurrentState->OnStateExit(animGraphInstance, targetState, nullptr);
            uniqueData->mCurrentState->OnStateEnd(animGraphInstance, targetState, nullptr);
        }

        // tell the new current node from which node we're coming
        if (targetState)
        {
            targetState->OnStateEntering(animGraphInstance, uniqueData->mCurrentState, nullptr);
            targetState->OnStateEnter(animGraphInstance, uniqueData->mCurrentState, nullptr);
        }

        // Inform the event manager.
        EventManager& eventManager = GetEventManager();
        eventManager.OnStateExit(animGraphInstance, uniqueData->mCurrentState);
        eventManager.OnStateEntering(animGraphInstance, targetState);
        eventManager.OnStateEnd(animGraphInstance, uniqueData->mCurrentState);
        eventManager.OnStateEnter(animGraphInstance, targetState);

        uniqueData->mPreviousState  = uniqueData->mCurrentState;
        uniqueData->mCurrentState   = targetState;
        uniqueData->mTargetState    = nullptr;
        uniqueData->mTransition     = nullptr;
    }


    // checks if there is a transition from the current to the target node and starts a transition towards it, in case there is no transition between them the target node just gets activated
    void AnimGraphStateMachine::TransitionToState(AnimGraphInstance* animGraphInstance, AnimGraphNode* targetState)
    {
        // get the currently activated state
        AnimGraphNode* currentState = GetCurrentState(animGraphInstance);

        // check if there is a transition between the current and the desired target state
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueNodeData(this));
        AnimGraphStateTransition* transition = FindTransition(animGraphInstance, currentState, targetState);
        if (transition && currentState)
        {
            StartTransition(animGraphInstance, uniqueData, transition);
        }
        else
        {
            SwitchToState(animGraphInstance, targetState);
        }
    }


    // true in case there is an active transition, false if not
    bool AnimGraphStateMachine::GetIsTransitioning(AnimGraphInstance* animGraphInstance) const
    {
        return (GetActiveTransition(animGraphInstance) != nullptr);
    }


    // return a pointer to the currently active transition
    AnimGraphStateTransition* AnimGraphStateMachine::GetActiveTransition(AnimGraphInstance* animGraphInstance) const
    {
        // get the unique data for this state machine and check if there is a transition set
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueNodeData(this));
        return uniqueData->mTransition;
    }


    // add a new transition to the state machine
    void AnimGraphStateMachine::AddTransition(AnimGraphStateTransition* transition)
    {
        mTransitions.push_back(transition);
    }


    // find the next transition to take
    AnimGraphStateTransition* AnimGraphStateMachine::FindTransition(AnimGraphInstance* animGraphInstance, AnimGraphNode* currentState, AnimGraphNode* targetState) const
    {
        MCORE_UNUSED(animGraphInstance);

        // check if we actually want to transit into another state, in case the final state is nullptr we can return directly
        if (targetState == nullptr)
        {
            return nullptr;
        }

        if (currentState == targetState)
        {
            return nullptr;
        }

        // TODO: optimize by giving each animgraph node also an array of transitions?

        ///////////////////////////////////////////////////////////////////////
        // PASS 1: Check if there is a direct connection to the target state
        ///////////////////////////////////////////////////////////////////////

        // variables that will hold the prioritized transition information
        int32                       highestPriority         = -1;
        AnimGraphStateTransition*  prioritizedTransition   = nullptr;

        // first check if there is a ready transition that points directly to the target state
        for (AnimGraphStateTransition* transition : mTransitions)
        {
            // get the current transition and skip it directly if in case it is disabled
            if (transition->GetIsDisabled())
            {
                continue;
            }

            // only do normal state transitions that end in the desired final anim graph node, no wildcard transitions
            if (!transition->GetIsWildcardTransition() && transition->GetSourceNode() == currentState && transition->GetTargetNode() == targetState)
            {
                // compare the priority values and overwrite it in case it is more important
                const int32 transitionPriority = transition->GetPriority();
                if (transitionPriority > highestPriority)
                {
                    highestPriority         = transitionPriority;
                    prioritizedTransition   = transition;
                }
            }
        }

        // check if we have found a direct transition to the desired final state and return in this case
        if (prioritizedTransition)
        {
            return prioritizedTransition;
        }


        ///////////////////////////////////////////////////////////////////////
        // PASS 2: Check if there is a wild card connection to the target state
        ///////////////////////////////////////////////////////////////////////
        // in case there is no direct and no indirect transition ready, check for wildcard transitions
        // there is a maximum number of one for wild card transitions, so we don't need to check the priority values here
        for (AnimGraphStateTransition* transition : mTransitions)
        {
            // get the current transition and skip it directly if in case it is disabled
            if (transition->GetIsDisabled())
            {
                continue;
            }

            // only handle wildcard transitions for the given target node this time
            if (transition->GetIsWildcardTransition() && transition->GetTargetNode() == targetState)
            {
                return transition;
            }
        }

        // no transition found
        return nullptr;
    }


    AZ::Outcome<size_t> AnimGraphStateMachine::FindTransitionIndexById(AnimGraphNodeId transitionId) const
    {
        const size_t numTransitions = mTransitions.size();
        for (size_t i = 0; i < numTransitions; ++i)
        {
            if (mTransitions[i]->GetId() == transitionId)
            {
                return AZ::Success(i);
            }
        }

        return AZ::Failure();
    }


    AnimGraphStateTransition* AnimGraphStateMachine::FindTransitionById(AnimGraphNodeId transitionId) const
    {
        const AZ::Outcome<size_t> transitionIndex = FindTransitionIndexById(transitionId);
        if (transitionIndex.IsSuccess())
        {
            return mTransitions[transitionIndex.GetValue()];
        }

        return nullptr;
    }


    // find the transition index
    uint32 AnimGraphStateMachine::FindTransitionIndex(AnimGraphStateTransition* transition) const
    {
        const auto& iterator = AZStd::find(mTransitions.begin(), mTransitions.end(), transition);
        if (iterator == mTransitions.end())
        {
            return MCORE_INVALIDINDEX32;
        }

        const size_t index = iterator - mTransitions.begin();
        return static_cast<uint32>(index);
    }


    // check if the given node has a wildcard transition
    bool AnimGraphStateMachine::CheckIfHasWildcardTransition(AnimGraphNode* state) const
    {
        for (const AnimGraphStateTransition* transition : mTransitions)
        {
            // check if the given transition is a wildcard transition and if the target node is the given one
            if (transition->GetTargetNode() == state && transition->GetIsWildcardTransition())
            {
                return true;
            }
        }

        // no wildcard transition found
        return false;
    }


    void AnimGraphStateMachine::RemoveTransition(size_t transitionIndex, bool delFromMem)
    {
        if (delFromMem)
        {
            delete mTransitions[transitionIndex];
        }

        mTransitions.erase(mTransitions.begin() + transitionIndex);
    }


    // get the size in bytes of the custom data
    uint32 AnimGraphStateMachine::GetCustomDataSize() const
    {
        return sizeof(uint32);
    }


    // read the custom data
    bool AnimGraphStateMachine::ReadCustomData(MCore::Stream* stream, uint32 version, MCore::Endian::EEndianType endianType)
    {
        MCORE_UNUSED(version);

        // read the entry state child node index
        if (stream->Read(&mEntryStateNodeNr, sizeof(uint32)) == 0)
        {
            return false;
        }

        // convert endian if needed
        MCore::Endian::ConvertUnsignedInt32(&mEntryStateNodeNr, endianType);

        // reset the entry state pointer so that we will update it next time when calling GetEntryState() we can't call that here yet though as not all nodes are loaded
        mEntryState = nullptr;
        return true;
    }


    AnimGraphNode* AnimGraphStateMachine::GetEntryState()
    {
        const AnimGraphNodeId entryStateId = GetEntryStateId();
        if (entryStateId.IsValid())
        {
            if (!mEntryState || (mEntryState && mEntryState->GetId() != entryStateId))
            {
                // Sync the entry state based on the id.
                mEntryState = FindChildNodeById(entryStateId);
            }
        }
        else
        {
            // Legacy file format way.
            if (!mEntryState)
            {
                if (mEntryStateNodeNr != MCORE_INVALIDINDEX32 && mEntryStateNodeNr < GetNumChildNodes())
                {
                    mEntryState = GetChildNode(mEntryStateNodeNr);
                }
            }
            // End: Legacy file format way.

            // TODO: Enable this line when deprecating the leagacy file format.
            //mEntryState = nullptr;
        }

        return mEntryState;
    }


    void AnimGraphStateMachine::SetEntryState(AnimGraphNode* entryState)
    {
        mEntryState = entryState;

        if (mEntryState)
        {
            m_entryStateId = mEntryState->GetId();
        }
        else
        {
            m_entryStateId = AnimGraphNodeId::InvalidId;
        }

        // Used for the legacy file format. Get rid of this along with the old file format.
        mEntryStateNodeNr = FindChildNodeIndex(mEntryState);
    }


    // get the currently active state
    AnimGraphNode* AnimGraphStateMachine::GetCurrentState(AnimGraphInstance* animGraphInstance)
    {
        // get the unique data for this state machine in a given anim graph instance
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueNodeData(this));
        if (uniqueData == nullptr)
        {
            uniqueData = aznew UniqueData(this, animGraphInstance);
            uniqueData->mCurrentState = mEntryState;
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }
        return uniqueData->mCurrentState;
    }


    // get if the state machine has reached an exit state
    bool AnimGraphStateMachine::GetExitStateReached(AnimGraphInstance* animGraphInstance) const
    {
        // get the unique data for this state machine in a given anim graph instance
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueNodeData(this));
        return uniqueData->mReachedExitState;
    }


    // recursively call the on change motion set callback function
    void AnimGraphStateMachine::RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet)
    {
        for (AnimGraphStateTransition* transition : mTransitions)
        {
            transition->OnChangeMotionSet(animGraphInstance, newMotionSet);
        }

        // call the anim graph node
        AnimGraphNode::RecursiveOnChangeMotionSet(animGraphInstance, newMotionSet);
    }


    // callback that gets called before a node gets removed
    void AnimGraphStateMachine::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        // is the node to remove the entry state?
        if (mEntryState == nodeToRemove)
        {
            SetEntryState(nullptr);
        }

        for (AnimGraphStateTransition* transition : mTransitions)
        {
            transition->OnRemoveNode(animGraph, nodeToRemove);
        }

        for (AnimGraphNode* childNode : mChildNodes)
        {
            childNode->OnRemoveNode(animGraph, nodeToRemove);
        }
    }


    void AnimGraphStateMachine::RecursiveResetUniqueData(AnimGraphInstance* animGraphInstance)
    {
        ResetUniqueData(animGraphInstance);

        for (AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveResetUniqueData(animGraphInstance);
        }
    }


    void AnimGraphStateMachine::RecursiveOnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        OnUpdateUniqueData(animGraphInstance);

        for (AnimGraphNode* childNode : mChildNodes)
        {
            childNode->RecursiveOnUpdateUniqueData(animGraphInstance);
        }

        for (AnimGraphStateTransition* transition : mTransitions)
        {
            transition->OnUpdateUniqueData(animGraphInstance);
        }
    }


    // update attributes
    void AnimGraphStateMachine::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // get the unique data for this node, or create it
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            uniqueData = aznew UniqueData(this, animGraphInstance);
            uniqueData->mCurrentState = mEntryState;
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

        OnUpdateTriggerActionsUniqueData(animGraphInstance);

        // check if any of the active states are invalid and reset them if they are
        if (uniqueData->mCurrentState && FindChildNodeIndex(uniqueData->mCurrentState) == MCORE_INVALIDINDEX32)
        {
            uniqueData->mCurrentState   = nullptr;
        }
        if (uniqueData->mTargetState && FindChildNodeIndex(uniqueData->mTargetState) == MCORE_INVALIDINDEX32)
        {
            uniqueData->mTargetState    = nullptr;
        }
        if (uniqueData->mPreviousState && FindChildNodeIndex(uniqueData->mPreviousState) == MCORE_INVALIDINDEX32)
        {
            uniqueData->mPreviousState  = nullptr;
        }

        // update unique data
        // check if the currently active transition is still valid and reset it if it is not
        if (uniqueData->mTransition && (FindTransitionIndex(uniqueData->mTransition) == MCORE_INVALIDINDEX32 || FindChildNodeIndex(uniqueData->mTransition->GetSourceNode(animGraphInstance)) == MCORE_INVALIDINDEX32 || FindChildNodeIndex(uniqueData->mTransition->GetTargetNode()) == MCORE_INVALIDINDEX32))
        {
            uniqueData->mTransition = nullptr;
        }

        for (AnimGraphStateTransition* transition : mTransitions)
        {
            transition->OnUpdateUniqueData(animGraphInstance);

            const size_t numConditions = transition->GetNumConditions();
            for (size_t c = 0; c < numConditions; ++c)
            {
                AnimGraphTransitionCondition* condition = transition->GetCondition(c);
                condition->OnUpdateUniqueData(animGraphInstance);
            }

            const size_t numActions = transition->GetTriggerActionSetup().GetNumActions();
            for (size_t a = 0; a < numActions; ++a)
            {
                AnimGraphTriggerAction* action = transition->GetTriggerActionSetup().GetAction(a);
                action->OnUpdateUniqueData(animGraphInstance);
            }
        }
    }



    // reset unique data
    void AnimGraphStateMachine::UniqueData::Reset()
    {
        //mLocalTime            = 0.0f;
        mTransition         = nullptr;
        mCurrentState       = nullptr;
        mTargetState        = nullptr;
        mPreviousState      = nullptr;
        mReachedExitState   = false;
    }


    // rewind the nodes in the state machine
    void AnimGraphStateMachine::Rewind(AnimGraphInstance* animGraphInstance)
    {
        // get the unique data for this state machine in a given anim graph instance
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));

        // get the entry state, this function call is needed as we have to update the pointer based on the node number
        AnimGraphNode* entryState = GetEntryState();

        // call the base class rewind
        AnimGraphNode::Rewind(animGraphInstance);
        uniqueData->SetPreSyncTime(uniqueData->GetCurrentPlayTime());

        // rewind the state machine
        if (m_alwaysStartInEntryState && entryState)
        {
            if (uniqueData->mCurrentState)
            {
                uniqueData->mCurrentState->OnStateExit(animGraphInstance, entryState, nullptr);
                uniqueData->mCurrentState->OnStateEnd(animGraphInstance, entryState, nullptr);

                GetEventManager().OnStateExit(animGraphInstance, uniqueData->mCurrentState);
                GetEventManager().OnStateEnd(animGraphInstance, uniqueData->mCurrentState);
            }

            // rewind the entry state and reset conditions of all outgoing transitions
            entryState->Rewind(animGraphInstance);
            ResetOutgoingTransitionConditions(animGraphInstance, entryState);

            mEntryState->OnStateEntering(animGraphInstance, uniqueData->mCurrentState, nullptr);
            mEntryState->OnStateEnter(animGraphInstance, uniqueData->mCurrentState, nullptr);
            GetEventManager().OnStateEntering(animGraphInstance, entryState);
            GetEventManager().OnStateEnter(animGraphInstance, entryState);

            // reset the the unique data of the state machine and overwrite the current state as that is not nullptr but the entry state
            uniqueData->Reset();
            uniqueData->mCurrentState = entryState;
        }
    }


    // recursively reset flags
    void AnimGraphStateMachine::RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToDisable)
    {
        // clear the output for all child nodes, just to make sure
        for (const AnimGraphNode* childNode : mChildNodes)
        {
            animGraphInstance->DisableObjectFlags(childNode->GetObjectIndex(), flagsToDisable);
        }

        // disable it for this node
        animGraphInstance->DisableObjectFlags(mObjectIndex, flagsToDisable);

        // get the unique data for this state machine in a given anim graph instance
        AnimGraphNode* stateA = nullptr;
        AnimGraphNode* stateB = nullptr;
        GetActiveStates(animGraphInstance, &stateA, &stateB);

        if (stateA)
        {
            stateA->RecursiveResetFlags(animGraphInstance, flagsToDisable);
        }

        if (stateB)
        {
            stateB->RecursiveResetFlags(animGraphInstance, flagsToDisable);
        }
    }


    bool AnimGraphStateMachine::GetIsDeletable() const
    {
        // Only the root state machine is not deletable.
        return GetParentNode() != nullptr;
    }


    // reset all conditions for all outgoing transitions of the given state
    void AnimGraphStateMachine::ResetOutgoingTransitionConditions(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        for (AnimGraphStateTransition* transition : mTransitions)
        {
            // get the transition, check if it is a possible outgoing connection for our given state and reset it in this case
            if (transition->GetIsWildcardTransition() || (transition->GetIsWildcardTransition() == false && transition->GetSourceNode() == state))
            {
                transition->ResetConditions(animGraphInstance);
            }
        }
    }


    // count the number of incoming transitions
    uint32 AnimGraphStateMachine::CalcNumIncomingTransitions(AnimGraphNode* state) const
    {
        uint32 result = 0;

        for (const AnimGraphStateTransition* transition : mTransitions)
        {
            // get the transition and check if the target node is the given node, if yes increase our counter
            if (transition->GetTargetNode() == state)
            {
                result++;
            }
        }

        return result;
    }


    // count the number of attached wildcard transitions
    uint32 AnimGraphStateMachine::CalcNumWildcardTransitions(AnimGraphNode* state) const
    {
        uint32 result = 0;

        for (const AnimGraphStateTransition* transition : mTransitions)
        {
            // get the transition and check if the target node is the given node and we're dealing with a wildcard transition, if yes increase our counter
            if (transition->GetIsWildcardTransition() && transition->GetTargetNode() == state)
            {
                result++;
            }
        }

        return result;
    }


    // count the number of outgoing transitions
    uint32 AnimGraphStateMachine::CalcNumOutgoingTransitions(AnimGraphNode* state) const
    {
        uint32 result = 0;

        for (const AnimGraphStateTransition* transition : mTransitions)
        {
            // get the transition and check if the source node is the given node, if yes increase our counter
            if (transition->GetIsWildcardTransition() == false && transition->GetSourceNode() == state)
            {
                result++;
            }
        }

        return result;
    }


    // collect internal objects
    void AnimGraphStateMachine::RecursiveCollectObjects(MCore::Array<AnimGraphObject*>& outObjects) const
    {
        for (const AnimGraphStateTransition* transition : mTransitions)
        {
            transition->RecursiveCollectObjects(outObjects); // this will automatically add all transition conditions as well
        }
        // add the node and its children
        AnimGraphNode::RecursiveCollectObjects(outObjects);
    }


    void AnimGraphStateMachine::RecursiveCollectObjectsOfType(const AZ::TypeId& objectType, AZStd::vector<AnimGraphObject*>& outObjects) const
    {
        AnimGraphNode::RecursiveCollectObjectsOfType(objectType, outObjects);

        const size_t numTransitions = GetNumTransitions();
        for (size_t i = 0; i < numTransitions; ++i)
        {
            const AnimGraphStateTransition* transition = GetTransition(i);
            if (azrtti_istypeof(objectType, transition))
            {
                outObjects.emplace_back(const_cast<AnimGraphStateTransition*>(transition));
            }

            // get the number of conditions and iterate through them
            const size_t numConditions = transition->GetNumConditions();
            for (size_t j = 0; j < numConditions; ++j)
            {
                // check if the given condition is of the given type and add it to the output array in this case
                AnimGraphTransitionCondition* condition = transition->GetCondition(j);
                if (azrtti_istypeof(objectType, condition))
                {
                    outObjects.emplace_back(condition);
                }
            }
        }
    }


    // gather the currently processed nodes
    void AnimGraphStateMachine::GetActiveStates(AnimGraphInstance* animGraphInstance, AnimGraphNode** outStateA, AnimGraphNode** outStateB) const
    {
        // get the unique data
        const UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueNodeData(this));

        // if there is no transition
        if (uniqueData->mTransition == nullptr)
        {
            *outStateA = uniqueData->mCurrentState;
            *outStateB = nullptr;
        }
        else // there is a transition
        {
            *outStateA = uniqueData->mCurrentState;
            *outStateB = uniqueData->mTargetState;
        }
    }



    // top down update
    void AnimGraphStateMachine::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // get the active states
        AnimGraphNode* nodeA = nullptr;
        AnimGraphNode* nodeB = nullptr;
        GetActiveStates(animGraphInstance, &nodeA, &nodeB);

        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));

        // get the currently active transition
        AnimGraphStateTransition* activeTransition = GetActiveTransition(animGraphInstance);
        if (activeTransition == nullptr)
        {
            // in case no transition is active we're fully blended in a single node
            if (nodeA)
            {
                HierarchicalSyncInputNode(animGraphInstance, nodeA, uniqueData);
                nodeA->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
            }
        }
        else
        {
            // get the source and the target nodes of the currently active transition
            AnimGraphNode* sourceNode = nodeA;
            AnimGraphNode* targetNode = nodeB;

            // get the transition blend weight
            float weight = activeTransition->GetBlendWeight(animGraphInstance);

            // mark this node recursively as synced
            const ESyncMode syncMode = activeTransition->GetSyncMode();
            if (syncMode != SYNCMODE_DISABLED)
            {
                if (animGraphInstance->GetIsObjectFlagEnabled(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_SYNCED) == false)
                {
                    sourceNode->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, true);
                    animGraphInstance->SetObjectFlags(sourceNode->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_IS_SYNCMASTER, true);
                    targetNode->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, true);
                }

                //bool useInternal = true;
                //if (nodeA->GetNumChildNodes() > 0)
                //useInternal = false;

                HierarchicalSyncInputNode(animGraphInstance, sourceNode, uniqueData);
                targetNode->AutoSync(animGraphInstance, sourceNode, weight, syncMode, false, true);
            }
            else
            {
                // child node speed propagation in case the transition is set to not syncing the states
                sourceNode->SetPlaySpeed(animGraphInstance, uniqueData->GetPlaySpeed());
                targetNode->SetPlaySpeed(animGraphInstance, uniqueData->GetPlaySpeed());
            }

            sourceNode->FindUniqueNodeData(animGraphInstance)->SetGlobalWeight(uniqueData->GetGlobalWeight() * (1.0f - weight));
            sourceNode->FindUniqueNodeData(animGraphInstance)->SetLocalWeight(1.0f - weight);
            sourceNode->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);

            // update both top-down
            targetNode->FindUniqueNodeData(animGraphInstance)->SetGlobalWeight(uniqueData->GetGlobalWeight() * weight);
            targetNode->FindUniqueNodeData(animGraphInstance)->SetLocalWeight(weight);
            targetNode->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
        }
    }

    // constructor
    AnimGraphStateMachine::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
        Reset();
    }


    // recursively set object data flag
    void AnimGraphStateMachine::RecursiveSetUniqueDataFlag(AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled)
    {
        // set the state machine flag
        animGraphInstance->SetObjectFlags(mObjectIndex, flag, enabled);

        // get the active states
        AnimGraphNode* nodeA = nullptr;
        AnimGraphNode* nodeB = nullptr;
        GetActiveStates(animGraphInstance, &nodeA, &nodeB);

        // update the nodes
        if (nodeA)
        {
            nodeA->RecursiveSetUniqueDataFlag(animGraphInstance, flag, enabled);
        }

        if (nodeB)
        {
            nodeB->RecursiveSetUniqueDataFlag(animGraphInstance, flag, enabled);
        }
    }



    // recursively collect active animgraph nodes
    void AnimGraphStateMachine::RecursiveCollectActiveNodes(AnimGraphInstance* animGraphInstance, MCore::Array<AnimGraphNode*>* outNodes, const AZ::TypeId& nodeType) const
    {
        // check and add this node
        if (azrtti_typeid(this) == nodeType || nodeType.IsNull())
        {
            if (animGraphInstance->GetIsOutputReady(mObjectIndex)) // if we processed this node
            {
                outNodes->Add(const_cast<AnimGraphStateMachine*>(this));
            }
        }

        // get the active states
        AnimGraphNode* nodeA = nullptr;
        AnimGraphNode* nodeB = nullptr;
        GetActiveStates(animGraphInstance, &nodeA, &nodeB);

        // recurse into the active nodes
        if (nodeA)
        {
            nodeA->RecursiveCollectActiveNodes(animGraphInstance, outNodes, nodeType);
        }

        if (nodeB)
        {
            nodeB->RecursiveCollectActiveNodes(animGraphInstance, outNodes, nodeType);
        }
    }
    

    void AnimGraphStateMachine::RecursiveCollectActiveNetTimeSyncNodes(AnimGraphInstance * animGraphInstance, AZStd::vector<AnimGraphNode*>* outNodes) const
    {
        // get the active states
        AnimGraphNode* nodeA = nullptr;
        AnimGraphNode* nodeB = nullptr;
        GetActiveStates(animGraphInstance, &nodeA, &nodeB);

        // recurse into the active nodes
        if (nodeA)
        {
            nodeA->RecursiveCollectActiveNetTimeSyncNodes(animGraphInstance, outNodes);
        }

        if (nodeB)
        {
            nodeB->RecursiveCollectActiveNetTimeSyncNodes(animGraphInstance, outNodes);
        }
    }


    void AnimGraphStateMachine::ReserveTransitions(size_t numTransitions)
    {
        mTransitions.reserve(numTransitions);
    }

    void AnimGraphStateMachine::SetEntryStateId(AnimGraphNodeId entryStateId)
    {
        m_entryStateId = entryStateId;
    }

    void AnimGraphStateMachine::SetAlwaysStartInEntryState(bool alwaysStartInEntryState)
    {
        m_alwaysStartInEntryState = alwaysStartInEntryState;
    }

    void AnimGraphStateMachine::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphStateMachine, AnimGraphNode>()
            ->Version(1)
            ->Field("entryStateId", &AnimGraphStateMachine::m_entryStateId)
            ->Field("transitions", &AnimGraphStateMachine::mTransitions)
            ->Field("alwaysStartInEntryState", &AnimGraphStateMachine::m_alwaysStartInEntryState);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphStateMachine>("State Machine", "State machine attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphStateMachine::m_alwaysStartInEntryState, "Always Start In Entry State", "Set state machine back to entry state when it gets activated?");
    }
} // namespace EMotionFX
