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

struct IClipVolume;

namespace LmbrCentral
{
    //! Requests for the Clip Volume component.
    class ClipVolumeComponentRequests
        : public AZ::ComponentBus
    {
    public:

        virtual ~ClipVolumeComponentRequests() {}

        //! Returns the currently used render clip volume.
        //! @return The current clip volume.
        virtual IClipVolume* GetClipVolume() = 0;
    };

    using ClipVolumeComponentRequestBus = AZ::EBus<ClipVolumeComponentRequests>;

    //! Notifications for the clip volume component. Used to be notified when a clip volume is created or destroyed.
    class ClipVolumeComponentNotifications
        : public AZ::ComponentBus
    {
    public:

        //! Custom connection policy notifies connecting listeners immediately if clip volume has already been created.
        template<class Bus>
        struct ClipVolumeConnectionPolicy
            : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                IClipVolume* instance = nullptr;
                ClipVolumeComponentRequestBus::EventResult(instance, id, &ClipVolumeComponentRequestBus::Events::GetClipVolume);
                if (instance)
                {
                    handler->OnClipVolumeCreated(instance);
                }
            }
        };
        template<typename Bus>
        using ConnectionPolicy = ClipVolumeConnectionPolicy<Bus>;

        //! Called when a clip volume is created.
        //! @param clipVolume The created clip volume.
        virtual void OnClipVolumeCreated(IClipVolume* clipVolume) = 0;

        //! Called when a clip volume is destroyed.
        //! @param clipVolume The clip volume that was destroyed.
        virtual void OnClipVolumeDestroyed(IClipVolume* clipVolume) = 0;
    };

    using ClipVolumeComponentNotificationBus = AZ::EBus<ClipVolumeComponentNotifications>;

} // namespace LmbrCentral
