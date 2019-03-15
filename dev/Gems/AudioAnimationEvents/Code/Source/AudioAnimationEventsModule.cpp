/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

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
