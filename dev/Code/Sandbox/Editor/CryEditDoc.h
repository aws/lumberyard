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

#ifndef CRYINCLUDE_EDITOR_CRYEDITDOC_H
#define CRYINCLUDE_EDITOR_CRYEDITDOC_H
#pragma once

#include "DocMultiArchive.h"
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

class CFrameWnd;
class CMission;
class CLevelShaderCache;
class CClouds;
struct LightingSettings;

// Filename of the temporary file used for the hold / fetch operation
#define HOLD_FETCH_FILE "_hold"

class SANDBOX_API CCryEditDoc
    : public QObject
    , protected AzToolsFramework::EditorEntityContextNotificationBus::Handler
{
    Q_OBJECT
    Q_PROPERTY(bool modified READ IsModified WRITE SetModifiedFlag);
    Q_PROPERTY(QString pathName READ GetPathName WRITE SetPathName);
    Q_PROPERTY(QString title READ GetTitle WRITE SetTitle);
 
public: // Create from serialization only
    Q_INVOKABLE CCryEditDoc();

    virtual ~CCryEditDoc();

    bool IsModified() const;
    void SetModifiedFlag(bool modified);

    QString GetPathName() const;
    void SetPathName(const QString& pathName);

    QString GetTitle() const;
    void SetTitle(const QString& title);

    static bool IsBackupOrTempLevelSubdirectory(const QString& folderName);

    bool DoSave(const QString& pathName, bool replace);

    // ClassWizard generated virtual function overrides
    virtual bool OnOpenDocument(const QString& lpszPathName);
    // Currently it's not possible to disable one single flag and eModifiedModule is ignored
    // if bModified is false.
    virtual void SetModifiedModules(EModifiedModule eModifiedModule, bool boSet = true);
    virtual void DeleteContents();
    virtual BOOL DoFileSave();
    virtual bool BackupBeforeSave(bool bForce = false);

    struct TOpenDocContext
    {
        CTimeValue loading_start_time;
        QString absoluteLevelPath;
    };
    bool BeforeOpenDocument(const QString& lpszPathName, TOpenDocContext& context);
    bool DoOpenDocument(const QString& lpszPathName, TOpenDocContext& context);
    virtual BOOL LoadXmlArchiveArray(TDocMultiArchive& arrXmlAr, const QString& absoluteLevelPath, const QString& levelPath);
    virtual void ReleaseXmlArchiveArray(TDocMultiArchive& arrXmlAr);


    virtual void Load(TDocMultiArchive& arrXmlAr, const QString& szFilename);
    virtual void StartStreamingLoad(){}
    virtual void SyncCurrentMissionContent(bool bRetrieve);
    virtual void RepositionVegetation();

    const char* GetTemporaryLevelName() const;
    void DeleteTemporaryLevel();
    void InitEmptyLevel(int resolution = 0, int unitSize = 0, bool bUseTerrain = false);
    void CreateDefaultLevelAssets(int resolution = 0, int unitSize = 0);
    bool IsLevelExported() const;
    void SetLevelExported(bool boExported = true);
    int GetModifiedModule();
    void ChangeMission();
    bool Save();
    void Save(CXmlArchive& xmlAr);
    void Load(CXmlArchive& xmlAr, const QString& szFilename);
    void Save(TDocMultiArchive& arrXmlAr);
    bool SaveLevel(const QString& filename);
    bool LoadLevel(TDocMultiArchive& arrXmlAr, const QString& absoluteCryFilePath);
    bool LoadEntities(const QString& levelPakFile);
    void Hold(const QString& holdName);
    void Fetch(const QString& holdName, bool bShowMessages = true, bool bDelHoldFolder = false);
    void SaveAutoBackup(bool bForce = false);
    void SerializeFogSettings(CXmlArchive& xmlAr);
    virtual void SerializeViewSettings(CXmlArchive& xmlAr);
    void SerializeMissions(TDocMultiArchive& arrXmlAr, QString& currentMission, bool bPartsInXml);
    void SerializeShaderCache(CXmlArchive& xmlAr);
    void SerializeNameSelection(CXmlArchive& xmlAr);
    LightingSettings* GetLighting();
    void SetWaterColor(const QColor& col) { m_waterColor = col; };
    QColor GetWaterColor() { return m_waterColor; };
    void ForceSkyUpdate();
    BOOL CanCloseFrame(CFrameWnd* pFrame);
    void GetMemoryUsage(ICrySizer* pSizer);
    XmlNodeRef& GetFogTemplate() { return m_fogTemplate; };
    XmlNodeRef& GetEnvironmentTemplate() { return m_environmentTemplate; };
    //! Return currently active Mission.
    CMission*   GetCurrentMission(bool bSkipLoadingAIWhenSyncingContent = false);
    //! Get number of missions on Map.
    int GetMissionCount() const { return m_missions.size(); };
    //! Get Mission by index.
    CMission*   GetMission(int index) const { return m_missions[index]; };
    //! Find Mission by name.
    CMission*   FindMission(const QString& name) const;
    CClouds* GetClouds() { return m_pClouds; }
    //! Makes specified mission current.
    void SetCurrentMission(CMission* mission);
    //! Add new mission to map.
    void AddMission(CMission* mission);
    //! Remove existing mission from map.
    void RemoveMission(CMission* mission);
    void RegisterListener(IDocListener* listener);
    void UnregisterListener(IDocListener* listener);
    void LogLoadTime(int time);
    bool IsDocumentReady() const { return m_bDocumentReady; }
    void SetDocumentReady(bool bReady);
    //! For saving binary data (voxel object)
    CXmlArchive* GetTmpXmlArch(){ return m_pTmpXmlArchHack; }
    CLevelShaderCache* GetShaderCache() { return m_pLevelShaderCache; }

    void OnEnvironmentPropertyChanged(IVariable* pVar);

    struct TSaveDocContext
    {
        bool bSaved;
    };
    bool BeforeSaveDocument(const QString& lpszPathName, TSaveDocContext& context);
    bool DoSaveDocument(const QString& lpszPathName, TSaveDocContext& context);
    bool AfterSaveDocument(const QString& lpszPathName, TSaveDocContext& context, bool bShowPrompt = true);

    bool SaveModified();

    virtual bool OnNewDocument();
    virtual bool OnSaveDocument(const QString& lpszPathName);
    virtual void OnFileSaveAs();
    void LoadTemplates();
    //! called immediately after saving the level.
    void AfterSave();
    void ClearMissions();
    void RegisterConsoleVariables();
    void OnStartLevelResourceList();
    static void OnValidateSurfaceTypesChanged(ICVar*);

    QString GetCryIndexPath(const LPCTSTR levelFilePath);

    const bool IsLevelLoadFailed() const { return m_bLoadFailed; }

    //////////////////////////////////////////////////////////////////////////
    // EditorEntityContextNotificationBus::Handler
    void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress, const AzFramework::SliceInstantiationTicket& /*ticket*/) override;
    void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId, const AzFramework::SliceInstantiationTicket& /*ticket*/) override;
    //////////////////////////////////////////////////////////////////////////

private:
    QString m_strMasterCDFolder;
    bool m_bLoadFailed;
    QColor m_waterColor;
    XmlNodeRef m_fogTemplate;
    XmlNodeRef m_environmentTemplate;
    CMission*   m_mission;
    CClouds* m_pClouds;
    std::vector<CMission*> m_missions;
    std::list<IDocListener*> m_listeners;
    bool m_bDocumentReady;
    CXmlArchive* m_pTmpXmlArchHack;
    CLevelShaderCache* m_pLevelShaderCache;
    ICVar* doc_validate_surface_types;
    int m_modifiedModuleFlags;
    bool m_boLevelExported;
    bool m_modified;
    QString m_pathName;
    QString m_title;
    AZ::Data::AssetId m_envProbeSliceAssetId;
    float m_terrainSize;
    const char* m_envProbeSliceFullPath;
    const float m_envProbeHeight;
};

class CAutoDocNotReady
{
public:
    CAutoDocNotReady()
    {
        m_prevState = GetIEditor()->GetDocument()->IsDocumentReady();
        GetIEditor()->GetDocument()->SetDocumentReady(false);
    }

    ~CAutoDocNotReady()
    {
        GetIEditor()->GetDocument()->SetDocumentReady(m_prevState);
    }

private:
    bool m_prevState;
};
#endif // CRYINCLUDE_EDITOR_CRYEDITDOC_H
