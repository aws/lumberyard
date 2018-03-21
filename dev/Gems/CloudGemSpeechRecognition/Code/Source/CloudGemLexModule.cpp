
#include "CloudGemSpeechRecognition_precompiled.h"
#include <platform_impl.h>

#include "CloudGemLexSystemComponent.h"
#include "VoiceRecorderSystemComponent.h"

#include <IGem.h>

namespace CloudGemLex
{
    class CloudGemLexModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CloudGemLexModule, "{B5F7B7ED-07B3-41C9-9E67-5CDBC6B7DF7B}", CryHooksModule);

        CloudGemLexModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemLexSystemComponent::CreateDescriptor(),
                VoiceRecorderSystemComponent::CreateDescriptor()
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemLexSystemComponent>(),
                azrtti_typeid<VoiceRecorderSystemComponent>()
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemLex_6f2e05e82d6f4b76abaa18a70f25d7fe, CloudGemLex::CloudGemLexModule)
