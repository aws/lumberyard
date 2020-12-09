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
#include "NvClothTest.h"

#include <AzCore/Interface/Interface.h>
#include <NvCloth/IClothSystem.h>

#include <Components/ClothComponent.h>
#include <System/FabricCooker.h>
#include <System/SystemComponent.h>

using ::testing::NiceMock;

namespace NvCloth
{
    void CrySystemMock::BroadcastCrySystemInitialized()
    {
        SSystemInitParams defaultSystemInitParams;
        CrySystemEventBus::Broadcast(&CrySystemEvents::OnCrySystemInitialized,
            *this,
            defaultSystemInitParams);
    }

    void NvClothTestEnvironment::SetupEnvironment()
    {
        AZ::Test::GemTestEnvironment::SetupEnvironment();
    }

    void NvClothTestEnvironment::AddGemsAndComponents()
    {
        AddComponentDescriptors({
            SystemComponent::CreateDescriptor(),
            ClothComponent::CreateDescriptor()
            });
        AddRequiredComponents({ SystemComponent::TYPEINFO_Uuid() });
    }

    void NvClothTestEnvironment::TeardownEnvironment()
    {
        GemTestEnvironment::TeardownEnvironment();
        m_fabricCooker.reset();
        NvCloth::SystemComponent::TearDownNvClothLibrary(); // This call destroys AzClothAllocator. SystemAllocator destroy must come after this call.
        GemTestEnvironment::TeardownAllocatorAndTraceBus(); // SystemAllocator, OSAllocator, and AllocatorManager destroyed here.
    }

    void NvClothTestEnvironment::PostCreateApplication()
    {
        NvCloth::SystemComponent::InitializeNvClothLibrary(); // Must be called after environment setup.
        m_fabricCooker = AZStd::make_unique<FabricCooker>();
    }

    void NvClothTestFixture::SetUpTestCase()
    {
        if (!AZ::Interface<NvCloth::IClothSystem>::Get())
        {
            NiceMock<CrySystemMock> crySystemMock;
            crySystemMock.BroadcastCrySystemInitialized();

            using milliseconds = AZStd::chrono::milliseconds;
            const milliseconds waitInterval = AZStd::chrono::milliseconds(100);
            const milliseconds timeOut = AZStd::chrono::milliseconds(5000);

            // Wait for cloth system component to initialize.
            for (milliseconds timePassedMs = AZStd::chrono::milliseconds(0); timePassedMs < timeOut; timePassedMs += waitInterval)
            {
                if (AZ::Interface<NvCloth::IClothSystem>::Get())
                {
                    return;
                }
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(waitInterval));
            }

            // Timed out initializing cloth, try again to assert that cloth is initialized, otherwise error.
            ASSERT_TRUE(AZ::Interface<NvCloth::IClothSystem>::Get());
        }
    }

    void NvClothTestFixture::TearDownTestCase()
    {
    }

    void NvClothTestFixture::SetUp()
    {
        m_preSimulationEventHandler = NvCloth::ICloth::PreSimulationEvent::Handler(
            [this](NvCloth::ClothId clothId, float deltaTime)
            {
                this->OnPreSimulation(clothId, deltaTime);
            });
        m_postSimulationEventHandler = NvCloth::ICloth::PostSimulationEvent::Handler(
            [this](NvCloth::ClothId clothId, float deltaTime, const AZStd::vector<NvCloth::SimParticleFormat>& updatedParticles)
            {
                this->OnPostSimulation(clothId, deltaTime, updatedParticles);
            });

        bool clothCreated = CreateCloth();
        ASSERT_TRUE(clothCreated);
    }

    void NvClothTestFixture::TearDown()
    {
        DestroyCloth();
    }

    bool NvClothTestFixture::CreateCloth()
    {
        const float width = 2.0f;
        const float height = 2.0f;
        const AZ::u32 segmentsX = 10;
        const AZ::u32 segmentsY = 10;

        const TriangleInputPlane planeXY = TriangleInputPlane::CreatePlane(width, height, segmentsX, segmentsY);

        // Cook Fabric
        AZStd::optional<NvCloth::FabricCookedData> cookedData = AZ::Interface<NvCloth::IFabricCooker>::Get()->CookFabric(planeXY.m_vertices, planeXY.m_indices);
        if (!cookedData)
        {
            return false;
        }

        // Create cloth instance
        m_cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth(planeXY.m_vertices, *cookedData);
        if (!m_cloth)
        {
            return false;
        }

        m_sphereColliders.emplace_back(512.0f, 512.0f, 35.0f, 1.0f);
        m_clothTransform.SetTranslation(AZ::Vector3(512.0f, 519.0f, 35.0f));

        m_cloth->GetClothConfigurator()->SetTransform(m_clothTransform);
        m_cloth->GetClothConfigurator()->ClearInertia();

        // Add cloth to default solver to be simulated
        AZ::Interface<NvCloth::IClothSystem>::Get()->AddCloth(m_cloth);

        return true;
    }

    void NvClothTestFixture::DestroyCloth()
    {
        if (m_cloth)
        {
            AZ::Interface<NvCloth::IClothSystem>::Get()->RemoveCloth(m_cloth);
            AZ::Interface<NvCloth::IClothSystem>::Get()->DestroyCloth(m_cloth);
        }
    }

    void NvClothTestFixture::OnPreSimulation(NvCloth::ClothId clothId, float deltaTime)
    {
        m_cloth->GetClothConfigurator()->SetTransform(m_clothTransform);

        static float time = 0.0f;
        static float velocity = 1.0f;

        time += deltaTime;

        for (auto& sphere : m_sphereColliders)
        {
            sphere.SetY(sphere.GetY() + velocity * deltaTime);
        }

        auto clothInverseTransform = m_clothTransform.GetInverseFast();
        auto sphereColliders = m_sphereColliders;
        for (auto& sphere : sphereColliders)
        {
            sphere.Set(clothInverseTransform * sphere.GetAsVector3(), sphere.GetW());
        }
        m_cloth->GetClothConfigurator()->SetSphereColliders(sphereColliders);
    }

    void NvClothTestFixture::OnPostSimulation(
        NvCloth::ClothId clothId, float deltaTime,
        const AZStd::vector<NvCloth::SimParticleFormat>& updatedParticles)
    {
        AZ_UNUSED(clothId);
        AZ_UNUSED(deltaTime);
        AZ_UNUSED(updatedParticles);
        m_postSimulationEventInvoked = true;
    }

    TriangleInputPlane TriangleInputPlane::CreatePlane(float width, float height, AZ::u32 segmentsX, AZ::u32 segmentsY)
    {
        TriangleInputPlane plane;

        plane.m_vertices.resize((segmentsX + 1) * (segmentsY + 1));
        plane.m_uvs.resize((segmentsX + 1) * (segmentsY + 1));
        plane.m_indices.resize((segmentsX * segmentsY * 2) * 3);

        const NvCloth::SimParticleFormat topLeft(
            -width * 0.5f,
            -height * 0.5f,
            0.0f,
            0.0f);

        // Vertices and UVs
        for (AZ::u32 y = 0; y < segmentsY + 1; ++y)
        {
            for (AZ::u32 x = 0; x < segmentsX + 1; ++x)
            {
                const AZ::u32 segmentIndex = x + y * (segmentsX + 1);
                const float fractionX = ((float)x / (float)segmentsX);
                const float fractionY = ((float)y / (float)segmentsY);

                NvCloth::SimParticleFormat position(
                    fractionX * width,
                    fractionY * height,
                    0.0f,
                    (y > 0) ? 1.0f : 0.0f);

                plane.m_vertices[segmentIndex] = topLeft + position;
                plane.m_uvs[segmentIndex] = NvCloth::SimUVType(fractionX, fractionY);
            }
        }

        // Triangles indices
        for (AZ::u32 y = 0; y < segmentsY; ++y)
        {
            for (AZ::u32 x = 0; x < segmentsX; ++x)
            {
                const AZ::u32 segmentIndex = (x + y * segmentsX) * 2 * 3;

                const AZ::u32 firstTriangleStartIndex = segmentIndex;
                const AZ::u32 secondTriangleStartIndex = segmentIndex + 3;

                //Top left to bottom right

                plane.m_indices[firstTriangleStartIndex + 0] = static_cast<NvCloth::SimIndexType>((x + 0) + (y + 0) * (segmentsX + 1));
                plane.m_indices[firstTriangleStartIndex + 1] = static_cast<NvCloth::SimIndexType>((x + 1) + (y + 0) * (segmentsX + 1));
                plane.m_indices[firstTriangleStartIndex + 2] = static_cast<NvCloth::SimIndexType>((x + 1) + (y + 1) * (segmentsX + 1));

                plane.m_indices[secondTriangleStartIndex + 0] = static_cast<NvCloth::SimIndexType>((x + 0) + (y + 0) * (segmentsX + 1));
                plane.m_indices[secondTriangleStartIndex + 1] = static_cast<NvCloth::SimIndexType>((x + 1) + (y + 1) * (segmentsX + 1));
                plane.m_indices[secondTriangleStartIndex + 2] = static_cast<NvCloth::SimIndexType>((x + 0) + (y + 1) * (segmentsX + 1));
            }
        }

        return plane;
    }

    //! Smallest Z and largest Y coordinates for a list of particles before, and a list of particles after simulation for some time.
    struct ParticleBounds
    {
        float m_beforeSmallestZ = std::numeric_limits<float>::max();
        float m_beforeLargestY = -std::numeric_limits<float>::max();

        float m_afterSmallestZ = std::numeric_limits<float>::max();
        float m_afterLargestY = -std::numeric_limits<float>::max();
    };

    ParticleBounds GetBeforeAndAfterParticleBounds(const AZStd::vector<NvCloth::SimParticleFormat>& particlesBefore,
        const AZStd::vector<NvCloth::SimParticleFormat>& particlesAfter)
    {
        assert(particlesBefore.size() == particlesAfter.size());

        ParticleBounds beforeAndAfterParticleBounds;

        for (size_t particleIndex = 0; particleIndex < particlesBefore.size(); ++particleIndex)
        {
            if (particlesBefore[particleIndex].GetZ() < beforeAndAfterParticleBounds.m_beforeSmallestZ)
            {
                beforeAndAfterParticleBounds.m_beforeSmallestZ = particlesBefore[particleIndex].GetZ();
            }
            if (particlesBefore[particleIndex].GetY() > beforeAndAfterParticleBounds.m_beforeLargestY)
            {
                beforeAndAfterParticleBounds.m_beforeLargestY = particlesBefore[particleIndex].GetY();
            }

            if (particlesAfter[particleIndex].GetZ() < beforeAndAfterParticleBounds.m_afterSmallestZ)
            {
                beforeAndAfterParticleBounds.m_afterSmallestZ = particlesAfter[particleIndex].GetZ();
            }
            if (particlesAfter[particleIndex].GetY() > beforeAndAfterParticleBounds.m_afterLargestY)
            {
                beforeAndAfterParticleBounds.m_afterLargestY = particlesAfter[particleIndex].GetY();
            }
        }

        return beforeAndAfterParticleBounds;
    }

    void NvClothTestFixture::TickClothSimulation(const AZ::u32 tickBefore,
        const AZ::u32 tickAfter,
        AZStd::vector<NvCloth::SimParticleFormat>& particlesBefore)
    {
        const float timeOneFrameSeconds = 0.016f; //approx 60 fps

        for (AZ::u32 tickCount = 0; tickCount < tickAfter; ++tickCount)
        {
            AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick,
                timeOneFrameSeconds,
                AZ::ScriptTimePoint(AZStd::chrono::system_clock::now()));

            if (tickCount == tickBefore)
            {
                particlesBefore = m_cloth->GetParticles();
            }
        }
    }

    //! Tests that basic cloth simulation works.
    TEST_F(NvClothTestFixture, Cloth_NoCollision_FallWithGravity)
    {
        const AZ::u32 tickBefore = 150;
        const AZ::u32 tickAfter = 300;
        AZStd::vector<NvCloth::SimParticleFormat> particlesBefore;
        TickClothSimulation(tickBefore, tickAfter, particlesBefore);

        ParticleBounds particleBounds = GetBeforeAndAfterParticleBounds(particlesBefore,
            m_cloth->GetParticles());

        // Cloth was extended horizontally in the y-direction earlier.
        // If cloth fell with gravity, its y-extent should be smaller later,
        // and its z-extent should go lower to a smaller Z value later.
        ASSERT_TRUE((particleBounds.m_afterLargestY < particleBounds.m_beforeLargestY) &&
            (particleBounds.m_afterSmallestZ < particleBounds.m_beforeSmallestZ));
    }

    //! Tests that collision works and pre/post simulation events work.
    TEST_F(NvClothTestFixture, Cloth_Collision_CollidedWithPrePostSimEvents)
    {
        m_cloth->ConnectPreSimulationEventHandler(m_preSimulationEventHandler); // The pre-simulation callback moves the sphere collider towards the cloth every tick.
        m_cloth->ConnectPostSimulationEventHandler(m_postSimulationEventHandler);

        const AZ::u32 tickBefore = 150;
        const AZ::u32 tickAfter = 320;
        AZStd::vector<NvCloth::SimParticleFormat> particlesBefore;
        TickClothSimulation(tickBefore, tickAfter, particlesBefore);

        ParticleBounds particleBounds = GetBeforeAndAfterParticleBounds(particlesBefore,
            m_cloth->GetParticles());

        // Cloth starts extended horizontally (along Y-axis). Simulation makes it swing down with gravity (as tested with the other unit test).
        // Then the sphere collider collides with the cloth and pushes it back up. So it is again extended in the Y-direction and
        // at about the same vertical height (Z-coord) as before.
        const float threshold = 0.25f;
        EXPECT_TRUE(AZ::IsClose(particleBounds.m_beforeSmallestZ , -0.97f, threshold));
        EXPECT_TRUE(AZ::IsClose(particleBounds.m_beforeLargestY, 0.76f, threshold));
        EXPECT_TRUE(AZ::IsClose(particleBounds.m_afterSmallestZ, -1.1f, threshold));
        EXPECT_TRUE(AZ::IsClose(particleBounds.m_afterLargestY, 0.72f, threshold));
        ASSERT_TRUE((fabsf(particleBounds.m_afterLargestY - particleBounds.m_beforeLargestY) < threshold) &&
            (fabsf(particleBounds.m_afterSmallestZ - particleBounds.m_beforeSmallestZ) < threshold));

        // Check that post simulation event was invoked.
        ASSERT_TRUE(m_postSimulationEventInvoked);

        m_preSimulationEventHandler.Disconnect();
        m_postSimulationEventHandler.Disconnect();
    }

    AZ_UNIT_TEST_HOOK(new NvClothTestEnvironment);
}
