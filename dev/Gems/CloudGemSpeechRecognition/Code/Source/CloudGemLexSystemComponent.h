
#pragma once

#include <AzCore/Component/Component.h>

#include <CloudGemLex/CloudGemLexBus.h>

namespace CloudGemLex
{
    class VoiceRecorderSystemComponent;

    class CloudGemLexSystemComponent
        : public AZ::Component
        , protected CloudGemLexRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CloudGemLexSystemComponent, "{498454B8-1903-4998-9821-6ACDCF93E552}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        void BeginSpeechCapture();
        void EndSpeechCaptureAndCallBot(
            const AZStd::string& botName, 
            const AZStd::string& botAlias, 
            const AZStd::string& userId,
            const AZStd::string& sessionAttributes);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // CloudGemLexRequestBus interface implementation

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
