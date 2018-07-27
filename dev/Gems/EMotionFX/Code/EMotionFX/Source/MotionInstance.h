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
#include "Motion.h"
#include "Pose.h"
#include "MotionLink.h"
#include "MotionGroup.h"
#include "PlayBackInfo.h"
#include "EventInfo.h"
#include "MotionInstancePool.h"

#include <MCore/Source/Array.h>
#include <MCore/Source/CompressedFloat.h>


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class MotionInstanceEventHandler;
    class AnimGraphEventBuffer;

    /**
     * The MotionInstance class.
     * Since Motion objects can be shared between different Actors, there needs to be a mechanism which allows this.
     * By introducing this MotionInstance class, we can create instances from Motions, where the instance also contains
     * playback information. This playback information allows us to play the same animation data at different actors
     * all with unique play positions and speeds, etc.
     */
    class EMFX_API MotionInstance
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL
        friend class RepositioningLayerPass;
        friend class AnimGraphMotionNode;
        friend class MotionInstancePool;

    public:
        static MotionInstance* Create(Motion* motion, ActorInstance* actorInstance, uint32 startNodeIndex);
        static MotionInstance* Create(void* memLocation, Motion* motion, ActorInstance* actorInstance, uint32 startNodeIndex);

        // init it for sampling
        void InitForSampling();
        bool GetIsReadyForSampling() const;

        /**
         * Update the motion info.
         * @param timePassed The time passed, in seconds.
         */
        void Update(float timePassed);

        /**
         * Update based on an old and new time value.
         * This will update the motion instance internally as it was previously at oldTime and now has progressed towards newTime.
         * This does not simply change the current time value, but really detects loops, increasing loop counts, triggering events, etc.
         * @param oldTime The previous time value of the current motion time.
         * @param newTime The new current motion time.
         * @param outEventBuffer The output event buffer. This can be nullptr if you want to skip triggering events.
         */
        void UpdateByTimeValues(float oldTime, float newTime, AnimGraphEventBuffer* outEventBuffer);

        // process events
        void ProcessEvents(float oldTime, float newTime);

        // extract events to be executed
        void ExtractEvents(float oldTime, float newTime, AnimGraphEventBuffer* outBuffer);
        void ExtractEventsNonLoop(float oldTime, float newTime, AnimGraphEventBuffer* outBuffer);

        /**
         * Set the custom data pointer.
         * This can be used to link your own custom data to this MotionInstance object.
         * You are responsible later on for deleting any allocated data by this custom data.
         * You can use the MotionEventHandler::OnDeleteMotionInstance() callback to detect when a motion instance is being deleted.
         * @param customDataPointer The pointer to your custom data.
         */
        void SetCustomData(void* customDataPointer);

        /**
         * Get the custom data pointer value.
         * This can be used to link your own custom data to this MotionInstance object.
         * You are responsible later on for deleting any allocated data by this custom data.
         * You can use the MotionEventHandler::OnDeleteMotionInstance() callback to detect when a motion instance is being deleted.
         * On default the custom data value will return nullptr, unless you have used SetCustomData() to adjust the value.
         * @result The pointer to your custom data that is linked to this MotionInstance object.
         */
        void* GetCustomData() const;

        /**
         * Get the unique identification number for the motion instance.
         * @return The unique identification number.
         */
        MCORE_INLINE uint32 GetID() const                                       { return mID; }

        /**
         * Get the blend in time.
         * This is the time passed to the SetWeight(...) method where when the target weight is bigger than the current.
         * So only blend ins are counted and not blending out towards for example a weight of 0.
         * When you never call SetWeight(...) yourself, this means that this will contain the value specificied to PlayBackInfo::mBlendInTime
         * at the time of MotionSystem::PlayMotion(...).
         * @result The blend-in time, in seconds.
         */
        float GetBlendInTime() const;

        /**
         * Returns the passed time since the last update.
         * @result The passed time since the last update, in seconds.
         */
        float GetPassedTime() const;

        /**
         * Returns the current time in the playback of the motion.
         * @result The current time, in seconds.
         */
        float GetCurrentTime() const;

        /**
         * Get the maximum time of this motion.
         * @result The maximum time of this motion, in seconds.
         */
        float GetMaxTime() const;

        /**
         * Get the duration of the motion, which is the difference between the clip start and end time.
         * @result The playback duration of this motion instance, in seconds.
         */
        float GetDuration() const;

        /**
         * Return the play speed factor (1.0 is normal, 0.5 is half speed, etc.).
         * @result The play speed factor.
         */
        float GetPlaySpeed() const;

        /**
         * Returns motion it is using.
         * @result The original motion it's using.
         */
        Motion* GetMotion() const;

        void SetMotion(Motion* motion);

        /**
         * Get the motion group object.
         * If the number of motion instances inside this group equals zero, no motion grouping is used.
         * You can also use the IsGroupedMotion() method to check this.
         * @result The motion group object inside this motion.
         * @see IsGroupedMotion.
         */
        MotionGroup* GetMotionGroup() const;

        /**
         * Checks if this motion is a grouped motion or not.
         * A grouped motion can contain any number of motion instances that get blend together using normalized weighting.
         * For example you can put four motions in a group, with their weight values summing up to 1. Now the result of this group
         * will be the blend of those four motions, which will be the output of this motion instance. The main motion instance (which is this object)
         * can control the weight of the blend output of the motion group as well, like all regular motion instances.
         * @result Returns true when this is a grouped motion, which is when the group returned by GetMotionGroup() has one or more motion instances in it. Otherwise false is returned.
         */
        bool GetIsGroupedMotion() const;

        /**
         * Set the current time in the animation (automatic wrapping/looping performed when out of range).
         * Normalized in this case means from 0.0 to 1.0. The maximum time of this animation is then 1.0.
         * @param normalizedTimeValue The new normalized time value, in seconds.
         */
        void SetCurrentTimeNormalized(float normalizedTimeValue);

        /**
         * Returns the current time in the playback of the motion. Normalized in this
         * case means from 0.0 to 1.0. The maximum time of this animation is then 1.0.
         * @result The normalized current time, in seconds.
         */
        float GetCurrentTimeNormalized() const;

        /**
         * Set the current time in the animation (automatic wrapping/looping performed when out of range).
         * @param time The current time in the animation, in seconds.
         * @param resetLastAndPastTime When set to true, the last frame time will be set equal to the new current time as well, and the passed time will be set to zero.
         */
        void SetCurrentTime(float time, bool resetLastAndPastTime = true);

        void InitMotionGroup();
        void RemoveMotionGroup();

        /**
         * Get the current time of the previous update.
         * @result The time value, in seconds, of the current playback time, in the previous update.
         */
        float GetLastCurrentTime() const;

        /**
         * Set the passed time of the animation (automatic wrapping/looping performed when out of range).
         * @param timePassed The passed time in the animation, in seconds.
         */
        void SetPassedTime(float timePassed);

        /**
         * Set the current play speed (1.0 is normal, 0.5 is half speed, etc.).
         * The speed has to be bigger or equal to 0. You should not use negative playback speeds.
         * If you want to play backward, use the SetPlayMode( PLAYMODE_BACKWARD ), or use the PlayBackInfo::mPlayMode value.
         * @param speed The current play speed (1.0 is normal, 0.5 is half speed, etc.).
         */
        void SetPlaySpeed(float speed);

        /**
         * Set the play mode, which defines the direction the motion is playing (forward or backward).
         * @param mode The playback mode to use.
         */
        void SetPlayMode(EPlayMode mode);

        /**
         * Get the play mode, which defines the deireciton the motion is playing (foward or backward).
         * @result The playback mode.
         */
        EPlayMode GetPlayMode() const;

        /**
         * Updates the current play time value.
         * This is automatically called.
         */
        void UpdateTime(float timePassed);

        /**
         * Set the fade-out time.
         * @param fadeTime The fade time, in seconds.
         */
        void SetFadeTime(float fadeTime);

        /**
         * Return the time spend to fade out the motion when it is being stopped automatically
         * when the motion has reached its end. This will only happen when the play mode is PLAYMODE_ONCE.
         * @result The fade time, in seconds.
         */
        float GetFadeTime() const;

        /**
         * Get the motion blending mode of this motion instance.
         * This describes how the motion gets blend with the other motions being played.
         * For more information about what the blendmodes exactly do, read the documentation of the SetBlendMode method.
         * @result The motion blend mode for this motion instance.
         * @see SetBlendMode
         */
        EMotionBlendMode GetBlendMode() const;

        /**
         * Returns the current weight of the layer.
         * This weight is in range of [0..1], where 0 means no influence and 1 means full influence.
         * @result The current weight value.
         */
        float GetWeight() const;

        /**
         * Returns the target weight. This is the weight we are blending towards.
         * If you specified a blendTimeInSeconds of zero in the SetWeight method, the target weight will return the same
         * as the value returned by GetWeight().
         * The value returned is in range of [0..1].
         * @result The target weight value, where we are blending towards.
         */
        float GetTargetWeight() const;

        /**
         * Set the target weight value.
         * This can be used to smoothly blend towards another weight value.
         * You specify the new (target) weight value, and the time in seconds in which we should blend into that weight.
         * A weight value of 0 means no influence, and a weight value of 1 means full influence.
         * Please keep in mind that motion layers inside the motion layer system will automatically be removed when we are in overwrite motion blend mode
         * and this motion reaches full influence. In order to prevent this from happening, you can blend towards a weight of for example 0.999. This will not
         * have any visual difference compared to a weight of 1, but will prevent motion instances and layers from being removed.
         * The same goes for motion weights of 0. Instead of motion weights of 0, you can use values like 0.001 in these cases.
         * @param targetWeight The weight value we want to blend towards.
         * @param blendTimeInSeconds The time, in seconds, in which we should blend from the current weight value into the specified target weight value.
         */
        void SetWeight(float targetWeight, float blendTimeInSeconds = 0);

        /**
         * Set the motion blend mode of this motion instance.
         * If you want to switch between two different motions, for example going from walk into run, you most likely
         * want to use the BLENDMODE_OVERWRITE mode. However, there are some cases where overwriting doesn't work well.
         * Think of a skateboard game, where you play a looping base animation, which is played while moving forwards.
         * Now on top of this you want to bend the character's upper body to the right. So you play a mixing motion of
         * the upper body. You now have a character, which is skate boarding, while bending his upper body to the right.
         * Now imagine you want to have some motion which shoots. You want the character to shoot, while it is skating and
         * while it is has bend his upper body to the right. If you would use overwrite mode, and the shoot animation adjusts
         * the bones in the upper body, the motion of the bend will be overwritten. This means you will only see a shoot animation
         * while skating, and not the desired, shoot, while bending right, while skating.
         * In order to solve this, you can play the shoot motion additive, on top of the bend and skate motions. EMotion FX will then
         * add all relative changes (compared to the original pose of the actor) to the current result, which in our case is the
         * skating, while bending right.
         * Playing an additive motion in mix-mode, will act the same as playing one in non-mixing mode.
         * @param mode The blendmode to use. The default blendmode of motion instances are set to BLEND_OVERWRITE.
         * @see EMotionBlendMode
         */
        void SetBlendMode(EMotionBlendMode mode);

        /**
         * Enable or disable motion mirroring.
         * On default motion mirroring is disabled.
         * Motion mirroring is often very useful in sport games, where you can choose whether your character is left or right-handed.
         * The motion mirroring feature allows you to create just one set of motions, for example right handed.
         * By enabling mirroring of EMotion FX we can turn the right handed motions into left handed motions on the fly, by using the right handed motion source data
         * and modifying it into a left handed motion. This does not really take more memory. Of course there is a little performance impact, but it is definitely worth
         * the memory savings and art time savings.
         * When mirroring is enabled the motion mirror plane normal is used to determine how to mirror the motion. This can be set with SetMirrorPlaneNormal().
         * @param enabled Set to true if you want to enable motion mirroring, or false when you want to disable it.
         */
        void SetMirrorMotion(bool enabled);

        /**
         * On default motion mirroring is disabled, so set to false.
         * Motion mirroring is often very useful in sport games, where you can choose whether your character is left or right-handed.
         * The motion mirroring feature allows you to create just one set of motions, for example right handed.
         * By enabling mirroring of EMotion FX we can turn the right handed motions into left handed motions on the fly, by using the right handed motion source data
         * and modifying it into a left handed motion. This does not really take more memory. Of course there is a little performance impact, but it is definitely worth
         * the memory savings and art time savings.
         * @result Returns true when motion mirroring is enabled, or false when it is disabled. On default it is disabled.
         */
        bool GetMirrorMotion() const;

        /**
         * Rewinds the motion instance. It sets the current time to 0 seconds.
         */
        void Rewind();

        /**
         * Check if this motion instance has ended or not.
         * This will only happen when the play mode is PLAYMODE_ONCE, because a looping motion will of course never end.
         * @result Returns true when the motion has reached the end of the animation, otherwise false is returned.
         */
        bool GetHasEnded() const;

        /**
         * Set the motion to mix mode or not.
         * @param mixModeEnabled Set to true when the motion has to mix, otherwise set to false.
         */
        void SetMixMode(bool mixModeEnabled);

        /**
         * Checks if the motion is currently stopping or not, so if it is fading out.
         * @result Returns true when the motion is currently stopping, so is fading out.
         */
        bool GetIsStopping() const;

        /**
         * Checks if the motion is currently playing or not.
         * @result Returns true when the motion is playing, otherwise false.
         */
        bool GetIsPlaying() const;

        /**
         * Checks if the motion is in mix mode or not.
         * @result Returns true when the motion is being mixed, otherwise false.
         */
        bool GetIsMixing() const;

        /**
         * Checks if the motion is being blended or not.
         * A blend could be the smooth fade in after starting the motion, or a smooth fade out when stopping the animation.
         * @result Returns true when the motion is currently in a blend process, otherwise false is returned.
         */
        bool GetIsBlending() const;

        /**
         * Pause the motion instance.
         */
        void Pause();

        /**
         * Unpause the motion instance.
         */
        void UnPause();

        /**
         * Set the pause mode.
         * @param pauseEnabled When true, the motion will be set to pause, else it will be unpaused.
         */
        void SetPause(bool pauseEnabled);

        /**
         * Check if the motion currently is paused or not.
         * @result Returns true when the motion is in pause mode.
         */
        bool GetIsPaused() const;

        /**
         * Set the number of loops the motion should play.
         * If you want to loop it forever, set the value to EMFX_LOOPFOREVER (which is defined in Actor.h).
         * @param numLoops The number of loops the motion should play, or EMFX_LOOPFOREVER in case it should play forever.
         */
        void SetMaxLoops(uint32 numLoops);

        /**
         * Get the number of loops the motion will play.
         * @result The number of times the motion will be played. This value will be EMFX_LOOPFOREVER (see Actor.h for the define) in case
         *         the motion will play forever.
         * @see IsPlayingForever
         */
        uint32 GetMaxLoops() const;

        /**
         * Check if the motion has looped since the last update.
         * This is the case when the number of loops returned by GetNumCurrentLoops is not equal to the number of loops before the
         * playback mode object has been updated.
         * @result Returns true when the motion has looped, otherwise false is returned.
         */
        bool GetHasLooped() const;

        /**
         * Set the new number of times the motion has been played. Changing this value will misrepresent the exact number.
         * @param numCurrentLoops The number of times the motion has been played.
         */
        void SetNumCurrentLoops(uint32 numCurrentLoops);

        void SetNumLastLoops(uint32 numCurrentLoops);
        uint32 GetNumLastLoops() const;

        /**
         * Get the number of times the motion currently has been played.
         * @result The number of times the motion has been completely played.
         */
        uint32 GetNumCurrentLoops() const;

        /**
         * Check if the motion will play forever or not.
         * @result Returns true when the motion is looping forever, or false when there will be a moment where it will be stopped.
         */
        bool GetIsPlayingForever() const;

        /**
         * Get the actor instance we are playing this motion instance on.
         * @result A pointer to the actor where this motion instance is playing on.
         */
        ActorInstance* GetActorInstance() const;

        /**
         * Get the priority level of the motion instance.
         * Higher values mean less change on getting overwritten by another motion.
         * A good example are facial motions being played on a walking character.
         * You would first play the walk motion, with priority of say 0.
         * After that you will play a facial motion, with mix mode, and priority level 5 for example.
         * Now you want to change the walk motion into a run motion. If we did not set the priority level for the
         * facial motion above 0, the run motion would overwrite the facial motion. But since the facial motion
         * has got a higher priority, it will not be overwritten by the run motion. If we now want to change the
         * facial motion with another one, we simply play the facial motion with the same or a higher priority level
         * as the previous facial motion. So a priority level of 5 or higher would work in the example case.
         * @result The priority level of the motion instance.
         */
        uint32 GetPriorityLevel() const;

        /**
         * Set the priority level of the motion instance.
         * Higher values mean less change on getting overwritten by another motion.
         * A good example are facial motions being played on a walking character.
         * You would first play the walk motion, with priority of say 0.
         * After that you will play a facial motion, with mix mode, and priority level 5 for example.
         * Now you want to change the walk motion into a run motion. If we did not set the priority level for the
         * facial motion above 0, the run motion would overwrite the facial motion. But since the facial motion
         * has got a higher priority, it will not be overwritten by the run motion. If we now want to change the
         * facial motion with another one, we simply play the facial motion with the same or a higher priority level
         * as the previous facial motion. So a priority level of 5 or higher would work in the example case.
         * @result The priority level of the motion instance.
         */
        void SetPriorityLevel(uint32 priorityLevel);

        /**
         * Check if this motion has motion extraction enabled or not.
         * @result Returns true if motion extraction is enabled, false if not.
         * @see Actor::SetMotionExtractionNode
         */
        bool GetMotionExtractionEnabled() const;

        /**
         * Enable or disable motion extraction.
         * @param enable Set to true when you want to enable motion extraction.
         * @see GetMotionExtractionEnabled
         */
        void SetMotionExtractionEnabled(bool enable);

        /**
         * Check if this motion instance is allowed to overwrite (and thus delete) other motion instances/layers.
         * This happens in the motion layer system when the weight of this motion instance becomes 1, which means it
         * would completely overwrite other motions because the other motions will not have any influence anymore.
         * On default this value is set to true.
         * @result Returns true when the motion instance will automatically delete other motion instances when its weight reaches
         *         a value of 1. If the motion instance will not delete/overwrite any other motion instances, false is returned.
         */
        bool GetCanOverwrite() const;

        /**
         * Enable or disable this motion instance to overwrite and so delete other motion instances.
         * This happens in the motion layer system when the weight of this motion instance becomes 1, which means it
         * would completely overwrite other motions because the other motions will not have any influence anymore.
         * On default overwriting is enabled, so the value would be true.
         * @param canOverwrite Set to true to allow this motion instance to overwrite/delete other motion instances/layers. Otherwise set to false.
         */
        void SetCanOverwrite(bool canOverwrite);

        /**
         * Check if this motion instance can delete itself when its weight equals zero, which means the motion would have no visual influence.
         * On default this value is set to true.
         * @result Returns true when the motion instance will delete itself when its weight equals a value of zero. Otherwise false is returned.
         */
        bool GetDeleteOnZeroWeight() const;

        /**
         * Allow or disallow the motion instance to delete itself when its weight equals zero.
         * When a motion instance has a weight of zero it means it would have no visual influence in the final result.
         * On default deletion on zero weights is enabled, so the value would be true.
         * @param deleteOnZeroWeight Set to true when you wish to allow the motion instance to delete itself when it has a weight of zero. Otherwise set it to false.
         */
        void SetDeleteOnZeroWeight(bool deleteOnZeroWeight);

        /**
         * Check if this motion instance uses a blend mask or not.
         * A blend mask allows you to specify a weight per node instead of just per motion instance.
         * This however is a bit slower as well. When the blend mask is disabled, performance will be highest.
         * You can use the DisableNodeWeights() method to disable the blend mask.
         * @result Returns true when a blend mask is being used. Otherwise false is returned.
         */
        bool GetHasBlendMask() const;

        /**
         * Stop the motion, using a given fade-out time.
         * This will first modify the fade-out time and then fade to a zero weight.
         * @param fadeOutTime The time it takes, in seconds, to fade out the motion.
         */
        void Stop(float fadeOutTime);

        /**
         * Stop the motion, using the currently set fade-out time.
         * This will blend the weight into zero.
         */
        void Stop();

        /**
         * Get the start node index, from where to start playing the motion.
         * If the start node example is set to left upper arm's node index, then the motion will only effect the
         * nodes down the hierarchy of the upper arm, which means the arm, hand and fingers will all be adjusted by this motion.
         * The rest of the body would then remain unchanged by this motion.
         * When this is set to MCORE_INVALIDINDEX32, which is the default value, no start node is being used.
         * @result The node index of the start node, or MCORE_INVALIDINDEX32 when it is not being used (default setting).
         */
        uint32 GetStartNodeIndex() const;

        /**
         * Check if motion retargeting on this motion instance is enabled or not.
         * @result Returns true when motion retargeting is enabled on this motion instance. Otherwise false is returned.
         */
        bool GetRetargetingEnabled() const;

        /**
         * Enable or disable motion retargeting on this motion instance.
         * Retargeting takes a bit more speed, but it is a very small performance difference.
         * @param enabled Set to true when you wish to enable motion retargeting, otherwise set to false (false is the default).
         */
        void SetRetargetingEnabled(bool enabled);

        /**
         * Check if the motion instance is active or not. On default the motion instance will be active.
         * Inactive motion instances do not get processed at all. They will not update their weight blending or time values.
         * The difference with paused motion instances is that paused instances do process their weight blending, while inactive
         * motion instances do not.
         * @result Returns true when the motion instance is active, otherwise false is returned.
         */
        bool GetIsActive() const;

        /**
         * Activate or deactivate this motion instance. On default the motion instance will be active.
         * Inactive motion instances do not get processed at all. They will not update their weight blending or time values.
         * The difference with paused motion instances is that paused instances do process their weight blending, while inactive
         * motion instances do not.
         * @param enabled Set to true when you want to activate the motion instance, or false when you wish to deactivate it.
         */
        void SetIsActive(bool enabled);

        /**
         * Check if we are frozen in the last frame or not.
         * This would only happen when the SetFreezeAtLastFrame(...) was set to true and when the maximum number of loops
         * has been reached. Instead of fading out the motion, it will remain in its last frame.
         * @result Returns true when the motion is frozen in its last frame.
         */
        bool GetIsFrozen() const;

        /**
         * Set if we are frozen in the last frame or not.
         * Instead of fading out the motion, it will remain in its last frame.
         */
        void SetIsFrozen(bool isFrozen);

        /**
         * Check if motion event processing is enabled for this motion instance.
         * @result Returns true when processing of motion events for this motion event is enabled. Otherwise false is returned.
         */
        bool GetMotionEventsEnabled() const;

        /**
         * Enable or disable processing of motion events for this motion instance.
         * @param enabled Set to true when you wish to enable processing of motion events. Otherwise set it to false.
         */
        void SetMotionEventsEnabled(bool enabled);

        /**
         * Set the motion event weight threshold for this motion instance.
         * The threshold value represents the minimum weight value the motion instance should have in order for it
         * to have its motion events executed.
         * For example if the motion event threshold is set to 0.3, and the motion instance weight value is 0.1, then
         * no motion events will be processed. If however the weight value of the motion instance is above or equal to this 0.3 value
         * then all events will be processed. On default the value is 0.0, which means that all events will be processed.
         * @param weightThreshold The motion event weight threshold. If the motion instance weight is below this value, no motion
         *                        events will be processed for this motion instance. Basically this value should be in range of [0..1].
         */
        void SetEventWeightThreshold(float weightThreshold);

        /**
         * Get the motion event weight threshold for this motion instance.
         * The threshold value represents the minimum weight value the motion instance should have in order for it
         * to have its motion events executed.
         * For example if the motion event threshold is set to 0.3, and the motion instance weight value is 0.1, then
         * no motion events will be processed. If however the weight value of the motion instance is above or equal to this 0.3 value
         * then all events will be processed. On default the value is 0.0, which means that all events will be processed.
         * @result The motion event threshold value.
         */
        float GetEventWeightThreshold() const;

        /**
         * Check if this motion instance will freeze at its last frame (when not using looping).
         * This only happens when using a maximum play time, or a given number of loops that is not infinite.
         * @result Returns true when the motion instance will freeze at the last frame. Otherwise false is returned.
         */
        bool GetFreezeAtLastFrame() const;

        /**
         * This option allows you to specify whether a motion should start fading out before the motion has ended (before it reached
         * its maximum number of loops) or if it should fade out after the maximum number of loops have been reached. In the latter case
         * it would mean that the motion will start another loop while it starts fading out. On default this option is enabled, which means
         * the motion will be faded out exactly when it reaches the last frame it should play.
         * @param enabled Set to true to to enable this option, or false to disable it. On default it is set to true.
         */
        void SetBlendOutBeforeEnded(bool enabled);

        /**
         * The "blend out before end"-option allows you to specify whether a motion should start fading out before the motion has ended (before it reached
         * its maximum number of loops) or if it should fade out after the maximum number of loops have been reached. In the latter case
         * it would mean that the motion will start another loop while it starts fading out. On default this option is enabled, which means
         * the motion will be faded out exactly when it reaches the last frame it should play.
         * @result Returns true when this option is enabled, otherwise false is returned.
         */
        bool GetBlendOutBeforeEnded() const;

        /**
         * Enable or disable freezing at the last frame.
         * This only happens when using a maximum play time, or a given number of loops that is not infinite.
         * When you play a full body death motion, you probably want to enable this.
         * If in that case this option would be disabled (default) it will blend back into its base/bind pose instead of freezing
         * at the last frame, keeping the character dead on the ground.
         * @param enabled Set to true when you want to enable the freeze at last frame option, otherwise set to false.
         */
        void SetFreezeAtLastFrame(bool enabled);

        /**
         * Get the total time this motion has been playing already.
         * @result The total time, in seconds, that this motion is already playing.
         */
        float GetTotalPlayTime() const;

        /**
         * Adjust the total play time that this motion is already playing.
         * @param playTime The total play time, in seconds.
         */
        void SetTotalPlayTime(float playTime);

        /**
         * Get the maximum play time of this motion instance (well actually of the motion where this is an instance from).
         * So this returns the duration of the motion, in seconds.
         * When this value is zero or a negative value, then the maximum play time option has been disabled for this motion instance.
         * @result The duration of the motion, in seconds.
         */
        float GetMaxPlayTime() const;

        /**
         * Set the maximum play time, in seconds, that this motion instance is allowed to play.
         * When you set this to zero (default), or a negative value, then this option is disabled.
         * @param playTime The maximum play time, in seconds, or zero or a negative value to disable the maximum play time.
         */
        void SetMaxPlayTime(float playTime);

        /**
         * Get the number of motion links. This should always be equal to the number of nodes in the actor.
         * @result The number of motion links.
         */
        uint32 GetNumMotionLinks() const;

        /**
         * Get a given motion link.
         * @param nr The motion link number to get, which must be in range of [0..GetNumMotionLinks()-1].
         * @result A pointer to the motion link.
         */
        MotionLink* GetMotionLink(uint32 nr);

        /**
         * Get a given motion link.
         * @param nr The motion link number to get, which must be in range of [0..GetNumMotionLinks()-1].
         * @result A pointer to the motion link.
         */
        const MotionLink* GetMotionLink(uint32 nr) const;

        /**
         * Set a given motion link.
         * @param nr The motion link number, which must be in range of [0..GetNumMotionLinks()-1].
         * @param link The motion link to use.
         */
        void SetMotionLink(uint32 nr, const MotionLink& link);

        /**
         * Initialize the motion instance from PlayBackInfo settings.
         * @param info The playback info settings to initialize from.
         * @param resetCurrentPlaytime Set back the current playtime, even though this is not an attribute of the playback info in case of true. In case of false the current time won't be modified.
         */
        void InitFromPlayBackInfo(const PlayBackInfo& info, bool resetCurrentPlaytime = true);

        /**
         * Get a weight of the current node (from the blend mask).
         * When no blend mask is setup, it will return 1.0, which means the node is fully active.
         * @param nodeIndex The node number to get the weight for.
         * @result The weight of the node in this motion instance.
         */
        float GetNodeWeight(uint32 nodeIndex) const;

        /**
         * Set the weight for a given node.
         * This will enable the blend mask system for this motion instance.
         * This allows you to setup weigths per node. On default all node weights are set to 1.0, which means full influence.
         * @param nodeIndex The node to set the weight for.
         * @param weight The weight of the to use, which must be in range of [0..1].
         */
        void SetNodeWeight(uint32 nodeIndex, float weight);

        /**
         * Disable the per node weights (blend mask).
         */
        void DisableNodeWeights();

        //------------------------------------------

        /**
         * Set the value of the last cached key index.
         * This is the key index that would be checked first internally when sampling.
         * The chance that this key would be the right one is higher, so we can speed things up by caching the last hit.
         * @param nodeIndex The node index we want to store the cached key for.
         * @param keyIndex The cached key index value.
         */
        MCORE_INLINE void SetCachedKey(uint32 nodeIndex, uint32 keyIndex)       { mCachedKeys[nodeIndex] = keyIndex; }

        /**
         * Get the cached key index value for a given node.
         * @param nodeIndex The node index to get the cached key value for.
         * @result The cached key value for the given node.
         */
        MCORE_INLINE uint32 GetCachedKey(uint32 nodeIndex) const                { return mCachedKeys[nodeIndex]; }

        /**
         * Reset the cache hit and misses counters.
         */
        void ResetCacheHitCounters();

        /**
         * Process a given cache hit result as returned by the sampling functions.
         * @param wasHitResult The hit test result from the sampling functions in the keytrack.
         */
        void ProcessCacheHitResult(uint8 wasHitResult);

        /**
         * Get the number of skeletal motion keytrack sampling cache hits so far.
         * @result The number of cache hits so far.
         */
        uint32 GetNumCacheHits() const;

        /**
         * Get the number of skeletal motion keytrack sampling cache misses so far.
         * @result The number of cache misses so far.
         */
        uint32 GetNumCacheMisses() const;

        /**
         * Calculate the percentage of skeletal motion keytrack sampling cache hits so far.
         * @result The percentage of cache hits.
         */
        float CalcCacheHitPercentage() const;

        /**
         * Get the time difference between the current play time and the end of the motion.
         * With end of motion we mean the point where there is no more motion data, so where it would need to loop to continue to play.
         * @result The time remaining until the loop point of the motion would be reached.
         */
        float GetTimeDifToLoopPoint() const;

        /**
         * Get the start time of the motion. When the motion starts, the initial time will be set to this time value.
         * When the motion loops it will also use this start position as loop point. By setting the start and end clip time you basically make
         * EMotion FX internally think that the motion is the part of the original motion between the start and end clip time.
         * You can use this to play only a section of the motion.
         * @result The start position, in seconds, of the motion.
         */
        float GetClipStartTime() const;

        /**
         * Set the start time of the motion. When the motion starts, the initial time will be set to this time value.
         * When the motion loops it will also use this start position as loop point. By setting the start and end clip time you basically make
         * EMotion FX internally think that the motion is the part of the original motion between the start and end clip time.
         * You can use this to play only a section of the motion.
         * @param timeInSeconds The time of the start clip point, in seconds.
         */
        void SetClipStartTime(float timeInSeconds);

        /**
         * Get the end time of the motion.
         * When the motion loops it will also use this end position as loop point. By setting the start and end clip time you basically make
         * EMotion FX internally think that the motion is the part of the original motion between the start and end clip time.
         * You can use this to play only a section of the motion.
         * @result The end position, in seconds, of the motion.
         */
        float GetClipEndTime() const;

        /**
         * Set the end time of the motion.
         * When the motion loops it will also use this end position as loop point. By setting the start and end clip time you basically make
         * EMotion FX internally think that the motion is the part of the original motion between the start and end clip time.
         * You can use this to play only a section of the motion.
         * @param timeInSeconds The time of the end clip point, in seconds.
         */
        void SetClipEndTime(float timeInSeconds);

        //--------------------------

        /**
         * Add an event handler to this motion instance.
         * After adding, the event handler will receive events.
         * @param eventHandler The new event handler to register.
         */
        void AddEventHandler(MotionInstanceEventHandler* eventHandler);

        /**
         * Find the index for the given event handler.
         * @param[in] eventHandler A pointer to the event handler to search.
         * @return The index of the event handler, or MCORE_INVALIDINDEX32 in case the event handler has not been found.
         */
        uint32 FindEventHandlerIndex(MotionInstanceEventHandler* eventHandler) const;

        /**
         * Remove the given event handler.
         * Even if the function returns false, if delFromMem is set to true it will delete the handler from memory.
         * @param eventHandler A pointer to the event handler to remove.
         * @param delFromMem When set to true, the event handler will be deleted from memory automatically when removing it.
         */
        bool RemoveEventHandler(MotionInstanceEventHandler* eventHandler, bool delFromMem = true);

        /**
         * Remove the event handler at the given index.
         * @param index The index of the event handler to remove.
         * @param delFromMem When set to true, the event handler will be deleted from memory automatically when removing it.
         */
        void RemoveEventHandler(uint32 index, bool delFromMem = true);

        /**
         * Remove all motion event handlers from this motion instance.
         * @param delFromMem Set to true when you want them to be deleted from memory as well.
         */
        void RemoveAllEventHandlers(bool delFromMem = true);

        /**
         * Get the event handler at the given index.
         * @result A pointer to the event handler at the given index.
         */
        MotionInstanceEventHandler* GetEventHandler(uint32 index) const;

        /**
         * Get the number of event handlers.
         * @result The number of event handlers assigned to the motion instance.
         */
        uint32 GetNumEventHandlers() const;

        //--------------------------------

        /**
         * The method that processes an event.
         * @param eventInfo The struct holding the information about the triggered event.
         */
        void OnEvent(const EventInfo& eventInfo);

        /**
         * The event that gets triggered when a motion instance is really being played.
         * This can be a manual call through MotionInstance::PlayMotion or when the MotionQueue class
         * will start playing a motion that was on the queue.
         * The difference between OnStartMotionInstance() and this OnPlayMotion() is that the OnPlayMotion doesn't
         * guarantee that the motion is being played yet, as it can also be added to the motion queue.
         * The OnStartMotionInstance() method will be called once the motion is really being played.
         * @param info The playback info used to play the motion.
         */
        void OnStartMotionInstance(PlayBackInfo* info);

        /**
         * The event that gets triggered once a MotionInstance object is being deleted.
         * This can happen when calling the MotionSystem::RemoveMotionInstance() method manually, or when
         * EMotion FX internally removes the motion instance because it has no visual influence anymore.
         * The destructor of the MotionInstance class automatically triggers this event.
         */
        void OnDeleteMotionInstance();

        /**
         * The event that gets triggered when a motion instance is being stopped using one of the MotionInstance::Stop() methods.
         * EMotion FX will internally stop the motion automatically when the motion instance reached its maximum playback time
         * or its maximum number of loops.
         */
        void OnStop();

        /**
         * This event gets triggered once a given motion instance has looped.
         */
        void OnHasLooped();

        /**
         * This event gets triggered once a given motion instance has reached its maximum number of allowed loops.
         * In this case the motion instance will also be stopped automatically afterwards.
         */
        void OnHasReachedMaxNumLoops();

        /**
         * This event gets triggered once a given motion instance has reached its maximum playback time.
         * For example if this motion instance is only allowed to play for 2 seconds, and the total playback time reaches
         * two seconds, then this event will be triggered.
         */
        void OnHasReachedMaxPlayTime();

        /**
         * This event gets triggered once the motion instance is set to freeze at the last frame once the
         * motion reached its end (when it reached its maximum number of loops or playtime).
         * In this case this event will be triggered once.
         */
        void OnIsFrozenAtLastFrame();

        /**
         * This event gets triggered once the motion pause state changes.
         * For example when the motion is unpaused but gets paused, then this even twill be triggered.
         * Paused motions don't get their playback times updated. They do however still perform blending, so it is
         * still possible to fade them in or out.
         */
        void OnChangedPauseState();

        /**
         * This event gets triggered once the motion active state changes.
         * For example when the motion is active but gets set to inactive using the MotionInstance::SetActive(...) method,
         * then this even twill be triggered.
         * Inactive motions don't get processed at all. They will not update their playback times, blending, nor will they take
         * part in any blending calculations of the final node transforms. In other words, it will just be like the motion instance
         * does not exist at all.
         */
        void OnChangedActiveState();

        /**
         * This event gets triggered once a motion instance is automatically changing its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, then once this blending starts, this event is being triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time bigger than zero, and
         * if the motion instance isn't currently already blending, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         */
        void OnStartBlending();

        /**
         * This event gets triggered once a motion instance stops it automatic changing of its weight value over time.
         * For example when a motion is automatically being faded in from weight 0 to a given target weight in half
         * a second, and once the target weight is reached after half a second, will cause this event to be triggered.
         * Once the the MotionInstance::SetWeight(...) method is being called with a blend time equal to zero and the
         * motion instance is currently blending its weight value, then this event will be triggered.
         * This event most likely will get triggered when using the MotionSystem::PlayMotion() and MotionInstance::Stop() methods.
         */
        void OnStopBlending();

        /**
         * This event gets triggered once the given motion instance gets added to the motion queue.
         * This happens when you set the PlayBackInfo::mPlayNow member to false. In that case the MotionSystem::PlayMotion() method (OnPlayMotion)
         * will not directly start playing the motion (OnStartMotionInstance), but will add it to the motion queue instead.
         * The motion queue will then start playing the motion instance once it should.
         * @param info The playback information used to play this motion instance.
         */
        void OnQueueMotionInstance(PlayBackInfo* info);

        /**
         * Marks the object as used by the engine runtime, as opposed to the tool suite.
         */
        void SetIsOwnedByRuntime(bool isOwnedByRuntime);
        bool GetIsOwnedByRuntime() const;

        MotionLink* GetMotionLinks() const;

        float GetFreezeAtTime() const;
        void SetFreezeAtTime(float timeInSeconds);

        void CalcRelativeTransform(Node* rootNode, float curTime, float oldTime, Transform* outTransform) const;
        bool ExtractMotion(Transform& outTrajectoryDelta);
        void CalcGlobalTransform(const MCore::Array<uint32>& hierarchyPath, float timeValue, Transform* outTransform) const;
        void CalcNewTimeAfterUpdate(float timePassed, float* outNewTime, float* outPassedTime, float* outTotalPlayTime, float* outTimeDifToEnd, uint32* outNumLoops, bool* outHasLooped, bool* outFrozenInLastFrame);
        void ResetTimes();

        MotionInstancePool::SubPool* GetSubPool() const;

    private:
        MCore::Array<MotionLink>    mMotionLinks;   /**< The motion links, one for each node. */
        uint32*             mCachedKeys;            /**< The cached rotation keyframe indices. */
        float               mCurrentTime;           /**< The current playtime. */
        float               mClipStartTime;         /**< The start playback position of the motion, as well as the start of the loop point. */
        float               mClipEndTime;           /**< The end of the motion and loop point. When set to zero or below, the duration of the motion is used internally. */
        float               mPassedTime;            /**< Passed time. */
        float               mBlendInTime;           /**< The blend in time. */
        float               mFadeTime;              /**< Fadeout speed, when playing the animation once. So when it is done playing once, it will fade out in 'mFadeTime' seconds. */
        float               mPlaySpeed;             /**< The playspeed (1.0=normal speed). */
        float               mTargetWeight;          /**< The target weight of the layer, when activating the motion. */
        float               mWeight;                /**< The current weight value, in range of [0..1]. */
        float               mWeightDelta;           /**< The precalculated weight delta value, used during blending between weights. */
        float               mLastCurTime;           /**< The last current time, so the current time in the previous update. */
        float               mTotalPlayTime;         /**< The current total play time that this motion is already playing. */
        float               mMaxPlayTime;           /**< The maximum play time of the motion. If the mTotalPlayTime is higher than this, the motion will be stopped, unless the max play time is zero or negative. */
        float               mEventWeightThreshold;  /**< If the weight of the motion instance is below this value, the events won't get processed (default = 0.0f). */
        float               mTimeDifToEnd;          /**< The time it takes until we reach the loop point in the motion. This also takes the playback direction into account (backward or forward play). */
        float               mFreezeAtTime;          /**< Freeze at a given time offset in seconds. The current play time would continue running though, and a blend out would be triggered, unlike the mFreezeAtLastFrame. Set to negative value to disable. Default=-1.*/
        MCore::Array<MotionInstanceEventHandler*>   mEventHandlers;         /**< The event handler to use to process events. */
        MCore::Array<MCore::Compressed8BitFloat>    mNodeWeights;   /**< The node weights, one for each node, or an empty array length when disabled. */
        uint32              mCacheHits;             /**< The number of cache hits in the last update. */
        uint32              mCacheMisses;           /**< The number of cache misses in the last update. */
        uint32              mCurLoops;              /**< Number of loops it currently has made (so the number of times the motion played already). */
        uint32              mMaxLoops;              /**< The maximum number of loops, before it has to stop. */
        uint32              mLastLoops;             /**< The current number of loops in the previous update. */
        uint32              mPriorityLevel;         /**< The priority level, where higher values mean higher priority. */
        uint32              mStartNodeIndex;        /**< The node to start the motion from, using MCORE_INVALIDINDEX32 to effect the whole body, or use for example the upper arm node to only play the motion on the arm. */
        uint32              mID;                    /**< The unique identification number for the motion instance. */
        Motion*             mMotion;                /**< The motion that this motion instance is using the keyframing data from. */
        ActorInstance*      mActorInstance;         /**< The actor instance where we are playing this motion instance on. */
        void*               mCustomData;            /**< The custom data pointer, which is nullptr on default. */
        EMotionBlendMode    mBlendMode;             /**< The motion blend mode [default=BLENDMODE_OVERWRITE]. */
        uint16              mBoolFlags;             /**< The boolean flags mask. */
        EPlayMode           mPlayMode;              /**< The motion playback mode [default=PLAYMODE_FORWARD]. */
        MotionGroup*        mMotionGroup;           /**< The motion group (which can be empty in case no motion group is used). */
        MotionInstancePool::SubPool* mSubPool;      /**< The subpool this motion instance is part of, or nullptr when it isn't part of any subpool. */

        /**
         * Instead of storing a bunch of booleans we use the bits of a 16 bit value.
         */
        enum
        {
            BOOL_ISPAUSED               = 1 << 0,   /**< Is the motion paused? */
            BOOL_ISSTOPPING             = 1 << 1,   /**< Are we stopping and fading out? */
            BOOL_ISBLENDING             = 1 << 2,   /**< Are we blending in or out? (changing weight with time interval). */
            BOOL_ISMIXING               = 1 << 3,   /**< Is this motion a mixing motion? */
            BOOL_USEMOTIONEXTRACTION    = 1 << 4,   /**< Use motion extraction?  */
            BOOL_CANOVERWRITE           = 1 << 5,   /**< Can this motion instance overwrite and remove other motion instances? */
            BOOL_DELETEONZEROWEIGHT     = 1 << 6,   /**< Will this motion instance be deleted when it reaches a weight of zero? */
            BOOL_RETARGET               = 1 << 7,   /**< Is retargeting enabled? */
            BOOL_FREEZEATLASTFRAME      = 1 << 8,   /**< Should we freeze at the last frame? */
            BOOL_ENABLEMOTIONEVENTS     = 1 << 9,   /**< Enable motion events for this motion instance? */
            BOOL_ISACTIVE               = 1 << 10,  /**< Is the motion instance active? */
            BOOL_ISFIRSTREPOSUPDATE     = 1 << 11,  /**< Is this the first time the repositioning is being updated for this motion instance? */
            BOOL_ISFROZENATLASTFRAME    = 1 << 12,  /**< Is the motion in a frozen state? */
            BOOL_BLENDBEFOREENDED       = 1 << 13,  /**< Start blending out before the motion has ended, so that it exactly is faded out when the motion is in its last frame? */
            BOOL_MIRRORMOTION           = 1 << 14,  /**< Mirror the motion? */

#if defined(EMFX_DEVELOPMENT_BUILD)
            BOOL_ISOWNEDBYRUNTIME       = 1 << 15,  /**< Is motion owned by the engine runtime? */
#endif // EMFX_DEVELOPMENT_BUILD

                //BOOL_AUTOMOTIONEXTRACT        = 1 << 16   /**< Is auto motion extraction mode enabled? (in auto mode the trajectory node doesnt need keyframes). */
        };

        /**
         * The constructor.
         * @param motion The motion from which this is an instance.
         * @param actorInstance The actor instance we are playing this motion instance on.
         * @param startNodeIndex The node to start playing the motion at. When set to MCORE_INVALIDINDEX32, all nodes can be modified by the motion.
         *                       For example when you wish to influence only the arm by this motion, pass a pointer to the upper arm node as start node.
         *                       This will make the upper arm, and all child nodes down the hierarchy to be influenced by this motion.
         */
        MotionInstance(Motion* motion, ActorInstance* actorInstance, uint32 startNodeIndex);

        /**
         * The destructor.
         */
        ~MotionInstance();

        void SetSubPool(MotionInstancePool::SubPool* subPool);

        /**
         * Enable boolean flags.
         * @param flag The flags to enable.
         */
        MCORE_INLINE void EnableFlag(uint16 flag)                   { mBoolFlags |= flag; }

        /**
         * Disable boolean flags.
         * @param flag The flags to disable.
         */
        MCORE_INLINE void DisableFlag(uint16 flag)                  { mBoolFlags &= ~flag; }

        /**
         * Enable or disable specific flags.
         * @param flag The flags to modify.
         * @param enabled Set to true to enable the flags, or false to disable them.
         */
        MCORE_INLINE void SetFlag(uint16 flag, bool enabled)
        {
            if (enabled)
            {
                mBoolFlags |= flag;
            }
            else
            {
                mBoolFlags &= ~flag;
            }
        }
    };
} // namespace EMotionFX
