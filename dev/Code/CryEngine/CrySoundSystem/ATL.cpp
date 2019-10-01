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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "ATL.h"

#include <AzFramework/StringFunc/StringFunc.h>

#include <SoundCVars.h>
#include <AudioProxy.h>
#include <ATLAudioObject.h>
#include <IAudioSystemImplementation.h>
#include <ISystem.h>
#include <IPhysics.h>
#include <IRenderAuxGeom.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
    #include <AzCore/Math/Color.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    inline EAudioRequestResult ConvertToRequestResult(const EAudioRequestStatus eAudioRequestStatus)
    {
        EAudioRequestResult eResult;

        switch (eAudioRequestStatus)
        {
            case eARS_SUCCESS:
            {
                eResult = eARR_SUCCESS;
                break;
            }
            case eARS_FAILURE:
            case eARS_FAILURE_INVALID_OBJECT_ID:
            case eARS_FAILURE_INVALID_CONTROL_ID:
            case eARS_FAILURE_INVALID_REQUEST:
            case eARS_PARTIAL_SUCCESS:
            {
                eResult = eARR_FAILURE;
                break;
            }
            default:
            {
                g_audioLogger.Log(eALT_ERROR, "Invalid AudioRequestStatus '%u'. Cannot be converted to an AudioRequestResult. ", eAudioRequestStatus);
                AZ_Assert(false, "Invalid AudioRequestStatus '%u'.  Cannot be converted to and AudioRequestResult.", eAudioRequestStatus);
                eResult = eARR_FAILURE;
                break;
            }
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioTranslationLayer::CAudioTranslationLayer()
        : m_pGlobalAudioObject(nullptr)
        , m_nGlobalAudioObjectID(GLOBAL_AUDIO_OBJECT_ID)
        , m_nTriggerInstanceIDCounter(1)
        , m_nextSourceId(INVALID_AUDIO_SOURCE_ID)
        , m_oAudioEventMgr()
        , m_oAudioObjectMgr(m_oAudioEventMgr)
        , m_oAudioListenerMgr()
        , m_oFileCacheMgr(m_cPreloadRequests)
        , m_oAudioEventListenerMgr()
        , m_oXmlProcessor(m_cTriggers, m_cRtpcs, m_cSwitches, m_cEnvironments, m_cPreloadRequests, m_oFileCacheMgr)
        , m_nFlags(eAIS_NONE)
    {
        gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

        InitATLControlIDs();

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        m_oAudioEventMgr.SetDebugNameStore(&m_oDebugNameStore);
        m_oAudioObjectMgr.SetDebugNameStore(&m_oDebugNameStore);
        m_oXmlProcessor.SetDebugNameStore(&m_oDebugNameStore);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioTranslationLayer::~CAudioTranslationLayer()
    {
        // By the time ATL is being destroyed, ReleaseImplComponent should have been called already
        // to release the implementation object.  See CAudioSystem::Release().

        gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::Initialize()
    {
        // Add the callback for the obstruction calculation.
        gEnv->pPhysicalWorld->AddEventClient(
            EventPhysRWIResult::id,
            &CATLAudioObject::CPropagationProcessor::OnObstructionTest,
            1);

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::ShutDown()
    {
        if (gEnv->pPhysicalWorld)
        {
            // remove the callback for the obstruction calculation
            gEnv->pPhysicalWorld->RemoveEventClient(
                EventPhysRWIResult::id,
                &CATLAudioObject::CPropagationProcessor::OnObstructionTest,
                1);
        }

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::ProcessRequest(CAudioRequestInternal& rRequest)
    {
        EAudioRequestStatus eResult = eARS_NONE;

        switch (rRequest.pData->eRequestType)
        {
            case eART_AUDIO_OBJECT_REQUEST:
            {
                eResult = ProcessAudioObjectRequest(rRequest);
                break;
            }
            case eART_AUDIO_LISTENER_REQUEST:
            {
                eResult = ProcessAudioListenerRequest(rRequest);
                break;
            }
            case eART_AUDIO_CALLBACK_MANAGER_REQUEST:
            {
                eResult = ProcessAudioCallbackManagerRequest(rRequest.pData);
                break;
            }
            case eART_AUDIO_MANAGER_REQUEST:
            {
                eResult = ProcessAudioManagerRequest(rRequest);
                break;
            }
            default:
            {
                g_audioLogger.Log(eALT_ERROR, "Unknown audio request type: %d", static_cast<int>(rRequest.pData->eRequestType));
                break;
            }
        }

        rRequest.eStatus = eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::Update(const float fUpdateIntervalMS)
    {
        UpdateSharedData();

        m_oAudioEventMgr.Update(fUpdateIntervalMS);
        m_oAudioObjectMgr.Update(fUpdateIntervalMS, m_oSharedData.m_oActiveListenerPosition);
        m_oAudioListenerMgr.Update(fUpdateIntervalMS);
        m_oFileCacheMgr.Update();

        AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::Update, fUpdateIntervalMS);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioControlID CAudioTranslationLayer::GetAudioTriggerID(const char* const sAudioTriggerName) const
    {
        auto nTriggerID = AudioStringToID<TAudioControlID>(sAudioTriggerName);

        if (!stl::find_in_map(m_cTriggers, nTriggerID, nullptr))
        {
            nTriggerID = INVALID_AUDIO_CONTROL_ID;
        }

        return nTriggerID;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioControlID CAudioTranslationLayer::GetAudioRtpcID(const char* const sAudioRtpcName) const
    {
        auto nRtpcID = AudioStringToID<TAudioControlID>(sAudioRtpcName);

        if (!stl::find_in_map(m_cRtpcs, nRtpcID, nullptr))
        {
            nRtpcID = INVALID_AUDIO_CONTROL_ID;
        }

        return nRtpcID;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioControlID CAudioTranslationLayer::GetAudioSwitchID(const char* const sAudioStateName) const
    {
        auto nSwitchID = AudioStringToID<TAudioControlID>(sAudioStateName);

        if (!stl::find_in_map(m_cSwitches, nSwitchID, nullptr))
        {
            nSwitchID = INVALID_AUDIO_CONTROL_ID;
        }

        return nSwitchID;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioSwitchStateID CAudioTranslationLayer::GetAudioSwitchStateID(const TAudioControlID nSwitchID, const char * const sAudioSwitchStateName) const
    {
        TAudioSwitchStateID nStateID = AudioStringToID<TAudioSwitchStateID>(sAudioSwitchStateName);

        if (const auto pSwitch = stl::find_in_map(m_cSwitches, nSwitchID, nullptr))
        {
            if (!stl::find_in_map(pSwitch->cStates, nStateID, nullptr))
            {
                nStateID = INVALID_AUDIO_SWITCH_STATE_ID;
            }
        }

        return nStateID;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioPreloadRequestID CAudioTranslationLayer::GetAudioPreloadRequestID(const char* const sAudioPreloadRequestName) const
    {
        auto nAudioPreloadRequestID = AudioStringToID<TAudioPreloadRequestID>(sAudioPreloadRequestName);

        if (!stl::find_in_map(m_cPreloadRequests, nAudioPreloadRequestID, nullptr))
        {
            nAudioPreloadRequestID = INVALID_AUDIO_PRELOAD_REQUEST_ID;
        }

        return nAudioPreloadRequestID;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioEnvironmentID CAudioTranslationLayer::GetAudioEnvironmentID(const char* const sAudioEnvironmentName) const
    {
        auto nEnvironmentID = AudioStringToID<TAudioEnvironmentID>(sAudioEnvironmentName);

        if (!stl::find_in_map(m_cEnvironments, nEnvironmentID, nullptr))
        {
            nEnvironmentID = INVALID_AUDIO_CONTROL_ID;
        }

        return nEnvironmentID;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::ReserveAudioObjectID(TAudioObjectID& rAudioObjectID)
    {
        return m_oAudioObjectMgr.ReserveID(rAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::ReleaseAudioObjectID(const TAudioObjectID nAudioObjectID)
    {
        const bool bSuccess = m_oAudioObjectMgr.ReleaseID(nAudioObjectID);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        if (bSuccess)
        {
            m_oDebugNameStore.RemoveAudioObject(nAudioObjectID);
        }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::ReserveAudioListenerID(TAudioObjectID& rAudioObjectID)
    {
        return m_oAudioListenerMgr.ReserveID(rAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::ReleaseAudioListenerID(const TAudioObjectID nAudioObjectID)
    {
        return m_oAudioListenerMgr.ReleaseID(nAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::SetAudioListenerOverrideID(const TAudioObjectID nAudioObjectID)
    {
        return m_oAudioListenerMgr.SetOverrideListenerID(nAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::AddRequestListener(const SAudioEventListener& listener)
    {
        m_oAudioEventListenerMgr.AddRequestListener(listener);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::RemoveRequestListener(const SAudioEventListener& listener)
    {
        m_oAudioEventListenerMgr.RemoveRequestListener(listener);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ParseControlsData(const char* const pConfigFolderPath, const EATLDataScope eDataScope)
    {
        m_oXmlProcessor.ParseControlsData(pConfigFolderPath, eDataScope);
        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ParsePreloadsData(const char* const pConfigFolderPath, const EATLDataScope eDataScope)
    {
        m_oXmlProcessor.ParsePreloadsData(pConfigFolderPath, eDataScope);
        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ClearControlsData(const EATLDataScope eDataScope)
    {
        m_oXmlProcessor.ClearControlsData(eDataScope);
        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ClearPreloadsData(const EATLDataScope eDataScope)
    {
        m_oXmlProcessor.ClearPreloadsData(eDataScope);
        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const AZStd::string& CAudioTranslationLayer::GetControlsImplSubPath() const
    {
        return m_implSubPath;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioSourceId CAudioTranslationLayer::CreateAudioSource(const SAudioInputConfig& sourceConfig)
    {
        AZ_Assert(sourceConfig.m_sourceId == INVALID_AUDIO_SOURCE_ID, "ATL - Request to CreateAudioSource already contains a valid source Id.\n");

        TAudioSourceId sourceId = ++m_nextSourceId;
        SAudioRequest request;
        SAudioManagerRequestData<eAMRT_CREATE_SOURCE> requestData(sourceConfig);
        requestData.m_sourceConfig.m_sourceId = sourceId;
        request.nFlags = (eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING);
        request.pData = &requestData;

        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, request);
        return sourceId;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DestroyAudioSource(TAudioSourceId sourceId)
    {
        SAudioRequest request;
        SAudioManagerRequestData<eAMRT_DESTROY_SOURCE> requestData(sourceId);
        request.nFlags = (eARF_PRIORITY_NORMAL);
        request.pData = &requestData;

        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::NotifyListener(const CAudioRequestInternal& rRequest)
    {
        // This should always be the main thread!
        TATLEnumFlagsType nSpecificAudioRequest = INVALID_AUDIO_ENUM_FLAG_TYPE;
        TAudioControlID nAudioControlID = INVALID_AUDIO_CONTROL_ID;
        TAudioEventID audioEventID = INVALID_AUDIO_EVENT_ID;

        switch (rRequest.pData->eRequestType)
        {
            case eART_AUDIO_MANAGER_REQUEST:
            {
                auto const pRequestDataBase = static_cast<const SAudioManagerRequestDataInternalBase*>(rRequest.pData.get());
                nSpecificAudioRequest = static_cast<TATLEnumFlagsType>(pRequestDataBase->eType);
                break;
            }
            case eART_AUDIO_CALLBACK_MANAGER_REQUEST:
            {
                auto const pRequestDataBase = static_cast<const SAudioCallbackManagerRequestDataInternalBase*>(rRequest.pData.get());
                nSpecificAudioRequest = static_cast<TATLEnumFlagsType>(pRequestDataBase->eType);
                switch (pRequestDataBase->eType)
                {
                    case eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE:
                    {
                        auto const pRequestData = static_cast<const SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE>*>(pRequestDataBase);
                        nAudioControlID = pRequestData->nAudioTriggerID;
                        break;
                    }
                    case eACMRT_REPORT_STARTED_EVENT:
                    {
                        auto const pRequestData = static_cast<const SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_STARTED_EVENT>*>(pRequestDataBase);
                        audioEventID = pRequestData->nEventID;
                        break;
                    }
                }
                break;
            }
            case eART_AUDIO_OBJECT_REQUEST:
            {
                auto const pRequestDataBase = static_cast<const SAudioObjectRequestDataInternalBase*>(rRequest.pData.get());
                nSpecificAudioRequest = static_cast<TATLEnumFlagsType>(pRequestDataBase->eType);

                switch (pRequestDataBase->eType)
                {
                    case eAORT_EXECUTE_TRIGGER:
                    {
                        auto const pRequestData = static_cast<const SAudioObjectRequestDataInternal<eAORT_EXECUTE_TRIGGER>*>(pRequestDataBase);
                        nAudioControlID = pRequestData->nTriggerID;
                        break;
                    }
                }
                break;
            }
            case eART_AUDIO_LISTENER_REQUEST:
            {
                auto const pRequestDataBase = static_cast<const SAudioListenerRequestDataInternalBase*>(rRequest.pData.get());
                nSpecificAudioRequest = static_cast<TATLEnumFlagsType>(pRequestDataBase->eType);
                break;
            }
            default:
            {
                g_audioLogger.Log(eALT_FATAL, "Unknown request type during CAudioTranslationLayer::NotifyListener!");
                break;
            }
        }

        const SAudioRequestInfo oResult(
            ConvertToRequestResult(rRequest.eStatus),
            rRequest.pOwner,
            rRequest.pUserData,
            rRequest.pUserDataOwner,
            rRequest.pData->eRequestType,
            nSpecificAudioRequest,
            nAudioControlID,
            rRequest.nAudioObjectID,
            audioEventID);

        m_oAudioEventListenerMgr.NotifyListener(&oResult);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ProcessAudioManagerRequest(const CAudioRequestInternal& rRequest)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        if (rRequest.pData)
        {
            auto const pRequestDataBase = static_cast<const SAudioManagerRequestDataInternalBase*>(rRequest.pData.get());

            switch (pRequestDataBase->eType)
            {
                case eAMRT_RESERVE_AUDIO_OBJECT_ID:
                {
                    auto const pRequestData = static_cast<const SAudioManagerRequestDataInternal<eAMRT_RESERVE_AUDIO_OBJECT_ID>*>(rRequest.pData.get());
                #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                    eResult = BoolToARS(ReserveAudioObjectID(*pRequestData->pObjectID, pRequestData->sObjectName.c_str()));
                #else
                    eResult = BoolToARS(ReserveAudioObjectID(*pRequestData->pObjectID));
                #endif // INCLUDE_AUDIO_PRODUCTION_CODE
                    break;
                }
                case eAMRT_CREATE_SOURCE:
                {
                    auto const pRequestData = static_cast<const SAudioManagerRequestDataInternal<eAMRT_CREATE_SOURCE>*>(rRequest.pData.get());
                    bool result = false;
                    AudioSystemImplementationRequestBus::BroadcastResult(result, &AudioSystemImplementationRequestBus::Events::CreateAudioSource, pRequestData->m_sourceConfig);
                    eResult = BoolToARS(result);
                    break;
                }
                case eAMRT_DESTROY_SOURCE:
                {
                    auto const pRequestData = static_cast<const SAudioManagerRequestDataInternal<eAMRT_DESTROY_SOURCE>*>(rRequest.pData.get());
                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DestroyAudioSource, pRequestData->m_sourceId);
                    eResult = eARS_SUCCESS;
                    break;
                }
                case eAMRT_INIT_AUDIO_IMPL:
                {
                    auto const pRequestData = static_cast<const SAudioManagerRequestDataInternal<eAMRT_INIT_AUDIO_IMPL>*>(rRequest.pData.get());
                    eResult = InitializeImplComponent();

                    // Initializing the implementation failed, immediately release it...
                    if (eResult != eARS_SUCCESS)
                    {
                        ReleaseImplComponent();
                    }
                    break;
                }
                case eAMRT_RELEASE_AUDIO_IMPL:
                {
                    ReleaseImplComponent();
                    eResult = eARS_SUCCESS;
                    break;
                }
                case eAMRT_REFRESH_AUDIO_SYSTEM:
                {
                #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                    auto const pRequestData = static_cast<const SAudioManagerRequestDataInternal<eAMRT_REFRESH_AUDIO_SYSTEM>*>(rRequest.pData.get());
                    eResult = RefreshAudioSystem(pRequestData->m_controlsPath.c_str(), pRequestData->m_levelName.c_str(), pRequestData->m_levelPreloadId);
                #else
                    eResult = eARS_FAILURE_INVALID_REQUEST;
                #endif // INCLUDE_AUDIO_PRODUCTION_CODE
                    break;
                }
                case eAMRT_LOSE_FOCUS:
                {
                #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                    if (g_audioCVars.m_nIgnoreWindowFocus == 0 && (m_nFlags & eAIS_IS_MUTED) == 0)
                #endif // INCLUDE_AUDIO_PRODUCTION_CODE
                    {
                        const CATLTrigger* const pTrigger = stl::find_in_map(m_cTriggers, SATLInternalControlIDs::nLoseFocusTriggerID, nullptr);

                        if (pTrigger)
                        {
                            eResult = ActivateTrigger(m_pGlobalAudioObject, pTrigger, 0.0f);
                        }
                        else
                        {
                            g_audioLogger.Log(eALT_WARNING, "ATL - Trigger not found for: SATLInternalControlIDs::nLoseFocusTriggerID");
                        }

                        AudioSystemImplementationNotificationBus::Broadcast(&AudioSystemImplementationNotificationBus::Events::OnAudioSystemLoseFocus);
                    }
                    break;
                }
                case eAMRT_GET_FOCUS:
                {
                #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                    if (g_audioCVars.m_nIgnoreWindowFocus == 0 && (m_nFlags & eAIS_IS_MUTED) == 0)
                #endif // INCLUDE_AUDIO_PRODUCTION_CODE
                    {
                        AudioSystemImplementationNotificationBus::Broadcast(&AudioSystemImplementationNotificationBus::Events::OnAudioSystemGetFocus);

                        const CATLTrigger* const pTrigger = stl::find_in_map(m_cTriggers, SATLInternalControlIDs::nGetFocusTriggerID, nullptr);

                        if (pTrigger)
                        {
                            eResult = ActivateTrigger(m_pGlobalAudioObject, pTrigger, 0.0f);
                        }
                        else
                        {
                            g_audioLogger.Log(eALT_WARNING, "ATL - Trigger not found for: SATLInternalControlIDs::nGetFocusTriggerID");
                        }
                    }
                    break;
                }
                case eAMRT_MUTE_ALL:
                {
                    const CATLTrigger* const pTrigger = stl::find_in_map(m_cTriggers, SATLInternalControlIDs::nMuteAllTriggerID, nullptr);

                    if (pTrigger)
                    {
                        eResult = ActivateTrigger(m_pGlobalAudioObject, pTrigger, 0.0f);
                    }
                    else
                    {
                        g_audioLogger.Log(eALT_WARNING, "ATL - Trigger not found for: SATLInternalControlIDs::nMuteAllTriggerID");
                    }

                    if (eResult == eARS_SUCCESS)
                    {
                        m_nFlags |= eAIS_IS_MUTED;
                    }

                    AudioSystemImplementationNotificationBus::Broadcast(&AudioSystemImplementationNotificationBus::Events::OnAudioSystemMuteAll);
                    break;
                }
                case eAMRT_UNMUTE_ALL:
                {
                    const CATLTrigger* const pTrigger = stl::find_in_map(m_cTriggers, SATLInternalControlIDs::nUnmuteAllTriggerID, nullptr);

                    if (pTrigger)
                    {
                        eResult = ActivateTrigger(m_pGlobalAudioObject, pTrigger, 0.0f);
                    }
                    else
                    {
                        g_audioLogger.Log(eALT_WARNING, "ATL - Trigger not found for: SATLInternalControlIDs::nUnmuteAllTriggerID");
                    }

                    if (eResult == eARS_SUCCESS)
                    {
                        m_nFlags &= ~eAIS_IS_MUTED;
                    }

                    AudioSystemImplementationNotificationBus::Broadcast(&AudioSystemImplementationNotificationBus::Events::OnAudioSystemUnmuteAll);
                    break;
                }
                case eAMRT_STOP_ALL_SOUNDS:
                {
                    AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::StopAllSounds);
                    break;
                }
                case eAMRT_PARSE_CONTROLS_DATA:
                {
                    auto const pRequestData = static_cast<const SAudioManagerRequestDataInternal<eAMRT_PARSE_CONTROLS_DATA>*>(rRequest.pData.get());
                    eResult = ParseControlsData(pRequestData->sControlsPath.c_str(), pRequestData->eDataScope);
                    break;
                }
                case eAMRT_PARSE_PRELOADS_DATA:
                {
                    auto const pRequestData = static_cast<const SAudioManagerRequestDataInternal<eAMRT_PARSE_PRELOADS_DATA>*>(rRequest.pData.get());
                    eResult = ParsePreloadsData(pRequestData->sControlsPath.c_str(), pRequestData->eDataScope);
                    break;
                }
                case eAMRT_CLEAR_CONTROLS_DATA:
                {
                    auto const pRequestData = static_cast<const SAudioManagerRequestDataInternal<eAMRT_CLEAR_CONTROLS_DATA>*>(rRequest.pData.get());
                    eResult = ClearControlsData(pRequestData->eDataScope);
                    break;
                }
                case eAMRT_CLEAR_PRELOADS_DATA:
                {
                    auto const pRequestData = static_cast<const SAudioManagerRequestDataInternal<eAMRT_CLEAR_PRELOADS_DATA>*>(rRequest.pData.get());
                    eResult = ClearPreloadsData(pRequestData->eDataScope);
                    break;
                }
                case eAMRT_PRELOAD_SINGLE_REQUEST:
                {
                    auto const pRequestData = static_cast<const SAudioManagerRequestDataInternal<eAMRT_PRELOAD_SINGLE_REQUEST>*>(rRequest.pData.get());
                    eResult = m_oFileCacheMgr.TryLoadRequest(pRequestData->nPreloadRequest, ((rRequest.nFlags & eARF_EXECUTE_BLOCKING) != 0), pRequestData->bAutoLoadOnly);
                    break;
                }
                case eAMRT_UNLOAD_SINGLE_REQUEST:
                {
                    auto const pRequestData = static_cast<const SAudioManagerRequestDataInternal<eAMRT_UNLOAD_SINGLE_REQUEST>*>(rRequest.pData.get());
                    eResult = m_oFileCacheMgr.TryUnloadRequest(pRequestData->nPreloadRequest);
                    break;
                }
                case eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE:
                {
                    auto const pRequestData = static_cast<const SAudioManagerRequestDataInternal<eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE>*>(rRequest.pData.get());
                    eResult = m_oFileCacheMgr.UnloadDataByScope(pRequestData->eDataScope);
                    break;
                }
                case eAMRT_CHANGE_LANGUAGE:
                {
                    SetImplLanguage();

                    m_oFileCacheMgr.UpdateLocalizedFileCacheEntries();
                    eResult = eARS_SUCCESS;
                    break;
                }
                case eAMRT_SET_AUDIO_PANNING_MODE:
                {
                    auto const requestData = static_cast<const SAudioManagerRequestDataInternal<eAMRT_SET_AUDIO_PANNING_MODE>*>(rRequest.pData.get());
                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::SetPanningMode, requestData->m_panningMode);
                    eResult = eARS_SUCCESS;
                    break;
                }
                case eAMRT_DRAW_DEBUG_INFO:
                {
                #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                    DrawAudioSystemDebugInfo();
                    eResult = eARS_SUCCESS;
                #endif // INCLUDE_AUDIO_PRODUCTION_CODE
                    break;
                }
                case eAMRT_NONE:
                {
                    eResult = eARS_SUCCESS;
                    break;
                }
                default:
                {
                    g_audioLogger.Log(eALT_WARNING, "ATL received an unknown AudioManager request: %u", pRequestDataBase->eType);
                    eResult = eARS_FAILURE_INVALID_REQUEST;
                    break;
                }
            }
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ProcessAudioCallbackManagerRequest(const SAudioRequestDataInternal* pPassedRequestData)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        if (pPassedRequestData)
        {
            auto const pRequestDataBase = static_cast<const SAudioCallbackManagerRequestDataInternalBase*>(pPassedRequestData);

            switch (pRequestDataBase->eType)
            {
                case eACMRT_REPORT_STARTED_EVENT:
                {
                    auto const pRequestData = static_cast<const SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_STARTED_EVENT>*>(pPassedRequestData);
                    CATLEvent* const pEvent = m_oAudioEventMgr.LookupID(pRequestData->nEventID);

                    if (pEvent)
                    {
                        pEvent->m_audioEventState = eAES_PLAYING_DELAYED;

                        if (pEvent->m_nObjectID != m_nGlobalAudioObjectID)
                        {
                            m_oAudioObjectMgr.ReportStartedEvent(pEvent);
                        }
                        else
                        {
                            m_pGlobalAudioObject->ReportStartedEvent(pEvent);
                        }
                    }

                    eResult = eARS_SUCCESS;
                    break;
                }
                case eACMRT_REPORT_FINISHED_EVENT:
                {
                    auto const pRequestData = static_cast<const SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_FINISHED_EVENT>*>(pPassedRequestData);
                    CATLEvent* const pEvent = m_oAudioEventMgr.LookupID(pRequestData->nEventID);

                    if (pEvent)
                    {
                        if (pEvent->m_nObjectID != m_nGlobalAudioObjectID)
                        {
                            m_oAudioObjectMgr.ReportFinishedEvent(pEvent, pRequestData->bSuccess);
                        }
                        else if (m_pGlobalAudioObject)
                        {   // safety-net check to prevent crash if a finished callback is received very late
                            m_pGlobalAudioObject->ReportFinishedEvent(pEvent, pRequestData->bSuccess);
                        }

                        m_oAudioEventMgr.ReleaseEvent(pEvent);
                    }

                    eResult = eARS_SUCCESS;
                    break;
                }
                case eACMRT_REPORT_PROCESSED_OBSTRUCTION_RAY:
                {
                    auto const pRequestData = static_cast<const SAudioCallbackManagerRequestDataInternal<eACMRT_REPORT_PROCESSED_OBSTRUCTION_RAY>*>(pPassedRequestData);
                    m_oAudioObjectMgr.ReportObstructionRay(pRequestData->nObjectID, pRequestData->nRayID);

                    eResult = eARS_SUCCESS;
                    break;
                }
                case eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE:
                case eACMRT_NONE:
                {
                    eResult = eARS_SUCCESS;
                    break;
                }
                default:
                {
                    eResult = eARS_FAILURE_INVALID_REQUEST;
                    g_audioLogger.Log(eALT_WARNING, "ATL received an unknown AudioCallbackManager request: %u", pRequestDataBase->eType);
                    break;
                }
            }
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ProcessAudioObjectRequest(const CAudioRequestInternal& rRequest)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;
        CATLAudioObjectBase* pObject = static_cast<CATLAudioObjectBase*>(m_pGlobalAudioObject);

        if (rRequest.nAudioObjectID != INVALID_AUDIO_OBJECT_ID)
        {
            pObject = static_cast<CATLAudioObjectBase*>(m_oAudioObjectMgr.LookupID(rRequest.nAudioObjectID));
        }

        if (pObject)
        {
            const SAudioRequestDataInternal* const pPassedRequestData = rRequest.pData;

            if (pPassedRequestData)
            {
                auto const pBaseRequestData = static_cast<const SAudioObjectRequestDataInternalBase*>(pPassedRequestData);

                switch (pBaseRequestData->eType)
                {
                    case eAORT_PREPARE_TRIGGER:
                    {
                        auto const pRequestData = static_cast<const SAudioObjectRequestDataInternal<eAORT_PREPARE_TRIGGER>*>(pPassedRequestData);

                        const CATLTrigger* const pTrigger = stl::find_in_map(m_cTriggers, pRequestData->nTriggerID, nullptr);

                        if (pTrigger)
                        {
                            eResult = PrepUnprepTriggerAsync(pObject, pTrigger, true);
                        }
                        else
                        {
                            eResult = eARS_FAILURE_INVALID_CONTROL_ID;
                        }
                        break;
                    }
                    case eAORT_UNPREPARE_TRIGGER:
                    {
                        auto const pRequestData = static_cast<const SAudioObjectRequestDataInternal<eAORT_UNPREPARE_TRIGGER>*>(pPassedRequestData);

                        const CATLTrigger* const pTrigger = stl::find_in_map(m_cTriggers, pRequestData->nTriggerID, nullptr);

                        if (pTrigger)
                        {
                            eResult = PrepUnprepTriggerAsync(pObject, pTrigger, false);
                        }
                        else
                        {
                            eResult = eARS_FAILURE_INVALID_CONTROL_ID;
                        }
                        break;
                    }
                    case eAORT_EXECUTE_TRIGGER:
                    {
                        auto const pRequestData = static_cast<const SAudioObjectRequestDataInternal<eAORT_EXECUTE_TRIGGER>*>(pPassedRequestData);

                        const CATLTrigger* const pTrigger = stl::find_in_map(m_cTriggers, pRequestData->nTriggerID, nullptr);

                        if (pTrigger)
                        {
                            eResult = ActivateTrigger(
                                pObject,
                                pTrigger,
                                pRequestData->fTimeUntilRemovalInMS,
                                rRequest.pOwner,
                                rRequest.pUserData,
                                rRequest.pUserDataOwner,
                                rRequest.nFlags);
                        }
                        else
                        {
                            eResult = eARS_FAILURE_INVALID_CONTROL_ID;
                        }
                        break;
                    }
                    case eAORT_STOP_TRIGGER:
                    {
                        auto const pRequestData = static_cast<const SAudioObjectRequestDataInternal<eAORT_STOP_TRIGGER>*>(pPassedRequestData);

                        const CATLTrigger* const pTrigger = stl::find_in_map(m_cTriggers, pRequestData->nTriggerID, nullptr);

                        if (pTrigger)
                        {
                            eResult = StopTrigger(pObject, pTrigger);
                        }
                        else
                        {
                            eResult = eARS_FAILURE_INVALID_CONTROL_ID;
                        }
                        break;
                    }
                    case eAORT_STOP_ALL_TRIGGERS:
                    {
                        auto const pRequestData = static_cast<const SAudioObjectRequestDataInternal<eAORT_STOP_ALL_TRIGGERS>*>(pPassedRequestData);

                        if (pRequestData->m_filterByOwner)
                        {
                            StopAllTriggers(pObject, rRequest.pOwner);
                        }
                        else
                        {
                            StopAllTriggers(pObject);
                        }

                        // Why not return the result of StopAllTriggers call?
                        eResult = eARS_SUCCESS;
                        break;
                    }
                    case eAORT_SET_POSITION:
                    {
                        if (pObject->HasPosition())
                        {
                            auto const pRequestData = static_cast<const SAudioObjectRequestDataInternal<eAORT_SET_POSITION>*>(pPassedRequestData);

                            auto const pPositionedObject = static_cast<CATLAudioObject*>(pObject);

                            AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::SetPosition,
                                pPositionedObject->GetImplDataPtr(),
                                pRequestData->oPosition);

                            if (eResult == eARS_SUCCESS)
                            {
                                pPositionedObject->SetPosition(pRequestData->oPosition);
                            }
                        }
                        else
                        {
                            g_audioLogger.Log(eALT_WARNING, "ATL received a request to set a position on a global object");
                        }
                        break;
                    }
                    case eAORT_SET_RTPC_VALUE:
                    {
                        eResult = eARS_FAILURE_INVALID_CONTROL_ID;

                        auto const pRequestData = static_cast<const SAudioObjectRequestDataInternal<eAORT_SET_RTPC_VALUE>*>(pPassedRequestData);

                        const CATLRtpc* const pRtpc = stl::find_in_map(m_cRtpcs, pRequestData->nControlID, nullptr);

                        if (pRtpc)
                        {
                            eResult = SetRtpc(pObject, pRtpc, pRequestData->fValue);
                        }
                        break;
                    }
                    case eAORT_SET_SWITCH_STATE:
                    {
                        eResult = eARS_FAILURE_INVALID_CONTROL_ID;
                        auto const pRequestData = static_cast<const SAudioObjectRequestDataInternal<eAORT_SET_SWITCH_STATE>*>(pPassedRequestData);

                        const CATLSwitch* const pSwitch = stl::find_in_map(m_cSwitches, pRequestData->nSwitchID, nullptr);

                        if (pSwitch)
                        {
                            const CATLSwitchState* const pState = stl::find_in_map(pSwitch->cStates, pRequestData->nStateID, nullptr);

                            if (pState)
                            {
                                eResult = SetSwitchState(pObject, pState);
                            }
                        }
                        break;
                    }
                    case eAORT_SET_VOLUME:
                    {
                        eResult = eARS_FAILURE_INVALID_CONTROL_ID;
                        //TODO
                        break;
                    }
                    case eAORT_SET_ENVIRONMENT_AMOUNT:
                    {
                        if (pObject->HasPosition())
                        {
                            eResult = eARS_FAILURE_INVALID_CONTROL_ID;
                            auto const pRequestData = static_cast<const SAudioObjectRequestDataInternal<eAORT_SET_ENVIRONMENT_AMOUNT>*>(pPassedRequestData);

                            const CATLAudioEnvironment* const pEnvironment = stl::find_in_map(m_cEnvironments, pRequestData->nEnvironmentID, nullptr);

                            if (pEnvironment)
                            {
                                eResult = SetEnvironment(pObject, pEnvironment, pRequestData->fAmount);
                            }
                        }
                        else
                        {
                            g_audioLogger.Log(eALT_WARNING, "ATL received a request to set an environment on a global object");
                        }
                        break;
                    }
                    case eAORT_RESET_ENVIRONMENTS:
                    {
                        eResult = ResetEnvironments(pObject);
                        break;
                    }
                    case eAORT_RESET_RTPCS:
                    {
                        eResult = ResetRtpcs(pObject);
                        break;
                    }
                    case eAORT_RELEASE_OBJECT:
                    {
                        eResult = eARS_FAILURE;

                        const TAudioObjectID nObjectID = pObject->GetID();

                        if (nObjectID != m_nGlobalAudioObjectID)
                        {
                            if (ReleaseAudioObjectID(nObjectID))
                            {
                                eResult = eARS_SUCCESS;
                            }
                        }
                        else
                        {
                            g_audioLogger.Log(eALT_WARNING, "ATL received a request to release the GlobalAudioObject");
                        }
                        break;
                    }
                    case eAORT_EXECUTE_SOURCE_TRIGGER:
                    {
                        eResult = eARS_FAILURE;
                        auto const requestData = static_cast<const SAudioObjectRequestDataInternal<eAORT_EXECUTE_SOURCE_TRIGGER>*>(pPassedRequestData);

                        const CATLTrigger* const pTrigger = stl::find_in_map(m_cTriggers, requestData->m_triggerId, nullptr);
                        if (pTrigger)
                        {
                            SATLSourceData sourceData(requestData->m_sourceInfo);
                            eResult = ActivateTrigger(
                                pObject,
                                pTrigger,
                                0.f,
                                rRequest.pOwner,
                                rRequest.pUserData,
                                rRequest.pUserDataOwner,
                                rRequest.nFlags,
                                &sourceData
                            );
                        }
                        else
                        {
                            eResult = eARS_FAILURE_INVALID_CONTROL_ID;
                        }

                        break;
                    }
                    case eAORT_SET_MULTI_POSITIONS:
                    {
                        if (pObject->HasPosition())
                        {
                            auto const pRequestData = static_cast<const SAudioObjectRequestDataInternal<eAORT_SET_MULTI_POSITIONS>*>(pPassedRequestData);

                            auto const pPositionedObject = static_cast<CATLAudioObject*>(pObject);

                            AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::SetMultiplePositions,
                                pPositionedObject->GetImplDataPtr(),
                                pRequestData->m_params);

                            if (eResult == eARS_SUCCESS)
                            {
                                pPositionedObject->SetPosition(SATLWorldPosition());
                            }
                        }
                        else
                        {
                            g_audioLogger.Log(eALT_WARNING, "ATL received a request to set multiple positions on a global object");
                        }
                        break;
                    }
                    case eAORT_NONE:
                    {
                        eResult = eARS_SUCCESS;
                        break;
                    }
                    default:
                    {
                        eResult = eARS_FAILURE_INVALID_REQUEST;
                        g_audioLogger.Log(eALT_WARNING, "ATL received an unknown AudioObject request type: %u", pBaseRequestData->eType);
                        break;
                    }
                }
            }
        }
        else
        {
            g_audioLogger.Log(eALT_WARNING, "ATL received a request to a non-existent AudioObject: %u", rRequest.nAudioObjectID);
            eResult = eARS_FAILURE_INVALID_OBJECT_ID;
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ProcessAudioListenerRequest(const CAudioRequestInternal& rRequest)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;
        TAudioObjectID listenerID = INVALID_AUDIO_OBJECT_ID;

        // Check for an Audio Listener Override.
        TAudioObjectID overrideListenerID = m_oAudioListenerMgr.GetOverrideListenerID();
        if (overrideListenerID != INVALID_AUDIO_OBJECT_ID)
        {
            if (rRequest.nAudioObjectID == overrideListenerID)
            {
                // Have an override set, and the ID in the request matches the override.
                // Reroute to the default listener.
                listenerID = m_oAudioListenerMgr.GetDefaultListenerID();
            }
            else if (rRequest.nAudioObjectID != INVALID_AUDIO_OBJECT_ID)
            {
                // Override is set, but the request specified a different listener ID, allow it.
                listenerID = rRequest.nAudioObjectID;
            }
            else
            {
                // Override is set, but no listener ID specified.  Typically this would go
                // to the default listener, but with overrides we explicitly ignore this.
                return eResult;
            }
        }
        else if (rRequest.nAudioObjectID == INVALID_AUDIO_OBJECT_ID)
        {
            listenerID = m_oAudioListenerMgr.GetDefaultListenerID();
        }
        else
        {
            listenerID = rRequest.nAudioObjectID;
        }

        CATLListenerObject* const pListener = m_oAudioListenerMgr.LookupID(listenerID);
        if (pListener)
        {
            if (rRequest.pData)
            {
                auto const pBaseRequestData = static_cast<const SAudioListenerRequestDataInternalBase*>(rRequest.pData.get());

                switch (pBaseRequestData->eType)
                {
                    case eALRT_SET_POSITION:
                    {
                        auto const pRequestData = static_cast<const SAudioListenerRequestDataInternal<eALRT_SET_POSITION>*>(rRequest.pData.get());

                        AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::SetListenerPosition,
                            pListener->m_pImplData,
                            pRequestData->oNewPosition);

                        if (eResult == eARS_SUCCESS)
                        {
                            pListener->oPosition = pRequestData->oNewPosition;
                        }
                        break;
                    }
                    case eALRT_NONE:
                    {
                        eResult = eARS_SUCCESS;
                        break;
                    }
                    default:
                    {
                        eResult = eARS_FAILURE_INVALID_REQUEST;
                        g_audioLogger.Log(eALT_WARNING, "ATL received an unknown AudioListener request type: %u", pBaseRequestData->eType);
                        break;
                    }
                }
            }
        }
        else
        {
            g_audioLogger.Log(eALT_ERROR, "Could not find listener with ID: %u", listenerID);
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::InitializeImplComponent()
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::Initialize);
        if (eResult == eARS_SUCCESS)
        {
            IATLAudioObjectData* pGlobalObjectData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(pGlobalObjectData, &AudioSystemImplementationRequestBus::Events::NewGlobalAudioObjectData, m_nGlobalAudioObjectID);

            m_pGlobalAudioObject = azcreate(CATLGlobalAudioObject, (m_nGlobalAudioObjectID, pGlobalObjectData), Audio::AudioSystemAllocator, "ATLGlobalAudioObject");

            m_oAudioObjectMgr.Initialize();
            m_oAudioEventMgr.Initialize();
            m_oAudioListenerMgr.Initialize();
            m_oXmlProcessor.Initialize();
            m_oFileCacheMgr.Initialize();

            SetImplLanguage();

            // Update the implementation subpath for locating ATL controls...
            AudioSystemImplementationRequestBus::BroadcastResult(m_implSubPath, &AudioSystemImplementationRequestBus::Events::GetImplSubPath);
        }

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        else
        {
            const char* implementationName = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(implementationName, &AudioSystemImplementationRequestBus::Events::GetImplementationNameString);
            g_audioLogger.Log(eALT_ERROR, "Failed to Initialize the AudioSystemImplementationComponent '%s'\n", implementationName);
        }
#endif //INCLUDE_AUDIO_PRODUCTION_CODE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::ReleaseImplComponent()
    {
        // During audio middleware shutdown we do not allow for any new requests originating from the "dying" audio middleware!
        m_nFlags |= eAIS_AUDIO_MIDDLEWARE_SHUTTING_DOWN;

        m_oXmlProcessor.ClearControlsData(eADS_ALL);
        m_oXmlProcessor.ClearPreloadsData(eADS_ALL);

        if (m_pGlobalAudioObject)
        {
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioObjectData, m_pGlobalAudioObject->GetImplDataPtr());
            azdestroy(m_pGlobalAudioObject, Audio::AudioSystemAllocator);
            m_pGlobalAudioObject = nullptr;
        }

        m_oAudioObjectMgr.Release();
        m_oAudioListenerMgr.Release();
        m_oAudioEventMgr.Release();
        m_oFileCacheMgr.Release();
        m_oXmlProcessor.Release();

        m_implSubPath.clear();

        EAudioRequestStatus eResult = eARS_FAILURE;
        AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::ShutDown);

        // If we allow developers to change the audio implementation module at run-time, these should be at Warning level.
        // If we ever revoke that functionality, these should be promoted to Asserts.
        AZ_Warning("ATL", eResult == eARS_SUCCESS, "ATL ReleaseImplComponent - Shutting down the audio implementation failed!");

        AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::Release);
        AZ_Warning("ATL", eResult == eARS_SUCCESS, "ATL ReleaseImplComponent - Releasing the audio implementation failed!");

        m_nFlags &= ~eAIS_AUDIO_MIDDLEWARE_SHUTTING_DOWN;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::PrepUnprepTriggerAsync(
        CATLAudioObjectBase* const pAudioObject,
        const CATLTrigger* const pTrigger,
        const bool bPrepare)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        const TAudioObjectID nATLObjectID = pAudioObject->GetID();
        const TAudioControlID nATLTriggerID = pTrigger->GetID();
        const TObjectTriggerImplStates& rTriggerImplStates = pAudioObject->GetTriggerImpls();

        for (auto const triggerImpl : pTrigger->m_cImplPtrs)
        {
            TATLEnumFlagsType nTriggerImplFlags = INVALID_AUDIO_ENUM_FLAG_TYPE;
            TObjectTriggerImplStates::const_iterator iPlace = rTriggerImplStates.end();
            if (FindPlaceConst(rTriggerImplStates, triggerImpl->m_nATLID, iPlace))
            {
                nTriggerImplFlags = iPlace->second.nFlags;
            }

            const EATLSubsystem eReceiver = triggerImpl->GetReceiver();
            CATLEvent* pEvent = m_oAudioEventMgr.GetEvent(eReceiver);

            EAudioRequestStatus ePrepUnprepResult = eARS_FAILURE;

            switch (eReceiver)
            {
                case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                {
                    if (bPrepare)
                    {
                        if (((nTriggerImplFlags & eATS_PREPARED) == 0) && ((nTriggerImplFlags & eATS_LOADING) == 0))
                        {
                            AudioSystemImplementationRequestBus::BroadcastResult(ePrepUnprepResult, &AudioSystemImplementationRequestBus::Events::PrepareTriggerAsync,
                                pAudioObject->GetImplDataPtr(),
                                triggerImpl->m_pImplData,
                                pEvent->m_pImplData);
                        }
                    }
                    else
                    {
                        if (((nTriggerImplFlags & eATS_PREPARED) != 0) && ((nTriggerImplFlags & eATS_UNLOADING) == 0))
                        {
                            AudioSystemImplementationRequestBus::BroadcastResult(ePrepUnprepResult, &AudioSystemImplementationRequestBus::Events::UnprepareTriggerAsync,
                                pAudioObject->GetImplDataPtr(),
                                triggerImpl->m_pImplData,
                                pEvent->m_pImplData);
                        }
                    }

                    if (ePrepUnprepResult ==  eARS_SUCCESS)
                    {
                        pEvent->m_nObjectID = nATLObjectID;
                        pEvent->m_nTriggerID = triggerImpl->m_nATLID;
                        pEvent->m_nTriggerImplID = triggerImpl->m_nATLID;

                        pEvent->m_audioEventState = bPrepare ? eAES_LOADING : eAES_UNLOADING;
                    }
                    break;
                }
                case eAS_ATL_INTERNAL:
                {
                    //TODO: handle this
                    break;
                }
                default:
                {
                    g_audioLogger.Log(eALT_ERROR, "Unknown ATL Recipient");
                    break;
                }
            }

            if (ePrepUnprepResult == eARS_SUCCESS)
            {
                pEvent->SetDataScope(pTrigger->GetDataScope());
                pAudioObject->ReportStartedEvent(pEvent);
                pAudioObject->IncrementRefCount();
                eResult = eARS_SUCCESS; // if at least one event fires, it is a success
            }
            else
            {
                m_oAudioEventMgr.ReleaseEvent(pEvent);
            }
        }

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        if (eResult != eARS_SUCCESS)
        {
            // No TriggerImpl produced an active event.
            g_audioLogger.Log(eALT_WARNING, "PrepUnprepTriggerAsync failed on AudioObject \"%s\" (ID: %u)", m_oDebugNameStore.LookupAudioObjectName(nATLObjectID), nATLObjectID);
        }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ActivateTrigger(
        CATLAudioObjectBase* const pAudioObject,
        const CATLTrigger* const pTrigger,
        const float fTimeUntilRemovalMS,
        void* const pOwner /* = nullptr */,
        void* const pUserData /* = nullptr */,
        void* const pUserDataOwner /* = nullptr */,
        const TATLEnumFlagsType nFlags /* = INVALID_AUDIO_ENUM_FLAG_TYPE */,
        const SATLSourceData* pSourceData /* = nullptr */)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        if (pAudioObject->HasPosition())
        {
            // If the AudioObject uses Obstruction/Occlusion then set the values before activating the trigger.
            auto const pPositionedAudioObject = static_cast<CATLAudioObject*>(pAudioObject);

            if (pPositionedAudioObject->CanRunObstructionOcclusion() && !m_oAudioObjectMgr.HasActiveEvents(pPositionedAudioObject))
            {
                pPositionedAudioObject->ResetObstructionOcclusion(m_oSharedData.m_oActiveListenerPosition);
            }
        }

        const TAudioControlID nATLTriggerID = pTrigger->GetID();

        // Sets eATS_STARTING on this TriggerInstance to avoid
        // reporting TriggerFinished while the events are being started.
        pAudioObject->ReportStartingTriggerInstance(m_nTriggerInstanceIDCounter, nATLTriggerID);

        for (auto const triggerImpl : pTrigger->m_cImplPtrs)
        {
            const EATLSubsystem eReceiver = triggerImpl->GetReceiver();
            CATLEvent* const pEvent = m_oAudioEventMgr.GetEvent(eReceiver);
            pEvent->m_pImplData->m_triggerId = nATLTriggerID;

            EAudioRequestStatus eActivateResult = eARS_FAILURE;

            switch (eReceiver)
            {
                case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                {
                    AudioSystemImplementationRequestBus::BroadcastResult(eActivateResult, &AudioSystemImplementationRequestBus::Events::ActivateTrigger,
                        pAudioObject->GetImplDataPtr(),
                        triggerImpl->m_pImplData,
                        pEvent->m_pImplData,
                        pSourceData);
                    break;
                }
                case eAS_ATL_INTERNAL:
                {
                    eActivateResult = ActivateInternalTrigger(
                            pAudioObject,
                            triggerImpl->m_pImplData,
                            pEvent->m_pImplData);
                    break;
                }
                default:
                {
                    g_audioLogger.Log(eALT_ERROR, "Unknown ATL Recipient");
                    break;
                }
            }

            if (eActivateResult == eARS_SUCCESS || eActivateResult == eARS_PENDING)
            {
                pEvent->m_nObjectID = pAudioObject->GetID();
                pEvent->m_nTriggerID = nATLTriggerID;
                pEvent->m_nTriggerImplID = triggerImpl->m_nATLID;
                pEvent->m_nTriggerInstanceID = m_nTriggerInstanceIDCounter;
                pEvent->SetDataScope(pTrigger->GetDataScope());

                if (eActivateResult == eARS_SUCCESS)
                {
                    pEvent->m_audioEventState = eAES_PLAYING;
                }
                else if (eActivateResult == eARS_PENDING)
                {
                    pEvent->m_audioEventState = eAES_LOADING;
                }

                pAudioObject->ReportStartedEvent(pEvent);
                pAudioObject->IncrementRefCount();

                // If at least one event fires, it is a success: the trigger has been activated.
                eResult = eARS_SUCCESS;
            }
            else
            {
                m_oAudioEventMgr.ReleaseEvent(pEvent);
            }
        }

        // Either removes the eATS_STARTING flag on this trigger instance or removes it if no event was started.
        pAudioObject->ReportStartedTriggerInstance(m_nTriggerInstanceIDCounter++, pOwner, pUserData, pUserDataOwner, nFlags);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        if (eResult != eARS_SUCCESS)
        {
            // No TriggerImpl generated an active event.
            g_audioLogger.Log(eALT_WARNING, "Trigger \"%s\" failed on AudioObject \"%s\" (ID: %u)", m_oDebugNameStore.LookupAudioTriggerName(nATLTriggerID), m_oDebugNameStore.LookupAudioObjectName(pAudioObject->GetID()), pAudioObject->GetID());
        }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::StopTrigger(
        CATLAudioObjectBase* const pAudioObject,
        const CATLTrigger* const pTrigger)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        const TAudioObjectID nATLObjectID = pAudioObject->GetID();
        const TAudioControlID nATLTriggerID = pTrigger->GetID();

        TObjectEventSet rEvents = pAudioObject->GetActiveEvents();

        for (auto eventId : rEvents)
        {
            const CATLEvent* const pEvent = m_oAudioEventMgr.LookupID(eventId);

            if (pEvent && pEvent->IsPlaying() && (pEvent->m_nTriggerID == nATLTriggerID))
            {
                switch (pEvent->m_eSender)
                {
                    case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                    {
                        AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::StopEvent,
                            pAudioObject->GetImplDataPtr(),
                            pEvent->m_pImplData);
                        break;
                    }
                    case eAS_ATL_INTERNAL:
                    {
                        eResult = StopInternalEvent(pAudioObject, pEvent->m_pImplData);
                        break;
                    }
                    default:
                    {
                        g_audioLogger.Log(eALT_ERROR, "ATL - StopTrigger: Unknown ATL Recipient");
                        break;
                    }
                }
            }
        }

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::StopAllTriggers(CATLAudioObjectBase* const pAudioObject, void* const pOwner /* = nullptr*/)
    {
        if (!pOwner)
        {
            EAudioRequestStatus eResult = eARS_FAILURE;
            AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::StopAllEvents, pAudioObject->GetImplDataPtr());
            return eResult;
        }
        else
        {
            EAudioRequestStatus eResult = eARS_SUCCESS;

            auto triggerInstances = pAudioObject->GetTriggerInstancesByOwner(pOwner);

            auto activeEvents = pAudioObject->GetActiveEvents();

            for (auto eventId : activeEvents)
            {
                const CATLEvent* const atlEvent = m_oAudioEventMgr.LookupID(eventId);

                if (atlEvent && triggerInstances.find(atlEvent->m_nTriggerInstanceID) != triggerInstances.end())
                {
                    EAudioRequestStatus eSingleResult = eARS_FAILURE;

                    switch (atlEvent->m_eSender)
                    {
                        case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                        {
                            AudioSystemImplementationRequestBus::BroadcastResult(eSingleResult, &AudioSystemImplementationRequestBus::Events::StopEvent,
                                pAudioObject->GetImplDataPtr(),
                                atlEvent->m_pImplData);
                            break;
                        }

                        case eAS_ATL_INTERNAL:
                        {
                            eSingleResult = StopInternalEvent(pAudioObject, atlEvent->m_pImplData);
                            break;
                        }

                        default:
                        {
                            g_audioLogger.Log(eALT_ERROR, "ATL - StopAllTriggersFiltered: Unknown ATL Recipient");
                            break;
                        }
                    }

                    if (eSingleResult != eARS_SUCCESS)
                    {
                        eResult = eARS_FAILURE;
                    }
                }
            }

            return eResult;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::SetSwitchState(
        CATLAudioObjectBase* const pAudioObject,
        const CATLSwitchState* const pState)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        for (auto switchStateImpl : pState->m_cImplPtrs)
        {
            const EATLSubsystem eReceiver = switchStateImpl->GetReceiver();
            EAudioRequestStatus eSetStateResult = eARS_FAILURE;

            switch (eReceiver)
            {
                case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                {
                    AudioSystemImplementationRequestBus::BroadcastResult(eSetStateResult, &AudioSystemImplementationRequestBus::Events::SetSwitchState,
                        pAudioObject->GetImplDataPtr(),
                        switchStateImpl->m_pImplData);
                    break;
                }
                case eAS_ATL_INTERNAL:
                {
                    eSetStateResult = SetInternalSwitchState(pAudioObject, switchStateImpl->m_pImplData);
                    break;
                }
                default:
                {
                    g_audioLogger.Log(eALT_ERROR, "Unknown ATL Recipient");
                    break;
                }
            }

            if (eSetStateResult == eARS_SUCCESS)
            {
                eResult = eARS_SUCCESS;// if at least one of the implementations is set successfully, it is a success
            }
        }

        if (eResult == eARS_SUCCESS)
        {
            pAudioObject->SetSwitchState(pState->GetParentID(), pState->GetID());
        }
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        else
        {
            const char* const sSwitchName = m_oDebugNameStore.LookupAudioSwitchName(pState->GetParentID());
            const char* const sSwitchStateName = m_oDebugNameStore.LookupAudioSwitchStateName(pState->GetParentID(), pState->GetID());
            const char* const sAudioObjectName = m_oDebugNameStore.LookupAudioObjectName(pAudioObject->GetID());
            g_audioLogger.Log(eALT_WARNING, "Failed to set the ATLSwitch \"%s\" to ATLSwitchState \"%s\" on AudioObject \"%s\" (ID: %u)", sSwitchName, sSwitchStateName, sAudioObjectName, pAudioObject->GetID());
        }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::SetRtpc(
        CATLAudioObjectBase* const pAudioObject,
        const CATLRtpc* const pRtpc,
        const float fValue)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        for (auto rtpcImpl : pRtpc->m_cImplPtrs)
        {
            const EATLSubsystem eReceiver = rtpcImpl->GetReceiver();
            EAudioRequestStatus eSetRtpcResult = eARS_FAILURE;

            switch (eReceiver)
            {
                case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                {
                    AudioSystemImplementationRequestBus::BroadcastResult(eSetRtpcResult, &AudioSystemImplementationRequestBus::Events::SetRtpc,
                        pAudioObject->GetImplDataPtr(),
                        rtpcImpl->m_pImplData,
                        fValue);
                    break;
                }
                case eAS_ATL_INTERNAL:
                {
                    eSetRtpcResult = SetInternalRtpc(pAudioObject, rtpcImpl->m_pImplData, fValue);
                    break;
                }
                default:
                {
                    g_audioLogger.Log(eALT_ERROR, "Unknown ATL Recipient");
                    break;
                }
            }

            if (eSetRtpcResult == eARS_SUCCESS)
            {
                eResult = eARS_SUCCESS;// if at least one of the implementations is set successfully, it is a success
            }
        }

        if (eResult == eARS_SUCCESS)
        {
            pAudioObject->SetRtpc(pRtpc->GetID(), fValue);
        }
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        else
        {
            const char* const sRtpcName = m_oDebugNameStore.LookupAudioRtpcName(pRtpc->GetID());
            const char* const sAudioObjectName = m_oDebugNameStore.LookupAudioObjectName(pAudioObject->GetID());
            g_audioLogger.Log(eALT_WARNING, "Failed to set the ATLRtpc \"%s\" to %f on AudioObject \"%s\" (ID: %u)", sRtpcName, fValue, sAudioObjectName, pAudioObject->GetID());
        }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ResetRtpcs(CATLAudioObjectBase* const pAudioObject)
    {
        const TObjectRtpcMap rRtpcs = pAudioObject->GetRtpcs();
        EAudioRequestStatus eResult = eARS_SUCCESS;

        for (auto& rtpcPair : rRtpcs)
        {
            const CATLRtpc* const pRtpc = stl::find_in_map(m_cRtpcs, rtpcPair.first, nullptr);

            if (pRtpc)
            {
                for (auto rtpcImpl : pRtpc->m_cImplPtrs)
                {
                    EAudioRequestStatus eResetRtpcResult = eARS_FAILURE;
                    switch (rtpcImpl->GetReceiver())
                    {
                        case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                        {
                            AudioSystemImplementationRequestBus::BroadcastResult(eResetRtpcResult, &AudioSystemImplementationRequestBus::Events::ResetRtpc,
                                pAudioObject->GetImplDataPtr(),
                                rtpcImpl->m_pImplData);
                            break;
                        }

                        case eAS_ATL_INTERNAL:
                        {
                            // Implement internal Rtpcs later
                            eResetRtpcResult = eARS_SUCCESS;
                            break;
                        }

                        default:
                        {
                            g_audioLogger.Log(eALT_ERROR, "ATL - ResetRtpc: Unknown ATL Recipient");
                            break;
                        }
                    }

                    // If any reset failed, consider it an overall failure
                    if (eResetRtpcResult != eARS_SUCCESS)
                    {
                        eResult = eARS_FAILURE;
                    }
                }
            }
        }

        if (eResult == eARS_SUCCESS)
        {
            pAudioObject->ClearRtpcs();
        }
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        else
        {
            const TAudioObjectID objectId = pAudioObject->GetID();

            g_audioLogger.Log(eALT_WARNING,
                "Failed to Reset Rtpcs on AudioObject \"%s\" (ID: %u)",
                m_oDebugNameStore.LookupAudioObjectName(objectId),
                objectId);
        }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::SetEnvironment(
        CATLAudioObjectBase* const pAudioObject,
        const CATLAudioEnvironment* const pEnvironment,
        const float fAmount)
    {
        EAudioRequestStatus eResult = eARS_FAILURE;

        for (auto environmentImpl : pEnvironment->m_cImplPtrs)
        {
            const EATLSubsystem eReceiver = environmentImpl->GetReceiver();
            EAudioRequestStatus eSetEnvResult = eARS_FAILURE;

            switch (eReceiver)
            {
                case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                {
                    AudioSystemImplementationRequestBus::BroadcastResult(eSetEnvResult, &AudioSystemImplementationRequestBus::Events::SetEnvironment,
                        pAudioObject->GetImplDataPtr(),
                        environmentImpl->m_pImplData,
                        fAmount);
                    break;
                }
                case eAS_ATL_INTERNAL:
                {
                    eSetEnvResult = SetInternalEnvironment(pAudioObject, environmentImpl->m_pImplData, fAmount);
                    break;
                }
                default:
                {
                    g_audioLogger.Log(eALT_ERROR, "Unknown ATL Recipient");
                    break;
                }
            }

            if (eSetEnvResult == eARS_SUCCESS)
            {
                eResult = eARS_SUCCESS;// if at least one of the implementations is set successfully, it is a success
            }
        }

        if (eResult == eARS_SUCCESS)
        {
            pAudioObject->SetEnvironmentAmount(pEnvironment->GetID(), fAmount);
        }
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        else
        {
            const char* const sEnvironmentName = m_oDebugNameStore.LookupAudioEnvironmentName(pEnvironment->GetID());
            const char* const sAudioObjectName = m_oDebugNameStore.LookupAudioObjectName(pAudioObject->GetID());
            g_audioLogger.Log(eALT_WARNING, "Failed to set the ATLAudioEnvironment \"%s\" to %f on AudioObject \"%s\" (ID: %u)", sEnvironmentName, fAmount, sAudioObjectName, pAudioObject->GetID());
        }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ResetEnvironments(CATLAudioObjectBase* const pAudioObject)
    {
        const TObjectEnvironmentMap rEnvironments = pAudioObject->GetEnvironments();

        EAudioRequestStatus eResult = eARS_SUCCESS;

        for (auto& environmentAmountPair : rEnvironments)
        {
            const CATLAudioEnvironment* const pEnvironment = stl::find_in_map(m_cEnvironments, environmentAmountPair.first, nullptr);

            if (pEnvironment)
            {
                const EAudioRequestStatus eSetEnvResult = SetEnvironment(pAudioObject, pEnvironment, 0.0f);

                if (eSetEnvResult != eARS_SUCCESS)
                {
                    // If setting at least one Environment fails, we consider this a failure.
                    eResult = eARS_FAILURE;
                }
            }
        }

        if (eResult == eARS_SUCCESS)
        {
            pAudioObject->ClearEnvironments();
        }
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        else
        {
            const TAudioObjectID nObjectID = pAudioObject->GetID();

            g_audioLogger.Log(
                eALT_WARNING,
                "Failed to Reset AudioEnvironments on AudioObject \"%s\" (ID: %u)",
                m_oDebugNameStore.LookupAudioObjectName(nObjectID),
                nObjectID);
        }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

        return eResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::ActivateInternalTrigger(
        CATLAudioObjectBase* const pAudioObject,
        const IATLTriggerImplData* const pTriggerData,
        IATLEventData* const pEventData)
    {
        //TODO implement
        return eARS_FAILURE;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::StopInternalEvent(
        CATLAudioObjectBase* const pAudioObject,
        const IATLEventData* const pEventData)
    {
        //TODO implement
        return eARS_FAILURE;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::StopAllInternalEvents(CATLAudioObjectBase* const pAudioObject)
    {
        //TODO implement
        return eARS_FAILURE;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::SetInternalRtpc(
        CATLAudioObjectBase* const pAudioObject,
        const IATLRtpcImplData* const pRtpcData,
        const float fValue)
    {
        //TODO implement
        return eARS_FAILURE;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::SetInternalSwitchState(
        CATLAudioObjectBase* const pAudioObject,
        const IATLSwitchStateImplData* const pSwitchStateData)
    {
        auto const pInternalStateData = static_cast<const SATLSwitchStateImplData_internal*>(pSwitchStateData);

        //TODO: once there is more than one internal switch, a more sensible approach needs to be developed
        if (pInternalStateData->nATLInternalSwitchID == SATLInternalControlIDs::nObstructionOcclusionCalcSwitchID)
        {
            if (pAudioObject->HasPosition())
            {
                auto const pPositionedAudioObject = static_cast<CATLAudioObject*>(pAudioObject);

                if (pInternalStateData->nATLInternalStateID == SATLInternalControlIDs::nOOCStateIDs[eAOOCT_IGNORE])
                {
                    pPositionedAudioObject->SetObstructionOcclusionCalc(eAOOCT_IGNORE);
                    SATLSoundPropagationData oPropagationData;
                    pPositionedAudioObject->GetPropagationData(oPropagationData);
                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::SetObstructionOcclusion,
                        pPositionedAudioObject->GetImplDataPtr(),
                        oPropagationData.fObstruction,
                        oPropagationData.fOcclusion);
                }
                else if (pInternalStateData->nATLInternalStateID == SATLInternalControlIDs::nOOCStateIDs[eAOOCT_SINGLE_RAY])
                {
                    pPositionedAudioObject->SetObstructionOcclusionCalc(eAOOCT_SINGLE_RAY);
                }
                else if (pInternalStateData->nATLInternalStateID == SATLInternalControlIDs::nOOCStateIDs[eAOOCT_MULTI_RAY])
                {
                    pPositionedAudioObject->SetObstructionOcclusionCalc(eAOOCT_MULTI_RAY);
                }
                else
                {
                    g_audioLogger.Log(eALT_WARNING, "SetInternalSwitchState - Unknown value specified for SetObstructionOcclusionCalc");
                }
            }
        }
        else if (pInternalStateData->nATLInternalSwitchID == SATLInternalControlIDs::nObjectVelocityTrackingSwitchID)
        {
            if (pAudioObject->HasPosition())
            {
                auto const pPositionedAudioObject = static_cast<CATLAudioObject*>(pAudioObject);
                if (pInternalStateData->nATLInternalStateID == SATLInternalControlIDs::nOVTOnStateID)
                {
                    pPositionedAudioObject->SetVelocityTracking(true);
                }
                else if (pInternalStateData->nATLInternalStateID == SATLInternalControlIDs::nOVTOffStateID)
                {
                    pPositionedAudioObject->SetVelocityTracking(false);
                }
                else
                {
                    g_audioLogger.Log(eALT_WARNING, "SetInternalSwitchState - Unknown value specified for SetVelocityTracking (ly-fixit update this name!)");
                }
            }
        }

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::SetInternalEnvironment(
        CATLAudioObjectBase* const pAudioObject,
        const IATLEnvironmentImplData* const pEnvironmentImplData,
        const float fAmount)
    {
        // TODO: implement
        return eARS_FAILURE;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::UpdateSharedData()
    {
        m_oAudioListenerMgr.GetDefaultListenerPosition(m_oSharedData.m_oActiveListenerPosition);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::SetImplLanguage()
    {
        if (ICVar* pCVar = gEnv->pConsole->GetCVar("g_languageAudio"))
        {
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::SetLanguage, pCVar->GetString());
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        switch (event)
        {
            case ESYSTEM_EVENT_LEVEL_UNLOAD:
            {
                CATLAudioObject::CPropagationProcessor::s_canIssueRWIs = false;
                break;
            }
            case ESYSTEM_EVENT_LEVEL_GAMEPLAY_START:
            case ESYSTEM_EVENT_LEVEL_PRECACHE_START:
            {
                m_oAudioObjectMgr.ReleasePendingRays();
                CATLAudioObject::CPropagationProcessor::s_canIssueRWIs = true;
                break;
            }
            case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
            {
                m_oAudioObjectMgr.ReleasePendingRays();
                break;
            }
            // Disable Obstruction raycasts in AI/Physics mode (LY-66446)
            // Bugs were discovered related to async obstruction raycasting,
            // deeper discovery and fixes are needed, this is a temporary patch.
            //case ESYSTEM_EVENT_EDITOR_SIMULATION_MODE_CHANGED:
            case ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED:
            {
                if (wparam)
                {
                    // Game Mode begin or AI/Physics enabled
                    m_oAudioObjectMgr.ReleasePendingRays();
                    CATLAudioObject::CPropagationProcessor::s_canIssueRWIs = true;
                }
                else
                {
                    // Editor Mode begin or AI/Physics disabled
                    CATLAudioObject::CPropagationProcessor::s_canIssueRWIs = false;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    EAudioRequestStatus CAudioTranslationLayer::RefreshAudioSystem(const char* const controlsPath, const char* const levelName, TAudioPreloadRequestID levelPreloadId)
    {
        g_audioLogger.Log(eALT_ALWAYS, "$8Beginning to refresh the AudioSystem!");

        if (!controlsPath || controlsPath[0] == '\0')
        {
            g_audioLogger.Log(eALT_ERROR, "ATL RefreshAudioSystem - Controls path is null, can't complete the refresh!");
            return eARS_FAILURE;
        }

        EAudioRequestStatus eResult = eARS_FAILURE;
        AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::StopAllSounds);
        AZ_Error("AudioTranslationLayer", eResult == eARS_SUCCESS, "ATL RefreshAudioSystem - Failed to StopAllSounds!");

        eResult = m_oFileCacheMgr.UnloadDataByScope(eADS_LEVEL_SPECIFIC);
        AZ_Error("AudioTranslationLayer", eResult == eARS_SUCCESS, "ATL RefreshAudioSystem - Failed to unload old level data!");

        eResult = m_oFileCacheMgr.UnloadDataByScope(eADS_GLOBAL);
        AZ_Error("AudioTranslationLayer", eResult == eARS_SUCCESS, "ATL RefreshAudioSystem - Failed to unload old global data!");

        eResult = ClearControlsData(eADS_ALL);
        AZ_Error("AudioTranslationLayer", eResult == eARS_SUCCESS, "ATL RefreshAudioSystem - Failed to clear old controls data!");

        eResult = ClearPreloadsData(eADS_ALL);
        AZ_Error("AudioTranslationLayer", eResult == eARS_SUCCESS, "ATL RefreshAudioSystem - Failed to clear old preloads data!");

        AudioSystemImplementationNotificationBus::Broadcast(&AudioSystemImplementationNotificationBus::Events::OnAudioSystemRefresh);

        SetImplLanguage();

        eResult = ParseControlsData(controlsPath, eADS_GLOBAL);
        AZ_Error("AudioTranslationLayer", eResult == eARS_SUCCESS, "ATL RefreshAudioSystem - Failed to parse fresh global controls data!");

        eResult = ParsePreloadsData(controlsPath, eADS_GLOBAL);
        AZ_Error("AudioTranslationLayer", eResult == eARS_SUCCESS, "ATL RefreshAudioSystem - Failed to parse fresh global preloads data!");

        eResult = m_oFileCacheMgr.TryLoadRequest(SATLInternalControlIDs::nGlobalPreloadRequestID, true, true);
        AZ_Error("AudioTranslationLayer", eResult == eARS_SUCCESS, "ATL RefreshAudioSystem - Failed to load fresh global preloads!");

        if (levelName && levelName[0] != '\0')
        {
            AZStd::string levelControlsPath(controlsPath);
            levelControlsPath.append("levels/");
            levelControlsPath.append(levelName);
            AzFramework::StringFunc::RelativePath::Normalize(levelControlsPath);

            eResult = ParseControlsData(levelControlsPath.c_str(), eADS_LEVEL_SPECIFIC);
            AZ_Error("AudioTranslationLayer", eResult == eARS_SUCCESS, "ATL RefreshAudioSystem - Failed to parse fresh level controls data!");

            eResult = ParsePreloadsData(levelControlsPath.c_str(), eADS_LEVEL_SPECIFIC);
            AZ_Error("AudioTranslationLayer", eResult == eARS_SUCCESS, "ATL RefreshAudioSystem - Failed to parse fresh level preloads data!");

            if (levelPreloadId != INVALID_AUDIO_PRELOAD_REQUEST_ID)
            {
                eResult = m_oFileCacheMgr.TryLoadRequest(levelPreloadId, true, true);
                AZ_Error("AudioTranslationLayer", eResult == eARS_SUCCESS, "ATL RefreshAudioSystem - Failed to load fresh level preloads!");
            }
        }

        g_audioLogger.Log(eALT_ALWAYS, "$3Done refreshing the AudioSystem!");

        return eARS_SUCCESS;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioTranslationLayer::ReserveAudioObjectID(TAudioObjectID& rAudioObjectID, const char* const sAudioObjectName)
    {
        const bool bSuccess = m_oAudioObjectMgr.ReserveID(rAudioObjectID, sAudioObjectName);

        if (bSuccess)
        {
            m_oDebugNameStore.AddAudioObject(rAudioObjectID, sAudioObjectName);
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DrawAudioSystemDebugInfo()
    {
        FUNCTION_PROFILER_ALWAYS(gEnv->pSystem, PROFILE_AUDIO);

        if (IRenderAuxGeom* pAuxGeom = (g_audioCVars.m_nDrawAudioDebug > 0 && gEnv->pRenderer) ? gEnv->pRenderer->GetIRenderAuxGeom() : nullptr)
        {
            DrawAudioObjectDebugInfo(*pAuxGeom); // needs to be called first so that the rest of the labels are printed
            // on top (Draw2dLabel doesn't provide a way set which labels are printed on top)

            const size_t nPrimaryPoolSize = AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().Capacity();
            const size_t nPrimaryPoolUsedSize = AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().NumAllocatedBytes();

            float fPosX = 0.0f;
            float fPosY = 4.0f;

            const float fColor[4] = { 1.0f, 1.0f, 1.0f, 0.9f };
            const float fColorRed[4] = { 1.0f, 0.0f, 0.0f, 0.7f };
            const float fColorGreen[4] = { 0.0f, 1.0f, 0.0f, 0.7f };
            const float fColorBlue[4] = { 0.4f, 0.4f, 1.0f, 1.0f };

            const char* implementationName = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(implementationName, &AudioSystemImplementationRequestBus::Events::GetImplementationNameString);
            pAuxGeom->Draw2dLabel(fPosX, fPosY, 1.6f, fColorBlue, false, "AudioTranslationLayer with %s", implementationName);

            fPosX += 20.0f;
            fPosY += 17.0f;

            pAuxGeom->Draw2dLabel(fPosX, fPosY, 1.35f, fColor, false,
                "AudioSystem Memory: %.2f / %.2f MiB",
                (nPrimaryPoolUsedSize / 1024) / 1024.f,
                (nPrimaryPoolSize / 1024) / 1024.f
                );


            float const fLineHeight = 13.0f;

            SAudioImplMemoryInfo oMemoryInfo;
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::GetMemoryInfo, oMemoryInfo);

            fPosY += fLineHeight;
            pAuxGeom->Draw2dLabel(fPosX, fPosY, 1.35f, fColor, false,
                "AudioImpl Memory: %.2f / %.2f MiB",
                (oMemoryInfo.nPrimaryPoolUsedSize / 1024) / 1024.f,
                (oMemoryInfo.nPrimaryPoolSize / 1024) / 1024.f
                );

            static const float SMOOTHING_ALPHA = 0.2f;
            static float fSyncRays = 0;
            static float fAsyncRays = 0;

            const Vec3 vPos = m_oSharedData.m_oActiveListenerPosition.GetPositionVec();
            const Vec3 vFwd = m_oSharedData.m_oActiveListenerPosition.GetForwardVec();
            const size_t nNumAudioObjects = m_oAudioObjectMgr.GetNumAudioObjects();
            const size_t nNumActiveAudioObjects = m_oAudioObjectMgr.GetNumActiveAudioObjects();
            const size_t nEvents = m_oAudioEventMgr.GetNumActive();
            const size_t nListeners = m_oAudioListenerMgr.GetNumActive();
            const size_t nNumEventListeners = m_oAudioEventListenerMgr.GetNumEventListeners();
            fSyncRays += (CATLAudioObject::CPropagationProcessor::s_nTotalSyncPhysRays - fSyncRays) * SMOOTHING_ALPHA;
            fAsyncRays += (CATLAudioObject::CPropagationProcessor::s_nTotalAsyncPhysRays - fAsyncRays) * SMOOTHING_ALPHA * 0.1f;

            const bool bActive = true;
            const float fColorListener[4] =
            {
                bActive ? fColorGreen[0] : fColorRed[0],
                bActive ? fColorGreen[1] : fColorRed[1],
                bActive ? fColorGreen[2] : fColorRed[2],
                1.0f
            };

            const float* fColorNumbers = fColorBlue;

            TAudioObjectID activeListenerID = INVALID_AUDIO_OBJECT_ID;
            if (CATLListenerObject* overrideListener = m_oAudioListenerMgr.LookupID(m_oAudioListenerMgr.GetOverrideListenerID()))
            {
                activeListenerID = overrideListener->GetID();
            }
            else if (CATLListenerObject* defaultListener = m_oAudioListenerMgr.LookupID(m_oAudioListenerMgr.GetDefaultListenerID()))
            {
                activeListenerID = defaultListener->GetID();
            }

            fPosY += fLineHeight;
            pAuxGeom->Draw2dLabel(fPosX, fPosY, 1.35f, fColorListener, false,
                "Listener <%llu> PosXYZ: %.2f %.2f %.2f FwdXYZ: %.2f %.2f %.2f",
                activeListenerID,
                vPos.x, vPos.y, vPos.z,
                vFwd.x, vFwd.y, vFwd.z);

            fPosY += fLineHeight;
            pAuxGeom->Draw2dLabel(fPosX, fPosY, 1.35f, fColorNumbers, false,
                "Objects: %3" PRISIZE_T "/%3" PRISIZE_T " | Events: %3" PRISIZE_T "  EventListeners %3" PRISIZE_T " | Listeners: %" PRISIZE_T " | SyncRays: %3.1f  AsyncRays: %3.1f",
                nNumActiveAudioObjects, nNumAudioObjects, nEvents, nNumEventListeners, nListeners, fSyncRays, fAsyncRays);

            fPosY += fLineHeight;
            DrawATLComponentDebugInfo(*pAuxGeom, fPosX, fPosY);

            pAuxGeom->Commit(7);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DrawATLComponentDebugInfo(IRenderAuxGeom& auxGeom, float fPosX, const float fPosY)
    {
        m_oFileCacheMgr.DrawDebugInfo(auxGeom, fPosX, fPosY);

        if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_IMPL_MEMORY_POOL_USAGE) != 0)
        {
            DrawImplMemoryPoolDebugInfo(auxGeom, fPosX, fPosY);
        }

        if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_ACTIVE_OBJECTS) != 0)
        {
            m_oAudioObjectMgr.DrawDebugInfo(auxGeom, fPosX, fPosY);
            fPosX += 800.0f;
        }

        if ((g_audioCVars.m_nDrawAudioDebug & eADDF_SHOW_ACTIVE_EVENTS) != 0)
        {
            m_oAudioEventMgr.DrawDebugInfo(auxGeom, fPosX, fPosY);
        }

        if ((g_audioCVars.m_nDrawAudioDebug & eADDF_DRAW_LISTENER_SPHERE) != 0)
        {
            m_oAudioListenerMgr.DrawDebugInfo(auxGeom);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void BytesToString(AZ::u32 bytes, char* buffer, AZStd::size_t bufLength)
    {
        if (bytes < (1 << 10))
        {
            azsnprintf(buffer, bufLength, "%u B", bytes);
        }
        else if (bytes < (1 << 20))
        {
            azsnprintf(buffer, bufLength, "%.1f KB", static_cast<float>(bytes) / static_cast<float>(1 << 10));
        }
        else
        {
            azsnprintf(buffer, bufLength, "%.1f MB", static_cast<float>(bytes) / static_cast<float>(1 << 20));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DrawImplMemoryPoolDebugInfo(IRenderAuxGeom& auxGeom, float fPosX, float fPosY)
    {
        const float colorMax = 0.9f;
        const float colorMin = 0.1f;
        const float textSize = 1.5f;
        float color[4] = { colorMax, colorMax, colorMax, 0.9f };

        const AZ::Color greenColor(colorMin, colorMax, colorMin, 0.9f);
        const AZ::Color yellowColor(colorMax, colorMax, colorMin, 0.9f);
        const AZ::Color redColor(colorMax, colorMin, colorMin, 0.9f);

        const char* tableHeaderNames[8] = {
            "ID",
            "Name",
            "Total Size",
            "Used Size",
            "Used %%",
            "Peak Used",
            "Allocs",
            "Frees",
        };
        const float xTablePositions[8] = { 0.f, 40.f, 300.f, 400.f, 500.f, 600.f, 700.f, 800.f };

        float posY = fPosY;

        // Draw the header info:
        for (int column = 0; column < 8; ++column)
        {
            auxGeom.Draw2dLabel(fPosX + xTablePositions[column], posY, 1.5f, color, false, tableHeaderNames[column]);
        }

        // Get the memory pool information...
        AZStd::vector<AudioImplMemoryPoolInfo> poolInfos;
        AudioSystemImplementationRequestBus::BroadcastResult(poolInfos, &AudioSystemImplementationRequestBus::Events::GetMemoryPoolInfo);

        if (!poolInfos.empty())
        {
            const AZStd::size_t bufferSize = 32;
            char buffer[bufferSize] = { 0 };

            for (auto& poolInfo : poolInfos)
            {
                posY += 15.f;

                auto percentUsed = static_cast<float>(poolInfo.m_memoryUsed) / static_cast<float>(poolInfo.m_memoryReserved);

                // Calculate a color (green->-yellow->-red) based on percentage.
                AZ::Color percentColor;
                if (percentUsed < 0.5f)
                {
                    percentColor = greenColor.Lerp(yellowColor, percentUsed * 2.f);
                }
                else
                {
                    percentColor = yellowColor.Lerp(redColor, (percentUsed * 2.f) - 1.f);
                }
                percentColor.StoreToFloat4(color);

                // ID
                auxGeom.Draw2dLabel(
                    fPosX + xTablePositions[0], posY, textSize, color, false, "%d", poolInfo.m_poolId);

                // Name
                auxGeom.Draw2dLabel(
                    fPosX + xTablePositions[1], posY, textSize, color, false, "%s", poolInfo.m_poolName);

                // Total Size (bytes)
                BytesToString(poolInfo.m_memoryReserved, buffer, bufferSize);
                auxGeom.Draw2dLabel(
                    fPosX + xTablePositions[2], posY, textSize, color, false, "%s", buffer);

                // Used Size (bytes)
                BytesToString(poolInfo.m_memoryUsed, buffer, bufferSize);
                auxGeom.Draw2dLabel(
                    fPosX + xTablePositions[3], posY, textSize, color, false, "%s", buffer);

                // Used %
                auxGeom.Draw2dLabel(
                    fPosX + xTablePositions[4], posY, textSize, color, false, "%.1f %%", percentUsed * 100.f);

                // Peak Used (bytes)
                BytesToString(poolInfo.m_peakUsed, buffer, bufferSize);
                auxGeom.Draw2dLabel(
                    fPosX + xTablePositions[5], posY, textSize, color, false, "%s", buffer);

                // Allocs
                auxGeom.Draw2dLabel(
                    fPosX + xTablePositions[6], posY, textSize, color, false, "%u", poolInfo.m_numAllocs);

                // Frees
                auxGeom.Draw2dLabel(
                    fPosX + xTablePositions[7], posY, textSize, color, false, "%u", poolInfo.m_numFrees);
            }
        }
        else
        {
            auxGeom.Draw2dLabel(
                fPosX, posY + 15.f, 1.5f, color, false, "No memory pool information is available for display!");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioTranslationLayer::DrawAudioObjectDebugInfo(IRenderAuxGeom& auxGeom)
    {
        SATLWorldPosition oListenerPosition;
        m_oAudioListenerMgr.GetDefaultListenerPosition(oListenerPosition);
        m_oAudioObjectMgr.DrawPerObjectDebugInfo(auxGeom, oListenerPosition.GetPositionVec());
    }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
} // namespace Audio
