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
#include "MapHandle.h"

struct IRenderNode;

namespace Vegetation
{
    using UniqueVegetationInstancePtr = void*;

    //! Signals when static (legacy) vegetation instances are added or removed
    class StaticVegetationNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const bool LocklessDispatch = true;
        using MutexType = AZStd::recursive_mutex;

        /** Triggered when a static vegetation instance is added
            vegetationInstance is just a unique address to refer to the AABB being added, it is not dereferenced
        **/
        virtual void InstanceAdded(UniqueVegetationInstancePtr vegetationInstance, AZ::Aabb bounds) {}
        
        /** Triggered when a static vegetation instance is removed
            vegetationInstance is just a unique address to refer to the AABB being removed, it is not dereferenced
        **/
        virtual void InstanceRemoved(UniqueVegetationInstancePtr vegetationInstance, AZ::Aabb bounds) {}
        
        //! Triggered when all vegetation is cleared
        virtual void VegetationCleared() {}
    };

    using StaticVegetationNotificationBus = AZ::EBus<StaticVegetationNotifications>;

    //! Allows requesting info about static (legacy) vegetation
    class StaticVegetationRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const bool LocklessDispatch = true;
        using MutexType = AZStd::recursive_mutex;

        /** Returns a handle to the vegetation map
            The MapHandle class has a count of the total number of updates to the map that have been made.
            Classes that use the MapHandle and need to know if the map has updated are responsible for remembering
            the number of updates last time they called this function.
        **/
        virtual MapHandle GetStaticVegetation() = 0;
    };

    using StaticVegetationRequestBus = AZ::EBus<StaticVegetationRequests>;
}