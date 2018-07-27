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

#include <AzCore/Component/ComponentBus.h>

struct IMaterial;

namespace Lightning
{
    class LightningArcComponentRequests
        : public AZ::ComponentBus
    {
    public:
        // EBus Traits overrides (Configuring this Ebus)
        // Using Defaults
        
        //! Enables lightning arc emission
        virtual void Enable() = 0;
        //! Disables lightning arc emission
        virtual void Disable() = 0;
        //! Toggles lightning arc emission
        virtual void Toggle() = 0;
        /**
         * Returns whether or not the component is emitting lightning arcs.
         *
         * @return True if the component is enabled
         */
        virtual bool IsEnabled() = 0;

        /**
         * Sets the target entities that the component will emit arcs at.
         *
         * @param targets A new collection of targets entity ids
         */
        virtual void SetTargets(const AZStd::vector<AZ::EntityId>& targets) = 0;
        /**
         * Gets the target entities that the component emits arcs at.
         *
         * @return A collection of target entity ids
         */
        virtual const AZStd::vector<AZ::EntityId>& GetTargets() = 0;

        /**
         * Sets the time (in seconds) between emitted arcs.
         *
         * @param delay The new time between emitted arcs
         */
        virtual void SetDelay(double delay) = 0;
        /**
         * Gets the time (in seconds) between emitted arcs.
         *
         * @return The time between emitted arcs
         */
        virtual double GetDelay() = 0;

        /**
         * Sets the variation in time between emitted arcs.
         *
         * Delay variation is a random range applied to the Delay 
         * parameter to calculate the time to the next arc emission.
         *
         * The random variation is in the range [delayVariation * 0.5, delayVariation).
         *
         * Meaning with a delay of 2.0 and a delay variation of 1.0 the 
         * range of time between arc emissions is: [2.5 and 3.0)
         *
         * @param delayVariation The new variation (in seconds) to apply to the delay parameter
         */
        virtual void SetDelayVariation(double delayVariation) = 0;
        /**
         * Gets the variation in time between emitted arcs.
         *
         * @return The variation (in seconds) to apply to the delay parameter
         */
        virtual double GetDelayVariation() = 0;

        /**
         * Sets the minimum amount of time that an arc is kept alive.
         *
         * @param strikeTimeMin The new StrikeTimeMin value
         */
        virtual void SetStrikeTimeMin(float strikeTimeMin) = 0;
        /**
         * Gets the minimum amount of time that an arc is kept alive.
         *
         * @return The minimum amount of time that an arc is kept alive
         */
        virtual float GetStrikeTimeMin() = 0;

        /**
         * Sets the maximum amount of time that an arc is kept alive.
         *
         * @param strikeTimeMax The new StrikeTimeMax value
         */
        virtual void SetStrikeTimeMax(float strikeTimeMax) = 0;
        /**
         * Gets the maximum amount of time that an arc is kept alive.
         *
         * @return The maximum amount of time that an arc is kept alive
         */
        virtual float GetStrikeTimeMax() = 0;

        /**
         * Sets how long it will take for an arc to fade out.
         *
         * Note that fading out is done by reducing size to 0
         * not by adjusting transparency.
         *
         * @param strikeFadeOut The new time it will take for the arc to fade out
         */
        virtual void SetStrikeFadeOut(float strikeFadeOut) = 0;
        /**
         * Gets how long it will take for an arc to fade out.
         *
         * @return How long it will take for an arc to fade out 
         */
        virtual float GetStrikeFadeOut() = 0;

        /**
         * Sets the number of segments in an arc.
         *
         * The more segments there are the more "snaky" the 
         * arc will be in appearance. 
         *
         * A lightning arc cannot have 0 segments. Passing 0 will
         * issue a warning and 1 will be used instead.
         *
         * @param strikeSegmentCount The new number of segments in an arc
         */
        virtual void SetStrikeSegmentCount(AZ::u32 strikeSegmentCount) = 0;
        /**
         * Gets the number of segments in an arc.
         *
         * @return The number of segments in an arc
         */
        virtual AZ::u32 GetStrikeSegmentCount() = 0;

        /**
         * Sets the number of points per segment.
         *
         * The more points per segment the stronger the "noisy" effect
         * produced by the Fuzziness parameter. 
         *
         * A lightning arc cannot have 0 points. Passing 0 will
         * issue a warning and 1 will be used instead.
         *
         * @param strikePointCount The new number of points per segment
         */
        virtual void SetStrikePointCount(AZ::u32 strikePointCount) = 0;
        /**
         * Gets the number of points per segment in an arc.
         *
         * @return The number of points per segment in an arc
         */
        virtual AZ::u32 GetStrikePointCount() = 0;

        /**
         * Sets the amount of deviation applied to the arcs.
         *
         * The lower the value the more smooth an arc will appear.
         *
         * @param deviation The new lightning deviation value
         */
        virtual void SetLightningDeviation(float deviation) = 0;
        /**
         * Gets the amount of deviation applied to the arcs.
         *
         * @return The amount of lightning deviation
         */
        virtual float GetLightningDeviation() = 0;

        /**
         * Sets the amount of noise applied to the arcs.
         *
         * @param fuzziness The new fuzziness value applied to determine the noisiness of the arcs
         */
        virtual void SetLightningFuzziness(float fuzziness) = 0;
        /**
         * Gets the amount of noise applied to the arcs.
         *
         * @return The fuzziness value used to determine the noisiness of the arcs
         */
        virtual float GetLightningFuzziness() = 0;

        /**
         * Sets how fast an arc will drift upwards after being emitted.
         *
         * @velocity The new upwards velocity for arcs
         */
        virtual void SetLightningVelocity(float velocity) = 0;
        /**
         * Gets how fast an arc will drift upwards after being emitted.
         *
         * @return The upwards velocity for arcs
         */
        virtual float GetLightningVelocity() = 0;

        /**
         * Sets how likely it is for a child branch to spawn off of an arc.
         *
         * A child arc is an arc half the size and intensity. It still has the same
         * emission point and target as its parent. 
         *
         * A value of 0 means no children will spawn. 0.5f is a 50% chance of spawning
         * a child. 2.0f means a 100% chance of spawning 2 children. See SetBranchMaxLevel
         * to see the how to set the upper limit of branching arcs.
         *
         * @param branchProbability The new probability of a branching arc being spawned
         */
        virtual void SetBranchProbablity(float branchProbability) = 0;
        /**
         * Gets how likely it is for a child branch to spawn off of an arc.
         *
         * @return The probability of a branching arc being spawned
         */
        virtual float GetBranchProbablity() = 0;

        /**
         * Sets the max number of branches allowed to spawn off an arc.
         *
         * 0 will mean that no branches will spawn regardless of branch probability.
         * 3 will mean that between 0 and 3 branches can spawn depending on the 
         * branch probability. 
         *
         * @param branchMaxLevel The new max number of branches that can spawn off an arc
         */
        virtual void SetBranchMaxLevel(AZ::u32 branchMaxLevel) = 0;
        /**
         * Gets the max number of branches allowed to spawn off an arc.
         *
         * @return The max number of branches that can spawn off an arc
         */
        virtual AZ::u32 GetBranchMaxLevel() = 0;

        /**
         * Sets how many arcs can be alive at one time from this component.
         *
         * This includes parent and branch arcs.
         *
         * @param maxStrikeCount The max number of arc strikes alive at one time from this component
         */
        virtual void SetMaxStrikeCount(AZ::u32 maxStrikeCount) = 0;
        /**
         * Gets the maximum number of arcs that can be alive at one time from this component.
         *
         * @return The max number of arc strikes that can be alive at one time from this component
         */
        virtual AZ::u32 GetMaxStrikeCount() = 0;

        /**
         * Sets the size (width) of the generated arcs.
         *
         * Branch arcs will have half of this size.
         * 
         * @param beamSize The new size of arcs generated by this component
         */
        virtual void SetBeamSize(float beamSize) = 0;
        /**
         * Gets the size (width) of the generated arcs.
         *
         * @return The size of the generated arcs
         */
        virtual float GetBeamSize() = 0;

        /**
         * Sets the texture tiling based on the world size of the arc beam.
         *
         * A value of 2.0 means that the texture will wrap around twice every meter.
         * A value of 0.25 means the texture will wrap around 4 times every meter. 
         *
         * Only the U coordinate of the texture map is affected by this parameter.
         *
         * @param beamTexTiling The new texture tiling value 
         */
        virtual void SetBeamTexTiling (float beamTexTiling) = 0;
        /**
         * Gets the texture tiling parameter for the arc beam.
         *
         * @return The texture tiling value
         */
        virtual float GetBeamTexTiling() = 0;

        /**
         * Sets how fast to move through textures in the arc's animation.
         *
         * The U value of the texture coordinate will move at this given rate.
         * The V value will be automatically calculated to select the correct frame. 
         *
         * @param beamTexShift The new rate to adjust texture coordinates
         */
        virtual void SetBeamTexShift(float beamTexShift) = 0;
        /**
         * Gets how fast to move through textures in the arc's animation.
         *
         * @return The rate to adjust texture coordinates
         */
        virtual float GetBeamTexShift() = 0;

        /**
         * Sets how many frames are in an arc's animation.
         *
         * @param beamTexFrames The new number of frames in an arc's animation
         */
        virtual void SetBeamTexFrames(float beamTexFrames) = 0;
        /**
         * Gets the number of frames in an arc's animation.
         *
         * @return The number of frames in the given animation
         */
        virtual float GetBeamTexFrames() = 0;

        /**
         * Sets how many frames per second are in an arc's animation.
         *
         * @beamTexFPS The new frames per second in an arc's animation
         */
        virtual void SetBeamTexFPS(float beamTexFPS) = 0;
        /**
         * Gets how many frames per second are in an arc's animation.
         *
         * @return The frames per second in an arc's animation
         */
        virtual float GetBeamTexFPS() = 0;
    };

    using LightningArcComponentRequestBus = AZ::EBus<LightningArcComponentRequests>;

    class LightningArcComponentNotifications
        : public AZ::ComponentBus
    {
    public:
        // EBus Traits overrides (Configuring this Ebus)
        // Using Defaults

        virtual ~LightningArcComponentNotifications() {}
        
        //! Event that triggers when the given component fires a spark
        virtual void OnSpark() {};
    };

    using LightningArcComponentNotificationBus = AZ::EBus<LightningArcComponentNotifications>;

} //namespace Lightning