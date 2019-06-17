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

#include "CloudGemTextToSpeech/TextToSpeech.h"

#include <fstream>

#include <md5.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>

#include <CloudCanvasCommon/CloudCanvasCommonBus.h>
#include <CloudGemFramework/AwsApiRequestJob.h>

#pragma warning(disable: 4355)
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/json/JsonSerializer.h>
#pragma warning(default: 4355)

namespace {
    const char* AUDIO_FILE_EXT_PCM = ".pcm";
    const char* AUDIO_FILE_EXT_WAV = ".wav";
    const char* SPEECH_MARKS_FILE_EXT = ".txt";
    const char* MAPPING_FILE_NAME = "character_mapping.json";
    const char* MAIN_CACHE_LOCATION = "@assets@/ttscache/";
    const char* USER_CACHE_LOCATION = "@user@/ttscache/";
}

namespace CloudGemTextToSpeech
{
    void TextToSpeech::Init()
    {
    }


    void TextToSpeech::Activate()
    {
        EBUS_EVENT_RESULT(m_jobContext, CloudGemFramework::CloudGemFrameworkRequestBus, GetDefaultJobContext);

        CloudCanvas::PresignedURLResultBus::Handler::BusConnect(m_entity->GetId());
        TextToSpeechRequestBus::Handler::BusConnect(m_entity->GetId());
        ConversionNotificationBus::Handler::BusConnect(m_entity->GetId());
    }


    void TextToSpeech::Deactivate()
    {
        TextToSpeechRequestBus::Handler::BusDisconnect();
        ConversionNotificationBus::Handler::BusDisconnect();
    }


    // TextToSpeechRequestBus::Handler
    void TextToSpeech::ConvertTextToSpeechWithMarks(const AZStd::string& voice, const AZStd::string& text, const AZStd::string& speechMarks)
    {
        ConvertTextToSpeech(voice, text, speechMarks);
    }

    // TextToSpeechRequestBus::Handler
    void TextToSpeech::ConvertTextToSpeechWithoutMarks(const AZStd::string& voice, const AZStd::string& text)
    {
        ConvertTextToSpeech(voice, text, "");
    }

    // TextToSpeechRequestBus::Handler
    AZStd::string TextToSpeech::GetVoiceFromCharacter(const AZStd::string& character)
    {
        auto characterInfo = LoadCharacterFromMappingsFile(character);
        if (characterInfo.ValueExists("voice"))
        {
            return characterInfo.GetString("voice").c_str();
        }
        return "";
    }

    AZStd::string TextToSpeech::GetSpeechMarksFromCharacter(const AZStd::string& character)
    {
        auto characterInfo = LoadCharacterFromMappingsFile(character);
        if (characterInfo.ValueExists("speechMarks"))
        {
            return characterInfo.GetString("speechMarks").c_str();
        }
        return "";
    }

    AZStd::vector<AZStd::string> TextToSpeech::GetProsodyTagsFromCharacter(const AZStd::string& character)
    {
        AZStd::vector<AZStd::string> tags;
        auto characterInfo = LoadCharacterFromMappingsFile(character);

        if (characterInfo.ValueExists("ssmlTags"))
        {
            if (!characterInfo.GetObject("ssmlTags").IsListType())
            {
                return tags;
            }
            auto tagsList = characterInfo.GetArray("ssmlTags");
            for (size_t i = 0; i < tagsList.GetLength(); i++)
            {
                if (tagsList.GetItem(i).IsString())
                {
                    tags.push_back(tagsList.GetItem(i).AsString().c_str());
                }
            }
        }
        return tags;
    }

    AZStd::string TextToSpeech::GetLanguageOverrideFromCharacter(const AZStd::string& character)
    {
        auto characterInfo = LoadCharacterFromMappingsFile(character);
        if (characterInfo.ValueExists("ssmlLanguage"))
        {
            return characterInfo.GetString("ssmlLanguage").c_str();
        }

        return "";
    }

    int TextToSpeech::GetTimbreFromCharacter(const AZStd::string& character)
    {
        auto characterInfo = LoadCharacterFromMappingsFile(character);
        if (characterInfo.ValueExists("timbre"))
        {
            return characterInfo.GetInteger("timbre");
        }

        return 100;
    }


    Aws::Utils::Json::JsonValue TextToSpeech::LoadCharacterFromMappingsFile(const AZStd::string& character)
    {
        AZStd::vector<AZStd::string> tags;
        AZStd::string localCachePath = ResolvePath(MAIN_CACHE_LOCATION, true);
        if (!localCachePath.length())
        {
            AZ_Printf("TextToSpeech", "Could not resolve local cache path for Text to Speech");
        }
        AZStd::string mappingFile = ResolvePath((localCachePath + MAPPING_FILE_NAME).c_str(), false);

        if (AZ::IO::SystemFile::Exists(mappingFile.c_str()))
        {
            Aws::IFStream inputFile(mappingFile.c_str());
            Aws::Utils::Json::JsonValue characterMapping(inputFile);
            for (auto const &characterInfo : characterMapping.GetAllObjects())
            {
                if (characterInfo.first == character.c_str())
                {
                    return characterInfo.second;
                }
            }
        }
        return Aws::Utils::Json::JsonValue{};
    }

    void TextToSpeech::ConvertTextToSpeech(const AZStd::string& voice, const AZStd::string& text, const AZStd::string& speechMarks)
    {
        AZStd::string hash = voice + "-" + text;
        hash = GenerateMD5FromPayload(hash.c_str());

        AZStd::string userCachePath = ResolvePath(USER_CACHE_LOCATION, true);
        if (!userCachePath.length())
        {
            AZ_Printf("TextToSpeech", "Could not resolve user cache path for Text to Speech");
        }
        AZStd::string userVoiceFile = ResolvePath((userCachePath + hash  + AUDIO_FILE_EXT_PCM).c_str(), false);
        AZStd::string userMarksFile = ResolvePath((userCachePath + hash + "-" + speechMarks + SPEECH_MARKS_FILE_EXT).c_str(), false);

        AZStd::string voiceFile;

        // Check both caches for the files you need.
        if (userCachePath.length() > 0 && ( AZ::IO::SystemFile::Exists(userVoiceFile.c_str())) && (speechMarks.empty() || AZ::IO::SystemFile::Exists(userMarksFile.c_str())))
        {
            if (!speechMarks.empty())
            {
                EBUS_EVENT_ID(m_entity->GetId(), TextToSpeechPlaybackBus, PlayWithLipSync, GetAliasedUserCachePath(hash), userMarksFile);
            }
            else
            {
                EBUS_EVENT_ID(m_entity->GetId(), TextToSpeechPlaybackBus, PlaySpeech,  GetAliasedUserCachePath(hash));
            }
        }
        else if (MainCacheFilesExist(hash, speechMarks, voiceFile))
        {
            if (!speechMarks.empty())
            {
                AZStd::string marksFile{ MAIN_CACHE_LOCATION + hash + "-" + speechMarks + SPEECH_MARKS_FILE_EXT };
                EBUS_EVENT_ID(m_entity->GetId(), TextToSpeechPlaybackBus, PlayWithLipSync, voiceFile, marksFile);
            }
            else
            {
                EBUS_EVENT_ID(m_entity->GetId(), TextToSpeechPlaybackBus, PlaySpeech, voiceFile);
            }
        }
        else
        {
            if (!speechMarks.empty())
            {
                m_conversionIdToNumPending[hash] = 2;
            }
            else
            {
                m_conversionIdToNumPending[hash] = 1;
            }
            aznew TTSConversionJob(*this, voice, text, hash, speechMarks);
        }
    }

    bool TextToSpeech::MainCacheFilesExist(const AZStd::string& hash, const AZStd::string& speechMarks, AZStd::string& voiceFile)
    {
        voiceFile = MAIN_CACHE_LOCATION + hash + AUDIO_FILE_EXT_PCM;
        bool voiceFileSatisfied = AZ::IO::FileIOBase::GetInstance()->Exists(voiceFile.c_str());
        if (!voiceFileSatisfied)
        {
            voiceFile = MAIN_CACHE_LOCATION + hash + AUDIO_FILE_EXT_WAV;
            voiceFileSatisfied = AZ::IO::FileIOBase::GetInstance()->Exists(voiceFile.c_str());
        }

        if (!voiceFileSatisfied)
        {
            voiceFile.clear();
        }

        bool speechMarksSatisfied = true;
        if (!speechMarks.empty())
        {
            AZStd::string marksFile{MAIN_CACHE_LOCATION + hash + "-" + speechMarks + SPEECH_MARKS_FILE_EXT};
            speechMarksSatisfied = AZ::IO::FileIOBase::GetInstance()->Exists(marksFile.c_str());
        }

        return voiceFileSatisfied && speechMarksSatisfied;
    }

    // ConversionNotificationBus::Handler
    void TextToSpeech::GotDownloadUrl(const AZStd::string& hash, const AZStd::string& url, const AZStd::string& speechMarks)
    {
        AZStd::string localCachePath = ResolvePath(USER_CACHE_LOCATION, true);
        if (!localCachePath.length())
        {
            AZ_Printf("TextToSpeech", "Could not resolve local cache path for Text to Speech");
        }

        m_urlToConversionId[url] = hash;
        if (!speechMarks.empty())
        {
            m_idToUrls[hash].marks = url;
            m_idToSpeechMarksType[hash] = speechMarks;
        }
        else
        {
            m_idToUrls[hash].voice = url;
        }

        if (m_conversionIdToNumPending[hash] <= m_idToUrls[hash].size())
        {
            if (!m_idToUrls[hash].voice.empty())
            {
                AZStd::string voiceFile = ResolvePath((localCachePath + hash + AUDIO_FILE_EXT_PCM).c_str(), false);
                EBUS_EVENT(CloudCanvas::PresignedURLRequestBus, RequestDownloadSignedURL, m_idToUrls[hash].voice, voiceFile, m_entity->GetId());
            }
            if (!m_idToUrls[hash].marks.empty())
            {
                auto marks = m_idToSpeechMarksType[hash];
                AZStd::string marksFile = ResolvePath((localCachePath + hash + "-" + marks +  SPEECH_MARKS_FILE_EXT).c_str(), false);
                EBUS_EVENT(CloudCanvas::PresignedURLRequestBus, RequestDownloadSignedURL, m_idToUrls[hash].marks, marksFile, m_entity->GetId());
            }

            m_conversionIdToNumPending.erase(hash);
        }
    }

    // CloudCanvas::PresignedURLResultBus::Handler
    void TextToSpeech::GotPresignedURLResult(const AZStd::string& requestURL, int responseCode, const AZStd::string& responseMessage, const AZStd::string& outputFile)
    {
        if (responseCode == 200)
        {
            auto iter = m_urlToConversionId.find(requestURL);
            if (iter != m_urlToConversionId.end())
            {
                auto id = iter->second;
                // what kind of file is this?
                if (outputFile.find(AUDIO_FILE_EXT_PCM) != AZStd::string::npos)
                {
                    m_idToFiles[id].voice = outputFile;
                }
                else if (outputFile.find(SPEECH_MARKS_FILE_EXT) != AZStd::string::npos)
                {
                    m_idToFiles[id].marks = outputFile;
                }
                else
                {
                    AZ_Printf("TextToSpeech", "Conversion Job got unrecognized file type");
                }
                if (HasFilesFromConversion(id))
                {
                    AZStd::string voiceFile = m_idToFiles[id].voice;
                    AZ::EntityId entityId = m_entity->GetId();

                    // Expecting speech marks for this request
                    if (!m_idToFiles[id].marks.empty())
                    {
                        m_idToSpeechMarksType.erase(id);
                        AZStd::string marksFile = m_idToFiles[id].marks;
                        if (AZ::IO::SystemFile::Exists(voiceFile.c_str()) && AZ::IO::SystemFile::Exists(marksFile.c_str()))
                        {
                            AZStd::string aliasedVoicePath = GetAliasedUserCachePath(id);
                            AZStd::function<void()> playFunction = [ marksFile, entityId, aliasedVoicePath ] ()
                            {
                                EBUS_EVENT_ID(entityId, TextToSpeechPlaybackBus, PlayWithLipSync, aliasedVoicePath, marksFile);
                            };
                            EBUS_QUEUE_FUNCTION(AZ::TickBus, playFunction);
                        }
                        else
                        {
                            AZ_Printf("TextToSpeech", "Conversion Job finished, expected speech marks and voice files could not be found");
                        }
                    }
                    else if (AZ::IO::SystemFile::Exists(voiceFile.c_str()))  // Expecting just audio for this request
                    {
                        AZStd::string aliasedVoicePath = GetAliasedUserCachePath(id);
                        AZStd::function<void()> playFunction = [ aliasedVoicePath, entityId ] ()
                        {
                            EBUS_EVENT_ID(entityId, TextToSpeechPlaybackBus, PlaySpeech, aliasedVoicePath);
                        };
                        EBUS_QUEUE_FUNCTION(AZ::TickBus, playFunction);
                    }
                    else
                    {
                        AZ_Printf("TextToSpeech", "Conversion Job finished, expected voice file could not be found");
                    }
                    m_idToUrls.erase(id);
                    m_idToFiles.erase(id);
                }
                else
                {
                    AZ_Printf("TextToSpeech", "Still waiting for another file for this job");
                }
                m_urlToConversionId.erase(iter);
            }
        }
        else
        {
            AZStd::string printstring = "Failed to retrieve presigned url download with message: " + responseMessage;
            AZ_Printf("TextToSpeech", printstring.c_str());
        }
    }

    bool TextToSpeech::HasFilesFromConversion(AZStd::string id)
    {
        return m_idToUrls[id].size() == m_idToFiles[id].size();
    }


    AZStd::string TextToSpeech::ResolvePath(const char* path, bool isDir) const
    {
        char resolvedGameFolder[AZ_MAX_PATH_LEN] = { 0 };
        if (!AZ::IO::FileIOBase::GetInstance()->ResolvePath(path, resolvedGameFolder, AZ_MAX_PATH_LEN))
        {
            return "";
        }
        AZStd::string result = resolvedGameFolder;
        if (isDir && result.length() && result[result.length() - 1] != '/')
        {
            result += '/';
        }
        return result;
    }


    AZStd::string TextToSpeech::GenerateMD5FromPayload(const char* fileName) const
    {
        static const int hashLength = 16;
        AZStd::vector<unsigned char> hashVec;
        hashVec.resize(hashLength);
        GenerateMD5(fileName, &hashVec[0]);

        AZStd::string returnStr;
        char charHash[3] = { 0, 0, 0 };
        unsigned char vecMember;
        for (int i = 0; i < hashVec.size(); ++i)
        {
            vecMember = hashVec[i];
            sprintf_s(charHash, "%02x", vecMember);
            returnStr += charHash;
        }
        return returnStr;
    }

    // Some platforms don't play nicely with Crypak when passing absolute paths
    // Since the audio system still loads files through Crypak we can avoid this by passing an aliased path
    AZStd::string TextToSpeech::GetAliasedUserCachePath(const AZStd::string& hash) const
    {
        return AZStd::string{ USER_CACHE_LOCATION } + hash + ".pcm";
    }

    bool TextToSpeech::GenerateMD5(const char* szName, unsigned char* md5) const
    {
        if (!szName || !md5)
        {
            return false;
        }

        const uint32 c_MaxNameSize = 1024 * 1024;

        AZStd::vector<unsigned char>  myNewString{ szName };
        uint32 nameSize = strlen(szName);
        
        if (nameSize == 0 || nameSize >= c_MaxNameSize)
        {
            return false;
        }
        
        myNewString.resize(nameSize);
        memcpy(&myNewString[0], szName, nameSize * sizeof(char));
        if (myNewString.size() >= c_MaxNameSize)
        {
            return false;
        }

        MD5Context context;
        MD5Init(&context);
        MD5Update(&context, &myNewString[0], myNewString.size());
        MD5Final(md5, &context);
        return true;
    }


    TextToSpeech::TTSConversionJob::TTSConversionJob(TextToSpeech& component, const AZStd::string& voice, const AZStd::string& message, const AZStd::string& hash, const AZStd::string& speechMarks)
        : AZ::Job(true, component.GetJobContext())
        , m_entityId(component.GetEntity()->GetId())
        , m_speechMarks(speechMarks)
    {
        AZStd::string newhash(hash.c_str());
        m_hash = newhash;
        m_request.voice = voice;
        m_request.message = message;

        m_marksRequest.voice = voice;
        m_marksRequest.message = message;
        m_marksRequest.speechMarks = m_speechMarks;

        Start();
    }


    void TextToSpeech::TTSConversionJob::Process()
    {
        AZStd::string hash{ m_hash };
        AZ::EntityId entityId{ m_entityId };
        auto voiceJob = ServiceAPI::PostTtsVoicelineRequestJob::Create(
            [hash, entityId](ServiceAPI::PostTtsVoicelineRequestJob* job)
        {
            EBUS_EVENT_ID(entityId, ConversionNotificationBus, GotDownloadUrl, hash, job->result.url, "");
        },
            [](ServiceAPI::PostTtsVoicelineRequestJob* job)
        {
        }
        );
        voiceJob->parameters.request_info = m_request;
        StartAsChild(&voiceJob->GetHttpRequestJob());
        if (!m_speechMarks.empty())
        {
            auto speechMarks = m_speechMarks;
            auto speechMarksJob = ServiceAPI::PostTtsSpeechmarksRequestJob::Create(
                [hash, entityId, speechMarks](ServiceAPI::PostTtsSpeechmarksRequestJob* job)
                {
                    EBUS_EVENT_ID(entityId, ConversionNotificationBus, GotDownloadUrl, hash, job->result.url, speechMarks);
                },
                [](ServiceAPI::PostTtsSpeechmarksRequestJob* job)
                {
                }
            );
            speechMarksJob->parameters.request_info = m_marksRequest;
            StartAsChild(&speechMarksJob->GetHttpRequestJob());
        }
        WaitForChildren();
    }

    int TextToSpeech::ConversionSet::size()
    {
        int size = 0;
        if (!voice.empty()) size++;
        if (!marks.empty()) size++;
        return size;
    }

    void BehaviorTextToSpeechPlaybackHandler::PlaySpeech( AZStd::string voicePath)
    {
        Call(FN_PlaySpeech, voicePath);
    }

    void BehaviorTextToSpeechPlaybackHandler::PlayWithLipSync( AZStd::string voicePath,  AZStd::string speechMarksPath)
    {
        Call(FN_PlayWithLipSync, voicePath, speechMarksPath);
    }
}
