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

#include "Precompiled.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "GraphicsReflectContextSystemComponent.h"
#include "GraphicsNodeLibrary.h"

#include <IGem.h>

namespace GraphicsReflectContext
{
    class GraphicsReflectContextModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(GraphicsReflectContextModule, "{53349E30-3BAA-454D-AECA-CE7A91A06576}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(GraphicsReflectContextModule, AZ::SystemAllocator, 0);

        GraphicsReflectContextModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                GraphicsReflectContextSystemComponent::CreateDescriptor(),
            });

            AZStd::vector<AZ::ComponentDescriptor*> componentDescriptors(GraphicsNodeLibrary::GetComponentDescriptors());
            m_descriptors.insert(m_descriptors.end(), componentDescriptors.begin(), componentDescriptors.end());
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<GraphicsReflectContextSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(GraphicsReflectContext_81d5967d0b9b41d89754cce68a20882e, GraphicsReflectContext::GraphicsReflectContextModule)
