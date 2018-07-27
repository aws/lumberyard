
#include "CloudGemComputeFarm_precompiled.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "CloudGemComputeFarmSystemComponent.h"

#include <IGem.h>

namespace CloudGemComputeFarm
{
    class CloudGemComputeFarmModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CloudGemComputeFarmModule, "{B1A95129-DE0B-49CE-A592-9BFE89F175BF}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(CloudGemComputeFarmModule, AZ::SystemAllocator, 0);

        CloudGemComputeFarmModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemComputeFarmSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemComputeFarmSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemComputeFarm_99586a182bea4616a6d1d72cce733e8c, CloudGemComputeFarm::CloudGemComputeFarmModule)
