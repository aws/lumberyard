
#include "CloudGemMessageOfTheDay_precompiled.h"
#include <platform_impl.h>

#include "CloudGemMessageOfTheDaySystemComponent.h"

#include <IGem.h>

namespace CloudGemMessageOfTheDay
{
    class CloudGemMessageOfTheDayModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CloudGemMessageOfTheDayModule, "{797853F3-03BB-4571-A6BD-6616D89BB5E1}", CryHooksModule);

        CloudGemMessageOfTheDayModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemMessageOfTheDaySystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemMessageOfTheDaySystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemMessageOfTheDay_21cba308da7040439f88fc3a45c71e18, CloudGemMessageOfTheDay::CloudGemMessageOfTheDayModule)
