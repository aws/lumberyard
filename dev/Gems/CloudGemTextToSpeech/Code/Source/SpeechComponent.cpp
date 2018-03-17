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

#include "CloudGemTextToSpeech_precompiled.h"

#include <AzCore/EBus/EBus.h>

#include <IAudioInterfacesCommonData.h>
#include <IAudioSystem.h>

#include <AzCore/IO/SystemFile.h>
#include <MathConversion.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/document.h>

#include <LmbrCentral/Animation/SimpleAnimationComponentBus.h>
#include <Integration/AnimGraphComponentBus.h>
#include <LmbrCentral/Audio/AudioProxyComponentBus.h>

#include "CloudGemTextToSpeech/SpeechComponent.h"


namespace CloudGemTextToSpeech
{

    void SpeechComponent::Reflect(AZ::ReflectContext* context)
    {
        VisemeAnimation::Reflect(context);
        VisemeAnimationSet::Reflect(context);
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SpeechComponent>()
                ->Version(1)
                ->Field("Wwise Trigger", &SpeechComponent::m_triggerName)
                ->Field("Viseme animation set", &SpeechComponent::m_visemeSet)
                ->Field("Animation blend time", &SpeechComponent::m_blendTime)
                ->Field("Audio to anim delay", &SpeechComponent::m_audioToAnimDelay)
                ->Field("Max viseme hold time", &SpeechComponent::m_maxVisemeHoldTime)
                ->Field("Blend time to sil", &SpeechComponent::m_returnToSilBlendTime)
                ->Field("Cache generated lines", &SpeechComponent::m_cacheGeneratedLines);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<SpeechComponent>("SpeechComponent", "Plays back audio and speech marks from Polly")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "CloudGemTextToSpeech")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpeechComponent::m_triggerName, "Wwise trigger", "Wwise trigger name")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpeechComponent::m_visemeSet, "Viseme Animation Set", "Viseme Animation Set")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpeechComponent::m_blendTime, "Animation blend time [s]", "Blend time between Viseme animations")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpeechComponent::m_audioToAnimDelay, "Audio To Anim delay [s]", "Delay for Animation after starting sound")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpeechComponent::m_maxVisemeHoldTime, "Max viseme hold time [s]", "Time to hold viseme before returning to sil")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpeechComponent::m_returnToSilBlendTime, "Blend time to sil [s]", "Blend time to sil viseme")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SpeechComponent::m_cacheGeneratedLines, "Cache generated speech files locally.", "Save time and data costs by saving previously generated files on the client");
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<TextToSpeechClosedCaptionBus>("TextToSpeechClosedCaptionNotificationBus")
                ->Handler<BehaviorTextToSpeechClosedCaptionHandler>()
                ;
        }
    }

    void SpeechComponent::Init()
    {
        m_triggerId = INVALID_AUDIO_CONTROL_ID;
    }

    void SpeechComponent::Activate()
    {
        TextToSpeechPlaybackBus::Handler::BusConnect(m_entity->GetId());
        AZ::TickBus::Handler::BusConnect();
    }

    void SpeechComponent::Deactivate()
    {
        TextToSpeechPlaybackBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void SpeechComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        if (!m_speechMarks.empty() || m_closedCaption.HasSentences())
        {
            UpdateLipSyncAnimation(deltaTime);
            UpdateClosedCaptioning();
            m_speechTime += deltaTime;
        }
    }

    void SpeechComponent::UpdateLipSyncAnimation(float deltaTime)
    {
        size_t numSpeechMarks = m_speechMarks.size();

        // find where we are (current time + delay)
        int index = FindSpeechmarkIndex(); // returns -1 if past the last index

        if (index < 0) // Speech is finished
        {
            ClearSpeechMarks();
            m_currentPoseHoldTimer = 0;
            m_returnedToSil = false;
        }
        else if (index > m_currentIndex) // New pose
        {
            int intermediateIndex = m_currentIndex + 1;
            while (intermediateIndex < index)
            {
                if (m_speechMarks[intermediateIndex].m_anim->ShouldAlwaysPlay()) // do we need to bounds check this?
                {
                    index = intermediateIndex; // do not skip Visemes tagged "alwaysPlay"
                }
                intermediateIndex++;
            }
            VisemeAnimation* anim = m_speechMarks[index].m_anim;
            PlayAnimation(anim);
            m_currentPoseHoldTimer = 0;
            m_returnedToSil = false;
        }
        else // Already playing this one. Go back to sil if we have been on it too long
        {
            if (m_currentPoseHoldTimer >= m_maxVisemeHoldTime)
            {
                auto silAnim = m_visemeSet.GetVisemeByString("sil");
                if (silAnim && m_speechMarks[m_currentIndex].m_anim != silAnim && !m_returnedToSil)
                {
                    PlayAnimation(silAnim);
                    m_returnedToSil = true;
                }
            }
            m_currentPoseHoldTimer += deltaTime;
        }
        m_currentIndex = index;
    }

    void SpeechComponent::PlayAnimation(VisemeAnimation* anim)
    {
        static const AZ::u32 s_uNobodyHome = static_cast<AZ::u32>(0xFFFFFFFE);
        static const AZ::u32 s_uInvalidParam = static_cast<AZ::u32>(0xFFFFFFFF);
        AZ::u32 parameterIndex = s_uNobodyHome;
        EBUS_EVENT_ID_RESULT(parameterIndex, m_entity->GetId(), EMotionFX::Integration::AnimGraphComponentRequestBus, FindParameterIndex, "visemeIndex");
        if (parameterIndex == s_uNobodyHome)
        {
            AZ_Warning("SpeechComponent", false, "Missing Animation Graph Component");
        }
        else if (parameterIndex == s_uInvalidParam)
        {
            AZ_Warning("SpeechComponent", false, "Parameter index not found for parameter name == visemeIndex");
        }
        else
        {
            EBUS_EVENT_ID(m_entity->GetId(), EMotionFX::Integration::AnimGraphComponentRequestBus, SetParameterFloat, parameterIndex, (float)anim->GetVisemeIndex());
        }
    }

    void SpeechComponent::UpdateClosedCaptioning()
    {
        if (m_closedCaption.HasSentences())
        {
            ClosedCaptionSentence* previousSentence = m_closedCaption.GetLastRequestedSentence();
            ClosedCaptionSentence* currentSentence = m_closedCaption.GetCurrentSentence(m_speechTime);
            if (currentSentence)
            {
                if (currentSentence != previousSentence)
                {
                    //Send Notification
                    EBUS_EVENT_ID(m_entity->GetId(), TextToSpeechClosedCaptionBus, OnNewClosedCaptionSentence, currentSentence->m_sentence, currentSentence->m_startIndex, currentSentence->m_endIndex);
                }
                ClosedCaptionWord* previousWord = currentSentence->GetLastRequestedWord();
                ClosedCaptionWord* currentWord = currentSentence->GetCurrentWord(m_speechTime);
                if (currentWord && currentWord != previousWord)
                {
                    //Send Notification
                    EBUS_EVENT_ID(m_entity->GetId(), TextToSpeechClosedCaptionBus, OnNewClosedCaptionWord, currentWord->m_word, currentWord->m_startIndex, currentWord->m_endIndex);
                    if (m_closedCaption.HasReachedLastSentence() && currentSentence->HasReachedLastWord())
                    {
                        m_closedCaption.Clear();
                    }
                }

            }
        }
    }

    int SpeechComponent::FindSpeechmarkIndex()
    {
        float adjustedTime = m_speechTime - m_audioToAnimDelay;
        int i = m_currentIndex < 0 ? 0 : m_currentIndex;
        while (i < (int)m_speechMarks.size())
        {
            if (m_speechMarks[i].m_time > adjustedTime) // here's our animation
            {
                float timeDiff = m_speechMarks[i].m_time - adjustedTime;
                if (timeDiff > m_blendTime && i > 0) // until we are within blend time of that key, stick to the previous one
                {
                    return i - 1;
                }
                return i;
            }
            i++;
        }
        return -1;
    }

    void SpeechComponent::PlaySpeech(AZStd::string voicePath)
    {
        PlaySpeechAudio(voicePath);
    }

    void SpeechComponent::PlayWithLipSync(AZStd::string voicePath,  AZStd::string speechMarksPath)
    {
        PlaySpeechAudio(voicePath);
        PlayLipSync(speechMarksPath);
    }


    Audio::SAudioInputConfig SpeechComponent::CreateAudioInputConfig(const AZStd::string& voicePath)
    {
        AZStd::string fileExt;
        fileExt = voicePath.substr(voicePath.length() - 4, 4);

        Audio::AudioInputSourceType audioInputType = Audio::AudioInputSourceType::Unsupported;
        if (fileExt.compare(".wav") == 0)
        {
            audioInputType = Audio::AudioInputSourceType::WavFile;
        }
        else if (fileExt.compare(".pcm") == 0)
        {
            audioInputType = Audio::AudioInputSourceType::PcmFile;
        }
        else
        {
            AZ_Warning("CloudGemTextToSpeech", false, "Unsupported file type");
        }

        Audio::SAudioInputConfig audioInputConfig(audioInputType, voicePath.c_str());
        if (audioInputType == Audio::AudioInputSourceType::PcmFile)
        {
            audioInputConfig.m_bitsPerSample = 16;
            audioInputConfig.m_numChannels = 1;
            audioInputConfig.m_sampleRate = 16000;
            audioInputConfig.m_sampleType = Audio::AudioInputSampleType::Int;
        }

        return audioInputConfig;
    }

    void SpeechComponent::PlaySpeechAudio(const AZStd::string& voicePath)
    {
        Audio::SAudioInputConfig audioInputConfig = CreateAudioInputConfig(voicePath);
        Audio::TAudioSourceId sourceId = INVALID_AUDIO_SOURCE_ID;
        Audio::AudioSystemRequestBus::BroadcastResult(sourceId, &Audio::AudioSystemRequestBus::Events::CreateAudioSource, audioInputConfig);
        if (m_triggerId != INVALID_AUDIO_CONTROL_ID)
        {
            EBUS_EVENT_ID(m_entity->GetId(), LmbrCentral::AudioProxyComponentRequestBus, KillTrigger, m_triggerId);
        }

        if (sourceId != INVALID_AUDIO_SOURCE_ID)
        {
            Audio::AudioSystemRequestBus::BroadcastResult(m_triggerId, &Audio::AudioSystemRequestBus::Events::GetAudioTriggerID, m_triggerName.c_str());

            if (m_triggerId != INVALID_AUDIO_CONTROL_ID)
            {
                Audio::SAudioCallBackInfos callbackInfo;
                EBUS_EVENT_ID(m_entity->GetId(), LmbrCentral::AudioProxyComponentRequestBus, ExecuteSourceTrigger, m_triggerId, callbackInfo, sourceId);

                if (!m_cacheGeneratedLines)
                {
                    AZ::IO::SystemFile::Delete(voicePath.c_str());
                }
            }
            else
            {
                Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::DestroyAudioSource, sourceId);
                AZ_Warning("CloudGemTextToSpeech", false, "Failed to get valid Trigger ID");
            }
        }
        else
        {
            AZ_Warning("CloudGemTextToSpeech", false, "Failed to create valid audio source");
        }
    }

    void SpeechComponent::PlayLipSync(const AZStd::string& marksPath)
    {
        ParsePollySpeechMarks(marksPath);
    }


    void SpeechComponent::ParsePollySpeechMarks(const AZStd::string& filePath)
    {
        AZ::IO::SystemFile pFile;
        if (!pFile.Open(filePath.data(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
        {
            AZ_Warning("CloudGemTextToSpeech", false, "Failed to load speech marks");
            return;
        }
        size_t fileSize = pFile.Length();
        if (fileSize > 0)
        {
            ClearSpeechMarks();

            AZStd::string fileBuf;
            fileBuf.resize(fileSize);

            size_t read = pFile.Read(fileSize, fileBuf.data());
            pFile.Close();

            static const AZStd::string c_strSpace(" ");
            static const AZStd::string c_strEOL("\n");
            AZStd::size_t cursor = 0;
            while (cursor < fileSize)
            {
                AZStd::size_t eolPos = fileBuf.find(c_strEOL, cursor);
                if (eolPos == AZStd::string::npos)
                {
                    break;
                }
                AZStd::size_t bufLength = eolPos - cursor;
                AZStd::string line = fileBuf.substr(cursor, bufLength);
                cursor = (eolPos + 1);
                ParsePollySpeechMarksLine(line);
            }
        }
        if (!m_cacheGeneratedLines)
        {
            AZ::IO::SystemFile::Delete(filePath.c_str());
        }
    }


    void SpeechComponent::ParsePollySpeechMarksLine(const AZStd::string& line)
    {
        rapidjson::Document parseDoc;
        parseDoc.Parse<rapidjson::kParseStopWhenDoneFlag>(line.data());

        if (parseDoc.HasParseError())
        {
            return;
        }

        auto itr = parseDoc.FindMember("time");
        if (itr != parseDoc.MemberEnd() && itr->value.IsInt())
        {
            float timeInSec = 0.001f * (float)itr->value.GetInt();
            itr = parseDoc.FindMember("type");
            if (itr != parseDoc.MemberEnd() && itr->value.IsString())
            {
                AZStd::string typeStr(itr->value.GetString());
                static const AZStd::string s_typeViseme("viseme");
                static const AZStd::string s_typeSentence("sentence");
                static const AZStd::string s_typeWord("word");
                if (typeStr == s_typeViseme)
                {
                    itr = parseDoc.FindMember("value");
                    if (itr != parseDoc.MemberEnd() && itr->value.IsString())
                    {
                        AZStd::string strViseme(itr->value.GetString());
                        VisemeAnimation* visemeAnim = m_visemeSet.GetVisemeByString(strViseme);
                        if (visemeAnim)
                        {
                            m_speechMarks.push_back(SpeechMark(timeInSec, visemeAnim));
                        }
                    }
                }
                else if (typeStr == s_typeSentence)
                {
                    itr = parseDoc.FindMember("value");
                    if (itr != parseDoc.MemberEnd() && itr->value.IsString())
                    {
                        AZStd::string strSentence(itr->value.GetString());
                        itr = parseDoc.FindMember("start");
                        if (itr != parseDoc.MemberEnd() && itr->value.IsInt())
                        {
                            int startIndex = itr->value.GetInt();
                            itr = parseDoc.FindMember("end");
                            if (itr != parseDoc.MemberEnd() && itr->value.IsInt())
                            {
                                int endIndex = itr->value.GetInt();
                                m_closedCaption.AddSentence(timeInSec, startIndex, endIndex, strSentence);
                            }
                        }

                    }

                }
                else if (typeStr == s_typeWord)
                {
                    ClosedCaptionSentence* currentSentence = m_closedCaption.GetLastAddedSentence();
                    //A word must be part of a sentence in out system
                    if (currentSentence != nullptr)
                    {
                        itr = parseDoc.FindMember("value");
                        if (itr != parseDoc.MemberEnd() && itr->value.IsString())
                        {
                            AZStd::string strWord(itr->value.GetString());
                            itr = parseDoc.FindMember("start");
                            if (itr != parseDoc.MemberEnd() && itr->value.IsInt())
                            {
                                int startIndex = itr->value.GetInt();
                                itr = parseDoc.FindMember("end");
                                if (itr != parseDoc.MemberEnd() && itr->value.IsInt())
                                {
                                    int endIndex = itr->value.GetInt();
                                    currentSentence->AddWord(timeInSec, startIndex, endIndex, strWord);
                                }
                            }

                        }
                    }
                }
            }
        }
    }


    void SpeechComponent::ClearSpeechMarks()
    {
        m_speechMarks.clear();
        m_speechTime = 0.0f;
        m_currentIndex = 0;
    }


    VisemeAnimation::VisemeAnimation(const AZ::u32& visemeIndex, bool alwaysPlay)
        : m_visemeIndex(visemeIndex)
        , m_alwaysPlay(alwaysPlay)
    {
    }

    void VisemeAnimation::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<VisemeAnimation>()->
                Version(1)
                ->Field("Always Play", &VisemeAnimation::m_alwaysPlay)
                ->Field("Viseme Index", &VisemeAnimation::m_visemeIndex);

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<VisemeAnimation>(
                    "Viseme Animation", "Allows the configuration of facial poses to match visemes")->
                    ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Animation")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)->
                    DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimation::m_visemeIndex, "Viseme index",
                        "index that matches the parameter condition in the animation graph. ")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimation::m_alwaysPlay, "Always Play",
                        "Guarantees that this viseme will be played regardless of how long or short the viseme file specifies");

            }
        }
    }

    void VisemeAnimationSet::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<VisemeAnimationSet>()->
                Version(1)
                ->Field("Viseme p", &VisemeAnimationSet::m_p)
                ->Field("Viseme t", &VisemeAnimationSet::m_t)
                ->Field("Viseme SS", &VisemeAnimationSet::m_S)
                ->Field("Viseme TT", &VisemeAnimationSet::m_T)
                ->Field("Viseme f", &VisemeAnimationSet::m_f)
                ->Field("Viseme k", &VisemeAnimationSet::m_k)
                ->Field("Viseme i", &VisemeAnimationSet::m_i)
                ->Field("Viseme r", &VisemeAnimationSet::m_r)
                ->Field("Viseme s", &VisemeAnimationSet::m_s)
                ->Field("Viseme u", &VisemeAnimationSet::m_u)
                ->Field("Viseme amp", &VisemeAnimationSet::m_amp)
                ->Field("Viseme a", &VisemeAnimationSet::m_a)
                ->Field("Viseme e", &VisemeAnimationSet::m_e)
                ->Field("Viseme EE", &VisemeAnimationSet::m_E)
                ->Field("Viseme o", &VisemeAnimationSet::m_o)
                ->Field("Viseme OO", &VisemeAnimationSet::m_O)
                ->Field("Viseme sil", &VisemeAnimationSet::m_sil);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<VisemeAnimationSet>("VisemeAnimationSet", "Set of all Viseme animations")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_p, "Viseme p", "Viseme p")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_t, "Viseme t", "Viseme t")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_S, "Viseme SS", "Viseme S")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_T, "Viseme TT", "Viseme T")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_f, "Viseme f", "Viseme f")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_k, "Viseme k", "Viseme k")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_i, "Viseme i", "Viseme i")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_r, "Viseme r", "Viseme r")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_s, "Viseme s", "Viseme s")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_u, "Viseme u", "Viseme u")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_amp, "Viseme @", "Viseme @")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_a, "Viseme a", "Viseme a")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_e, "Viseme e", "Viseme e")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_E, "Viseme EE", "Viseme E")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_o, "Viseme o", "Viseme o")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_O, "Viseme OO", "Viseme O")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VisemeAnimationSet::m_sil, "Viseme sil", "Viseme sil");
            }
        }
    }

    VisemeAnimation* VisemeAnimationSet::GetVisemeByString(const AZStd::string& v)
    {
        if (v =="p") return &m_p;
        if (v == "t") return &m_t;
        if (v == "S") return &m_S;
        if (v == "T") return &m_T;
        if (v == "f") return &m_f;
        if (v == "k") return &m_k;
        if (v == "i") return &m_i;
        if (v == "r") return &m_r;
        if (v == "s") return &m_s;
        if (v == "u") return &m_u;
        if (v == "@") return &m_amp;
        if (v == "a") return &m_a;
        if (v == "e") return &m_e;
        if (v == "E") return &m_E;
        if (v == "o") return &m_o;
        if (v == "O") return &m_O;
        if (v == "sil") return &m_sil;
        return nullptr;
    }

    ClosedCaptionWord* ClosedCaptionSentence::GetCurrentWord(const float time)
    {
        size_t numWords = m_words.size();
        ClosedCaptionWord* currentWord = nullptr;
        if (numWords > 0)
        {
            if (m_lastRequestedWordIndex < 0)
            {
                m_lastRequestedWordIndex = 0;
            }
            int curIndex = m_lastRequestedWordIndex < numWords ? m_lastRequestedWordIndex : 0;
            currentWord = &m_words[curIndex];
            while (curIndex < numWords)
            {
                if (m_words[curIndex].m_time >= time)
                {
                    return currentWord;
                }
                m_lastRequestedWordIndex = curIndex;
                currentWord = &m_words[curIndex];
                ++curIndex;
            }
        }
        return currentWord;
    }

    ClosedCaptionWord* ClosedCaptionSentence::GetLastRequestedWord()
    {
        if (m_lastRequestedWordIndex >= 0 && m_lastRequestedWordIndex < m_words.size())
        {
            return &m_words[m_lastRequestedWordIndex];
        }
        return nullptr;
    }
    bool ClosedCaptionSentence::HasReachedLastWord()
    {
        size_t numWords = m_words.size();
        return m_lastRequestedWordIndex >= (numWords - 1);
    }

    void ClosedCaptionSentence::AddWord(const float time, const int startIndex, const int endIndex, const AZStd::string& word)
    {
        m_words.push_back(ClosedCaptionWord(time, startIndex, endIndex, word));
    }

    ClosedCaptionSentence* ClosedCaption::GetCurrentSentence(const float time)
    {
        size_t numSentences = m_sentences.size();
        ClosedCaptionSentence* currentSentence = nullptr;
        if (numSentences > 0)
        {
            if (m_lastRequestedSentenceIndex < 0)
            {
                m_lastRequestedSentenceIndex = 0;
            }
            int curIndex = m_lastRequestedSentenceIndex < numSentences ? m_lastRequestedSentenceIndex : 0;
            currentSentence = &m_sentences[curIndex];
            while (curIndex < numSentences)
            {
                if (m_sentences[curIndex].m_time >= time)
                {
                    return currentSentence;
                }
                m_lastRequestedSentenceIndex = curIndex;
                currentSentence = &m_sentences[curIndex];
                ++curIndex;
            }
        }
        
        return currentSentence;
    }

    ClosedCaptionSentence* ClosedCaption::GetLastRequestedSentence()
    {
        if (m_lastRequestedSentenceIndex >= 0 && m_lastRequestedSentenceIndex < m_sentences.size())
        {
            return &m_sentences[m_lastRequestedSentenceIndex];
        }
        return nullptr;
    }

    ClosedCaptionSentence* ClosedCaption::GetLastAddedSentence()
    {
        if (m_sentences.size() > 0)
        {
            return &m_sentences[m_sentences.size() - 1];
        }
        return nullptr;
    }

    bool ClosedCaption::HasReachedLastSentence()
    {
        size_t numSentences = m_sentences.size();
        return m_lastRequestedSentenceIndex >= (numSentences - 1);
    }

    void ClosedCaption::AddSentence(const float time, const int startIndex, const int endIndex, const AZStd::string& sentence)
    {
        m_sentences.push_back(ClosedCaptionSentence(time, startIndex, endIndex, sentence));
    }

    void ClosedCaption::Clear()
    {
        m_sentences.clear();
        m_lastRequestedSentenceIndex = -1;
    }

    void BehaviorTextToSpeechClosedCaptionHandler::OnNewClosedCaptionSentence( AZStd::string sentence, int startIndex, int endIndex)
    {
        Call(FN_OnNewClosedCaptionSentence, sentence, startIndex, endIndex);
    }

    void BehaviorTextToSpeechClosedCaptionHandler::OnNewClosedCaptionWord( AZStd::string word, int startIndex, int endIndex)
    {
        Call(FN_OnNewClosedCaptionWord, word, startIndex, endIndex);
    }
}