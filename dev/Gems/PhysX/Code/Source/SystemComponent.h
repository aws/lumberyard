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

#include <PxPhysicsAPI.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Physics/Action.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/ColliderComponentBus.h>
#include <AzFramework/Physics/Ghost.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <foundation/PxAllocatorCallback.h>
#include <foundation/PxErrorCallback.h>
#include <Include/PhysX/PhysXSystemComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <Terrain/Bus/LegacyTerrainBus.h>
#include <PhysXWorld.h>

#ifdef PHYSX_EDITOR
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#endif

// for the PhysX visual debugger
#define PVD_HOST "127.0.0.1"

namespace PhysX
{
    /**
    * System allocator to be used for all PhysX gem persistent allocations.
    */
    class PhysXAllocator
        : public AZ::SystemAllocator
    {
        friend class AZ::AllocatorInstance<PhysXAllocator>;

    public:

        AZ_TYPE_INFO(PhysXAllocator, "{C07BA28C-F6AF-4AFA-A45C-6747476DE07F}");

        const char* GetName() const override { return "PhysX System Allocator"; }
        const char* GetDescription() const override { return "PhysX general memory allocator"; }
    };

    /**
    * Implementation of the PhysX memory allocation callback interface using Lumberyard allocator.
    */
    class PxAzAllocatorCallback
        : public physx::PxAllocatorCallback
    {
        void* allocate(size_t size, const char* typeName, const char* filename, int line)
        {
            // PhysX requires 16-byte alignment
            void* ptr = AZ::AllocatorInstance<PhysXAllocator>::Get().Allocate(size, 16, 0, "PhysX", filename, line);
            AZ_Assert((reinterpret_cast<size_t>(ptr) & 15) == 0, "PhysX requires 16-byte aligned memory allocations.");
            return ptr;
        }

        void deallocate(void* ptr)
        {
            AZ::AllocatorInstance<PhysXAllocator>::Get().DeAllocate(ptr);
        }
    };

    /**
    * Implementation of the PhysX error callback interface directing errors to Lumberyard error output.
    */
    class PxAzErrorCallback
        : public physx::PxErrorCallback
    {
    public:
        virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
        {
            AZ_Error("PhysX", false, "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
        }
    };

    /**
    * Internal bus for communicating PhysX events to PhysX components.
    */
    class EntityPhysXEvents
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual ~EntityPhysXEvents() {}
        virtual void OnPostStep() {}
    };
    using EntityPhysXEventBus = AZ::EBus<EntityPhysXEvents>;

    /**
    * System component for PhysX.
    * The system component handles underlying tasks such as initialization and shutdown of PhysX, managing a Lumberyard memory allocator
    * for PhysX allocations, scheduling for PhysX jobs, and connections to the PhysX Visual Debugger.  It also owns fundamental PhysX
    * objects which manage worlds, rigid bodies, shapes, materials, constraints etc., and perform cooking (processing assets such as
    * meshes and heightfields ready for use in PhysX).
    */
    class PhysXSystemComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public LegacyTerrain::LegacyTerrainNotificationBus::Handler
        , public LegacyTerrain::LegacyTerrainRequestBus::Handler
        , public PhysXSystemRequestBus::Handler
        , public Physics::SystemRequestBus::Handler
#ifdef PHYSX_EDITOR
        , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
#endif
        , private AZ::Data::AssetManagerNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(PhysXSystemComponent, "{85F90819-4D9A-4A77-AB89-68035201F34B}");

        PhysXSystemComponent();
        ~PhysXSystemComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // Workaround for VS2013 - Delete the copy constructor and make it private
        // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
        PhysXSystemComponent(const PhysXSystemComponent&) = delete;

        physx::PxDefaultAllocator       m_defaultAllocator;
        physx::PxDefaultErrorCallback   m_errorCallback;
        PxAzAllocatorCallback           m_azAllocator;
        PxAzErrorCallback               m_azErrorCallback;
        physx::PxFoundation*            m_foundation = nullptr;
        physx::PxPhysics*               m_physics = nullptr;
        physx::PxScene*                 m_scene = nullptr;
        physx::PxMaterial*              m_material = nullptr;
        physx::PxPvdTransport*          m_pvdTransport = nullptr;
        physx::PxPvd*                   m_pvd = nullptr;
        physx::PxCooking*               m_cooking = nullptr;
        AzPhysXCpuDispatcher*           m_cpuDispatcher = nullptr;

        // Terrain data
        AZStd::vector<physx::PxRigidStatic*>   m_terrainTiles;
        AZ::u32                                m_numTerrainTiles = 0;
        AZ::u32                                m_terrainTileSize = 0;

        // PhysXSystemComponentRequestBus interface implementation
        void AddActor(physx::PxActor& actor) override;
        void RemoveActor(physx::PxActor& actor) override;
        physx::PxScene* CreateScene( physx::PxSceneDesc& sceneDesc) override;
        physx::PxConvexMesh* CreateConvexMesh(const void* vertices, AZ::u32 vertexNum, AZ::u32 vertexStride) override; // should we use AZ::Vector3* or physx::PxVec3 here?
        physx::PxConvexMesh* CreateConvexMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize) override;
        physx::PxTriangleMesh* CreateTriangleMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize) override;

        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // LegacyTerrain::LegacyTerrainEventBus::Handler
        void SetNumTiles(const AZ::u32 numTiles, const AZ::u32 tileSize) override;
        void UpdateTile(const AZ::u32 tileX, const AZ::u32 tileY, const AZ::u16* heightMap, const float heightMin, const float heightScale, const AZ::u32 tileSize, const AZ::u32 heightMapUnitSize) override;

#ifdef PHYSX_EDITOR
        // AzToolsFramework::EditorEntityContextNotificationBus implementation
        void OnStartPlayInEditor() override;
        void OnStopPlayInEditor() override;
#endif
        bool GetTerrainHeight(const AZ::u32 x, const AZ::u32 y, float& height) const override;

        // Physics::SystemRequestBus::Handler
        Physics::Ptr<Physics::World> CreateWorldByName(const char* worldName, const Physics::Ptr<Physics::WorldSettings>& settings) override;
        Physics::Ptr<Physics::World> CreateWorldById(AZ::u32 worldId, const Physics::Ptr<Physics::WorldSettings>& settings) override;
        Physics::Ptr<Physics::World> FindWorldByName(const char* worldName) override;
        Physics::Ptr<Physics::World> FindWorldById(AZ::u32 worldId) override;
        Physics::Ptr<Physics::World> GetDefaultWorld() override;
        bool DestroyWorldByName(const char* worldName) override;
        bool DestroyWorldById(AZ::u32 worldId) override;
        Physics::Ptr<Physics::RigidBody> CreateRigidBody(const Physics::Ptr<Physics::RigidBodySettings>& settings) override;
        Physics::Ptr<Physics::Ghost> CreateGhost(const Physics::Ptr<Physics::GhostSettings>& settings) override;
        Physics::Ptr<Physics::Character> CreateCharacter(const Physics::Ptr<Physics::CharacterSettings>& settings) override;
        Physics::Ptr<Physics::Action> CreateBuoyancyAction(const Physics::ActionSettings& settings) override;

        using WorldMap = AZStd::unordered_map<AZ::Crc32, Physics::Ptr<PhysXWorld>>;
        WorldMap m_worlds;
        WorldMap m_worldsToUpdate;

        // Assets related data
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;

    private:
        bool    m_createDefaultWorld;   ///< If true, a default world will be created by the system component.
        bool    m_enabled;              ///< If false, this component will not activate itself in the Activate() function.
    };
} // namespace PhysX
