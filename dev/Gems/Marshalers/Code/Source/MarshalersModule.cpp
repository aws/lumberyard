// Copyright 2017 FragLab Ltd. All rights reserved.

#include "StdAfx.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "MarshalersSystemComponent.h"

#include <IGem.h>

namespace FragLab
{
    namespace Marshalers
    {
        class MarshalersModule
            : public CryHooksModule
        {
        public:
            AZ_RTTI(MarshalersModule, "{9B34DA7E-E3A2-42A0-A186-0308EBF1ECAE}", CryHooksModule);
            AZ_CLASS_ALLOCATOR(MarshalersModule, AZ::SystemAllocator, 0);

            MarshalersModule()
                : CryHooksModule()
            {
                // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
                m_descriptors.insert(m_descriptors.end(), {
                    MarshalersSystemComponent::CreateDescriptor(),
                });
            }

            /**
             * Add required SystemComponents to the SystemEntity.
             */
            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList {
                           azrtti_typeid<MarshalersSystemComponent>(),
                };
            }
        };
    } // namespace Marshalers
}     // namespace FragLab

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Marshalers_7cedf2bfc6684bb8a046350371ca261c, FragLab::Marshalers::MarshalersModule)
