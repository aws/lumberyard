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

namespace nv
{
    namespace cloth
    {
        class Factory;
        class Cloth;
    }
}

namespace NvCloth
{
    class SystemRequests
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual nv::cloth::Factory* GetClothFactory() = 0;
        virtual void AddCloth(nv::cloth::Cloth* cloth) = 0;
        virtual void RemoveCloth(nv::cloth::Cloth* cloth) = 0;
    };
    using SystemRequestBus = AZ::EBus<SystemRequests>;

    class SystemNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual void OnPreUpdateClothSimulation(float deltaTime) = 0;
        virtual void OnPostUpdateClothSimulation(float deltaTime) = 0;
    };
    using SystemNotificationsBus = AZ::EBus<SystemNotifications>;
} // namespace NvCloth
