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
#include "stdafx.h"
#include "AudioUnitTestData.h"
#include "CryPath.h"

namespace Audio
{
    IAudioSystemImplementation* CAudioUnitTestData::ms_audioSystemImpl = nullptr;
    uint32 CAudioUnitTestData::ms_audioObjectNextId = 2;
    string CAudioUnitTestData::ms_audioSystemImplementationName = "";

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IAudioSystemImplementation* CAudioUnitTestData::GetAudioSystemImplementation()
    {
        if (ms_audioSystemImpl == nullptr)
        {
            ms_audioSystemImpl = static_cast<CAudioSystem*>(gEnv->pAudioSystem)->GetATL().GetAudioSystemImpl();
        }

        return ms_audioSystemImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    uint32 CAudioUnitTestData::GetNextUniqueId()
    {
        return ++ms_audioObjectNextId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioUnitTestData::Init()
    {
        ms_audioSystemImplementationName = GetAudioSystemImplementation()->GetImplSubPath();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const IATLTriggerImplData* CAudioUnitTestData::GenerateTriggerImplData(const char* fileName /*= nullptr*/)
    {
        // todo: respect the fileName passed in.
        string xmlFile(".\\TestAssets\\UnitTest\\Audio\\");
        xmlFile += ms_audioSystemImplementationName;
        xmlFile += "mintestTrigger.xml";
        const XmlNodeRef atlRootXmlNode(GetISystem()->LoadXmlFromFile(xmlFile.c_str()));
        if (atlRootXmlNode)
        {
            return GetAudioSystemImplementation()->NewAudioTriggerImplData(atlRootXmlNode);
        }
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const IATLRtpcImplData* CAudioUnitTestData::GenerateRtpcImplData(const char* fileName /*= nullptr*/)
    {
        // todo: respect the fileName passed in.
        string xmlFile(".\\TestAssets\\UnitTest\\Audio\\");
        xmlFile += ms_audioSystemImplementationName;
        xmlFile += "mintestRtpc.xml";
        const XmlNodeRef atlRootXmlNode(GetISystem()->LoadXmlFromFile(xmlFile.c_str()));
        if (atlRootXmlNode)
        {
            return GetAudioSystemImplementation()->NewAudioRtpcImplData(atlRootXmlNode);
        }
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const IATLSwitchStateImplData* CAudioUnitTestData::GenerateSwitchStateImplData(const char* fileName /*= nullptr*/)
    {
        // todo: respect the fileName passed in.
        string xmlFile(".\\TestAssets\\UnitTest\\Audio\\");
        xmlFile += ms_audioSystemImplementationName;
        xmlFile += "mintestSwitchState.xml";
        const XmlNodeRef atlRootXmlNode(GetISystem()->LoadXmlFromFile(xmlFile.c_str()));
        if (atlRootXmlNode)
        {
            return GetAudioSystemImplementation()->NewAudioSwitchStateImplData(atlRootXmlNode);
        }
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const IATLEnvironmentImplData* CAudioUnitTestData::GenerateEnvironmentImplData(const char* fileName /*= nullptr*/)
    {
        // todo: respect the fileName passed in.
        string xmlFile(".\\TestAssets\\UnitTest\\Audio\\");
        xmlFile += ms_audioSystemImplementationName;
        xmlFile += "mintestEnvironments.xml";
        const XmlNodeRef atlRootXmlNode(GetISystem()->LoadXmlFromFile(xmlFile.c_str()));
        if (atlRootXmlNode)
        {
            return GetAudioSystemImplementation()->NewAudioEnvironmentImplData(atlRootXmlNode);
        }
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLAudioFileEntryInfo* const CAudioUnitTestData::GenerateATLFileEntryInfoData(const char* fileName /*= nullptr*/)
    {
        // todo: respect the fileName passed in.
        string xmlFile(".\\TestAssets\\UnitTest\\Audio\\");
        xmlFile += ms_audioSystemImplementationName;
        xmlFile += "mintestPreloads.xml";
        const XmlNodeRef atlRootXmlNode(GetISystem()->LoadXmlFromFile(xmlFile.c_str()));
        if (atlRootXmlNode)
        {
            SATLAudioFileEntryInfo* audioFileEntryInfo = new SATLAudioFileEntryInfo();
            EAudioRequestStatus result = GetAudioSystemImplementation()->ParseAudioFileEntry(atlRootXmlNode, audioFileEntryInfo);
            if (result == eARS_SUCCESS)
            {
                return audioFileEntryInfo;
            }
        }
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioUnitTestData::ReleaseATLFileEntryInfoData(SATLAudioFileEntryInfo* audioFileEntryInfo)
    {
        if (audioFileEntryInfo)
        {
            // delete the impl data...
            GetAudioSystemImplementation()->DeleteAudioFileEntryData(audioFileEntryInfo->pImplData);

            // free the file data...
            if (audioFileEntryInfo->pFileData)
            {
                _aligned_free(audioFileEntryInfo->pFileData);
                audioFileEntryInfo->pFileData = nullptr;
            }
            delete audioFileEntryInfo;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioUnitTestData::LoadBankFile(SATLAudioFileEntryInfo* audioFileEntryInfo)
    {
        if (audioFileEntryInfo != nullptr)
        {
            // Read and load a bank file into memory
            FILE* pFile = fopen(audioFileEntryInfo->sFileName, "rb");
            if (pFile != nullptr)
            {
                fseek(pFile, 0, SEEK_END);
                const size_t fileSize = ftell(pFile);
                rewind(pFile);

                char* dummyData = static_cast<char*>(_aligned_malloc(fileSize, AUDIO_MEMORY_ALIGNMENT));
                fread(dummyData, sizeof(char), fileSize, pFile);
                fclose(pFile);

                audioFileEntryInfo->pFileData = dummyData;
                audioFileEntryInfo->nSize = fileSize;
            }
        }
    }
} // namespace Audio
