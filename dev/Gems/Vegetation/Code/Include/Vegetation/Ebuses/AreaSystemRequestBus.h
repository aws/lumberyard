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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>

namespace Vegetation
{
    struct InstanceData;

    //determines if enumeration should proceed
    enum class AreaSystemEnumerateCallbackResult : AZ::u8
    {
        StopEnumerating = 0,
        KeepEnumerating,
    };
    using AreaSystemEnumerateCallback = AZStd::function<AreaSystemEnumerateCallbackResult(const InstanceData&)>;

    /**
    * A bus to signal the life times of vegetation areas
    * Note: all the API are meant to be queued events
    */
    class AreaSystemRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        // singleton pattern 
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~AreaSystemRequests() AZ_DEFAULT_METHOD;

        // register an area to override vegetation; returns a handle to used to unregister the area
        virtual void RegisterArea(AZ::EntityId areaId) = 0;
        virtual void UnregisterArea(AZ::EntityId areaId) = 0;
        virtual void RefreshArea(AZ::EntityId areaId) = 0;
        virtual void RefreshAllAreas() = 0;
        virtual void ClearAllAreas() = 0;

        // to allow areas to be combined into an area blender
        virtual void MuteArea(AZ::EntityId areaId) = 0;
        virtual void UnmuteArea(AZ::EntityId areaId) = 0;

        // visit all instances contained within bounds until callback decides otherwise
        virtual void EnumerateInstancesInAabb(const AZ::Aabb& bounds, AreaSystemEnumerateCallback callback) const = 0;
    };

    using AreaSystemRequestBus = AZ::EBus<AreaSystemRequests>;
} // namespace Vegetation

