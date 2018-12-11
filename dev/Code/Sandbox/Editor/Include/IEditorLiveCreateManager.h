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
#ifndef CRYINCLUDE_EDITOR_INCLUDE_LIVECREATE_IEDITORLIVECREATEMANAGER_H
#define CRYINCLUDE_EDITOR_INCLUDE_LIVECREATE_IEDITORLIVECREATEMANAGER_H
#pragma once

#include <ILiveCreateCommon.h>
#include <ILiveCreatePlatform.h>
#include <ILiveCreateManager.h>
#include <ILiveCreateHost.h>
#include "ConfigGroup.h"

#ifndef NO_LIVECREATE

struct IBackgroundSchedule;

namespace LiveCreate
{
    class CBGTask_SearchForHosts;
    class CPowerStatusThread;
    struct ILiveCreateCommand;
    class CFileSyncManager;
    class CObjectSync;
    class CEditorHostInfo;

    enum ELiveCreateObjectType
    {
        eLiveCreateObjectType_Terrain,
        eLiveCreateObjectType_Brushes,
        eLiveCreateObjectType_Vegetation,
        eLiveCreateObjectType_Decals,
        eLiveCreateObjectType_Roads,

        eLiveCreateObjectType_MAX,
    };

    struct STerrainTextureInfo
    {
        uint8* pData;
        uint32 aDataSize;
        int32 posx;
        int32 posy;
        uint32 width;
        uint32 height;
        ETEX_Format eTFSrc;
    };

    // LiveCreate general settings
    class CGeneralSettings
        : public Config::CConfigGroup
    {
    public:
        CGeneralSettings();

        bool bResetBeforeStart;
        bool bExportLevelOnStart;
        bool bExportTerrainOnStart;
        bool bFastTargetReset;
        bool bShowSelectionBoxes;
        bool bShowSelectionNames;
        bool bSyncCameraFromGame;
        bool bAlwaysReloadLevel;
        bool bSyncFileUpdates;
    };

    // LiveCreate advanced settings
    class CAdvancedSettings
        : public Config::CConfigGroup
    {
    public:
        CAdvancedSettings();

        bool bEnableLog;
        bool bGodMode;
        bool bDisableAI;
        bool bDisableScripts;
        bool bSkipCinematics;
        bool bAutoConnect;
        bool bResumePhysicsOnDisconnect;
        bool bAutoWakeUpPhysicalObjects;
        bool bAlwaysSyncFullEntities;
        bool bUseCRCWithEntitySync;
        float fArchetypeSyncTime;
        float fParticleSyncTime;
        float fObjectsSyncTime;
        float fSlowResetWaitTime;
        float fFastResetWaitTime;
        int iReconnectionCount;
        float fReconnectionDelay;
    };

    /// Settings and profile manager
    struct IEditorManager
        : public IEditorNotifyListener
        , IConsoleVarSink
    {
        struct IListener
        {
            virtual ~IListener() {};
            virtual void OnHostAdded(CEditorHostInfo* pHost) {};
            virtual void OnHostRemoved(CEditorHostInfo* pHost) {};
            virtual void OnLiveCreateStarting(const float progress) {};
            virtual void OnLiveCreateStarted() {};
            virtual void OnLiveCreateStopped() {};
            virtual void OnLiveCreateError() {};
        };

        IEditorManager()
            : m_pManager(NULL)
            , m_bIsLoadingObjects(false)
            , m_bRequestedToStartLiveCreate(false)
        {}
        virtual ~IEditorManager(){}

        ILINE const CGeneralSettings& GetGeneralSettings() const
        {
            return *m_pGeneralSettings;
        }

        ILINE CGeneralSettings& GetGeneralSettings()
        {
            return *m_pGeneralSettings;
        }

        ILINE const CAdvancedSettings& GetAdvancedSettings() const
        {
            return *m_pAdvancedSettings;
        }

        ILINE CAdvancedSettings& GetAdvancedSettings()
        {
            return *m_pAdvancedSettings;
        }

        ILINE const uint GetNumPlatforms() const
        {
            return m_pManager->GetNumPlatforms();
        }

        ILINE IPlatformHandlerFactory* GetPlatform(const uint index) const
        {
            return m_pManager->GetPlatformFactory(index);
        }

        ILINE bool IsEnabled() const
        {
            return m_pManager->IsEnabled();
        }

        ILINE bool IsCameraSyncEnabled() const
        {
            return m_bIsCameraSyncEnabled;
        }

        ILINE bool CanSend() const
        {
            return !m_bIsLoadingObjects && m_pManager && m_pManager->CanSend();
        }

        virtual bool Initialize(LiveCreate::IManager* pManager) = 0;
        virtual void Shutdown() = 0;
        virtual void SetEnabled(bool bIsEnabled) = 0;
        virtual void SetCameraSync(bool bIsEnabled) = 0;

        // LiveCreate on/off
        virtual bool StartLiveCreate(CString& outReasonToFail, bool bQuick) = 0;
        virtual void StopLiveCreate() = 0;
        virtual bool IsStarting() const = 0;

        // General listener
        virtual void RegisterListener(IListener* pListener) = 0;
        virtual void UnregisterListener(IListener* pListener) = 0;

        // Host related functions, for thread safety all the Get*() functions increment the refcount for the objects
        virtual CEditorHostInfo* AddHostEntry(const char* szPlatformName, const char* szTargetName, const char* szKnownAddress) = 0;
        virtual bool RemoveEntry(CEditorHostInfo* pHostEntry) = 0;
        virtual void GetEnabledHosts(std::vector<CEditorHostInfo*>& outHosts) const = 0;
        virtual void GetHosts(std::vector<CEditorHostInfo*>& outHosts) const = 0;

        // Check the power on status of given platform handler
        virtual bool IsPlatformOn(IPlatformHandler* pHandler) const = 0;

        // Send live create command to connected hosts
        virtual bool SendCommand(const ILiveCreateCommand& command) = 0;

        // Log LiveCreate message (passes it to the low-level manager)
        virtual void LogMessagef(ELogMessageType aType, const char* szMessage, ...) = 0;

        // Profile related functions
        virtual void LoadSettings() = 0;
        virtual void SaveSettings() = 0;

        // Level loading phase
        virtual void BeginObjectsLoading() = 0;
        virtual void EndObjectsLoading() = 0;

        // Editor->Consoles synchronisation interface
        virtual bool SyncEnableLiveCreate() = 0;
        virtual bool SyncDisableLiveCreate() = 0;
        virtual bool SyncCameraTransform(const Matrix34& matrix) = 0;
        virtual bool SyncCameraFOV(const float fov) = 0;
        virtual bool SyncObjectTransform(const CBaseObject& object) = 0;
        virtual bool SyncObjectCreated(const CBaseObject& object) = 0;
        virtual bool SyncObjectDeleted(const CBaseObject& object) = 0;
        virtual bool SyncObjectFull(const CBaseObject& object) = 0;
        virtual bool SyncObjectProperty(const CBaseObject& object, IVariable* var) = 0;
        virtual bool SyncEntityProperty(const CEntityObject& entity, IVariable* var) = 0;
        virtual bool SyncCVarChange(const ICVar* pCVar) = 0;
        virtual bool SyncTimeOfDay(const float time) = 0;
        virtual bool SyncTimeOfDayVariable(const int varIndex, const char* displayName, ISplineInterpolator* pSpline) = 0;
        virtual bool SyncTimeOfDayFull() = 0;
        virtual bool SyncEnvironmentFull() = 0;
        virtual bool Sync3DEngine(const AABB& area, ELiveCreateObjectType type) = 0;
        virtual bool Sync3DObject(const CBaseObject& object) = 0;
        virtual bool SyncArchetypesFull() = 0;
        virtual bool SyncParticlesFull() = 0;
        virtual bool SyncTerrainTexture(const STerrainTextureInfo& texInfo) = 0;
        virtual bool SyncFile(const char* szPath) = 0;
        virtual bool SyncSelection() = 0;
        virtual bool SyncCameraFlag() = 0;
        virtual bool SyncObjectMaterial(const CBaseObject& entity, CMaterial* pNewMaterial) = 0;
        virtual bool SyncLayerVisibility(const CObjectLayer& layer, bool bIsVisible) = 0;

    protected:
        // The low-level manager (from CryLiveCreate)
        IManager* m_pManager;

        // LiveCreate settings profiles
        CGeneralSettings* m_pGeneralSettings;
        CAdvancedSettings* m_pAdvancedSettings;

        // Level flags
        bool m_bIsLoadingObjects;

        // Camera sync state is handled outside profile settings (it's to important)
        bool m_bIsCameraSyncEnabled;

        // LiveCreate startup schedule
        bool m_bRequestedToStartLiveCreate;
    };

    //-----------------------------------------------------------------------------
}

#endif // NO_LIVECREATE
#endif // CRYINCLUDE_EDITOR_INCLUDE_LIVECREATE_IEDITORLIVECREATEMANAGER_H
