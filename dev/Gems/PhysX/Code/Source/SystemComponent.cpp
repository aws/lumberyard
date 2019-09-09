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
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/IO/SystemFile.h>
#include <AzFramework/Physics/Utils.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <PhysX/MeshAsset.h>
#include <PhysX/HeightFieldAsset.h>
#include <Source/SystemComponent.h>
#include <Source/AzPhysXCpuDispatcher.h>
#include <Source/RigidBody.h>
#include <Source/RigidBodyStatic.h>
#include <Source/Utils.h>
#include <Source/Collision.h>
#include <Source/Shape.h>
#include <Source/Joint.h>
#include <Source/SphereColliderComponent.h>
#include <Source/BoxColliderComponent.h>
#include <Source/CapsuleColliderComponent.h>
#include <Source/Pipeline/MeshAssetHandler.h>
#include <Source/Pipeline/HeightFieldAssetHandler.h>
#include <Terrain/Bus/LegacyTerrainBus.h>

#ifdef PHYSX_EDITOR
#include <Source/EditorColliderComponent.h>
#include <Editor/EditorWindow.h>
#include <Editor/PropertyTypes.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#endif

namespace PhysX
{
    SystemComponent::PhysXSDKGlobals SystemComponent::m_physxSDKGlobals;
    static const char* defaultWorldName = "PhysX Default";
    static const char* defaultConfigurationPath = "default.physxconfiguration";

    bool SystemComponent::VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() <= 1)
        {
            int elementIndex = classElement.FindElement(AZ_CRC("PvdTransportType", 0x91e0b21e));

            if (elementIndex >= 0)
            {
                Settings::PvdTransportType pvdTransportTypeValue;
                AZ::SerializeContext::DataElementNode& pvdTransportElement = classElement.GetSubElement(elementIndex);
                pvdTransportElement.GetData<Settings::PvdTransportType>(pvdTransportTypeValue);

                if (pvdTransportTypeValue == static_cast<Settings::PvdTransportType>(2))
                {
                    // version 2 removes Disabled (2) value default to Network instead.
                    bool success = pvdTransportElement.SetData<Settings::PvdTransportType>(context, Settings::PvdTransportType::Network);
                    return success;
                }
            }
        }

        return true;
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        D6JointLimitConfiguration::Reflect(context);
        Pipeline::MeshAssetCookedData::Reflect(context);

        Physics::ReflectionUtils::ReflectPhysicsApi(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Settings::ColliderProximityVisualization>()
                ->Version(1)
                ->Field("Enabled", &Settings::ColliderProximityVisualization::m_enabled)
                ->Field("CameraPosition", &Settings::ColliderProximityVisualization::m_cameraPosition)
                ->Field("Radius", &Settings::ColliderProximityVisualization::m_radius)
            ;

            serialize->Class<Settings>()
                ->Version(2, &VersionConverter)
                ->Field("PvdHost", &Settings::m_pvdHost)
                ->Field("PvdPort", &Settings::m_pvdPort)
                ->Field("PvdTimeout", &Settings::m_pvdTimeoutInMilliseconds)
                ->Field("PvdTransportType", &Settings::m_pvdTransportType)
                ->Field("PvdFileName", &Settings::m_pvdFileName)
                ->Field("PvdAutoConnectMode", &Settings::m_pvdAutoConnectMode)
                ->Field("PvdReconnect", &Settings::m_pvdReconnect)
                ->Field("ColliderProximityVisualization", &Settings::m_colliderProximityVisualization)
            ;

            serialize->Class<EditorConfiguration>()
                ->Version(1)
                ->Field("COMDebugSize", &EditorConfiguration::m_centerOfMassDebugSize)
                ->Field("COMDebugColor", &EditorConfiguration::m_centerOfMassDebugColor);
            ;

            serialize->Class<Configuration>()
                ->Version(1)
                ->Field("Settings", &Configuration::m_settings)
                ->Field("WorldConfiguration", &Configuration::m_worldConfiguration)
                ->Field("CollisionLayers", &Configuration::m_collisionLayers)
                ->Field("CollisionGroups", &Configuration::m_collisionGroups)
                ->Field("EditorConfiguration", &Configuration::m_editorConfiguration)
            ;

            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(1)
                ->Field("Enabled", &SystemComponent::m_enabled)
                ->Field("ConfigurationPath", &SystemComponent::m_configurationPath)
            ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<Settings>("PhysX PVD Settings", "PhysX PVD Settings")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Settings::m_pvdTransportType,
                    "PVD Transport Type", "PVD supports writing to a TCP/IP network socket or to a file.")
                    ->EnumAttribute(Settings::PvdTransportType::Network, "Network")
                    ->EnumAttribute(Settings::PvdTransportType::File, "File")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &Settings::m_pvdHost,
                    "PVD Host", "Host IP address of the PhysX Visual Debugger application")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Settings::IsNetworkDebug)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Settings::m_pvdPort,
                    "PVD Port", "Port of the PhysX Visual Debugger application")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Settings::IsNetworkDebug)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Settings::m_pvdTimeoutInMilliseconds,
                    "PVD Timeout", "Timeout (in milliseconds) used when connecting to the PhysX Visual Debugger application")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Settings::IsNetworkDebug)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Settings::m_pvdFileName,
                    "PVD FileName", "Filename to output PhysX Visual Debugger data.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &Settings::IsFileDebug)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Settings::m_pvdAutoConnectMode,
                    "PVD Auto Connect", "Automatically connect to the PhysX Visual Debugger "
                    "(Requires PhysX Debug gem for Editor and Game modes).")
                    ->EnumAttribute(Settings::PvdAutoConnectMode::Disabled, "Disabled")
                    ->EnumAttribute(Settings::PvdAutoConnectMode::Editor, "Editor")
                    ->EnumAttribute(Settings::PvdAutoConnectMode::Game, "Game")

                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Settings::m_pvdReconnect,
                    "PVD Reconnect", "Reconnect (Disconnect and Connect) when switching between game and edit mode "
                    "(Requires PhysX Debug gem).")
                ;

                ec->Class<EditorConfiguration>("Editor Configuration", "Editor settings for PhysX")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &EditorConfiguration::m_centerOfMassDebugSize,
                    "Debug Draw Center of Mass Size", "The size of the debug draw circle representing the center of mass.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                    ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorConfiguration::m_centerOfMassDebugColor,
                    "Debug Draw Center of Mass Color", "The color of the debug draw circle representing the center of mass.")
                ;

                ec->Class<SystemComponent>("PhysX", "Global PhysX physics configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SystemComponent::m_enabled,
                    "Enabled", "Enables the PhysX system component.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SystemComponent::m_configurationPath,
                    "Configuration Path", "Path to PhysX configuration file relative to asset root")
                ;
            }
        }
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PhysXService", 0x75beae2d));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("PhysXService", 0x75beae2d));
    }

    void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    SystemComponent::SystemComponent()
        : m_enabled(true)
        , m_configurationPath(defaultConfigurationPath)
    {
    }

    // AZ::Component interface implementation
    void SystemComponent::Init()
    {
    }

    void SystemComponent::InitializePhysXSDK()
    {
        // Start PhysX allocator
        PhysXAllocator::Descriptor allocatorDescriptor;
        allocatorDescriptor.m_custom = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();
        AZ::AllocatorInstance<PhysXAllocator>::Create();

        // create PhysX basis
        m_physxSDKGlobals.m_foundation = PxCreateFoundation(PX_FOUNDATION_VERSION, m_physxSDKGlobals.m_azAllocator, m_physxSDKGlobals.m_azErrorCallback);
        m_physxSDKGlobals.m_pvd = PxCreatePvd(*m_physxSDKGlobals.m_foundation);
        m_physxSDKGlobals.m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_physxSDKGlobals.m_foundation, physx::PxTolerancesScale(), true, m_physxSDKGlobals.m_pvd);
        PxInitExtensions(*m_physxSDKGlobals.m_physics, m_physxSDKGlobals.m_pvd);

        // set up cooking for height fields, meshes etc.
        m_physxSDKGlobals.m_cooking =
#ifdef PHYSX_EDITOR
            // choose sensible defaults for cooking params when at edit-time (GetEditTimeCookingParams())
            PxCreateCooking(PX_PHYSICS_VERSION, *m_physxSDKGlobals.m_foundation, GetEditTimeCookingParams());
#else
            // choose sensible defaults for cooking params when at run-time (GetRealTimeCookingParams())
            PxCreateCooking(PX_PHYSICS_VERSION, *m_physxSDKGlobals.m_foundation, GetRealTimeCookingParams());
#endif
    }

    void SystemComponent::DestroyPhysXSDK()
    {
        m_physxSDKGlobals.m_cooking->release();
        m_physxSDKGlobals.m_cooking = nullptr;

        PxCloseExtensions();

        m_physxSDKGlobals.m_physics->release();
        m_physxSDKGlobals.m_physics = nullptr;

        m_physxSDKGlobals.m_pvd->release();
        m_physxSDKGlobals.m_pvd = nullptr;

        m_physxSDKGlobals.m_foundation->release();
        m_physxSDKGlobals.m_foundation = nullptr;

        AZ::AllocatorInstance<PhysXAllocator>::Destroy();
    }

    physx::PxAllocatorCallback* SystemComponent::GetPhysXAllocatorCallback()
    {
        AZ_Assert(m_physxSDKGlobals.m_physics, "Attempting to get the PhysX Allocator before the PhysX SDK has been initialized.");
        return &(m_physxSDKGlobals.m_azAllocator);
    }
    
    physx::PxErrorCallback* SystemComponent::GetPhysXErrorCallback()
    {
        AZ_Assert(m_physxSDKGlobals.m_physics, "Attempting to get the PhysX Error Callback before the PhysX SDK has been initialized.");
        return &(m_physxSDKGlobals.m_azErrorCallback);
    }


    template<typename AssetHandlerT, typename AssetT>
    void RegisterAsset(AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>>& assetHandlers)
    {
        AssetHandlerT* handler = aznew AssetHandlerT();
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, AZ::AzTypeInfo<AssetT>::Uuid());
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, AssetHandlerT::s_assetFileExtension);
        assetHandlers.emplace_back(handler);
    }

    void SystemComponent::Activate()
    {
        if (!m_enabled)
        {
            return;
        }

        m_materialManager.Connect();


        // Assets related work
        auto materialAsset = aznew AzFramework::GenericAssetHandler<Physics::MaterialLibraryAsset>("Physics Material", "Physics", "physmaterial");
        materialAsset->Register();
        m_assetHandlers.emplace_back(materialAsset);

        // Add asset types and extensions to AssetCatalog. Uses "AssetCatalogService".
        RegisterAsset<Pipeline::MeshAssetHandler, Pipeline::MeshAsset>(m_assetHandlers);
        RegisterAsset<Pipeline::HeightFieldAssetHandler, Pipeline::HeightFieldAsset>(m_assetHandlers);

        // Connect to relevant buses
        Physics::SystemRequestBus::Handler::BusConnect();
        PhysX::SystemRequestsBus::Handler::BusConnect();
        PhysX::ConfigurationRequestBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
        Physics::CollisionRequestBus::Handler::BusConnect();

#ifdef PHYSX_EDITOR
        PhysX::Editor::RegisterPropertyTypes();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
#endif

        // Set up CPU dispatcher
        m_cpuDispatcher = AzPhysXCpuDispatcherCreate();

        // Set Physics API constants to PhysX-friendly values
        Physics::DefaultRigidBodyConfiguration::m_computeInertiaTensor = true;
        Physics::DefaultRigidBodyConfiguration::m_sleepMinEnergy = 0.005f;

        LoadConfiguration();
    }

    bool SystemComponent::ConnectToPvd()
    {
        DisconnectFromPvd();

        // Select current PhysX Pvd debug type
        switch (m_configuration.m_settings.m_pvdTransportType)
        {
        case PhysX::Settings::PvdTransportType::File:
        {
            // Use current timestamp in the filename.
            AZ::u64 currentTimeStamp = AZStd::GetTimeUTCMilliSecond() / 1000;

            // Strip any filename used as .pxd2 forced (only .pvd or .px2 valid for PVD version 3.2016.12.21494747)
            AzFramework::StringFunc::Path::StripExtension(m_configuration.m_settings.m_pvdFileName);

            // Create output filename (format: <TimeStamp>-<FileName>.pxd2)
            AZStd::string filename = AZStd::to_string(currentTimeStamp);
            AzFramework::StringFunc::Append(filename, "-");
            AzFramework::StringFunc::Append(filename, m_configuration.m_settings.m_pvdFileName.c_str());
            AzFramework::StringFunc::Append(filename, ".pxd2");

            AZStd::string rootDirectory;
            EBUS_EVENT_RESULT(rootDirectory, AzFramework::ApplicationRequests::Bus, GetAppRoot);

            // Create the full filepath.
            AZStd::string safeFilePath;
            AzFramework::StringFunc::Path::Join
            (
                rootDirectory.c_str(),
                filename.c_str(),
                safeFilePath
            );

            m_pvdTransport = physx::PxDefaultPvdFileTransportCreate(safeFilePath.c_str());
            break;
        }
        case PhysX::Settings::PvdTransportType::Network:
        {
            m_pvdTransport = physx::PxDefaultPvdSocketTransportCreate
                (
                m_configuration.m_settings.m_pvdHost.c_str(),
                m_configuration.m_settings.m_pvdPort,
                m_configuration.m_settings.m_pvdTimeoutInMilliseconds
                );
            break;
        }
        default:
        {
            AZ_Error("PhysX", false, "Invalid PhysX Visual Debugger (PVD) Debug Type used %d.", m_configuration.m_settings.m_pvdTransportType);
            break;
        }
        }

        bool pvdConnectionSuccessful = false;
        if (m_pvdTransport)
        {
            pvdConnectionSuccessful = m_physxSDKGlobals.m_pvd->connect(*m_pvdTransport, physx::PxPvdInstrumentationFlag::eALL);
            if (pvdConnectionSuccessful)
            {
                AZ_Printf("PhysX", "Successfully connected to the PhysX Visual Debugger (PVD).");
            }
            else
            {
                AZ_Printf("PhysX", "Failed to connect to the PhysX Visual Debugger (PVD).");
            }
        }

        return pvdConnectionSuccessful;
    }

    void SystemComponent::DisconnectFromPvd()
    {
        if (m_physxSDKGlobals.m_pvd)
        {
            m_physxSDKGlobals.m_pvd->disconnect();
        }

        if (m_pvdTransport)
        {
            m_pvdTransport->release();
            m_pvdTransport = nullptr;
            AZ_Printf("PhysX", "Successfully disconnected from the PhysX Visual Debugger (PVD).");
        }
    }

    void SystemComponent::UpdateColliderProximityVisualization(bool enabled, const AZ::Vector3& cameraPosition, float radius)
    {
        Settings::ColliderProximityVisualization& colliderProximityVisualization = m_configuration.m_settings.m_colliderProximityVisualization;
        colliderProximityVisualization.m_enabled = enabled;
        colliderProximityVisualization.m_cameraPosition = cameraPosition;
        colliderProximityVisualization.m_radius = radius;

#ifdef PHYSX_EDITOR
        // We update the editor world when the camera position has moved sufficiently from the last updated position.
        if (m_cameraPositionCache.GetDistance(cameraPosition) > radius * 0.5f)
        {
            Physics::EditorWorldBus::Broadcast(&Physics::EditorWorldRequests::MarkEditorWorldDirty);
            m_cameraPositionCache = cameraPosition;
        }
#endif
    }

    void SystemComponent::Deactivate()
    {
#ifdef PHYSX_EDITOR
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
#endif
        Physics::CollisionRequestBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
        PhysX::ConfigurationRequestBus::Handler::BusDisconnect();
        PhysX::SystemRequestsBus::Handler::BusDisconnect();
        Physics::SystemRequestBus::Handler::BusDisconnect();

        // Reset material manager
        m_materialManager.ReleaseAllMaterials();

        m_materialManager.Disconnect();

        delete m_cpuDispatcher;
        m_cpuDispatcher = nullptr;
        m_assetHandlers.clear();
    }

#ifdef PHYSX_EDITOR

    // AztoolsFramework::EditorEvents::Bus::Handler overrides
    void SystemComponent::PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags)
    {
    }

    void SystemComponent::NotifyRegisterViews()
    {
        PhysX::Editor::EditorWindow::RegisterViewClass();
    }
#endif

    // PhysXSystemComponentRequestBus interface implementation
    physx::PxScene* SystemComponent::CreateScene(physx::PxSceneDesc& sceneDesc)
    {
        AZ_Assert(m_cpuDispatcher, "PhysX CPU dispatcher was not created");

        sceneDesc.cpuDispatcher = m_cpuDispatcher;
        return m_physxSDKGlobals.m_physics->createScene(sceneDesc);
    }

    physx::PxConvexMesh* SystemComponent::CreateConvexMesh(const void* vertices, AZ::u32 vertexNum, AZ::u32 vertexStride)
    {
        physx::PxConvexMeshDesc desc;
        desc.points.data = vertices;
        desc.points.count = vertexNum;
        desc.points.stride = vertexStride;
        // we provide points only, therefore the PxConvexFlag::eCOMPUTE_CONVEX flag must be specified
        desc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

        physx::PxConvexMesh* convex = m_physxSDKGlobals.m_cooking->createConvexMesh(desc,
            m_physxSDKGlobals.m_physics->getPhysicsInsertionCallback());
        AZ_Error("PhysX", convex, "Error. Unable to create convex mesh");

        return convex;
    }

    bool SystemComponent::CookConvexMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount)
    {
        physx::PxConvexMeshDesc convexDesc;
        convexDesc.points.count = vertexCount;
        convexDesc.points.stride = sizeof(AZ::Vector3);
        convexDesc.points.data = vertices;
        convexDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

        physx::PxDefaultMemoryOutputStream memoryStream;

        physx::PxConvexMeshCookingResult::Enum convexCookingResultCode = physx::PxConvexMeshCookingResult::eSUCCESS;

        if (m_physxSDKGlobals.m_cooking->cookConvexMesh(convexDesc, memoryStream, &convexCookingResultCode))
        {
            // Write into a file
            Pipeline::MeshAssetCookedData assetFileContent(memoryStream);
            assetFileContent.m_isConvexMesh = true;

            return Utils::WriteCookedMeshToFile(filePath, assetFileContent);
        }

        AZ_Error("PhysX", false, "Error in CookTriangleMeshToFile for %s. PhysX cooking failed: %s", filePath, Utils::ConvexCookingResultToString(convexCookingResultCode));
        return false;
    }

    bool SystemComponent::CookTriangleMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount,
        const AZ::u32* indices, AZ::u32 indexCount)
    {
        AZ_Error("PhysX", indexCount % 3 == 0, "Error in CookTriangleMeshToFile for %s. Index count (%d) should be a multiple of 3.",
            filePath, indexCount);

        // Prepare data
        physx::PxTriangleMeshDesc meshDesc;
        meshDesc.points.count = vertexCount;
        meshDesc.points.stride = sizeof(AZ::Vector3);
        meshDesc.points.data = vertices;

        meshDesc.triangles.count = indexCount / 3;
        meshDesc.triangles.stride = sizeof(AZ::u32) * 3;
        meshDesc.triangles.data = indices;

        // Do PhysX cooking
        physx::PxDefaultMemoryOutputStream memoryStream;

        physx::PxTriangleMeshCookingResult::Enum trimeshCookingResultCode = physx::PxTriangleMeshCookingResult::eSUCCESS;

        if (m_physxSDKGlobals.m_cooking->cookTriangleMesh(meshDesc, memoryStream, &trimeshCookingResultCode))
        {
            Pipeline::MeshAssetCookedData assetFileContent(memoryStream);
            assetFileContent.m_isConvexMesh = false;

            return Utils::WriteCookedMeshToFile(filePath, assetFileContent);
        }

        AZ_Error("PhysX", false, "Error in CookTriangleMeshToFile for %s. PhysX cooking failed: %s", filePath, Utils::TriMeshCookingResultToString(trimeshCookingResultCode));
        return false;
    }

    physx::PxConvexMesh* SystemComponent::CreateConvexMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize)
    {
        physx::PxDefaultMemoryInputData inpStream(reinterpret_cast<physx::PxU8*>(const_cast<void*>(cookedMeshData)), bufferSize);
        return m_physxSDKGlobals.m_physics->createConvexMesh(inpStream);
    }

    physx::PxTriangleMesh* SystemComponent::CreateTriangleMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize)
    {
        physx::PxDefaultMemoryInputData inpStream(reinterpret_cast<physx::PxU8*>(const_cast<void*>(cookedMeshData)), bufferSize);
        return m_physxSDKGlobals.m_physics->createTriangleMesh(inpStream);
    }

    AZStd::shared_ptr<Physics::World> SystemComponent::CreateWorld(AZ::Crc32 id)
    {
        return CreateWorldCustom(id, m_configuration.m_worldConfiguration);
    }

    AZStd::shared_ptr<Physics::World> SystemComponent::CreateWorldCustom(AZ::Crc32 id, const Physics::WorldConfiguration& settings)
    {
        AZStd::shared_ptr<World> physxWorld = AZStd::make_shared<World>(id, settings);
        return physxWorld;
    }

    AZStd::unique_ptr<Physics::RigidBodyStatic> SystemComponent::CreateStaticRigidBody(const Physics::WorldBodyConfiguration& configuration)
    {
        return AZStd::make_unique<PhysX::RigidBodyStatic>(configuration);
    }

    AZStd::unique_ptr<Physics::RigidBody> SystemComponent::CreateRigidBody(const Physics::RigidBodyConfiguration& configuration)
    {
        return AZStd::make_unique<RigidBody>(configuration);
    }

    AZStd::shared_ptr<Physics::Shape> SystemComponent::CreateShape(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& configuration)
    {
        auto shapePtr = AZStd::make_shared<PhysX::Shape>(colliderConfiguration, configuration);

        if (shapePtr->GetPxShape())
        {
            return shapePtr;
        }

        AZ_Error("PhysX", false, "SystemComponent::CreateShape error. Unable to create a shape from configuration.");

        return nullptr;
    }

    AZStd::shared_ptr<Physics::Material> SystemComponent::CreateMaterial(const Physics::MaterialConfiguration& materialConfiguration)
    {
        return AZStd::make_shared<PhysX::Material>(materialConfiguration);
    }

    AZStd::vector<AZStd::shared_ptr<Physics::Material>> SystemComponent::CreateMaterialsFromLibrary(const Physics::MaterialSelection& materialSelection)
    {
        AZStd::vector<physx::PxMaterial*> pxMaterials;
        m_materialManager.GetPxMaterials(materialSelection, pxMaterials);

        AZStd::vector<AZStd::shared_ptr<Physics::Material>> genericMaterials;
        genericMaterials.reserve(pxMaterials.size());

        for (physx::PxMaterial* pxMaterial : pxMaterials)
        {
            genericMaterials.push_back(static_cast<PhysX::Material*>(pxMaterial->userData)->shared_from_this());
        }

        return genericMaterials;
    }

    AZStd::shared_ptr<Physics::Material> SystemComponent::GetDefaultMaterial()
    {
        return m_materialManager.GetDefaultMaterial();
    }

    AZStd::vector<AZ::TypeId> SystemComponent::GetSupportedJointTypes()
    {
        return JointUtils::GetSupportedJointTypes();
    }

    AZStd::shared_ptr<Physics::JointLimitConfiguration> SystemComponent::CreateJointLimitConfiguration(AZ::TypeId jointType)
    {
        return JointUtils::CreateJointLimitConfiguration(jointType);
    }

    AZStd::shared_ptr<Physics::Joint> SystemComponent::CreateJoint(const AZStd::shared_ptr<Physics::JointLimitConfiguration>& configuration,
        Physics::WorldBody* parentBody, Physics::WorldBody* childBody)
    {
        return JointUtils::CreateJoint(configuration, parentBody, childBody);
    }

    void SystemComponent::GenerateJointLimitVisualizationData(
        const Physics::JointLimitConfiguration& configuration,
        const AZ::Quaternion& parentRotation,
        const AZ::Quaternion& childRotation,
        float scale,
        AZ::u32 angularSubdivisions,
        AZ::u32 radialSubdivisions,
        AZStd::vector<AZ::Vector3>& vertexBufferOut,
        AZStd::vector<AZ::u32>& indexBufferOut,
        AZStd::vector<AZ::Vector3>& lineBufferOut,
        AZStd::vector<bool>& lineValidityBufferOut)
    {
        JointUtils::GenerateJointLimitVisualizationData(configuration, parentRotation, childRotation, scale,
            angularSubdivisions, radialSubdivisions, vertexBufferOut, indexBufferOut, lineBufferOut, lineValidityBufferOut);
    }

    AZStd::unique_ptr<Physics::JointLimitConfiguration> SystemComponent::ComputeInitialJointLimitConfiguration(
        const AZ::TypeId& jointLimitTypeId,
        const AZ::Quaternion& parentWorldRotation,
        const AZ::Quaternion& childWorldRotation,
        const AZ::Vector3& axis,
        const AZStd::vector<AZ::Quaternion>& exampleLocalRotations)
    {
        return JointUtils::ComputeInitialJointLimitConfiguration(jointLimitTypeId, parentWorldRotation,
            childWorldRotation, axis, exampleLocalRotations);
    }

    Configuration SystemComponent::CreateDefaultConfiguration() const
    {
        Configuration configuration;
        configuration.m_collisionLayers.SetName(Physics::CollisionLayer::Default, "Default");
        configuration.m_collisionLayers.SetName(Physics::CollisionLayer::TouchBend, "TouchBend");

        configuration.m_collisionGroups.CreateGroup("All", Physics::CollisionGroup::All, Physics::CollisionGroups::Id(), true);
        configuration.m_collisionGroups.CreateGroup("None", Physics::CollisionGroup::None, Physics::CollisionGroups::Id::Create(), true);
        configuration.m_collisionGroups.CreateGroup("All_NoTouchBend", Physics::CollisionGroup::All_NoTouchBend, Physics::CollisionGroups::Id::Create(), true);

        return configuration;
    }

    void SystemComponent::LoadConfiguration()
    {
        // Load configuration from asset cache
        AZStd::string assetRoot, fullpath;
        EBUS_EVENT_RESULT(assetRoot, AzFramework::ApplicationRequests::Bus, GetAssetRoot);

        AZStd::string fullPath;
        AzFramework::StringFunc::Path::Join(assetRoot.c_str(), m_configurationPath.c_str(), fullPath);

        // Load configuration
        bool loaded = AZ::Utils::LoadObjectFromFileInPlace<Configuration>(fullPath.c_str(), m_configuration);
        if (loaded)
        {
            PhysX::ConfigurationNotificationBus::Broadcast(&PhysX::ConfigurationNotificationBus::Events::OnConfigurationLoaded);
        }
        else
        {
            SetConfiguration(CreateDefaultConfiguration());
        }
    }

    void SystemComponent::SaveConfiguration()
    {
        // Save configuration to source folder when in edit mode.
#ifdef PHYSX_EDITOR
        auto assetRoot = AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@");

        AZStd::string fullPath;
        AzFramework::StringFunc::Path::Join(assetRoot, m_configurationPath.c_str(), fullPath);

        bool saved = AZ::Utils::SaveObjectToFile<Configuration>(fullPath.c_str(), AZ::DataStream::ST_XML, &m_configuration);
        AZ_Warning("PhysXSystemComponent", saved, "Failed to save PhysX configuration");
        if (saved)
        {
            PhysX::ConfigurationNotificationBus::Broadcast(
                &PhysX::ConfigurationNotificationBus::Events::OnConfigurationRefreshed, m_configuration);
        }
#endif
    }

    void SystemComponent::CheckoutConfiguration()
    {
        // Checkout the configuration file so we can write to it later on
#ifdef PHYSX_EDITOR
        using AzToolsFramework::SourceControlFileInfo;
        using AzToolsFramework::SourceControlCommandBus;

        auto assetRoot = AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@");

        AZStd::string fullPath;
        AzFramework::StringFunc::Path::Join(assetRoot, m_configurationPath.c_str(), fullPath);

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEdit,
            fullPath.c_str(), true,
            [fullPath, this](bool /*success*/, const AzToolsFramework::SourceControlFileInfo& info)
        {
            // File is checked out
        });
#endif
    }

    void SystemComponent::AddColliderComponentToEntity(AZ::Entity* entity, const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration, bool addEditorComponents)
    {
        Physics::ShapeType shapeType = shapeConfiguration.GetShapeType();

#ifdef PHYSX_EDITOR
        if (addEditorComponents)
        {
            entity->CreateComponent<EditorColliderComponent>(colliderConfiguration, shapeConfiguration);
        }
        else
#else
        {
            if (shapeType == Physics::ShapeType::Sphere)
            {
                const Physics::SphereShapeConfiguration& sphereConfiguration = static_cast<const Physics::SphereShapeConfiguration&>(shapeConfiguration);
                entity->CreateComponent<SphereColliderComponent>(colliderConfiguration, sphereConfiguration);
            }
            else if (shapeType == Physics::ShapeType::Box)
            {
                const Physics::BoxShapeConfiguration& boxConfiguration = static_cast<const Physics::BoxShapeConfiguration&>(shapeConfiguration);
                entity->CreateComponent<BoxColliderComponent>(colliderConfiguration, boxConfiguration);
            }
            else if (shapeType == Physics::ShapeType::Capsule)
            {
                const Physics::CapsuleShapeConfiguration& capsuleConfiguration = static_cast<const Physics::CapsuleShapeConfiguration&>(shapeConfiguration);
                entity->CreateComponent<CapsuleColliderComponent>(colliderConfiguration, capsuleConfiguration);
            }
        }

        AZ_Error("PhysX System", !addEditorComponents, "AddColliderComponentToEntity(): Trying to add an Editor collider component in a stand alone build.",
            static_cast<AZ::u8>(shapeType));

#endif
        {
            AZ_Error("PhysX System", shapeType == Physics::ShapeType::Sphere || shapeType == Physics::ShapeType::Box || shapeType == Physics::ShapeType::Capsule,
                "AddColliderComponentToEntity(): Using Shape of type %d is not implemented.", static_cast<AZ::u8>(shapeType));
        }
    }

    const Configuration& SystemComponent::GetConfiguration()
    {
        return m_configuration;
    }

    void SystemComponent::SetConfiguration(const Configuration& configuration)
    {
        const bool gravityChanged =
            m_configuration.m_worldConfiguration.m_gravity != configuration.m_worldConfiguration.m_gravity;

        m_configuration = configuration;

        if (gravityChanged)
        {
            Physics::WorldRequestBus::Broadcast(
                &Physics::WorldRequests::SetGravity, m_configuration.m_worldConfiguration.m_gravity);
        }

        SaveConfiguration();
    }

    void SystemComponent::OnCrySystemInitialized(ISystem&, const SSystemInitParams&)
    {
        // Configuration is loaded after cry system is initialised as this is when filesystem
        // is ready. It can't be loaded during Activate() as the asset root hasn't been
        // set at this time.
        LoadConfiguration();
    }

    void SystemComponent::OnCryEditorInitialized()
    {
        // Checkout configuration at startup ahead of time.
        CheckoutConfiguration();
    }

    Physics::CollisionLayer SystemComponent::GetCollisionLayerByName(const AZStd::string& layerName)
    {
        return m_configuration.m_collisionLayers.GetLayer(layerName);
    }

    Physics::CollisionGroup SystemComponent::GetCollisionGroupByName(const AZStd::string& groupName)
    {
        return m_configuration.m_collisionGroups.FindGroupByName(groupName);
    }

    Physics::CollisionGroup SystemComponent::GetCollisionGroupById(const Physics::CollisionGroups::Id& groupId)
    {
        return m_configuration.m_collisionGroups.FindGroupById(groupId);
    }

    physx::PxFilterData SystemComponent::CreateFilterData(const Physics::CollisionLayer& layer, const Physics::CollisionGroup& group)
    {
        return PhysX::Collision::CreateFilterData(layer, group);
    }

    AZStd::shared_ptr<Physics::Shape> SystemComponent::CreateWrappedNativeShape(physx::PxShape* nativeShape)
    {
        return AZStd::make_shared<Shape>(nativeShape);
    }

    physx::PxCooking* SystemComponent::GetCooking()
    {
        return m_physxSDKGlobals.m_cooking;
    }

    physx::PxControllerManager* SystemComponent::CreateControllerManager(physx::PxScene* scene)
    {
        return PxCreateControllerManager(*scene);
    }

    void SystemComponent::ReleaseControllerManager(physx::PxControllerManager* controllerManager)
    {
        controllerManager->release();
    }

    physx::PxCookingParams GetRealTimeCookingParams()
    {
        physx::PxCookingParams params { physx::PxTolerancesScale() };
        // disable mesh cleaning - perform mesh validation on development configurations
        params.meshPreprocessParams |= physx::PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
        // disable edge pre-compute, edges are set for each triangle, slows contact generation
        params.meshPreprocessParams |= physx::PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
        // lower hierarchy for internal mesh
        params.meshCookingHint = physx::PxMeshCookingHint::eCOOKING_PERFORMANCE;

        return params;
    }

    physx::PxCookingParams GetEditTimeCookingParams()
    {
        physx::PxCookingParams params { physx::PxTolerancesScale() };
        // when set, mesh welding is performed - clean mesh must be enabled
        params.meshPreprocessParams |= physx::PxMeshPreprocessingFlag::eWELD_VERTICES;
        // note: default value in PxCookingParams is 0.0f;
        const float physx_CookWeldTolerance = 0.0001f;
        params.meshWeldTolerance = physx_CookWeldTolerance;

        return params;
    }
} // namespace PhysX
