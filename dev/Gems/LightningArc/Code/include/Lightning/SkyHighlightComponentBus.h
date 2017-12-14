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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Color.h>

namespace Lightning
{
    // Request bus for the component
    class SkyHighlightComponentRequests
        : public AZ::ComponentBus
    {
    public:
        // EBus Traits overrides (Configuring this EBus)
        // Using Defaults

        virtual ~SkyHighlightComponentRequests() {}

        //! Enables the sky highlight effect
        virtual void Enable() = 0;
        
        //! Disables the sky highlight effect
        virtual void Disable() = 0;
        
        //! Toggles whether or not the sky highlight effect is enabled
        virtual void Toggle() = 0;
        /**
         * Gets whether or not the sky highlight effect is enabled
         *
         * @return True if the sky highlight effect is enabled
         */
        virtual bool IsEnabled() = 0;

        /**
         * Sets the color of the sky highlight
         *
         * This will be multiplied by the ColorMultiplier parameter to calculate the final sky highlight color.
         *
         * @param color The new color to set for the sky highlight effect
         */
        virtual void SetColor(const AZ::Color& color) = 0;
        /**
         * Gets the color of the sky highlight
         *
         * This value is not affected by the ColorMultiplier parameter.
         *
         * @return The color of the sky highlight effect
         */
        virtual const AZ::Color& GetColor() = 0;

        /**
         * Sets the color multiplier of the sky highlight's color
         *
         * This will be multiplied into the Color parameter to calculate the final sky highlight color.
         *
         * @param colorMultiplier The new colorMultiplier to set for the sky highlight effect
         */
        virtual void SetColorMultiplier(float colorMultiplier) = 0;
        /**
         * Gets the color multiplier of the sky highlight's color
         *
         * @return The color multiplier for the sky highlight effect
         */
        virtual float GetColorMultiplier() = 0;

        /**
         * Sets how far offset on the global Z axis the sky highlight effect should be from the entity's transform
         *
         * @param verticalOffset The new vertical offset to set for the sky highlight effect
         */
        virtual void SetVerticalOffset(float verticalOffset) = 0;
        /**
         * Gets the vertical offset of the sky highlight
         *
         * @return The vertical offset of the sky highlight effect from the entity's transform
         */
        virtual float GetVerticalOffset() = 0;

        /**
         * Sets the size of the sky highlight
         *
         * @param size The new size to set for the sky highlight effect
         */
        virtual void SetSize(float size) = 0;
        /**
         * Gets the size of the sky highlight
         *
         * @return The size or "attenuation" of the sky highlight effect
         */
        virtual float GetSize() = 0;
    };

    using SkyHighlightComponentRequestBus = AZ::EBus<SkyHighlightComponentRequests>;

} // namespace Lightning
