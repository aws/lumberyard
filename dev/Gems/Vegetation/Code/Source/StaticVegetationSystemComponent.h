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

#include <AzCore/Component/Component.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/parallel/thread.h>
#include <GradientSignal/Ebuses/SectorDataRequestBus.h>
#include <Vegetation/StaticVegetationBus.h>
#include <CrySystemBus.h>

namespace Vegetation
{
    // Helper class that keeps a record of all the instances of static vegetation in the level
    class StaticVegetationSystemComponent :
        public AZ::Component,
        public StaticVegetationNotificationBus::Handler,
        public StaticVegetationRequestBus::Handler
    {
    public:
        AZ_COMPONENT(StaticVegetationSystemComponent, "{17D0362E-ADD8-487B-9CB0-54E5B1F0B9A9}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        //StaticVegetationRequestBus::Handler
        MapHandle GetStaticVegetation() override;

        // StaticVegetationNotificationBus::Handler
        void InstanceAdded(void* vegetationInstance, AZ::Aabb aabb) override;
        void InstanceRemoved(void* vegetationInstance, AZ::Aabb aabb) override;
        void VegetationCleared() override;

    private:
        StaticVegetationMap m_vegetation;
        AZ::u64 m_vegetationUpdateCount = 0;
        StaticVegetationMap m_vegetationReturnCopy;
        AZStd::atomic_ullong m_vegetationCopyUpdateCount {0};

        AZStd::mutex m_mapMutex;
        AZStd::shared_mutex m_mapCopyReadMutex;
    };
}
