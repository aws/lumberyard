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
#include <AzCore/Component/ComponentApplicationBus.h>

#include <Source/SystemComponent.h>
#include <Source/TerrainComponent.h>
#include <Source/RigidBodyComponent.h>
#include <Source/BaseColliderComponent.h>
#include <Source/MeshColliderComponent.h>
#include <Source/BoxColliderComponent.h>
#include <Source/SphereColliderComponent.h>
#include <Source/CapsuleColliderComponent.h>

#if defined(PHYSX_EDITOR)
#include <Source/EditorSystemComponent.h>
#include <Source/EditorTerrainComponent.h>
#include <Source/EditorRigidBodyComponent.h>
#include <Source/EditorColliderComponent.h>
#include <Source/Pipeline/MeshExporter.h>
#include <Source/Pipeline/MeshBehavior.h>
#include <Source/Pipeline/CgfMeshBuilder/CgfMeshAssetBuilderComponent.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#endif // defined(PHYSX_EDITOR)

namespace PhysX
{
    class Module
        : public AZ::Module
    {
    public:
        AZ_RTTI(PhysX::Module, "{160C59B1-FA68-4CDC-8562-D1204AB78FC1}", AZ::Module);

        Module()
        {
#if defined(PHYSX_EDITOR)
            m_sceneCoreModule = AZ::DynamicModuleHandle::Create("SceneCore");
            m_sceneCoreModule->Load(true);
#endif // defined(PHYSX_EDITOR)

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
#if defined(PHYSX_EDITOR)
                    EditorSystemComponent::CreateDescriptor(),
                    EditorTerrainComponent::CreateDescriptor(),
                    EditorRigidBodyComponent::CreateDescriptor(),
                    EditorColliderComponent::CreateDescriptor(),
                    Pipeline::MeshExporter::CreateDescriptor(),
                    Pipeline::MeshBehavior::CreateDescriptor(),
                    Pipeline::CgfMeshAssetBuilderComponent::CreateDescriptor()
#endif // defined(PHYSX_EDITOR)
                });
        }

        ~Module()
        {
            SystemComponent::DestroyPhysXSDK();
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
#if defined(PHYSX_EDITOR)
        AZStd::unique_ptr<AZ::DynamicModuleHandle> m_sceneCoreModule;
#endif // defined(PHYSX_EDITOR)
    };
} // namespace PhysX

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(PhysX_4e08125824434932a0fe3717259caa47, PhysX::Module)
