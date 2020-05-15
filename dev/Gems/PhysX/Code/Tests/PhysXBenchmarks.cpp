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

#include <vector>

#include <AzTest/AzTest.h>
#include <Physics/PhysicsTests.h>
#include <Tests/PhysXTestCommon.h>

#include <AzCore/Math/Random.h>
#include <AzCore/std/parallel/thread.h>

#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include "PhysXGenericTest.h"

#ifdef HAVE_BENCHMARK
#include <benchmark/benchmark.h>

namespace PhysX::Benchmarks
{
    namespace Constants
    {
        const AZ::Vector3 BoxDimensions = AZ::Vector3::CreateOne();
        static const float SphereShapeRadius = 2.0f;
        static const AZ::u32 MinRadius = 2u;
        static const int Seed = 100;

        static const std::vector<std::vector<std::pair<int64_t, int64_t>>> BenchmarkConfigs =
        {
            // {{a, b}, {c, d}} means generate all pairs of parameters {x, y} for the benchmark
            // such as x = a * 2 ^ k && x <= b and y = c * 2 ^ l && y <= d
            // We have several configurations here because number of box entities might become
            // larger than possible number of box locations if the maxRadius is small
            {{4, 16}, {8, 512}},
            {{32, 256}, {16, 512}},
            {{512, 1024}, {32, 512}},
            {{2048, 4096}, {64, 512}}
        };
    }

    class PhysXBenchmarkFixture
        : public benchmark::Fixture
        , public Physics::GenericPhysicsFixture
        , public Environment
    {
    public:

        //! Spawns box entities in unique locations in 1/8 of sphere with all non-negative dimensions between radii[2, max radius].
        //! Accepts 2 parameters from \state.
        //!
        //! \state.range(0) - number of box entities to spawn
        //! \state.range(1) - max radius
        void SetUp(const ::benchmark::State& state) override
        {
            Environment::SetupInternal();
            Physics::GenericPhysicsFixture::SetUpInternal();

            m_random = AZ::SimpleLcgRandom(Constants::Seed);
            m_numBoxes = aznumeric_cast<AZ::u32>(state.range(0));
            const auto minRadius = Constants::MinRadius;
            const auto maxRadius = aznumeric_cast<AZ::u32>(state.range(1));

            std::vector<AZ::Vector3> possibleBoxes;
            AZ::u32 possibleBoxesCount = 0;

            // Generate all possible boxes in radii [minRadius, maxRadius] where all the dimensions are positive.
            for (auto x = 0u; x <= maxRadius; ++x)
            {
                for (auto y = 0u; y * y <= maxRadius * maxRadius - x * x; ++y)
                {
                    for (auto z = aznumeric_cast<AZ::u32>(sqrt(std::max(x * x - y * y - minRadius * minRadius, 0u))); z * z <= maxRadius * maxRadius - x * x - y * y; ++z)
                    {
                        const auto sqDist = x * x + y * y + z * z;
                        if (minRadius * minRadius <= sqDist && sqDist <= maxRadius * maxRadius)
                        {
                            auto boxPosition = AZ::Vector3(aznumeric_cast<float>(x), aznumeric_cast<float>(y), aznumeric_cast<float>(z));
                            possibleBoxes.push_back(boxPosition);
                            possibleBoxesCount++;
                        }
                    }
                }
            }
            AZ_Assert(m_numBoxes <= possibleBoxesCount, "Number of supplied boxes should be less than or equal to possible positions for boxes.");

            // Shuffle to pick first m_numBoxes
            for (AZ::u32 i = 1; i < possibleBoxesCount; ++i)
            {
                std::swap(possibleBoxes[i], possibleBoxes[m_random.GetRandom() % (i + 1)]);
            }

            m_boxes = std::vector<AZ::Vector3>(possibleBoxes.begin(), possibleBoxes.begin() + m_numBoxes);
            for (auto box : m_boxes)
            {
                auto newEntity = TestUtils::CreateBoxEntity(box, Constants::BoxDimensions, false);
                Physics::RigidBodyRequestBus::Event(newEntity->GetId(), &Physics::RigidBodyRequestBus::Events::SetGravityEnabled, false);
                m_entities.push_back(newEntity);
            }
        }

        void TearDown(const ::benchmark::State& state) override
        {
            m_boxes.clear();
            m_entities.clear();
            Physics::GenericPhysicsFixture::TearDownInternal();
            Environment::TeardownInternal();
        }

    protected:
        std::vector<EntityPtr> m_entities;
        std::vector<AZ::Vector3> m_boxes;
        AZ::u32 m_numBoxes = 0;
        AZ::SimpleLcgRandom m_random;
    };

    //! Used to measure how much overhead fixture adds to benchmark runs.
    class BenchmarkablePhysXBenchmarkFixture
        : public Physics::GenericPhysicsFixture
        , public Environment
    {
    public:
        void SetUp()
        {
            Environment::SetupInternal();
            Physics::GenericPhysicsFixture::SetUpInternal();
        }

        void TearDown()
        {
            Physics::GenericPhysicsFixture::TearDownInternal();
            Environment::TeardownInternal();
        }
    };


    BENCHMARK_DEFINE_F(PhysXBenchmarkFixture, BM_RaycastRandomBoxes)(benchmark::State& state)
    {
        Physics::RayCastRequest request;
        request.m_start = AZ::Vector3::CreateZero();
        request.m_distance = 2000.0f;
        Physics::RayCastHit result;

        auto next = 0;
        for (auto _ : state)
        {
            request.m_direction = m_boxes[next].GetNormalizedExact();

            Physics::WorldRequestBus::BroadcastResult(result, &Physics::WorldRequests::RayCast, request);
            benchmark::DoNotOptimize(result);
            next = (next + 1) % m_numBoxes;
        }
    }

    BENCHMARK_DEFINE_F(PhysXBenchmarkFixture, BM_ShapecastRandomBoxes)(benchmark::State& state)
    {
        Physics::SphereShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_radius = Constants::SphereShapeRadius;
        Physics::ShapeCastRequest request;
        request.m_start = AZ::Transform::CreateIdentity();
        request.m_distance = 2000.0f;
        request.m_shapeConfiguration = &shapeConfiguration;
        Physics::RayCastHit result;

        auto next = 0;
        for (auto _ : state)
        {
            request.m_direction = m_boxes[next].GetNormalizedExact();

            Physics::WorldRequestBus::BroadcastResult(result, &Physics::WorldRequests::ShapeCast, request);
            benchmark::DoNotOptimize(result);
            next = (next + 1) % m_numBoxes;
        }
    }

    BENCHMARK_DEFINE_F(PhysXBenchmarkFixture, BM_OverlapRandomBoxes)(benchmark::State& state)
    {
        Physics::SphereShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_radius = Constants::SphereShapeRadius;
        Physics::OverlapRequest request;
        request.m_shapeConfiguration = &shapeConfiguration;
        AZStd::vector<Physics::OverlapHit> result;

        auto next = 0;
        for (auto _ : state)
        {
            request.m_pose = AZ::Transform::CreateTranslation(m_boxes[next]);

            Physics::WorldRequestBus::BroadcastResult(result, &Physics::WorldRequests::Overlap, request);
            benchmark::DoNotOptimize(result);
            next = (next + 1) % m_numBoxes;
        }
    }

    BENCHMARK_REGISTER_F(PhysXBenchmarkFixture, BM_RaycastRandomBoxes)
        ->RangeMultiplier(2)
        ->Ranges(Constants::BenchmarkConfigs[0])
        ->Ranges(Constants::BenchmarkConfigs[1])
        ->Ranges(Constants::BenchmarkConfigs[2])
        ->Ranges(Constants::BenchmarkConfigs[3])
        ->ReportAggregatesOnly(true);
    BENCHMARK_REGISTER_F(PhysXBenchmarkFixture, BM_ShapecastRandomBoxes)
        ->RangeMultiplier(2)
        ->Ranges(Constants::BenchmarkConfigs[0])
        ->Ranges(Constants::BenchmarkConfigs[1])
        ->Ranges(Constants::BenchmarkConfigs[2])
        ->Ranges(Constants::BenchmarkConfigs[3])
        ->ReportAggregatesOnly(true);

    void BM_PhysXBenchmarkFixture(benchmark::State& state)
    {
        for(auto _ : state)
        {
            auto fixture = std::make_unique<BenchmarkablePhysXBenchmarkFixture>();
            fixture->SetUp();
            fixture->TearDown();
            benchmark::DoNotOptimize(*fixture);
        }
    }

    BENCHMARK(BM_PhysXBenchmarkFixture)->Unit(benchmark::kMillisecond);
}
#endif
