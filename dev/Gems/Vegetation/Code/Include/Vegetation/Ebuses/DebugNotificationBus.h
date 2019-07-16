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

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/ComponentBus.h>
#include <Vegetation/InstanceData.h>

namespace Vegetation
{
    using TimePoint = AZStd::chrono::system_clock::time_point;
    using TimeSpan = AZStd::sys_time_t;
    using FilterReasonCount = AZStd::unordered_map<AZStd::string_view, AZ::u32>;

    class DebugNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        static const bool EnableEventQueue = true;
        static const bool EventQueueingActiveByDefault = false;

        // fill reporting

        virtual void FillSectorStart(int sectorX, int sectorY, TimePoint timePoint) {};
        virtual void FillSectorEnd(int sectorX, int sectorY, TimePoint timePoint, AZ::u32 unusedClaimPointCount) {};

        virtual void FillAreaStart(AZ::EntityId areaId, TimePoint timePoint) {};
        virtual void MarkAreaRejectedByMask(AZ::EntityId areaId) {};
        virtual void FillAreaEnd(AZ::EntityId areaId, TimePoint timePoint, AZ::u32 unusedClaimPointCount) {};

        virtual void FilterInstance(AZ::EntityId areaId, AZStd::string_view filterReason) {};
        virtual void CreateInstance(InstanceId instanceId, AZ::Vector3 position, AZ::EntityId areaId) {};
        virtual void DeleteInstance(InstanceId instanceId) {};
        virtual void DeleteAllInstances() {};

        virtual void SetAreaDebugColor(AZ::EntityId areaId, AZ::Color debugColor, bool render) {};

        // input requests

        virtual void ExportCurrentReport() {}; // writes out the current report to disk, helper for cvars
        virtual void ToggleVisualization() {}; // toggles the 3D visualization in the 3D client, helper for cvars.
    };
    using DebugNotificationBus = AZ::EBus<DebugNotifications>;

    // VEG_PROFILE_ENABLED is defined in the wscript
    // VEG_PROFILE_ENABLED is only defined in the Vegetation gem by default
#if defined(VEG_PROFILE_ENABLED)
#define VEG_PROFILE_METHOD(Method) Method
#else
#define VEG_PROFILE_METHOD(...) // no-op
#endif
}