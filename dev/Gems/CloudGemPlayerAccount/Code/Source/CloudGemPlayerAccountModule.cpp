
#include "CloudGemPlayerAccount_precompiled.h"
#include <platform_impl.h>

#include "CloudGemPlayerAccountSystemComponent.h"

#include <IGem.h>

namespace CloudGemPlayerAccount
{
    class CloudGemPlayerAccountModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CloudGemPlayerAccountModule, "{2007F458-16CB-4C8D-A68C-75E1885FEDA5}", CryHooksModule);

        CloudGemPlayerAccountModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemPlayerAccountSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemPlayerAccountSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemPlayerAccount_fd4ea4ff80a64bb9a90e55b46e9539ef, CloudGemPlayerAccount::CloudGemPlayerAccountModule)
