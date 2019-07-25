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

#include <AzCore/EBus/Results.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Physics/ShapeConfiguration.h>

#include <PhysX/ColliderShapeBus.h>
#include <PhysX/SystemComponentBus.h>
#include <PhysX/MeshAsset.h>
#include <PhysX/Utils.h>
#include <Source/SystemComponent.h>
#include <Source/Collision.h>
#include <Source/Pipeline/MeshAssetHandler.h>
#include <Source/Shape.h>
#include <Source/TerrainComponent.h>
#include <Source/RigidBodyStatic.h>
#include <Source/Utils.h>

namespace PhysX
{
    namespace Utils
    {
        bool CreatePxGeometryFromConfig(const Physics::ShapeConfiguration& shapeConfiguration, physx::PxGeometryHolder& pxGeometry, bool warnForNullAsset)
        {
            if (!shapeConfiguration.m_scale.IsGreaterThan(AZ::Vector3::CreateZero()))
            {
                AZ_Error("PhysX Utils", false, "Negative or zero values are invalid for shape configuration scale values %s",
                    ToString(shapeConfiguration.m_scale).c_str());
                return false;
            }

            auto shapeType = shapeConfiguration.GetShapeType();

            switch (shapeType)
            {
            case Physics::ShapeType::Sphere:
            {
                const Physics::SphereShapeConfiguration& sphereConfig = static_cast<const Physics::SphereShapeConfiguration&>(shapeConfiguration);
                if (sphereConfig.m_radius <= 0.0f)
                {
                    AZ_Error("PhysX Utils", false, "Invalid radius value: %f", sphereConfig.m_radius);
                    return false;
                }
                pxGeometry.storeAny(physx::PxSphereGeometry(sphereConfig.m_radius * shapeConfiguration.m_scale.GetMaxElement()));
                break;
            }
            case Physics::ShapeType::Box:
            {
                const Physics::BoxShapeConfiguration& boxConfig = static_cast<const Physics::BoxShapeConfiguration&>(shapeConfiguration);
                if (!boxConfig.m_dimensions.IsGreaterThan(AZ::Vector3::CreateZero()))
                {
                    AZ_Error("PhysX Utils", false, "Negative or zero values are invalid for box dimensions %s",
                        ToString(boxConfig.m_dimensions).c_str());
                    return false;
                }
                pxGeometry.storeAny(physx::PxBoxGeometry(PxMathConvert(boxConfig.m_dimensions * 0.5f * shapeConfiguration.m_scale)));
                break;
            }
            case Physics::ShapeType::Capsule:
            {
                const Physics::CapsuleShapeConfiguration& capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(shapeConfiguration);
                float height = capsuleConfig.m_height * capsuleConfig.m_scale.GetZ();
                float radius = capsuleConfig.m_radius * capsuleConfig.m_scale.GetX().GetMax(capsuleConfig.m_scale.GetY());

                if (height <= 0.0f || radius <= 0.0f)
                {
                    AZ_Error("PhysX Utils", false, "Negative or zero values are invalid for capsule dimensions (height: %f, radius: %f)",
                        capsuleConfig.m_height, capsuleConfig.m_radius);
                    return false;
                }

                float halfHeight = 0.5f * height - radius;
                if (halfHeight <= 0.0f)
                {
                    AZ_Warning("PhysX", false, "Height must exceed twice the radius in capsule configuration (height: %f, radius: %f)",
                        capsuleConfig.m_height, capsuleConfig.m_radius);
                    halfHeight = std::numeric_limits<float>::epsilon();
                }
                pxGeometry.storeAny(physx::PxCapsuleGeometry(radius, halfHeight));
                break;
            }
            case Physics::ShapeType::PhysicsAsset:
            {
                const Physics::PhysicsAssetShapeConfiguration& assetShapeConfig = static_cast<const Physics::PhysicsAssetShapeConfiguration&>(shapeConfiguration);
                AZ::Vector3 scale = assetShapeConfig.m_assetScale * assetShapeConfig.m_scale;

                Pipeline::MeshAsset* physXMeshAsset = assetShapeConfig.m_asset.GetAs<Pipeline::MeshAsset>();
                if (!physXMeshAsset)
                {
                    if (warnForNullAsset)
                    {
                        AZ_Error("PhysX Rigid Body", false, "PhysXUtils::CreatePxGeometryFromConfig. Mesh asset is null.");
                    }
                    return false;
                }

                physx::PxBase* meshData = physXMeshAsset->GetMeshData();

                return MeshDataToPxGeometry(meshData, pxGeometry, scale);
            }
            case Physics::ShapeType::Native:
            {
                const Physics::NativeShapeConfiguration& nativeShapeConfig = static_cast<const Physics::NativeShapeConfiguration&>(shapeConfiguration);
                AZ::Vector3 scale = nativeShapeConfig.m_nativeShapeScale * nativeShapeConfig.m_scale;
                physx::PxBase* meshData = reinterpret_cast<physx::PxBase*>(nativeShapeConfig.m_nativeShapePtr);
                return MeshDataToPxGeometry(meshData, pxGeometry, scale);
            }
            default:
                AZ_Warning("PhysX Rigid Body", false, "Shape not supported in PhysX. Shape Type: %d", shapeType);
                return false;
            }

            return true;
        }

        physx::PxShape* CreatePxShapeFromConfig(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration, Physics::CollisionGroup& assignedCollisionGroup)
        {
            AZStd::vector<physx::PxMaterial*> materials;
            MaterialManagerRequestsBus::Broadcast(&MaterialManagerRequestsBus::Events::GetPxMaterials, colliderConfiguration.m_materialSelection, materials);

            if (materials.empty())
            {
                AZ_Error("PhysX", false, "Material array can't be empty!");
                return nullptr;
            }

            physx::PxGeometryHolder pxGeomHolder;
            if (Utils::CreatePxGeometryFromConfig(shapeConfiguration, pxGeomHolder))
            {
                auto materialsCount = static_cast<physx::PxU16>(materials.size());
                physx::PxShape* shape = PxGetPhysics().createShape(pxGeomHolder.any(), materials.begin(), materialsCount, colliderConfiguration.m_isExclusive);

                if (shape)
                {
                    Physics::CollisionGroup collisionGroup;
                    Physics::CollisionRequestBus::BroadcastResult(collisionGroup, &Physics::CollisionRequests::GetCollisionGroupById, colliderConfiguration.m_collisionGroupId);

                    physx::PxFilterData filterData = PhysX::Collision::CreateFilterData(colliderConfiguration.m_collisionLayer, collisionGroup);
                    shape->setSimulationFilterData(filterData);
                    shape->setQueryFilterData(filterData);

                    // Do custom logic for specific shape types
                    if (pxGeomHolder.getType() == physx::PxGeometryType::eCAPSULE)
                    {
                        // PhysX capsules are oriented around x by default.
                        physx::PxQuat pxQuat(AZ::Constants::HalfPi, physx::PxVec3(0.0f, 1.0f, 0.0f));
                        shape->setLocalPose(physx::PxTransform(pxQuat));
                    }

                    if (colliderConfiguration.m_isTrigger)
                    {
                        shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
                        shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
                        shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
                    }

                    physx::PxTransform pxShapeTransform = PxMathConvert(colliderConfiguration.m_position, colliderConfiguration.m_rotation);
                    shape->setLocalPose(pxShapeTransform * shape->getLocalPose());

                    assignedCollisionGroup = collisionGroup;
                    return shape;
                }
                else
                {
                    AZ_Error("PhysX Rigid Body", false, "Failed to create shape.");
                    return nullptr;
                }
            }
            return nullptr;
        }

        World* GetDefaultWorld()
        {
            AZStd::shared_ptr<Physics::World> world = nullptr;
            Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);
            return static_cast<World*>(world.get());
        }

        AZStd::string ConvexCookingResultToString(physx::PxConvexMeshCookingResult::Enum convexCookingResultCode)
        {
            static const AZStd::string resultToString[] = { "eSUCCESS", "eZERO_AREA_TEST_FAILED", "ePOLYGONS_LIMIT_REACHED", "eFAILURE" };
            if (AZ_ARRAY_SIZE(resultToString) > convexCookingResultCode)
            {
                return resultToString[convexCookingResultCode];
            }
            else
            {
                AZ_Error("PhysX", false, "Unknown convex cooking result code: %i", convexCookingResultCode);
                return "";
            }
        }

        AZStd::string TriMeshCookingResultToString(physx::PxTriangleMeshCookingResult::Enum triangleCookingResultCode)
        {
            static const AZStd::string resultToString[] = { "eSUCCESS", "eLARGE_TRIANGLE", "eFAILURE" };
            if (AZ_ARRAY_SIZE(resultToString) > triangleCookingResultCode)
            {
                return resultToString[triangleCookingResultCode];
            }
            else
            {
                AZ_Error("PhysX", false, "Unknown trimesh cooking result code: %i", triangleCookingResultCode);
                return "";
            }
        }

        bool WriteCookedMeshToFile(const AZStd::string& filePath, const Pipeline::MeshAssetCookedData& cookedMesh)
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            return AZ::Utils::SaveObjectToFile(filePath, AZ::DataStream::ST_BINARY, &cookedMesh, serializeContext);
        }

        bool MeshDataToPxGeometry(physx::PxBase* meshData, physx::PxGeometryHolder& pxGeometry, const AZ::Vector3& scale)
        {
            if (meshData)
            {
                if (meshData->is<physx::PxTriangleMesh>())
                {
                    pxGeometry.storeAny(physx::PxTriangleMeshGeometry(reinterpret_cast<physx::PxTriangleMesh*>(meshData), physx::PxMeshScale(PxMathConvert(scale))));
                }
                else
                {
                    pxGeometry.storeAny(physx::PxConvexMeshGeometry(reinterpret_cast<physx::PxConvexMesh*>(meshData), physx::PxMeshScale(PxMathConvert(scale))));
                }

                return true;
            }
            else
            {
                AZ_Error("PhysXUtils::MeshDataToPxGeometry", false, "Mesh data is null.");
                return false;
            }
        }

        bool ReadFile(const AZStd::string& path, AZStd::vector<uint8_t>& buffer)
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (!fileIO)
            {
                AZ_Warning("PhysXUtils::ReadFile", false, "No File System");
                return false;
            }

            // Open file
            AZ::IO::HandleType file;
            if (!fileIO->Open(path.c_str(), AZ::IO::OpenMode::ModeRead, file))
            {
                AZ_Warning("PhysXUtils::ReadFile", false, "Failed to open file:%s", path.c_str());
                return false;
            }

            // Get file size, we want to read the whole thing in one go
            AZ::u64 fileSize;
            if (!fileIO->Size(file, fileSize))
            {
                AZ_Warning("PhysXUtils::ReadFile", false, "Failed to read file size:%s", path.c_str());
                fileIO->Close(file);
                return false;
            }

            if (fileSize <= 0)
            {
                AZ_Warning("PhysXUtils::ReadFile", false, "File is empty:%s", path.c_str());
                fileIO->Close(file);
                return false;
            }

            buffer.resize(fileSize);

            AZ::u64 bytesRead = 0;
            bool failOnFewerThanSizeBytesRead = false;
            if (!fileIO->Read(file, &buffer[0], fileSize, failOnFewerThanSizeBytesRead, &bytesRead))
            {
                AZ_Warning("PhysXUtils::ReadFile", false, "Failed to read file:%s", path.c_str());
                fileIO->Close(file);
                return false;
            }

            fileIO->Close(file);

            return true;
        }

        void GetMaterialList(
            AZStd::vector<physx::PxMaterial*>& pxMaterials, const AZStd::vector<int>& terrainSurfaceIdIndexMapping,
            const Physics::TerrainMaterialSurfaceIdMap& terrainMaterialsToSurfaceIds)
        {
            pxMaterials.reserve(terrainSurfaceIdIndexMapping.size());

            AZStd::shared_ptr<Material> defaultMaterial;
            MaterialManagerRequestsBus::BroadcastResult(defaultMaterial, &MaterialManagerRequestsBus::Events::GetDefaultMaterial);

            if (terrainSurfaceIdIndexMapping.empty())
            {
                pxMaterials.push_back(defaultMaterial->GetPxMaterial());
                return;
            }

            AZStd::vector<physx::PxMaterial*> materials;

            for (auto& surfaceId : terrainSurfaceIdIndexMapping)
            {
                const auto& userAssignedMaterials = terrainMaterialsToSurfaceIds;
                const auto& matSelectionIterator = userAssignedMaterials.find(surfaceId);
                if (matSelectionIterator != userAssignedMaterials.end())
                {
                    MaterialManagerRequestsBus::Broadcast(&MaterialManagerRequests::GetPxMaterials, matSelectionIterator->second, materials);

                    if (!materials.empty())
                    {
                        pxMaterials.push_back(materials.front());
                    }
                    else
                    {
                        AZ_Error("PhysX", false, "Creating materials: array with materials can't be empty");
                        pxMaterials.push_back(defaultMaterial->GetPxMaterial());
                    }
                }
                else
                {
                    pxMaterials.push_back(defaultMaterial->GetPxMaterial());
                }
            }
        }

        AZStd::unique_ptr<Physics::RigidBodyStatic> CreateTerrain(
            const PhysX::TerrainConfiguration& configuration, const AZ::EntityId& entityId, const AZStd::string_view& name)
        {
            using namespace physx;

            if (!configuration.m_heightFieldAsset.IsReady())
            {
                AZ_Warning("PhysXUtils::CreateTerrain", false, "Heightfield asset not ready");
                return nullptr;
            }

            physx::PxHeightField* heightField = configuration.m_heightFieldAsset.Get()->GetHeightField();
            if (!heightField)
            {
                AZ_Warning("PhysXUtils::CreateTerrain", false, "HeightField Asset has no heightfield");
                return nullptr;
            }

            // Get terrain materials
            AZStd::vector<physx::PxMaterial*> materialList;
            GetMaterialList(materialList, configuration.m_terrainSurfaceIdIndexMapping, configuration.m_terrainMaterialsToSurfaceIds);

            const float heightScale = configuration.m_scale.GetZ();
            const float rowScale = configuration.m_scale.GetX();
            const float colScale = configuration.m_scale.GetY();

            physx::PxHeightFieldGeometry heightfieldGeom(heightField, physx::PxMeshGeometryFlags(), heightScale, rowScale, colScale);
            AZ_Warning("Terrain Component", heightfieldGeom.isValid(), "Invalid height field");

            if (!heightfieldGeom.isValid())
            {
                AZ_Warning("Terrain Component", false, "Invalid height field");
                return nullptr;
            }

            PxShape* pxShape = PxGetPhysics().createShape(heightfieldGeom, materialList.begin(), static_cast<physx::PxU16>(materialList.size()), true);
            pxShape->setLocalPose(PxTransform(PxVec3(PxZero), PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)) * PxQuat(PxHalfPi, PxVec3(1.0f, 0.0f, 0.0f))));

            AZStd::shared_ptr<PhysX::Shape> heightFieldShape = AZStd::make_shared<PhysX::Shape>(pxShape);
            pxShape->release();

            PhysX::Configuration globalConfiguration;
            PhysX::ConfigurationRequestBus::BroadcastResult(globalConfiguration, &ConfigurationRequests::GetConfiguration);

            Physics::CollisionLayer terrainCollisionLayer = configuration.m_collisionLayer;
            Physics::CollisionGroup terrainCollisionGroup = Physics::CollisionGroup::All;
            Physics::CollisionRequestBus::BroadcastResult(terrainCollisionGroup, &Physics::CollisionRequests::GetCollisionGroupById, configuration.m_collisionGroup);

            heightFieldShape->SetCollisionLayer(terrainCollisionLayer);
            heightFieldShape->SetCollisionGroup(terrainCollisionGroup);
            heightFieldShape->SetName(name.data());

            Physics::WorldBodyConfiguration staticRigidBodyConfiguration;
            staticRigidBodyConfiguration.m_position = AZ::Vector3::CreateZero();
            staticRigidBodyConfiguration.m_entityId = entityId;
            staticRigidBodyConfiguration.m_debugName = name;

            AZStd::unique_ptr<Physics::RigidBodyStatic> terrainTile = AZStd::make_unique<PhysX::RigidBodyStatic>(staticRigidBodyConfiguration);
            terrainTile->AddShape(heightFieldShape);

            return terrainTile;
        }

        AZStd::string ReplaceAll(AZStd::string str, const AZStd::string& fromString, const AZStd::string& toString) {
            size_t positionBegin = 0;
            while ((positionBegin = str.find(fromString, positionBegin)) != AZStd::string::npos) 
            {
                str.replace(positionBegin, fromString.length(), toString);
                positionBegin += toString.length();
            }
            return str;
        }

        void WarnEntityNames(const AZStd::vector<AZ::EntityId>& entityIds, const char* category, const char* message)
        {
            AZStd::string messageOutput = message;
            messageOutput += "\n";
            for (const auto& entityId : entityIds)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
                if (entity)
                {
                    messageOutput += entity->GetName() + "\n";
                }
            }

            AZStd::string percentageSymbol("%");
            AZStd::string percentageReplace("%%"); //Replacing % with %% serves to escape the % character when printing out the entity names in printf style.
            messageOutput = ReplaceAll(messageOutput, percentageSymbol, percentageReplace);

            AZ_Warning(category, false, messageOutput.c_str());
        }

        AZ::Transform GetColliderLocalTransform(const AZ::Vector3& colliderRelativePosition
            , const AZ::Quaternion& colliderRelativeRotation)
        {
            return AZ::Transform::CreateFromQuaternionAndTranslation(colliderRelativeRotation, colliderRelativePosition);
        }

        AZ::Transform GetColliderWorldTransform(const AZ::Transform& worldTransform
            , const AZ::Vector3& colliderRelativePosition
            , const AZ::Quaternion& colliderRelativeRotation)
        {
            return worldTransform * GetColliderLocalTransform(colliderRelativePosition, colliderRelativeRotation);
        }

        void ColliderPointsLocalToWorld(AZStd::vector<AZ::Vector3>& pointsInOut
            , const AZ::Transform& worldTransform
            , const AZ::Vector3& colliderRelativePosition
            , const AZ::Quaternion& colliderRelativeRotation)
        {
            AZ::Transform transform = GetColliderWorldTransform(worldTransform
                , colliderRelativePosition
                , colliderRelativeRotation);

            for (AZ::Vector3& point : pointsInOut)
            {
                point = transform * point;
            }
        }

        AZ::Aabb GetColliderAabb(const AZ::Transform& worldTransform
            , const ::Physics::ShapeConfiguration& shapeConfiguration
            , const ::Physics::ColliderConfiguration& colliderConfiguration)
        {
            AZ::Aabb aabb;
            physx::PxGeometryHolder geometryHolder;
            if (CreatePxGeometryFromConfig(shapeConfiguration, geometryHolder, false))
            {
                physx::PxBounds3 bounds = physx::PxGeometryQuery::getWorldBounds(geometryHolder.any()
                    , physx::PxTransform(PxMathConvert(PhysX::Utils::GetColliderWorldTransform(worldTransform
                        , colliderConfiguration.m_position
                        , colliderConfiguration.m_rotation))));
                aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(PxMathConvert(bounds.minimum))
                    , AZ::Vector3(PxMathConvert(bounds.maximum)));
            }
            else
            {
                // AABB of collider is at the least, just a point at the position of the collider.
                aabb = AZ::Aabb::CreateFromPoint(worldTransform.GetPosition());
            }
            return aabb;
        }

        bool TriggerColliderExists(AZ::EntityId entityId)
        {
            AZ::EBusLogicalResult<bool, AZStd::logical_or<bool>> response(false);
            PhysX::ColliderShapeRequestBus::EventResult(response
                , entityId
                , &PhysX::ColliderShapeRequestBus::Events::IsTrigger);
            return response.value;
        }
    }

    namespace ReflectionUtils
    {
        void ReflectPhysXOnlyApi(AZ::ReflectContext* context)
        {
            ForceRegionBusBehaviorHandler::Reflect(context);
        }

        void ForceRegionBusBehaviorHandler::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<PhysX::ForceRegionNotificationBus>("ForceRegionNotificationBus")
                    ->Handler<ForceRegionBusBehaviorHandler>()
                    ;
            }
        }

        void ForceRegionBusBehaviorHandler::OnCalculateNetForce(AZ::EntityId forceRegionEntityId
            , AZ::EntityId targetEntityId
            , const AZ::Vector3& netForceDirection
            , float netForceMagnitude)
        {
            Call(FN_OnCalculateNetForce
                , forceRegionEntityId
                , targetEntityId
                , netForceDirection
                , netForceMagnitude);
        }
    }
    
    namespace PxActorFactories
    {
        physx::PxRigidDynamic* CreatePxRigidBody(const Physics::RigidBodyConfiguration& configuration)
        {
            physx::PxTransform pxTransform(PxMathConvert(configuration.m_position),
                PxMathConvert(configuration.m_orientation).getNormalized());

            physx::PxRigidDynamic* rigidDynamic = PxGetPhysics().createRigidDynamic(pxTransform);

            if (!rigidDynamic)
            {
                AZ_Error("PhysX Rigid Body", false, "Failed to create PhysX rigid actor. Name: %s", configuration.m_debugName.c_str());
                return nullptr;
            }

            rigidDynamic->setMass(configuration.m_mass);
            rigidDynamic->setSleepThreshold(configuration.m_sleepMinEnergy);
            rigidDynamic->setLinearVelocity(PxMathConvert(configuration.m_initialLinearVelocity));
            rigidDynamic->setAngularVelocity(PxMathConvert(configuration.m_initialAngularVelocity));
            rigidDynamic->setLinearDamping(configuration.m_linearDamping);
            rigidDynamic->setAngularDamping(configuration.m_angularDamping);
            rigidDynamic->setCMassLocalPose(physx::PxTransform(PxMathConvert(configuration.m_centerOfMassOffset)));
            rigidDynamic->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, configuration.m_kinematic);
            return rigidDynamic;
        }

        physx::PxRigidStatic* CreatePxStaticRigidBody(const Physics::WorldBodyConfiguration& configuration)
        {
            physx::PxTransform pxTransform(PxMathConvert(configuration.m_position),
                PxMathConvert(configuration.m_orientation).getNormalized());
            physx::PxRigidStatic* rigidStatic = PxGetPhysics().createRigidStatic(pxTransform);
            return rigidStatic;
        }

        void ReleaseActor(physx::PxActor* actor)
        {
            if (!actor)
            {
                return;
            }

            physx::PxScene* scene = actor->getScene();
            if (scene)
            {
                scene->removeActor(*actor);
            }

            if (auto userData = Utils::GetUserData(actor))
            {
                userData->Invalidate();
            }

            actor->release();
        }
    }
}
