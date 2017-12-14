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

    /*!
    * RainComponentRequestBus
    * Messages serviced by the Rain component.
    */
    class RainComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~RainComponentRequests() = default;

        virtual void Enable() = 0;
        virtual void Disable() = 0;
        virtual void Toggle() = 0;

        virtual bool IsEnabled() = 0;

        /*
            Getters and Setters for individual parameters
        */

        virtual bool GetIgnoreVisAreas() = 0;
        virtual void SetIgnoreVisAreas(bool ignoreVisAreas) = 0;

        virtual bool GetDisableOcclusion() = 0;
        virtual void SetDisableOcclusion(bool disableOcclusion) = 0;

        virtual float GetRadius() = 0;
        virtual void SetRadius(float radius) = 0;

        virtual float GetAmount() = 0;
        virtual void SetAmount(float amount) = 0;
        
        virtual float GetDiffuseDarkening() = 0;
        virtual void SetDiffuseDarkening(float diffuseDarkening) = 0;

        virtual float GetRainDropsAmount() = 0;
        virtual void SetRainDropsAmount(float rainDropsAmount) = 0;

        virtual float GetRainDropsSpeed() = 0;
        virtual void SetRainDropsSpeed(float rainDropsSpeed) = 0;

        virtual float GetRainDropsLighting() = 0;
        virtual void SetRainDropsLighting(float rainDropsLighting) = 0;

        virtual float GetPuddlesAmount() = 0;
        virtual void SetPuddlesAmount(float puddlesAmount) = 0;

        virtual float GetPuddlesMaskAmount() = 0;
        virtual void SetPuddlesMaskAmount(float puddlesMaskAmount) = 0;

        virtual float GetPuddlesRippleAmount() = 0;
        virtual void SetPuddlesRippleAmount(float puddlesRippleAmount) = 0;

        virtual float GetSplashesAmount() = 0;
        virtual void SetSplashesAmount(float splashesAmount) = 0;

        /*
            Getter and Setter for entire RainOptions object
        */
        virtual RainOptions GetRainOptions() = 0;
        virtual void SetRainOptions(RainOptions rainOptions) = 0;

        /**
        * Provides the rain settings for this component to the 
        * underlying 3DEngine for rendering rain. Whichever 
        * Rain component calls this last is the one that wins out
        * and whose settings get used by the engine.
        */
        virtual void UpdateRain() = 0;
    };

    using RainComponentRequestBus = AZ::EBus<RainComponentRequests>;
} //namespace Rain