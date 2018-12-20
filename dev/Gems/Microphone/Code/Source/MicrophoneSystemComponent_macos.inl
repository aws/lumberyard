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

#include "MicrophoneSystemComponent.h"
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace Audio
{

// Wrapper interface for platform specific code
class MicrophoneSystemComponent_Wrapper
{
public:
    virtual ~MicrophoneSystemComponent_Wrapper() {};
    virtual bool InitializeDevice() = 0;
    virtual void ShutdownDevice() = 0;
    virtual bool StartSession() = 0;
    virtual void EndSession() = 0;
    virtual bool IsCapturing() = 0;
    virtual SAudioInputConfig GetFormatConfig() const = 0;
    virtual AZStd::size_t GetData(void** outputData, AZStd::size_t numFrames, const SAudioInputConfig& targetConfig, bool shouldDeinterleave) = 0;
};

class MicrophoneSystemComponent::Pimpl
   : public MicrophoneRequestBus::Handler
{
public:
    AZ_CLASS_ALLOCATOR(Pimpl, AZ::SystemAllocator, 0);

    bool InitializeDevice() override
    {
    	Construct();
    	return m_wrapper->InitializeDevice();
    }

    void ShutdownDevice() override
    {
    	m_wrapper->ShutdownDevice();
    }

    bool StartSession() override
    {
    	return m_wrapper->StartSession();
    }

    void EndSession() override
    {
    	m_wrapper->EndSession();
    }

    bool IsCapturing() override
    {
    	return m_wrapper->IsCapturing();
    }

    SAudioInputConfig GetFormatConfig() const override
    {
    	return m_wrapper->GetFormatConfig();
    }

    AZStd::size_t GetData(void** outputData, AZStd::size_t numFrames, const SAudioInputConfig& targetConfig, bool shouldDeinterleave) override
    {
    	return m_wrapper->GetData(outputData, numFrames, targetConfig, shouldDeinterleave);
    }

    void Construct();

private:
	AZStd::unique_ptr<MicrophoneSystemComponent_Wrapper> m_wrapper;
};


}
