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
#include "MemoryCategories.h"
#include "MotionInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "EventManager.h"
#include "BaseObject.h"
#include "Importer/Importer.h"


namespace EMotionFX
{
    /**
     * The event handler, which is responsible for processing the events.
     * This class contains several methods which you can overload to perform custom things event comes up.
     * You can inherit your own event handler from this base class and add it to the event manager using EMFX_EVENTMGR.AddEventHandler(...) method
     * to make it use your custom event handler.
     */
    class EMFX_API EventHandler
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * Default creation method.
         */
        static EventHandler* Create();

        /**
         * The main method that processes a event.
         * @param eventInfo The struct holding the information about the triggered event.
         */
        virtual void OnEvent(const EventInfo& eventInfo)
        {
            MCORE_UNUSED(eventInfo);
            // find the event index in the array
            //          const uint32 eventIndex  = GetEventManager().FindEventTypeIndex( eventInfo.mEventTypeID );

            // get the name of the event
            //          const char* eventName = (eventIndex != MCORE_INVALIDINDEX32) ? GetEventManager().GetEventTypeString( eventIndex ) : "<unknown>";

            // log it
            //          MCore::LogDetailedInfo("EMotionFX: *** Motion event %s at time %f of type ID %d ('%s') with parameters '%s' for actor '%s' on motion '%s' has been triggered", isEventStart ? "START" : "END", timeValue, eventTypeID, eventName, parameters, actorInstance->GetActor()->GetName(), motionInstance->GetMotion()->GetName());

            /*
                // when you implement your own event handler, of course you can just do something like a switch statement.
                // then the FindEventTypeIndex and GetEventTypeString aren't needed at all
                switch (eventTypeID)
                {
                    case EVENT_SOUND:
                        //....
                        break;

                    case EVENT_SCRIPT:
                        //....
                        break;

                    //....
                };
            */
        }

        /**
         * The event that gets triggered when a MotionSystem::PlayMotion(...) is being executed.
         * The difference between OnStartMotionInstance() and this OnPlayMotion() is that the OnPlayMotion doesn't
         * guarantee that the motion is being played yet, as it can also be added to the motion queue.
         * The OnStartMotionInstance() method will be called once the motion is really being played.
         * @param motion The motion that is being played.
         * @param info The playback info used to play the motion.
         */
        virtual void OnPlayMotion(Motion* motion, PlayBackInfo* info)                                                                       { MCORE_UNUSED(motion); MCORE_UNUSED(info); }

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
        virtual void OnStartMotionInstance(MotionInstance* motionInstance, PlayBackInfo* info)                                              { MCORE_UNUSED(motionInstance); MCORE_UNUSED(info); }

        /**
         * The event that gets triggered once a MotionInstance object is being deleted.
         * This can happen when calling the MotionSystem::RemoveMotionInstance() method manually, or when
         * EMotion FX internally removes the motion instance because it has no visual influence anymore.
         * The destructor of the MotionInstance class automatically triggers this event.
         * @param motionInstance The motion instance that is being deleted.
         */
        virtual void OnDeleteMotionInstance(MotionInstance* motionInstance)                                                                 { MCORE_UNUSED(motionInstance); }

        /**
         * The event that gets triggered once a Motion object is being deleted.
         * You could for example use this event to delete any allocations you have done inside the
         * custom user data object linked with the Motion object.
         * You can get and set this data object with the Motion::GetCustomData() and Motion::SetCustomData(...) methods.
         * @param motion The motion that is being deleted.
         */
        virtual void OnDeleteMotion(Motion* motion)                                                                                         { MCORE_UNUSED(motion); }

        /**
         * The event that gets triggered when a motion instance is being stopped using one of the MotionInstance::Stop() methods.
         * EMotion FX will internally stop the motion automatically when the motion instance reached its maximum playback time
         * or its maximum number of loops.
         * @param motionInstance The motion instance that is being stopped.
         */
        virtual void OnStop(MotionInstance* motionInstance)                                                                                 { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once a given motion instance has looped.
         * @param motionInstance The motion instance that got looped.
         */
        virtual void OnHasLooped(MotionInstance* motionInstance)                                                                            { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once a given motion instance has reached its maximum number of allowed loops.
         * In this case the motion instance will also be stopped automatically afterwards.
         * @param motionInstance The motion instance that reached its maximum number of allowed loops.
         */
        virtual void OnHasReachedMaxNumLoops(MotionInstance* motionInstance)                                                                { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once a given motion instance has reached its maximum playback time.
         * For example if this motion instance is only allowed to play for 2 seconds, and the total playback time reaches
         * two seconds, then this event will be triggered.
         * @param motionInstance The motion instance that reached its maximum playback time.
         */
        virtual void OnHasReachedMaxPlayTime(MotionInstance* motionInstance)                                                                { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once the motion instance is set to freeze at the last frame once the
         * motion reached its end (when it reached its maximum number of loops or playtime).
         * In this case this event will be triggered once.
         * @param motionInstance The motion instance that got into a frozen state.
         */
        virtual void OnIsFrozenAtLastFrame(MotionInstance* motionInstance)                                                                  { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once the motion pause state changes.
         * For example when the motion is unpaused but gets paused, then this even twill be triggered.
         * Paused motions don't get their playback times updated. They do however still perform blending, so it is
         * still possible to fade them in or out.
         * @param motionInstance The motion instance that changed its paused state.
         */
        virtual void OnChangedPauseState(MotionInstance* motionInstance)                                                                    { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once the motion active state changes.
         * For example when the motion is active but gets set to inactive using the MotionInstance::SetActive(...) method,
         * then this even twill be triggered.
         * Inactive motions don't get processed at all. They will not update their playback times, blending, nor will they take
         * part in any blending calculations of the final node transforms. In other words, it will just be like the motion instance
         * does not exist at all.
         * @param motionInstance The motion instance that changed its active state.
         */
        virtual void OnChangedActiveState(MotionInstance* motionInstance)                                                                   { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once a motion instance is automatically changing its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, then once this blending starts, this event is being triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time bigger than zero, and
         * if the motion instance isn't currently already blending, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         * @param motionInstance The motion instance which is changing its weight value over time.
         */
        virtual void OnStartBlending(MotionInstance* motionInstance)                                                                        { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once a motion instance stops it automatic changing of its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, and once the target weight is reached after half a second, will cause this event to be triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time equal to zero and the
         * motion instance is currently blending its weight value, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         * @param motionInstance The motion instance for which the automatic weight blending just stopped.
         */
        virtual void OnStopBlending(MotionInstance* motionInstance)                                                                         { MCORE_UNUSED(motionInstance); }

        /**
         * This event gets triggered once the given motion instance gets added to the motion queue.
         * This happens when you set the PlayBackInfo::mPlayNow member to false. In that case the MotionSystem::PlayMotion() method (OnPlayMotion)
         * will not directly start playing the motion (OnStartMotionInstance), but will add it to the motion queue instead.
         * The motion queue will then start playing the motion instance once it should.
         * @param motionInstance The motion instance that gets added to the motion queue.
         * @param info The playback information used to play this motion instance.
         */
        virtual void OnQueueMotionInstance(MotionInstance* motionInstance, PlayBackInfo* info)                                              { MCORE_UNUSED(motionInstance); MCORE_UNUSED(info); }

        //---------------------------------------------------------------------

        /**
         * The event that gets triggered once an Actor object is being deleted.
         * You could for example use this event to delete any allocations you have done inside the
         * custom user data object linked with the Actor object.
         * You can get and set this data object with the Actor::GetCustomData() and Actor::SetCustomData(...) methods.
         * @param actor The actor that is being deleted.
         */
        virtual void OnDeleteActor(Actor* actor)                                                                                            { MCORE_UNUSED(actor); }

        /**
         * The event that gets triggered once an ActorInstance object is being deleted.
         * You could for example use this event to delete any allocations you have done inside the
         * custom user data object linked with the ActorInstance object.
         * You can get and set this data object with the ActorInstance::GetCustomData() and ActorInstance::SetCustomData(...) methods.
         * @param actorInstance The actorInstance that is being deleted.
         */
        virtual void OnDeleteActorInstance(ActorInstance* actorInstance)                                                                    { MCORE_UNUSED(actorInstance); }

        virtual void OnSimulatePhysics(float timeDelta)                                                                                     { MCORE_UNUSED(timeDelta); }
        virtual void OnCustomEvent(uint32 eventType, void* data)                                                                            { MCORE_UNUSED(eventType); MCORE_UNUSED(data); }

        virtual void OnDrawLine(const AZ::Vector3& posA, const AZ::Vector3& posB, uint32 color)                                       { MCORE_UNUSED(posA); MCORE_UNUSED(posB); MCORE_UNUSED(color); }
        virtual void OnDrawTriangle(const AZ::Vector3& posA, const AZ::Vector3& posB, const AZ::Vector3& posC, const AZ::Vector3& normalA, const AZ::Vector3& normalB, const AZ::Vector3& normalC, uint32 color) { MCORE_UNUSED(posA); MCORE_UNUSED(posB); MCORE_UNUSED(posC); MCORE_UNUSED(normalA); MCORE_UNUSED(normalB); MCORE_UNUSED(normalC); MCORE_UNUSED(color); }
        virtual void OnDrawTriangles() {}

        // creation callbacks
        virtual void OnCreateAnimGraph(AnimGraph* animGraph)                                                                             { MCORE_UNUSED(animGraph); }
        virtual void OnCreateAnimGraphInstance(AnimGraphInstance* animGraphInstance)                                                     { MCORE_UNUSED(animGraphInstance); }
        virtual void OnCreateMotion(Motion* motion)                                                                                         { MCORE_UNUSED(motion); }
        virtual void OnCreateMotionSet(MotionSet* motionSet)                                                                                { MCORE_UNUSED(motionSet); }
        virtual void OnCreateMotionInstance(MotionInstance* motionInstance)                                                                 { MCORE_UNUSED(motionInstance); }
        virtual void OnCreateMotionSystem(MotionSystem* motionSystem)                                                                       { MCORE_UNUSED(motionSystem); }
        virtual void OnCreateActor(Actor* actor)                                                                                            { MCORE_UNUSED(actor); }
        virtual void OnCreateActorInstance(ActorInstance* actorInstance)                                                                    { MCORE_UNUSED(actorInstance); }
        virtual void OnPostCreateActor(Actor* actor)                                                                                        { MCORE_UNUSED(actor); }

        // delete callbacks
        virtual void OnDeleteAnimGraph(AnimGraph* animGraph)                                                                             { MCORE_UNUSED(animGraph); }
        virtual void OnDeleteAnimGraphInstance(AnimGraphInstance* animGraphInstance)                                                     { MCORE_UNUSED(animGraphInstance); }
        virtual void OnDeleteMotionSet(MotionSet* motionSet)                                                                                { MCORE_UNUSED(motionSet); }
        virtual void OnDeleteMotionSystem(MotionSystem* motionSystem)                                                                       { MCORE_UNUSED(motionSystem); }

        /**
         * Perform a ray intersection test and return the intersection info.
         * The first event handler registered that sets the IntersectionInfo::mIsValid to true will be outputting to the outIntersectInfo parameter.
         * @param start The start point, in global space.
         * @param end The end point, in global space.
         * @param outIntersectInfo The resulting intersection info.
         * @result Returns true when an intersection occurred and false when no intersection occurred.
         */
        virtual bool OnRayIntersectionTest(const AZ::Vector3& start, const AZ::Vector3& end, IntersectionInfo* outIntersectInfo)      { MCORE_UNUSED(start); MCORE_UNUSED(end); MCORE_UNUSED(outIntersectInfo); return false; }

        virtual void OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                                 { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                              { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                                  { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                                   { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStartTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition)                            { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(transition); }
        virtual void OnEndTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition)                              { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(transition); }
        virtual void Sync(AnimGraphInstance* animGraphInstance, AnimGraphNode* animGraphNode)                                                { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(animGraphNode); }
        virtual void OnSetVisualManipulatorOffset(AnimGraphInstance* animGraphInstance, uint32 paramIndex, const AZ::Vector3& offset)       { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(paramIndex); MCORE_UNUSED(offset); }
        virtual void OnParameterNodeMaskChanged(BlendTreeParameterNode* parameterNode, const AZStd::vector<AZStd::string>& newParameterMask)                                                      { MCORE_UNUSED(parameterNode); }

        virtual void OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const AZStd::string& oldName)                              { MCORE_UNUSED(animGraph); MCORE_UNUSED(node); MCORE_UNUSED(oldName); }
        virtual void OnCreatedNode(AnimGraph* animGraph, AnimGraphNode* node)                                                            { MCORE_UNUSED(animGraph); MCORE_UNUSED(node); }
        virtual void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)                                                     { MCORE_UNUSED(animGraph); MCORE_UNUSED(nodeToRemove); }
        virtual void OnRemovedChildNode(AnimGraph* animGraph, AnimGraphNode* parentNode)                                                 { MCORE_UNUSED(animGraph); MCORE_UNUSED(parentNode); }

        virtual void OnProgressStart()                                                                                                      {}
        virtual void OnProgressEnd()                                                                                                        {}
        virtual void OnProgressText(const char* text)                                                                                       { MCORE_UNUSED(text); }
        virtual void OnProgressValue(float percentage)                                                                                      { MCORE_UNUSED(percentage); }
        virtual void OnSubProgressText(const char* text)                                                                                    { MCORE_UNUSED(text); }
        virtual void OnSubProgressValue(float percentage)                                                                                   { MCORE_UNUSED(percentage); }

        virtual void OnScaleActorData(Actor* actor, float scaleFactor)                                                                      { MCORE_UNUSED(actor); MCORE_UNUSED(scaleFactor); }
        virtual void OnScaleMotionData(Motion* motion, float scaleFactor)                                                                   { MCORE_UNUSED(motion); MCORE_UNUSED(scaleFactor); }

    protected:
        /**
         * The constructor.
         */
        EventHandler();

        /**
         * The destructor.
         */
        virtual ~EventHandler();
    };


    /**
     * The per anim graph instance event handlers.
     * This allows you to capture events triggered on a specific anim graph instance, rather than globally.
     */
    class EMFX_API AnimGraphInstanceEventHandler
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        static AnimGraphInstanceEventHandler* Create();

        virtual void OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                                 { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                              { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                                  { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* state)                                                   { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(state); }
        virtual void OnStartTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition)                            { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(transition); }
        virtual void OnEndTransition(AnimGraphInstance* animGraphInstance, AnimGraphStateTransition* transition)                              { MCORE_UNUSED(animGraphInstance); MCORE_UNUSED(transition); }

    protected:
        AnimGraphInstanceEventHandler();
        virtual ~AnimGraphInstanceEventHandler();
    };


    /**
     * The per motion instance event handlers.
     * This allows you to capture events triggered on a specific motion instance, rather than globally.
     */
    class EMFX_API MotionInstanceEventHandler
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        static MotionInstanceEventHandler* Create();

        void SetMotionInstance(MotionInstance* motionInstance)          { mMotionInstance = motionInstance; }
        MCORE_INLINE MotionInstance* GetMotionInstance()                { return mMotionInstance; }

        /**
         * The method that processes an event.
         * @param eventInfo The struct holding the information about the triggered event.
         */
        virtual void OnEvent(const EventInfo& eventInfo)                { MCORE_UNUSED(eventInfo); }

        /**
         * The event that gets triggered when a motion instance is really being played.
         * This can be a manual call through MotionInstance::PlayMotion or when the MotionQueue class
         * will start playing a motion that was on the queue.
         * The difference between OnStartMotionInstance() and this OnPlayMotion() is that the OnPlayMotion doesn't
         * guarantee that the motion is being played yet, as it can also be added to the motion queue.
         * The OnStartMotionInstance() method will be called once the motion is really being played.
         * @param info The playback info used to play the motion.
         */
        virtual void OnStartMotionInstance(PlayBackInfo* info)          { MCORE_UNUSED(info); }

        /**
         * The event that gets triggered once a MotionInstance object is being deleted.
         * This can happen when calling the MotionSystem::RemoveMotionInstance() method manually, or when
         * EMotion FX internally removes the motion instance because it has no visual influence anymore.
         * The destructor of the MotionInstance class automatically triggers this event.
         */
        virtual void OnDeleteMotionInstance() {}

        /**
         * The event that gets triggered when a motion instance is being stopped using one of the MotionInstance::Stop() methods.
         * EMotion FX will internally stop the motion automatically when the motion instance reached its maximum playback time
         * or its maximum number of loops.
         */
        virtual void OnStop() {}

        /**
         * This event gets triggered once a given motion instance has looped.
         */
        virtual void OnHasLooped() {}

        /**
         * This event gets triggered once a given motion instance has reached its maximum number of allowed loops.
         * In this case the motion instance will also be stopped automatically afterwards.
         */
        virtual void OnHasReachedMaxNumLoops() {}

        /**
         * This event gets triggered once a given motion instance has reached its maximum playback time.
         * For example if this motion instance is only allowed to play for 2 seconds, and the total playback time reaches
         * two seconds, then this event will be triggered.
         */
        virtual void OnHasReachedMaxPlayTime() {}

        /**
         * This event gets triggered once the motion instance is set to freeze at the last frame once the
         * motion reached its end (when it reached its maximum number of loops or playtime).
         * In this case this event will be triggered once.
         */
        virtual void OnIsFrozenAtLastFrame() {}

        /**
         * This event gets triggered once the motion pause state changes.
         * For example when the motion is unpaused but gets paused, then this even twill be triggered.
         * Paused motions don't get their playback times updated. They do however still perform blending, so it is
         * still possible to fade them in or out.
         */
        virtual void OnChangedPauseState() {}

        /**
         * This event gets triggered once the motion active state changes.
         * For example when the motion is active but gets set to inactive using the MotionInstance::SetActive(...) method,
         * then this even twill be triggered.
         * Inactive motions don't get processed at all. They will not update their playback times, blending, nor will they take
         * part in any blending calculations of the final node transforms. In other words, it will just be like the motion instance
         * does not exist at all.
         */
        virtual void OnChangedActiveState() {}

        /**
         * This event gets triggered once a motion instance is automatically changing its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, then once this blending starts, this event is being triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time bigger than zero, and
         * if the motion instance isn't currently already blending, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         */
        virtual void OnStartBlending() {}

        /**
         * This event gets triggered once a motion instance stops it automatic changing of its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, and once the target weight is reached after half a second, will cause this event to be triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time equal to zero and the
         * motion instance is currently blending its weight value, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         */
        virtual void OnStopBlending() {}

        /**
         * This event gets triggered once the given motion instance gets added to the motion queue.
         * This happens when you set the PlayBackInfo::mPlayNow member to false. In that case the MotionSystem::PlayMotion() method (OnPlayMotion)
         * will not directly start playing the motion (OnStartMotionInstance), but will add it to the motion queue instead.
         * The motion queue will then start playing the motion instance once it should.
         * @param info The playback information used to play this motion instance.
         */
        virtual void OnQueueMotionInstance(PlayBackInfo* info)      { MCORE_UNUSED(info); }

    protected:
        MotionInstance* mMotionInstance;

        MotionInstanceEventHandler();
        virtual ~MotionInstanceEventHandler();
    };
}   // namespace EMotionFX
