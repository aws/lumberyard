#pragma once
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>

namespace CloudGemTextToSpeech
{
    class TextToSpeechRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~TextToSpeechRequests() {}
        virtual void ConvertTextToSpeechWithoutMarks(const AZStd::string& voice, const AZStd::string& text) = 0;
        virtual void ConvertTextToSpeechWithMarks(const AZStd::string& voice, const AZStd::string& text, const AZStd::string& speechMarks) = 0;
        virtual AZStd::string GetVoiceFromCharacter(const AZStd::string& character) = 0;
        virtual AZStd::string GetSpeechMarksFromCharacter(const AZStd::string& character) = 0;

        virtual AZStd::vector<AZStd::string> GetProsodyTagsFromCharacter(const AZStd::string& character) = 0;
        virtual AZStd::string GetLanguageOverrideFromCharacter(const AZStd::string& character) = 0;
        virtual int GetTimbreFromCharacter(const AZStd::string& character) = 0;
    };

    using TextToSpeechRequestBus = AZ::EBus<TextToSpeechRequests>;


    class ConversionJobNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual ~ConversionJobNotifications() {}
        virtual void GotDownloadUrl(const AZStd::string& hash, const AZStd::string& url, const AZStd::string& speechMarks) = 0;
    };

    using ConversionNotificationBus = AZ::EBus<ConversionJobNotifications>;
}