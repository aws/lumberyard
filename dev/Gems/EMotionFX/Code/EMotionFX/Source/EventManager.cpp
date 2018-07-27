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
#include "EventManager.h"
#include "MotionInstance.h"
#include "EventHandler.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraph.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(EventManager, MotionEventManagerAllocator, 0)


    // the constructor
    EventManager::EventManager()
        : BaseObject()
    {
        mRegisteredEventTypes.SetMemoryCategory(EMFX_MEMCATEGORY_EVENTS);
        mEventHandlers.SetMemoryCategory(EMFX_MEMCATEGORY_EVENTHANDLERS);
    }


    // the destructor
    EventManager::~EventManager()
    {
        // destroy all event handlers
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->Destroy();
        }
        mEventHandlers.Clear();

        mRegisteredEventTypes.Clear();
    }


    // create
    EventManager* EventManager::Create()
    {
        return aznew EventManager();
    }


    // lock the event manager
    void EventManager::Lock()
    {
        mLock.Lock();
    }


    // unlock the event manager
    void EventManager::Unlock()
    {
        mLock.Unlock();
    }


    // register event handler to the manager
    void EventManager::AddEventHandler(EventHandler* eventHandler)
    {
        mEventHandlers.Add(eventHandler);
    }


    // find the index of the given event handler
    uint32 EventManager::FindEventHandlerIndex(EventHandler* eventHandler) const
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


    // unregister event handler from the manager
    bool EventManager::RemoveEventHandler(EventHandler* eventHandler, bool delFromMem)
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


    // unregister event handler from the manager
    void EventManager::RemoveEventHandler(uint32 index, bool delFromMem)
    {
        if (delFromMem)
        {
            mEventHandlers[index]->Destroy();
        }

        mEventHandlers.Remove(index);
    }


    // register a given event
    uint32 EventManager::RegisterEventType(const char* eventType, uint32 uniqueEventID)
    {
        MCore::LockGuardRecursive lock(mLock);

        // if we want to auto-assign a unique ID to the stuff
        if (uniqueEventID == MCORE_INVALIDINDEX32)
        {
            // check if there is already an event registered with the given type, if so, return the event ID for that
            const uint32 index = FindEventTypeIndex(eventType);
            if (index != MCORE_INVALIDINDEX32) // if there is one with the same name
            {
                return mRegisteredEventTypes[index].mEventID;
            }

            // the new ID we will assign
            uint32 curID = MCORE_INVALIDINDEX32;

            // if the event type hasn't been registered yet, we have to generate a new ID for this event type
            // first try to get the last registered event ID, and add one to it, if that doesn't work, we will
            // go into the slow way of finding a free event ID
            if (mRegisteredEventTypes.GetLength() > 0)
            {
                uint32 newEventID = mRegisteredEventTypes.GetLast().mEventID + 1;
                if (CheckIfHasEventType(newEventID) == false)
                {
                    curID = newEventID;
                }
            }

            // find a free ID the slow way by trying out all numbers
            // until a free one has been found
            // but only do this when our previous quick test didn't result in a free ID
            if (curID == MCORE_INVALIDINDEX32)
            {
                const uint32 maxInt = (uint32)(1 << 31);
                curID = 0;
                while (curID < maxInt)
                {
                    // check if the ID is already in use, if not break out of the loop
                    if (CheckIfHasEventType(curID) == false)
                    {
                        break;
                    }

                    // increase the ID counter
                    ++curID;
                }
            }

            // register the event type
            mRegisteredEventTypes.AddEmpty();
            mRegisteredEventTypes.GetLast().mEventID            = curID;
            mRegisteredEventTypes.GetLast().mEventType          = eventType;

            // return the new ID that we generated
            return curID;
        }
        else
        {
            // check if there is already an event with the same name
            const uint32 index = FindEventTypeIndex(eventType);

            // if there is one with the same name
            if (index != MCORE_INVALIDINDEX32)
            {
                // if the event ID's are different, return an error
                if (mRegisteredEventTypes[index].mEventID != uniqueEventID)
                {
                    MCORE_ASSERT(false); // you are trying to register an event type with the same name, but with another ID
                    // so for example you first try to link "SOUND" with an ID of 10, and now you
                    // try to register "SOUND" again, but another ID value

                    return MCORE_INVALIDINDEX32;
                }
                else // you are registering the same event type again, so just skip the registration part and return the ID
                {
                    return uniqueEventID;
                }
            }

            // if there isn't an event with the specified name yet, make sure there is no event
            // that has the same ID yet
            const bool hasEventTypeWithSameID = CheckIfHasEventType(uniqueEventID);

            // assert in case there is an event with the same ID already, as that shouldn't happen
            MCORE_ASSERT(hasEventTypeWithSameID == false);

            // in release mode exit this function, debug mode will give an assert
            if (hasEventTypeWithSameID)
            {
                return MCORE_INVALIDINDEX32;
            }

            // register the event type
            mRegisteredEventTypes.AddEmpty();
            mRegisteredEventTypes.GetLast().mEventID            = uniqueEventID;
            mRegisteredEventTypes.GetLast().mEventType          = eventType;

            return uniqueEventID;
        }
    }



    // unregister a given event type with a given ID
    void EventManager::UnregisterEventType(uint32 eventID)
    {
        // iterate through all registered event types
        const uint32 numEvents = mRegisteredEventTypes.GetLength();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            // check if this event ID matches with the one we are searching for
            if (mRegisteredEventTypes[i].mEventID == eventID)
            {
                // remove it from the registered event collection and return
                mRegisteredEventTypes.Remove(i);
                return;
            }
        }
    }


    // unregister a given event type
    void EventManager::UnregisterEventType(const char* eventType)
    {
        // iterate through all registered events
        const uint32 numEvents = mRegisteredEventTypes.GetLength();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            // check if this event type name matches with the one we are searching for
            if (mRegisteredEventTypes[i].mEventType == eventType)
            {
                // remove it from the registered event collection and return
                mRegisteredEventTypes.Remove(i);
                return;
            }
        }
    }


    // try to locate the event ID of a given event
    uint32 EventManager::FindEventID(const char* eventType) const
    {
        // iterate through all registered events
        const uint32 numEvents = mRegisteredEventTypes.GetLength();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            if (AzFramework::StringFunc::Equal(mRegisteredEventTypes[i].mEventType.c_str(), eventType, false /* no case */))
            {
                return mRegisteredEventTypes[i].mEventID;
            }
        }

        // no event type with the given string found
        return MCORE_INVALIDINDEX32;
    }



    // check if a given event type has been registered
    bool EventManager::CheckIfHasEventType(uint32 eventID) const
    {
        // iterate through all registered events
        const uint32 numEvents = mRegisteredEventTypes.GetLength();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            if (mRegisteredEventTypes[i].mEventID == eventID)
            {
                return true;
            }
        }

        // no such event type found
        return false;
    }


    // check if a given event type has been registered
    bool EventManager::CheckIfHasEventType(const char* eventType) const
    {
        return (FindEventID(eventType) != MCORE_INVALIDINDEX32);
    }


    // get the number of registered event types
    uint32 EventManager::GetNumRegisteredEventTypes() const
    {
        return mRegisteredEventTypes.GetLength();
    }


    // get the event type ID for a given event number
    uint32 EventManager::GetEventTypeID(uint32 eventIndex) const
    {
        return mRegisteredEventTypes[eventIndex].mEventID;
    }


    // get a event type string for a given event number
    const char* EventManager::GetEventTypeString(uint32 eventIndex) const
    {
        return mRegisteredEventTypes[eventIndex].mEventType.c_str();
    }


    // get a event type string for a given event number
    const AZStd::string& EventManager::GetEventTypeStringAsString(uint32 eventIndex) const
    {
        return mRegisteredEventTypes[eventIndex].mEventType;
    }


    // find the event
    uint32 EventManager::FindEventTypeIndex(uint32 eventTypeID) const
    {
        // iterate through all registered events
        const uint32 numEvents = mRegisteredEventTypes.GetLength();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            if (mRegisteredEventTypes[i].mEventID == eventTypeID)
            {
                return i;
            }
        }

        // no such event type found
        return MCORE_INVALIDINDEX32;
    }


    // find the registered event type index for a given event type string
    uint32 EventManager::FindEventTypeIndex(const char* eventType) const
    {
        // iterate through all registered events
        const uint32 numEvents = mRegisteredEventTypes.GetLength();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            if (AzFramework::StringFunc::Equal(mRegisteredEventTypes[i].mEventType.c_str(), eventType, false /* no case */))
            {
                return i;
            }
        }

        // no such event type found
        return MCORE_INVALIDINDEX32;
    }


    // dump a list of registered events to the log system
    void EventManager::LogRegisteredEventTypes()
    {
        // get the number of events
        const uint32 numEventTypes = GetNumRegisteredEventTypes();

        MCore::LogInfo("---------------------------------");
        MCore::LogInfo("Total Registered Events = %d", GetNumRegisteredEventTypes());
        MCore::LogInfo("---------------------------------");

        // dump all events
        for (uint32 i = 0; i < numEventTypes; ++i)
        {
            MCore::LogInfo("Event #%d: Name = '%s'   -    Unique ID = %d", i, GetEventTypeString(i), GetEventTypeID(i));
        }

        MCore::LogInfo("---------------------------------");
    }


    // trigger an event in all handlers
    void EventManager::OnEvent(const EventInfo& eventInfo)
    {
        // trigger the event handlers inside the motion instance
        if (eventInfo.mMotionInstance)
        {
            eventInfo.mMotionInstance->OnEvent(eventInfo);
        }

        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnEvent(eventInfo);
        }
    }


    void EventManager::OnPlayMotion(Motion* motion, PlayBackInfo* info)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnPlayMotion(motion, info);
        }
    }


    void EventManager::OnStartMotionInstance(MotionInstance* motionInstance, PlayBackInfo* info)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnStartMotionInstance(info);

        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnStartMotionInstance(motionInstance, info);
        }
    }


    void EventManager::OnDeleteMotionInstance(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnDeleteMotionInstance();

        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnDeleteMotionInstance(motionInstance);
        }
    }


    void EventManager::OnDeleteMotion(Motion* motion)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnDeleteMotion(motion);
        }
    }


    void EventManager::OnStop(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnStop();

        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnStop(motionInstance);
        }
    }


    void EventManager::OnHasLooped(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnHasLooped();

        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnHasLooped(motionInstance);
        }
    }


    void EventManager::OnHasReachedMaxNumLoops(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnHasReachedMaxNumLoops();

        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnHasReachedMaxNumLoops(motionInstance);
        }
    }


    void EventManager::OnHasReachedMaxPlayTime(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnHasReachedMaxPlayTime();

        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnHasReachedMaxPlayTime(motionInstance);
        }
    }


    void EventManager::OnIsFrozenAtLastFrame(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnIsFrozenAtLastFrame();

        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnIsFrozenAtLastFrame(motionInstance);
        }
    }


    void EventManager::OnChangedPauseState(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnChangedPauseState();

        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnChangedPauseState(motionInstance);
        }
    }


    void EventManager::OnChangedActiveState(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnChangedActiveState();

        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnChangedActiveState(motionInstance);
        }
    }


    void EventManager::OnStartBlending(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnStartBlending();

        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnStartBlending(motionInstance);
        }
    }


    void EventManager::OnStopBlending(MotionInstance* motionInstance)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnStopBlending();

        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnStopBlending(motionInstance);
        }
    }


    void EventManager::OnQueueMotionInstance(MotionInstance* motionInstance, PlayBackInfo* info)
    {
        // trigger the event handlers inside the motion instance
        motionInstance->OnQueueMotionInstance(info);

        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnQueueMotionInstance(motionInstance, info);
        }
    }


    void EventManager::OnDeleteActor(Actor* actor)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnDeleteActor(actor);
        }
    }


    void EventManager::OnDeleteActorInstance(ActorInstance* actorInstance)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnDeleteActorInstance(actorInstance);
        }
    }


    // draw a debug line
    void EventManager::OnDrawLine(const AZ::Vector3& posA, const AZ::Vector3& posB, uint32 color)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnDrawLine(posA, posB, color);
        }
    }


    // draw a debug triangle
    void EventManager::OnDrawTriangle(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& normalA, const AZ::Vector3& normalB, const AZ::Vector3& normalC, uint32 color)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnDrawTriangle(posA, posB, posC, normalA, normalB, normalC, color);
        }
    }


    // draw the triangles that got added using OnDrawTriangles()
    void EventManager::OnDrawTriangles()
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnDrawTriangles();
        }
    }


    // simulate physics
    void EventManager::OnSimulatePhysics(float timeDelta)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnSimulatePhysics(timeDelta);
        }
    }


    void EventManager::OnCustomEvent(uint32 eventType, void* data)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnCustomEvent(eventType, data);
        }
    }


    void EventManager::OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnStateEnter(animGraphInstance, state);
        }

        // pass the callback call to the anim graph instance
        animGraphInstance->OnStateEnter(state);
    }


    void EventManager::OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnStateEntering(animGraphInstance, state);
        }

        // pass the callback call to the anim graph instance
        animGraphInstance->OnStateEntering(state);
    }


    void EventManager::OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnStateExit(animGraphInstance, state);
        }

        // pass the callback call to the anim graph instance
        animGraphInstance->OnStateExit(state);
    }


    void EventManager::OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnStateEnd(animGraphInstance, state);
        }

        // pass the callback call to the anim graph instance
        animGraphInstance->OnStateEnd(state);
    }


    void EventManager::OnStartTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnStartTransition(animGraphInstance, transition);
        }

        // pass the callback call to the anim graph instance
        animGraphInstance->OnStartTransition(transition);
    }


    void EventManager::OnEndTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnEndTransition(animGraphInstance, transition);
        }

        // pass the callback call to the anim graph instance
        animGraphInstance->OnEndTransition(transition);
    }


    // call the callbacks for when we renamed a node
    void EventManager::OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const AZStd::string& oldName)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnRenamedNode(animGraph, node, oldName);
        }
    }


    // call the callbacks for a node creation
    void EventManager::OnCreatedNode(AnimGraph* animGraph, AnimGraphNode* node)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnCreatedNode(animGraph, node);
        }
    }


    // call the callbacks for a node removal
    void EventManager::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        // get the root state machine and call the callbacks recursively
        AnimGraphStateMachine* rootSM = animGraph->GetRootStateMachine();
        rootSM->OnRemoveNode(animGraph, nodeToRemove);

        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnRemoveNode(animGraph, nodeToRemove);
        }
    }


    // call the callbacks
    void EventManager::OnRemovedChildNode(AnimGraph* animGraph, AnimGraphNode* parentNode)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnRemovedChildNode(animGraph, parentNode);
        }
    }


    void EventManager::Sync(AnimGraphInstance* animGraphInstance, AnimGraphNode* animGraphNode)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->Sync(animGraphInstance, animGraphNode);
        }
    }


    void EventManager::OnSetVisualManipulatorOffset(AnimGraphInstance* animGraphInstance, uint32 paramIndex, const AZ::Vector3& offset)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnSetVisualManipulatorOffset(animGraphInstance, paramIndex, offset);
        }
    }


    void EventManager::OnParameterNodeMaskChanged(BlendTreeParameterNode* parameterNode, const AZStd::vector<AZStd::string>& newParameterMask)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnParameterNodeMaskChanged(parameterNode, newParameterMask);
        }
    }


    void EventManager::OnProgressStart()
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnProgressStart();
            mEventHandlers[i]->OnProgressValue(0.0f);
        }
    }


    void EventManager::OnProgressEnd()
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnProgressValue(100.0f);
            mEventHandlers[i]->OnProgressEnd();
        }
    }


    void EventManager::OnProgressText(const char* text)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnProgressText(text);
        }
    }


    void EventManager::OnProgressValue(float percentage)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnProgressValue(percentage);
        }
    }


    void EventManager::OnSubProgressText(const char* text)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnSubProgressText(text);
        }
    }


    void EventManager::OnSubProgressValue(float percentage)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnSubProgressValue(percentage);
        }
    }


    // perform a ray intersection test
    bool EventManager::OnRayIntersectionTest(const AZ::Vector3& start, const AZ::Vector3& end, IntersectionInfo* outIntersectInfo)
    {
        // get the number of event handlers and iterate through them
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            bool result = mEventHandlers[i]->OnRayIntersectionTest(start, end, outIntersectInfo);
            if (outIntersectInfo->mIsValid)
            {
                return result;
            }
        }

        return false;
    }


    // create an animgraph
    void EventManager::OnCreateAnimGraph(AnimGraph* animGraph)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnCreateAnimGraph(animGraph);
        }
    }


    // create an animgraph instance
    void EventManager::OnCreateAnimGraphInstance(AnimGraphInstance* animGraphInstance)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnCreateAnimGraphInstance(animGraphInstance);
        }
    }


    // create a motion
    void EventManager::OnCreateMotion(Motion* motion)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnCreateMotion(motion);
        }
    }


    // create a motion set
    void EventManager::OnCreateMotionSet(MotionSet* motionSet)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnCreateMotionSet(motionSet);
        }
    }


    // create a motion instance
    void EventManager::OnCreateMotionInstance(MotionInstance* motionInstance)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnCreateMotionInstance(motionInstance);
        }
    }


    // create a motion system
    void EventManager::OnCreateMotionSystem(MotionSystem* motionSystem)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnCreateMotionSystem(motionSystem);
        }
    }


    // create an actor
    void EventManager::OnCreateActor(Actor* actor)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnCreateActor(actor);
        }
    }


    // create an actor instance
    void EventManager::OnCreateActorInstance(ActorInstance* actorInstance)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnCreateActorInstance(actorInstance);
        }
    }


    // on post create actor
    void EventManager::OnPostCreateActor(Actor* actor)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnPostCreateActor(actor);
        }
    }


    // delete an animgraph
    void EventManager::OnDeleteAnimGraph(AnimGraph* animGraph)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnDeleteAnimGraph(animGraph);
        }
    }


    // delete an animgraph instance
    void EventManager::OnDeleteAnimGraphInstance(AnimGraphInstance* animGraphInstance)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnDeleteAnimGraphInstance(animGraphInstance);
        }
    }


    // delete motion set
    void EventManager::OnDeleteMotionSet(MotionSet* motionSet)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnDeleteMotionSet(motionSet);
        }
    }


    // delete a motion system
    void EventManager::OnDeleteMotionSystem(MotionSystem* motionSystem)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnDeleteMotionSystem(motionSystem);
        }
    }


    // scale actor data
    void EventManager::OnScaleActorData(Actor* actor, float scaleFactor)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnScaleActorData(actor, scaleFactor);
        }
    }


    // scale motion data
    void EventManager::OnScaleMotionData(Motion* motion, float scaleFactor)
    {
        const uint32 numEventHandlers = mEventHandlers.GetLength();
        for (uint32 i = 0; i < numEventHandlers; ++i)
        {
            mEventHandlers[i]->OnScaleMotionData(motion, scaleFactor);
        }
    }

} // namespace EMotionFX
