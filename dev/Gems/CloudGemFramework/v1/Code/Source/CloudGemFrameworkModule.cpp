
#include "StdAfx.h"
#include <platform_impl.h>

#include "CloudGemFrameworkSystemComponent.h"

#include <CloudGemFramework/HttpClientComponent.h>

#include <IGem.h>

namespace CloudGemFramework
{
    class CloudGemFrameworkModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CloudGemFrameworkModule, "{0003F1F0-2E9F-4753-8E65-01E2A5CFBE76}", CryHooksModule);

        CloudGemFrameworkModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemFrameworkSystemComponent::CreateDescriptor(),
                HttpClientComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemFrameworkSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemFramework_6fc787a982184217a5a553ca24676cfa, CloudGemFramework::CloudGemFrameworkModule)
