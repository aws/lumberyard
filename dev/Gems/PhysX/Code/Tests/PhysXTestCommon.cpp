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

#include <Tests/PhysXTestCommon.h>

#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>

#include <PhysX/SystemComponentBus.h>

#include <BoxColliderComponent.h>
#include <RigidBodyComponent.h>
#include <SphereColliderComponent.h>
#include <TerrainComponent.h>

namespace PhysX
{
    namespace TestUtils
    {
        void UpdateDefaultWorld(float timeStep, AZ::u32 numSteps)
        {
            AZStd::shared_ptr<Physics::World> defaultWorld;
            Physics::DefaultWorldBus::BroadcastResult(defaultWorld, &Physics::DefaultWorldRequests::GetDefaultWorld);

            for (AZ::u32 i = 0; i < numSteps; i++)
            {
                defaultWorld->Update(timeStep);
            }
        }

        AZ::Data::Asset<PhysX::Pipeline::HeightFieldAsset> CreateHeightField(const AZStd::vector<uint16_t>& samples, int numRows, int numCols)
        {
            AZ_Assert((numRows * numCols) == samples.size(), "Mismatch between rows and cols with num samples");

            physx::PxCooking* cooking = nullptr;
            PhysX::SystemRequestsBus::BroadcastResult(cooking, &PhysX::SystemRequests::GetCooking);
            AZ_Assert(cooking != nullptr, "No cooking is present.");

            AZStd::vector<physx::PxHeightFieldSample> pxSamples;
            pxSamples.resize(numRows * numCols);

            for (size_t i = 0; i < samples.size(); ++i)
            {
                pxSamples[i].height = samples[i];
            }

            physx::PxHeightFieldDesc description;
            description.format = physx::PxHeightFieldFormat::eS16_TM;
            description.nbRows = numRows;
            description.nbColumns = numCols;
            description.samples.data = pxSamples.begin();
            description.samples.stride = sizeof(physx::PxHeightFieldSample);

            AZ::Data::Asset<PhysX::Pipeline::HeightFieldAsset> heightFieldAsset(AZ::Data::AssetManager::Instance().CreateAsset<PhysX::Pipeline::HeightFieldAsset>(AZ::Uuid::CreateRandom()));
            heightFieldAsset.Get()->SetHeightField(cooking->createHeightField(description, PxGetPhysics().getPhysicsInsertionCallback()));
            return heightFieldAsset;
        }

        EntityPtr CreateFlatTestTerrain(float width /*= 1.0f*/, float depth /*= 1.0f*/)
        {
            // Creates a single tiled, flat terrain at height 0
            EntityPtr terrain = AZStd::make_unique<AZ::Entity>("FlatTerrain");

            // 4 Corners, each at height zero
            AZStd::vector<uint16_t> samples = { 0, 0, 0, 0 };

            TerrainConfiguration configuration;
            configuration.m_scale = AZ::Vector3(width, depth, 1.0);
            configuration.m_heightFieldAsset = CreateHeightField(samples, 2, 2);
            terrain->AddComponent(aznew TerrainComponent(configuration));

            terrain->Init();
            terrain->Activate();

            return terrain;
        }

        EntityPtr CreateFlatTestTerrainWithMaterial(float width /*= 1.0f*/, float depth /*= 1.0f*/, const Physics::MaterialSelection& materialSelection /*= Physics::MaterialSelection()*/)
        {
            // Creates a single tiled, flat terrain at height 0
            EntityPtr terrain = AZStd::make_unique<AZ::Entity>("FlatTerrain");

            // 4 Corners, each at height zero
            AZStd::vector<uint16_t> samples = { 0, 0, 0, 0 };

            TerrainConfiguration configuration;
            configuration.m_scale = AZ::Vector3(width, depth, 1.0);
            configuration.m_heightFieldAsset = CreateHeightField(samples, 2, 2);
            configuration.m_terrainMaterialsToSurfaceIds.emplace(0, materialSelection);
            configuration.m_terrainSurfaceIdIndexMapping.emplace_back(0);

            terrain->AddComponent(aznew TerrainComponent(configuration));

            terrain->Init();
            terrain->Activate();

            return terrain;
        }

        EntityPtr CreateSlopedTestTerrain(float width /*= 1.0f*/, float depth /*= 1.0f*/, float height /*= 1.0f*/)
        {
            EntityPtr terrain = AZStd::make_unique<AZ::Entity>("SlopedTerrain");

            // Creates a 3x3 tiled sloped terrain (9 samples), where each sample's height is sum of grid coordinates:
            // i.e h = x + y
            AZStd::vector<uint16_t> samples;
            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    samples.push_back(i + j);
                }
            }

            TerrainConfiguration configuration;
            configuration.m_scale = AZ::Vector3(width, depth, height);
            configuration.m_heightFieldAsset = CreateHeightField(samples, 3, 3);
            terrain->AddComponent(aznew TerrainComponent(configuration));

            terrain->Init();
            terrain->Activate();

            return terrain;
        }

        EntityPtr CreateSphereEntity(const AZ::Vector3& position, const float radius, const AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig)
        {
            EntityPtr entity = AZStd::make_unique<AZ::Entity>("TestSphereEntity");
            entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
            entity->Init();

            entity->Activate();

            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

            entity->Deactivate();

            auto shapeConfig = AZStd::make_shared<Physics::SphereShapeConfiguration>(radius);
            auto shpereColliderComponent = entity->CreateComponent<PhysX::SphereColliderComponent>();
            shpereColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

            Physics::RigidBodyConfiguration rigidBodyConfig;
            entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

            entity->Activate();
            return entity;
        }

        EntityPtr CreateBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions,
            const AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig)
        {
            EntityPtr entity = AZStd::make_unique<AZ::Entity>("TestBoxEntity");
            entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
            entity->Init();

            entity->Activate();

            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

            entity->Deactivate();

            auto shapeConfig = AZStd::make_shared<Physics::BoxShapeConfiguration>(dimensions);
            auto boxColliderComponent = entity->CreateComponent<PhysX::BoxColliderComponent>();
            boxColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

            Physics::RigidBodyConfiguration rigidBodyConfig;
            entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

            entity->Activate();
            return entity;
        }
    }
}