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

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace LegacyTerrain
{
    class LegacyTerrainLevelConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(LegacyTerrainLevelConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(LegacyTerrainLevelConfig, "{65950218-99C8-4DF5-A949-6642A8C69444}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
    };

    class LegacyTerrainLevelComponent
        : public AZ::Component
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(LegacyTerrainLevelComponent, "{9BA33CA7-07DF-409F-A240-5A7AA67EFFE8}");

        LegacyTerrainLevelComponent(const LegacyTerrainLevelConfig& configuration);
        LegacyTerrainLevelComponent() = default;
        ~LegacyTerrainLevelComponent() = default;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;
        ////////////////////////////////////////////////////////////////////////

    private:
        LegacyTerrainLevelConfig m_configuration;
    };
}
