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
#include "ATLComponents.h"
#include "SoundCVars.h"
#include <IAudioSystemImplementation.h>
#include <IPhysics.h>
#include <ISurfaceType.h>
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
    #include <AzCore/std/string/conversions.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // AudioObjectIDFactory
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const TAudioObjectID AudioObjectIDFactory::s_invalidAudioObjectID = INVALID_AUDIO_OBJECT_ID;
    const TAudioObjectID AudioObjectIDFactory::s_globalAudioObjectID = GLOBAL_AUDIO_OBJECT_ID;
    const TAudioObjectID AudioObjectIDFactory::s_minValidAudioObjectID = (GLOBAL_AUDIO_OBJECT_ID + 1);
    const TAudioObjectID AudioObjectIDFactory::s_maxValidAudioObjectID = static_cast<TAudioObjectID>(-256);
    // Beyond the max ID value, allow for a range of 255 ID values which will be reserved for the audio middleware.

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // static
    TAudioObjectID AudioObjectIDFactory::GetNextID()
    {
        static TAudioObjectID s_nextId = s_minValidAudioObjectID;

        return (s_nextId <= s_maxValidAudioObjectID ? s_nextId++ : s_invalidAudioObjectID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //  CAudioEventManager
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioEventManager::CAudioEventManager()
        : m_oAudioEventPool(g_audioCVars.m_nAudioEventPoolSize, 1)
    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        , m_pDebugNameStore(nullptr)
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioEventManager::~CAudioEventManager()
    {
        Release();

        stl::free_container(m_oAudioEventPool.m_cReserved);
        stl::free_container(m_cActiveAudioEvents);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::Initialize()
    {
        size_t const numActiveAudioEvents = m_cActiveAudioEvents.size();

        for (size_t i = 0; i < m_oAudioEventPool.m_nReserveSize - numActiveAudioEvents; ++i)
        {
            const TAudioEventID nEventID = m_oAudioEventPool.GetNextID();
            IATLEventData* pNewEventData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(pNewEventData, &AudioSystemImplementationRequestBus::Events::NewAudioEventData, nEventID);
            auto pNewEvent = azcreate(CATLEvent, (nEventID, eAS_AUDIO_SYSTEM_IMPLEMENTATION, pNewEventData), Audio::AudioSystemAllocator, "ATLEvent");
            m_oAudioEventPool.m_cReserved.push_back(pNewEvent);
        }

        TActiveEventMap::const_iterator Iter(m_cActiveAudioEvents.begin());
        TActiveEventMap::const_iterator const IterEnd(m_cActiveAudioEvents.end());

        for (; Iter != IterEnd; ++Iter)
        {
            CATLEvent* const pEvent = Iter->second;
            IATLEventData* pNewEventData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(pNewEventData, &AudioSystemImplementationRequestBus::Events::NewAudioEventData, pEvent->GetID());
            pEvent->m_pImplData = pNewEventData;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::Release()
    {
        for (auto audioEvent : m_oAudioEventPool.m_cReserved)
        {
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioEventData, audioEvent->m_pImplData);
            azdestroy(audioEvent, Audio::AudioSystemAllocator);
        }
        m_oAudioEventPool.m_cReserved.clear();

        TActiveEventMap::const_iterator IterActiveAudioEvents(m_cActiveAudioEvents.begin());
        TActiveEventMap::const_iterator const IterActiveAudioEventsEnd(m_cActiveAudioEvents.end());

        for (; IterActiveAudioEvents != IterActiveAudioEventsEnd; ++IterActiveAudioEvents)
        {
            CATLEvent* const pEvent = IterActiveAudioEvents->second;
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::ResetAudioEventData, pEvent->m_pImplData);
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioEventData, pEvent->m_pImplData);
            pEvent->m_pImplData = nullptr;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::Update(float fUpdateIntervalMS)
    {
        //TODO: implement
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLEvent* CAudioEventManager::GetEvent(const EATLSubsystem eSender)
    {
        CATLEvent* pATLEvent = nullptr;

        switch (eSender)
        {
            case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
            {
                pATLEvent = GetImplInstance();
                break;
            }
            case eAS_ATL_INTERNAL:
            {
                pATLEvent = GetInternalInstance();
                break;
            }
            default:
            {
                AZ_Assert(false, "Unknown sender specified in GetEvent (%d)", eSender);
                break;
            }
        }

        if (pATLEvent)
        {
            m_cActiveAudioEvents[pATLEvent->GetID()] = pATLEvent;
        }

        return pATLEvent;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLEvent* CAudioEventManager::LookupID(const TAudioEventID nID) const
    {
        auto iPlace = m_cActiveAudioEvents.begin();
        const bool bLookupResult = FindPlaceConst(m_cActiveAudioEvents, nID, iPlace);

        return bLookupResult ? iPlace->second : nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::ReleaseEvent(CATLEvent* const pEvent)
    {
        if (pEvent)
        {
            m_cActiveAudioEvents.erase(pEvent->GetID());

            switch (pEvent->m_eSender)
            {
                case eAS_AUDIO_SYSTEM_IMPLEMENTATION:
                {
                    ReleaseImplInstance(pEvent);
                    break;
                }
                case eAS_ATL_INTERNAL:
                {
                    ReleaseInternalInstance(pEvent);
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Unknown sender specified in ReleaseEvent (%d)", pEvent->m_eSender);
                    break;
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLEvent* CAudioEventManager::GetImplInstance()
    {
        // must be called within a block protected by a critical section!

        CATLEvent* pEvent = nullptr;

        if (!m_oAudioEventPool.m_cReserved.empty())
        {
            //have reserved instances
            pEvent = m_oAudioEventPool.m_cReserved.back();
            m_oAudioEventPool.m_cReserved.pop_back();
        }
        else
        {
            //need to get a new instance
            const TAudioEventID nNewID = m_oAudioEventPool.GetNextID();

            IATLEventData* pNewEventData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(pNewEventData, &AudioSystemImplementationRequestBus::Events::NewAudioEventData, nNewID);
            pEvent = azcreate(CATLEvent, (nNewID, eAS_AUDIO_SYSTEM_IMPLEMENTATION, pNewEventData), Audio::AudioSystemAllocator, "ATLEvent");

            if (!pEvent)
            {
                --m_oAudioEventPool.m_nIDCounter;

                g_audioLogger.Log(eALT_WARNING, "Failed to get a new instance of an AudioEvent from the implementation");
                //failed to get a new instance from the implementation
            }
        }

        return pEvent;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::ReleaseImplInstance(CATLEvent* const pOldEvent)
    {
        // must be called within a block protected by a critical section!

        if (pOldEvent)
        {
            pOldEvent->Clear();

            if (m_oAudioEventPool.m_cReserved.size() < m_oAudioEventPool.m_nReserveSize)
            {
                // can return the instance to the reserved pool
                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::ResetAudioEventData, pOldEvent->m_pImplData);
                m_oAudioEventPool.m_cReserved.push_back(pOldEvent);
            }
            else
            {
                // the reserve pool is full, can return the instance to the implementation to dispose
                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioEventData, pOldEvent->m_pImplData);
                azdestroy(pOldEvent, Audio::AudioSystemAllocator);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLEvent* CAudioEventManager::GetInternalInstance()
    {
        // must be called within a block protected by a critical section!

        AZ_Assert(false, "GetInternalInstance was called yet it has no implementation!"); // implement when it is needed
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::ReleaseInternalInstance(CATLEvent* const pOldEvent)
    {
        // must be called within a block protected by a critical section!

        AZ_Assert(false, "ReleaseInternalInstance was called yet it has no implementation!"); // implement when it is needed
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    size_t CAudioEventManager::GetNumActive() const
    {
        return m_cActiveAudioEvents.size();
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //  CAudioObjectManager
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioObjectManager::CAudioObjectManager(CAudioEventManager& refAudioEventManager)
        : m_cObjectPool(g_audioCVars.m_nAudioObjectPoolSize, AudioObjectIDFactory::s_minValidAudioObjectID)
        , m_fTimeSinceLastVelocityUpdateMS(0.0f)
        , m_refAudioEventManager(refAudioEventManager)
    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        , m_pDebugNameStore(nullptr)
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioObjectManager::~CAudioObjectManager()
    {
        stl::free_container(m_cObjectPool.m_cReserved);
        stl::free_container(m_cAudioObjects);
    }


    float CAudioObjectManager::s_fVelocityUpdateIntervalMS = 100.0f;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::Update(const float fUpdateIntervalMS, const SATLWorldPosition& rListenerPosition)
    {
    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        //reset the ray counts
        CATLAudioObject::CPropagationProcessor::s_nTotalAsyncPhysRays = 0;
        CATLAudioObject::CPropagationProcessor::s_nTotalSyncPhysRays = 0;
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE

        m_fTimeSinceLastVelocityUpdateMS += fUpdateIntervalMS;
        const bool bUpdateVelocity = m_fTimeSinceLastVelocityUpdateMS > s_fVelocityUpdateIntervalMS;

        for (auto& audioObjectPair : m_cAudioObjects)
        {
            CATLAudioObject* const pObject = audioObjectPair.second;

            if (HasActiveEvents(pObject))
            {
                pObject->Update(fUpdateIntervalMS, rListenerPosition);

                if (pObject->CanRunObstructionOcclusion())
                {
                    SATLSoundPropagationData oPropagationData;
                    pObject->GetPropagationData(oPropagationData);

                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::SetObstructionOcclusion,
                        pObject->GetImplDataPtr(),
                        oPropagationData.fObstruction,
                        oPropagationData.fOcclusion);
                }

                if (bUpdateVelocity && pObject->GetVelocityTracking())
                {
                    pObject->UpdateVelocity(m_fTimeSinceLastVelocityUpdateMS);
                }

                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::UpdateAudioObject, pObject->GetImplDataPtr());
            }
        }

        if (bUpdateVelocity)
        {
            m_fTimeSinceLastVelocityUpdateMS = 0.0f;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioObjectManager::ReserveID(TAudioObjectID& rAudioObjectID)
    {
        CATLAudioObject* const pNewObject = GetInstance();

        bool bSuccess = false;
        rAudioObjectID = INVALID_AUDIO_OBJECT_ID;

        if (pNewObject)
        {
            EAudioRequestStatus eImplResult = eARS_FAILURE;
            AudioSystemImplementationRequestBus::BroadcastResult(eImplResult, &AudioSystemImplementationRequestBus::Events::RegisterAudioObject, pNewObject->GetImplDataPtr(), nullptr);

            if (eImplResult == eARS_SUCCESS)
            {
                pNewObject->IncrementRefCount();
                rAudioObjectID = pNewObject->GetID();
                m_cAudioObjects.emplace(rAudioObjectID, pNewObject);
                bSuccess = true;
            }
            else
            {
                ReleaseInstance(pNewObject);
                bSuccess = false;
            }
        }

        return bSuccess;
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioObjectManager::ReleaseID(const TAudioObjectID nAudioObjectID)
    {
        bool bSuccess = false;
        CATLAudioObject* const pOldObject = LookupID(nAudioObjectID);

        if (pOldObject)
        {
            if (pOldObject->GetRefCount() < 2)
            {
                bSuccess = ReleaseInstance(pOldObject);
            }
            else
            {
                pOldObject->DecrementRefCount();
            }
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLAudioObject* CAudioObjectManager::LookupID(const TAudioObjectID nID) const
    {
        TActiveObjectMap::const_iterator iPlace;
        CATLAudioObject* pResult = nullptr;

        if (FindPlaceConst(m_cAudioObjects, nID, iPlace))
        {
            pResult = iPlace->second;
        }

        return pResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::ReportStartedEvent(CATLEvent const* const pEvent)
    {
        if (pEvent)
        {
            CATLAudioObject* const pObject = LookupID(pEvent->m_nObjectID);

            if (pObject)
            {
                pObject->ReportStartedEvent(pEvent);
            }
        #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
            else
            {
                g_audioLogger.Log(
                    eALT_WARNING,
                    "Failed to report starting event %u on object %s as it does not exist!",
                    pEvent->GetID(),
                    m_pDebugNameStore->LookupAudioObjectName(pEvent->m_nObjectID));
            }
        #endif // INCLUDE_AUDIO_PRODUCTION_CODE
        }
        else
        {
            g_audioLogger.Log(eALT_WARNING, "NULL pEvent in CAudioObjectManager::ReportStartedEvent");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::ReportFinishedEvent(const CATLEvent* const pEvent, const bool bSuccess)
    {
        if (pEvent)
        {
            CATLAudioObject* const pObject = LookupID(pEvent->m_nObjectID);

            if (pObject)
            {
                pObject->ReportFinishedEvent(pEvent, bSuccess);

                if (pObject->GetRefCount() == 0)
                {
                    ReleaseInstance(pObject);
                }
            }
        #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
            else
            {
                g_audioLogger.Log(
                    eALT_WARNING,
                    "Removing Event %u from Object %s: Object no longer exists!",
                    pEvent->GetID(),
                    m_pDebugNameStore->LookupAudioObjectName(pEvent->m_nObjectID));
            }
        #endif // INCLUDE_AUDIO_PRODUCTION_CODE
        }
        else
        {
            g_audioLogger.Log(eALT_WARNING, "nullptr pEvent in CAudioObjectManager::ReportFinishedEvent");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::ReportObstructionRay(const TAudioObjectID nAudioObjectID, const size_t nRayID)
    {
        CATLAudioObject* const pObject = LookupID(nAudioObjectID);

        if (pObject)
        {
            pObject->ReportPhysicsRayProcessed(nRayID);

            if (pObject->GetRefCount() == 0)
            {
                ReleaseInstance(pObject);
            }
        }
    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        else
        {
            g_audioLogger.Log(
                eALT_WARNING,
                "Reporting Ray %" PRISIZE_T " from Object %s: Object no longer exists!",
                nRayID,
                m_pDebugNameStore->LookupAudioObjectName(nAudioObjectID));
        }
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLAudioObject* CAudioObjectManager::GetInstance()
    {
        CATLAudioObject* pObject = nullptr;

        if (!m_cObjectPool.m_cReserved.empty())
        {
            //have reserved instances
            pObject = m_cObjectPool.m_cReserved.back();
            m_cObjectPool.m_cReserved.pop_back();
        }
        else
        {
            //need to get a new instance
            const TAudioObjectID nNewID = AudioObjectIDFactory::GetNextID();
            IATLAudioObjectData* pObjectData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(pObjectData, &AudioSystemImplementationRequestBus::Events::NewAudioObjectData, nNewID);
           
            size_t unallocatedMemorySize = AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().GetUnAllocatedMemory();

            const size_t minimalMemorySize = 100 * 1024;

            if (unallocatedMemorySize < minimalMemorySize)
            {
                AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().GarbageCollect();
                unallocatedMemorySize = AZ::AllocatorInstance<Audio::AudioSystemAllocator>::Get().GetUnAllocatedMemory();
            }

            if (unallocatedMemorySize >= minimalMemorySize)
            {
                pObject = azcreate(CATLAudioObject, (nNewID, pObjectData), Audio::AudioSystemAllocator, "ATLAudioObject");
            }

            if (!pObject)
            {
                --m_cObjectPool.m_nIDCounter;

                const char* msg = "Failed to get a new instance of an AudioObject from the implementation. "
                    "If this limit was reached from legitimate content creation and not a scripting error, "
                    "try increasing the Capacity of Audio::AudioSystemAllocator.";

                g_audioLogger.Log(eALT_FATAL, msg);
                //failed to get a new instance from the implementation
            }
        }

        return pObject;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioObjectManager::ReleaseInstance(CATLAudioObject* const pOldObject)
    {
        bool bSuccess = false;
        if (pOldObject)
        {
            const TAudioObjectID nObjectID = pOldObject->GetID();
            m_cAudioObjects.erase(nObjectID);

        #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
            m_pDebugNameStore->RemoveAudioObject(nObjectID);
            pOldObject->CheckBeforeRemoval(m_pDebugNameStore);
        #endif // INCLUDE_AUDIO_PRODUCTION_CODE

            pOldObject->Clear();
            EAudioRequestStatus eResult = eARS_FAILURE;
            AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::UnregisterAudioObject, pOldObject->GetImplDataPtr());
            bSuccess = (eResult == eARS_SUCCESS);

            if (m_cObjectPool.m_cReserved.size() < m_cObjectPool.m_nReserveSize)
            {
                // can return the instance to the reserved pool
                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::ResetAudioObject, pOldObject->GetImplDataPtr());
                m_cObjectPool.m_cReserved.push_back(pOldObject);
            }
            else
            {
                // the reserve pool is full, can return the instance to the implementation to dispose
                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioObjectData, pOldObject->GetImplDataPtr());
                azdestroy(pOldObject, Audio::AudioSystemAllocator);
            }
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::Initialize()
    {
        const size_t numRegisteredObjects = m_cAudioObjects.size();

        for (size_t i = 0; i < m_cObjectPool.m_nReserveSize - numRegisteredObjects; ++i)
        {
            const auto nObjectID = AudioObjectIDFactory::GetNextID();
            IATLAudioObjectData* pObjectData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(pObjectData, &AudioSystemImplementationRequestBus::Events::NewAudioObjectData, nObjectID);
            auto pObject = azcreate(CATLAudioObject, (nObjectID, pObjectData), Audio::AudioSystemAllocator, "ATLAudioObject");
            m_cObjectPool.m_cReserved.push_back(pObject);
        }

        TActiveObjectMap::const_iterator IterObjects(m_cAudioObjects.begin());
        TActiveObjectMap::const_iterator const IterObjectsEnd(m_cAudioObjects.end());

        for (; IterObjects != IterObjectsEnd; ++IterObjects)
        {
            CATLAudioObject* const pAudioObject = IterObjects->second;
            IATLAudioObjectData* pObjectData = nullptr;
            AudioSystemImplementationRequestBus::BroadcastResult(pObjectData, &AudioSystemImplementationRequestBus::Events::NewAudioObjectData, pAudioObject->GetID());
            pAudioObject->SetImplDataPtr(pObjectData);

            char const* szAudioObjectName = nullptr;
        #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
            szAudioObjectName = m_pDebugNameStore->LookupAudioObjectName(pAudioObject->GetID());
        #endif // INCLUDE_AUDIO_PRODUCTION_CODE

            EAudioRequestStatus eResult = eARS_FAILURE;
            AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::RegisterAudioObject, pAudioObject->GetImplDataPtr(), szAudioObjectName);
            AZ_Assert(eResult == eARS_SUCCESS, "RegisterAudioObject failed to register object named '%s'", szAudioObjectName);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::Release()
    {
        for (auto audioObject : m_cObjectPool.m_cReserved)
        {
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioObjectData, audioObject->GetImplDataPtr());
            azdestroy(audioObject, Audio::AudioSystemAllocator);
        }

        m_cObjectPool.m_cReserved.clear();

        TActiveObjectMap::const_iterator IterObjects(m_cAudioObjects.begin());
        TActiveObjectMap::const_iterator const IterObjectsEnd(m_cAudioObjects.end());

        for (; IterObjects != IterObjectsEnd; ++IterObjects)
        {
            CATLAudioObject* const pAudioObject = IterObjects->second;
            if (auto implObject = pAudioObject->GetImplDataPtr())
            {
                EAudioRequestStatus eResult = eARS_FAILURE;
                AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::UnregisterAudioObject, implObject);
                AZ_Error("CAudioObjectManager", eResult == eARS_SUCCESS, "Failed to Unregister Audio Object!");
                AudioSystemImplementationRequestBus::BroadcastResult(eResult, &AudioSystemImplementationRequestBus::Events::ResetAudioObject, implObject);
                AZ_Error("CAudioObjectManager", eResult = eARS_SUCCESS, "Failed to Reset Audio Object!");
                AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioObjectData, implObject);
                pAudioObject->SetImplDataPtr(nullptr);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::ReleasePendingRays()
    {
        if (!m_cAudioObjects.empty())
        {
            CInstanceManager<CATLAudioObject>::TPointerContainer aObjectsToRelease;

            for (auto& audioObjectPair : m_cAudioObjects)
            {
                CATLAudioObject* const pObject = audioObjectPair.second;

                if (pObject)
                {
                    pObject->ReleasePendingRays();

                    if (pObject->GetRefCount() == 0)
                    {
                        aObjectsToRelease.push_back(pObject);
                    }
                }
            }

            if (!aObjectsToRelease.empty())
            {
                for (auto audioObject : aObjectsToRelease)
                {
                    ReleaseInstance(audioObject);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioObjectManager::HasActiveEvents(const CATLAudioObjectBase* const pAudioObject) const
    {
        bool bFoundActiveEvent = false;

        const TObjectEventSet& rActiveEvents = pAudioObject->GetActiveEvents();
        TObjectEventSet::const_iterator IterActiveEvents(rActiveEvents.begin());
        TObjectEventSet::const_iterator const IterActiveEventsEnd(rActiveEvents.end());

        for (; IterActiveEvents != IterActiveEventsEnd; ++IterActiveEvents)
        {
            const CATLEvent* const pEvent = m_refAudioEventManager.LookupID(*IterActiveEvents);

            if (pEvent->IsPlaying())
            {
                bFoundActiveEvent = true;
                break;
            }
        }

        return bFoundActiveEvent;
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //  CAudioListenerManager
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioListenerManager::CAudioListenerManager()
        : m_pDefaultListenerObject(nullptr)
        , m_nDefaultListenerID(AudioObjectIDFactory::GetNextID())
        , m_listenerOverrideID(INVALID_AUDIO_OBJECT_ID)
    {
        m_cListenerPool.reserve(m_numReservedListeners);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioListenerManager::~CAudioListenerManager()
    {
        Release();

        stl::free_container(m_cListenerPool);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioListenerManager::Initialize()
    {
        IATLListenerData* pNewListenerData = nullptr;

        // Default listener...
        AudioSystemImplementationRequestBus::BroadcastResult(pNewListenerData, &AudioSystemImplementationRequestBus::Events::NewDefaultAudioListenerObjectData, m_nDefaultListenerID);
        m_pDefaultListenerObject = azcreate(CATLListenerObject, (m_nDefaultListenerID, pNewListenerData), Audio::AudioSystemAllocator, "ATLListenerObject-Default");
        if (m_pDefaultListenerObject)
        {
            m_cActiveListeners[m_nDefaultListenerID] = m_pDefaultListenerObject;
        }

        // Additional listeners...
        for (size_t listener = 0; listener < m_numReservedListeners; ++listener)
        {
            const TAudioObjectID listenerId = AudioObjectIDFactory::GetNextID();
            AudioSystemImplementationRequestBus::BroadcastResult(pNewListenerData, &AudioSystemImplementationRequestBus::Events::NewAudioListenerObjectData, listenerId);
            auto listenerObject = azcreate(CATLListenerObject, (listenerId, pNewListenerData), Audio::AudioSystemAllocator, "ATLListenerObject");
            m_cListenerPool.push_back(listenerObject);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioListenerManager::Release()
    {
        if (m_pDefaultListenerObject) // guard against double deletions
        {
            m_cActiveListeners.erase(m_nDefaultListenerID);

            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioListenerObjectData, m_pDefaultListenerObject->m_pImplData);
            azdestroy(m_pDefaultListenerObject, Audio::AudioSystemAllocator);
            m_pDefaultListenerObject = nullptr;
        }

        // Release any remaining active audio listeners back to the listener pool
        for (auto listener : m_cActiveListeners)
        {
            ReleaseID(listener.first);
        }

        // Delete all from the audio listener pool
        for (auto listener : m_cListenerPool)
        {
            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioListenerObjectData, listener->m_pImplData);
            azdestroy(listener, Audio::AudioSystemAllocator);
        }

        m_cListenerPool.clear();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioListenerManager::Update(const float fUpdateIntervalMS)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioListenerManager::ReserveID(TAudioObjectID& rAudioObjectID)
    {
        bool bSuccess = false;

        if (!m_cListenerPool.empty())
        {
            CATLListenerObject* pListener = m_cListenerPool.back();
            m_cListenerPool.pop_back();

            const TAudioObjectID nID = pListener->GetID();
            m_cActiveListeners.emplace(nID, pListener);

            rAudioObjectID = nID;
            bSuccess = true;
        }
        else
        {
            g_audioLogger.Log(eALT_WARNING, "CAudioListenerManager::ReserveID - Reserved pool of pre-allocated Audio Listeners has been exhausted!");
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioListenerManager::ReleaseID(const TAudioObjectID nAudioObjectID)
    {
        bool bSuccess = false;

        CATLListenerObject* pListener = LookupID(nAudioObjectID);
        if (pListener)
        {
            m_cActiveListeners.erase(nAudioObjectID);
            m_cListenerPool.push_back(pListener);
            bSuccess = true;
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLListenerObject* CAudioListenerManager::LookupID(const TAudioObjectID nID) const
    {
        CATLListenerObject* pListenerObject = nullptr;

        TActiveListenerMap::const_iterator iPlace = m_cActiveListeners.begin();

        if (FindPlaceConst(m_cActiveListeners, nID, iPlace))
        {
            pListenerObject = iPlace->second;
        }

        return pListenerObject;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    size_t CAudioListenerManager::GetNumActive() const
    {
        return m_cActiveListeners.size();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioListenerManager::GetDefaultListenerPosition(SATLWorldPosition& oPosition)
    {
        if (m_pDefaultListenerObject)
        {
            oPosition = m_pDefaultListenerObject->oPosition;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioListenerManager::SetOverrideListenerID(const TAudioObjectID nAudioObjectID)
    {
        // If passed ID is INVALID_AUDIO_OBJECT_ID, override is being turned off.
        TActiveListenerMap::const_iterator itBegin = m_cActiveListeners.begin();

        if (nAudioObjectID == INVALID_AUDIO_OBJECT_ID
            || FindPlaceConst(m_cActiveListeners, nAudioObjectID, itBegin))
        {
            m_listenerOverrideID = nAudioObjectID;
            return true;
        }

        return false;
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //  CAudioEventListenerManager
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioEventListenerManager::CAudioEventListenerManager()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CAudioEventListenerManager::~CAudioEventListenerManager()
    {
        stl::free_container(m_cListeners);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventListenerManager::AddRequestListener(const SAudioEventListener& listener)
    {
        for (const auto& currentListener : m_cListeners)
        {
            if (currentListener == listener)
            {
            #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                g_audioLogger.Log(eALT_WARNING, "AudioEventListenerManager::AddRequestListener - Request listener being added already exists!");
            #endif // INCLUDE_AUDIO_PRODUCTION_CODE
                return;
            }
        }

        m_cListeners.push_back(listener);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventListenerManager::RemoveRequestListener(const SAudioEventListener& listener)
    {
        for (auto iter = m_cListeners.begin(); iter != m_cListeners.end(); ++iter)
        {
            if ((iter->m_fnOnEvent == listener.m_fnOnEvent || listener.m_fnOnEvent == nullptr) && iter->m_callbackOwner == listener.m_callbackOwner)
            {
                // Copy the back element into this iter position and pop the back element...
                (*iter) = m_cListeners.back();
                m_cListeners.pop_back();
                return;
            }
        }

    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        g_audioLogger.Log(eALT_WARNING, "AudioEventListenerManager::RemoveRequestListener - Failed to remove a request listener (not found)!");
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventListenerManager::NotifyListener(const SAudioRequestInfo* const pResultInfo)
    {
        // This should always be on the main thread!
        FUNCTION_PROFILER_ALWAYS(gEnv->pSystem, PROFILE_AUDIO);

        auto found = AZStd::find_if(m_cListeners.begin(), m_cListeners.end(),
            [pResultInfo](const SAudioEventListener& currentListener)
            {
                // 1) Is the listener interested in this request type?
                // 2) Is the listener interested in this request sub-type?
                // 3) Is the listener interested in this owner (or any owner)?
                return ((currentListener.m_requestType == eART_AUDIO_ALL_REQUESTS || currentListener.m_requestType == pResultInfo->eAudioRequestType)
                    && ((currentListener.m_specificRequestMask & pResultInfo->nSpecificAudioRequest) != 0)
                    && (currentListener.m_callbackOwner == nullptr || currentListener.m_callbackOwner == pResultInfo->pOwner));
            }
        );

        if (found != m_cListeners.end())
        {
            found->m_fnOnEvent(pResultInfo);
        }
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //  CATLXMLProcessor
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLXmlProcessor::CATLXmlProcessor(
        TATLTriggerLookup& rTriggers,
        TATLRtpcLookup& rRtpcs,
        TATLSwitchLookup& rSwitches,
        TATLEnvironmentLookup& rEnvironments,
        TATLPreloadRequestLookup& rPreloadRequests,
        CFileCacheManager& rFileCacheMgr)
        : m_rTriggers(rTriggers)
        , m_rRtpcs(rRtpcs)
        , m_rSwitches(rSwitches)
        , m_rEnvironments(rEnvironments)
        , m_rPreloadRequests(rPreloadRequests)
        , m_nTriggerImplIDCounter(AUDIO_TRIGGER_IMPL_ID_NUM_RESERVED)
        , m_rFileCacheMgr(rFileCacheMgr)
    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
        , m_pDebugNameStore(nullptr)
    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLXmlProcessor::~CATLXmlProcessor()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::Initialize()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::Release()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ParseControlsData(const char* const sFolderPath, const EATLDataScope eDataScope)
    {
        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sRootFolderPath(sFolderPath);

        sRootFolderPath.TrimRight("/\\");
        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH + MAX_AUDIO_FILE_NAME_LENGTH> sSearch(sRootFolderPath + "/*.xml");
        _finddata_t fd;
        ZeroStruct(fd);
        intptr_t handle = gEnv->pCryPak->FindFirst(sSearch.c_str(), &fd);

        if (handle != -1)
        {
            CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH + MAX_AUDIO_FILE_NAME_LENGTH> sFileName;

            do
            {
                sFileName = sRootFolderPath.c_str();
                sFileName += PathUtil::GetSlash();
                sFileName += fd.name;

                const XmlNodeRef pATLConfigRoot(GetISystem()->LoadXmlFromFile(sFileName));

                if (pATLConfigRoot)
                {
                    if (azstricmp(pATLConfigRoot->getTag(), SATLXmlTags::sRootNodeTag) == 0)
                    {
                        const size_t nATLConfigChildrenCount = static_cast<size_t>(pATLConfigRoot->getChildCount());

                        for (size_t i = 0; i < nATLConfigChildrenCount; ++i)
                        {
                            const XmlNodeRef pAudioConfigNode(pATLConfigRoot->getChild(i));

                            if (pAudioConfigNode)
                            {
                                const char* const sAudioConfigNodeTag = pAudioConfigNode->getTag();

                                if (azstricmp(sAudioConfigNodeTag, SATLXmlTags::sTriggersNodeTag) == 0)
                                {
                                    ParseAudioTriggers(pAudioConfigNode, eDataScope);
                                }
                                else if (azstricmp(sAudioConfigNodeTag, SATLXmlTags::sRtpcsNodeTag) == 0)
                                {
                                    ParseAudioRtpcs(pAudioConfigNode, eDataScope);
                                }
                                else if (azstricmp(sAudioConfigNodeTag, SATLXmlTags::sSwitchesNodeTag) == 0)
                                {
                                    ParseAudioSwitches(pAudioConfigNode, eDataScope);
                                }
                                else if (azstricmp(sAudioConfigNodeTag, SATLXmlTags::sEnvironmentsNodeTag) == 0)
                                {
                                    ParseAudioEnvironments(pAudioConfigNode, eDataScope);
                                }
                                else if (azstricmp(sAudioConfigNodeTag, SATLXmlTags::sPreloadsNodeTag) == 0)
                                {
                                    // This tag is valid but ignored here.
                                }
                                else
                                {
                                    g_audioLogger.Log(eALT_ERROR, "ParseControlsData - Unknown xml node '%s'", sAudioConfigNodeTag);
                                }
                            }
                        }
                    }
                }
            } while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);

            gEnv->pCryPak->FindClose(handle);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ParsePreloadsData(const char* const sFolderPath, const EATLDataScope eDataScope)
    {
        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sRootFolderPath(sFolderPath);

        sRootFolderPath.TrimRight("/\\");
        CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH + MAX_AUDIO_FILE_NAME_LENGTH> sSearch(sRootFolderPath + "/*.xml");
        _finddata_t fd;
        ZeroStruct(fd);
        intptr_t handle = gEnv->pCryPak->FindFirst(sSearch.c_str(), &fd);

        if (handle != -1)
        {
            CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH + MAX_AUDIO_FILE_NAME_LENGTH> sFileName;

            do
            {
                sFileName = sRootFolderPath.c_str();
                sFileName += PathUtil::GetSlash();
                sFileName += fd.name;

                const XmlNodeRef pATLConfigRoot(GetISystem()->LoadXmlFromFile(sFileName));

                if (pATLConfigRoot)
                {
                    if (azstricmp(pATLConfigRoot->getTag(), SATLXmlTags::sRootNodeTag) == 0)
                    {
                        const size_t nATLConfigChildrenCount = static_cast<size_t>(pATLConfigRoot->getChildCount());

                        for (size_t i = 0; i < nATLConfigChildrenCount; ++i)
                        {
                            const XmlNodeRef pAudioConfigNode(pATLConfigRoot->getChild(i));

                            if (pAudioConfigNode)
                            {
                                const char* const sAudioConfigNodeTag = pAudioConfigNode->getTag();

                                if (azstricmp(sAudioConfigNodeTag, SATLXmlTags::sPreloadsNodeTag) == 0)
                                {
                                    const size_t nLastSlashIndex = sRootFolderPath.rfind('/');

                                    if (sRootFolderPath.npos != nLastSlashIndex)
                                    {
                                        const CryFixedStringT<MAX_AUDIO_FILE_PATH_LENGTH> sFolderName(sRootFolderPath.substr(nLastSlashIndex + 1, sRootFolderPath.size()));
                                        ParseAudioPreloads(pAudioConfigNode, eDataScope, sFolderName.c_str());
                                    }
                                    else
                                    {
                                        ParseAudioPreloads(pAudioConfigNode, eDataScope, nullptr);
                                    }
                                }
                                else if (azstricmp(sAudioConfigNodeTag, SATLXmlTags::sTriggersNodeTag) == 0
                                    || azstricmp(sAudioConfigNodeTag, SATLXmlTags::sRtpcsNodeTag) == 0
                                    || azstricmp(sAudioConfigNodeTag, SATLXmlTags::sSwitchesNodeTag) == 0
                                    || azstricmp(sAudioConfigNodeTag, SATLXmlTags::sEnvironmentsNodeTag) == 0
                                    )
                                {
                                    // These tags are valid but ignored here.
                                }
                                else
                                {
                                    g_audioLogger.Log(eALT_ERROR, "ParsePreloadsData - Unknown xml node '%s'", sAudioConfigNodeTag);
                                }
                            }
                        }
                    }
                }
            } while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);

            gEnv->pCryPak->FindClose(handle);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ClearControlsData(const EATLDataScope eDataScope)
    {
        // Remove Triggers...
        for (auto itRemove = m_rTriggers.begin(); itRemove != m_rTriggers.end(); )
        {
            auto const pTrigger = itRemove->second;
            if (eDataScope == eADS_ALL || (pTrigger->GetDataScope() == eDataScope))
            {
            #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                m_pDebugNameStore->RemoveAudioTrigger(pTrigger->GetID());
            #endif // INCLUDE_AUDIO_PRODUCTION_CODE

                DeleteAudioTrigger(pTrigger);
                itRemove = m_rTriggers.erase(itRemove);
            }
            else
            {
                ++itRemove;
            }
        }

        // Remove Rtpcs...
        for (auto itRemove = m_rRtpcs.begin(); itRemove != m_rRtpcs.end(); )
        {
            auto const pRtpc = itRemove->second;
            if (eDataScope == eADS_ALL || (pRtpc->GetDataScope() == eDataScope))
            {
            #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                m_pDebugNameStore->RemoveAudioRtpc(pRtpc->GetID());
            #endif // INCLUDE_AUDIO_PRODUCTION_CODE

                DeleteAudioRtpc(pRtpc);
                itRemove = m_rRtpcs.erase(itRemove);
            }
            else
            {
                ++itRemove;
            }
        }

        // Remove Switches...
        for (auto itRemove = m_rSwitches.begin(); itRemove != m_rSwitches.end(); )
        {
            auto const pSwitch = itRemove->second;
            if (eDataScope == eADS_ALL || (pSwitch->GetDataScope() == eDataScope))
            {
            #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                m_pDebugNameStore->RemoveAudioSwitch(pSwitch->GetID());
            #endif // INCLUDE_AUDIO_PRODUCTION_CODE

                DeleteAudioSwitch(pSwitch);
                itRemove = m_rSwitches.erase(itRemove);
            }
            else
            {
                ++itRemove;
            }
        }

        // Remove Environments...
        for (auto itRemove = m_rEnvironments.begin(); itRemove != m_rEnvironments.end(); )
        {
            auto const pEnvironment = itRemove->second;
            if (eDataScope == eADS_ALL || (pEnvironment->GetDataScope() == eDataScope))
            {
            #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                m_pDebugNameStore->RemoveAudioEnvironment(pEnvironment->GetID());
            #endif // INCLUDE_AUDIO_PRODUCTION_CODE

                DeleteAudioEnvironment(pEnvironment);
                itRemove = m_rEnvironments.erase(itRemove);
            }
            else
            {
                ++itRemove;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ParseAudioPreloads(const XmlNodeRef pPreloadDataRoot, const EATLDataScope eDataScope, const char* const sFolderName)
    {
        LOADING_TIME_PROFILE_SECTION;

        const size_t nPreloadRequestCount = static_cast<size_t>(pPreloadDataRoot->getChildCount());

        for (size_t i = 0; i < nPreloadRequestCount; ++i)
        {
            const XmlNodeRef pPreloadRequestNode(pPreloadDataRoot->getChild(i));

            if (pPreloadRequestNode && azstricmp(pPreloadRequestNode->getTag(), SATLXmlTags::sATLPreloadRequestTag) == 0)
            {
                TAudioPreloadRequestID nAudioPreloadRequestID = SATLInternalControlIDs::nGlobalPreloadRequestID;
                const char* sAudioPreloadRequestName = SATLInternalControlNames::sGlobalPreloadRequestName;
                const bool bAutoLoad = (azstricmp(pPreloadRequestNode->getAttr(SATLXmlTags::sATLTypeAttribute), SATLXmlTags::sATLDataLoadType) == 0);

                if (!bAutoLoad)
                {
                    sAudioPreloadRequestName = pPreloadRequestNode->getAttr(SATLXmlTags::sATLNameAttribute);
                    nAudioPreloadRequestID = AudioStringToID<TAudioPreloadRequestID>(sAudioPreloadRequestName);
                }
                else if (eDataScope == eADS_LEVEL_SPECIFIC)
                {
                    sAudioPreloadRequestName = sFolderName;
                    nAudioPreloadRequestID = AudioStringToID<TAudioPreloadRequestID>(sAudioPreloadRequestName);
                }

                if (nAudioPreloadRequestID != INVALID_AUDIO_PRELOAD_REQUEST_ID)
                {
                    const size_t nPreloadRequestChidrenCount = static_cast<size_t>(pPreloadRequestNode->getChildCount());

                    if (nPreloadRequestChidrenCount > 1)
                    {
                        // We need to have at least two children: ATLPlatforms and at least one ATLConfigGroup
                        const XmlNodeRef pPlatformsNode(pPreloadRequestNode->getChild(0));
                        const char* sATLConfigGroupName = nullptr;

                        if (pPlatformsNode && azstricmp(pPlatformsNode->getTag(), SATLXmlTags::sATLPlatformsTag) == 0)
                        {
                            const size_t nPlatformCount = pPlatformsNode->getChildCount();

                            for (size_t j = 0; j < nPlatformCount; ++j)
                            {
                                const XmlNodeRef pPlatformNode(pPlatformsNode->getChild(j));

                                if (pPlatformNode && azstricmp(pPlatformNode->getAttr(SATLXmlTags::sATLNameAttribute), SATLXmlTags::sPlatform) == 0)
                                {
                                    sATLConfigGroupName = pPlatformNode->getAttr(SATLXmlTags::sATLConfigGroupAttribute);
                                    break;
                                }
                            }
                        }

                        if (sATLConfigGroupName)
                        {
                            for (size_t j = 1; j < nPreloadRequestChidrenCount; ++j)
                            {
                                const XmlNodeRef pConfigGroupNode(pPreloadRequestNode->getChild(j));

                                if (pConfigGroupNode && azstricmp(pConfigGroupNode->getAttr(SATLXmlTags::sATLNameAttribute), sATLConfigGroupName) == 0)
                                {
                                    // Found the config group corresponding to the specified platform.
                                    const size_t nFileCount = static_cast<size_t>(pConfigGroupNode->getChildCount());

                                    CATLPreloadRequest::TFileEntryIDs cFileEntryIDs;
                                    cFileEntryIDs.reserve(nFileCount);

                                    for (size_t k = 0; k < nFileCount; ++k)
                                    {
                                        const TAudioFileEntryID nID = m_rFileCacheMgr.TryAddFileCacheEntry(pConfigGroupNode->getChild(k), eDataScope, bAutoLoad);

                                        if (nID != INVALID_AUDIO_FILE_ENTRY_ID)
                                        {
                                            cFileEntryIDs.push_back(nID);
                                        }
                                        else
                                        {
                                            g_audioLogger.Log(eALT_WARNING, "Preload request \"%s\" could not create file entry from tag \"%s\"!", sAudioPreloadRequestName, pConfigGroupNode->getChild(k)->getTag());
                                        }
                                    }

                                    CATLPreloadRequest* pPreloadRequest = stl::find_in_map(m_rPreloadRequests, nAudioPreloadRequestID, nullptr);

                                    if (!pPreloadRequest)
                                    {
                                        pPreloadRequest = azcreate(CATLPreloadRequest, (nAudioPreloadRequestID, eDataScope, bAutoLoad, cFileEntryIDs), Audio::AudioSystemAllocator, "ATLPreloadRequest");

                                        if (pPreloadRequest)
                                        {
                                            m_rPreloadRequests[nAudioPreloadRequestID] = pPreloadRequest;

                                        #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                                            m_pDebugNameStore->AddAudioPreloadRequest(nAudioPreloadRequestID, sAudioPreloadRequestName);
                                        #endif // INCLUDE_AUDIO_PRODUCTION_CODE
                                        }
                                        else
                                        {
                                            g_audioLogger.Log(eALT_FATAL, "Failed to allocate CATLPreloadRequest");
                                        }
                                    }
                                    else
                                    {
                                        // Add to existing preload request.
                                        pPreloadRequest->m_cFileEntryIDs.insert(pPreloadRequest->m_cFileEntryIDs.end(), cFileEntryIDs.begin(), cFileEntryIDs.end());
                                    }

                                    // no need to look through the rest of the ConfigGroups
                                    break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    g_audioLogger.Log(eALT_WARNING, "Preload request '%s' already exists! Skipping this entry!", sAudioPreloadRequestName);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ClearPreloadsData(const EATLDataScope eDataScope)
    {
        for (auto itRemove = m_rPreloadRequests.begin(); itRemove != m_rPreloadRequests.end(); )
        {
            auto const pRequest = itRemove->second;
            if (eDataScope == eADS_ALL || (pRequest->GetDataScope() == eDataScope))
            {
            #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                m_pDebugNameStore->RemoveAudioPreloadRequest(pRequest->GetID());
            #endif // INCLUDE_AUDIO_PRODUCTION_CODE

                DeleteAudioPreloadRequest(pRequest);
                itRemove = m_rPreloadRequests.erase(itRemove);
            }
            else
            {
                ++itRemove;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ParseAudioEnvironments(const XmlNodeRef pAudioEnvironmentRoot, const EATLDataScope eDataScope)
    {
        const size_t nAudioEnvironmentCount = static_cast<size_t>(pAudioEnvironmentRoot->getChildCount());

        for (size_t i = 0; i < nAudioEnvironmentCount; ++i)
        {
            const XmlNodeRef pAudioEnvironmentNode(pAudioEnvironmentRoot->getChild(i));

            if (pAudioEnvironmentNode && azstricmp(pAudioEnvironmentNode->getTag(), SATLXmlTags::sATLEnvironmentTag) == 0)
            {
                const char* const sATLEnvironmentName = pAudioEnvironmentNode->getAttr(SATLXmlTags::sATLNameAttribute);
                const auto nATLEnvironmentID = AudioStringToID<TAudioEnvironmentID>(sATLEnvironmentName);

                if ((nATLEnvironmentID != INVALID_AUDIO_ENVIRONMENT_ID) && (stl::find_in_map(m_rEnvironments, nATLEnvironmentID, nullptr) == nullptr))
                {
                    //there is no entry yet with this ID in the container
                    const size_t nAudioEnvironmentChildrenCount = static_cast<size_t>(pAudioEnvironmentNode->getChildCount());

                    CATLAudioEnvironment::TImplPtrVec cImplPtrs;
                    cImplPtrs.reserve(nAudioEnvironmentChildrenCount);

                    for (size_t j = 0; j < nAudioEnvironmentChildrenCount; ++j)
                    {
                        const XmlNodeRef pEnvironmentImplNode(pAudioEnvironmentNode->getChild(j));

                        if (pEnvironmentImplNode)
                        {
                            const IATLEnvironmentImplData* pEnvironmentImplData = nullptr;
                            EATLSubsystem eReceiver = eAS_NONE;

                            if (azstricmp(pEnvironmentImplNode->getTag(), SATLXmlTags::sATLEnvironmentRequestTag) == 0)
                            {
                                pEnvironmentImplData = NewAudioEnvironmentImplDataInternal(pEnvironmentImplNode);
                                eReceiver = eAS_ATL_INTERNAL;
                            }
                            else
                            {
                                AudioSystemImplementationRequestBus::BroadcastResult(pEnvironmentImplData, &AudioSystemImplementationRequestBus::Events::NewAudioEnvironmentImplData, pEnvironmentImplNode);
                                eReceiver = eAS_AUDIO_SYSTEM_IMPLEMENTATION;
                            }

                            if (pEnvironmentImplData)
                            {
                                auto pEnvironmentImpl = azcreate(CATLEnvironmentImpl, (eReceiver, pEnvironmentImplData), Audio::AudioSystemAllocator, "ATLEnvironmentImpl");
                                cImplPtrs.push_back(pEnvironmentImpl);
                            }
                            else
                            {
                                g_audioLogger.Log(eALT_WARNING, "Could not parse an Environment Implementation with XML tag %s", pEnvironmentImplNode->getTag());
                            }
                        }
                    }

                    if (!cImplPtrs.empty())
                    {
                        auto pNewEnvironment = azcreate(CATLAudioEnvironment, (nATLEnvironmentID, eDataScope, cImplPtrs), Audio::AudioSystemAllocator, "ATLAudioEnvironment");

                        if (pNewEnvironment)
                        {
                            m_rEnvironments[nATLEnvironmentID] = pNewEnvironment;

                        #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                            m_pDebugNameStore->AddAudioEnvironment(nATLEnvironmentID, sATLEnvironmentName);
                        #endif // INCLUDE_AUDIO_PRODUCTION_CODE
                        }
                    }
                }
                else
                {
                    g_audioLogger.Log(eALT_ERROR, "ParseAudioEnvironments - Environment '%s' already exists!", sATLEnvironmentName);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ParseAudioTriggers(const XmlNodeRef pXMLTriggerRoot, const EATLDataScope eDataScope)
    {
        const size_t nAudioTriggersChildrenCount = static_cast<size_t>(pXMLTriggerRoot->getChildCount());

        for (size_t i = 0; i < nAudioTriggersChildrenCount; ++i)
        {
            const XmlNodeRef pAudioTriggerNode(pXMLTriggerRoot->getChild(i));

            if (pAudioTriggerNode && azstricmp(pAudioTriggerNode->getTag(), SATLXmlTags::sATLTriggerTag) == 0)
            {
                const char* const sATLTriggerName = pAudioTriggerNode->getAttr(SATLXmlTags::sATLNameAttribute);
                const auto nATLTriggerID = AudioStringToID<TAudioControlID>(sATLTriggerName);

                if ((nATLTriggerID != INVALID_AUDIO_CONTROL_ID) && (stl::find_in_map(m_rTriggers, nATLTriggerID, nullptr) == nullptr))
                {
                    //there is no entry yet with this ID in the container
                    const size_t nAudioTriggerChildrenCount = static_cast<size_t>(pAudioTriggerNode->getChildCount());

                    CATLTrigger::TImplPtrVec cImplPtrs;
                    cImplPtrs.reserve(nAudioTriggerChildrenCount);

                    for (size_t m = 0; m < nAudioTriggerChildrenCount; ++m)
                    {
                        const XmlNodeRef pTriggerImplNode(pAudioTriggerNode->getChild(m));

                        if (pTriggerImplNode)
                        {
                            const IATLTriggerImplData* pTriggerImplData = nullptr;
                            EATLSubsystem eReceiver = eAS_NONE;

                            if (azstricmp(pTriggerImplNode->getTag(), SATLXmlTags::sATLTriggerRequestTag) == 0)
                            {
                                pTriggerImplData = NewAudioTriggerImplDataInternal(pTriggerImplNode);
                                eReceiver = eAS_ATL_INTERNAL;
                            }
                            else
                            {
                                AudioSystemImplementationRequestBus::BroadcastResult(pTriggerImplData, &AudioSystemImplementationRequestBus::Events::NewAudioTriggerImplData, pTriggerImplNode);
                                eReceiver = eAS_AUDIO_SYSTEM_IMPLEMENTATION;
                            }

                            if (pTriggerImplData)
                            {
                                auto pTriggerImpl = azcreate(CATLTriggerImpl, (++m_nTriggerImplIDCounter, nATLTriggerID, eReceiver, pTriggerImplData), Audio::AudioSystemAllocator, "ATLTriggerImpl");
                                cImplPtrs.push_back(pTriggerImpl);
                            }
                            else
                            {
                                g_audioLogger.Log(eALT_WARNING, "Could not parse a Trigger Implementation with XML tag %s", pTriggerImplNode->getTag());
                            }
                        }
                    }

                    auto pNewTrigger = azcreate(CATLTrigger, (nATLTriggerID, eDataScope, cImplPtrs), Audio::AudioSystemAllocator, "ATLTrigger");

                    if (pNewTrigger)
                    {
                        m_rTriggers[nATLTriggerID] = pNewTrigger;

                    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                        m_pDebugNameStore->AddAudioTrigger(nATLTriggerID, sATLTriggerName);
                    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
                    }
                }
                else
                {
                    g_audioLogger.Log(eALT_ERROR, "ParseAudioTriggers - Trigger '%s' already exists!", sATLTriggerName);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ParseAudioSwitches(const XmlNodeRef pXMLSwitchRoot, const EATLDataScope eDataScope)
    {
        const size_t nAudioSwitchesChildrenCount = static_cast<size_t>(pXMLSwitchRoot->getChildCount());

        for (size_t i = 0; i < nAudioSwitchesChildrenCount; ++i)
        {
            const XmlNodeRef pATLSwitchNode(pXMLSwitchRoot->getChild(i));

            if (pATLSwitchNode && azstricmp(pATLSwitchNode->getTag(), SATLXmlTags::sATLSwitchTag) == 0)
            {
                const char* const sATLSwitchName = pATLSwitchNode->getAttr(SATLXmlTags::sATLNameAttribute);
                const auto nATLSwitchID = AudioStringToID<TAudioControlID>(sATLSwitchName);

                if ((nATLSwitchID != INVALID_AUDIO_CONTROL_ID) && (stl::find_in_map(m_rSwitches, nATLSwitchID, nullptr) == nullptr))
                {
                    auto pNewSwitch = azcreate(CATLSwitch, (nATLSwitchID, eDataScope), Audio::AudioSystemAllocator, "ATLSwitch");

                #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                    m_pDebugNameStore->AddAudioSwitch(nATLSwitchID, sATLSwitchName);
                #endif // INCLUDE_AUDIO_PRODUCTION_CODE

                    const size_t nATLSwitchStatesCount = static_cast<size_t>(pATLSwitchNode->getChildCount());

                    for (size_t j = 0; j < nATLSwitchStatesCount; ++j)
                    {
                        const XmlNodeRef pATLSwitchStateNode(pATLSwitchNode->getChild(j));

                        if (pATLSwitchStateNode && azstricmp(pATLSwitchStateNode->getTag(), SATLXmlTags::sATLSwitchStateTag) == 0)
                        {
                            const char* const sATLSwitchStateName = pATLSwitchStateNode->getAttr(SATLXmlTags::sATLNameAttribute);
                            const auto nATLSwitchStateID = AudioStringToID<TAudioSwitchStateID>(sATLSwitchStateName);

                            if (nATLSwitchStateID != INVALID_AUDIO_SWITCH_STATE_ID)
                            {
                                const size_t nSwitchStateImplCount = static_cast<size_t>(pATLSwitchStateNode->getChildCount());

                                CATLSwitchState::TImplPtrVec cSwitchStateImplVec;
                                cSwitchStateImplVec.reserve(nSwitchStateImplCount);

                                for (size_t k = 0; k < nSwitchStateImplCount; ++k)
                                {
                                    const XmlNodeRef pStateImplNode(pATLSwitchStateNode->getChild(k));
                                    if (pStateImplNode)
                                    {
                                        const char* const sStateImplNodeTag = pStateImplNode->getTag();
                                        const IATLSwitchStateImplData* pNewSwitchStateImplData = nullptr;
                                        EATLSubsystem eReceiver = eAS_NONE;

                                        if (azstricmp(sStateImplNodeTag, SATLXmlTags::sATLSwitchRequestTag) == 0)
                                        {
                                            pNewSwitchStateImplData = NewAudioSwitchStateImplDataInternal(pStateImplNode);
                                            eReceiver = eAS_ATL_INTERNAL;
                                        }
                                        else
                                        {
                                            AudioSystemImplementationRequestBus::BroadcastResult(pNewSwitchStateImplData, &AudioSystemImplementationRequestBus::Events::NewAudioSwitchStateImplData, pStateImplNode);
                                            eReceiver = eAS_AUDIO_SYSTEM_IMPLEMENTATION;
                                        }

                                        if (pNewSwitchStateImplData)
                                        {
                                            auto pSwitchStateImpl = azcreate(CATLSwitchStateImpl, (eReceiver, pNewSwitchStateImplData), Audio::AudioSystemAllocator, "ATLSwitchStateImpl");
                                            cSwitchStateImplVec.push_back(pSwitchStateImpl);
                                        }
                                    }
                                }

                                auto pNewState = azcreate(CATLSwitchState, (nATLSwitchID, nATLSwitchStateID, cSwitchStateImplVec), Audio::AudioSystemAllocator, "ATLSwitchState");
                                pNewSwitch->cStates[nATLSwitchStateID] = pNewState;

                            #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                                m_pDebugNameStore->AddAudioSwitchState(nATLSwitchID, nATLSwitchStateID, sATLSwitchStateName);
                            #endif // INCLUDE_AUDIO_PRODUCTION_CODE
                            }
                        }
                    }
                    m_rSwitches[nATLSwitchID] = pNewSwitch;
                }
                else
                {
                    g_audioLogger.Log(eALT_ERROR, "ParseAudioSwitches - Switch '%s' already exists!", sATLSwitchName);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::ParseAudioRtpcs(const XmlNodeRef pXMLRtpcRoot, const EATLDataScope eDataScope)
    {
        const size_t nAudioRtpcChildrenCount = static_cast<size_t>(pXMLRtpcRoot->getChildCount());

        for (size_t i = 0; i < nAudioRtpcChildrenCount; ++i)
        {
            const XmlNodeRef pAudioRtpcNode(pXMLRtpcRoot->getChild(i));

            if (pAudioRtpcNode && azstricmp(pAudioRtpcNode->getTag(), SATLXmlTags::sATLRtpcTag) == 0)
            {
                const char* const sATLRtpcName = pAudioRtpcNode->getAttr(SATLXmlTags::sATLNameAttribute);
                const auto nATLRtpcID = AudioStringToID<TAudioControlID>(sATLRtpcName);

                if ((nATLRtpcID != INVALID_AUDIO_CONTROL_ID) && (stl::find_in_map(m_rRtpcs, nATLRtpcID, nullptr) == nullptr))
                {
                    const size_t nRtpcNodeChildrenCount = static_cast<size_t>(pAudioRtpcNode->getChildCount());
                    CATLRtpc::TImplPtrVec cImplPtrs;
                    cImplPtrs.reserve(nRtpcNodeChildrenCount);

                    for (size_t j = 0; j < nRtpcNodeChildrenCount; ++j)
                    {
                        const XmlNodeRef pRtpcImplNode(pAudioRtpcNode->getChild(j));

                        if (pRtpcImplNode)
                        {
                            const char* const sRtpcImplNodeTag = pRtpcImplNode->getTag();
                            const IATLRtpcImplData* pNewRtpcImplData = nullptr;
                            EATLSubsystem eReceiver = eAS_NONE;

                            if (azstricmp(sRtpcImplNodeTag, SATLXmlTags::sATLRtpcRequestTag) == 0)
                            {
                                pNewRtpcImplData = NewAudioRtpcImplDataInternal(pRtpcImplNode);
                                eReceiver = eAS_ATL_INTERNAL;
                            }
                            else
                            {
                                AudioSystemImplementationRequestBus::BroadcastResult(pNewRtpcImplData, &AudioSystemImplementationRequestBus::Events::NewAudioRtpcImplData, pRtpcImplNode);
                                eReceiver = eAS_AUDIO_SYSTEM_IMPLEMENTATION;
                            }

                            if (pNewRtpcImplData)
                            {
                                auto pRtpcImpl = azcreate(CATLRtpcImpl, (eReceiver, pNewRtpcImplData), Audio::AudioSystemAllocator, "ATLRtpcImpl");
                                cImplPtrs.push_back(pRtpcImpl);
                            }
                        }
                    }

                    auto pNewRtpc = azcreate(CATLRtpc, (nATLRtpcID, eDataScope, cImplPtrs), Audio::AudioSystemAllocator, "ATLRtpc");

                    if (pNewRtpc)
                    {
                        m_rRtpcs[nATLRtpcID] = pNewRtpc;

                    #if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
                        m_pDebugNameStore->AddAudioRtpc(nATLRtpcID, sATLRtpcName);
                    #endif // INCLUDE_AUDIO_PRODUCTION_CODE
                    }
                }
                else
                {
                    g_audioLogger.Log(eALT_ERROR, "ParseAudioRtpcs - Rtpc '%s' already exists!", sATLRtpcName);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const IATLTriggerImplData* CATLXmlProcessor::NewAudioTriggerImplDataInternal(const XmlNodeRef pXMLTriggerRoot)
    {
        //TODO: implement
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const IATLRtpcImplData* CATLXmlProcessor::NewAudioRtpcImplDataInternal(const XmlNodeRef pXMLRtpcRoot)
    {
        //TODO: implement
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const IATLSwitchStateImplData* CATLXmlProcessor::NewAudioSwitchStateImplDataInternal(const XmlNodeRef pXMLSwitchRoot)
    {
        SATLSwitchStateImplData_internal* pSwitchStateImpl = nullptr;

        const char* const sInternalSwitchNodeName = pXMLSwitchRoot->getAttr(SATLXmlTags::sATLNameAttribute);

        if (sInternalSwitchNodeName && (sInternalSwitchNodeName[0] != '\0') && (pXMLSwitchRoot->getChildCount() == 1))
        {
            const XmlNodeRef pValueNode(pXMLSwitchRoot->getChild(0));

            if (pValueNode && azstricmp(pValueNode->getTag(), SATLXmlTags::sATLValueTag) == 0)
            {
                const char* sInternalSwitchStateName = pValueNode->getAttr(SATLXmlTags::sATLNameAttribute);

                if (sInternalSwitchStateName && (sInternalSwitchStateName[0] != '\0'))
                {
                    const auto nATLInternalSwitchID = AudioStringToID<TAudioControlID>(sInternalSwitchNodeName);
                    const auto nATLInternalStateID = AudioStringToID<TAudioSwitchStateID>(sInternalSwitchStateName);
                    pSwitchStateImpl = azcreate(SATLSwitchStateImplData_internal, (nATLInternalSwitchID, nATLInternalStateID), Audio::AudioSystemAllocator, "ATLSwitchDataImplData_internal");
                }
            }
        }
        else
        {
            g_audioLogger.Log(
                eALT_WARNING,
                "An ATLSwitchRequest %s inside ATLSwitchState needs to have exactly one ATLValue.",
                sInternalSwitchNodeName);
        }

        return pSwitchStateImpl;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const IATLEnvironmentImplData* CATLXmlProcessor::NewAudioEnvironmentImplDataInternal(const XmlNodeRef pXMLEnvironmentRoot)
    {
        // TODO: implement
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::DeleteAudioTrigger(const CATLTrigger* const pOldTrigger)
    {
        if (pOldTrigger)
        {
            for (auto const triggerImpl : pOldTrigger->m_cImplPtrs)
            {
                if (triggerImpl->GetReceiver() == eAS_ATL_INTERNAL)
                {
                    azdestroy(const_cast<IATLTriggerImplData*>(triggerImpl->m_pImplData), Audio::AudioSystemAllocator);
                }
                else
                {
                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioTriggerImplData, triggerImpl->m_pImplData);
                }

                azdestroy(const_cast<CATLTriggerImpl*>(triggerImpl), Audio::AudioSystemAllocator);
            }

            azdestroy(const_cast<CATLTrigger*>(pOldTrigger), Audio::AudioSystemAllocator);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::DeleteAudioRtpc(const CATLRtpc* const pOldRtpc)
    {
        if (pOldRtpc)
        {
            for (auto const rtpcImpl : pOldRtpc->m_cImplPtrs)
            {
                if (rtpcImpl->GetReceiver() == eAS_ATL_INTERNAL)
                {
                    azdestroy(const_cast<IATLRtpcImplData*>(rtpcImpl->m_pImplData), Audio::AudioSystemAllocator);
                }
                else
                {
                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioRtpcImplData, rtpcImpl->m_pImplData);
                }

                azdestroy(const_cast<CATLRtpcImpl*>(rtpcImpl), Audio::AudioSystemAllocator);
            }

            azdestroy(const_cast<CATLRtpc*>(pOldRtpc), Audio::AudioSystemAllocator);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::DeleteAudioSwitch(const CATLSwitch* const pOldSwitch)
    {
        if (pOldSwitch)
        {
            for (auto& statePair : pOldSwitch->cStates)
            {
                auto const switchState = statePair.second;
                if (switchState)
                {
                    for (auto const stateImpl : switchState->m_cImplPtrs)
                    {
                        if (stateImpl->GetReceiver() == eAS_ATL_INTERNAL)
                        {
                            azdestroy(const_cast<IATLSwitchStateImplData*>(stateImpl->m_pImplData), Audio::AudioSystemAllocator);
                        }
                        else
                        {
                            AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioSwitchStateImplData, stateImpl->m_pImplData);
                        }

                        azdestroy(const_cast<CATLSwitchStateImpl*>(stateImpl), Audio::AudioSystemAllocator);
                    }

                    azdestroy(const_cast<CATLSwitchState*>(switchState), Audio::AudioSystemAllocator);
                }
            }

            azdestroy(const_cast<CATLSwitch*>(pOldSwitch), Audio::AudioSystemAllocator);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::DeleteAudioPreloadRequest(const CATLPreloadRequest* const pOldPreloadRequest)
    {
        if (pOldPreloadRequest)
        {
            const EATLDataScope eScope = pOldPreloadRequest->GetDataScope();
            for (auto preloadFileId : pOldPreloadRequest->m_cFileEntryIDs)
            {
                m_rFileCacheMgr.TryRemoveFileCacheEntry(preloadFileId, eScope);
            }

            azdestroy(const_cast<CATLPreloadRequest*>(pOldPreloadRequest), Audio::AudioSystemAllocator);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::DeleteAudioEnvironment(const CATLAudioEnvironment* const pOldEnvironment)
    {
        if (pOldEnvironment)
        {
            for (auto const environmentImpl : pOldEnvironment->m_cImplPtrs)
            {
                if (environmentImpl->GetReceiver() == eAS_ATL_INTERNAL)
                {
                    azdestroy(const_cast<IATLEnvironmentImplData*>(environmentImpl->m_pImplData), Audio::AudioSystemAllocator);
                }
                else
                {
                    AudioSystemImplementationRequestBus::Broadcast(&AudioSystemImplementationRequestBus::Events::DeleteAudioEnvironmentImplData, environmentImpl->m_pImplData);
                }

                azdestroy(const_cast<CATLEnvironmentImpl*>(environmentImpl), Audio::AudioSystemAllocator);
            }

            azdestroy(const_cast<CATLAudioEnvironment*>(pOldEnvironment), Audio::AudioSystemAllocator);
        }
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //  SATLSharedData
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLSharedData::SATLSharedData()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    SATLSharedData::~SATLSharedData()
    {
    }



#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::DrawDebugInfo(IRenderAuxGeom& rAuxGeom, float fPosX, float fPosY) const
    {
        static float const fHeaderColor[4] = { 1.0f, 1.0f, 1.0f, 0.9f };
        static float const fItemPlayingColor[4] = { 0.3f, 0.6f, 0.3f, 0.9f };
        static float const fItemLoadingColor[4] = { 0.9f, 0.2f, 0.2f, 0.9f };
        static float const fItemOtherColor[4] = { 0.8f, 0.8f, 0.8f, 0.9f };
        static float const fNoImplColor[4] = { 1.0f, 0.6f, 0.6f, 0.9f };

        rAuxGeom.Draw2dLabel(fPosX, fPosY, 1.6f, fHeaderColor, false, "Audio Events [%" PRISIZE_T "]", m_cActiveAudioEvents.size());
        fPosX += 20.0f;
        fPosY += 17.0f;

        AZStd::string triggerFilter(g_audioCVars.m_pAudioTriggersDebugFilter->GetString());
        AZStd::to_lower(triggerFilter.begin(), triggerFilter.end());

        for (auto& audioEventPair : m_cActiveAudioEvents)
        {
            auto const atlEvent = audioEventPair.second;

            AZStd::string triggerName(m_pDebugNameStore->LookupAudioTriggerName(atlEvent->m_nTriggerID));
            AZStd::to_lower(triggerName.begin(), triggerName.end());

            if (AudioDebugDrawFilter(triggerName, triggerFilter))
            {
                const float* pColor = fItemOtherColor;

                if (atlEvent->IsPlaying())
                {
                    pColor = fItemPlayingColor;
                }
                else if (atlEvent->m_audioEventState == eAES_LOADING)
                {
                    pColor = fItemLoadingColor;
                }

                rAuxGeom.Draw2dLabel(fPosX, fPosY, 1.6f,
                    pColor,
                    false,
                    "%s (%llu): %s (%llu)",
                    m_pDebugNameStore->LookupAudioObjectName(atlEvent->m_nObjectID),
                    atlEvent->m_nObjectID,
                    triggerName.c_str(),
                    atlEvent->GetID());

                fPosY += 16.0f;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioEventManager::SetDebugNameStore(const CATLDebugNameStore* const pDebugNameStore)
    {
        m_pDebugNameStore = pDebugNameStore;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CAudioObjectManager::ReserveID(TAudioObjectID& rAudioObjectID, const char* const sAudioObjectName)
    {
        CATLAudioObject* const pNewObject  = GetInstance();

        bool bSuccess = false;
        rAudioObjectID = INVALID_AUDIO_OBJECT_ID;

        if (pNewObject)
        {
            EAudioRequestStatus eImplResult = eARS_FAILURE;
            AudioSystemImplementationRequestBus::BroadcastResult(eImplResult, &AudioSystemImplementationRequestBus::Events::RegisterAudioObject, pNewObject->GetImplDataPtr(), sAudioObjectName);

            if (eImplResult == eARS_SUCCESS)
            {
                pNewObject->IncrementRefCount();
                rAudioObjectID = pNewObject->GetID();
                m_cAudioObjects.emplace(rAudioObjectID, pNewObject);
                bSuccess = true;
            }
            else
            {
                ReleaseInstance(pNewObject);
                bSuccess = false;
            }
        }

        return bSuccess;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    size_t CAudioObjectManager::GetNumAudioObjects() const
    {
        return m_cAudioObjects.size();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    size_t CAudioObjectManager::GetNumActiveAudioObjects() const
    {
        size_t nNumActiveAudioObjects = 0;

        for (auto& audioObjectPair : m_cAudioObjects)
        {
            auto const audioObject = audioObjectPair.second;

            if (HasActiveEvents(audioObject))
            {
                ++nNumActiveAudioObjects;
            }
        }

        return nNumActiveAudioObjects;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::DrawPerObjectDebugInfo(IRenderAuxGeom& rAuxGeom, const Vec3& rListenerPos) const
    {
        AZStd::string audioObjectFilter(g_audioCVars.m_pAudioObjectsDebugFilter->GetString());
        AZStd::to_lower(audioObjectFilter.begin(), audioObjectFilter.end());

        for (auto& audioObjectPair : m_cAudioObjects)
        {
            auto const audioObject = audioObjectPair.second;

            AZStd::string audioObjectName(m_pDebugNameStore->LookupAudioObjectName(audioObject->GetID()));
            AZStd::to_lower(audioObjectName.begin(), audioObjectName.end());

            bool bDraw = AudioDebugDrawFilter(audioObjectName, audioObjectFilter);

            bDraw = bDraw && (g_audioCVars.m_nShowActiveAudioObjectsOnly == 0 || HasActiveEvents(audioObject));

            if (bDraw)
            {
                audioObject->DrawDebugInfo(rAuxGeom, rListenerPos, m_pDebugNameStore);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::DrawDebugInfo(IRenderAuxGeom& rAuxGeom, float fPosX, float fPosY) const
    {
        static const float fHeaderColor[4] = { 1.0f, 1.0f, 1.0f, 0.9f };
        static const float fItemActiveColor[4] = { 0.3f, 0.6f, 0.3f, 0.9f };
        static const float fItemInactiveColor[4] = { 0.8f, 0.8f, 0.8f, 0.9f };
        static const float fOverloadColor[4] = { 1.0f, 0.3f, 0.3f, 0.9f };

        size_t activeObjects = 0;
        size_t aliveObjects = m_cAudioObjects.size();
        size_t remainingObjects = (m_cObjectPool.m_nReserveSize > aliveObjects ? m_cObjectPool.m_nReserveSize - aliveObjects : 0);
        const float fHeaderPosY = fPosY;

        fPosX += 20.0f;
        fPosY += 17.0f;

        AZStd::string audioObjectFilter(g_audioCVars.m_pAudioObjectsDebugFilter->GetString());
        AZStd::to_lower(audioObjectFilter.begin(), audioObjectFilter.end());

        for (auto& audioObjectPair : m_cAudioObjects)
        {
            auto const audioObject = audioObjectPair.second;

            AZStd::string audioObjectName(m_pDebugNameStore->LookupAudioObjectName(audioObject->GetID()));
            AZStd::to_lower(audioObjectName.begin(), audioObjectName.end());

            bool bDraw = AudioDebugDrawFilter(audioObjectName, audioObjectFilter);
            bool hasActiveEvents = HasActiveEvents(audioObject);
            bDraw = bDraw && (g_audioCVars.m_nShowActiveAudioObjectsOnly == 0 || hasActiveEvents);

            if (bDraw)
            {
                rAuxGeom.Draw2dLabel(fPosX, fPosY, 1.6f,
                    hasActiveEvents ? fItemActiveColor : fItemInactiveColor,
                    false,
                    "[%.2f  %.2f  %.2f] (%llu): %s",
                    audioObject->GetPosition().mPosition.GetColumn3().x,
                    audioObject->GetPosition().mPosition.GetColumn3().y,
                    audioObject->GetPosition().mPosition.GetColumn3().z,
                    audioObject->GetID(),
                    audioObjectName.c_str());

                fPosY += 16.0f;
            }

            if (hasActiveEvents)
            {
                ++activeObjects;
            }
        }

        static const char* headerFormat = "Audio Objects [Active : %3" PRISIZE_T " | Alive: %3" PRISIZE_T " | Pool: %3" PRISIZE_T " | Remaining: %3" PRISIZE_T "]";
        const bool overloaded = (m_cAudioObjects.size() > m_cObjectPool.m_nReserveSize);

        rAuxGeom.Draw2dLabel(
            fPosX,
            fHeaderPosY,
            1.6f,
            overloaded ? fOverloadColor : fHeaderColor,
            false,
            headerFormat,
            activeObjects,
            aliveObjects,
            m_cObjectPool.m_nReserveSize,
            remainingObjects);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioObjectManager::SetDebugNameStore(CATLDebugNameStore* const pDebugNameStore)
    {
        m_pDebugNameStore = pDebugNameStore;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLXmlProcessor::SetDebugNameStore(CATLDebugNameStore* const pDebugNameStore)
    {
        m_pDebugNameStore = pDebugNameStore;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CAudioListenerManager::DrawDebugInfo(IRenderAuxGeom& rAuxGeom) const
    {
        static const ColorB audioListenerColor(55, 155, 225, 230);
        static const ColorB xAxisColor(255, 0, 0, 230);
        static const ColorB yAxisColor(0, 255, 0, 230);
        static const ColorB zAxisColor(0, 0, 255, 230);

        if (m_pDefaultListenerObject)
        {
            Vec3 vListenerPos = m_pDefaultListenerObject->oPosition.GetPositionVec();

            const SAuxGeomRenderFlags previousAuxGeomRenderFlags = rAuxGeom.GetRenderFlags();
            SAuxGeomRenderFlags newAuxGeomRenderFlags(e_Def3DPublicRenderflags | e_AlphaBlended);
            newAuxGeomRenderFlags.SetCullMode(e_CullModeNone);
            rAuxGeom.SetRenderFlags(newAuxGeomRenderFlags);

            // Draw Axes...
            rAuxGeom.DrawLine(vListenerPos, xAxisColor,
                vListenerPos + m_pDefaultListenerObject->oPosition.mPosition.GetColumn0(), xAxisColor);
            rAuxGeom.DrawLine(vListenerPos, yAxisColor,
                vListenerPos + m_pDefaultListenerObject->oPosition.mPosition.GetColumn1(), yAxisColor);
            rAuxGeom.DrawLine(vListenerPos, zAxisColor,
                vListenerPos + m_pDefaultListenerObject->oPosition.mPosition.GetColumn2(), zAxisColor);

            // Draw Sphere...
            const float radius = 0.15f; // 0.15 meters
            rAuxGeom.DrawSphere(vListenerPos, radius, audioListenerColor);

            rAuxGeom.SetRenderFlags(previousAuxGeomRenderFlags);
        }
    }

#endif // INCLUDE_AUDIO_PRODUCTION_CODE

} // namespace Audio
