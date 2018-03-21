
#include "CloudGemTextToSpeech_precompiled.h"
#include <platform_impl.h>

#include "CloudGemTextToSpeechSystemComponent.h"

#include <IGem.h>

namespace CloudGemTextToSpeech
{
    class CloudGemTextToSpeechModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CloudGemTextToSpeechModule, "{10FC2C0F-EA9E-4518-A5F0-AC9191B89520}", CryHooksModule);

        CloudGemTextToSpeechModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemTextToSpeechSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemTextToSpeechSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemTextToSpeech_bf49ab6b1b624e16854dcbaa94b1aa24, CloudGemTextToSpeech::CloudGemTextToSpeechModule)
