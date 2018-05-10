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
#include "ATLEntities.h"

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    #if defined(AZ_PLATFORM_WINDOWS)
    const char* const SATLXmlTags::sPlatform = "Windows";
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif defined(AZ_PLATFORM_APPLE_OSX)
    const char* const SATLXmlTags::sPlatform = "Mac";
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif defined(AZ_PLATFORM_LINUX_X64)
    const char* const SATLXmlTags::sPlatform = "Linux";
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif defined(AZ_PLATFORM_ANDROID)
    const char* const SATLXmlTags::sPlatform = "Android";
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif defined(AZ_PLATFORM_APPLE_IOS)
    const char* const SATLXmlTags::sPlatform = "iOS";
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
    #elif defined(AZ_PLATFORM_APPLE_TV)
    const char* const SATLXmlTags::sPlatform = "AppleTV";
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(ATLEntities_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
    #else
    #error "Unsupported platform."
    #endif

    const char* const SATLXmlTags::sRootNodeTag = "ATLConfig";
    const char* const SATLXmlTags::sTriggersNodeTag = "AudioTriggers";
    const char* const SATLXmlTags::sRtpcsNodeTag = "AudioRtpcs";
    const char* const SATLXmlTags::sSwitchesNodeTag = "AudioSwitches";
    const char* const SATLXmlTags::sPreloadsNodeTag = "AudioPreloads";
    const char* const SATLXmlTags::sEnvironmentsNodeTag = "AudioEnvironments";

    const char* const SATLXmlTags::sATLTriggerTag = "ATLTrigger";
    const char* const SATLXmlTags::sATLSwitchTag = "ATLSwitch";
    const char* const SATLXmlTags::sATLRtpcTag = "ATLRtpc";
    const char* const SATLXmlTags::sATLSwitchStateTag = "ATLSwitchState";
    const char* const SATLXmlTags::sATLEnvironmentTag = "ATLEnvironment";
    const char* const SATLXmlTags::sATLPlatformsTag = "ATLPlatforms";
    const char* const SATLXmlTags::sATLConfigGroupTag = "ATLConfigGroup";

    const char* const SATLXmlTags::sATLTriggerRequestTag = "ATLTriggerRequest";
    const char* const SATLXmlTags::sATLSwitchRequestTag = "ATLSwitchRequest";
    const char* const SATLXmlTags::sATLRtpcRequestTag = "ATLRtpcRequest";
    const char* const SATLXmlTags::sATLPreloadRequestTag = "ATLPreloadRequest";
    const char* const SATLXmlTags::sATLEnvironmentRequestTag = "ATLEnvironmentRequest";
    const char* const SATLXmlTags::sATLValueTag = "ATLValue";

    const char* const SATLXmlTags::sATLNameAttribute = "atl_name";
    const char* const SATLXmlTags::sATLInternalNameAttribute = "atl_internal_name";
    const char* const SATLXmlTags::sATLTypeAttribute = "atl_type";
    const char* const SATLXmlTags::sATLConfigGroupAttribute = "atl_config_group_name";

    const char* const SATLXmlTags::sATLDataLoadType = "AutoLoad";

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* const SATLInternalControlNames::sLoseFocusName = "lose_focus";
    const char* const SATLInternalControlNames::sGetFocusName = "get_focus";
    const char* const SATLInternalControlNames::sMuteAllName = "mute_all";
    const char* const SATLInternalControlNames::sUnmuteAllName = "unmute_all";
    const char* const SATLInternalControlNames::sDoNothingName = "do_nothing";
    const char* const SATLInternalControlNames::sObjectSpeedName = "object_speed";
    const char* const SATLInternalControlNames::sObstructionOcclusionCalcName = "ObstructionOcclusionCalculationType";
    const char* const SATLInternalControlNames::sOOCIgnoreStateName = "Ignore";
    const char* const SATLInternalControlNames::sOOCSingleRayStateName = "SingleRay";
    const char* const SATLInternalControlNames::sOOCMultiRayStateName = "MultiRay";
    const char* const SATLInternalControlNames::sObjectVelocityTrackingName = "object_velocity_tracking";
    const char* const SATLInternalControlNames::sOVTOnStateName = "on";
    const char* const SATLInternalControlNames::sOVTOffStateName = "off";
    const char* const SATLInternalControlNames::sGlobalPreloadRequestName = "global_atl_preloads";

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TAudioControlID SATLInternalControlIDs::nLoseFocusTriggerID = INVALID_AUDIO_CONTROL_ID;
    TAudioControlID SATLInternalControlIDs::nGetFocusTriggerID = INVALID_AUDIO_CONTROL_ID;
    TAudioControlID SATLInternalControlIDs::nMuteAllTriggerID = INVALID_AUDIO_CONTROL_ID;
    TAudioControlID SATLInternalControlIDs::nUnmuteAllTriggerID = INVALID_AUDIO_CONTROL_ID;
    TAudioControlID SATLInternalControlIDs::nDoNothingTriggerID = INVALID_AUDIO_CONTROL_ID;
    TAudioControlID SATLInternalControlIDs::nObjectSpeedRtpcID = INVALID_AUDIO_CONTROL_ID;
    TAudioControlID SATLInternalControlIDs::nObstructionOcclusionCalcSwitchID = INVALID_AUDIO_CONTROL_ID;
    TAudioSwitchStateID SATLInternalControlIDs::nOOCStateIDs[eAOOCT_COUNT] =
    {
        INVALID_AUDIO_SWITCH_STATE_ID,
        INVALID_AUDIO_SWITCH_STATE_ID,
        INVALID_AUDIO_SWITCH_STATE_ID,
        INVALID_AUDIO_SWITCH_STATE_ID,
    };
    TAudioControlID SATLInternalControlIDs::nObjectVelocityTrackingSwitchID = INVALID_AUDIO_CONTROL_ID;
    TAudioSwitchStateID SATLInternalControlIDs::nOVTOnStateID = INVALID_AUDIO_SWITCH_STATE_ID;
    TAudioSwitchStateID SATLInternalControlIDs::nOVTOffStateID = INVALID_AUDIO_SWITCH_STATE_ID;
    TAudioPreloadRequestID SATLInternalControlIDs::nGlobalPreloadRequestID = INVALID_AUDIO_PRELOAD_REQUEST_ID;


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void InitATLControlIDs()
    {
        // initializes the values in SATLInternalControlIDs
        SATLInternalControlIDs::nLoseFocusTriggerID = AudioStringToID<TAudioControlID>(SATLInternalControlNames::sLoseFocusName);
        SATLInternalControlIDs::nGetFocusTriggerID = AudioStringToID<TAudioControlID>(SATLInternalControlNames::sGetFocusName);
        SATLInternalControlIDs::nMuteAllTriggerID = AudioStringToID<TAudioControlID>(SATLInternalControlNames::sMuteAllName);
        SATLInternalControlIDs::nUnmuteAllTriggerID = AudioStringToID<TAudioControlID>(SATLInternalControlNames::sUnmuteAllName);
        SATLInternalControlIDs::nDoNothingTriggerID = AudioStringToID<TAudioControlID>(SATLInternalControlNames::sDoNothingName);
        SATLInternalControlIDs::nObjectSpeedRtpcID = AudioStringToID<TAudioControlID>(SATLInternalControlNames::sObjectSpeedName);

        SATLInternalControlIDs::nObstructionOcclusionCalcSwitchID = AudioStringToID<TAudioControlID>(SATLInternalControlNames::sObstructionOcclusionCalcName);
        SATLInternalControlIDs::nOOCStateIDs[eAOOCT_IGNORE] = AudioStringToID<TAudioSwitchStateID>(SATLInternalControlNames::sOOCIgnoreStateName);
        SATLInternalControlIDs::nOOCStateIDs[eAOOCT_SINGLE_RAY] = AudioStringToID<TAudioSwitchStateID>(SATLInternalControlNames::sOOCSingleRayStateName);
        SATLInternalControlIDs::nOOCStateIDs[eAOOCT_MULTI_RAY] = AudioStringToID<TAudioSwitchStateID>(SATLInternalControlNames::sOOCMultiRayStateName);

        SATLInternalControlIDs::nObjectVelocityTrackingSwitchID = AudioStringToID<TAudioControlID>(SATLInternalControlNames::sObjectVelocityTrackingName);
        SATLInternalControlIDs::nOVTOnStateID = AudioStringToID<TAudioSwitchStateID>(SATLInternalControlNames::sOVTOnStateName);
        SATLInternalControlIDs::nOVTOffStateID = AudioStringToID<TAudioSwitchStateID>(SATLInternalControlNames::sOVTOffStateName);

        SATLInternalControlIDs::nGlobalPreloadRequestID = AudioStringToID<TAudioPreloadRequestID>(SATLInternalControlNames::sGlobalPreloadRequestName);
    }

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLDebugNameStore::CATLDebugNameStore()
        : m_bATLObjectsChanged(false)
        , m_bATLTriggersChanged(false)
        , m_bATLRtpcsChanged(false)
        , m_bATLSwitchesChanged(false)
        , m_bATLPreloadsChanged(false)
        , m_bATLEnvironmentsChanged(false)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLDebugNameStore::~CATLDebugNameStore()
    {
        // the containers only hold numbers and strings, no ATL specific objects,
        // so there is no need to call the implementation to do the cleanup
        stl::free_container(m_cATLObjectNames);
        stl::free_container(m_cATLTriggerNames);
        stl::free_container(m_cATLSwitchNames);
        stl::free_container(m_cATLRtpcNames);
        stl::free_container(m_cATLPreloadRequestNames);
        stl::free_container(m_cATLEnvironmentNames);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::SyncChanges(const CATLDebugNameStore& rOtherNameStore)
    {
        if (rOtherNameStore.m_bATLObjectsChanged)
        {
            m_cATLObjectNames.clear();
            m_cATLObjectNames.insert(rOtherNameStore.m_cATLObjectNames.begin(), rOtherNameStore.m_cATLObjectNames.end());
            rOtherNameStore.m_bATLObjectsChanged = false;
        }

        if (rOtherNameStore.m_bATLTriggersChanged)
        {
            m_cATLTriggerNames.clear();
            m_cATLTriggerNames.insert(rOtherNameStore.m_cATLTriggerNames.begin(), rOtherNameStore.m_cATLTriggerNames.end());
            rOtherNameStore.m_bATLTriggersChanged = false;
        }

        if (rOtherNameStore.m_bATLRtpcsChanged)
        {
            m_cATLRtpcNames.clear();
            m_cATLRtpcNames.insert(rOtherNameStore.m_cATLRtpcNames.begin(), rOtherNameStore.m_cATLRtpcNames.end());
            rOtherNameStore.m_bATLRtpcsChanged = false;
        }

        if (rOtherNameStore.m_bATLSwitchesChanged)
        {
            m_cATLSwitchNames.clear();
            m_cATLSwitchNames.insert(rOtherNameStore.m_cATLSwitchNames.begin(), rOtherNameStore.m_cATLSwitchNames.end());
            rOtherNameStore.m_bATLSwitchesChanged = false;
        }

        if (rOtherNameStore.m_bATLPreloadsChanged)
        {
            m_cATLPreloadRequestNames.clear();
            m_cATLPreloadRequestNames.insert(rOtherNameStore.m_cATLPreloadRequestNames.begin(), rOtherNameStore.m_cATLPreloadRequestNames.end());
            rOtherNameStore.m_bATLPreloadsChanged = false;
        }

        if (rOtherNameStore.m_bATLEnvironmentsChanged)
        {
            m_cATLEnvironmentNames.clear();
            m_cATLEnvironmentNames.insert(rOtherNameStore.m_cATLEnvironmentNames.begin(), rOtherNameStore.m_cATLEnvironmentNames.end());
            rOtherNameStore.m_bATLEnvironmentsChanged = false;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::AddAudioObject(const TAudioObjectID nObjectID, const char* const sName)
    {
        m_cATLObjectNames[nObjectID] = sName;
        m_bATLObjectsChanged = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::AddAudioTrigger(const TAudioControlID nTriggerID, const char* const sName)
    {
        m_cATLTriggerNames[nTriggerID] = sName;
        m_bATLTriggersChanged = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::AddAudioRtpc(const TAudioControlID nRtpcID, const char* const sName)
    {
        m_cATLRtpcNames[nRtpcID] = sName;
        m_bATLRtpcsChanged = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::AddAudioSwitch(const TAudioControlID nSwitchID, const char* const sName)
    {
        m_cATLSwitchNames[nSwitchID] = AZStd::make_pair(sName, TAudioSwitchStateMap());
        m_bATLSwitchesChanged = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::AddAudioSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID, const char* const sName)
    {
        m_cATLSwitchNames[nSwitchID].second[nStateID] = sName;
        m_bATLSwitchesChanged = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::AddAudioPreloadRequest(const TAudioPreloadRequestID nRequestID, const char* const sName)
    {
        m_cATLPreloadRequestNames[nRequestID] = sName;
        m_bATLPreloadsChanged = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::AddAudioEnvironment(const TAudioEnvironmentID nEnvironmentID, const char* const sName)
    {
        m_cATLEnvironmentNames[nEnvironmentID] = sName;
        m_bATLEnvironmentsChanged = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::RemoveAudioObject(const TAudioObjectID nObjectID)
    {
        auto num = m_cATLObjectNames.erase(nObjectID);
        m_bATLObjectsChanged |= (num > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::RemoveAudioTrigger(const TAudioControlID nTriggerID)
    {
        auto num = m_cATLTriggerNames.erase(nTriggerID);
        m_bATLTriggersChanged |= (num > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::RemoveAudioRtpc(const TAudioControlID nRtpcID)
    {
        auto num = m_cATLRtpcNames.erase(nRtpcID);
        m_bATLRtpcsChanged |= (num > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::RemoveAudioSwitch(const TAudioControlID nSwitchID)
    {
        auto num = m_cATLSwitchNames.erase(nSwitchID);
        m_bATLSwitchesChanged |= (num > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::RemoveAudioSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID)
    {
        TAudioSwitchMap::iterator iPlace = m_cATLSwitchNames.begin();
        if (FindPlace(m_cATLSwitchNames, nSwitchID, iPlace))
        {
            auto num = iPlace->second.second.erase(nStateID);
            m_bATLSwitchesChanged |= (num > 0);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::RemoveAudioPreloadRequest(const TAudioPreloadRequestID nRequestID)
    {
        auto num = m_cATLPreloadRequestNames.erase(nRequestID);
        m_bATLPreloadsChanged |= (num > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::RemoveAudioEnvironment(const TAudioEnvironmentID nEnvironmentID)
    {
        auto num = m_cATLEnvironmentNames.erase(nEnvironmentID);
        m_bATLEnvironmentsChanged |= (num > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CATLDebugNameStore::LookupAudioObjectName(const TAudioObjectID nObjectID) const
    {
        TAudioObjectMap::const_iterator iterObject(m_cATLObjectNames.begin());
        const char* sResult = nullptr;

        if (FindPlaceConst(m_cATLObjectNames, nObjectID, iterObject))
        {
            sResult = iterObject->second.c_str();
        }
        else if (nObjectID == GLOBAL_AUDIO_OBJECT_ID)
        {
            sResult = "GlobalAudioObject";
        }

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CATLDebugNameStore::LookupAudioTriggerName(const TAudioControlID nTriggerID) const
    {
        TAudioControlMap::const_iterator iterTrigger(m_cATLTriggerNames.begin());
        const char* sResult = nullptr;

        if (FindPlaceConst(m_cATLTriggerNames, nTriggerID, iterTrigger))
        {
            sResult = iterTrigger->second.c_str();
        }

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CATLDebugNameStore::LookupAudioRtpcName(const TAudioControlID nRtpcID) const
    {
        TAudioControlMap::const_iterator iterRtpc(m_cATLRtpcNames.begin());
        char const* sResult = nullptr;

        if (FindPlaceConst(m_cATLRtpcNames, nRtpcID, iterRtpc))
        {
            sResult = iterRtpc->second.c_str();
        }

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CATLDebugNameStore::LookupAudioSwitchName(const TAudioControlID nSwitchID) const
    {
        TAudioSwitchMap::const_iterator iterSwitch(m_cATLSwitchNames.begin());
        const char* sResult = nullptr;

        if (FindPlaceConst(m_cATLSwitchNames, nSwitchID, iterSwitch))
        {
            sResult = iterSwitch->second.first.c_str();
        }

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CATLDebugNameStore::LookupAudioSwitchStateName(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID) const
    {
        TAudioSwitchMap::const_iterator iterSwitch(m_cATLSwitchNames.begin());
        const char* sResult = nullptr;

        if (FindPlaceConst(m_cATLSwitchNames, nSwitchID, iterSwitch))
        {
            const TAudioSwitchStateMap cStates = iterSwitch->second.second;
            TAudioSwitchStateMap::const_iterator iterState = cStates.begin();

            if (FindPlaceConst(cStates, nStateID, iterState))
            {
                sResult = iterState->second.c_str();
            }
        }

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CATLDebugNameStore::LookupAudioPreloadRequestName(const TAudioPreloadRequestID nRequestID) const
    {
        TAudioPreloadRequestsMap::const_iterator iterPreload(m_cATLPreloadRequestNames.begin());
        const char* sResult = nullptr;

        if (FindPlaceConst(m_cATLPreloadRequestNames, nRequestID, iterPreload))
        {
            sResult = iterPreload->second.c_str();
        }

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CATLDebugNameStore::LookupAudioEnvironmentName(const TAudioEnvironmentID nEnvironmentID) const
    {
        TAudioEnvironmentMap::const_iterator iterEnvironment(m_cATLEnvironmentNames.begin());
        const char* sResult = nullptr;

        if (FindPlaceConst(m_cATLEnvironmentNames, nEnvironmentID, iterEnvironment))
        {
            sResult = iterEnvironment->second.c_str();
        }

        return sResult;
    }
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
} // namespace Audio
