
#include "CloudGemSpeechRecognition_precompiled.h"
#include <platform_impl.h>

#include "CloudGemSpeechRecognitionSystemComponent.h"
#include "VoiceRecorderSystemComponent.h"

#include <IGem.h>

namespace CloudGemSpeechRecognition
{
    class CloudGemSpeechRecognitionModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CloudGemSpeechRecognitionModule, "{B5F7B7ED-07B3-41C9-9E67-5CDBC6B7DF7B}", CryHooksModule);

        CloudGemSpeechRecognitionModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemSpeechRecognitionSystemComponent::CreateDescriptor(),
                VoiceRecorderSystemComponent::CreateDescriptor()
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemSpeechRecognitionSystemComponent>(),
                azrtti_typeid<VoiceRecorderSystemComponent>()
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemSpeechRecognition_6f2e05e82d6f4b76abaa18a70f25d7fe, CloudGemSpeechRecognition::CloudGemSpeechRecognitionModule)
