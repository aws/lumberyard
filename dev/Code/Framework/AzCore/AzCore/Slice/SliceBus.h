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
#include <AzCore/Component/EntityId.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    class Entity;

    /**
     */
    class SliceEvents
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        virtual ~SliceEvents() {}
    };

    typedef EBus<SliceEvents> SliceBus;

    /**
    * Events dispatched during the creation, modification, and destruction of a slice instance
    */
    class SliceInstanceEvents
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        /// Called when a slice metadata entity has been destroyed.
        virtual void OnMetadataEntityCreated(const SliceComponent::SliceInstanceAddress& /*sliceAddress*/, AZ::Entity& /*entity*/) {}

        virtual void OnMetadataEntityDestroyed(AZ::EntityId /*entityId*/) {}
    };
    using SliceInstanceNotificationBus = AZ::EBus<SliceInstanceEvents>;

    /**
     * Dispatches events at key points of SliceAsset serialization.
     */
    class SliceAssetSerializationNotifications
        : public AZ::EBusTraits
    {
    public:
        using MutexType = AZStd::recursive_mutex;

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
} // namespace AZ