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
#include "BaseObject.h"
#include <MCore/Source/Array.h>
#include <MCore/Source/MultiThreadManager.h>
#include "MemoryCategories.h"
#include "MotionInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "AnimGraphNode.h"
#include "BlendTreeParameterNode.h"
#include "AnimGraphStateTransition.h"
#include "EventInfo.h"


namespace EMotionFX
{
    // forward declarations
    class EventHandler;

    /**
     * Intersection information, used by the event system, to return the results of an intersection test.
     */
    struct EMFX_API IntersectionInfo
    {
        AZ::Vector3     mPosition;
        AZ::Vector3     mNormal;
        AZ::Vector2     mUV;
        float           mBaryCentricU;
        float           mBaryCentricV;
        ActorInstance*  mActorInstance;
        ActorInstance*  mIgnoreActorInstance;
        Node*           mNode;
        Mesh*           mMesh;
        uint32          mStartIndex;
        bool            mIsValid;

        IntersectionInfo()
        {
            mPosition = AZ::Vector3::CreateZero();
            mNormal.Set(0.0f, 1.0f, 0.0f);
            mUV = AZ::Vector2::CreateZero();
            mBaryCentricU   = 0.0f;
            mBaryCentricV   = 0.0f;
            mActorInstance  = nullptr;
            mStartIndex     = 0;
            mIgnoreActorInstance = nullptr;
            mNode           = nullptr;
            mMesh           = nullptr;
            mIsValid        = false;
        }
    };


    /**
     * The event manager, which is used to specify which event handler to use.
     * You can use the macro named GetEventManager() to quickly access this singleton class.
     * If you want to override the way events are processed, you have to create your own
     * class inherited from the EventHandler base class.
     * @see EventHandler.
     * @see MotionEvent.
     * @see MotionEventTable.
     */
    class EMFX_API EventManager
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

        friend class Initializer;
        friend class EMotionFXManager;

    public:
        static EventManager* Create();

        void Lock();
        void Unlock();

        /**
         * Add event handler to the manager.
         * @param eventHandler The new event handler to register, this must not be nullptr.
         */
        void AddEventHandler(EventHandler* eventHandler);

        /**
         * Find the index for the given event handler.
         * @param[in] eventHandler A pointer to the event handler to search.
         * @return The index of the event handler inside the event manager. MCORE_INVALIDINDEX32 in case the event handler has not been found.
         */
        uint32 FindEventHandlerIndex(EventHandler* eventHandler) const;

        /**
         * Remove the given event handler.
         * @param eventHandler A pointer to the event handler to remove.
         * @param delFromMem When set to true, the event handler will be deleted from memory automatically when removing it.
         */
        bool RemoveEventHandler(EventHandler* eventHandler, bool delFromMem = true);

        /**
         * Remove the event handler at the given index.
         * @param index The index of the event handler to remove.
         * @param delFromMem When set to true, the event handler will be deleted from memory automatically when removing it.
         */
        void RemoveEventHandler(uint32 index, bool delFromMem = true);

        /**
         * Get the event handler at the given index.
         * @result A pointer to the event handler at the given index.
         */
        MCORE_INLINE EventHandler* GetEventHandler(uint32 index) const      { return mEventHandlers[index]; }

        /**
         * Get the number of registered event handlers, which you can use to loop over the event handlers using the GetEventHandler method.
         * @result The number of event handlers.
         */
        MCORE_INLINE uint32 GetNumEventHandlers() const                     { return mEventHandlers.GetLength(); }

        /**
         * Register a specific even type.
         * This links a specific event type string with a specified integer ID value.
         * This integer ID value can then be used for much faster checks to see what event is being triggered.
         * When the event ID is set to MCORE_INVALIDINDEX32 this will automatically assign a free ID to the to be
         * registered event.
         * If you register an event type that has already been registered before, nothing will happen and the ID
         * of the given event will be returned (and not the one specified as second parameter).
         * It is not possible to register the same event type again, but with another ID. If you do this, in release mode
         * a value of MCORE_INVALIDINDEX32 will be returned, and in debug mode an assert will happen.
         * @param eventType The string identifying the event type. This is not case sensitive.
         * @param uniqueEventID The event ID to link with this, or MCORE_INVALIDINDEX32 to automatically assign a new ID.
         * @result The ID that has been assigned to the event type. This is different from the parameter value that you
         *         specify to "uniqueEventID" in case it is set to MCORE_INVALIDINDEX32. A value of MCORE_INVALIDINDEX32
         *         will be returned when something went wrong.
         */
        uint32 RegisterEventType(const char* eventType, uint32 uniqueEventID = MCORE_INVALIDINDEX32);

        /**
         * Unregister the event of a given type.
         * @param eventID The event ID to unregister.
         */
        void UnregisterEventType(uint32 eventID);

        /**
         * Unregister a specific event type, based on its event string.
         * @param eventType The string containing the event type. This is not case sensitive.
         */
        void UnregisterEventType(const char* eventType);

        /**
         * Find the ID for a given event, based on a string.
         * @param eventType The string containing the event type. This is not case sensitive.
         * @result The event ID linked with this string or MCORE_INVALIDINDEX32 when it has not been found.
         */
        uint32 FindEventID(const char* eventType) const;

        /**
         * Check if there are any events registered with the given ID.
         * @param eventID The ID to check for.
         * @result Returns true when there is an event registered using the specified ID. Otherwise false is returned.
         */
        bool CheckIfHasEventType(uint32 eventID) const;

        /**
         * Check if there are any events registered with the given even type.
         * @param eventType The event type string to check for.
         * @result Returns true when there is an event registered using the specified event type string. Otherwise false is returned.
         */
        bool CheckIfHasEventType(const char* eventType) const;

        /**
         * Get the number of registered event types.
         * @result The number of registered event types.
         */
        uint32 GetNumRegisteredEventTypes() const;

        /**
         * Get the event ID of a given event number.
         * @param eventIndex The event number, which must be in range of [0..GetNumRegisteredEventTypes()-1].
         * @result The ID of the event number.
         */
        uint32 GetEventTypeID(uint32 eventIndex) const;

        /**
         * Get the event type string of a given event number.
         * @param eventIndex The event number, which must be in range of [0..GetNumRegisteredEventTypes()-1].
         * @result The string of the event type.
         */
        const char* GetEventTypeString(uint32 eventIndex) const;

        /**
         * Get the event type string of a given event number.
         * @param eventIndex The event number, which must be in range of [0..GetNumRegisteredEventTypes()-1].
         * @result The string of the event type.
         */
        const AZStd::string& GetEventTypeStringAsString(uint32 eventIndex) const;

        /**
         * Find the event type number for an event with the given type ID.
         * @param eventTypeID The event type ID to search for.
         * @result The event type number, which is in range of [0..GetNumRegisteredEventTypes()-1], or MCORE_INVALIDINDEX32 when not found.
         */
        uint32 FindEventTypeIndex(uint32 eventTypeID) const;

        /**
         * Find the event type number for an event with the given type string.
         * @param eventType The event type string to search for.
         * @result The event type number, which is in range of [0..GetNumRegisteredEventTypes()-1], or MCORE_INVALIDINDEX32 when not found.
         */
        uint32 FindEventTypeIndex(const char* eventType) const;

        /**
         * Log all registered event types using the MCore::LOG function.
         */
        void LogRegisteredEventTypes();

        //---------------------------------------------------------------------

        /**
         * The main method that processes an event.
         * @param eventInfo The struct holding the information about the triggered event.
         */
        void OnEvent(const EventInfo& eventInfo);

        /**
         * The event that gets triggered when a MotionSystem::PlayMotion(...) is being executed.
         * The difference between OnStartMotionInstance() and this OnPlayMotion() is that the OnPlayMotion doesn't
         * guarantee that the motion is being played yet, as it can also be added to the motion queue.
         * The OnStartMotionInstance() method will be called once the motion is really being played.
         * @param motion The motion that is being played.
         * @param info The playback info used to play the motion.
         */
        void OnPlayMotion(Motion* motion, PlayBackInfo* info);

        /**
         * The event that gets triggered when a motion instance is really being played.
         * This can be a manual call through MotionInstance::PlayMotion or when the MotionQueue class
         * will start playing a motion that was on the queue.
         * The difference between OnStartMotionInstance() and this OnPlayMotion() is that the OnPlayMotion doesn't
         * guarantee that the motion is being played yet, as it can also be added to the motion queue.
         * The OnStartMotionInstance() method will be called once the motion is really being played.
         * @param motionInstance The motion instance that is being played.
         * @param info The playback info used to play the motion.
         */
        void OnStartMotionInstance(MotionInstance* motionInstance, PlayBackInfo* info);

        /**
         * The event that gets triggered once a MotionInstance object is being deleted.
         * This can happen when calling the MotionSystem::RemoveMotionInstance() method manually, or when
         * EMotion FX internally removes the motion instance because it has no visual influence anymore.
         * The destructor of the MotionInstance class automatically triggers this event.
         * @param motionInstance The motion instance that is being deleted.
         */
        void OnDeleteMotionInstance(MotionInstance* motionInstance);

        /**
         * The event that gets triggered once a Motion object is being deleted.
         * You could for example use this event to delete any allocations you have done inside the
         * custom user data object linked with the Motion object.
         * You can get and set this data object with the Motion::GetCustomData() and Motion::SetCustomData(...) methods.
         * @param motion The motion that is being deleted.
         */
        void OnDeleteMotion(Motion* motion);

        /**
         * The event that gets triggered when a motion instance is being stopped using one of the MotionInstance::Stop() methods.
         * EMotion FX will internally stop the motion automatically when the motion instance reached its maximum playback time
         * or its maximum number of loops.
         * @param motionInstance The motion instance that is being stopped.
         */
        void OnStop(MotionInstance* motionInstance);

        /**
         * This event gets triggered once a given motion instance has looped.
         * @param motionInstance The motion instance that got looped.
         */
        void OnHasLooped(MotionInstance* motionInstance);

        /**
         * This event gets triggered once a given motion instance has reached its maximum number of allowed loops.
         * In this case the motion instance will also be stopped automatically afterwards.
         * @param motionInstance The motion instance that reached its maximum number of allowed loops.
         */
        void OnHasReachedMaxNumLoops(MotionInstance* motionInstance);

        /**
         * This event gets triggered once a given motion instance has reached its maximum playback time.
         * For example if this motion instance is only allowed to play for 2 seconds, and the total playback time reaches
         * two seconds, then this event will be triggered.
         * @param motionInstance The motion instance that reached its maximum playback time.
         */
        void OnHasReachedMaxPlayTime(MotionInstance* motionInstance);

        /**
         * This event gets triggered once the motion instance is set to freeze at the last frame once the
         * motion reached its end (when it reached its maximum number of loops or playtime).
         * In this case this event will be triggered once.
         * @param motionInstance The motion instance that got into a frozen state.
         */
        void OnIsFrozenAtLastFrame(MotionInstance* motionInstance);

        /**
         * This event gets triggered once the motion pause state changes.
         * For example when the motion is unpaused but gets paused, then this even twill be triggered.
         * Paused motions don't get their playback times updated. They do however still perform blending, so it is
         * still possible to fade them in or out.
         * @param motionInstance The motion instance that changed its paused state.
         */
        void OnChangedPauseState(MotionInstance* motionInstance);

        /**
         * This event gets triggered once the motion active state changes.
         * For example when the motion is active but gets set to inactive using the MotionInstance::SetActive(...) method,
         * then this even twill be triggered.
         * Inactive motions don't get processed at all. They will not update their playback times, blending, nor will they take
         * part in any blending calculations of the final node transforms. In other words, it will just be like the motion instance
         * does not exist at all.
         * @param motionInstance The motion instance that changed its active state.
         */
        void OnChangedActiveState(MotionInstance* motionInstance);

        /**
         * This event gets triggered once a motion instance is automatically changing its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, then once this blending starts, this event is being triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time bigger than zero, and
         * if the motion instance isn't currently already blending, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         * @param motionInstance The motion instance which is changing its weight value over time.
         */
        void OnStartBlending(MotionInstance* motionInstance);

        /**
         * This event gets triggered once a motion instance stops it automatic changing of its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, and once the target weight is reached after half a second, will cause this event to be triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time equal to zero and the
         * motion instance is currently blending its weight value, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         * @param motionInstance The motion instance for which the automatic weight blending just stopped.
         */
        void OnStopBlending(MotionInstance* motionInstance);

        /**
         * This event gets triggered once the given motion instance gets added to the motion queue.
         * This happens when you set the PlayBackInfo::mPlayNow member to false. In that case the MotionSystem::PlayMotion() method (OnPlayMotion)
         * will not directly start playing the motion (OnStartMotionInstance), but will add it to the motion queue instead.
         * The motion queue will then start playing the motion instance once it should.
         * @param motionInstance The motion instance that gets added to the motion queue.
         * @param info The playback information used to play this motion instance.
         */
        void OnQueueMotionInstance(MotionInstance* motionInstance, PlayBackInfo* info);

        //---------------------------------------------------------------------

        /**
         * The event that gets triggered once an Actor object is being deleted.
         * You could for example use this event to delete any allocations you have done inside the
         * custom user data object linked with the Actor object.
         * You can get and set this data object with the Actor::GetCustomData() and Actor::SetCustomData(...) methods.
         * @param actor The actor that is being deleted.
         */
        void OnDeleteActor(Actor* actor);

        /**
         * The event that gets triggered once an ActorInstance object is being deleted.
         * You could for example use this event to delete any allocations you have done inside the
         * custom user data object linked with the ActorInstance object.
         * You can get and set this data object with the ActorInstance::GetCustomData() and ActorInstance::SetCustomData(...) methods.
         * @param actorInstance The actorInstance that is being deleted.
         */
        void OnDeleteActorInstance(ActorInstance* actorInstance);

        void OnSimulatePhysics(float timeDelta);
        void OnCustomEvent(uint32 eventType, void* data);
        void OnDrawLine(const AZ::Vector3& posA, const AZ::Vector3& posB, uint32 color);
        void OnDrawTriangle(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& normalA, const AZ::Vector3& normalB, const AZ::Vector3& normalC, uint32 color);
        void OnDrawTriangles();

        void OnScaleActorData(Actor* actor, float scaleFactor);
        void OnScaleMotionData(Motion* motion, float scaleFactor);

        /**
         * Perform a ray intersection test and return the intersection info.
         * The first event handler registered that sets the IntersectionInfo::mIsValid to true will be outputting to the outIntersectInfo parameter.
         * @param start The start point, in global space.
         * @param end The end point, in global space.
         * @param outIntersectInfo The resulting intersection info.
         * @result Returns true when an intersection occurred and false when no intersection occurred.
         */
        bool OnRayIntersectionTest(const AZ::Vector3& start, const AZ::Vector3& end, IntersectionInfo* outIntersectInfo);

        void OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* state);
        void OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* state);
        void OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* state);
        void OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* state);
        void OnStartTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition);
        void OnEndTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition);
        void Sync(AnimGraphInstance* animGraphInstance, AnimGraphNode* animGraphNode);
        void OnSetVisualManipulatorOffset(AnimGraphInstance* animGraphInstance, uint32 paramIndex, const AZ::Vector3& offset);
        void OnParameterNodeMaskChanged(BlendTreeParameterNode* parameterNode, const AZStd::vector<AZStd::string>& newParameterMask);

        void OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const AZStd::string& oldName);
        void OnCreatedNode(AnimGraph* animGraph, AnimGraphNode* node);
        void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove);
        void OnRemovedChildNode(AnimGraph* animGraph, AnimGraphNode* parentNode);

        void OnProgressStart();
        void OnProgressEnd();
        void OnProgressText(const char* text);
        void OnProgressValue(float percentage);
        void OnSubProgressText(const char* text);
        void OnSubProgressValue(float percentage);

        void OnCreateAnimGraph(AnimGraph* animGraph);
        void OnCreateAnimGraphInstance(AnimGraphInstance* animGraphInstance);
        void OnCreateMotion(Motion* motion);
        void OnCreateMotionSet(MotionSet* motionSet);
        void OnCreateMotionInstance(MotionInstance* motionInstance);
        void OnCreateMotionSystem(MotionSystem* motionSystem);
        void OnCreateActor(Actor* actor);
        void OnCreateActorInstance(ActorInstance* actorInstance);
        void OnPostCreateActor(Actor* actor);

        // delete callbacks
        void OnDeleteAnimGraph(AnimGraph* animGraph);
        void OnDeleteAnimGraphInstance(AnimGraphInstance* animGraphInstance);
        void OnDeleteMotionSet(MotionSet* motionSet);
        void OnDeleteMotionSystem(MotionSystem* motionSystem);


    private:
        /**
         * The structure that links a given event type name with a unique ID.
         * The unique ID is passed to the ProcessEvent function to eliminate string compares to check what type of
         * event has been triggered.
         */
        struct EMFX_API RegisteredEventType
        {
            AZStd::string   mEventType;                             /**< The string that describes the event, this is what artists type in 3DSMax/Maya. */
            uint32          mEventID;                               /**< The unique ID for this event. */
        };

        MCore::Array<EventHandler*>         mEventHandlers;         /**< The event handler to use to process events. */
        MCore::Array<RegisteredEventType>   mRegisteredEventTypes;  /**< A collection of registered events. */
        MCore::MutexRecursive               mLock;

        /**
         * The constructor, which will initialize the manager to use the default dummy event handler.
         */
        EventManager();

        /**
         * The destructor, which deletes the event handler automatically.
         */
        ~EventManager();
    };
} // namespace EMotionFX
