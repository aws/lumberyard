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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <PhysXSystemComponent.h>
#include <AzPhysXCpuDispatcher.h>
#include <PhysXRigidBody.h>
#include <Pipeline/PhysXMeshAsset.h>
#include <Terrain/Bus/LegacyTerrainBus.h>

namespace PhysX
{
    static const char* defaultWorldName = "PhysX Default";

    void PhysXSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PhysXSystemComponent, AZ::Component>()
                ->Version(0)
                ->Field("Enabled", &PhysXSystemComponent::m_enabled)
                ->Field("CreateDefaultWorld", &PhysXSystemComponent::m_createDefaultWorld)
            ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<PhysXSystemComponent>("PhysX", "Global PhysX physics configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PhysXSystemComponent::m_enabled, "Enabled", "Enables the PhysX system component.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PhysXSystemComponent::m_createDefaultWorld, "Create Default World", "Enables creation of a default PhysX world.")
                ;
            }
        }
    }

    void PhysXSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PhysXService", 0x75beae2d));
    }

    void PhysXSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("PhysXService", 0x75beae2d));
    }

    void PhysXSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void PhysXSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    PhysXSystemComponent::PhysXSystemComponent()
        : m_enabled(true)
        , m_createDefaultWorld(true)
    {
    }

    // AZ::Component interface implementation
    void PhysXSystemComponent::Init()
    {
    }

    void PhysXSystemComponent::Activate()
    {
        if (!m_enabled)
        {
            return;
        }

        // Start PhysX allocator
        PhysXAllocator::Descriptor allocatorDescriptor;
        allocatorDescriptor.m_custom = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();
        AZ::AllocatorInstance<PhysXAllocator>::Create();

        // create PhysX basis
        m_foundation = PxCreateFoundation(PX_FOUNDATION_VERSION, m_azAllocator, m_azErrorCallback);
        m_pvdTransport = physx::PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
        m_pvd = PxCreatePvd(*m_foundation);
        m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, physx::PxTolerancesScale(), true, m_pvd);

        // set up cooking for height fields, meshes etc.
        m_cooking = PxCreateCooking(PX_PHYSICS_VERSION, *m_foundation, physx::PxCookingParams(physx::PxTolerancesScale()));

        // create a default material
        m_material = m_physics->createMaterial(0.5f, 0.5f, 0.5f);

        // connect to relevant buses
        Physics::SystemRequestBus::Handler::BusConnect();
        PhysXSystemRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
        LegacyTerrain::LegacyTerrainNotificationBus::Handler::BusConnect();
        LegacyTerrain::LegacyTerrainRequestBus::Handler::BusConnect();
#ifdef PHYSX_EDITOR
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
#endif

        // set up CPU dispatcher
        m_cpuDispatcher = AzPhysXCpuDispatcherCreate();

        // create default world
        if (m_createDefaultWorld)
        {
            Physics::Ptr<Physics::WorldSettings> worldSettings = aznew Physics::WorldSettings();
            worldSettings->m_fixedTimeStep = 1.0f / 60.0f;
            m_scene = static_cast<physx::PxScene*>(CreateWorldByName(defaultWorldName, worldSettings)->GetNativePointer());
        }

        // Assets related work
        m_assetHandlers.emplace_back(aznew Pipeline::PhysXMeshAssetHandler);

        // Add asset types and extensions to AssetCatalog. Uses "AssetCatalogService".
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, AZ::AzTypeInfo<Pipeline::PhysXMeshAsset>::Uuid());
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, Pipeline::PhysXMeshAsset::m_assetFileExtention);
    }

#ifdef PHYSX_EDITOR
    void PhysXSystemComponent::OnStartPlayInEditor()
    {
        // set up visual debugger
        m_pvd->connect(*m_pvdTransport, physx::PxPvdInstrumentationFlag::eALL);

        physx::PxPvdSceneClient* pvdClient = m_scene->getScenePvdClient();
        if (pvdClient)
        {
            pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
            pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
            pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        }
    }

    void PhysXSystemComponent::OnStopPlayInEditor()
    {
        m_pvd->disconnect();
    }
#endif

    void PhysXSystemComponent::Deactivate()
    {
#ifdef PHYSX_EDITOR
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
#endif
        LegacyTerrain::LegacyTerrainRequestBus::Handler::BusDisconnect();
        LegacyTerrain::LegacyTerrainNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        PhysXSystemRequestBus::Handler::BusDisconnect();
        Physics::SystemRequestBus::Handler::BusDisconnect();

        m_terrainTiles.clear();
        m_worlds.clear();

        delete m_cpuDispatcher;
        m_cpuDispatcher = nullptr;

        m_material->release();
        m_cooking->release();
        m_physics->release();
        m_pvd->release();
        m_pvdTransport->release();
        m_foundation->release();
        m_scene = nullptr;
        m_material = nullptr;
        m_cooking = nullptr;
        m_physics = nullptr;
        m_pvd = nullptr;
        m_pvdTransport = nullptr;
        m_foundation = nullptr;

        m_assetHandlers.clear();

        AZ::AllocatorInstance<PhysXAllocator>::Destroy();
    }

    // PhysXSystemComponentRequestBus interface implementation
    void PhysXSystemComponent::AddActor(physx::PxActor& actor)
    {
        m_scene->addActor(actor);
    }

    void PhysXSystemComponent::RemoveActor(physx::PxActor& actor)
    {
        m_scene->removeActor(actor);
    }

    physx::PxScene* PhysXSystemComponent::CreateScene( physx::PxSceneDesc& sceneDesc)
    {
        AZ_Assert(m_cpuDispatcher, "PhysX CPU dispatcher was not created");

        sceneDesc.cpuDispatcher = m_cpuDispatcher;
        return m_physics->createScene(sceneDesc);
    }

    physx::PxConvexMesh* PhysXSystemComponent::CreateConvexMesh(const void* vertices, AZ::u32 vertexNum, AZ::u32 vertexStride)
    {
        physx::PxConvexMeshDesc desc;
        desc.points.data = vertices;
        desc.points.count = vertexNum;
        desc.points.stride = vertexStride;
        desc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX; // We provide points only, therefore the PxConvexFlag::eCOMPUTE_CONVEX flag must be specified

        physx::PxConvexMesh* convex = m_cooking->createConvexMesh(desc, m_physics->getPhysicsInsertionCallback());
        AZ_Error("PhysX", convex, "Error. Unable to create convex mesh");

        return convex;
    }

    physx::PxConvexMesh* PhysXSystemComponent::CreateConvexMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize)
    {
        physx::PxDefaultMemoryInputData inpStream(reinterpret_cast<physx::PxU8*>(const_cast<void*>(cookedMeshData)), bufferSize);
        return m_physics->createConvexMesh(inpStream);
    }

    physx::PxTriangleMesh* PhysXSystemComponent::CreateTriangleMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize)
    {
        physx::PxDefaultMemoryInputData inpStream(reinterpret_cast<physx::PxU8*>(const_cast<void*>(cookedMeshData)), bufferSize);
        return m_physics->createTriangleMesh(inpStream);
    }

    void PhysXSystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        for (auto world_it : m_worlds)
        {
            world_it.second->Update(deltaTime);
        }
    }

    // LegacyTerrainRequestBus
    bool PhysXSystemComponent::GetTerrainHeight(const AZ::u32 x, const AZ::u32 y, float& height) const
    {
        if (m_terrainTileSize == 0 || m_numTerrainTiles == 0)
        {
            AZ_Error("PhysX Terrain", false, "Tried to get terrain height before terrain was populated.");
            return false;
        }

        AZ::u32 xClamped = AZ::GetMin(x, m_numTerrainTiles * m_terrainTileSize - 1);
        AZ::u32 yClamped = AZ::GetMin(y, m_numTerrainTiles * m_terrainTileSize - 1);
        AZ::u32 tileX = xClamped / m_terrainTileSize;
        AZ::u32 tileY = yClamped / m_terrainTileSize;
        AZ::u32 tileIndex = tileX * m_numTerrainTiles + tileY;

        if (tileIndex >= m_terrainTiles.size())
        {
            AZ_Error("PhysX Terrain", false, "Out of bounds terrain actor.");
            return false;
        }

        physx::PxRigidStatic* actor = m_terrainTiles[tileIndex];

        if (!actor || actor->getNbShapes() != 1)
        {
            // It is legitimate to end up here if we ask for the height at co-ordinates which get clamped into the
            // tile we are currently modifying, so don't post an error here.
            return false;
        }

        physx::PxShape* shape;
        actor->getShapes(&shape, 1, 0);

        // Heightfield has an additional row and column of vertices from adjacent tiles to fill gaps between tiles,
        // so expect the heightfield dimensions to be 1 larger than the terrain tile size.
        physx::PxHeightFieldGeometry geometry;
        if (!shape->getHeightFieldGeometry(geometry) ||
            geometry.heightField->getNbRows() != m_terrainTileSize + 1 ||
            geometry.heightField->getNbColumns() != m_terrainTileSize + 1)
        {
            AZ_Error("PhysX Terrain", false, "Shape contains invalid geometry.")
            return false;
        }

        float row = (yClamped % m_terrainTileSize) * geometry.columnScale;
        float col = (xClamped % m_terrainTileSize) * geometry.rowScale;

        height = actor->getGlobalPose().p.z + geometry.heightScale * geometry.heightField->getHeight(row, col);
        return true;
    }

    // LegacyTerrainNotificationBus
    void PhysXSystemComponent::SetNumTiles(const AZ::u32 numTiles, const AZ::u32 tileSize)
    {
        if (m_numTerrainTiles == numTiles && m_terrainTileSize == tileSize)
        {
            return;
        }

        for (AZ::u32 i = 0; i < m_terrainTiles.size(); i++)
        {
            if (m_terrainTiles[i])
            {
                m_terrainTiles[i]->release();
                m_terrainTiles[i] = nullptr;
            }
        }

        m_numTerrainTiles = numTiles;
        m_terrainTileSize = tileSize;
        m_terrainTiles.resize(m_numTerrainTiles * m_numTerrainTiles, nullptr);
    }

    void PhysXSystemComponent::UpdateTile(const AZ::u32 tileX, const AZ::u32 tileY, const AZ::u16* heightMap,
        const float heightMin, const float heightScale, const AZ::u32 tileSize, const AZ::u32 heightMapUnitSize)
    {
        AZ::u32 tileIndex = tileX * m_numTerrainTiles + tileY;

        if (tileX >= m_numTerrainTiles || tileY >= m_numTerrainTiles || tileIndex >= m_terrainTiles.size())
        {
            AZ_Error("PhysX Terrain", false, "Invalid tile co-ordinates (%i, %i) or terrain container size incorrect.  "
                "Tile co-ordinates expected to be <%i", tileX, tileY, m_numTerrainTiles);
            return;
        }

        if (m_terrainTiles[tileIndex])
        {
            m_terrainTiles[tileIndex]->release();
            m_terrainTiles[tileIndex] = nullptr;
        }

        // Boundary vertices along the edges / corner furthest from 0, 0 are shared with neighbouring tiles,
        // or padded if there is no neighbouring tile.
        // Need to check whether the vertices from adjoining tiles will fit into the height range of this tile,
        // otherwise rescaling is required.
        float heightMax = heightMin + heightScale * UINT16_MAX;
        float neighbourMin = heightMin;
        float neighbourMax = heightMax;
        float neighbourHeight = neighbourMin;

        for (AZ::u32 y = 0; y < m_terrainTileSize; y++)
        {
            GetTerrainHeight((tileX + 1) * m_terrainTileSize, tileY * m_terrainTileSize + y, neighbourHeight);
            neighbourMin = AZ::GetMin(neighbourMin, neighbourHeight);
            neighbourMax = AZ::GetMax(neighbourMax, neighbourHeight);
        }

        // This loop is <= rather than < to get the corner vertex.
        for (AZ::u32 x = 0; x <= m_terrainTileSize; x++)
        {
            GetTerrainHeight(tileX * m_terrainTileSize + x, (tileY + 1) * m_terrainTileSize, neighbourHeight);
            neighbourMin = AZ::GetMin(neighbourMin, neighbourHeight);
            neighbourMax = AZ::GetMax(neighbourMax, neighbourHeight);
        }

        bool rescale = (neighbourMin < heightMin || neighbourMax > heightMax);
        float rescaledHeightMin = neighbourMin;
        float rescaledHeightScale = (neighbourMax - neighbourMin) / static_cast<float>(UINT16_MAX);

        AZ::u32 allocationSize = sizeof(physx::PxHeightFieldSample) * (tileSize + 1) * (tileSize + 1);
        physx::PxHeightFieldSample* samples = static_cast<physx::PxHeightFieldSample*>(azmalloc(allocationSize));
        memset(samples, 0, sizeof(physx::PxHeightFieldSample) * (tileSize + 1) * (tileSize + 1));

        float horizontalScale = static_cast<float>(heightMapUnitSize);

        for (AZ::u32 x = 0; x < tileSize + 1; x++)
        {
            for (AZ::u32 y = 0; y < tileSize + 1; y++)
            {
                AZ::u32 xClamped = AZ::GetMin(x, tileSize - 1);
                AZ::u32 yClamped = AZ::GetMin(y, tileSize - 1);
                AZ::u32 pxHeightFieldIndex = y * (tileSize + 1) + x;
                AZ::u32 lyHeightMapIndex = xClamped * (tileSize + 1) + yClamped;
                if (rescale)
                {
                    float height = heightMin + heightScale * heightMap[lyHeightMapIndex];
                    samples[pxHeightFieldIndex].height =
                        static_cast<physx::PxI16>((height - rescaledHeightMin) / rescaledHeightScale + INT16_MIN);
                }
                else
                {
                    samples[pxHeightFieldIndex].height = static_cast<physx::PxI16>(heightMap[lyHeightMapIndex] + INT16_MIN);
                }
                // This parameter in PhysX has 7 bits for a material index and 1 bit indicating the tesselation direction.
                // The bit values here match the tesselation direction from LY.
                // Note that tesselation directions do not appear to be rendered correctly in the PhysX visual debugger.
                samples[pxHeightFieldIndex].materialIndex0 = physx::PxBitAndByte(0, false);
                samples[pxHeightFieldIndex].materialIndex1 = physx::PxBitAndByte(0, false);
            }
        }

        // get boundary heights from adjacent tiles
        for (AZ::u32 i = 0; i <= tileSize; i++)
        {
            if (GetTerrainHeight((tileX + 1) * m_terrainTileSize, tileY * m_terrainTileSize + i, neighbourHeight))
            {
                samples[(tileSize + 1) * (i + 1) - 1].height =
                    static_cast<physx::PxI16>((neighbourHeight - heightMin) / heightScale + INT16_MIN);
            }
            if (GetTerrainHeight((tileX) * m_terrainTileSize + i, (tileY + 1) * m_terrainTileSize, neighbourHeight))
            {
                samples[tileSize * (tileSize + 1) + i].height =
                    static_cast<physx::PxI16>((neighbourHeight - heightMin) / heightScale + INT16_MIN);
            }
        }

        physx::PxHeightFieldDesc heightFieldDesc;
        heightFieldDesc.format = physx::PxHeightFieldFormat::eS16_TM;
        heightFieldDesc.nbColumns = tileSize + 1;
        heightFieldDesc.nbRows = tileSize + 1;
        heightFieldDesc.samples.stride = sizeof(physx::PxHeightFieldSample);
        heightFieldDesc.samples.data = samples;

        physx::PxHeightField* heightField = m_cooking->createHeightField(heightFieldDesc,
                m_physics->getPhysicsInsertionCallback());
        physx::PxHeightFieldGeometry heightFieldGeom(heightField, physx::PxMeshGeometryFlags(),
            rescale ? rescaledHeightScale : heightScale, horizontalScale, horizontalScale);

        // Don't have material editing yet, so just use a default material.
        auto heightFieldShape = m_physics->createShape(heightFieldGeom, &m_material, 1);

        // PhysX creates height fields with the vertical direction along +y, so need to rotate.
        float posX = static_cast<float>(tileX * tileSize * heightMapUnitSize);
        float posY = static_cast<float>(tileY * tileSize * heightMapUnitSize);
        float posZ = heightMin - INT16_MIN * heightScale;
        if (rescale)
        {
            posZ = rescaledHeightMin - INT16_MIN * rescaledHeightScale;
        }
        heightFieldShape->setLocalPose(physx::PxTransform(physx::PxQuat(physx::PxHalfPi, physx::PxVec3(1, 0, 0)) *
                physx::PxQuat(physx::PxHalfPi, physx::PxVec3(0, 1, 0))));
        m_terrainTiles[tileIndex] = m_physics->createRigidStatic(physx::PxTransform(physx::PxVec3(posX, posY, posZ)));
        if (!m_terrainTiles[tileIndex])
        {
            AZ_Error("PhysX Terrain", false, "Failed to create terrain actor.");
        }
        else
        {
            m_terrainTiles[tileIndex]->attachShape(*heightFieldShape);
            m_scene->addActor(*m_terrainTiles[tileIndex]);
        }

        heightFieldShape->release();
        heightField->release();
        azfree(samples);
    }

    Physics::Ptr<Physics::World> PhysXSystemComponent::CreateWorldByName(const char* worldName, const Physics::Ptr<Physics::WorldSettings>& settings)
    {
        const AZ::Crc32 worldId(worldName);
        return CreateWorldById(worldId, settings);
    }

    Physics::Ptr<Physics::World> PhysXSystemComponent::CreateWorldById(AZ::u32 worldId, const Physics::Ptr<Physics::WorldSettings>& settings)
    {
        if (FindWorldById(worldId))
        {
            // world with this ID already exists
            return nullptr;
        }

        Physics::Ptr<PhysXWorld> physxWorld = aznew PhysXWorld(worldId, settings);
        m_worlds[worldId] = physxWorld;

        return physxWorld;
    }

    Physics::Ptr<Physics::World> PhysXSystemComponent::FindWorldByName(const char* worldName)
    {
        const AZ::Crc32 worldId(worldName);
        return FindWorldById(worldId);
    }

    Physics::Ptr<Physics::World> PhysXSystemComponent::FindWorldById(AZ::u32 worldId)
    {
        auto worldIter = m_worlds.find(worldId);
        if (worldIter != m_worlds.end())
        {
            return worldIter->second;
        }

        return nullptr;
    }

    Physics::Ptr<Physics::World> PhysXSystemComponent::GetDefaultWorld()
    {
        return FindWorldByName(defaultWorldName);
    }

    bool PhysXSystemComponent::DestroyWorldByName(const char* worldName)
    {
        const AZ::Crc32 worldId(worldName);
        return DestroyWorldById(worldId);
    }

    bool PhysXSystemComponent::DestroyWorldById(AZ::u32 worldId)
    {
        auto worldPtr = FindWorldById(worldId);

        if (worldPtr)
        {
            m_worlds.erase(worldId);
            return true;
        }

        AZ_Error("PhysX System", false, "DestroyWorldById(): Unable to find world");
        return false;
    }

    Physics::Ptr<Physics::RigidBody> PhysXSystemComponent::CreateRigidBody(const Physics::Ptr<Physics::RigidBodySettings>& settings)
    {
        Physics::Ptr<Physics::RigidBody> rigidBody = aznew PhysXRigidBody(settings);
        return rigidBody;
    }

    Physics::Ptr<Physics::Ghost> PhysXSystemComponent::CreateGhost(const Physics::Ptr<Physics::GhostSettings>& settings)
    {
        return Physics::Ptr<Physics::Ghost>();
    }

    Physics::Ptr<Physics::Character> PhysXSystemComponent::CreateCharacter(const Physics::Ptr<Physics::CharacterSettings>& settings)
    {
        return Physics::Ptr<Physics::Character>();
    }

    Physics::Ptr<Physics::Action> PhysXSystemComponent::CreateBuoyancyAction(const Physics::ActionSettings& settings)
    {
        return Physics::Ptr<Physics::Action>();
    }
} // namespace PhysX
