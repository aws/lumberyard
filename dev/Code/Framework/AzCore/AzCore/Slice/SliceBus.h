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
#ifndef AZCORE_SLICE_BUS_H
#define AZCORE_SLICE_BUS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class SliceComponent;

    /**
     */
    class SliceEvents
        : public ComponentBus
    {
    public:
        virtual ~SliceEvents() {}
    };

    typedef EBus<SliceEvents> SliceBus;


    /**
     * Dispatches events at key points of SliceAsset serialization.
     */
    class SliceAssetSerializationNotifications
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Multiple;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single;
        ////////////////////////////////////////////////////////////////////////

        /// Called after writing deserialized data to a SliceAsset instance.
        virtual void OnWriteDataToSliceAssetEnd(AZ::SliceComponent& /*sliceAsset*/) {}
    };
    using SliceAssetSerializationNotificationBus = AZ::EBus<SliceAssetSerializationNotifications>;

    /// @deprecated Use SliceComponent.
    using PrefabComponent = SliceComponent;

    /// @deprecated Use SliceEvents.
    using PrefabEvents = SliceEvents;

    /// @deprecated Use SliceBus.
    using PrefabBus = SliceBus;
}

#endif // AZCORE_SLICE_BUS_H
#pragma once