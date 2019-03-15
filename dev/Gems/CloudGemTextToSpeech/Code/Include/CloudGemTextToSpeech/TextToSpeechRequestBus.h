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