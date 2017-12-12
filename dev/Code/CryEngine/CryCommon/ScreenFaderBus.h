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

namespace AZ
{
    //////////////////////////////////////////////////////////////////////////
    /// This bus allows controlling of basic screen fading through a specific fade controller
    //////////////////////////////////////////////////////////////////////////
    class ScreenFaderRequests
        : public AZ::EBusTraits
    {
    public:
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef int BusIdType; // Fader ID
        
        //! Triggers fading out to a solid color
        //! \param  color             Color to fade out to
        //! \param  duration          Number of seconds the fade should take
        //! \param  useCurrentColor   If true, the transition begins from the current color left over from any prior fading, and then fades to "color". If false, the transition begins fully transparent and then fades to "color".
        //! \param  updateAlways      Continue fading even when the game is paused
        virtual void FadeOut(const AZ::Color& color, float duration, bool useCurrentColor = true, bool updateAlways = false) = 0;

        //! Triggers fading back in from a solid color
        //! \param  color             Color to fade through. Should be ignored if useCurrentColor is true.
        //! \param  duration          Number of seconds the fade should take
        //! \param  useCurrentColor   If true, the transition begins from the current color left over from any prior fading, and the "color" param is ignored. If false, the transition begins from "color" and fades to transparent.
        //! \param  updateAlways      Continue fading even when the game is paused
        virtual void FadeIn(const AZ::Color& color, float duration, bool useCurrentColor = true, bool updateAlways = false) = 0;

        //! Sets a texture to be used my the fade mask. Use empty string to clear the texture.
        virtual void SetTexture(const AZStd::string& textureName) = 0;

        //! Sets the screen coordinates where the fade mask will be drawn (left,top,right,bottom). The default is fullscreen (0,0,1,1).
        virtual void SetScreenCoordinates(const AZ::Vector4& screenCoordinates) = 0;

        //! Returns the current color of the fade mask
        virtual AZ::Color GetCurrentColor() = 0;
    };
    using ScreenFaderRequestBus = AZ::EBus<ScreenFaderRequests>;

    //////////////////////////////////////////////////////////////////////////
    /// This bus allows ScreenFader to send notifications
    //////////////////////////////////////////////////////////////////////////
    class ScreenFaderNotifications
        : public AZ::EBusTraits
    {
    public:
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef int BusIdType; // Fader ID
        
        //! Called when a fade-out has completed
        virtual void OnFadeOutComplete() {}

        //! Called when a fade-in has completed
        virtual void OnFadeInComplete() {}

    };
    using ScreenFaderNotificationBus = AZ::EBus<ScreenFaderNotifications>;
    
    //////////////////////////////////////////////////////////////////////////
    /// This bus supports management of the set of available ScreenFaders
    //////////////////////////////////////////////////////////////////////////
    class ScreenFaderManagementRequests
        : public AZ::EBusTraits
    {
    public:
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single;

        //! Returns the number of available fader IDs (not necessarily the number of Faders that have been created).
        virtual int GetNumFaderIDs() = 0;
    };
    using ScreenFaderManagementRequestBus = AZ::EBus<ScreenFaderManagementRequests>;


} // namespace AZ