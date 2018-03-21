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

# pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <CloudGemFramework/AwsApiRequestJob.h>

#include <PresignedURL/PresignedURLBus.h>
#include "CloudGemTextToSpeech/TextToSpeechRequestBus.h"
#include "CloudGemTextToSpeech/TextToSpeechPlaybackBus.h"
#include "AWS/ServiceAPI/CloudGemTextToSpeechClientComponent.h"

#pragma warning(disable: 4355) // <future> includes ppltasks.h which throws a C4355 warning: 'this' used in base member initializer list
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/json/JsonSerializer.h>
#pragma warning(default: 4355)



namespace CloudGemTextToSpeech
{
    class BehaviorTextToSpeechPlaybackHandler
        : public TextToSpeechPlaybackBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorTextToSpeechPlaybackHandler, "{9efe19cf-1fc6-4081-bc7a-769270f34cb5}", AZ::SystemAllocator
        , PlaySpeech
        , PlayWithLipSync
        );

        void PlaySpeech( AZStd::string voicePath) override;
        void PlayWithLipSync( AZStd::string voicePath, AZStd::string speechMarksPath) override;
    };

    /*
     * This Component takes in text and converts it to audio file and (optionally) a speech marks file
     * it then fires off a TextToSpeechPlaybackBus call to start playback and lipsync
     */
    class TextToSpeech
        : public AZ::Component
        , public CloudCanvas::PresignedURLResultBus::Handler
        , public TextToSpeechRequestBus::Handler
        , public ConversionNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(TextToSpeech, "{32FD5DA8-1B8A-467F-9FA9-159BC3088481}");
        virtual ~TextToSpeech() = default;


        void Init() override;
        void Activate() override;
        void Deactivate() override;
        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                // we must include any fields we want to expose to the editor or lua in the serialize context
                serializeContext->Class<TextToSpeech>()
                    ->Version(1);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<TextToSpeech>("TextToSpeech", "Submit messages for conversion to Text to speech using Polly")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "CloudGemTextToSpeech")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"));
                }
            }

            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->EBus<TextToSpeechRequestBus>("TextToSpeechRequestBus")
                    ->Event("ConvertTextToSpeechWithoutMarks", &TextToSpeechRequestBus::Events::ConvertTextToSpeechWithoutMarks)
                    ->Event("ConvertTextToSpeechWithMarks", &TextToSpeechRequestBus::Events::ConvertTextToSpeechWithMarks)
                    ->Event("GetVoiceFromCharacter", &TextToSpeechRequestBus::Events::GetVoiceFromCharacter)
                    ->Event("GetSpeechMarksFromCharacter", &TextToSpeechRequestBus::Events::GetSpeechMarksFromCharacter)
                    ->Event("GetProsodyTagsFromCharacter", &TextToSpeechRequestBus::Events::GetProsodyTagsFromCharacter)
                    ->Event("GetLanguageOverrideFromCharacter", &TextToSpeechRequestBus::Events::GetLanguageOverrideFromCharacter)
                    ->Event("GetTimbreFromCharacter", &TextToSpeechRequestBus::Events::GetTimbreFromCharacter)
                    ;
                behaviorContext->EBus<TextToSpeechPlaybackBus>("TextToSpeechPlaybackBus")
                    ->Event("PlaySpeech", &TextToSpeechPlaybackBus::Events::PlaySpeech)
                    ->Event("PlayWithLipSync", &TextToSpeechPlaybackBus::Events::PlayWithLipSync)
                    ;

                behaviorContext->EBus<TextToSpeechPlaybackBus>("TextToSpeechPlaybackNotificationBus")
                    ->Handler<BehaviorTextToSpeechPlaybackHandler>()
                    ;
            }
        }


        AZ::JobContext* GetJobContext()
        {
            return m_jobContext;
        }


        // TextToSpeechRequestBus::Handler

        void ConvertTextToSpeechWithoutMarks(const AZStd::string& voice, const AZStd::string& text) override;
        void ConvertTextToSpeechWithMarks(const AZStd::string& voice, const AZStd::string& text, const AZStd::string& speechMarks) override;
        AZStd::string GetVoiceFromCharacter(const AZStd::string& character) override;
        AZStd::string GetSpeechMarksFromCharacter(const AZStd::string& character) override;

        AZStd::vector<AZStd::string> GetProsodyTagsFromCharacter(const AZStd::string& character) override;
        AZStd::string GetLanguageOverrideFromCharacter(const AZStd::string& character) override;
        int GetTimbreFromCharacter(const AZStd::string& character) override;

        // ConversionNotificationBus::Handler
        void GotDownloadUrl(const AZStd::string& hash, const AZStd::string& url, const AZStd::string& speechMarks) override;

        // CloudCanvas::PresignedURLResultBus::Handler
        void GotPresignedURLResult(const AZStd::string& requestURL, int responseCode, const AZStd::string& responseMessage, const AZStd::string& outputFile) override;

    private:
        AZ::JobContext* m_jobContext;
        Aws::Utils::Json::JsonValue LoadCharacterFromMappingsFile(const AZStd::string& character);
        bool HasFilesFromConversion(AZStd::string id);

        class ConversionSet
        {
        public:
            AZ_CLASS_ALLOCATOR(ConversionSet, AZ::SystemAllocator, 0);
            AZStd::string voice;
            AZStd::string marks;
            int size();
        };
        AZStd::string GenerateMD5FromPayload(const char* fileName) const;
        bool GenerateMD5(const char* szName, unsigned char* md5) const;
        AZStd::string ResolvePath(const char* path, bool isDir) const;

        AZStd::string GetAliasedUserCachePath(const AZStd::string& hash) const;

        void ConvertTextToSpeech(const AZStd::string& voice, const AZStd::string& text, const AZStd::string& speechMarks);
        AZStd::string FindCachedVoiceFileExtension(const AZStd::string& dir, const AZStd::string& hash) const;

        class TTSConversionJob : public AZ::Job
        {
        public:
            AZ_CLASS_ALLOCATOR(TTSConversionJob, AZ::SystemAllocator, 0);

            TTSConversionJob(TextToSpeech& component, const AZStd::string& voice, const AZStd::string& message, const AZStd::string& hash, const AZStd::string& speechMarks);
            virtual ~TTSConversionJob() {}

            void Process() override;

        private:
            AZ::EntityId m_entityId;
            ServiceAPI::VoiceRequest m_request;
            ServiceAPI::SpeechMarksRequest m_marksRequest;
            AZStd::string m_hash;
            AZStd::string m_speechMarks;
        };

        AZStd::unordered_map<AZStd::string, AZStd::string> m_urlToConversionId;
        AZStd::unordered_map<AZStd::string, ConversionSet> m_idToUrls;
        AZStd::unordered_map<AZStd::string, ConversionSet> m_idToFiles;
        AZStd::unordered_map<AZStd::string, int> m_conversionIdToNumPending;
        AZStd::unordered_map<AZStd::string, AZStd::string> m_idToSpeechMarksType;
    };
}