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
            case physx::PxErrorCode::eDEBUG_INFO:
            case physx::PxErrorCode::eNO_ERROR:
                AZ_TracePrintf("PhysX", "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
                break;

            case physx::PxErrorCode::eDEBUG_WARNING:
            case physx::PxErrorCode::ePERF_WARNING:
                AZ_Warning("PhysX", false, "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
                break;

            default:
                AZ_Error("PhysX", false, "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
                break;
            }
        }
    };

    /// System component for PhysX.
    /// The system component handles underlying tasks such as initialization and shutdown of PhysX, managing a
    /// Lumberyard memory allocator for PhysX allocations, scheduling for PhysX jobs, and connections to the PhysX
    /// Visual Debugger.  It also owns fundamental PhysX objects which manage worlds, rigid bodies, shapes, materials,
    /// constraints etc., and perform cooking (processing assets such as meshes and heightfields ready for use in PhysX).
    class SystemComponent
        : public AZ::Component
        , public Physics::SystemRequestBus::Handler
        , public PhysX::SystemRequestsBus::Handler
        , public ConfigurationRequestBus::Handler
#ifdef PHYSX_EDITOR
        , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
#endif
        , private CrySystemEventBus::Handler
        , private Physics::CollisionRequestBus::Handler
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
        // Workaround for VS2013 - Delete the copy constructor and make it private
        // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
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

        physx::PxPvdTransport* m_pvdTransport = nullptr;
        AzPhysXCpuDispatcher* m_cpuDispatcher = nullptr;

        // SystemRequestsBus
        physx::PxScene* CreateScene(physx::PxSceneDesc& sceneDesc) override;
        physx::PxConvexMesh* CreateConvexMesh(const void* vertices, AZ::u32 vertexNum, AZ::u32 vertexStride) override; // should we use AZ::Vector3* or physx::PxVec3 here?
        physx::PxConvexMesh* CreateConvexMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize) override;
        physx::PxTriangleMesh* CreateTriangleMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize) override;
        bool ConnectToPvd() override;
        void DisconnectFromPvd() override;

        bool CookConvexMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount) override;
        bool CookTriangleMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount,
            const AZ::u32* indices, AZ::u32 indexCount) override;

        void AddColliderComponentToEntity(AZ::Entity* entity, const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration, bool addEditorComponents = false) override;

        physx::PxFilterData CreateFilterData(const Physics::CollisionLayer& layer, const Physics::CollisionGroup& group) override;
        AZStd::shared_ptr<Physics::Shape> CreateWrappedNativeShape(physx::PxShape* nativeShape) override;
        physx::PxCooking* GetCooking() override;
        physx::PxControllerManager* CreateControllerManager(physx::PxScene* scene) override;
        void ReleaseControllerManager(physx::PxControllerManager* controllerManager) override;

        void UpdateColliderProximityVisualization(bool enabled, const AZ::Vector3& cameraPosition, float radius) override;

        // PhysX::ConfigurationRequestBus
        const Configuration& GetConfiguration() override;
        void SetConfiguration(const Configuration&) override;

        // CrySystemEventBus
        void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;
        void OnCryEditorInitialized() override;

        // CollisionRequestBus
        Physics::CollisionLayer GetCollisionLayerByName(const AZStd::string& layerName) override;
        Physics::CollisionGroup GetCollisionGroupByName(const AZStd::string& groupName) override;
        Physics::CollisionGroup GetCollisionGroupById(const Physics::CollisionGroups::Id& groupId) override;

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
        AZStd::shared_ptr<Physics::World> CreateWorld(AZ::Crc32 id) override;
        AZStd::shared_ptr<Physics::World> CreateWorldCustom(AZ::Crc32 id, const Physics::WorldConfiguration& settings) override;
        AZStd::shared_ptr<Physics::RigidBodyStatic> CreateStaticRigidBody(const Physics::WorldBodyConfiguration& configuration) override;
        AZStd::shared_ptr<Physics::RigidBody> CreateRigidBody(const Physics::RigidBodyConfiguration& configuration) override;
        AZStd::shared_ptr<Physics::Shape> CreateShape(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& configuration) override;
        AZStd::shared_ptr<Physics::Material> CreateMaterial(const Physics::MaterialConfiguration& materialConfiguration) override;
        AZStd::shared_ptr<Physics::Material> GetDefaultMaterial() override;
        AZStd::vector<AZStd::shared_ptr<Physics::Material>> CreateMaterialsFromLibrary(const Physics::MaterialSelection& materialSelection) override;

        AZStd::vector<AZ::TypeId> GetSupportedJointTypes() override;
        AZStd::shared_ptr<Physics::JointLimitConfiguration> CreateJointLimitConfiguration(AZ::TypeId jointType) override;
        AZStd::shared_ptr<Physics::Joint> CreateJoint(const AZStd::shared_ptr<Physics::JointLimitConfiguration>& configuration,
            const AZStd::shared_ptr<Physics::WorldBody>& parentBody, const AZStd::shared_ptr<Physics::WorldBody>& childBody) override;
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

        Configuration CreateDefaultConfiguration() const;
        void LoadConfiguration();
        void SaveConfiguration();
        void CheckoutConfiguration();

        // Assets related data
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
        PhysX::MaterialsManager m_materialManager;

        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

    private:

        bool m_enabled; ///< If false, this component will not activate itself in the Activate() function.
        AZStd::string m_configurationPath;
        Configuration m_configuration;
        AZ::Vector3 m_cameraPositionCache = AZ::Vector3::CreateZero();
    };

    /// Return PxCookingParams better suited for use at run-time, these parameters will improve cooking time.
    /// Reference: https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Geometry.html#triangle-meshes
    physx::PxCookingParams GetRealTimeCookingParams();
    /// Return PxCookingParams better suited for use at edit-time, these parameters will
    /// increase cooking time but improve accuracy/precision.
    physx::PxCookingParams GetEditTimeCookingParams();
} // namespace PhysX
