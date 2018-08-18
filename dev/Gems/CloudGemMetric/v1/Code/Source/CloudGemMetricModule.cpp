
#include "StdAfx.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "CloudGemMetricSystemComponent.h"

#include <IGem.h>

namespace CloudGemMetric
{
    class CloudGemMetricModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CloudGemMetricModule, "{EF973D14-A8B5-44E3-A630-8792BC94F638}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(CloudGemMetricModule, AZ::SystemAllocator, 0);

        CloudGemMetricModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemMetricSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemMetricSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemMetric_e1bf83aa0ddc49418c6b5689c111cb26, CloudGemMetric::CloudGemMetricModule)
