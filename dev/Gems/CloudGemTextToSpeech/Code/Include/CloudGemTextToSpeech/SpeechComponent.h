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
#include <AZCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <IAudioInterfacesCommonData.h>

#include "CloudGemTextToSpeech/TextToSpeechPlaybackBus.h"
#include "CloudGemTextToSpeech/TextToSpeechClosedCaptionBus.h"


namespace CloudGemTextToSpeech {

    class VisemeAnimation
    {
    public:
        AZ_TYPE_INFO(VisemeAnimation, "{08AAF296-1290-4022-A72E-EBA102202AE2}")
        AZ_CLASS_ALLOCATOR(VisemeAnimation, AZ::SystemAllocator, 0);

        VisemeAnimation(const AZStd::string& animationName, bool alwaysPlay = true);
        VisemeAnimation() {}

        static void Reflect(AZ::ReflectContext* context);

        const AZStd::string& GetAnimationName() const
        {
            return m_animationName;
        }

        bool ShouldAlwaysPlay() const
        {
            return m_alwaysPlay;
        }

    private:
        AZStd::string m_animationName;
        bool m_alwaysPlay = false;
    };

    class VisemeAnimationSet
    {
    public:
        AZ_TYPE_INFO(VisemeAnimationSet, "{A7D090EC-2BCF-470F-8A44-E3D6437CA3A9}")
        AZ_CLASS_ALLOCATOR(VisemeAnimationSet, AZ::SystemAllocator, 0);

        VisemeAnimationSet() {}
        static void Reflect(AZ::ReflectContext* context);
        VisemeAnimation* GetVisemeByString(const AZStd::string& v);

    private:
        VisemeAnimation m_p;
        VisemeAnimation m_t;
        VisemeAnimation m_S;
        VisemeAnimation m_T;
        VisemeAnimation m_f;
        VisemeAnimation m_k;
        VisemeAnimation m_i;
        VisemeAnimation m_r;
        VisemeAnimation m_s;
        VisemeAnimation m_u;
        VisemeAnimation m_amp;
        VisemeAnimation m_a;
        VisemeAnimation m_e;
        VisemeAnimation m_E;
        VisemeAnimation m_o;
        VisemeAnimation m_O;
        VisemeAnimation m_sil;
    };


    struct SpeechMark
    {
        SpeechMark(const float time, VisemeAnimation* anim)
            : m_time(time),
            m_anim(anim)
        {
        }

        float m_time;
        VisemeAnimation* m_anim;
    };

    //********************************************************************************
    // Closed Captioning
    //********************************************************************************
    class BehaviorTextToSpeechClosedCaptionHandler
        : public TextToSpeechClosedCaptionBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorTextToSpeechClosedCaptionHandler, "{9B7D734D-DA5E-4D3F-A300-8ECB4BD1CDA0}", AZ::SystemAllocator
            , OnNewClosedCaptionSentence
            , OnNewClosedCaptionWord
        );

        void OnNewClosedCaptionSentence(AZStd::string sentence, int startIndex, int endIndex) override;
        void OnNewClosedCaptionWord(AZStd::string word, int startIndex, int endIndex) override;
    };

    struct ClosedCaptionWord
    {
        ClosedCaptionWord(const float time, const int startIndex, const int endIndex, const AZStd::string& word)
            : m_time(time),
            m_startIndex(startIndex),
            m_endIndex(endIndex),
            m_word(word)
        {

        }
        float m_time;
        int m_startIndex;
        int m_endIndex;
        AZStd::string m_word;
    };

    struct ClosedCaptionSentence
    {
        ClosedCaptionSentence(const float time, const int startIndex, const int endIndex, const AZStd::string& sentence)
            : m_time(time),
            m_startIndex(startIndex),
            m_endIndex(endIndex),
            m_sentence(sentence)
        {

        }
        float m_time;
        int m_startIndex;
        int m_endIndex;
        AZStd::string m_sentence;
        AZStd::vector<ClosedCaptionWord> m_words;
        int m_lastRequestedWordIndex = -1;

        ClosedCaptionWord* GetLastRequestedWord();
        ClosedCaptionWord* GetCurrentWord(const float time);
        bool HasReachedLastWord();
        void AddWord(const float time, const int startIndex, const int endIndex, const AZStd::string& word);
    };

    struct ClosedCaption
    {
        AZStd::vector<ClosedCaptionSentence> m_sentences;
        int m_lastRequestedSentenceIndex = -1;

        ClosedCaptionSentence* GetLastRequestedSentence();
        ClosedCaptionSentence* GetCurrentSentence(const float time);
        ClosedCaptionSentence* GetLastAddedSentence();
        bool HasReachedLastSentence();
        inline bool HasSentences() { return (m_sentences.size() > 0); }
        void AddSentence(const float time, const int startIndex, const int endIndex, const AZStd::string& sentence);
        void Clear();
    };

    class SpeechComponent
        : public AZ::Component
        , public TextToSpeechPlaybackBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(SpeechComponent, "{bdfda000-0760-4e97-ad2b-eb26f9e46c50}");

        static void Reflect(AZ::ReflectContext* context);

        // Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // TextToSpeechPlaybackBus::Handler
        void PlaySpeech(AZStd::string voicePath) override;
        void PlayWithLipSync(AZStd::string voicePath,  AZStd::string speechMarksPath) override;
        AZStd::string m_triggerName = "TriggerName";

    private:
        void PlaySpeechAudio(const AZStd::string& voicePath);
        Audio::SAudioInputConfig CreateAudioInputConfig(const AZStd::string& voicePath);
        void PlayLipSync(const AZStd::string& marksPath);
        void ParsePollySpeechMarks(const AZStd::string& filePath);
        void ParsePollySpeechMarksLine(const AZStd::string& line);
        void ClearSpeechMarks();
        void UpdateLipSyncAnimation(const float deltaTime);
        int FindSpeechmarkIndex();
        void UpdateClosedCaptioning();

        // Properties
        VisemeAnimationSet m_visemeSet;
        float m_blendTime = 0.15f;
        float m_audioToAnimDelay = 1.0f;
        float m_maxVisemeHoldTime = 0.2f;
        float m_returnToSilBlendTime = 0.15f;
        bool m_cacheGeneratedLines = true;

        // Playback
        AZStd::vector<SpeechMark> m_speechMarks;
        float m_speechTime = 0.0f;
        int m_currentIndex = -1;
        float m_currentPoseHoldTimer = 0.0f;
        bool m_returnedToSil = false;
        Audio::TAudioControlID m_triggerId;

        //Closed caption
        ClosedCaption m_closedCaption;
    };
};