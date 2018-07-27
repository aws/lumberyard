
#include "CloudGemDefectReportSample_precompiled.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "CloudGemDefectReportSampleSystemComponent.h"

#include <IGem.h>

namespace CloudGemDefectReportSample
{
    class CloudGemDefectReportSampleModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CloudGemDefectReportSampleModule, "{DF38CA34-7D30-46E1-88CA-2650C8F366C2}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(CloudGemDefectReportSampleModule, AZ::SystemAllocator, 0);

        CloudGemDefectReportSampleModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemDefectReportSampleSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemDefectReportSampleSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemDefectReportSample_99fd2a91d540451fa728e40d2e4dd16f, CloudGemDefectReportSample::CloudGemDefectReportSampleModule)
