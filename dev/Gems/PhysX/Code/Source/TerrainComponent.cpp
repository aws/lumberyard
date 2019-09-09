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

#include <PhysX_precompiled.h>
#include <TerrainComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/IO/SystemFile.h>

#include <AzFramework/Physics/Utils.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/World.h>

#include <PhysX/ConfigurationBus.h>

#include <Source/RigidBodyStatic.h>
#include <Source/Utils.h>

#include <Shape.h>
#include <Material.h>

namespace PhysX
{
    void TerrainConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TerrainConfiguration>()
                ->Version(1)
                ->Field("CollisionLayer", &TerrainConfiguration::m_collisionLayer)
                ->Field("CollisionGroup", &TerrainConfiguration::m_collisionGroup)
                ->Field("MaterialMapping", &TerrainConfiguration::m_terrainSurfaceIdIndexMapping)
                ->Field("Scale", &TerrainConfiguration::m_scale)
                ->Field("HeightField", &TerrainConfiguration::m_heightFieldAsset)
                ->Field("TerrainMaterials", &TerrainConfiguration::m_terrainMaterialsToSurfaceIds)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TerrainConfiguration>("Terrain Configuration", "configuration for terrain")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainConfiguration::m_collisionLayer, "Collision Layer", "Collision layer assigned to the terrain")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainConfiguration::m_collisionGroup, "Collision Group", "Collision group assigned to the terrain")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainConfiguration::m_heightFieldAsset, "HeightField Asset", "Height field asset")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                    ;
            }
        }
    }

    void TerrainComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainConfiguration::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainComponent, AZ::Component>()
                ->Field("TerrainConfiguration", &TerrainComponent::m_configuration)
                ->Version(0)
                ;
        }
    }

    void TerrainComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PhysXTerrain", 0x5c55674e));
    }

    void TerrainComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("PhysXTerrain", 0x5c55674e));
    }

    void TerrainComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void TerrainComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    TerrainComponent::TerrainComponent(const TerrainConfiguration& configuration)
        : m_configuration(configuration)
    {
    }

    void TerrainComponent::Activate()
    {
        m_terrainTiles.clear();
        Physics::TerrainRequestBus::Handler::BusConnect(GetEntityId());
        LoadTerrain();
        Utils::LogWarningIfMultipleComponents<Physics::TerrainRequestBus>(
            "TerrainComponent", 
            "Multiple TerrainComponents found in the scene on these entities:");
    }

    void TerrainComponent::Deactivate()
    {
        Physics::TerrainRequestBus::Handler::BusDisconnect();
        m_terrainTiles.clear();
    }

    float TerrainComponent::GetHeight(float x, float y)
    {
        auto physxGenericShape = static_cast<PhysX::Shape*>(m_terrainTiles[0]->GetShape(0).get());
        physx::PxShape* pxShape = physxGenericShape->GetPxShape();
        physx::PxHeightFieldGeometry geometry;
        if (pxShape->getHeightFieldGeometry(geometry))
        {
            return geometry.heightField->getHeight(x / m_configuration.m_scale.GetX(), y / m_configuration.m_scale.GetY()) * m_configuration.m_scale.GetZ();
        }
        else
        {
            AZ_Warning("TerrainComponent", false, "Shape type is not a heightfield.");
            return 0.0f;
        }
    }

    Physics::RigidBodyStatic* TerrainComponent::GetTerrainTile(float x, float y)
    {
        if (!m_terrainTiles.empty())
        {
            return m_terrainTiles[0].get();
        }
        return nullptr;
    }

    void TerrainComponent::LoadTerrain()
    {
        // Add to all physics worlds
        Physics::WorldRequestBus::EnumerateHandlers([this](Physics::World* world)
        {
            // Create terrain actor and add to the world
            AZStd::unique_ptr<Physics::RigidBodyStatic> terrainTile = Utils::CreateTerrain(m_configuration, GetEntityId(), GetEntity()->GetName().c_str());
            if (terrainTile)
            {
                world->AddBody(*terrainTile);
                m_terrainTiles.push_back(AZStd::move(terrainTile));
            }
            return true;
        });
    }
}