/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include "Microphone_precompiled.h"
#include <MicrophoneSystemComponent.h>

namespace Audio
{
// This gets around a redefinition issue on uber builds so we don't have to add all the
// platform-specific waf_files yet.  We can include this file in the main waf_files and
// exclude the definition on Windows because there's already a real Windows impl.
// When we add more platform implementations, we can add waf_files for all platforms.
#if !defined(AZ_PLATFORM_WINDOWS)
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Null Implementation of a Microphone
    class MicrophoneSystemComponent::Pimpl
        : protected MicrophoneRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(Pimpl, AZ::SystemAllocator, 0);

        bool InitializeDevice() override
        {
            return true;
        }

        void ShutdownDevice() override
        {
        }

        bool StartSession() override
        {
            return true;
        }

        void EndSession() override
        {
        }

        bool IsCapturing() override
        {
            return false;
        }

        SAudioInputConfig GetFormatConfig() const override
        {
            return m_config;
        }

        AZStd::size_t GetData(void**, AZStd::size_t, const SAudioInputConfig&, bool) override
        {
            return 0;
        }

    private:
        SAudioInputConfig m_config;
    };
#endif // AZ_PLATFORM_*
} // namespace Audio
