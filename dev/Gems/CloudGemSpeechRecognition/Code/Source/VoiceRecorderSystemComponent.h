
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

#include <AzCore/Component/Component.h>
#include <AzCore/RTTI/RTTI.h>
#include <MicrophoneBus.h>

namespace CloudGemSpeechRecognition 
{
    class VoiceRecorderRequests : public AZ::ComponentBus
    {
    public:
        virtual void StartRecording() = 0;
        virtual void StopRecording() = 0;
        virtual bool IsMicrophoneAvailable() = 0;
        virtual AZStd::string GetSoundBufferBase64() = 0;

    };
    using VoiceRecorderRequestBus = AZ::EBus<VoiceRecorderRequests>;

    class VoiceRecorderNotifications : public AZ::EBusTraits
    {
    public:
    };
    using VoiceRecorderNotificationBus = AZ::EBus<VoiceRecorderNotifications>;
    
    class VoiceRecorderSystemComponent 
        : public AZ::Component
        , public VoiceRecorderRequestBus::Handler
    {
    public:
        AZ_COMPONENT(VoiceRecorderSystemComponent, "{D684766D-D126-4746-B570-97AEEF575C08}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        VoiceRecorderSystemComponent();

        void StartRecording();
        void StopRecording();
        bool IsMicrophoneAvailable();
        AZStd::string GetSoundBufferBase64();

    protected:
        VoiceRecorderSystemComponent(const VoiceRecorderSystemComponent&) = delete;
        VoiceRecorderSystemComponent(VoiceRecorderSystemComponent&&) = delete;
        VoiceRecorderSystemComponent& operator=(const VoiceRecorderSystemComponent&) = delete;
        VoiceRecorderSystemComponent& operator=(VoiceRecorderSystemComponent&&) = delete;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        void SetWAVHeader(AZ::u8* buffer, AZ::u32 bufferSize);
        void ReadBuffer();

    public:
        class Implementation
        {
        public:
            static AZStd::unique_ptr<Implementation> Create();

            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator, 0);
            virtual ~Implementation();

            virtual void Init() = 0;
            virtual void Activate() = 0;
            virtual void Deactivate() = 0;
            virtual void ReadBuffer() = 0;
            virtual void StartRecording() = 0;
            virtual void StopRecording() = 0;
            virtual bool IsMicrophoneAvailable() = 0;

        protected:
            AZStd::unique_ptr<AZ::s8> m_currentB64Buffer;
            int m_currentB64BufferSize{ 0 };

            friend VoiceRecorderSystemComponent;
        };

    private:
        AZStd::unique_ptr<Implementation> m_impl;
    };
}

