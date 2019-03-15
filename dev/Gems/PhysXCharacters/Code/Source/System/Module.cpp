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

#include <PhysXCharacters_precompiled.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <Components/CharacterControllerComponent.h>
#include <Components/RagdollComponent.h>
#include <System/SystemComponent.h>
#if defined(PHYSX_CHARACTERS_EDITOR)
#include <Components/EditorCharacterControllerComponent.h>
#endif // defined(PHYSX_CHARACTERS_EDITOR)

namespace PhysXCharacters
{
    class Module
        : public AZ::Module
    {
    public:
        AZ_RTTI(PhysXCharacters::Module, "{EAA1CDD6-5099-463A-9104-F2EB1931B430}", AZ::Module);
        AZ_CLASS_ALLOCATOR(PhysXCharacters::Module, AZ::SystemAllocator, 0);

        Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                SystemComponent::CreateDescriptor(),
                RagdollComponent::CreateDescriptor(),
                CharacterControllerComponent::CreateDescriptor(),
#if defined(PHYSX_CHARACTERS_EDITOR)
                EditorCharacterControllerComponent::CreateDescriptor(),
#endif // defined(PHYSX_CHARACTERS_EDITOR)
            });
        }

        /// Add required SystemComponents to the SystemEntity.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<SystemComponent>(),
            };
        }
    };
} // namespace PhysXCharacters

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(PhysXCharacters_50f9ae1e09ac471bbd9d86ca8063ddf9, PhysXCharacters::Module)
