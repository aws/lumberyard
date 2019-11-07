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

#include <AzCore/Module/Module.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <Source/SystemComponent.h>
#include <Source/TerrainComponent.h>
#include <Source/RigidBodyComponent.h>
#include <Source/BaseColliderComponent.h>
#include <Source/MeshColliderComponent.h>
#include <Source/BoxColliderComponent.h>
#include <Source/SphereColliderComponent.h>
#include <Source/CapsuleColliderComponent.h>
#include <Source/ForceRegionComponent.h>

#if defined(PHYSX_EDITOR)
#include <Source/EditorSystemComponent.h>
#include <Source/EditorTerrainComponent.h>
#include <Source/EditorRigidBodyComponent.h>
#include <Source/EditorColliderComponent.h>
#include <Source/EditorForceRegionComponent.h>
#include <Source/Pipeline/MeshExporter.h>
#include <Source/Pipeline/MeshBehavior.h>
#include <Source/Pipeline/CgfMeshBuilder/CgfMeshAssetBuilderComponent.h>
#endif // defined(PHYSX_EDITOR)

#include <PhysX_Traits_Platform.h>

namespace PhysX
{
    class Module
        : public AZ::Module
    {
    public:
        AZ_RTTI(PhysX::Module, "{160C59B1-FA68-4CDC-8562-D1204AB78FC1}", AZ::Module);

        Module()
        {
            LoadModules();

            SystemComponent::InitializePhysXSDK();
            m_descriptors.insert(m_descriptors.end(), {
                    SystemComponent::CreateDescriptor(),
                    TerrainComponent::CreateDescriptor(),
                    RigidBodyComponent::CreateDescriptor(),
                    BaseColliderComponent::CreateDescriptor(),
                    MeshColliderComponent::CreateDescriptor(),
                    BoxColliderComponent::CreateDescriptor(),
                    SphereColliderComponent::CreateDescriptor(),
                    CapsuleColliderComponent::CreateDescriptor(),
                    ForceRegionComponent::CreateDescriptor(),
#if defined(PHYSX_EDITOR)
                    EditorSystemComponent::CreateDescriptor(),
                    EditorTerrainComponent::CreateDescriptor(),
                    EditorRigidBodyComponent::CreateDescriptor(),
                    EditorColliderComponent::CreateDescriptor(),
                    EditorForceRegionComponent::CreateDescriptor(),
                    Pipeline::MeshExporter::CreateDescriptor(),
                    Pipeline::MeshBehavior::CreateDescriptor(),
                    Pipeline::CgfMeshAssetBuilderComponent::CreateDescriptor()
#endif // defined(PHYSX_EDITOR)
                });
        }

        virtual ~Module()
        {
            SystemComponent::DestroyPhysXSDK();

            UnloadModules();
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<SystemComponent>()
#if defined(PHYSX_EDITOR)
                , azrtti_typeid<EditorSystemComponent>()
#endif
            };
        }

    private:
        void LoadModules()
        {
            #if defined(PHYSX_EDITOR)
            {
                AZStd::unique_ptr<AZ::DynamicModuleHandle> sceneCoreModule = AZ::DynamicModuleHandle::Create("SceneCore");
                bool ok = sceneCoreModule->Load(true/*isInitializeFunctionRequired*/);
                AZ_Error("PhysX::Module", ok, "Error loading SceneCore module");

                m_modules.push_back(AZStd::move(sceneCoreModule));
            }
            #endif // defined(PHYSX_EDITOR)

            // Load PhysX SDK dynamic libraries when running on a non-monolithic build.
            // The PhysX Gem module was linked with the PhysX SDK dynamic libraries, but
            // some platforms may not detect the dependency when the gem is loaded, so we 
            // may have to load them ourselves.
#if AZ_TRAIT_PHYSX_FORCE_LOAD_MODULES && !defined(AZ_MONOLITHIC_BUILD)
            {
                const AZStd::vector<AZ::OSString> physXModuleNames = { "PhysX", "PhysXCooking", "PhysXFoundation", "PhysXCommon" };
                for (const auto& physXModuleName : physXModuleNames)
                {
                    AZ::OSString modulePathName = AZ_TRAIT_PHYSX_LIB_PATH(physXModuleName);
                    AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::ResolveModulePath, modulePathName);

                    AZStd::unique_ptr<AZ::DynamicModuleHandle> physXModule = AZ::DynamicModuleHandle::Create(modulePathName.c_str());
                    bool ok = physXModule->Load(false/*isInitializeFunctionRequired*/);
                    AZ_Error("PhysX::Module", ok, "Error loading %s module", physXModuleName.c_str());

                    m_modules.push_back(AZStd::move(physXModule));
                }
            }
#endif
        }

        void UnloadModules()
        {
            // Unload modules in reserve order that were loaded
            for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it)
            {
                it->reset();
            }
            m_modules.clear();
        }

        /// Required modules to load/unload when PhysX Gem module is created/destroyed
        AZStd::vector<AZStd::unique_ptr<AZ::DynamicModuleHandle>> m_modules;
    };
} // namespace PhysX

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(PhysX_4e08125824434932a0fe3717259caa47, PhysX::Module)
