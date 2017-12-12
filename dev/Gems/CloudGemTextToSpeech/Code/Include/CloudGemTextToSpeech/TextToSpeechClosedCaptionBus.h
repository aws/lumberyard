#pragma once
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>

namespace CloudGemTextToSpeech
{
    class TextToSpeechClosedCaptionEvent
        : public AZ::ComponentBus
    {
    public:
        static const bool EnableEventQueue = true;

        virtual ~TextToSpeechClosedCaptionEvent() {}
        virtual void OnNewClosedCaptionSentence( AZStd::string sentence,  int startIndex,  int endIndex) = 0;
        virtual void OnNewClosedCaptionWord( AZStd::string word, int startIndex, int endIndex) = 0;
    };

    using TextToSpeechClosedCaptionBus = AZ::EBus<TextToSpeechClosedCaptionEvent>;
}