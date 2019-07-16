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

#include "StdAfx.h"
#include "AudioSystemImpl_NoSound.h"
#include "AudioSystemImplCVars_NoSound.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//      NoSound Audio System Implementation
///////////////////////////////////////////////////////////////////////////////////////////////////


namespace Audio
{

    static const unsigned int s_nosoundDefaultMemoryAlignment = 16;

    const char* const CAudioSystemImpl_NoSound::s_nosoundShortName = "NoSound";
    const char* const CAudioSystemImpl_NoSound::s_nosoundLongName = "NoSound Audio Implementation";
    const char* const CAudioSystemImpl_NoSound::s_nosoundImplSubPath = "nosound/";
    const char* const CAudioSystemImpl_NoSound::s_nosoundFileTag = "NoSoundFile";
    const char* const CAudioSystemImpl_NoSound::s_nosoundNameAttr = "nosound_name";
    const float CAudioSystemImpl_NoSound::s_obstructionOcclusionMin = 0.0f;
    const float CAudioSystemImpl_NoSound::s_obstructionOcclusionMax = 1.0f;


    // callbacks
    void PrepareEventCallback(void* pCookie)
    {
        (void)pCookie; // dummy callback for now
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioSystemImpl_NoSound::CAudioSystemImpl_NoSound()
        : m_registeredAudioObjects()
        , m_registeredTriggerCallbackInfo()
    {
        AudioSystemImplementationRequestBus::Handler::BusConnect();
        AudioSystemImplementationNotificationBus::Handler::BusConnect();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioSystemImpl_NoSound::~CAudioSystemImpl_NoSound()
    {
        AudioSystemImplementationRequestBus::Handler::BusDisconnect();
        AudioSystemImplementationNotificationBus::Handler::BusDisconnect();
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // AZ::Component
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::Init()
    {
        //CryLogAlways("AZ::Component - CAudioSystemImpl_NoSound::Init()");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::Activate()
    {
        //CryLogAlways("AZ::Component - CAudioSystemImpl_NoSound::Activate()");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::Deactivate()
    {
        //CryLogAlways("AZ::Component - CAudioSystemImpl_NoSound::Deactivate()");
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // AudioSystemImplementationNotificationBus
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::OnAudioSystemLoseFocus()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::OnAudioSystemGetFocus()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::OnAudioSystemMuteAll()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::OnAudioSystemUnmuteAll()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::OnAudioSystemRefresh()
    {
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // AudioSystemImplementationRequestBus
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::Update(const float fUpdateIntervalMS)
    {
        // this is a minimal implementation that can cover the needs for prepare/unprepare
        // triggers both synchronously and asynchronously
        //
        // simulate the process of prepare/unprepare by spending some time
        // if the processing time of a trigger has reached its total time,
        // "finish" the trigger by removing it from the registry list
        for (auto it = m_registeredTriggerCallbackInfo.begin(); it != m_registeredTriggerCallbackInfo.end(); )
        {
            STriggerCallbackInfo* trigCallbackInfo = it->second;
            trigCallbackInfo->m_timeElasped += fUpdateIntervalMS / 1000.0f;
            if (trigCallbackInfo->m_timeElasped > trigCallbackInfo->m_timeTotal)
            {
                it = m_registeredTriggerCallbackInfo.erase(it);
                delete trigCallbackInfo;
            }
            else
            {
                ++it;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::Initialize()
    {
        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::ShutDown()
    {
        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::Release()
    {
        g_audioImplCVars_nosound.UnregisterVariables();

        azdestroy(this, Audio::AudioImplAllocator, CAudioSystemImpl_NoSound);

        if (AZ::AllocatorInstance<Audio::AudioImplAllocator>::IsReady())
        {
            AZ::AllocatorInstance<Audio::AudioImplAllocator>::Destroy();
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::StopAllSounds()
    {
        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::RegisterAudioObject(IATLAudioObjectData* const pObject, const char* const sObjectName)
    {
        if (!pObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called RegisterAudioObject with nullptr audio data!");
            return eARS_FAILURE;
        }

        if (!sObjectName || sObjectName[0] == '\0')
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called RegisterAudioObject with invalid or empty name!");
            return eARS_FAILURE;
        }

        if (m_registeredAudioObjects.find(pObject) != m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called RegisterAudioObject on an audio data that has already been registered!");
        }
        else
        {
            m_registeredAudioObjects.insert(pObject);
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::UnregisterAudioObject(IATLAudioObjectData* const pObject)
    {
        if (!pObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called UnregisterAudioObject with nullptr audio data!");
            return eARS_FAILURE;
        }

        // unregister the same object twice is ok, just send a warning
        if (m_registeredAudioObjects.find(pObject) == m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called UnregisterAudioObject on an audio data that has not been registered!");
        }
        else
        {
            m_registeredAudioObjects.erase(pObject);
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::ResetAudioObject(IATLAudioObjectData* const pObject)
    {
        if (!pObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called ResetAudioObject with nullptr audio data!");
            return eARS_FAILURE;
        }

        if (m_registeredAudioObjects.find(pObject) == m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_COMMENT, "Called ResetAudioObject on an audio data that isn't registered!");
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::UpdateAudioObject(IATLAudioObjectData* const pObject)
    {
        // wenjia-> the expected procedure here is that the audio object will only update when it really needs to update environment, 
        // currently can't find a way to simulate this. This function will fail to unit test 7_2_1
        if (!pObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called UpdateAudioObject with nullptr audio data!");
            return eARS_FAILURE;
        }

        if (m_registeredAudioObjects.find(pObject) == m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called UpdateAudioObject on an audio data that has not been registered!");
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::PrepareTriggerSync(IATLAudioObjectData* const pAudioObject, const IATLTriggerImplData* const pTrigger)
    {
        return PrepUnprepTriggerSync(pAudioObject, pTrigger, true);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::UnprepareTriggerSync(IATLAudioObjectData* const pAudioObject, const IATLTriggerImplData* const pTrigger)
    {
        return PrepUnprepTriggerSync(pAudioObject, pTrigger, false);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::PrepareTriggerAsync(IATLAudioObjectData* const pAudioObject, const IATLTriggerImplData* const pTrigger, IATLEventData* const pEvent)
    {
        return PrepUnprepTriggerAsync(pAudioObject, pTrigger, pEvent, true);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::UnprepareTriggerAsync(IATLAudioObjectData* const pAudioObject, const IATLTriggerImplData* const pTrigger, IATLEventData* const pEvent)
    {
        return PrepUnprepTriggerAsync(pAudioObject, pTrigger, pEvent, false);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::ActivateTrigger(IATLAudioObjectData* const pAudioObject, const IATLTriggerImplData* const pTrigger, IATLEventData* const pEvent, TAudioSourceId sourceId)
    {
        if (!pAudioObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called ActivateTrigger with nullptr audio data!");
            return eARS_FAILURE;
        }

        if (m_registeredAudioObjects.find(pAudioObject) == m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called ActivateTrigger on an audio data that has not been registered!");
        }

        if (!pTrigger)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called ActivateTrigger with nullptr trigger data!");
            return eARS_FAILURE;
        }

        if (!pEvent)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called ActivateTrigger with nullptr event data!");
            return eARS_FAILURE;
        }

        // NOTE: by referring to wwise implementation, the pEvent is passed with a callback function 
        // to a PostEvent function so that upon completion, the callback function will be triggered 
        // with pEvent as an input. Not sure whether/how we should simulate this process.

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::StopEvent(IATLAudioObjectData* const pAudioObject, const IATLEventData* const pEvent)
    {
        if (!pAudioObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called StopEvent with nullptr audio data!");
            return eARS_FAILURE;
        }

        if (m_registeredAudioObjects.find(pAudioObject) == m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called StopEvent on an audio data that has not been registered!");
        }

        if (!pEvent)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called StopEvent with nullptr event data!");
            return eARS_FAILURE;
        }

        // NOTE: by referring to wwise implementation, the pEvent will be casted to a specific wwise event, 
        // and based on its type variable, it will decide whether to stop the event or not.  

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::StopAllEvents(IATLAudioObjectData* const pAudioObject)
    {
        if (!pAudioObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called StopALLEvents with nullptr audio data!");
            return eARS_FAILURE;
        }

        if (m_registeredAudioObjects.find(pAudioObject) == m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called StopAllEvents on an audio data that has not been registered!");
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::SetPosition(IATLAudioObjectData* const pAudioObject, const SATLWorldPosition& sWorldPosition)
    {
        if (!pAudioObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetPosition with nullptr audio data!");
            return eARS_FAILURE;
        }

        if (m_registeredAudioObjects.find(pAudioObject) == m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetPosition on an audio data that has not been registered!");
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::SetMultiplePositions(IATLAudioObjectData* const pAudioObjectData, const MultiPositionParams& multiPositions)
    {
        if (!pAudioObjectData)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetPosition with nullptr audio data!");
            return eARS_FAILURE;
        }

        if (m_registeredAudioObjects.find(pAudioObjectData) == m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetPosition on an audio data that has not been registered!");
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::SetEnvironment(IATLAudioObjectData* const pAudioObject, const IATLEnvironmentImplData* const pEnvironmentImpl, const float fValue)
    {
        if (!pAudioObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetEnvironment with nullptr audio data!");
            return eARS_FAILURE;
        }

        if (m_registeredAudioObjects.find(pAudioObject) == m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetEnvironment on an audio data that has not been registered!");
        }

        if (!pEnvironmentImpl)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetEnvironment with nullptr environment data!");
            return eARS_FAILURE;
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::SetRtpc(IATLAudioObjectData* const pAudioObject, const IATLRtpcImplData* const pRtpc, const float fValue)
    {
        if (!pAudioObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetRtpc with nullptr audio data!");
            return eARS_FAILURE;
        }

        if (m_registeredAudioObjects.find(pAudioObject) == m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetRtpc on an audio data that has not been registered!");
        }

        if (!pRtpc)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetRtpc with nullptr rptc data!");
            return eARS_FAILURE;
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::SetSwitchState(IATLAudioObjectData* const pAudioObject, const IATLSwitchStateImplData* const pState)
    {
        if (!pAudioObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetSwitchState with nullptr audio data!");
            return eARS_FAILURE;
        }

        if (m_registeredAudioObjects.find(pAudioObject) == m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetSwitchState on an audio data that has not been registered!");
        }

        if (!pState)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetSwitchState with nullptr state data!");
            return eARS_FAILURE;
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::SetObstructionOcclusion(IATLAudioObjectData* const pAudioObject, const float fObstruction, const float fOcclusion)
    {
        if (!pAudioObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetObstructionOcclusion with nullptr audio data!");
            return eARS_FAILURE;
        }

        if (m_registeredAudioObjects.find(pAudioObject) == m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetObstructionOcclusion on an audio data that has not been registered!");
        }

        if (fObstruction < s_obstructionOcclusionMin || fObstruction > s_obstructionOcclusionMax)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetObstructionOcclusion with invalid fObstruction range (valid: [0.0f, 1.0f])!");
        }

        if (fOcclusion < s_obstructionOcclusionMin || fOcclusion > s_obstructionOcclusionMax)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetObstructionOcclusion with invalid fOcclusion range (valid: [0.0f, 1.0f])!");
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::SetListenerPosition(IATLListenerData* const pListener, const SATLWorldPosition& oNewPosition)
    {
        auto eResult = eARS_FAILURE;
        if (!pListener)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetListenerPosition with null listener!");
        }
        else
        {
            eResult = eARS_SUCCESS;
        }
        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::ResetRtpc(IATLAudioObjectData* const pAudioObjectData, const IATLRtpcImplData* const pRtpcData)
    {
        auto eResult = eARS_FAILURE;
        if (!pAudioObjectData || !pRtpcData)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called ResetRtpc with null parameter(s)! pAudioObjectData = %p, pRtpcData = %p", pAudioObjectData, pRtpcData);
        }
        else
        {
            eResult = eARS_SUCCESS;
        }
        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::RegisterInMemoryFile(SATLAudioFileEntryInfo* const pAudioFileEntry)
    {
        auto eResult = eARS_FAILURE;
        if (pAudioFileEntry)
        {
            if (m_soundBanks.count(pAudioFileEntry->pImplData) > 0)
            {
                g_audioImplLogger_nosound.Log(eALT_WARNING, "Called RegisterInMemoryFile with already registered sound bank data!");
            }
            else
            {
                m_soundBanks.insert(pAudioFileEntry->pImplData);
            }
            eResult = eARS_SUCCESS;
        }
        else
        {
            g_audioImplLogger_nosound.Log(eALT_ERROR, "Called RegisterInMemoryFile with nullptr AudioFileEntryData!");
        }
        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::UnregisterInMemoryFile(SATLAudioFileEntryInfo* const pAudioFileEntry)
    {
        auto eResult = eARS_FAILURE;
        if (pAudioFileEntry)
        {
            if (m_soundBanks.count(pAudioFileEntry->pImplData) > 0)
            {
                m_soundBanks.erase(pAudioFileEntry->pImplData);
                eResult = eARS_SUCCESS;
            }
            else
            {
                g_audioImplLogger_nosound.Log(eALT_WARNING, "Called UnregisterInMemoryFile with unregistered sound bank data!");
            }
        }
        else
        {
            g_audioImplLogger_nosound.Log(eALT_ERROR, "Called UnregisterInMemoryFile with nullptr AudioFileEntryData!");
        }
        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::ParseAudioFileEntry(const XmlNodeRef pAudioFileEntryNode, SATLAudioFileEntryInfo* const pFileEntryInfo)
    {
        auto eResult = eARS_FAILURE;

        // not loading any soundbank file for NOSOUND implementation
        if (pFileEntryInfo)
        {
            pFileEntryInfo->pFileData = nullptr;
            pFileEntryInfo->nMemoryBlockAlignment = s_nosoundDefaultMemoryAlignment;
            pFileEntryInfo->nSize = 0;
            pFileEntryInfo->sFileName = "";
            pFileEntryInfo->pImplData = azcreate(IATLAudioFileEntryData, (), Audio::AudioImplAllocator, "ATLAudioFileEntryData");

            eResult = eARS_SUCCESS;
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::DeleteAudioFileEntryData(IATLAudioFileEntryData* const pOldAudioFileEntry)
    {
        if (!pOldAudioFileEntry)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called DeleteAudioFileEntryData with null file entry data!");
        }
        azdestroy(pOldAudioFileEntry, Audio::AudioImplAllocator);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* const CAudioSystemImpl_NoSound::GetAudioFileLocation(SATLAudioFileEntryInfo* const pFileEntryInfo)
    {
        char const* sResult = nullptr;

        // ly-todo: localized and regular sound bank folders
        //if (pFileEntryInfo)
        //{
        //  sResult = pFileEntryInfo->bLocalized ? m_sLocalizedSoundBankFolder.c_str() : m_sRegularSoundBankFolder.c_str();
        //}

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const IATLTriggerImplData* CAudioSystemImpl_NoSound::NewAudioTriggerImplData(const XmlNodeRef pAudioTriggerNode)
    {
        if (!pAudioTriggerNode)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called NewAudioTriggerImplData with null audio trigger xml node!");
            return nullptr;
        }
        auto pNewTriggerImpl = azcreate(IATLTriggerImplData, (), Audio::AudioImplAllocator, "ATLTriggerImplData");
        return pNewTriggerImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::DeleteAudioTriggerImplData(const IATLTriggerImplData* const pOldTriggerImpl)
    {
        if (!pOldTriggerImpl)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called DeleteAudioTriggerImplData with null object!");
            return;
        }
        m_registeredTriggerCallbackInfo.erase(pOldTriggerImpl); // delete prepare/unprepare trigger anyway even it's not completed
        azdestroy(const_cast<IATLTriggerImplData*>(pOldTriggerImpl), Audio::AudioImplAllocator);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const IATLRtpcImplData* CAudioSystemImpl_NoSound::NewAudioRtpcImplData(const XmlNodeRef pAudioRtpcNode)
    {
        if (!pAudioRtpcNode)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called NewAudioRtpcImplData with null audio Rtpc xml node!");
            return nullptr;
        }
        auto pNewRtpcImpl = azcreate(IATLRtpcImplData, (), Audio::AudioImplAllocator, "ATLRtpcImplData");
        m_registeredRtpcData.insert(pNewRtpcImpl);
        return pNewRtpcImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::DeleteAudioRtpcImplData(const IATLRtpcImplData* const pOldRtpcImpl)
    {
        if (!pOldRtpcImpl)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called DeleteAudioRtpcImplData with null object!");
            return;
        }
        m_registeredRtpcData.erase(pOldRtpcImpl);
        azdestroy(const_cast<IATLRtpcImplData*>(pOldRtpcImpl), Audio::AudioImplAllocator);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const IATLSwitchStateImplData* CAudioSystemImpl_NoSound::NewAudioSwitchStateImplData(const XmlNodeRef pAudioSwitchNode)
    {
        if (!pAudioSwitchNode)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called NewAudioSwitchStateImplData with null audio switch state xml node!");
            return nullptr;
        }
        auto pNewSwitchImpl = azcreate(IATLSwitchStateImplData, (), Audio::AudioImplAllocator, "ATLSwitchStateImplData");
        m_registeredSwitchStateData.insert(pNewSwitchImpl);
        return pNewSwitchImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::DeleteAudioSwitchStateImplData(const IATLSwitchStateImplData* const pOldSwitchState)
    {
        if (!pOldSwitchState)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called DeleteAudioSwitchStateImplData with null object!");
            return;
        }
        m_registeredSwitchStateData.erase(pOldSwitchState);
        azdestroy(const_cast<IATLSwitchStateImplData*>(pOldSwitchState), Audio::AudioImplAllocator);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const IATLEnvironmentImplData* CAudioSystemImpl_NoSound::NewAudioEnvironmentImplData(const XmlNodeRef pAudioEnvironmentNode)
    {
        if (!pAudioEnvironmentNode)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called NewAudioEnvironmentImplData with null audio environment xml node!");
            return nullptr;
        }
        auto pNewEnvironmentImpl = azcreate(IATLEnvironmentImplData, (), Audio::AudioImplAllocator, "ATLEnvironmentImplData");
        m_registeredEnvironmentData.insert(pNewEnvironmentImpl);
        return pNewEnvironmentImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::DeleteAudioEnvironmentImplData(const IATLEnvironmentImplData* const pOldEnvironmentImpl)
    {
        if (!pOldEnvironmentImpl)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called DeleteAudioEnvironmentImplData with null object!");
            return;
        }
        m_registeredEnvironmentData.erase(pOldEnvironmentImpl);
        azdestroy(const_cast<IATLEnvironmentImplData*>(pOldEnvironmentImpl), Audio::AudioImplAllocator);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLAudioObjectData* CAudioSystemImpl_NoSound::NewGlobalAudioObjectData(const TAudioObjectID nObjectID)
    {
        if (nObjectID == INVALID_AUDIO_OBJECT_ID)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called NewGlobalAudioObjectData with invalid object id!");
            return nullptr;
        }
        auto pNewObject = azcreate(IATLAudioObjectData, (), Audio::AudioImplAllocator, "ATLAudioObjectData-Global");
        return pNewObject;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLAudioObjectData* CAudioSystemImpl_NoSound::NewAudioObjectData(const TAudioObjectID nObjectID)
    {
        if (nObjectID == INVALID_AUDIO_OBJECT_ID)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called NewAudioObjectData with invalid object id!");
            return nullptr;
        }
        auto pNewObject = azcreate(IATLAudioObjectData, (), Audio::AudioImplAllocator, "ATLAudioObjectData");
        return pNewObject;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::DeleteAudioObjectData(IATLAudioObjectData* const pOldObject)
    {
        if (!pOldObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called DeleteAudioObjectData with null object!");
            return;
        }
        azdestroy(pOldObject, Audio::AudioImplAllocator);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLListenerData* CAudioSystemImpl_NoSound::NewDefaultAudioListenerObjectData(const TATLIDType nListenerID)
    {
        auto pNewObject = azcreate(IATLListenerData, (), Audio::AudioImplAllocator, "ATLListenerData-Default");
        return pNewObject;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLListenerData* CAudioSystemImpl_NoSound::NewAudioListenerObjectData(const TATLIDType nListenerID)
    {
        auto pNewObject = azcreate(IATLListenerData, (), Audio::AudioImplAllocator, "ATLListenerData");
        return pNewObject;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::DeleteAudioListenerObjectData(IATLListenerData* const pOldListener)
    {
        if (!pOldListener)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called DeleteAudioListenerObjectData with null object!");
            return;
        }
        azdestroy(pOldListener, Audio::AudioImplAllocator);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    IATLEventData* CAudioSystemImpl_NoSound::NewAudioEventData(const TAudioEventID nEventID)
    {
        // how to validate the event id??
        auto pNewEvent = azcreate(IATLEventData, (), Audio::AudioImplAllocator, "ATLEventData");
        return pNewEvent;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::DeleteAudioEventData(IATLEventData* const pOldEventImpl)
    {
        if (!pOldEventImpl)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called DeleteAudioEventData with null object!");
            return;
        }
        azdestroy(pOldEventImpl, Audio::AudioImplAllocator);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::ResetAudioEventData(IATLEventData* const pEventData)
    {
        if (!pEventData)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called ResetAudioEventData with null object!");
            return;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    const char* const CAudioSystemImpl_NoSound::GetImplSubPath() const
    {
        return s_nosoundImplSubPath;
    }

    //////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::SetLanguage(const char* const sLanguage)
    {
        if (!sLanguage)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called SetLanguage with null language string!");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioSystemImpl_NoSound::GetMemoryInfo(SAudioImplMemoryInfo& oMemoryInfo) const
    {
        oMemoryInfo.nPrimaryPoolSize = AZ::AllocatorInstance<Audio::AudioImplAllocator>::Get().Capacity();
        oMemoryInfo.nPrimaryPoolUsedSize = AZ::AllocatorInstance<Audio::AudioImplAllocator>::Get().NumAllocatedBytes();
        oMemoryInfo.nPrimaryPoolAllocations = 0;

    #if defined(PROVIDE_AUDIO_IMPL_SECONDARY_POOL)
        oMemoryInfo.nSecondaryPoolSize = g_audioImplMemoryPoolSecondary_nosound.MemSize();
        oMemoryInfo.nSecondaryPoolUsedSize = oMemoryInfo.nSecondaryPoolSize - g_audioImplMemoryPoolSecondary_nosound.MemFree();
        oMemoryInfo.nSecondaryPoolAllocations = g_audioImplMemoryPoolSecondary_nosound.FragmentCount();
    #else
        oMemoryInfo.nSecondaryPoolSize = 0;
        oMemoryInfo.nSecondaryPoolUsedSize = 0;
        oMemoryInfo.nSecondaryPoolAllocations = 0;
    #endif // PROVIDE_AUDIO_IMPL_SECONDARY_POOL
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::PrepUnprepTriggerSync(IATLAudioObjectData* const pAudioObject, const IATLTriggerImplData* const pTrigger, bool bPrepare)
    {
        string sPrepare = bPrepare ? "PrepareTriggerSync" : "UnprepareTriggerSync";

        // by referring to Wwise implementation, pAudioObject is not used any where in the function
        // so just send warning but not return FAILURE
        if (!pAudioObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called %s with nullptr audio data!", sPrepare.c_str());
        }

        if (m_registeredAudioObjects.find(pAudioObject) == m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called %s on an audio data that has not been registered!", sPrepare.c_str());
        }

        // check validity of pTrigger
        if (!pTrigger)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called %s with nullptr trigger data!", sPrepare.c_str());
            return eARS_FAILURE;
        }

        // NOTE: as discussion went, we don't need to register the trigger to track

        // since this is synchronous, we can do validation of inputs to the degree that feels correct,
        // then assume that the preparation has succeeded.  successive calls to prepare event should succeed as well because it has already
        // happened and isn't interrupting an asynchronous call.
        //
        // only need to track if the trigger is prepared or not
        if (m_registeredTriggerCallbackInfo.count(pTrigger) > 0)
        {
            if (m_registeredTriggerCallbackInfo[pTrigger]->m_isPrepared == bPrepare) // Wwise: prepare twice is ok
            {
                g_audioImplLogger_nosound.Log(eALT_WARNING, "Called %s repeatedly with same trigger data!", sPrepare.c_str());
                return eARS_SUCCESS;
            }
        }
        else
        {
            m_registeredTriggerCallbackInfo[pTrigger] = new STriggerCallbackInfo();
        }

        if (bPrepare)
        {
            m_registeredTriggerCallbackInfo[pTrigger]->PrepareEvent();
        }
        else
        {
            m_registeredTriggerCallbackInfo[pTrigger]->UnprepareEvent();
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioSystemImpl_NoSound::PrepUnprepTriggerAsync(IATLAudioObjectData* const pAudioObject, const IATLTriggerImplData* const pTrigger, IATLEventData* const pEvent, bool bPrepare)
    {
        string sPrepare = bPrepare ? "PrepareTriggerAsync" : "UnprepareTriggerAsync";

        // by referring to Wwise implementation, pAudioObject is not used any where in the function
        // so just send warning but not return FAILURE
        if (!pAudioObject)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called %s with nullptr audio data!", sPrepare.c_str());
        }

        if (m_registeredAudioObjects.find(pAudioObject) == m_registeredAudioObjects.end())
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called  %s on an audio data that has not been registered!", sPrepare.c_str());
        }

        // check validity of pTrigger
        if (!pTrigger)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called %s with nullptr trigger data!", sPrepare.c_str());
            return eARS_FAILURE;
        }

        if (!pEvent)
        {
            g_audioImplLogger_nosound.Log(eALT_WARNING, "Called %s with nullptr event data!", sPrepare.c_str());
            return eARS_FAILURE;
        }

        if (m_registeredTriggerCallbackInfo.count(pTrigger) > 0)
        {
            if (m_registeredTriggerCallbackInfo[pTrigger]->m_isPrepared == bPrepare) // cannot prepare/unprepare asynchronously a trigger twice
            {
                g_audioImplLogger_nosound.Log(eALT_WARNING, "Called %s repeatedly with same trigger data!", sPrepare.c_str());
            }
        }
        else
        {
            m_registeredTriggerCallbackInfo[pTrigger] = new STriggerCallbackInfo();
        }

        if (bPrepare)
        {
            m_registeredTriggerCallbackInfo[pTrigger]->PrepareEvent();
            m_registeredTriggerCallbackInfo[pTrigger]->SendEvent(PrepareEventCallback, pEvent);
        }
        else
        {
            m_registeredTriggerCallbackInfo[pTrigger]->UnprepareEvent();
            m_registeredTriggerCallbackInfo[pTrigger]->SendEvent(PrepareEventCallback, pEvent);
        }

        return eARS_SUCCESS;
    }
} // namespace Audio
