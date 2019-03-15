namespace Physics
{
    TEST_F(PhysicsComponentBusTest, SetLinearDamping_DynamicSphere_MoreDampedBodyFallsSlower)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetLinearDamping, 0.1f);
        Physics::RigidBodyRequestBus::Event(sphereB->GetId(),
            &Physics::RigidBodyRequests::SetLinearDamping, 0.2f);

        UpdateDefaultWorld(1.0f / 60.0f, 60);

        float dampingA, dampingB;
        Physics::RigidBodyRequestBus::EventResult(dampingA, sphereA->GetId(),
            &Physics::RigidBodyRequests::GetLinearDamping);
        Physics::RigidBodyRequestBus::EventResult(dampingB, sphereB->GetId(),
            &Physics::RigidBodyRequests::GetLinearDamping);
        EXPECT_NEAR(dampingA, 0.1f, 1e-3f);
        EXPECT_NEAR(dampingB, 0.2f, 1e-3f);

        float zA = GetPositionElement(sphereA, 2);
        float zB = GetPositionElement(sphereB, 2);
        EXPECT_GT(zB, zA);

        AZ::Vector3 vA = AZ::Vector3::CreateZero();
        AZ::Vector3 vB = AZ::Vector3::CreateZero();
        Physics::RigidBodyRequestBus::EventResult(vA, sphereA->GetId(),
            &Physics::RigidBodyRequests::GetLinearVelocity);
        Physics::RigidBodyRequestBus::EventResult(vB, sphereB->GetId(),
            &Physics::RigidBodyRequests::GetLinearVelocity);
        EXPECT_GT(static_cast<float>(vA.GetLength()), static_cast<float>(vB.GetLength()));

        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, SetLinearDampingNegative_DynamicSphere_NegativeValueRejected)
    {
        ErrorHandler errorHandler("Negative linear damping value");

        auto sphere = AddSphereEntity(AZ::Vector3::CreateZero(), 0.5f);

        float damping = 0.0f, initialDamping = 0.0f;
        Physics::RigidBodyRequestBus::EventResult(initialDamping, sphere->GetId(),
            &Physics::RigidBodyRequests::GetLinearDamping);

        // a negative damping value should be rejected and the damping should remain at its previous value
        Physics::RigidBodyRequestBus::Event(sphere->GetId(),
            &Physics::RigidBodyRequests::SetLinearDamping, -0.1f);
        Physics::RigidBodyRequestBus::EventResult(damping, sphere->GetId(),
            &Physics::RigidBodyRequests::GetLinearDamping);

        EXPECT_NEAR(damping, initialDamping, 1e-3f);
        EXPECT_TRUE(errorHandler.GetErrorCount() > 0);

        delete sphere;
    }

    TEST_F(PhysicsComponentBusTest, SetAngularDamping_DynamicSphere_MoreDampedBodyRotatesSlower)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 1.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 1.0f), 0.5f);
        auto floor = AddStaticBoxEntity(AZ::Vector3::CreateZero(), AZ::Vector3(100.0f, 100.0f, 1.0f));

        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetAngularDamping, 0.1f);
        Physics::RigidBodyRequestBus::Event(sphereB->GetId(),
            &Physics::RigidBodyRequests::SetAngularDamping, 0.2f);

        float dampingA, dampingB;
        Physics::RigidBodyRequestBus::EventResult(dampingA, sphereA->GetId(),
            &Physics::RigidBodyRequests::GetAngularDamping);
        Physics::RigidBodyRequestBus::EventResult(dampingB, sphereB->GetId(),
            &Physics::RigidBodyRequests::GetAngularDamping);
        EXPECT_NEAR(dampingA, 0.1f, 1e-3f);
        EXPECT_NEAR(dampingB, 0.2f, 1e-3f);

        AZ::Vector3 impulse(10.0f, 0.0f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::ApplyLinearImpulse, impulse);
        Physics::RigidBodyRequestBus::Event(sphereB->GetId(),
            &Physics::RigidBodyRequests::ApplyLinearImpulse, impulse);

        UpdateDefaultWorld(1.0f / 60.0f, 10);
        auto angularVelocityA = AZ::Vector3::CreateZero();
        auto angularVelocityB = AZ::Vector3::CreateZero();

        for (int timestep = 0; timestep < 10; timestep++)
        {
            Physics::RigidBodyRequestBus::EventResult(angularVelocityA, sphereA->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            Physics::RigidBodyRequestBus::EventResult(angularVelocityB, sphereB->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            EXPECT_GT(static_cast<float>(angularVelocityA.GetLength()), static_cast<float>(angularVelocityB.GetLength()));
            UpdateDefaultWorld(1.0f / 60.0f, 1);
        }

        delete floor;
        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, SetAngularDampingNegative_DynamicSphere_NegativeValueRejected)
    {
        ErrorHandler errorHandler("Negative angular damping value");

        auto sphere = AddSphereEntity(AZ::Vector3::CreateZero(), 0.5f);

        float damping = 0.0f, initialDamping = 0.0f;
        Physics::RigidBodyRequestBus::EventResult(initialDamping, sphere->GetId(),
            &Physics::RigidBodyRequests::GetAngularDamping);

        // a negative damping value should be rejected and the damping should remain at its previous value
        Physics::RigidBodyRequestBus::Event(sphere->GetId(),
            &Physics::RigidBodyRequests::SetAngularDamping, -0.1f);
        Physics::RigidBodyRequestBus::EventResult(damping, sphere->GetId(),
            &Physics::RigidBodyRequests::GetAngularDamping);

        EXPECT_NEAR(damping, initialDamping, 1e-3f);
        EXPECT_TRUE(errorHandler.GetErrorCount() > 0);

        delete sphere;
    }

    TEST_F(PhysicsComponentBusTest, AddImpulse_DynamicSphere_AffectsTrajectory)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 impulse(10.0f, 0.0f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::ApplyLinearImpulse, impulse);

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = GetPositionElement(sphereA, 0);
            float xPreviousB = GetPositionElement(sphereB, 0);
            UpdateDefaultWorld(1.0f / 60.0f, 10);
            EXPECT_GT(GetPositionElement(sphereA, 0), xPreviousA);
            EXPECT_NEAR(GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);
        }

        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, SetLinearVelocity_DynamicSphere_AffectsTrajectory)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 velocity(10.0f, 0.0f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetLinearVelocity, velocity);

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = GetPositionElement(sphereA, 0);
            float xPreviousB = GetPositionElement(sphereB, 0);
            UpdateDefaultWorld(1.0f / 60.0f, 10);
            EXPECT_GT(GetPositionElement(sphereA, 0), xPreviousA);
            EXPECT_NEAR(GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);
        }

        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, AddImpulseAtWorldPoint_DynamicSphere_AffectsTrajectoryAndRotation)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 impulse(10.0f, 0.0f, 0.0f);
        AZ::Vector3 worldPoint(0.0f, -5.0f, 0.25f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::ApplyLinearImpulseAtWorldPoint, impulse, worldPoint);

        AZ::Vector3 angularVelocityA = AZ::Vector3::CreateZero();
        AZ::Vector3 angularVelocityB = AZ::Vector3::CreateZero();

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = GetPositionElement(sphereA, 0);
            float xPreviousB = GetPositionElement(sphereB, 0);
            UpdateDefaultWorld(1.0f / 60.0f, 10);
            EXPECT_GT(GetPositionElement(sphereA, 0), xPreviousA);
            EXPECT_NEAR(GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);

            Physics::RigidBodyRequestBus::EventResult(angularVelocityA, sphereA->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            Physics::RigidBodyRequestBus::EventResult(angularVelocityB, sphereB->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            EXPECT_FALSE(angularVelocityA.IsClose(AZ::Vector3::CreateZero()));
            EXPECT_NEAR(static_cast<float>(angularVelocityA.GetX()), 0.0f, 1e-3f);
            EXPECT_NEAR(static_cast<float>(angularVelocityA.GetZ()), 0.0f, 1e-3f);
            EXPECT_TRUE(angularVelocityB.IsClose(AZ::Vector3::CreateZero()));
        }

        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, AddAngularImpulse_DynamicSphere_AffectsRotation)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 angularImpulse(0.0f, 10.0f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::ApplyAngularImpulse, angularImpulse);

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = GetPositionElement(sphereA, 0);
            float xPreviousB = GetPositionElement(sphereB, 0);
            UpdateDefaultWorld(1.0f / 60.0f, 10);
            EXPECT_NEAR(GetPositionElement(sphereA, 0), xPreviousA, 1e-3f);
            EXPECT_NEAR(GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);

            AZ::Vector3 angularVelocityA = AZ::Vector3::CreateZero();
            AZ::Vector3 angularVelocityB = AZ::Vector3::CreateZero();
            Physics::RigidBodyRequestBus::EventResult(angularVelocityA, sphereA->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            Physics::RigidBodyRequestBus::EventResult(angularVelocityB, sphereB->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            EXPECT_FALSE(angularVelocityA.IsClose(AZ::Vector3::CreateZero()));
            EXPECT_NEAR(static_cast<float>(angularVelocityA.GetX()), 0.0f, 1e-3f);
            EXPECT_NEAR(static_cast<float>(angularVelocityA.GetZ()), 0.0f, 1e-3f);
            EXPECT_TRUE(angularVelocityB.IsClose(AZ::Vector3::CreateZero()));
        }

        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, SetAngularVelocity_DynamicSphere_AffectsRotation)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 0.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 0.0f), 0.5f);

        AZ::Vector3 angularVelocity(0.0f, 10.0f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetAngularVelocity, angularVelocity);

        for (int i = 1; i < 10; i++)
        {
            float xPreviousA = GetPositionElement(sphereA, 0);
            float xPreviousB = GetPositionElement(sphereB, 0);
            UpdateDefaultWorld(1.0f / 60.0f, 10);
            EXPECT_NEAR(GetPositionElement(sphereA, 0), xPreviousA, 1e-3f);
            EXPECT_NEAR(GetPositionElement(sphereB, 0), xPreviousB, 1e-3f);

            AZ::Vector3 angularVelocityA = AZ::Vector3::CreateZero();
            AZ::Vector3 angularVelocityB = AZ::Vector3::CreateZero();
            Physics::RigidBodyRequestBus::EventResult(angularVelocityA, sphereA->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            Physics::RigidBodyRequestBus::EventResult(angularVelocityB, sphereB->GetId(),
                &Physics::RigidBodyRequests::GetAngularVelocity);
            EXPECT_FALSE(angularVelocityA.IsClose(AZ::Vector3::CreateZero()));
            EXPECT_NEAR(static_cast<float>(angularVelocityA.GetX()), 0.0f, 1e-3f);
            EXPECT_NEAR(static_cast<float>(angularVelocityA.GetZ()), 0.0f, 1e-3f);
            EXPECT_TRUE(angularVelocityB.IsClose(AZ::Vector3::CreateZero()));
        }

        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, GetLinearVelocity_FallingSphere_VelocityIncreasesOverTime)
    {
        auto sphere = AddSphereEntity(AZ::Vector3(0.0f, 0.0f, 0.0f), 0.5f);
        Physics::RigidBodyRequestBus::Event(sphere->GetId(),
            &Physics::RigidBodyRequests::SetLinearDamping, 0.0f);

        float previousSpeed = 0.0f;

        for (int timestep = 0; timestep < 60; timestep++)
        {
            UpdateDefaultWorld(1.0f / 60.0f, 1);
            AZ::Vector3 velocity;
            Physics::RigidBodyRequestBus::EventResult(velocity, sphere->GetId(),
                &Physics::RigidBodyRequests::GetLinearVelocity);
            float speed = velocity.GetLength();
            EXPECT_GT(speed, previousSpeed);
            previousSpeed = speed;
        }

        delete sphere;
    }

    TEST_F(PhysicsComponentBusTest, SetSleepThreshold_RollingSpheres_LowerThresholdSphereTravelsFurther)
    {
        auto sphereA = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 1.0f), 0.5f);
        auto sphereB = AddSphereEntity(AZ::Vector3(0.0f, 5.0f, 1.0f), 0.5f);
        auto floor = AddStaticBoxEntity(AZ::Vector3::CreateZero(), AZ::Vector3(100.0f, 100.0f, 1.0f));

        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetAngularDamping, 0.75f);
        Physics::RigidBodyRequestBus::Event(sphereB->GetId(),
            &Physics::RigidBodyRequests::SetAngularDamping, 0.75f);

        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::SetSleepThreshold, 1.0f);
        Physics::RigidBodyRequestBus::Event(sphereB->GetId(),
            &Physics::RigidBodyRequests::SetSleepThreshold, 0.5f);

        float sleepThresholdA, sleepThresholdB;
        Physics::RigidBodyRequestBus::EventResult(sleepThresholdA, sphereA->GetId(),
            &Physics::RigidBodyRequests::GetSleepThreshold);
        Physics::RigidBodyRequestBus::EventResult(sleepThresholdB, sphereB->GetId(),
            &Physics::RigidBodyRequests::GetSleepThreshold);

        EXPECT_NEAR(sleepThresholdA, 1.0f, 1e-3f);
        EXPECT_NEAR(sleepThresholdB, 0.5f, 1e-3f);

        AZ::Vector3 impulse(0.0f, 0.1f, 0.0f);
        Physics::RigidBodyRequestBus::Event(sphereA->GetId(),
            &Physics::RigidBodyRequests::ApplyAngularImpulse, impulse);
        Physics::RigidBodyRequestBus::Event(sphereB->GetId(),
            &Physics::RigidBodyRequests::ApplyAngularImpulse, impulse);

        UpdateDefaultWorld(1.0f / 60.0f, 300);

        EXPECT_GT(GetPositionElement(sphereB, 0), GetPositionElement(sphereA, 0));

        delete floor;
        delete sphereA;
        delete sphereB;
    }

    TEST_F(PhysicsComponentBusTest, SetSleepThresholdNegative_DynamicSphere_NegativeValueRejected)
    {
        ErrorHandler errorHandler("Negative sleep threshold value");

        auto sphere = AddSphereEntity(AZ::Vector3(0.0f, -5.0f, 1.0f), 0.5f);

        float threshold = 0.0f, initialThreshold = 0.0f;
        Physics::RigidBodyRequestBus::EventResult(initialThreshold, sphere->GetId(),
            &Physics::RigidBodyRequests::GetSleepThreshold);
        Physics::RigidBodyRequestBus::Event(sphere->GetId(),
            &Physics::RigidBodyRequests::SetSleepThreshold, -0.5f);
        Physics::RigidBodyRequestBus::EventResult(threshold, sphere->GetId(),
            &Physics::RigidBodyRequests::GetSleepThreshold);

        EXPECT_NEAR(threshold, initialThreshold, 1e-3f);
        EXPECT_TRUE(errorHandler.GetErrorCount() > 0);

        delete sphere;
    }

    TEST_F(PhysicsComponentBusTest, SetMass_Seesaw_TipsDownAtHeavierEnd)
    {
        auto floor = AddStaticBoxEntity(AZ::Vector3::CreateZero(), AZ::Vector3(100.0f, 100.0f, 1.0f));
        auto pivot = AddStaticBoxEntity(AZ::Vector3(0.0f, 0.0f, 0.7f), AZ::Vector3(0.4f, 1.0f, 0.4f));
        auto seesaw = AddBoxEntity(AZ::Vector3(0.0f, 0.0f, 0.95f), AZ::Vector3(20.0f, 1.0f, 0.1f));
        auto boxA = AddBoxEntity(AZ::Vector3(-9.0f, 0.0f, 1.5f), AZ::Vector3::CreateOne());
        auto boxB = AddBoxEntity(AZ::Vector3(9.0f, 0.0f, 1.5f), AZ::Vector3::CreateOne());

        Physics::RigidBodyRequestBus::Event(boxA->GetId(), &Physics::RigidBodyRequests::SetMass, 5.0f);
        float mass = 0.0f;
        Physics::RigidBodyRequestBus::EventResult(mass, boxA->GetId(),
            &Physics::RigidBodyRequests::GetMass);
        EXPECT_NEAR(mass, 5.0f, 1e-3f);

        UpdateDefaultWorld(1.0f / 60.0f, 30);
        EXPECT_GT(1.5f, GetPositionElement(boxA, 2));
        EXPECT_LT(1.5f, GetPositionElement(boxB, 2));

        Physics::RigidBodyRequestBus::Event(boxB->GetId(), &Physics::RigidBodyRequests::SetMass, 20.0f);
        Physics::RigidBodyRequestBus::EventResult(mass, boxB->GetId(),
            &Physics::RigidBodyRequests::GetMass);
        EXPECT_NEAR(mass, 20.0f, 1e-3f);

        UpdateDefaultWorld(1.0f / 60.0f, 60);
        EXPECT_LT(1.5f, GetPositionElement(boxA, 2));
        EXPECT_GT(1.5f, GetPositionElement(boxB, 2));

        delete floor;
        delete pivot;
        delete seesaw;
        delete boxA;
        delete boxB;
    }

    TEST_F(PhysicsComponentBusTest, GetAabb_Sphere_ValidExtents)
    {
        AZ::Vector3 spherePosition(2.0f, -3.0f, 1.0f);
        auto sphere = AddSphereEntity(spherePosition, 0.5f);

        AZ::Aabb sphereAabb;
        Physics::RigidBodyRequestBus::EventResult(sphereAabb, sphere->GetId(),
            &Physics::RigidBodyRequests::GetAabb);

        EXPECT_TRUE(sphereAabb.GetMin().IsClose(spherePosition - 0.5f * AZ::Vector3::CreateOne()));
        EXPECT_TRUE(sphereAabb.GetMax().IsClose(spherePosition + 0.5f * AZ::Vector3::CreateOne()));

        // rotate the sphere and check the bounding box is still correct
        AZ::Quaternion quat = AZ::Quaternion::CreateRotationZ(0.25f * AZ::Constants::Pi);
        AZ::TransformBus::Event(sphere->GetId(), &AZ::TransformInterface::SetWorldTM,
            AZ::Transform::CreateFromQuaternionAndTranslation(quat, spherePosition));
        sphere->Deactivate();
        sphere->Activate();

        Physics::RigidBodyRequestBus::EventResult(sphereAabb, sphere->GetId(),
            &Physics::RigidBodyRequests::GetAabb);

        EXPECT_TRUE(sphereAabb.GetMin().IsClose(spherePosition - 0.5f * AZ::Vector3::CreateOne()));
        EXPECT_TRUE(sphereAabb.GetMax().IsClose(spherePosition + 0.5f * AZ::Vector3::CreateOne()));

        delete sphere;
    }

    TEST_F(PhysicsComponentBusTest, GetAabb_Box_ValidExtents)
    {
        AZ::Vector3 boxPosition(2.0f, -3.0f, 1.0f);
        AZ::Vector3 boxDimensions(3.0f, 4.0f, 5.0f);
        auto box = AddBoxEntity(boxPosition, boxDimensions);

        AZ::Aabb boxAabb;
        Physics::RigidBodyRequestBus::EventResult(boxAabb, box->GetId(),
            &Physics::RigidBodyRequests::GetAabb);

        EXPECT_TRUE(boxAabb.GetMin().IsClose(boxPosition - 0.5f * boxDimensions));
        EXPECT_TRUE(boxAabb.GetMax().IsClose(boxPosition + 0.5f * boxDimensions));

        // rotate the box and check the bounding box is still correct
        AZ::Quaternion quat = AZ::Quaternion::CreateRotationZ(0.25f * AZ::Constants::Pi);
        AZ::TransformBus::Event(box->GetId(), &AZ::TransformInterface::SetWorldTM,
            AZ::Transform::CreateFromQuaternionAndTranslation(quat, boxPosition));
        box->Deactivate();
        box->Activate();

        Physics::RigidBodyRequestBus::EventResult(boxAabb, box->GetId(),
            &Physics::RigidBodyRequests::GetAabb);

        AZ::Vector3 expectedRotatedDimensions(3.5f * sqrtf(2.0f), 3.5f * sqrtf(2.0f), 5.0f);
        EXPECT_TRUE(boxAabb.GetMin().IsClose(boxPosition - 0.5f * expectedRotatedDimensions));
        EXPECT_TRUE(boxAabb.GetMax().IsClose(boxPosition + 0.5f * expectedRotatedDimensions));

        delete box;
    }

    TEST_F(PhysicsComponentBusTest, GetAabb_Capsule_ValidExtents)
    {
        AZ::Vector3 capsulePosition(1.0f, -3.0f, 5.0f);
        float capsuleHeight = 2.0f;
        float capsuleRadius = 0.3f;
        auto capsule = AddCapsuleEntity(capsulePosition, capsuleHeight, capsuleRadius);

        AZ::Aabb capsuleAabb;
        Physics::RigidBodyRequestBus::EventResult(capsuleAabb, capsule->GetId(),
            &Physics::RigidBodyRequests::GetAabb);

        AZ::Vector3 expectedCapsuleHalfExtents(capsuleRadius, capsuleRadius, 0.5f * capsuleHeight);

        EXPECT_TRUE(capsuleAabb.GetMin().IsClose(capsulePosition - expectedCapsuleHalfExtents));
        EXPECT_TRUE(capsuleAabb.GetMax().IsClose(capsulePosition + expectedCapsuleHalfExtents));

        // rotate the capsule and check the bounding box is still correct
        AZ::Quaternion quat = AZ::Quaternion::CreateRotationY(0.25f * AZ::Constants::Pi);
        AZ::TransformBus::Event(capsule->GetId(), &AZ::TransformInterface::SetWorldTM,
            AZ::Transform::CreateFromQuaternionAndTranslation(quat, capsulePosition));
        capsule->Deactivate();
        capsule->Activate();

        Physics::RigidBodyRequestBus::EventResult(capsuleAabb, capsule->GetId(),
            &Physics::RigidBodyRequests::GetAabb);

        float rotatedHalfHeight = 0.25f * sqrtf(2.0f) * capsuleHeight + (1.0f - 0.5f * sqrt(2.0f)) * capsuleRadius;
        expectedCapsuleHalfExtents = AZ::Vector3(rotatedHalfHeight, capsuleRadius, rotatedHalfHeight);
        EXPECT_TRUE(capsuleAabb.GetMin().IsClose(capsulePosition - expectedCapsuleHalfExtents));
        EXPECT_TRUE(capsuleAabb.GetMax().IsClose(capsulePosition + expectedCapsuleHalfExtents));

        delete capsule;
    }

    TEST_F(PhysicsComponentBusTest, ForceAwakeForceAsleep_DynamicSphere_SleepStateCorrect)
    {
        auto floor = AddStaticBoxEntity(AZ::Vector3::CreateZero(), AZ::Vector3(100.0f, 100.0f, 1.0f));
        auto boxA = AddBoxEntity(AZ::Vector3(-5.0f, 0.0f, 1.0f), AZ::Vector3::CreateOne());
        auto boxB = AddBoxEntity(AZ::Vector3(5.0f, 0.0f, 100.0f), AZ::Vector3::CreateOne());

        UpdateDefaultWorld(1.0f / 60.0f, 60);
        bool isAwakeA = false;
        bool isAwakeB = false;
        Physics::RigidBodyRequestBus::EventResult(isAwakeA, boxA->GetId(),
            &Physics::RigidBodyRequests::IsAwake);
        Physics::RigidBodyRequestBus::EventResult(isAwakeB, boxB->GetId(),
            &Physics::RigidBodyRequests::IsAwake);

        EXPECT_FALSE(isAwakeA);
        EXPECT_TRUE(isAwakeB);

        Physics::RigidBodyRequestBus::Event(boxA->GetId(), &Physics::RigidBodyRequests::ForceAwake);
        Physics::RigidBodyRequestBus::Event(boxB->GetId(), &Physics::RigidBodyRequests::ForceAsleep);

        UpdateDefaultWorld(1.0f / 60.0f, 1);

        Physics::RigidBodyRequestBus::EventResult(isAwakeA, boxA->GetId(),
            &Physics::RigidBodyRequests::IsAwake);
        Physics::RigidBodyRequestBus::EventResult(isAwakeB, boxB->GetId(),
            &Physics::RigidBodyRequests::IsAwake);

        EXPECT_TRUE(isAwakeA);
        EXPECT_FALSE(isAwakeB);

        UpdateDefaultWorld(1.0f / 60.0f, 60);

        Physics::RigidBodyRequestBus::EventResult(isAwakeA, boxA->GetId(),
            &Physics::RigidBodyRequests::IsAwake);
        Physics::RigidBodyRequestBus::EventResult(isAwakeB, boxB->GetId(),
            &Physics::RigidBodyRequests::IsAwake);

        EXPECT_FALSE(isAwakeA);
        EXPECT_FALSE(isAwakeB);

        delete boxA;
        delete boxB;
        delete floor;
    }

    TEST_F(PhysicsComponentBusTest, DisableEnablePhysics_DynamicSphere)
    {
        using namespace AzFramework;

        auto sphere = AddSphereEntity(AZ::Vector3(0.0f, 0.0f, 0.0f), 0.5f);
        Physics::RigidBodyRequestBus::Event(sphere->GetId(), &Physics::RigidBodyRequests::SetLinearDamping, 0.0f);

        for (int timestep = 0; timestep < 30; timestep++)
        {
            UpdateDefaultWorld(1.0f / 60.0f, 1);
        }

        Physics::RigidBodyRequestBus::Event(sphere->GetId(), &Physics::RigidBodyRequests::DisablePhysics);

        AZ::Vector3 velocity;
        Physics::RigidBodyRequestBus::EventResult(velocity, sphere->GetId(), &Physics::RigidBodyRequests::GetLinearVelocity);
        float speed = velocity.GetLength();
        EXPECT_FLOAT_EQ(speed, 0.0f);

        bool physicsEnabled = true;
        Physics::RigidBodyRequestBus::EventResult(physicsEnabled, sphere->GetId(), &Physics::RigidBodyRequests::IsPhysicsEnabled);
        EXPECT_FALSE(physicsEnabled);

        Physics::RigidBodyRequestBus::Event(sphere->GetId(), &Physics::RigidBodyRequests::EnablePhysics);

        float previousSpeed = 0.0f;
        for (int timestep = 0; timestep < 60; timestep++)
        {
            UpdateDefaultWorld(1.0f / 60.0f, 1);
            Physics::RigidBodyRequestBus::EventResult(velocity, sphere->GetId(), &Physics::RigidBodyRequests::GetLinearVelocity);
            speed = velocity.GetLength();
            EXPECT_GT(speed, previousSpeed);
            previousSpeed = speed;
        }

        delete sphere;
    }
} // namespace Physics