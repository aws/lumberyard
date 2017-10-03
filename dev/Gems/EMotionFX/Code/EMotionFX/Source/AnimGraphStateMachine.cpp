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
#include "EventManager.h"
#include "AnimGraphStateMachine.h"
#include "BlendTree.h"
#include "AnimGraph.h"
#include "AnimGraphInstance.h"
#include "ActorInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphStateTransition.h"
#include "AnimGraphTransitionCondition.h"
#include "AnimGraphExitNode.h"
#include "AnimGraphNodeGroup.h"
#include "AnimGraphEntryNode.h"
#include "AnimGraphManager.h"
#include <MCore/Source/AttributeSettings.h>


namespace EMotionFX
{
    // constructor
    AnimGraphStateMachine::AnimGraphStateMachine(AnimGraph* animGraph, const char* name)
        : AnimGraphNode(animGraph, name, TYPE_ID)
    {
        mEntryState         = nullptr;
        mEntryStateNodeNr   = MCORE_INVALIDINDEX32;

        // allocate space for the variables
        CreateAttributeValues();
        RegisterPorts();
        InitInternalAttributesForAllInstances();

        mNodeMask.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_STATEMACHINES);
        mTransitions.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_STATEMACHINES);
    }


    // the destructor
    AnimGraphStateMachine::~AnimGraphStateMachine()
    {
        // NOTE: base class automatically removes all child nodes (states)
        RemoveAllTransitions();
    }


    // create
    AnimGraphStateMachine* AnimGraphStateMachine::Create(AnimGraph* animGraph, const char* name)
    {
        return new AnimGraphStateMachine(animGraph, name);
    }


    // create unique data
    AnimGraphObjectData* AnimGraphStateMachine::CreateObjectData()
    {
        return new UniqueData(this, nullptr);
    }


    // remove all transitions
    void AnimGraphStateMachine::RemoveAllTransitions()
    {
        // get the number of transitions, iterate through and remove them from memory
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            mTransitions[i]->Destroy();
        }

        // clear the transition array
        mTransitions.Clear();
    }


    // register the ports
    void AnimGraphStateMachine::RegisterPorts()
    {
        // setup the ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    // register the parameters
    void AnimGraphStateMachine::RegisterAttributes()
    {
        MCore::AttributeSettings* attrib = RegisterAttribute("Always Start In Entry State", "activateInEntryState", "Set state machine back to entry state when it gets activated?", MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX);
        attrib->SetDefaultValue(MCore::AttributeFloat::Create(1));
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
    #ifdef EMFX_EMSTUDIOBUILD
        SetHasError(animGraphInstance, false);
    #endif

        // in case no state is currently active and evaluated, use the bind pose
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));
        if (uniqueData->mCurrentState == nullptr)
        {
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
        #ifdef EMFX_EMSTUDIOBUILD
            if (GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
            }
        #endif

            return;
        }

        AnimGraphPose* outputPose;

        // get the current transition
        AnimGraphStateTransition* transition = uniqueData->mTransition;
        if (transition == nullptr)
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
            if (uniqueData->mCurrentState->GetMainOutputPose(animGraphInstance) && uniqueData->mTargetState->GetMainOutputPose(animGraphInstance)) // TODO: check why this is needed
            {
                transition->CalcTransitionOutput(animGraphInstance, *uniqueData->mCurrentState->GetMainOutputPose(animGraphInstance), *uniqueData->mTargetState->GetMainOutputPose(animGraphInstance), outputPose);
            }
            uniqueData->mCurrentState->DecreaseRef(animGraphInstance);
            uniqueData->mTargetState->DecreaseRef(animGraphInstance);
        }

        // visualize it
    #ifdef EMFX_EMSTUDIOBUILD
        if (GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose->GetPose(), mVisualizeColor);
        }
    #endif
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
        const uint32                numTransitions          = mTransitions.GetLength();
        const bool                  isTransitioning         = uniqueData->mTransition != nullptr;
        const uint32                sourceNodeID            = sourceNode->GetID();

        // iterate through the transitions
        //MCore::LogInfo("***** %s numT=%d", GetName(), numTransitions);
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            // get the current transition and skip it directly if in case it is disabled
            AnimGraphStateTransition* curTransition = mTransitions[i];
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

            //if (curTransition->GetSourceNode())
            //MCore::LogInfo("%s ---> %s", curTransition->GetSourceNode()->GetName(), curTransition->GetTargetNode()->GetName());

            // check allowed node groups for wildcard transitions
            if (isWildcardTransition)
            {
                // default to skip this transition
                bool skipTransition = true;

                // get the state filter attribute
                MCore::Attribute* stateFilterAttribute = curTransition->GetAttribute(AnimGraphStateTransition::ATTRIB_ALLOWEDSTATES);
                AttributeStateFilterLocal* stateFilter = static_cast<AttributeStateFilterLocal*>(stateFilterAttribute);

                // get the number of node groups and iterate through them
                const uint32 numGroups = stateFilter->GetNumNodeGroups();
                for (uint32 g = 0; g < numGroups; ++g)
                {
                    // find the node group object based on the string id rather than using string comparisons and search by name
                    AnimGraphNodeGroup* nodeGroup = mAnimGraph->FindNodeGroupByNameID(stateFilter->GetNodeGroupNameID(g));
                    if (nodeGroup == nullptr)
                    {
                        continue;
                    }

                    // enable the checking the conditions of this transition in case any of the node groups contains the source node
                    if (nodeGroup->Contains(sourceNodeID))
                    {
                        skipTransition = false;
                        break;
                    }
                }

                // only check the allowed states in case the source node isn't already in one of the allowed node groups
                // get the number of allowed states
                const uint32 numStates = stateFilter->GetNumNodes();
                if (skipTransition)
                {
                    // enable the checking the conditions of this transition in case the source node is inside the allowed states array
                    for (uint32 s = 0; s < numStates; ++s)
                    {
                        if (stateFilter->GetNodeNameID(s) == sourceNodeID)
                        {
                            skipTransition = false;
                            break;
                        }
                    }
                }

                // check the conditions of this transition in case no node groups are specified, no node groups mean all nodes are allowed
                if (numGroups == 0 && numStates == 0)
                {
                    skipTransition = false;
                }

                // skip checking this transition in case the allowed node groups don't contain our source node
                if (skipTransition)
                {
                    continue;
                }
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

        // get the number of transitions and iterate through them
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            // get the current transition and skip it directly if in case it is disabled
            AnimGraphStateTransition* transition = mTransitions[i];
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

            // get the number of conditions assigned to the transition and iterate through them
            const uint32 numConditions = transition->GetNumConditions();
            for (uint32 j = 0; j < numConditions; ++j)
            {
                // update the transition condition
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


    // update
    void AnimGraphStateMachine::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // get the unique data for this state machine in a given anim graph instance
        UniqueData* uniqueData = static_cast<UniqueData*>(FindUniqueNodeData(animGraphInstance));

        //////////////////////////////////////////////////////////////////////////
        // PHASE 1: Update conditions and check which of them are ready to transit
        //////////////////////////////////////////////////////////////////////////

        // enable the exit state reached flag when are entering an exit state or if the current state is an exit state
        if ((uniqueData->mCurrentState && uniqueData->mCurrentState->GetType() == AnimGraphExitNode::TYPE_ID) ||
            (uniqueData->mTargetState && uniqueData->mTargetState->GetType() == AnimGraphExitNode::TYPE_ID))
        {
            uniqueData->mReachedExitState = true;
        }
        else
        {
            uniqueData->mReachedExitState = false;
        }

        // update all conditions from the transitions that start from the current state
        UpdateConditions(animGraphInstance, uniqueData->mCurrentState, uniqueData, timePassedInSeconds);

        // check all transition conditions starting from the current node
        CheckConditions(uniqueData->mCurrentState, animGraphInstance, uniqueData);

        //////////////////////////////////////////////////////////////////////////
        // PHASE 2: Check for transitions we need to start or states we need to jump in
        //////////////////////////////////////////////////////////////////////////

        // if there is currently no transition going on
        /*AnimGraphStateTransition* transition = uniqueData->mTransition;
        if (transition == nullptr)
        {
            // check if we need to start transitioning
            transition = FindNextTransition( animGraphInstance, uniqueData->mCurrentState, uniqueData->mTargetState );
            if (transition)
                StartTransition( animGraphInstance, uniqueData, transition );
        }*/

        //////////////////////////////////////////////////////////////////////////
        // PHASE 3: In case we are currently transitioning check if the transition is done and activate the target state
        //////////////////////////////////////////////////////////////////////////
        AnimGraphStateTransition* transition = uniqueData->mTransition;
        if (transition)
        {
            AnimGraphNode* targetState = uniqueData->mTargetState;

            // if the transition has finished with transitioning
            if (transition->GetIsDone(animGraphInstance))
            {
                transition->OnEndTransition(animGraphInstance);
                uniqueData->mCurrentState->OnStateEnd(animGraphInstance, targetState, transition);
                targetState->OnStateEnter(animGraphInstance, uniqueData->mCurrentState, transition);

                GetEventManager().OnEndTransition(animGraphInstance, transition);
                GetEventManager().OnStateEnd(animGraphInstance, uniqueData->mCurrentState);
                GetEventManager().OnStateEnter(animGraphInstance, targetState);

                // DO NOT RESET THE OUTGOING TRANSITIONS WHEN FINISHING A TRANSITION, as it could be that one of the conditions from the target state already triggered and we then
                // directly want to start that transition, if we reset here all that information will be lost
                //ResetOutgoingTransitionConditions(animGraphInstance, targetState);

                // reset the conditions of the transition that has just ended
                transition->ResetConditions(animGraphInstance);

                uniqueData->mPreviousState  = uniqueData->mCurrentState;
                uniqueData->mCurrentState   = targetState;
                uniqueData->mTargetState    = nullptr;
                uniqueData->mTransition     = nullptr;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // PHASE 4: Update the the currently active states and transitions
        //////////////////////////////////////////////////////////////////////////
        //MCORE_ASSERT( (uniqueData->mCurrentState != uniqueData->mTargetState) || (uniqueData->mCurrentState == nullptr && uniqueData->mTargetState == nullptr) );

        // update the state we're currently in
        if (uniqueData->mCurrentState)
        {
            uniqueData->mCurrentState->IncreasePoseRefCount(animGraphInstance);
            uniqueData->mCurrentState->IncreaseRefDataRefCount(animGraphInstance);
            uniqueData->mCurrentState->PerformUpdate(animGraphInstance, timePassedInSeconds);
        }

        // update the target state
        if (uniqueData->mTargetState && (uniqueData->mCurrentState != uniqueData->mTargetState))
        {
            uniqueData->mTargetState->IncreasePoseRefCount(animGraphInstance);
            uniqueData->mTargetState->IncreaseRefDataRefCount(animGraphInstance);
            uniqueData->mTargetState->PerformUpdate(animGraphInstance, timePassedInSeconds);
        }

        // if we are currently already transitioning
        if (transition)
        {
            // update the transition
            // NOTE: do not update the transition before updating the current and the target states
            transition->Update(animGraphInstance, timePassedInSeconds);

            // update all conditions from the transitions that start from the target state
            UpdateConditions(animGraphInstance, uniqueData->mTargetState, uniqueData, timePassedInSeconds);

            // check transitions conditions starting from the target node, if one of these fires we'll start a transit while transitioning
            CheckConditions(uniqueData->mTargetState, animGraphInstance, uniqueData, false);

            // when reaching this line it means no state is currently fully blended in so we can return directly as we don't need to check the conditions
            // from the state we started transitioning from
            //return;
        }

        // get the active states
        AnimGraphNode* nodeA = nullptr;
        AnimGraphNode* nodeB = nullptr;
        GetActiveStates(animGraphInstance, &nodeA, &nodeB);

        // update the sync tracks (this goes recursively)
        if (nodeA)
        {
            uniqueData->Init(animGraphInstance, nodeA);

            if (transition && nodeB)
            {
                // get the blend weight from the transition
                const float transitionWeight = transition->GetBlendWeight(animGraphInstance);

                // interpolate the playspeeds of the two currently active and transitioning nodes based on the transition blend weight and set it to the unique data
                //const float interpolatedPlaySpeed = MCore::LinearInterpolate<float>(nodeA->GetPlaySpeed(animGraphInstance), nodeB->GetPlaySpeed(animGraphInstance), transitionWeight );
                //uniqueData->SetPlaySpeed( interpolatedPlaySpeed );

                // output the correct play speed
                float factorA;
                float factorB;
                float playSpeed;
                const ESyncMode syncMode = (ESyncMode)transition->GetSyncMode();
                AnimGraphNode::CalcSyncFactors(animGraphInstance, nodeA, nodeB, syncMode, transitionWeight, &factorA, &factorB, &playSpeed);
                uniqueData->SetPlaySpeed(playSpeed * factorA);
            }
        }
        else
        {
            uniqueData->Clear();
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

        // inform the event manager
        GetEventManager().OnStateExit(animGraphInstance, uniqueData->mCurrentState);
        GetEventManager().OnStateEntering(animGraphInstance, targetState);
        GetEventManager().OnStateEnd(animGraphInstance, uniqueData->mCurrentState);
        GetEventManager().OnStateEnter(animGraphInstance, targetState);

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
        mTransitions.Add(transition);
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

        uint32 i;

        // TODO: optimize by giving each animgraph node also an array of transitions?

        ///////////////////////////////////////////////////////////////////////
        // PASS 1: Check if there is a direct connection to the target state
        ///////////////////////////////////////////////////////////////////////

        // variables that will hold the prioritized transition information
        int32                       highestPriority         = -1;
        AnimGraphStateTransition*  prioritizedTransition   = nullptr;

        // first check if there is a ready transition that points directly to the target state
        const uint32 numTransitions = mTransitions.GetLength();
        for (i = 0; i < numTransitions; ++i)
        {
            // get the current transition and skip it directly if in case it is disabled
            AnimGraphStateTransition* transition = mTransitions[i];
            if (transition->GetIsDisabled())
            {
                continue;
            }

            // only do normal state transitions that end in the desired final anim graph node, no wildcard transitions
            if (transition->GetIsWildcardTransition() == false && transition->GetSourceNode() == currentState && transition->GetTargetNode() == targetState)
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
        for (i = 0; i < numTransitions; ++i)
        {
            // get the current transition and skip it directly if in case it is disabled
            AnimGraphStateTransition* transition = mTransitions[i];
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


    // find a transition index based on the identification value
    uint32 AnimGraphStateMachine::FindTransitionIndexByID(uint32 transitionID) const
    {
        // get the number of transitions and iterate through them
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            // get the current transition and check if its id is the one we are searching for, return the index in case of success
            AnimGraphStateTransition* transition = mTransitions[i];
            if (transition->GetID() == transitionID)
            {
                return i;
            }
        }

        // failure, the transition with the given id hasn't been found
        return MCORE_INVALIDINDEX32;
    }


    // find a transition based on the identification value
    AnimGraphStateTransition* AnimGraphStateMachine::FindTransitionByID(uint32 transitionID) const
    {
        // find the transition index based on the id
        const uint32 transitionIndex = FindTransitionIndexByID(transitionID);

        // check if the index is valid and return directly in this case
        if (transitionIndex == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        // success, return the transition
        return mTransitions[transitionIndex];
    }


    // find the transition index
    uint32 AnimGraphStateMachine::FindTransitionIndex(AnimGraphStateTransition* transition) const
    {
        return mTransitions.Find(transition);
    }


    // check if the given node has a wildcard transition
    bool AnimGraphStateMachine::CheckIfHasWildcardTransition(AnimGraphNode* state) const
    {
        // get the number of transitions and iterate through them
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            // check if the given transition is a wildcard transition and if the target node is the given one
            if (mTransitions[i]->GetTargetNode() == state && mTransitions[i]->GetIsWildcardTransition())
            {
                return true;
            }
        }

        // no wildcard transition found
        return false;
    }


    // clone the state machine
    AnimGraphObject* AnimGraphStateMachine::Clone(AnimGraph* animGraph)
    {
        AnimGraphStateMachine* clone = AnimGraphStateMachine::Create(animGraph, GetName());

        // copy base settings
        CopyBaseObjectTo(clone);

        // copy entry state number
        clone->mEntryStateNodeNr = mEntryStateNodeNr;

        return clone;
    }


    // remove a given transition
    void AnimGraphStateMachine::RemoveTransition(uint32 transitionIndex, bool delFromMem)
    {
        // destroy the memory for the transition
        if (delFromMem)
        {
            mTransitions[transitionIndex]->Destroy();
        }

        // remove the transition from our array
        mTransitions.Remove(transitionIndex);
    }


    // get the size in bytes of the custom data
    uint32 AnimGraphStateMachine::GetCustomDataSize() const
    {
        return sizeof(uint32);
    }


    // write the custom data
    bool AnimGraphStateMachine::WriteCustomData(MCore::Stream* stream, MCore::Endian::EEndianType targetEndianType)
    {
        // find the child node index for the current entry state
        mEntryStateNodeNr = FindChildNodeIndex(GetEntryState());

        // convert endian
        uint32 fileEntryState = mEntryStateNodeNr;
        MCore::Endian::ConvertUnsignedInt32To(&fileEntryState, targetEndianType);

        // write the entry state child node index
        if (stream->Write(&fileEntryState, sizeof(uint32)) == 0)
        {
            return false;
        }

        return true;
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


    // get a pointer to the initial state
    AnimGraphNode* AnimGraphStateMachine::GetEntryState()
    {
        // update the entry state based on the value we read from disk file
        if (mEntryState == nullptr)
        {
            if (mEntryStateNodeNr != MCORE_INVALIDINDEX32 && mEntryStateNodeNr < GetNumChildNodes())
            {
                mEntryState = GetChildNode(mEntryStateNodeNr);
            }
        }
        //  else
        //  {
        //      if (mEntryStateNodeNr == MCORE_INVALIDINDEX32 || mEntryStateNodeNr >= GetNumChildNodes())
        //          mEntryState = nullptr;
        //  }

        return mEntryState;
    }


    // set the entry state
    void AnimGraphStateMachine::SetEntryState(AnimGraphNode* entryState)
    {
        mEntryState = entryState;

        // find the child node index for the current entry state
        mEntryStateNodeNr = FindChildNodeIndex(mEntryState);
    }


    // get the currently active state
    AnimGraphNode* AnimGraphStateMachine::GetCurrentState(AnimGraphInstance* animGraphInstance)
    {
        // get the unique data for this state machine in a given anim graph instance
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueNodeData(this));
        if (uniqueData == nullptr)
        {
            //uniqueData = new UniqueData(this, animGraphInstance);
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
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
        // get the number of transitions and iterate through them
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            mTransitions[i]->OnChangeMotionSet(animGraphInstance, newMotionSet);
        }

        // call the anim graph node
        AnimGraphNode::RecursiveOnChangeMotionSet(animGraphInstance, newMotionSet);
    }


    // recursively update all attributes
    void AnimGraphStateMachine::RecursiveUpdateAttributes()
    {
        // update data of this node
        OnUpdateAttributes();
        /*
            // get the number of transitions, iterate through them and update their data
            const uint32 numTransitions = mTransitions.GetLength();
            for (uint32 i=0; i<numTransitions; ++i)
                mTransitions[i]->OnUpdateAttributes();
        */
        // recurse through all child nodes
        const uint32 numChildNodes = mChildNodes.GetLength();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            mChildNodes[i]->RecursiveUpdateAttributes();
        }
    }


    // update everything in the state machine
    void AnimGraphStateMachine::OnUpdateAttributes()
    {
        // get the number of transitions, iterate through them and update their data
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            mTransitions[i]->OnUpdateAttributes();
        }
    }


    // callback for when we renamed a node
    void AnimGraphStateMachine::OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const MCore::String& oldName)
    {
        // get the number of transitions, iterate through them and call the callback
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            mTransitions[i]->OnRenamedNode(animGraph, node, oldName);
        }

        // get the number of child nodes, iterate through and call the callback
        const uint32 numChildNodes = mChildNodes.GetLength();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            mChildNodes[i]->OnRenamedNode(animGraph, node, oldName);
        }
    }


    // callback that gets called before a node gets removed
    void AnimGraphStateMachine::OnCreatedNode(AnimGraph* animGraph, AnimGraphNode* node)
    {
        // get the number of transitions, iterate through them and call the callback
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            mTransitions[i]->OnCreatedNode(animGraph, node);
        }

        // get the number of child nodes, iterate through and call the callback
        const uint32 numChildNodes = mChildNodes.GetLength();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            mChildNodes[i]->OnCreatedNode(animGraph, node);
        }
    }



    // callback that gets called before a node gets removed
    void AnimGraphStateMachine::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        // is the node to remove the entry state?
        if (mEntryState == nodeToRemove)
        {
            SetEntryState(nullptr);
        }

        // get the number of transitions, iterate through them and call the callback
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            mTransitions[i]->OnRemoveNode(animGraph, nodeToRemove);
        }

        // get the number of child nodes, iterate through and call the callback
        const uint32 numChildNodes = mChildNodes.GetLength();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            mChildNodes[i]->OnRemoveNode(animGraph, nodeToRemove);
        }
    }


    // recursively update all unique data
    void AnimGraphStateMachine::RecursiveOnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        OnUpdateUniqueData(animGraphInstance);

        // get the number of child nodes, iterate through and update their attributes
        const uint32 numChildNodes = mChildNodes.GetLength();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            mChildNodes[i]->RecursiveOnUpdateUniqueData(animGraphInstance);
        }
    }


    // update attributes
    void AnimGraphStateMachine::OnUpdateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        // get the unique data for this node, or create it
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindUniqueObjectData(this));
        if (uniqueData == nullptr)
        {
            //uniqueData = new UniqueData(this, animGraphInstance);
            uniqueData = (UniqueData*)GetEMotionFX().GetAnimGraphManager()->GetObjectDataPool().RequestNew(TYPE_ID, this, animGraphInstance);
            uniqueData->mCurrentState = mEntryState;
            animGraphInstance->RegisterUniqueObjectData(uniqueData);
        }

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

        //  UpdateUniqueData(animGraphInstance);

        // get the number of transitions, iterate through them and update their attributes
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            AnimGraphStateTransition* transition = mTransitions[i];
            transition->OnUpdateUniqueData(animGraphInstance);

            // for all transition conditions
            const uint32 numConditions = transition->GetNumConditions();
            for (uint32 c = 0; c < numConditions; ++c)
            {
                transition->GetCondition(c)->OnUpdateUniqueData(animGraphInstance);
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
        if (GetAttributeFloatAsBool(ATTRIB_REWIND) && entryState)
        {
            if (uniqueData->mCurrentState)
            {
                uniqueData->mCurrentState->OnStateExit(animGraphInstance, entryState, nullptr);
                uniqueData->mCurrentState->OnStateEnd(animGraphInstance, entryState, nullptr);
            }
            GetEventManager().OnStateExit(animGraphInstance, uniqueData->mCurrentState);
            GetEventManager().OnStateEnd(animGraphInstance, uniqueData->mCurrentState);

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

            // get the number of child nodes, iterate through and rewind them as well
            const uint32 numChildNodes = mChildNodes.GetLength(); // TODO: only rewind relevant states instead of all child nodes?
            for (uint32 i = 0; i < numChildNodes; ++i)
            {
                if (mChildNodes[i] != entryState) // prevent a double rewind, as we already rewinded the entry state before
                {
                    mChildNodes[i]->Rewind(animGraphInstance);
                }
            }
        }
    }


    // recursively reset flags
    void AnimGraphStateMachine::RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToDisable)
    {
        // clear the output for all child nodes, just to make sure
        const uint32 numChildNodes = mChildNodes.GetLength();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            animGraphInstance->DisableObjectFlags(mChildNodes[i]->GetObjectIndex(), flagsToDisable);
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


    // get the number of parent state machine levels
    uint32 AnimGraphStateMachine::GetHierarchyLevel(AnimGraphNode* animGraphNode)
    {
        // in case we're dealing with an invalid node or in case we reached the top most parent node
        if (animGraphNode == nullptr)
        {
            return 0;
        }

        // initialize the result and in case the given node is a state machine, increase the resulting hierarchy level by one
        uint32 result = 0;
        if (animGraphNode->GetType() == AnimGraphStateMachine::TYPE_ID)
        {
            result++;
        }

        // recurse upwards
        return result + GetHierarchyLevel(animGraphNode->GetParentNode());
    }


    // reset all conditions for all outgoing transitions of the given state
    void AnimGraphStateMachine::ResetOutgoingTransitionConditions(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        // get the number of transitions and iterate through them
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            // get the transition, check if it is a possible outgoing connection for our given state and reset it in this case
            AnimGraphStateTransition* transition = mTransitions[i];
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

        // get the number of transitions and iterate through them
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            // get the transition and check if the target node is the given node, if yes increase our counter
            AnimGraphStateTransition* transition = mTransitions[i];
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

        // get the number of transitions and iterate through them
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            // get the transition and check if the target node is the given node and we're dealing with a wildcard transition, if yes increase our counter
            AnimGraphStateTransition* transition = mTransitions[i];
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

        // get the number of transitions and iterate through them
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            // get the transition and check if the source node is the given node, if yes increase our counter
            AnimGraphStateTransition* transition = mTransitions[i];
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
        // add all transitions
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            mTransitions[i]->RecursiveCollectObjects(outObjects); // this will automatically add all transition conditions as well
        }
        // add the node and its children
        AnimGraphNode::RecursiveCollectObjects(outObjects);
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


    // recursively init
    void AnimGraphStateMachine::RecursiveInit(AnimGraphInstance* animGraphInstance)
    {
        // init the current node
        Init(animGraphInstance);

        // recurse into all child nodes
        const uint32 numChildNodes = mChildNodes.GetLength();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            mChildNodes[i]->RecursiveInit(animGraphInstance);
        }

        // recursive init all transitions
        const uint32 numTransitions = mTransitions.GetLength();
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            mTransitions[i]->RecursiveInit(animGraphInstance);
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
    void AnimGraphStateMachine::RecursiveCollectActiveNodes(AnimGraphInstance* animGraphInstance, MCore::Array<AnimGraphNode*>* outNodes, uint32 nodeTypeID) const
    {
        // check and add this node
        if (GetType() == nodeTypeID || nodeTypeID == MCORE_INVALIDINDEX32)
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
            nodeA->RecursiveCollectActiveNodes(animGraphInstance, outNodes, nodeTypeID);
        }

        if (nodeB)
        {
            nodeB->RecursiveCollectActiveNodes(animGraphInstance, outNodes, nodeTypeID);
        }
    }


    // recursive cloning post process
    void AnimGraphStateMachine::RecursiveClonePostProcess(AnimGraphNode* resultNode)
    {
        // process the base class function first
        AnimGraphNode::RecursiveClonePostProcess(resultNode);

        // now do custom things
        // get a state machine version of the result node
        MCORE_ASSERT(resultNode->GetType() == AnimGraphStateMachine::TYPE_ID);
        AnimGraphStateMachine* resultStateMachine = static_cast<AnimGraphStateMachine*>(resultNode);
        AnimGraph* resultSetup = resultNode->GetAnimGraph();

        // clone all the transitions
        const uint32 numTransitions = mTransitions.GetLength();
        resultStateMachine->ReserveTransitions(numTransitions);
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            // clone it
            AnimGraphObject* transitionClone = mTransitions[i]->RecursiveClone(resultSetup, resultStateMachine);
            MCORE_ASSERT(transitionClone->GetType() == AnimGraphStateTransition::TYPE_ID);
            AnimGraphStateTransition* resultTransition = static_cast<AnimGraphStateTransition*>(transitionClone);

            // add the transition
            resultStateMachine->AddTransition(resultTransition);
        }
    }


    void AnimGraphStateMachine::ReserveTransitions(uint32 numTransitions)
    {
        mTransitions.Reserve(numTransitions);
    }


    void AnimGraphStateMachine::SetNodeMask(const MCore::Array<uint32>& nodeMask)
    {
        mNodeMask = nodeMask;
    }


    void AnimGraphStateMachine::GetNodeMask(MCore::Array<uint32>& outMask) const
    {
        outMask = mNodeMask;
    }


    MCore::Array<uint32>& AnimGraphStateMachine::GetNodeMask()
    {
        return mNodeMask;
    }


    const MCore::Array<uint32>& AnimGraphStateMachine::GetNodeMask() const
    {
        return mNodeMask;
    }


    uint32 AnimGraphStateMachine::GetNumNodesInNodeMask() const
    {
        return mNodeMask.GetLength();
    }
} // namespace EMotionFX
