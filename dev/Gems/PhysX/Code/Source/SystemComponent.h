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
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <foundation/PxAllocatorCallback.h>
#include <foundation/PxErrorCallback.h>
#include <PhysX/SystemComponentBus.h>
#include <PhysX/ConfigurationBus.h>
#include <Terrain/Bus/LegacyTerrainBus.h>
#include <World.h>
#include <Material.h>

#include <CrySystemBus.h>

#ifdef PHYSX_EDITOR
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#endif

namespace PhysX
{
    /// System allocator to be used for all PhysX gem persistent allocations.
    class PhysXAllocator
        : public AZ::SystemAllocator
    {
        friend class AZ::AllocatorInstance<PhysXAllocator>;

    public:

        AZ_TYPE_INFO(PhysXAllocator, "{C07BA28C-F6AF-4AFA-A45C-6747476DE07F}");

        const char* GetName() const override { return "PhysX System Allocator"; }
        const char* GetDescription() const override { return "PhysX general memory allocator"; }
    };

    /// Implementation of the PhysX memory allocation callback interface using Lumberyard allocator.
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

    /// Implementation of the PhysX error callback interface directing errors to Lumberyard error output.
    class PxAzErrorCallback
        : public physx::PxErrorCallback
    {
    public:
        virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
        {
            switch (code)
            {
            case physx::PxErrorCode::eDEBUG_INFO: [[fallthrough]];
            case physx::PxErrorCode::eNO_ERROR:
                AZ_TracePrintf("PhysX", "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
                break;

            case physx::PxErrorCode::eDEBUG_WARNING: [[fallthrough]];
            case physx::PxErrorCode::ePERF_WARNING:
                AZ_Warning("PhysX", false, "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
                break;

            case physx::PxErrorCode::eINVALID_OPERATION: [[fallthrough]];
            case physx::PxErrorCode::eINTERNAL_ERROR: [[fallthrough]];
            case physx::PxErrorCode::eOUT_OF_MEMORY: [[fallthrough]];
            case physx::PxErrorCode::eABORT:
                AZ_Assert(false, "PhysX - PxErrorCode %i: %s (line %i in %s)", code, message, line, file)
                break;

            case physx::PxErrorCode::eINVALID_PARAMETER: [[fallthrough]];
            default:
                AZ_Error("PhysX", false, "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
                break;
            }           
        }
    };

    /// Implementation of the PhysX profiler callback interface.
    class PxAzProfilerCallback
        : public physx::PxProfilerCallback
    {
    public:

        //! Mark the beginning of a nested profile block.
        //! @eventName Event name. Must be a persistent const char *.
        //! @detached True for cross thread events.
        //! @contextId The context id of this zone. Zones with the same id belong to the same group. 0 is used for no specific group.
        //! @return Returns implementation-specific profiler data for this event.
        void* zoneStart(const char* eventName, bool detached, uint64_t contextId) override;

        //! Mark the end of a nested profile block.
        //! @profilerData The data returned by the corresponding zoneStart call (or NULL if not available)
        //! @eventName The name of the zone ending, must match the corresponding name passed with 'zoneStart'. Must be a persistent const char *.
        //! @detached True for cross thread events. Should match the value passed to zoneStart.
        //! @contextId The context of this zone. Should match the value passed to zoneStart.
        //! Note: eventName plus contextId can be used to uniquely match up start and end of a zone.
        void zoneEnd(void* profilerData, const char* eventName, bool detached, uint64_t contextId) override;
    };

    /// System component for PhysX.
    /// The system component handles underlying tasks such as initialization and shutdown of PhysX, managing a
    /// Lumberyard memory allocator for PhysX allocations, scheduling for PhysX jobs, and connections to the PhysX
    /// Visual Debugger.  It also owns fundamental PhysX objects which manage worlds, rigid bodies, shapes, materials,
    /// constraints etc., and perform cooking (processing assets such as meshes and heightfields ready for use in PhysX).
    class SystemComponent
        : public AZ::Component
        , public AZ::Interface<Physics::CollisionRequests>::Registrar
        , public AZ::Interface<Physics::System>::Registrar
        , public Physics::SystemRequestBus::Handler
        , public PhysX::SystemRequestsBus::Handler
        , public AZ::Interface<ConfigurationRequests>::Registrar
        , public ConfigurationRequestBus::Handler
#ifdef PHYSX_EDITOR
        , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
#endif
        , private CrySystemEventBus::Handler
        , private Physics::CollisionRequestBus::Handler
        , private AZ::Data::AssetBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{85F90819-4D9A-4A77-AB89-68035201F34B}");

        SystemComponent();
        ~SystemComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        static void InitializePhysXSDK();
        static void DestroyPhysXSDK();

    protected:
        SystemComponent(const SystemComponent&) = delete;
        static struct PhysXSDKGlobals
        {
            physx::PxDefaultAllocator m_defaultAllocator;
            physx::PxDefaultErrorCallback m_errorCallback;
            PxAzAllocatorCallback m_azAllocator;
            PxAzErrorCallback m_azErrorCallback;
            physx::PxFoundation* m_foundation = nullptr;
            physx::PxPhysics* m_physics = nullptr;
            physx::PxCooking* m_cooking = nullptr;
            physx::PxPvd* m_pvd = nullptr;
        } m_physxSDKGlobals;

        struct PhysicsConfiguration
        {
            AZ_CLASS_ALLOCATOR(PhysicsConfiguration, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(PhysicsConfiguration, "{C8CFDC4C-93B9-4E81-BCB0-0F70F03FE7F6}");

            Physics::WorldConfiguration m_defaultWorldConfiguration; ///< Configuration the system component is going to be using as default for new worlds, configurable through SystemBus API.
            Physics::CollisionConfiguration m_collisionConfiguration; ///< Collision configuration exposed by the system component in the CollisionBus API.
            AZ::Data::Asset<Physics::MaterialLibraryAsset> m_defaultMaterialLibrary = AZ::Data::AssetLoadBehavior::NoLoad; ///< Material Library exposed by the system component SystemBus API.
        };
        
        physx::PxPvdTransport* m_pvdTransport = nullptr;
        physx::PxCpuDispatcher* m_cpuDispatcher = nullptr;

        // SystemRequestsBus
        const Physics::WorldConfiguration& GetDefaultWorldConfiguration() override;
        const AZ::Data::Asset<Physics::MaterialLibraryAsset>* GetDefaultMaterialLibraryAssetPtr() override;
        physx::PxScene* CreateScene(physx::PxSceneDesc& sceneDesc) override;
        physx::PxConvexMesh* CreateConvexMesh(const void* vertices, AZ::u32 vertexNum, AZ::u32 vertexStride) override; // should we use AZ::Vector3* or physx::PxVec3 here?
        physx::PxConvexMesh* CreateConvexMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize) override;
        physx::PxTriangleMesh* CreateTriangleMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize) override;
        bool ConnectToPvd() override;
        void DisconnectFromPvd() override;
        // Expose shared allocator and error callbacks for other Gems that need to initialize their own instances of the PhysX SDK.
        physx::PxAllocatorCallback* GetPhysXAllocatorCallback() override;
        physx::PxErrorCallback* GetPhysXErrorCallback() override;

        bool CookConvexMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount) override;
        
        bool CookConvexMeshToMemory(const AZ::Vector3* vertices, AZ::u32 vertexCount, AZStd::vector<AZ::u8>& result) override;

        bool CookTriangleMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount,
            const AZ::u32* indices, AZ::u32 indexCount) override;

        bool CookTriangleMeshToMemory(const AZ::Vector3* vertices, AZ::u32 vertexCount,
            const AZ::u32* indices, AZ::u32 indexCount, AZStd::vector<AZ::u8>& result) override;

        void AddColliderComponentToEntity(AZ::Entity* entity, const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration, bool addEditorComponents = false) override;

        physx::PxFilterData CreateFilterData(const Physics::CollisionLayer& layer, const Physics::CollisionGroup& group) override;
        AZStd::shared_ptr<Physics::Shape> CreateWrappedNativeShape(physx::PxShape* nativeShape) override;
        physx::PxCooking* GetCooking() override;
        physx::PxControllerManager* CreateControllerManager(physx::PxScene* scene) override;
        void ReleaseControllerManager(physx::PxControllerManager* controllerManager) override;

        void UpdateColliderProximityVisualization(bool enabled, const AZ::Vector3& cameraPosition, float radius) override;

        // PhysX::ConfigurationRequestBus
        // LUMBERYARD_DEPRECATED(LY-109358)
        /// @deprecated Please use the alternative configuration getters instead.
        void SetConfiguration(const Configuration&) override;
        // LUMBERYARD_DEPRECATED(LY-109358)
        /// @deprecated Please use the alternative configuration setters instead.
        const Configuration& GetConfiguration() override;
        void SetPhysXConfiguration(const PhysXConfiguration&) override;
        const PhysXConfiguration& GetPhysXConfiguration() override;

        // CrySystemEventBus
        void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;
        void OnCryEditorInitialized() override;

        // CollisionRequestBus
        Physics::CollisionLayer GetCollisionLayerByName(const AZStd::string& layerName) override;
        AZStd::string GetCollisionLayerName(const Physics::CollisionLayer& layer) override;
        bool TryGetCollisionLayerByName(const AZStd::string& layerName, Physics::CollisionLayer& layer) override;
        Physics::CollisionGroup GetCollisionGroupByName(const AZStd::string& groupName) override;
        bool TryGetCollisionGroupByName(const AZStd::string& layerName, Physics::CollisionGroup& group) override;
        AZStd::string GetCollisionGroupName(const Physics::CollisionGroup& collisionGroup) override;
        Physics::CollisionGroup GetCollisionGroupById(const Physics::CollisionGroups::Id& groupId) override;
        void SetCollisionLayerName(int index, const AZStd::string& layerName) override;
        void CreateCollisionGroup(const AZStd::string& groupName, const Physics::CollisionGroup& group) override;
        void SetCollisionConfiguration(const Physics::CollisionConfiguration& configuration) override;
        Physics::CollisionConfiguration GetCollisionConfiguration() override;

        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

#ifdef PHYSX_EDITOR

        // AztoolsFramework::EditorEvents::Bus::Handler overrides
        void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override;
        void NotifyRegisterViews() override;
#endif

        // Physics::SystemRequestBus::Handler
        void SetDefaultWorldConfiguration(const Physics::WorldConfiguration& worldConfiguration) override;
        void SetDefaultMaterialLibrary(const AZ::Data::Asset<Physics::MaterialLibraryAsset>& worldConfiguration) override;
        AZStd::shared_ptr<Physics::World> CreateWorld(AZ::Crc32 id) override;
        AZStd::shared_ptr<Physics::World> CreateWorldCustom(AZ::Crc32 id, const Physics::WorldConfiguration& settings) override;
        AZStd::unique_ptr<Physics::RigidBodyStatic> CreateStaticRigidBody(const Physics::WorldBodyConfiguration& configuration) override;
        AZStd::unique_ptr<Physics::RigidBody> CreateRigidBody(const Physics::RigidBodyConfiguration& configuration) override;
        AZStd::shared_ptr<Physics::Shape> CreateShape(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& configuration) override;
        AZStd::shared_ptr<Physics::Material> CreateMaterial(const Physics::MaterialConfiguration& materialConfiguration) override;
        AZStd::shared_ptr<Physics::Material> GetDefaultMaterial() override;
        AZStd::vector<AZStd::shared_ptr<Physics::Material>> CreateMaterialsFromLibrary(const Physics::MaterialSelection& materialSelection) override;

        // AZ::Data::AssetBus::Handler
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        AZStd::vector<AZ::TypeId> GetSupportedJointTypes() override;
        AZStd::shared_ptr<Physics::JointLimitConfiguration> CreateJointLimitConfiguration(AZ::TypeId jointType) override;
        AZStd::shared_ptr<Physics::Joint> CreateJoint(const AZStd::shared_ptr<Physics::JointLimitConfiguration>& configuration,
            Physics::WorldBody* parentBody, Physics::WorldBody* childBody) override;
        void GenerateJointLimitVisualizationData(
            const Physics::JointLimitConfiguration& configuration,
            const AZ::Quaternion& parentRotation,
            const AZ::Quaternion& childRotation,
            float scale,
            AZ::u32 angularSubdivisions,
            AZ::u32 radialSubdivisions,
            AZStd::vector<AZ::Vector3>& vertexBufferOut,
            AZStd::vector<AZ::u32>& indexBufferOut,
            AZStd::vector<AZ::Vector3>& lineBufferOut,
            AZStd::vector<bool>& lineValidityBufferOut) override;
        AZStd::unique_ptr<Physics::JointLimitConfiguration> ComputeInitialJointLimitConfiguration(
            const AZ::TypeId& jointLimitTypeId,
            const AZ::Quaternion& parentWorldRotation,
            const AZ::Quaternion& childWorldRotation,
            const AZ::Vector3& axis,
            const AZStd::vector<AZ::Quaternion>& exampleLocalRotations) override;

        void ReleaseNativeMeshObject(void* nativeMeshObject) override;

        Physics::CollisionConfiguration CreateDefaultCollisionConfiguration() const;
        void LoadConfiguration();
        void SaveConfiguration();
        void CheckoutConfiguration();

        // Assets related data
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
        PhysX::MaterialsManager m_materialManager;

        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        bool LoadDefaultMaterialLibrary() override;

        bool UpdateMaterialSelection(const Physics::ShapeConfiguration& shapeConfiguration,
            Physics::ColliderConfiguration& colliderConfiguration) override;

    private:
        bool UpdateMaterialSelectionFromPhysicsAsset(
            const Physics::PhysicsAssetShapeConfiguration& assetConfiguration,
            Physics::ColliderConfiguration& colliderConfiguration);

        bool m_enabled; ///< If false, this component will not activate itself in the Activate() function.
        AZStd::string m_configurationPath;
        Configuration m_configuration;
        PhysXConfiguration m_physxConfiguration;
        PhysicsConfiguration m_physicsConfiguration;
        AZ::Vector3 m_cameraPositionCache = AZ::Vector3::CreateZero();
        PxAzProfilerCallback m_pxAzProfilerCallback;
    };

    /// Return PxCookingParams better suited for use at run-time, these parameters will improve cooking time.
    /// Reference: https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Geometry.html#triangle-meshes
    physx::PxCookingParams GetRealTimeCookingParams();
    /// Return PxCookingParams better suited for use at edit-time, these parameters will
    /// increase cooking time but improve accuracy/precision.
    physx::PxCookingParams GetEditTimeCookingParams();
} // namespace PhysX
