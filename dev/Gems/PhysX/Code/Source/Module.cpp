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

#if defined(PHYSX_WIN)
#include <PhysXSystemComponent.h>
#include <PhysXRigidBodyComponent.h>
#include <PhysXColliderComponent.h>
#include <PhysXMeshShapeComponent.h>
#include <PhysXTriggerAreaComponent.h>
#endif // defined(PHYSX_WIN)

#if defined(PHYSX_EDITOR_WIN)
#include <EditorPhysXRigidBodyComponent.h>
#include <Pipeline/PhysXMeshExporter.h>
#include <Pipeline/PhysXMeshBehavior.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#endif // defined(PHYSX_EDITOR_WIN)

namespace PhysX
{
    class PhysXModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(PhysXModule, "{160C59B1-FA68-4CDC-8562-D1204AB78FC1}", Module);

        PhysXModule()
        {
#if defined(PHYSX_EDITOR_WIN)
            m_sceneCoreModule = AZ::DynamicModuleHandle::Create("SceneCore");
            m_sceneCoreModule->Load(true);
#endif // defined(PHYSX_EDITOR_WIN)
            m_descriptors.insert(m_descriptors.end(), {
#if defined(PHYSX_WIN)
                    PhysXSystemComponent::CreateDescriptor(),
                    PhysXRigidBodyComponent::CreateDescriptor(),
                    PhysXColliderComponent::CreateDescriptor(),
                    PhysXMeshShapeComponent::CreateDescriptor(),
                    PhysXTriggerAreaComponent::CreateDescriptor(),
#endif // defined(PHYSX_WIN)
#if defined(PHYSX_EDITOR_WIN)
                    EditorPhysXRigidBodyComponent::CreateDescriptor(),
                    Pipeline::PhysXMeshExporter::CreateDescriptor(),
                    Pipeline::PhysXMeshBehavior::CreateDescriptor()
#endif // defined(PHYSX_EDITOR_WIN)
                });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
#if defined(PHYSX_WIN)
                azrtti_typeid<PhysXSystemComponent>(),
#endif // defined(PHYSX_WIN)
            };
        }
#if defined(PHYSX_EDITOR_WIN)
        AZStd::unique_ptr<AZ::DynamicModuleHandle> m_sceneCoreModule;
#endif // defined(PHYSX_EDITOR_WIN)
    };
} // namespace PhysX

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(PhysX_4e08125824434932a0fe3717259caa47, PhysX::PhysXModule)
