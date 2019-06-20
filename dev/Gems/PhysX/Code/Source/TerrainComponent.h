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
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/Collision.h>
#include <AzFramework/Physics/TerrainBus.h>
#include <AzFramework/Physics/Material.h>
#include <PhysX/ComponentTypeIds.h>
#include <PhysX/HeightFieldAsset.h>

namespace PhysX
{
    class Shape;

    /// Configuration structure passed between editor and runtime.
    struct TerrainConfiguration
    {
        AZ_CLASS_ALLOCATOR(TerrainConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(TerrainConfiguration, "{0B545165-8629-4237-842A-59F1FE3FB6FA}");
        static void Reflect(AZ::ReflectContext* context);

        Physics::CollisionLayer m_collisionLayer;
        Physics::CollisionGroups::Id m_collisionGroup;
        AZStd::vector<int> m_terrainSurfaceIdIndexMapping;
        AZ::Vector3 m_scale = AZ::Vector3::CreateOne();
        AZ::Data::Asset<Pipeline::HeightFieldAsset> m_heightFieldAsset;
        Physics::TerrainMaterialSurfaceIdMap m_terrainMaterialsToSurfaceIds; ///< Lookup table mapping from
                                                                             ///< surface ids to terrain materials.
    };

    class TerrainComponent
        : public AZ::Component
        , public Physics::TerrainRequestBus::Handler
    {
    public:
        AZ_COMPONENT(TerrainComponent, TerrainComponentTypeId);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        TerrainComponent() = default;
        TerrainComponent(const TerrainComponent&) = delete;
        explicit TerrainComponent(const TerrainConfiguration& configuration);

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // TerrainRequestBus
        float GetHeight(float x, float y) override;
        Physics::RigidBodyStatic* GetTerrainTile(float x, float y) override;

    private:
        void LoadTerrain();

        AZStd::vector<AZStd::unique_ptr<Physics::RigidBodyStatic>> m_terrainTiles; ///< Terrain tile bodies.
        TerrainConfiguration m_configuration; ///< Terrain configuration.
    };
}