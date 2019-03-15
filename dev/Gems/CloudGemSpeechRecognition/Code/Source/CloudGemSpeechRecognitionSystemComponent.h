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

#pragma once

#include <AzCore/Component/Component.h>

#include <CloudGemSpeechRecognition/CloudGemSpeechRecognitionBus.h>

namespace CloudGemSpeechRecognition
{
    class VoiceRecorderSystemComponent;

    class CloudGemSpeechRecognitionSystemComponent
        : public AZ::Component
        , protected SpeechRecognitionRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CloudGemSpeechRecognitionSystemComponent, "{498454B8-1903-4998-9821-6ACDCF93E552}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        void BeginSpeechCapture() override;
        void EndSpeechCaptureAndCallBot(
            const AZStd::string& botName, 
            const AZStd::string& botAlias, 
            const AZStd::string& userId,
            const AZStd::string& sessionAttributes) override;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CloudGemSpeechRecognitionRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    
        bool m_isRecording = false;
    };
}
