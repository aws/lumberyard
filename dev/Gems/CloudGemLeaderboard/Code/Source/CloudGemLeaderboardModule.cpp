
#include "CloudGemLeaderboard_precompiled.h"
#include <platform_impl.h>

#include "CloudGemLeaderboardSystemComponent.h"

#include <IGem.h>

namespace CloudGemLeaderboard
{
    class CloudGemLeaderboardModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CloudGemLeaderboardModule, "{AD272FCE-0082-42EB-B251-8C6EA063778B}", CryHooksModule);

        CloudGemLeaderboardModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemLeaderboardSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemLeaderboardSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemLeaderboard_a6db8cb7bfd645509f538bcb0ddc5343, CloudGemLeaderboard::CloudGemLeaderboardModule)
