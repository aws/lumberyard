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

#include <AzCore/Math/Transform.h>
#include <AzTest/GemTestEnvironment.h>
#include <Mocks/ISystemMock.h>
#include <NvCloth/ICloth.h>

namespace NvCloth
{
    class FabricCooker;
    class ICloth;

    //! Mock CrySystem. The cloth system component listens to OnCrySystemInitialized for initialization.
    class CrySystemMock : public SystemMock
    {
    public:
        //! Sends mock OnCrySystemInitialized event to trigger the cloth system component initialization.
        void BroadcastCrySystemInitialized();
    };

    //! Sets up gem test environment, required components, and shared objects used by cloth (e.g. FabricCooker) for all test cases.
    class NvClothTestEnvironment
        : public AZ::Test::GemTestEnvironment
    {
    protected:
        void SetupEnvironment() override;
        void TeardownEnvironment() override;
        void TeardownAllocatorAndTraceBus() override {} //! override as empty to implement custom order for allocator destruction in TeardownEnvironment
        void AddGemsAndComponents() override;
        void PostCreateApplication() override;

    private:
        AZStd::unique_ptr<FabricCooker> m_fabricCooker;
    };

    //! Sets up cloth library, cloth and collider instances for each test case.
    class NvClothTestFixture
        : public ::testing::Test
    {
    protected:
        static void SetUpTestCase();
        static void TearDownTestCase();

        void SetUp() override;
        void TearDown() override;

        // ICloth notifications
        void OnPreSimulation(NvCloth::ClothId clothId, float deltaTime);
        void OnPostSimulation(NvCloth::ClothId clothId,
            float deltaTime,
            const AZStd::vector<NvCloth::SimParticleFormat>& updatedParticles);

        //! Sends tick events to make cloth simulation happen.
        //! Returns the position of cloth particles at tickBefore, continues ticking till tickAfter.
        void TickClothSimulation(const AZ::u32 tickBefore,
            const AZ::u32 tickAfter,
            AZStd::vector<NvCloth::SimParticleFormat>& particlesBefore);

        NvCloth::ICloth* m_cloth = nullptr;
        NvCloth::ICloth::PreSimulationEvent::Handler m_preSimulationEventHandler;
        NvCloth::ICloth::PostSimulationEvent::Handler m_postSimulationEventHandler;
        bool m_postSimulationEventInvoked = false;

    private:
        bool CreateCloth();
        void DestroyCloth();
        
        AZ::Transform m_clothTransform = AZ::Transform::CreateIdentity();
        AZStd::vector<AZ::Vector4> m_sphereColliders;
    };

    //! Class to provide triangle input data for tests
    struct TriangleInputPlane
    {
        //! Creates triangle data for a plane in XY axis with any dimensions and segments.
        static TriangleInputPlane CreatePlane(float width,
            float height,
            AZ::u32 segmentsX,
            AZ::u32 segmentsY);

        AZStd::vector<NvCloth::SimParticleFormat> m_vertices;
        AZStd::vector<NvCloth::SimIndexType> m_indices;
        AZStd::vector<NvCloth::SimUVType> m_uvs;
    };
}
