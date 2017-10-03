#pragma once
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>

namespace CloudGemTextToSpeech
{
    class TextToSpeechPlaybackRequests
        : public AZ::ComponentBus
    {
    public:
        static const bool EnableEventQueue = true;

        virtual ~TextToSpeechPlaybackRequests() {}
        virtual void PlaySpeech( AZStd::string voicePath) = 0;
        virtual void PlayWithLipSync( AZStd::string voicePath, AZStd::string speechMarksPath) = 0;
    };

    using TextToSpeechPlaybackBus = AZ::EBus<TextToSpeechPlaybackRequests>;
}