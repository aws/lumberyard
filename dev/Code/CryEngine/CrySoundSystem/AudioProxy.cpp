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
#include "AudioProxy.h"

#include <ATLCommon.h>
#include <SoundCVars.h>
#include <AudioLogger.h>
#include <ISystem.h>
#include <IEntitySystem.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct AreaInfoCompare
    {
        bool operator() (const SAudioAreaInfo& oAreaInfo1, const SAudioAreaInfo& oAreaInfo2)
        {
            bool bResult = false;
            const int nGroup1 = oAreaInfo1.pArea->GetGroup();
            const int nGroup2 = oAreaInfo2.pArea->GetGroup();

            if (nGroup1 == nGroup2)
            {
                bResult = (oAreaInfo1.pArea->GetPriority() > oAreaInfo2.pArea->GetPriority());
            }
            else
            {
                bResult = (nGroup1 > nGroup2);
            }

            return bResult;
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioProxy::CAudioProxy()
        : m_nAudioObjectID(INVALID_AUDIO_OBJECT_ID)
        , m_nFlags(eAPF_NONE)
        , m_eCurrentLipSyncMethod(eLSM_None)
        , m_nCurrentLipSyncID(INVALID_AUDIO_CONTROL_ID)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioProxy::~CAudioProxy()
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) != 0)
        {
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::RemoveRequestListener, &CAudioProxy::OnAudioEvent, this);
        }

        AZ_Assert(m_nAudioObjectID == INVALID_AUDIO_OBJECT_ID, "Expected AudioObjectID [%d] to be invalid when the audio proxy is destructed.", m_nAudioObjectID);
        stl::free_container(m_aQueuedAudioCommands);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::Initialize(const char* const sObjectName, const bool bInitAsync /* = true */)
    {
        if ((bInitAsync && g_audioCVars.m_nAudioProxiesInitType == 0) || g_audioCVars.m_nAudioProxiesInitType == 2)
        {
            if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
            {
                // Add the request listener to receive callback when the audio object ID has been registered with middleware...
                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::AddRequestListener, &CAudioProxy::OnAudioEvent, this, eART_AUDIO_MANAGER_REQUEST, eAMRT_RESERVE_AUDIO_OBJECT_ID);

                m_nFlags |= eAPF_WAITING_FOR_ID;

                SAudioRequest oRequest;
                SAudioManagerRequestData<eAMRT_RESERVE_AUDIO_OBJECT_ID> oRequestData(&m_nAudioObjectID, sObjectName);
                oRequest.nFlags = (eARF_PRIORITY_HIGH | eARF_SYNC_CALLBACK);
                oRequest.pOwner = this;
                oRequest.pData = &oRequestData;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
            }
            else
            {
                SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_INITIALIZE);
                oQueuedCommand.sValue = sObjectName;
                TryAddQueuedCommand(oQueuedCommand);
            }
        }
        else
        {
            SAudioRequest oRequest;
            SAudioManagerRequestData<eAMRT_RESERVE_AUDIO_OBJECT_ID> oRequestData(&m_nAudioObjectID, sObjectName);
            oRequest.nFlags = (eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING);
            oRequest.pData = &oRequestData;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequestBlocking, oRequest);

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
            if (m_nAudioObjectID == INVALID_AUDIO_OBJECT_ID)
            {
                g_audioLogger.Log(eALT_FATAL, "Failed to reserve audio object ID on AudioProxy (%s)!", sObjectName);
            }
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ExecuteSourceTrigger(
        TAudioControlID nTriggerID,
        const SAudioSourceInfo& rSourceInfo,
        const SAudioCallBackInfos& rCallbackInfos /* = SAudioCallBackInfos::GetEmptyObject() */)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            AZ_Assert(m_nAudioObjectID != INVALID_AUDIO_OBJECT_ID, "Invalid AudioObjectID found when an audio proxy is executing a source trigger!");

            SAudioRequest oRequest;
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = rCallbackInfos.nRequestFlags;

            SAudioObjectRequestData<eAORT_EXECUTE_SOURCE_TRIGGER> oRequestData(nTriggerID, rSourceInfo);
            oRequest.pOwner = (rCallbackInfos.pObjectToNotify != nullptr) ? rCallbackInfos.pObjectToNotify : this;
            oRequest.pUserData = rCallbackInfos.pUserData;
            oRequest.pUserDataOwner = rCallbackInfos.pUserDataOwner;
            oRequest.pData = &oRequestData;
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_EXECUTE_SOURCE_TRIGGER);
            oQueuedCommand.nTriggerID = nTriggerID;
            oQueuedCommand.pOwnerOverride = rCallbackInfos.pObjectToNotify;
            oQueuedCommand.pUserData = rCallbackInfos.pUserData;
            oQueuedCommand.pUserDataOwner = rCallbackInfos.pUserDataOwner;
            oQueuedCommand.nRequestFlags = rCallbackInfos.nRequestFlags;
            oQueuedCommand.rSourceInfo = rSourceInfo;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ExecuteTrigger(
        const TAudioControlID nTriggerID,
        const ELipSyncMethod eLipSyncMethod,
        const SAudioCallBackInfos& rCallbackInfos /* = SAudioCallBackInfos::GetEmptyObject() */)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            AZ_Assert(m_nAudioObjectID != INVALID_AUDIO_OBJECT_ID, "Invalid AudioObjectID found when an audio proxy is executing a trigger!");

            SAudioRequest oRequest;
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = rCallbackInfos.nRequestFlags;

            SAudioObjectRequestData<eAORT_EXECUTE_TRIGGER> oRequestData(nTriggerID, 0.0f);
            oRequestData.eLipSyncMethod = eLipSyncMethod;
            oRequest.pOwner = (rCallbackInfos.pObjectToNotify != nullptr) ? rCallbackInfos.pObjectToNotify : this;
            oRequest.pUserData = rCallbackInfos.pUserData;
            oRequest.pUserDataOwner = rCallbackInfos.pUserDataOwner;
            oRequest.pData = &oRequestData;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_EXECUTE_TRIGGER);
            oQueuedCommand.nTriggerID = nTriggerID;
            oQueuedCommand.eLipSyncMethod = eLipSyncMethod;
            oQueuedCommand.pOwnerOverride = rCallbackInfos.pObjectToNotify;
            oQueuedCommand.pUserData = rCallbackInfos.pUserData;
            oQueuedCommand.pUserDataOwner = rCallbackInfos.pUserDataOwner;
            oQueuedCommand.nRequestFlags = rCallbackInfos.nRequestFlags;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::StopAllTriggers()
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_STOP_ALL_TRIGGERS> oRequestData;
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = eARF_PRIORITY_NORMAL;
            oRequest.pData = &oRequestData;
            oRequest.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_STOP_ALL_TRIGGERS);
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::StopTrigger(const TAudioControlID nTriggerID)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_STOP_TRIGGER> oRequestData(nTriggerID);
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = eARF_PRIORITY_NORMAL;
            oRequest.pData = &oRequestData;
            oRequest.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_STOP_TRIGGER);
            oQueuedCommand.nTriggerID = nTriggerID;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_SET_SWITCH_STATE> oRequestData(nSwitchID, nStateID);
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = eARF_PRIORITY_NORMAL;
            oRequest.pData = &oRequestData;
            oRequest.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_SET_SWITCH_STATE);
            oQueuedCommand.nSwitchID = nSwitchID;
            oQueuedCommand.nStateID = nStateID;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetRtpcValue(const TAudioControlID nRtpcID, const float fValue)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_SET_RTPC_VALUE> oRequestData(nRtpcID, fValue);
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = eARF_PRIORITY_NORMAL;
            oRequest.pData = &oRequestData;
            oRequest.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_SET_RTPC_VALUE);
            oQueuedCommand.nRtpcID = nRtpcID;
            oQueuedCommand.fValue = fValue;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetObstructionCalcType(const EAudioObjectObstructionCalcType eObstructionType)
    {
        const TATLEnumFlagsType nObstructionCalcIndex = static_cast<TATLEnumFlagsType>(eObstructionType);

        if (nObstructionCalcIndex < eAOOCT_COUNT)
        {
            SetSwitchState(SATLInternalControlIDs::nObstructionOcclusionCalcSwitchID, SATLInternalControlIDs::nOOCStateIDs[eObstructionType]);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetPosition(const SATLWorldPosition& refPosition)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            // Update position only if the delta exceeds a given value.
            // Ideally this should be done on the caller's side!
            if (g_audioCVars.m_fPositionUpdateThreshold <= 0.f
                || !refPosition.mPosition.GetTranslation().IsEquivalent(m_oPosition.mPosition.GetTranslation(), g_audioCVars.m_fPositionUpdateThreshold))
            {
                m_oPosition = refPosition;

                // Make sure the forward direction is normalized
                m_oPosition.NormalizeForwardVec();

                SAudioRequest oRequest;
                SAudioObjectRequestData<eAORT_SET_POSITION> oRequestData(m_oPosition);
                oRequest.nAudioObjectID = m_nAudioObjectID;
                oRequest.nFlags = eARF_PRIORITY_NORMAL;
                oRequest.pData = &oRequestData;
                oRequest.pOwner = this;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
            }
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_SET_POSITION);
            oQueuedCommand.oPosition = refPosition;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetPosition(const Vec3& refPosition)
    {
        SetPosition(SATLWorldPosition(refPosition));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetMultiplePositions(const MultiPositionParams& params)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest request;
            SAudioObjectRequestData<eAORT_SET_MULTI_POSITIONS> requestData(params);
            request.nAudioObjectID = m_nAudioObjectID;
            request.nFlags = eARF_PRIORITY_NORMAL;
            request.pData = &requestData;
            request.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_SET_MULTI_POSITIONS);
            oQueuedCommand.oMultiPosParams= params;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetEnvironmentAmount(const TAudioEnvironmentID nEnvironmentID, const float fValue)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_SET_ENVIRONMENT_AMOUNT> oRequestData(nEnvironmentID, fValue);
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = eARF_PRIORITY_NORMAL;
            oRequest.pData = &oRequestData;
            oRequest.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_SET_ENVIRONMENT_AMOUNT);
            oQueuedCommand.nEnvironmentID = nEnvironmentID;
            oQueuedCommand.fValue = fValue;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetCurrentEnvironments(const EntityId nEntityToIgnore)
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            IAreaManager* const pAreaManager = gEnv->pEntitySystem->GetAreaManager();

            SAudioAreaInfo m_aAreaQueries[sMaxAreas];
            size_t nAreaCount = 0;

            ClearEnvironments();

            if (!pAreaManager->QueryAudioAreas(m_oPosition.GetPositionVec(), m_aAreaQueries, sMaxAreas, nAreaCount))
            {
                g_audioLogger.Log(eALT_WARNING, "QueryAreas failed for AudioObjectID %u", m_nAudioObjectID);
            }

            if (nAreaCount > 0)
            {
                std::sort(m_aAreaQueries, m_aAreaQueries + nAreaCount, AreaInfoCompare());

                int nLastGroupID = -1;// default value for the CArea::m_AreaGroupID
                int nLastPriority = 0;// default value for the CArea::m_nPriority
                bool bIgnoreThisGroup = false;
                float fCurrentAmount = 0.0f;

                for (size_t nArea = 0; nArea < nAreaCount; ++nArea)
                {
                    const SAudioAreaInfo& rAreaInfo = m_aAreaQueries[nArea];
                    const int nCurrentGroupID = rAreaInfo.pArea->GetGroup();
                    const int nCurrentPriority = rAreaInfo.pArea->GetPriority();

                    if (nEntityToIgnore == INVALID_ENTITYID || nEntityToIgnore != rAreaInfo.nEnvProvidingEntityID)
                    {
                        if (nCurrentGroupID != nLastGroupID)
                        {
                            // new group, highest priority
                            fCurrentAmount = rAreaInfo.fEnvironmentAmount;
                            nLastGroupID = nCurrentGroupID;
                            nLastPriority = nCurrentPriority;

                            SetEnvironmentAmount(rAreaInfo.nEnvironmentID, rAreaInfo.fEnvironmentAmount);
                        }
                        else if (nCurrentPriority != nLastPriority)
                        {
                            // same group, lower priority
                            if (fCurrentAmount < 1.0f)
                            {
                                fCurrentAmount = rAreaInfo.fEnvironmentAmount;
                                nLastPriority = nCurrentPriority;

                                SetEnvironmentAmount(rAreaInfo.nEnvironmentID, fCurrentAmount);
                            }
                        }
                        else
                        {
                            // same group, same priority
                            // this assumes that all areas of the same priority within a group use the same Environment
                            const float fAmount = rAreaInfo.fEnvironmentAmount;

                            if (fAmount > fCurrentAmount)
                            {
                                SetEnvironmentAmount(rAreaInfo.nEnvironmentID, fAmount);
                                fCurrentAmount = fAmount;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            SQueuedAudioCommand oQueuedCommand = SQueuedAudioCommand(eQACT_SET_CURRENT_ENVIRONMENTS);
            oQueuedCommand.nEntityID = nEntityToIgnore;
            TryAddQueuedCommand(oQueuedCommand);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::SetLipSyncProvider(ILipSyncProvider* const pILipSyncProvider)
    {
        if (pILipSyncProvider == m_oLipSyncProvider.get())
        {
            return;
        }

        m_oLipSyncProvider.reset(pILipSyncProvider);
        m_nCurrentLipSyncID = INVALID_AUDIO_CONTROL_ID;
        m_eCurrentLipSyncMethod = eLSM_None;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ClearEnvironments()
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_RESET_ENVIRONMENTS> oRequestData;
            oRequest.nAudioObjectID = m_nAudioObjectID;
            oRequest.nFlags = eARF_PRIORITY_NORMAL;
            oRequest.pData = &oRequestData;
            oRequest.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);
        }
        else
        {
            TryAddQueuedCommand(SQueuedAudioCommand(eQACT_CLEAR_ENVIRONMENTS));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ResetRtpcValues()
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            SAudioRequest request;
            SAudioObjectRequestData<eAORT_RESET_RTPCS> requestData;
            request.nAudioObjectID = m_nAudioObjectID;
            request.nFlags = eARF_PRIORITY_NORMAL;
            request.pData = &requestData;
            request.pOwner = this;

            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, request);
        }
        else
        {
            TryAddQueuedCommand(SQueuedAudioCommand(eQACT_CLEAR_RTPCS));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::Release()
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            Reset();
            AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::FreeAudioProxy, this);
        }
        else
        {
            TryAddQueuedCommand(SQueuedAudioCommand(eQACT_RELEASE));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::Reset()
    {
        if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
        {
            if (m_nAudioObjectID != INVALID_AUDIO_OBJECT_ID)
            {
                // Request must be asynchronous and lowest priority!
                SAudioRequest oRequest;
                SAudioObjectRequestData<eAORT_RELEASE_OBJECT> oRequestData;
                oRequest.nAudioObjectID = m_nAudioObjectID;
                oRequest.pData = &oRequestData;

                AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::PushRequest, oRequest);

                m_nAudioObjectID = INVALID_AUDIO_OBJECT_ID;
            }

            m_oPosition = SATLWorldPosition();
            m_eCurrentLipSyncMethod = eLSM_None;
            m_nCurrentLipSyncID = INVALID_AUDIO_CONTROL_ID;
        }
        else
        {
            TryAddQueuedCommand(SQueuedAudioCommand(eQACT_RESET));
        }
    }

    // Audio: need a way to periodically update the LipSyncProvider
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //void CAudioProxy::Update(SEntityUpdateContext &ctx)
    //{
    //  if (m_currentLipSyncId != INVALID_AUDIO_CONTROL_ID)
    //  {
    //      if (m_pLipSyncProvider)
    //      {
    //          m_pLipSyncProvider->UpdateLipSync(this, m_currentLipSyncId, m_currentLipSyncMethod);
    //      }
    //  }
    //}

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::ExecuteQueuedCommands()
    {
        // Remove the request listener once the audio system has properly reserved the audio object ID for this proxy.
        AudioSystemRequestBus::Broadcast(&AudioSystemRequestBus::Events::RemoveRequestListener, &CAudioProxy::OnAudioEvent, this);

        m_nFlags &= ~eAPF_WAITING_FOR_ID;

        if (!m_aQueuedAudioCommands.empty())
        {
            TQueuedAudioCommands::iterator Iter(m_aQueuedAudioCommands.begin());
            TQueuedAudioCommands::const_iterator IterEnd(m_aQueuedAudioCommands.end());
            for (; Iter != IterEnd; ++Iter)
            {
                const SQueuedAudioCommand& refCommand = (*Iter);
                // TODO: pass the refCommand.pUserData to all methods (maybe pass refCommand as a parameter to all functions)

                switch (refCommand.eType)
                {
                    case eQACT_EXECUTE_TRIGGER:
                    {
                        const SAudioCallBackInfos callbackInfos(refCommand.pOwnerOverride, refCommand.pUserData, refCommand.pUserDataOwner, refCommand.nRequestFlags);
                        ExecuteTrigger(refCommand.nTriggerID, refCommand.eLipSyncMethod, callbackInfos);
                        break;
                    }
                    case eQACT_EXECUTE_SOURCE_TRIGGER:
                    {
                        const SAudioCallBackInfos callbackInfos(refCommand.pOwnerOverride, refCommand.pUserData, refCommand.pUserDataOwner, refCommand.nRequestFlags);
                        ExecuteSourceTrigger(refCommand.nTriggerID, refCommand.rSourceInfo, callbackInfos);
                        break;
                    }
                    case eQACT_STOP_TRIGGER:
                    {
                        StopTrigger(refCommand.nTriggerID);
                        break;
                    }
                    case eQACT_SET_SWITCH_STATE:
                    {
                        SetSwitchState(refCommand.nSwitchID, refCommand.nStateID);
                        break;
                    }
                    case eQACT_SET_RTPC_VALUE:
                    {
                        SetRtpcValue(refCommand.nRtpcID, refCommand.fValue);
                        break;
                    }
                    case eQACT_SET_POSITION:
                    {
                        SetPosition(refCommand.oPosition);
                        break;
                    }
                    case eQACT_SET_ENVIRONMENT_AMOUNT:
                    {
                        SetEnvironmentAmount(refCommand.nEnvironmentID, refCommand.fValue);
                        break;
                    }
                    case eQACT_SET_CURRENT_ENVIRONMENTS:
                    {
                        SetCurrentEnvironments(refCommand.nEntityID);
                        break;
                    }
                    case eQACT_CLEAR_ENVIRONMENTS:
                    {
                        ClearEnvironments();
                        break;
                    }
                    case eQACT_CLEAR_RTPCS:
                    {
                        ResetRtpcValues();
                        break;
                    }
                    case eQACT_RESET:
                    {
                        Reset();
                        break;
                    }
                    case eQACT_RELEASE:
                    {
                        Release();
                        break;
                    }
                    case eQACT_INITIALIZE:
                    {
                        Initialize(refCommand.sValue.c_str(), true);
                        break;
                    }
                    case eQACT_STOP_ALL_TRIGGERS:
                    {
                        StopAllTriggers();
                        break;
                    }
                    case eQACT_SET_MULTI_POSITIONS:
                    {
                        SetMultiplePositions(refCommand.oMultiPosParams);
                        break;
                    }
                    default:
                    {
                        g_audioLogger.Log(eALT_FATAL, "Unknown command type in CAudioProxy::ExecuteQueuedCommands!");
                        break;
                    }
                }

                if ((m_nFlags & eAPF_WAITING_FOR_ID) != 0)
                {
                    // An Initialize command was queued up.
                    // Here we need to keep all commands after the Initialize.
                    break;
                }
            }

            if ((m_nFlags & eAPF_WAITING_FOR_ID) == 0)
            {
                m_aQueuedAudioCommands.clear();
            }
            else
            {
                // An Initialize command was queued up.
                // Here we need to keep queued commands except for Reset and Initialize.
                Iter = m_aQueuedAudioCommands.begin();

                while (Iter != IterEnd)
                {
                    const SQueuedAudioCommand& refCommand = (*Iter);

                    if (refCommand.eType == eQACT_RESET || refCommand.eType == eQACT_INITIALIZE)
                    {
                        Iter = m_aQueuedAudioCommands.erase(Iter);
                        IterEnd = m_aQueuedAudioCommands.end();
                        continue;
                    }

                    ++Iter;
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::TryAddQueuedCommand(const SQueuedAudioCommand& refCommand)
    {
        bool bAdd = true;

        switch (refCommand.eType)
        {
            case eQACT_EXECUTE_TRIGGER:
            case eQACT_EXECUTE_SOURCE_TRIGGER:
            case eQACT_STOP_TRIGGER:
            {
                // These type of commands get always pushed back!
                break;
            }
            case eQACT_SET_SWITCH_STATE:
            {
                if (!m_aQueuedAudioCommands.empty())
                {
                    bAdd = (AZStd::find_if(m_aQueuedAudioCommands.begin(), m_aQueuedAudioCommands.end(), SFindSetSwitchState(refCommand.nSwitchID, refCommand.nStateID)) == m_aQueuedAudioCommands.end());
                }
                break;
            }
            case eQACT_SET_RTPC_VALUE:
            {
                if (!m_aQueuedAudioCommands.empty())
                {
                    bAdd = (AZStd::find_if(m_aQueuedAudioCommands.begin(), m_aQueuedAudioCommands.end(), SFindSetRtpcValue(refCommand.nRtpcID, refCommand.fValue)) == m_aQueuedAudioCommands.end());
                }
                break;
            }
            case eQACT_SET_POSITION:
            {
                if (!m_aQueuedAudioCommands.empty())
                {
                    bAdd = (AZStd::find_if(m_aQueuedAudioCommands.begin(), m_aQueuedAudioCommands.end(), SFindSetPosition(refCommand.oPosition)) == m_aQueuedAudioCommands.end());
                }
                break;
            }
            case eQACT_SET_ENVIRONMENT_AMOUNT:
            {
                if (!m_aQueuedAudioCommands.empty())
                {
                    bAdd = (AZStd::find_if(m_aQueuedAudioCommands.begin(), m_aQueuedAudioCommands.end(), SFindSetEnvironmentAmount(refCommand.nEnvironmentID, refCommand.fValue)) == m_aQueuedAudioCommands.end());
                }
                break;
            }
            case eQACT_SET_CURRENT_ENVIRONMENTS:
            case eQACT_CLEAR_ENVIRONMENTS:
            case eQACT_CLEAR_RTPCS:
            case eQACT_RELEASE:
            {
                // These type of commands don't need another instance!
                if (!m_aQueuedAudioCommands.empty())
                {
                    bAdd = (AZStd::find_if(m_aQueuedAudioCommands.begin(), m_aQueuedAudioCommands.end(), SFindCommand(refCommand.eType)) == m_aQueuedAudioCommands.end());
                }
                break;
            }
            case eQACT_RESET:
            {
                for (const SQueuedAudioCommand& rLocalCommand : m_aQueuedAudioCommands)
                {
                    if (rLocalCommand.eType == eQACT_RELEASE)
                    {
                        // If eQACT_RELEASE is already queued up then there is no need for adding a eQACT_RESET command.
                        bAdd = false;
                        break;
                    }
                }

                if (!bAdd)
                {
                    // If this proxy is resetting then there is no need for any pending commands.
                    m_aQueuedAudioCommands.clear();
                }
                break;
            }
            case eQACT_INITIALIZE:
            {
                // There must be only 1 Initialize command be queued up.
                m_aQueuedAudioCommands.clear();

                // Precede the Initialization with a Reset command to release the pending audio object.
                m_aQueuedAudioCommands.push_back(SQueuedAudioCommand(eQACT_RESET));
                break;
            }
            case eQACT_STOP_ALL_TRIGGERS:
            {
                if (!m_aQueuedAudioCommands.empty())
                {
                    // only add if the last request is different...
                    bAdd = (AZStd::find_if(m_aQueuedAudioCommands.end() - 1, m_aQueuedAudioCommands.end(), SFindCommand(refCommand.eType)) == m_aQueuedAudioCommands.end());
                }
                break;
            }
            case eQACT_SET_MULTI_POSITIONS:
            {
                if (!m_aQueuedAudioCommands.empty())
                {
                    // Find+Update or Add.
                    // Can morph a SetPosition command into a Multi-Position command.
                    bAdd = (AZStd::find_if(m_aQueuedAudioCommands.begin(), m_aQueuedAudioCommands.end(), SFindSetMultiplePositions(refCommand.oMultiPosParams)) == m_aQueuedAudioCommands.end());
                }
                break;
            }
            default:
            {
                g_audioLogger.Log(eALT_ERROR, "Unknown queued command type [%d] in CAudioProxy::TryAddQueuedCommand!", refCommand.eType);
                bAdd = false;
                break;
            }
        }

        if (bAdd)
        {
            if (refCommand.eType == eQACT_SET_POSITION || refCommand.eType == eQACT_SET_MULTI_POSITIONS)
            {
                // Make sure we set position first!
                m_aQueuedAudioCommands.push_front(refCommand);
            }
            else
            {
                m_aQueuedAudioCommands.push_back(refCommand);
            }
        }
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioProxy::OnAudioEvent(const SAudioRequestInfo* const pAudioRequestInfo)
    {
        if (pAudioRequestInfo->eResult == eARR_SUCCESS && pAudioRequestInfo->eAudioRequestType == eART_AUDIO_MANAGER_REQUEST)
        {
            const auto eAudioManagerRequestType = static_cast<const EAudioManagerRequestType>(pAudioRequestInfo->nSpecificAudioRequest);

            if (eAudioManagerRequestType == eAMRT_RESERVE_AUDIO_OBJECT_ID)
            {
                auto const pAudioProxy = static_cast<CAudioProxy*>(pAudioRequestInfo->pOwner);

                if (pAudioProxy)
                {
                    pAudioProxy->ExecuteQueuedCommands();
                }
            }
        }
    }

} // namespace Audio
