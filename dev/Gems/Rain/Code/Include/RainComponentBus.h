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

namespace Rain
{
    class RainOptions;

    /**
     * RainComponentRequestBus
     * Messages serviced by the Rain component.
     */
    class RainComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~RainComponentRequests() = default;

        //! Enables the rain effect
        virtual void Enable() = 0;

        //! Disables the rain effect
        virtual void Disable() = 0;

        //! Toggles whether or not the rain effect is enabled 
        virtual void Toggle() = 0;

        /**
         * Gets whether or not the rain effect is enabled
         *
         * @return True if the rain effect is enabled
         */
        virtual bool IsEnabled() = 0;

        /**
         * Sets whether or not the rain effect should be affected by VisAreas
         *
         * When false the rain effects will still be visible even when inside a VisArea.
         * Otherwise rain will stop rendering upon entering a VisArea.
         *
         * @param useVisAreas Pass false to ignore vis area volumes for rain rendering
         */
        virtual void SetUseVisAreas(bool useVisAreas) = 0;
        /**
         * Gets whether or not the rain effect will be affected by VisAreas
         *
         * @return True if the rain component will use VisAreas
         */
        virtual bool GetUseVisAreas() = 0;
        
        /**
         * Sets whether or not the rain effect will consider objects marked as occluders
         *
         * When true objects marked as rain occluders will not occlude rain.
         *
         * @param disableOcclusion Pass true to ignore occlusion volumes for rain rendering
         */
        virtual void SetDisableOcclusion(bool disableOcclusion) = 0;
        /**
         * Gets whether or not the rain effect will ignore occluders
         *
         * @return True if the rain component will ignore occlusion volumes
         */
        virtual bool GetDisableOcclusion() = 0;
        
        /**
         * Sets the radius of the rain effect
         *
         * Puddles will only be generated inside of this radius.
         * Raindrops will always render throughout the level.
         *
         * @param radius The new radius of the rain volume
         */
        virtual void SetRadius(float radius) = 0;
        /**
         * Gets the radius of the rain effect
         *
         * @return The radius of the rain effect
         */
        virtual float GetRadius() = 0;
        
        /**
         * Sets the overall amount of rain, puddles, splashes
         *
         * @param amount The new overall amount 
         */
        virtual void SetAmount(float amount) = 0;
        /**
         * Gets the overall amount of rain, puddles, splashes 
         *
         * @return The overall amount
         */
        virtual float GetAmount() = 0;
        
        /**
         * Sets the amount of darkening applied by the wetness of rain
         *
         * This will cause puddles and surfaces inside the radius to appear darker.
         *
         * @param diffuseDarkening The new amount of darkening applied by the wetness of the rain
         */
        virtual void SetDiffuseDarkening(float diffuseDarkening) = 0;
        /**
         * Gets the amount of darkening applied by the wetness of the rain
         *
         * @return The amount of darkening applied by the wetness of the rain
         */
        virtual float GetDiffuseDarkening() = 0;

        /**
         * Sets the amount of raindrops that can be seen in the air
         *
         * @param rainDropsAmount The new amount of raindrops that can be seen in the air
         */
        virtual void SetRainDropsAmount(float rainDropsAmount) = 0;
        /**
         * Gets the amount of raindrops that can be seen in the air
         *
         * @return The amount of raindrops that can be seen in the air
         */
        virtual float GetRainDropsAmount() = 0;
        
        /**
         * Sets the speed of raindrops as they fall through the air
         *
         * @param rainDropsAmount The new speed of raindrops
         */
        virtual void SetRainDropsSpeed(float rainDropsSpeed) = 0;
        /**
         * Gets the speed of raindrops as they fall through the air
         *
         * @return The speed of raindrops
         */
        virtual float GetRainDropsSpeed() = 0;
        
        /**
         * Sets the brightness of the raindrops
         *
         * @param rainDropsAmount The new brightness of the raindrops
         */
        virtual void SetRainDropsLighting(float rainDropsLighting) = 0;
        /**
         * Gets the brightness of the raindrops
         *
         * @return The brightness of the raindrops
         */
        virtual float GetRainDropsLighting() = 0;
        
        /**
         * Sets the depth and brightness of the puddles generated
         *
         * @param puddlesAmount The new depth and brightness of the puddles generated
         */
        virtual void SetPuddlesAmount(float puddlesAmount) = 0;
        /**
         * Gets the depth and brightness of the puddles generated
         *
         * @return The depth and brightness of the puddles generated
         */
        virtual float GetPuddlesAmount() = 0;
        
        /**
         * Sets the strength of the puddle mask applied to balance puddle results
         *
         * @param puddlesMaskAmount The new strength of the puddle mask applied to balance puddle results
         */
        virtual void SetPuddlesMaskAmount(float puddlesMaskAmount) = 0;
        /**
         * Gets the strength of the puddle mask applied to balance puddle results
         *
         * @return The strength of the puddle mask applied to balance puddle results
         */
        virtual float GetPuddlesMaskAmount() = 0;
        
        /**
         * Sets the strength and frequency of the ripples generated in puddles
         *
         * @param puddlesRippleAmount The new strength and frequency of the ripples generated in puddles
         */
         virtual void SetPuddlesRippleAmount(float puddlesRippleAmount) = 0;
        /**
         * Gets the strength and frequency of the ripples generated in puddles
         *
         * @return The strength and frequency of the ripples generated in puddles
         */
        virtual float GetPuddlesRippleAmount() = 0;
        
        /**
         * Sets the strength of the splash effects generated by the rain
         *
         * @param splashesAmount The new strength of the splash effects generated by the rain
         */
        virtual void SetSplashesAmount(float splashesAmount) = 0;
        /**
         * Gets the strength of the splash effects generated by the rain
         *
         * @return The strength of the splash effects generated by the rain
         */
        virtual float GetSplashesAmount() = 0;
        
        /**
         * Sets a collection of all rain parameters
         *
         * @param rainOptions The RainOptions object that will override all options for this rain component
         */
        virtual void SetRainOptions(RainOptions rainOptions) = 0;
        /**
         * Gets a collection of all rain parameters
         *
         * @return The RainOptions object used by the rain component
         */
        virtual RainOptions GetRainOptions() = 0;
        
        /**
         * Updates the engine to use this rain component as the base for rain simulation
         *
         * Whichever RainComponent calls this last is the one that "wins out"
         * and whose settings get used by the engine.
         */
        virtual void UpdateRain() = 0;
    };

    using RainComponentRequestBus = AZ::EBus<RainComponentRequests>;
} //namespace Rain