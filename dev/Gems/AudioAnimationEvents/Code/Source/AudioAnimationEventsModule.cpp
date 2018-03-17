
#include "AudioAnimationEvents_precompiled.h"
#include <platform_impl.h>

#include "AudioAnimationEventsProxyComponent.h"
#include <IGem.h>

namespace AudioAnimationEvents
{
    class AudioAnimationEventsModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(AudioAnimationEventsModule, "{7717A03C-B31E-47D0-A22C-90A9EA5DB468}", CryHooksModule);

        AudioAnimationEventsModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                AudioAnimationEventsProxyComponent::CreateDescriptor(),
            });
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(AudioAnimationEvents_0549200795a94b7f902727db81d5d6d8, AudioAnimationEvents::AudioAnimationEventsModule)
