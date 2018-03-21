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
#include <PhysXMathConversion.h>
#include <PhysXRigidBody.h>
#include <PxPhysicsAPI.h>
#include <AzCore/Serialization/EditContext.h>
#include <Include/PhysX/PhysXSystemComponentBus.h>
#include <Include/PhysX/PhysXColliderComponentBus.h>
#include <Include/PhysX/PhysXMeshShapeComponentBus.h>
#include <Include/PhysX/PhysXNativeTypeIdentifiers.h>
#include <extensions/PxRigidBodyExt.h>

namespace PhysX
{
    void PhysXRigidBodyConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PhysXRigidBodyConfiguration>()
                ->Version(1)
                ->Field("Motion type", &PhysXRigidBodyConfiguration::m_motionType)
                ->Field("Initial linear velocity", &PhysXRigidBodyConfiguration::m_initialLinearVelocity)
                ->Field("Initial angular velocity", &PhysXRigidBodyConfiguration::m_initialAngularVelocity)
                ->Field("Mass", &PhysXRigidBodyConfiguration::m_mass)
                ->Field("Compute inertia", &PhysXRigidBodyConfiguration::m_computeInertiaDiagonal)
                ->Field("Inertia diagonal values", &PhysXRigidBodyConfiguration::m_inertiaDiagonal)
                ->Field("Centre of mass offset", &PhysXRigidBodyConfiguration::m_centreOfMassOffset)
                ->Field("Linear damping", &PhysXRigidBodyConfiguration::m_linearDamping)
                ->Field("Angular damping", &PhysXRigidBodyConfiguration::m_angularDamping)
                ->Field("Sleep threshold", &PhysXRigidBodyConfiguration::m_sleepMinEnergy)
                ->Field("Continuous collision detection", &PhysXRigidBodyConfiguration::m_continuousCollisionDetection)
            ;
        }
    }

    PhysXRigidBody::PhysXRigidBody(const Physics::Ptr<Physics::RigidBodySettings>& settings)
    {
        m_config.m_motionType = settings->m_motionType;
        m_config.m_initialLinearVelocity = settings->m_initialLinearVelocity;
        m_config.m_initialAngularVelocity = settings->m_initialAngularVelocity;
        m_config.m_mass = settings->m_mass;
        m_config.m_inertiaDiagonal = settings->m_inertiaTensor.GetDiagonal();
        m_config.m_computeInertiaDiagonal = settings->m_computeInertiaTensor;
        m_config.m_linearDamping = settings->m_linearDamping;
        m_config.m_angularDamping = settings->m_angularDamping;
        m_config.m_sleepMinEnergy = settings->m_sleepMinEnergy;
        m_config.m_continuousCollisionDetection = settings->m_continuousType != Physics::ContinuousType::None;
        m_config.m_bodyShape = settings->m_bodyShape;
        CreatePhysXActor();
    }

    void PhysXRigidBody::Reflect(AZ::ReflectContext* context)
    {
        PhysXRigidBodyConfiguration::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PhysXRigidBody>()
                ->Version(1)
                ->Field("PhysXRigidBodyConfiguration", &PhysXRigidBody::m_config)
            ;
        }
    }

    physx::PxRigidActor* PhysXRigidBody::CreatePhysXActor()
    {
        physx::PxTransform pxTransform = PxTransformFromLYTransform(m_transform);

        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            m_pxRigidActor = PxGetPhysics().createRigidStatic(pxTransform);

            if (!m_pxRigidActor)
            {
                AZ_Error("PhysX Rigid Body", false, "Failed to create PhysX rigid actor.");
                return nullptr;
            }
        }

        else
        {
            physx::PxRigidDynamic* rigidDynamic = PxGetPhysics().createRigidDynamic(pxTransform);

            if (!rigidDynamic)
            {
                AZ_Error("PhysX Rigid Body", false, "Failed to create PhysX rigid actor.");
                return nullptr;
            }

            rigidDynamic->setMass(m_config.m_mass);
            if (!m_config.m_computeInertiaDiagonal)
            {
                rigidDynamic->setMassSpaceInertiaTensor(PxVec3FromLYVec3(m_config.m_inertiaDiagonal));
            }

            if (m_config.m_motionType == Physics::MotionType::Keyframed)
            {
                rigidDynamic->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
            }
            else
            {
                rigidDynamic->setSleepThreshold(m_config.m_sleepMinEnergy);
                rigidDynamic->setLinearVelocity(PxVec3FromLYVec3(m_config.m_initialLinearVelocity));
                rigidDynamic->setAngularVelocity(PxVec3FromLYVec3(m_config.m_initialAngularVelocity));
                rigidDynamic->setLinearDamping(m_config.m_linearDamping);
                rigidDynamic->setAngularDamping(m_config.m_angularDamping);
            }

            rigidDynamic->setCMassLocalPose(physx::PxTransform(PxVec3FromLYVec3(m_config.m_centreOfMassOffset)));
            m_pxRigidActor = rigidDynamic;
        }

        if (m_entity)
        {
            m_pxRigidActor->userData = reinterpret_cast<void*>(static_cast<AZ::u64>(m_entity->GetId()));
            m_pxRigidActor->setName(m_entity->GetName().c_str());
        }

        // check for mesh data on the entity and add if it exists, otherwise add shapes based on shape configuration
        AZ::Data::Asset<Pipeline::PhysXMeshAsset> meshColliderAsset = nullptr;
        if (m_entity)
        {
            PhysXMeshShapeComponentRequestBus::EventResult(meshColliderAsset, m_entity->GetId(), &PhysXMeshShapeComponentRequestBus::Events::GetMeshAsset);
        }

        if (meshColliderAsset)
        {
            auto meshData = meshColliderAsset.Get()->GetMeshData();
            AZ_Error("PhysX", meshData != nullptr, "Error. Invalid mesh collider asset on entity %s", m_entity->GetName().c_str());

            if (meshData)
            {
                auto mat = PxGetPhysics().createMaterial(0.5f, 0.5f, 0.5f);

                physx::PxShape* meshShape = nullptr;

                if (meshData->is<physx::PxTriangleMesh>())
                {
                    if(m_config.m_motionType == Physics::MotionType::Static)
                    {
                        physx::PxTriangleMeshGeometry geom(reinterpret_cast<physx::PxTriangleMesh*>(meshData), physx::PxMeshScale(PxVec3FromLYVec3(m_config.m_scale)));
                        meshShape = PxGetPhysics().createShape(geom, *mat);
                    }
                    else
                    {
                        AZ_Error("PhysX", false, "Triangle mesh cannot be used on dynamic objects."
                            " Please use the asset exported as a Convex or change the motion type to Static on entity %s", 
                            m_entity->GetName().c_str());
                    }
                }
                else if (meshData->is<physx::PxConvexMesh>())
                {
                    physx::PxConvexMeshGeometry geom(reinterpret_cast<physx::PxConvexMesh*>(meshData), physx::PxMeshScale(PxVec3FromLYVec3(m_config.m_scale)));
                    meshShape = PxGetPhysics().createShape(geom, *mat);
                }
                else
                {
                    AZ_Error("PhysX", false, "Incorrect mesh data type on entity %s", m_entity->GetName().c_str());
                }

                if (meshShape)
                {
                    m_pxRigidActor->attachShape(*meshShape);
                }
                else
                {
                    AZ_Error("PhysX Rigid Body", false, "Failed to create shape on entity %s", m_entity->GetName().c_str());
                }
            }
        }
        else
        {
            AddShapes();
        }

        return m_pxRigidActor;
    }

    void PhysXRigidBody::AddShapes()
    {
        if (!m_config.m_bodyShape || !m_pxRigidActor)
        {
            return;
        }

        physx::PxMaterial* mat = PxGetPhysics().createMaterial(0.5f, 0.5f, 0.5f);
        physx::PxShape* shape = nullptr;

        if (m_config.m_bodyShape->GetShapeType() == Physics::ShapeType::Sphere)
        {
            Physics::SphereShapeConfiguration* sphereConfig = static_cast<Physics::SphereShapeConfiguration*>(m_config.m_bodyShape.get());
            shape = PxGetPhysics().createShape(physx::PxSphereGeometry(sphereConfig->m_radius), *mat);
        }

        else if (m_config.m_bodyShape->GetShapeType() == Physics::ShapeType::Box)
        {
            Physics::BoxShapeConfiguration* boxConfig = static_cast<Physics::BoxShapeConfiguration*>(m_config.m_bodyShape.get());
            shape = PxGetPhysics().createShape(physx::PxBoxGeometry(PxVec3FromLYVec3(boxConfig->m_halfExtents)), *mat);
        }

        else if (m_config.m_bodyShape->GetShapeType() == Physics::ShapeType::Capsule)
        {
            Physics::CapsuleShapeConfiguration* capsuleConfig = static_cast<Physics::CapsuleShapeConfiguration*>(m_config.m_bodyShape.get());
            float halfHeight = 0.5f * capsuleConfig->m_pointA.GetDistance(capsuleConfig->m_pointB);
            shape = PxGetPhysics().createShape(physx::PxCapsuleGeometry(halfHeight, capsuleConfig->m_radius), *mat);

            // PhysX creates capsules with the axis along the x direction, so set the pose to rotate to the desired orientation
            AZ::Vector3 capsuleAxis = (capsuleConfig->m_pointB - capsuleConfig->m_pointA).GetNormalized();
            AZ::Vector3 xAxis(1.0f, 0.0f, 0.0f);
            float cosTheta = xAxis.Dot(capsuleAxis);
            // don't need to rotate if capsule axis is parallel to x-axis (and rotation axis would be degenerate)
            if (fabsf(cosTheta) < 1.0f - 1e-3f && shape)
            {
                AZ::Vector3 rotationAxis = xAxis.Cross(capsuleAxis);
                physx::PxQuat pxQuat(acosf(cosTheta), PxVec3FromLYVec3(rotationAxis));
                shape->setLocalPose(physx::PxTransform(pxQuat));
            }
        }

        else
        {
            AZ_Warning("PhysX Rigid Body", false, "Shape not supported in PhysX. Entity %s", m_entity->GetName().c_str());
        }

        if (shape)
        {
            m_pxRigidActor->attachShape(*shape);
        }

        else
        {
            AZ_Error("PhysX Rigid Body", false, "Failed to create shape. Entity %s", m_entity->GetName().c_str());
        }

        if (m_config.m_motionType != Physics::MotionType::Static && m_config.m_computeInertiaDiagonal)
        {
            auto massLocalPose = PxVec3FromLYVec3(m_config.m_centreOfMassOffset);
            auto pxRigidBody = static_cast<physx::PxRigidBody*>(m_pxRigidActor);
            physx::PxRigidBodyExt::setMassAndUpdateInertia(*pxRigidBody, m_config.m_mass, &massLocalPose);
        }
    }

    void PhysXRigidBody::ReleasePhysXActor()
    {
        if (!m_pxRigidActor)
        {
            return;
        }

        physx::PxScene* scene = m_pxRigidActor->getScene();
        if (scene)
        {
            scene->removeActor(*m_pxRigidActor);
        }

        m_pxRigidActor->release();
        m_pxRigidActor = nullptr;
    }

    void PhysXRigidBody::GetTransformUpdateFromPhysX()
    {
        if (m_pxRigidActor)
        {
            m_transform = LYTransformFromPxTransform(m_pxRigidActor->getGlobalPose());
        }
    }

    void PhysXRigidBody::SetConfig(const PhysXRigidBodyConfiguration& config)
    {
        m_config = config;
    }

    void PhysXRigidBody::SetShape(Physics::Ptr<Physics::ShapeConfiguration> shape)
    {
        m_config.m_bodyShape = shape;
    }

    void PhysXRigidBody::SetScale(const AZ::Vector3&& scale)
    {
        m_config.m_scale = scale;
    }

    void PhysXRigidBody::SetEntity(AZ::Entity* entity)
    {
        m_entity = entity;
    }

    // Physics::RigidBody
    Physics::Ptr<Physics::NativeShape> PhysXRigidBody::GetNativeShape()
    {
        // Just check if there are any shapes attached to the rigid body
        // and return the first one if there are any.
        // This will need to be revisited when compound colliders are implemented.
        auto nbShapes = m_pxRigidActor->getNbShapes();
        if (nbShapes)
        {
            physx::PxShape* shapeBuffer[1];
            m_pxRigidActor->getShapes(shapeBuffer, 1, 0);
            Physics::Ptr<Physics::NativeShape> shapePtr = aznew Physics::NativeShape(static_cast<void*>(shapeBuffer[0]));
            return shapePtr;
        }

        else
        {
            AZ_Warning("PhysX Rigid Body", false, "GetNativeShape called on rigid body with no shapes attached.")
        }

        return Physics::Ptr<Physics::NativeShape>();
    }

    AZ::Vector3 PhysXRigidBody::GetCenterOfMassWorld() const
    {
        AZ::Transform transform = LYTransformFromPxTransform(m_pxRigidActor->getGlobalPose());
        return transform * m_config.m_centreOfMassOffset;
    }

    AZ::Vector3 PhysXRigidBody::GetCenterOfMassLocal() const
    {
        return m_config.m_centreOfMassOffset;
    }

    AZ::Matrix3x3 PhysXRigidBody::GetInverseInertiaWorld() const
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            AZ_Warning("PhysX Rigid Body", false, "Inverse inertia is not defined for static rigid bodies.");
            return AZ::Matrix3x3::CreateZero();
        }

        physx::PxRigidBody* rigidBody = static_cast<physx::PxRigidBody*>(m_pxRigidActor);
        AZ::Vector3 inverseInertiaDiagonal = LYVec3FromPxVec3(rigidBody->getMassSpaceInvInertiaTensor());
        AZ::Matrix3x3 rotationToWorld = AZ::Matrix3x3::CreateFromQuaternion(LYQuatFromPxQuat(m_pxRigidActor->getGlobalPose().q.getConjugate()));
        return Physics::InverseInertiaLocalToWorld(inverseInertiaDiagonal, rotationToWorld);
    }

    AZ::Matrix3x3 PhysXRigidBody::GetInverseInertiaLocal() const
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            AZ_Warning("PhysX Rigid Body", false, "Inverse inertia is not defined for static rigid bodies.");
            return AZ::Matrix3x3::CreateZero();
        }

        physx::PxVec3 inverseInertiaDiagonal = static_cast<physx::PxRigidBody*>(m_pxRigidActor)->getMassSpaceInvInertiaTensor();
        return AZ::Matrix3x3::CreateDiagonal(LYVec3FromPxVec3(inverseInertiaDiagonal));
    }

    float PhysXRigidBody::GetEnergy() const
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            AZ_Warning("PhysX Rigid Body", false, "Energy is not defined for static rigid bodies.");
            return 0.0f;
        }

        physx::PxRigidDynamic* rigidDynamic = static_cast<physx::PxRigidDynamic*>(m_pxRigidActor);
        physx::PxVec3 localAngularVelocity = rigidDynamic->getGlobalPose().rotateInv(rigidDynamic->getAngularVelocity());
        physx::PxVec3 localAngularMomentum = rigidDynamic->getMassSpaceInertiaTensor().multiply(localAngularVelocity);
        float angularEnergy = 0.5f * localAngularVelocity.dot(localAngularMomentum);

        physx::PxVec3 linearVelocity = rigidDynamic->getLinearVelocity();
        float linearEnergy = 0.5f * rigidDynamic->getMass() * linearVelocity.dot(linearVelocity);

        return angularEnergy + linearEnergy;
    }

    Physics::MotionType PhysXRigidBody::GetMotionType() const
    {
        return m_config.m_motionType;
    }

    void PhysXRigidBody::SetMotionType(Physics::MotionType motionType)
    {
        // don't need to do anything if the existing motion type matches the requested type
        if (m_config.m_motionType == motionType)
        {
            return;
        }

        // changing to or from static
        if (m_config.m_motionType == Physics::MotionType::Static || motionType == Physics::MotionType::Static)
        {
            // can't switch between dynamic and static rigid bodies so release and recreate
            // copy previous transform and shapes
            // don't need to copy linear and angular velocity because either
            // the body was previous static and so had 0 velocity or
            // it is changing to static and so will have 0 velocity
            if (m_pxRigidActor)
            {
                // transfer the shapes from the old actor
                physx::PxU32 nbShapes = m_pxRigidActor->getNbShapes();
                AZStd::vector<physx::PxShape*> shapes(nbShapes);
                
                m_pxRigidActor->getShapes(shapes.data(), nbShapes);
                
                // Check for trimesh shapes. They are not supported for dynamic objects.
                if( AZStd::any_of(shapes.begin(), shapes.end(), 
                    [](physx::PxShape* shape) { return shape->getGeometryType() == physx::PxGeometryType::eTRIANGLEMESH; }) )
                {
                    AZ_Error("PhysX Rigid Body", false, "Failed to convert the motion type of %s. Triangle Meshes cannot be used for dynamic objects.", m_entity->GetName());
                    return;
                }


                physx::PxRigidActor* newActor = nullptr;
                physx::PxTransform pose = m_pxRigidActor->getGlobalPose();

                if (motionType == Physics::MotionType::Static)
                {
                    newActor = PxGetPhysics().createRigidStatic(pose);

                    if (!newActor)
                    {
                        AZ_Error("PhysX Rigid Body", false, "Failed to create rigid body when changing motion type.");
                        return;
                    }
                }

                else
                {
                    newActor = PxGetPhysics().createRigidDynamic(pose);

                    if (!newActor)
                    {
                        AZ_Error("PhysX Rigid Body", false, "Failed to create rigid body when changing motion type.");
                        return;
                    }

                    static_cast<physx::PxRigidDynamic*>(newActor)->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, motionType == Physics::MotionType::Keyframed);
                }

                AZStd::for_each(shapes.begin(), shapes.end(), [newActor](physx::PxShape* shape) { newActor->attachShape(*shape); });

                // copy the user data and name from the old actor
                newActor->userData = m_pxRigidActor->userData;
                newActor->setName(m_pxRigidActor->getName());

                // release the old actor and replace it with the new one
                // if the actor is in a scene, need to remove and re-add it
                physx::PxScene* scene = m_pxRigidActor->getScene();
                m_pxRigidActor->release();
                m_pxRigidActor = newActor;
                if (scene)
                {
                    scene->addActor(*m_pxRigidActor);
                }
            }
        }

        // only remaining case is switching between dynamic and keyframed
        else
        {
            if (m_pxRigidActor)
            {
                if (m_pxRigidActor->getType() == physx::PxActorType::eRIGID_DYNAMIC)
                {
                    static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, motionType == Physics::MotionType::Keyframed);
                }
            }
        }

        // finally update the motion type
        m_config.m_motionType = motionType;
    }

    float PhysXRigidBody::GetMass() const
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            AZ_Warning("PhysX Rigid Body", false, "Mass is not defined for static rigid bodies.");
            return 0.0f;
        }

        return static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->getMass();
    }

    float PhysXRigidBody::GetInverseMass() const
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            AZ_Warning("PhysX Rigid Body", false, "Inverse mass is not defined for static rigid bodies.");
            return 0.0f;
        }

        return static_cast<physx::PxRigidBody*>(m_pxRigidActor)->getInvMass();
    }

    void PhysXRigidBody::SetMass(float mass) const
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            AZ_Warning("PhysX Rigid Body", false, "SetMass has no effect for static rigid bodies.");
            return;
        }

        static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->setMass(mass);
    }

    AZ::Vector3 PhysXRigidBody::GetLinearVelocity() const
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            return AZ::Vector3::CreateZero();
        }

        return LYVec3FromPxVec3(static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->getLinearVelocity());
    }

    void PhysXRigidBody::SetLinearVelocity(AZ::Vector3 velocity)
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            AZ_Warning("PhysX Rigid Body", false, "SetLinearVelocity has no effect for static rigid bodies.");
            return;
        }

        static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->setLinearVelocity(PxVec3FromLYVec3(velocity));
    }

    AZ::Vector3 PhysXRigidBody::GetAngularVelocity() const
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            AZ_Warning("PhysX Rigid Body", false, "Angular velocity always 0 for static rigid bodies.");
            return AZ::Vector3::CreateZero();
        }

        return LYVec3FromPxVec3(static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->getAngularVelocity());
    }

    void PhysXRigidBody::SetAngularVelocity(AZ::Vector3 angularVelocity)
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            AZ_Warning("PhysX Rigid Body", false, "SetAngularVelocity has no effect for static rigid bodies.");
            return;
        }

        static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->setAngularVelocity(PxVec3FromLYVec3(angularVelocity));
    }

    AZ::Vector3 PhysXRigidBody::GetLinearVelocityAtWorldPoint(AZ::Vector3 worldPoint)
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            AZ_Warning("PhysX Rigid Body", false, "Linear velocity always 0 for static rigid bodies.");
            return AZ::Vector3::CreateZero();
        }

        return GetLinearVelocity() + GetAngularVelocity().Cross(worldPoint - GetCenterOfMassWorld());
    }

    void PhysXRigidBody::ApplyLinearImpulse(AZ::Vector3 impulse)
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            AZ_Warning("PhysX Rigid Body", false, "ApplyLinearImpulse is not valid for static bodies.");
            return;
        }

        physx::PxScene* scene = m_pxRigidActor->getScene();
        if (!scene)
        {
            AZ_Warning("PhysX Rigid Body", false, "ApplyLinearImpulse is only valid if rigid body has been added to a scene.");
            return;
        }

        static_cast<physx::PxRigidBody*>(m_pxRigidActor)->addForce(PxVec3FromLYVec3(impulse), physx::PxForceMode::eIMPULSE);
    }

    void PhysXRigidBody::ApplyLinearImpulseAtWorldPoint(AZ::Vector3 impulse, AZ::Vector3 worldPoint)
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            AZ_Warning("PhysX Rigid Body", false, "ApplyLinearImpulseAtWorldPoint is not valid for static bodies.");
            return;
        }

        physx::PxRigidBodyExt::addForceAtPos(*static_cast<physx::PxRigidBody*>(m_pxRigidActor), PxVec3FromLYVec3(impulse), PxVec3FromLYVec3(worldPoint), physx::PxForceMode::eIMPULSE);
    }

    void PhysXRigidBody::ApplyAngularImpulse(AZ::Vector3 angularImpulse)
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            AZ_Warning("PhysX Rigid Body", false, "ApplyAngularImpulse is not valid for static bodies.");
            return;
        }

        physx::PxScene* scene = m_pxRigidActor->getScene();
        if (!scene)
        {
            AZ_Warning("PhysX Rigid Body", false, "ApplyAngularImpulse is only valid if rigid body has been added to a scene.");
            return;
        }

        static_cast<physx::PxRigidBody*>(m_pxRigidActor)->addTorque(PxVec3FromLYVec3(angularImpulse), physx::PxForceMode::eIMPULSE);
    }

    bool PhysXRigidBody::IsAwake() const
    {
        if (m_config.m_motionType == Physics::MotionType::Static)
        {
            return false;
        }

        return !(static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->isSleeping());
    }

    void PhysXRigidBody::ForceAsleep()
    {
        if (m_config.m_motionType != Physics::MotionType::Dynamic)
        {
            AZ_Warning("PhysX Rigid Body", false, "ForceAsleep is only valid for dynamic rigid bodies.");
            return;
        }

        static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->putToSleep();
    }

    void PhysXRigidBody::ForceAwake()
    {
        if (m_config.m_motionType != Physics::MotionType::Dynamic)
        {
            AZ_Warning("PhysX Rigid Body", false, "ForceAwake is only valid for dynamic rigid bodies.");
            return;
        }

        static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->wakeUp();
    }

    // Physics::WorldBody
    AZ::Transform PhysXRigidBody::GetTransform() const
    {
        return LYTransformFromPxTransform(m_pxRigidActor->getGlobalPose());
    }

    void PhysXRigidBody::SetTransform(const AZ::Transform& transform)
    {
        m_transform = transform;
        if (m_pxRigidActor)
        {
            m_pxRigidActor->setGlobalPose(PxTransformFromLYTransform(transform));
        }
    }

    AZ::Vector3 PhysXRigidBody::GetPosition() const
    {
        return LYVec3FromPxVec3(m_pxRigidActor->getGlobalPose().p);
    }

    AZ::Quaternion PhysXRigidBody::GetOrientation() const
    {
        return LYQuatFromPxQuat(m_pxRigidActor->getGlobalPose().q);
    }

    Physics::ObjectCollisionFilter PhysXRigidBody::GetCollisionFilter() const
    {
        AZ_Warning("PhysX Rigid Body", false, "Collision filters not implemented.");
        return Physics::ObjectCollisionFilter();
    }

    void PhysXRigidBody::SetCollisionFilter(const Physics::ObjectCollisionFilter& filter)
    {
        AZ_Warning("PhysX Rigid Body", false, "Collision filters not implemented.");
    }

    AZ::Aabb PhysXRigidBody::GetAabb() const
    {
        physx::PxBounds3 bounds = m_pxRigidActor->getWorldBounds(1.0f);
        return AZ::Aabb::CreateFromMinMax(LYVec3FromPxVec3(bounds.minimum), LYVec3FromPxVec3(bounds.maximum));
    }

    void PhysXRigidBody::RayCast(const Physics::RayCastRequest& request, Physics::RayCastResult& result) const
    {
        AZ_Warning("PhysX Rigid Body", false, "RayCast not implemented.");
    }

    // Physics::ReferenceBase
    AZ::Crc32 PhysXRigidBody::GetNativeType() const
    {
        return PhysX::NativeTypeIdentifiers::PhysXRigidBody;
    }

    void* PhysXRigidBody::GetNativePointer() const
    {
        return m_pxRigidActor;
    }

    // Not in API but needed to support PhysicsComponentBus
    float PhysXRigidBody::GetLinearDamping()
    {
        if (m_config.m_motionType != Physics::MotionType::Dynamic)
        {
            AZ_Warning("PhysX Rigid Body", false, "Linear damping is only defined for dynamic rigid bodies.");
            return 0.0f;
        }

        return static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->getLinearDamping();
    }

    void PhysXRigidBody::SetLinearDamping(float damping)
    {
        if (m_config.m_motionType != Physics::MotionType::Dynamic)
        {
            AZ_Warning("PhysX Rigid Body", false, "Linear damping is only defined for dynamic rigid bodies.");
            return;
        }

        if (damping < 0.0f)
        {
            AZ_Warning("PhysX Rigid Body", false, "Negative linear damping value (%6.4e).", damping);
            return;
        }

        static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->setLinearDamping(damping);
    }

    float PhysXRigidBody::GetAngularDamping()
    {
        if (m_config.m_motionType != Physics::MotionType::Dynamic)
        {
            AZ_Warning("PhysX Rigid Body", false, "Angular damping is only defined for dynamic rigid bodies.");
            return 0.0f;
        }

        return static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->getAngularDamping();
    }

    void PhysXRigidBody::SetAngularDamping(float damping)
    {
        if (m_config.m_motionType != Physics::MotionType::Dynamic)
        {
            AZ_Warning("PhysX Rigid Body", false, "Angular damping is only defined for dynamic rigid bodies.");
            return;
        }

        if (damping < 0.0f)
        {
            AZ_Warning("PhysX Rigid Body", false, "Negative angular damping value (%6.4e).", damping);
            return;
        }

        static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->setAngularDamping(damping);
    }

    float PhysXRigidBody::GetSleepThreshold()
    {
        if (m_config.m_motionType != Physics::MotionType::Dynamic)
        {
            AZ_Warning("PhysX Rigid Body", false, "Sleep threshold is only defined for dynamic rigid bodies.");
            return 0.0f;
        }

        return static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->getSleepThreshold();
    }

    void PhysXRigidBody::SetSleepThreshold(float threshold)
    {
        if (m_config.m_motionType != Physics::MotionType::Dynamic)
        {
            AZ_Warning("PhysX Rigid Body", false, "Sleep threshold is only defined for dynamic rigid bodies.");
            return;
        }

        if (threshold < 0.0f)
        {
            AZ_Warning("PhysX Rigid Body", false, "Negative sleep threshold value (%6.4e).", threshold);
            return;
        }

        static_cast<physx::PxRigidDynamic*>(m_pxRigidActor)->setSleepThreshold(threshold);
    }
}