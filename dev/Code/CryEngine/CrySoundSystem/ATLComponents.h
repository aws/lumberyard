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

#pragma once

#include <IAudioSystem.h>

#include <ATLAudioObject.h>
#include <ATLEntities.h>
#include <ATLUtils.h>
#include <FileCacheManager.h>

class CATLAudioObjectBase;

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class AudioObjectIDFactory
    {
    public:
        static TAudioObjectID GetNextID();

        static const TAudioObjectID s_invalidAudioObjectID;
        static const TAudioObjectID s_globalAudioObjectID;
        static const TAudioObjectID s_minValidAudioObjectID;
        static const TAudioObjectID s_maxValidAudioObjectID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioEventManager
    {
    public:
        CAudioEventManager();
        ~CAudioEventManager();

        CAudioEventManager(const CAudioEventManager&) = delete;         // not defined; calls will fail at compile time
        CAudioEventManager& operator=(const CAudioEventManager&) = delete; // not defined; calls will fail at compile time

        void Initialize();
        void Release();
        void Update(const float fUpdateIntervalMS);

        CATLEvent* GetEvent(const EATLSubsystem eSender);
        CATLEvent* LookupID(const TAudioEventID nID) const;
        void ReleaseEvent(CATLEvent* const pEvent);

        size_t GetNumActive() const;

    private:
        CATLEvent* GetImplInstance();
        void ReleaseImplInstance(CATLEvent* const pOldEvent);
        CATLEvent* GetInternalInstance();
        void ReleaseInternalInstance(CATLEvent* const pOldEvent);

        using TActiveEventMap = ATLMapLookupType<TAudioEventID, CATLEvent*>;
        TActiveEventMap m_cActiveAudioEvents;

        CInstanceManager<CATLEvent, TAudioEventID> m_oAudioEventPool;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
    public:
        void SetDebugNameStore(const CATLDebugNameStore* const pDebugNameStore);
        void DrawDebugInfo(IRenderAuxGeom& rAuxGeom, float fPosX, float fPosY) const;

    private:
        const CATLDebugNameStore* m_pDebugNameStore;
#endif //INCLUDE_AUDIO_PRODUCTION_CODE
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioObjectManager
    {
    public:
        CAudioObjectManager(CAudioEventManager& refAudioEventManager);
        ~CAudioObjectManager();

        CAudioObjectManager(const CAudioObjectManager&) = delete;           // not defined; calls will fail at compile time
        CAudioObjectManager& operator=(const CAudioObjectManager&) = delete; // not defined; calls will fail at compile time

        void Initialize();
        void Release();
        void Update(const float fUpdateIntervalMS, const SATLWorldPosition& rListenerPosition);

        bool ReserveID(TAudioObjectID& rAudioObjectID);
        bool ReleaseID(const TAudioObjectID nAudioObjectID);
        CATLAudioObject* LookupID(const TAudioObjectID nAudioObjectID) const;

        void ReportStartedEvent(const CATLEvent* const pEvent);
        void ReportFinishedEvent(const CATLEvent* const pEvent, const bool bSuccess);
        void ReportObstructionRay(const TAudioObjectID nAudioObjectID, const size_t nRayID);

        void ReleasePendingRays();

        bool HasActiveEvents(const CATLAudioObjectBase* const pAudioObject) const;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        using TActiveObjectMap = ATLMapLookupType<TAudioObjectID, CATLAudioObject*>;

        bool ReserveID(TAudioObjectID& rAudioObjectID, const char* const sAudioObjectName);
        void SetDebugNameStore(CATLDebugNameStore* const pDebugNameStore);
        size_t GetNumAudioObjects() const;
        size_t GetNumActiveAudioObjects() const;
        const TActiveObjectMap& GetActiveAudioObjects() const
        {
            return m_cAudioObjects;
        }
        void DrawPerObjectDebugInfo(IRenderAuxGeom& rAuxGeom, Vec3 const& rListenerPos) const;
        void DrawDebugInfo(IRenderAuxGeom& rAuxGeom, float fPosX, float fPosY) const;

    private:
        CATLDebugNameStore* m_pDebugNameStore;
#else //INCLUDE_AUDIO_PRODUCTION_CODE
    private:
        using TActiveObjectMap = ATLMapLookupType<TAudioObjectID, CATLAudioObject*>;
#endif //INCLUDE_AUDIO_PRODUCTION_CODE

        static float s_fVelocityUpdateIntervalMS;

        CATLAudioObject* GetInstance();
        bool ReleaseInstance(CATLAudioObject* const pOldObject);

        TActiveObjectMap m_cAudioObjects;
        CInstanceManager<CATLAudioObject, TAudioObjectID> m_cObjectPool;
        float m_fTimeSinceLastVelocityUpdateMS;

        CAudioEventManager& m_refAudioEventManager;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioListenerManager
    {
    public:
        CAudioListenerManager();
        ~CAudioListenerManager();

        CAudioListenerManager(const CAudioListenerManager&) = delete;           // not defined; calls will fail at compile time
        CAudioListenerManager& operator=(const CAudioListenerManager&) = delete; // not defined; calls will fail at compile time

        void Initialize();
        void Release();
        void Update(const float fUpdateIntervalMS);

        bool ReserveID(TAudioObjectID& rAudioObjectID);
        bool ReleaseID(const TAudioObjectID nAudioObjectID);
        CATLListenerObject* LookupID(const TAudioObjectID nAudioObjectID) const;

        size_t GetNumActive() const;
        void GetDefaultListenerPosition(SATLWorldPosition& oPosition);
        TAudioObjectID GetDefaultListenerID() const
        {
            return m_nDefaultListenerID;
        }
        bool SetOverrideListenerID(const TAudioObjectID nAudioObjectID);
        TAudioObjectID GetOverrideListenerID() const
        {
            return m_listenerOverrideID;
        }

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        void DrawDebugInfo(IRenderAuxGeom& rAuxGeom) const;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

    private:
        using TListenerPtrContainer = AZStd::vector<CATLListenerObject*, Audio::AudioSystemStdAllocator>;
        using TActiveListenerMap = ATLMapLookupType<TAudioObjectID, CATLListenerObject*>;

        TActiveListenerMap m_cActiveListeners;
        TListenerPtrContainer m_cListenerPool;

        CATLListenerObject* m_pDefaultListenerObject;
        const TAudioObjectID m_nDefaultListenerID;
        TAudioObjectID m_listenerOverrideID;

        // No longer have a maximum, but we can create a number of additional listeners at startup.
        // TODO: Control this by a cvar
        const size_t m_numReservedListeners = 8;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioEventListenerManager
    {
    public:
        CAudioEventListenerManager();
        ~CAudioEventListenerManager();

        CAudioEventListenerManager(const CAudioEventListenerManager& other) = delete;               // Copy protection
        CAudioEventListenerManager& operator=(const CAudioEventListenerManager& other) = delete;    // Copy protection

        void AddRequestListener(const SAudioEventListener& listener);
        void RemoveRequestListener(const SAudioEventListener& listener);
        void NotifyListener(const SAudioRequestInfo* const pRequestInfo);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        size_t GetNumEventListeners() const
        {
            return m_cListeners.size();
        }
#endif //INCLUDE_AUDIO_PRODUCTION_CODE

    private:
        using TListenerArray = AZStd::vector<SAudioEventListener, Audio::AudioSystemStdAllocator>;
        TListenerArray m_cListeners;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLXmlProcessor
    {
    public:
        CATLXmlProcessor(
            TATLTriggerLookup& rTriggers,
            TATLRtpcLookup& rRtpcs,
            TATLSwitchLookup& rSwitches,
            TATLEnvironmentLookup& rEnvironments,
            TATLPreloadRequestLookup& rPreloadRequests,
            CFileCacheManager& rFileCacheMgr);

        ~CATLXmlProcessor();

        void Initialize();
        void Release();

        void ParseControlsData(const char* const sFolderPath, const EATLDataScope eDataScope);
        void ClearControlsData(const EATLDataScope eDataScope);
        void ParsePreloadsData(const char* const sFolderPath, const EATLDataScope eDataScope);
        void ClearPreloadsData(const EATLDataScope eDataScope);

    private:
        void ParseAudioTriggers(const XmlNodeRef pXMLTriggerRoot, const EATLDataScope eDataScope);
        void ParseAudioSwitches(const XmlNodeRef pXMLSwitchRoot, const EATLDataScope eDataScope);
        void ParseAudioRtpcs(const XmlNodeRef pXMLRtpcRoot, const EATLDataScope eDataScope);
        void ParseAudioPreloads(const XmlNodeRef pPreloadDataRoot, const EATLDataScope eDataScope, const char* const sFolderName);
        void ParseAudioEnvironments(const XmlNodeRef pAudioEnvironmentRoot, const EATLDataScope eDataScope);

        const IATLTriggerImplData* NewAudioTriggerImplDataInternal(const XmlNodeRef pXMLTriggerRoot);
        const IATLRtpcImplData* NewAudioRtpcImplDataInternal(const XmlNodeRef pXMLRtpcRoot);
        const IATLSwitchStateImplData* NewAudioSwitchStateImplDataInternal(const XmlNodeRef pXMLSwitchRoot);
        const IATLEnvironmentImplData* NewAudioEnvironmentImplDataInternal(const XmlNodeRef pXMLEnvironmentRoot);

        void DeleteAudioTrigger(const CATLTrigger* const pOldTrigger);
        void DeleteAudioRtpc(const CATLRtpc* const pOldRtpc);
        void DeleteAudioSwitch(const CATLSwitch* const pOldSwitch);
        void DeleteAudioPreloadRequest(const CATLPreloadRequest* const pOldPreloadRequest);
        void DeleteAudioEnvironment(const CATLAudioEnvironment* const pOldEnvironment);

        TATLTriggerLookup& m_rTriggers;
        TATLRtpcLookup& m_rRtpcs;
        TATLSwitchLookup& m_rSwitches;
        TATLEnvironmentLookup& m_rEnvironments;
        TATLPreloadRequestLookup& m_rPreloadRequests;

        TAudioTriggerImplID m_nTriggerImplIDCounter;

        CFileCacheManager& m_rFileCacheMgr;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
    public:
        void SetDebugNameStore(CATLDebugNameStore* const pDebugNameStore);

    private:
        CATLDebugNameStore* m_pDebugNameStore;
#endif //INCLUDE_AUDIO_PRODUCTION_CODE
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLSharedData
    {
        SATLSharedData();
        ~SATLSharedData();

        SATLWorldPosition m_oActiveListenerPosition;
    };
} // namespace Audio
