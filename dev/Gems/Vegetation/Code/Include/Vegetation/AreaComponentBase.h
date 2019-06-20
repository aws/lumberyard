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
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <Vegetation/Ebuses/AreaConfigRequestBus.h>
#include <Vegetation/EBuses/AreaNotificationBus.h>
#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <Vegetation/Ebuses/AreaInfoBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzCore/Component/TransformBus.h>

namespace Vegetation
{
    class AreaConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(AreaConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(AreaConfig, "{61599E53-2B6A-40AC-B5B8-FC1C3F87275E}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AreaLayer m_layer = AreaLayer::Foreground;

        static constexpr float s_priorityMin = 0.0f;
        static constexpr float s_priorityMax = 1.0f;
        float m_priority = s_priorityMin;
    };

    class AreaComponentBase
        : public AZ::Component
        , protected AreaNotificationBus::Handler
        , protected AreaRequestBus::Handler
        , protected AreaInfoBus::Handler
        , protected LmbrCentral::DependencyNotificationBus::Handler
        , protected AZ::TransformNotificationBus::Handler
        , protected LmbrCentral::ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_RTTI(AreaComponentBase, "{A50180C3-C14C-4292-BDBA-D7215F2EA7AB}", AZ::Component);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        AreaComponentBase(const AreaConfig& configuration);
        AreaComponentBase() = default;
        ~AreaComponentBase() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // AreaInfoBus
        float GetPriority() const override;
        AZ::u32 GetChangeIndex() const override;

        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::DependencyNotificationBus
        void OnCompositionChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // AreaNotificationBus
        void OnAreaConnect() override;
        void OnAreaDisconnect() override;
        void OnAreaRefreshed() override;

        //////////////////////////////////////////////////////////////////////////
        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        ////////////////////////////////////////////////////////////////////////
        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons reasons) override;

    private:
        AreaConfig m_configuration;
        AZStd::atomic_bool m_refreshPending{ false };
        AZStd::atomic_int m_changeIndex{ 0 };
    };
}