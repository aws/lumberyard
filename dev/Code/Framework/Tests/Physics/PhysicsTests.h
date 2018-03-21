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

#ifdef AZ_TESTS_ENABLED

#include <AzTest/AzTest.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/RigidBody.h>

namespace Physics
{
    class PhysicsTestEnvironment
        : public AZ::Test::ITestEnvironment
    {
    protected:
        void SetupEnvironment() override;
        void TeardownEnvironment() override;
    };

    /**
     * Class to contain tests which any implementation of the AzFramework::Physics API should pass
     * Each gem which implements the common physics API can run the generic API tests by:
     * - including this header file and the corresponding .inl
     * - deriving from PhysicsTestEnvironment and extending the environment functions to set up the gem system component etc.
     * - implementing the helper functions required for the tests using gem specific components etc.
     * - adding a AZ_UNIT_TEST_HOOK with the derived environment class
     */
    class GenericPhysicsInterfaceTest
        : public ::testing::Test
    {
    public:
        // Helper functions for setting up test worlds using API only
        // These can be implemented here as they should not require any gem specific functions
        Physics::Ptr<Physics::World> CreateTestWorld();
        void DestroyTestWorld();

        // Helper functions for setting up entities used in tests
        // These need to be implemented in the gem as they may require gem specific components etc.
        AZ::Entity* AddSphereEntity(const AZ::Vector3& position, float radius, MotionType motionType = MotionType::Dynamic);
    };
} // namespace Physics
#endif // AZ_TESTS_ENABLED
