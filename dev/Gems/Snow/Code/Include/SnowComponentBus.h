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

namespace Snow
{
    class SnowOptions;

    /**
    * SnowComponentRequestBus
    * Messages serviced by the Snow component.
    */
    class SnowComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~SnowComponentRequests() = default;

        //! Enables the snow effect
        virtual void Enable() = 0;

        //! Disables the snow effect
        virtual void Disable() = 0;
        
        //! Toggles whether or not the snow effect is enabled
        virtual void Toggle() = 0;

        /**
         * Gets whether or not the snow effect is enabled
         * 
         * @return True if the snow effect is enabled
         */
        virtual bool IsEnabled() = 0;

        /**
         * Sets the radius of the snowy surface
         *
         * @param radius The new radius of the snowy surface
         */
        virtual void SetRadius(float radius) = 0;
        /**
         * Gets the radius of the snowy surface
         *
         * @return The radius of the snowy surface
         */
        virtual float GetRadius() = 0;

        /**
         * Sets the amount of snow on the surface
         *
         * @param snowAmount The new amount of snow on the surface
         */
        virtual void SetSnowAmount(float snowAmount) = 0;
        /**
         * Gets the amount of snow on the surface
         *
         * @return the amount of snow on the surface
         */
        virtual float GetSnowAmount() = 0;
        
        /**
         * Sets the amount of frost on the snowy surface
         *
         * @param frostAmount The new amount of frost on the snowy surface
         */
        virtual void SetFrostAmount(float frostAmount) = 0;
        /**
         * Gets the amount of frost on the snowy surface
         *
         * @return the amount of frost on the snowy surface
         */
        virtual float GetFrostAmount() = 0;
        
        /**
         * Sets the intensity of the surface freezing effect on the surface
         *
         * @param surfaceFreezing The new intensity of the surface freezing effect
         */
        virtual void SetSurfaceFreezing(float surfaceFreezing) = 0;
        /**
         * Gets the intensity of the surface freezing effect on the surface
         *
         * @return the intensity of the surface freezing effect
         */
        virtual float GetSurfaceFreezing() = 0;
        
        /**
         * Sets the amount of snowflakes produced
         *
         * This parameter is affected by the SnowAmount parameter
         *
         * @param snowFlakeCount The new amount of snowflakes produced
         */
        virtual void SetSnowFlakeCount(AZ::u32 snowFlakeCount) = 0;
        /**
         * Gets the amount of snowflakes produced
         *
         * @return The amount of snowflakes produced
         */
        virtual AZ::u32 GetSnowFlakeCount() = 0;
        
        /**
         * Sets size of the snowflakes produced
         *
         * @param snowFlakeSize The new size of the snowflakes produced
         */
        virtual void SetSnowFlakeSize(float snowFlakeSize) = 0;
        /**
         * Gets the size of the snowflakes produced
         *
         * @return The size of the snowflakes produced
         */
        virtual float GetSnowFlakeSize() = 0;
        
        /**
         * Sets the brightness of the snowflakes
         *
         * @param snowFallBrightness The new brightness of the snowflakes
         */
        virtual void SetSnowFallBrightness(float snowFallBrightness) = 0;
        /**
         * Gets the brightness of the snowflakes
         *
         * @return The brightness of the snowflakes 
         */
        virtual float GetSnowFallBrightness() = 0;
        
        /**
         * Sets the scale applied to the gravity affecting the snowflakes
         *
         * @param snowFallGravityScale The new scale applied to the gravity affecting the snowflakes
         */
        virtual void SetSnowFallGravityScale(float snowFallGravityScale) = 0;
        /**
         * Gets the scale applied to the gravity affecting the snowflakes
         *
         * @return The scale applied to the gravity affecting the snowflakes
         */
        virtual float GetSnowFallGravityScale() = 0;
        
        /**
         * Sets the scale applied to the wind affecting the snowflakes
         *
         * @param snowFallWindScale The new scale applied to the wind affecting the snowflakes
         */
        virtual void SetSnowFallWindScale(float snowFallWindScale) = 0;
        /**
         * Gets the scale applied to the wind affecting the snowflakes
         *
         * @return The scale applied to the wind affecting the snowflakes
         */
        virtual float GetSnowFallWindScale() = 0;
        
        /**
        * Sets the turbulence applied to the snowflakes
        *
        * @param snowFallTurbulence The new turbulence applied to the snowflakes
        */
        virtual void SetSnowFallTurbulence(float snowFallTurbulence) = 0;
        /**
         * Gets the turbulence applied to the snowflakes
         *
         * @return The turbulence applied to the snowflakes
         */
        virtual float GetSnowFallTurbulence() = 0;
        
        /**
         * Sets the turbulence frequency applied to the snowflakes
         *
         * @param snowFallTurbulenceFreq The new turbulence frequency applied to the snowflakes
         */
        virtual void SetSnowFallTurbulenceFreq(float snowFallTurbulenceFreq) = 0;
        /**
         * Gets the turbulence frequency applied to the snowflakes
         *
         * @return The turbulence frequency applied to the snowflakes
         */
        virtual float GetSnowFallTurbulenceFreq() = 0;
        
        /**
         * Sets all options for the snow component
         *
         * @param snowOptions The SnowOptions object that will override all options for this snow component
         */
        virtual void SetSnowOptions(SnowOptions snowOptions) = 0;
        /**
         * Gets a structure that contains all the options for the snow component
         *
         * @return The SnowOptions object used by the snow component
         */
        virtual SnowOptions GetSnowOptions() = 0;
        
        /**
         * Updates the engine to use this snow component as the base for snow simulation
         *
         * Whichever SnowComponent calls this last is the one that "wins out"
         * and whose settings get used by the engine.
         */
        virtual void UpdateSnow() = 0;
    };

    using SnowComponentRequestBus = AZ::EBus<SnowComponentRequests>;
} //namespace Snow