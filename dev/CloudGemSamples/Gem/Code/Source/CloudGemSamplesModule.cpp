
#include "StdAfx.h"
#include <platform_impl.h>

#include "CloudGemSamplesSystemComponent.h"

#include <IGem.h>

namespace LYGame
{
    class CloudGemSamplesModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CloudGemSamplesModule, "{8381F150-4CC9-4115-A1BA-447709E77C62}", CryHooksModule);

        CloudGemSamplesModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemSamplesSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemSamplesSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemSamples_fcbd8e2e64af487fb1faecd2a133a0ce, LYGame::CloudGemSamplesModule)
