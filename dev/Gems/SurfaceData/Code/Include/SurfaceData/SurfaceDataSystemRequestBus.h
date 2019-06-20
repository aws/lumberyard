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

#include <AzCore/Component/Entity.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <SurfaceData/SurfaceDataTypes.h>

namespace SurfaceData
{
    /**
    * the EBus is used to request information about a surface
    */
    class SurfaceDataSystemRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        ////////////////////////////////////////////////////////////////////////

        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        virtual void GetSurfacePoints(const AZ::Vector3& inPosition, const SurfaceTagVector& masks, SurfacePointList& surfacePointList) const = 0;

        virtual SurfaceDataRegistryHandle RegisterSurfaceDataProvider(const SurfaceDataRegistryEntry& entry) = 0;
        virtual void UnregisterSurfaceDataProvider(const SurfaceDataRegistryHandle& handle) = 0;
        virtual void UpdateSurfaceDataProvider(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry, const AZ::Aabb& dirtyBoundsOverride) = 0;

        virtual SurfaceDataRegistryHandle RegisterSurfaceDataModifier(const SurfaceDataRegistryEntry& entry) = 0;
        virtual void UnregisterSurfaceDataModifier(const SurfaceDataRegistryHandle& handle) = 0;
        virtual void UpdateSurfaceDataModifier(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry, const AZ::Aabb& dirtyBoundsOverride) = 0;
    };

    typedef AZ::EBus<SurfaceDataSystemRequests> SurfaceDataSystemRequestBus;
}