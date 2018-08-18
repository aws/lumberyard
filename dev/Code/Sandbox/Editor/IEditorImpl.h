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

// Description : IEditor interface implementation.

#pragma once

#include "IEditor.h"
#include "MainWindow.h"

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <memory> // for shared_ptr
#include <QMap>
#include <QApplication>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <AzCore/std/string/string.h>

class QMenu;


#define GET_PLUGIN_ID_FROM_MENU_ID(ID) (((ID) & 0x000000FF))
#define GET_UI_ELEMENT_ID_FROM_MENU_ID(ID) ((((ID) & 0x0000FF00) >> 8))

class CObjectManager;
class CUndoManager;
class CGameEngine;
class CEquipPackLib;
class EditorScriptEnvironment;
class CExportManager;
class CErrorsDlg;
class CLensFlareManager;
class CIconManager;
class CBackgroundTaskManager;
class CTrackViewSequenceManager;
class CEditorFileMonitor;
class AzAssetWindow;
class AzAssetBrowserRequestHandler;
class AssetEditorRequestsHandler;
class CAlembicCompiler;

namespace Editor
{
    class EditorQtApplication;
}

namespace BackgroundScheduleManager
{
    class CScheduleManager;
}

namespace BackgroundTaskManager
{
    class CTaskManager;
}

namespace WinWidget
{
    class WinWidgetManager;
}

namespace AssetDatabase
{
    class AssetDatabaseLocationListener;
}

namespace LyMetrics
{
    class LyEditorShutdownStatusReporter;
}

class CEditorImpl 
    : public IEditor
    , protected AzToolsFramework::EditorEntityContextNotificationBus::Handler
{
public:
    CEditorImpl();
    ~CEditorImpl();

    void Initialize();
    void InitializeCrashLog();
    void OnBeginShutdownSequence();
    void OnEarlyExitShutdownSequence();
    void Uninitialize();


    void SetGameEngine(CGameEngine* ge);
    void DeleteThis() { delete this; };
    IEditorClassFactory* GetClassFactory();
    CEditorCommandManager* GetCommandManager() { return m_pCommandManager; };
    ICommandManager* GetICommandManager() { return m_pCommandManager; }
    void ExecuteCommand(const char* sCommand, ...);
    void ExecuteCommand(const QString& command);
    void SetDocument(CCryEditDoc* pDoc);
    CCryEditDoc* GetDocument() const;
    void SetModifiedFlag(bool modified = true);
    void SetModifiedModule(EModifiedModule  eModifiedModule, bool boSet = true);
    bool IsLevelExported() const;
    bool SetLevelExported(bool boExported = true);
    void InitFinished();
    bool IsModified();
    bool IsInitialized() const{ return m_bInitialized; }
    bool SaveDocument();
    ISystem*    GetSystem();
    IGame*      GetGame();
    I3DEngine*  Get3DEngine();
    IRenderer*  GetRenderer();
    void WriteToConsole(const char* string) { CLogFile::WriteLine(string); };
    void WriteToConsole(const QString& string) { CLogFile::WriteLine(string); };
    // Change the message in the status bar
    void SetStatusText(const QString& pszString);
    virtual IMainStatusBar* GetMainStatusBar() override;
    bool ShowConsole(bool show)
    {
        //if (AfxGetMainWnd())return ((CMainFrame *) (AfxGetMainWnd()))->ShowConsole(show);
        return false;
    }

    void SetConsoleVar(const char* var, float value);
    float GetConsoleVar(const char* var);
    //! Query main window of the editor
    QMainWindow* GetEditorMainWindow() const override
    {
        return MainWindow::instance();
    };

    QString GetMasterCDFolder();
    QString GetLevelName() override;
    QString GetLevelFolder();
    QString GetLevelDataFolder();
    QString GetSearchPath(EEditorPathName path);
    QString GetResolvedUserFolder();
    bool ExecuteConsoleApp(const QString& CommandLine, QString& OutputText, bool bNoTimeOut = false, bool bShowWindow = false);
    virtual bool IsInGameMode() override;
    virtual void SetInGameMode(bool inGame) override;
    virtual bool IsInSimulationMode() override;
    virtual bool IsInTestMode() override;
    virtual bool IsInPreviewMode() override;
    virtual bool IsInConsolewMode() override;
    virtual bool IsInLevelLoadTestMode() override;
    virtual bool IsInMatEditMode() override { return m_bMatEditMode; }

    //! Enables/Disable updates of editor.
    void EnableUpdate(bool enable) { m_bUpdates = enable; };
    //! Enable/Disable accelerator table, (Enabled by default).
    void EnableAcceleratos(bool bEnable);
    CGameEngine* GetGameEngine() { return m_pGameEngine; };
    CDisplaySettings*   GetDisplaySettings() { return m_pDisplaySettings; };
    const SGizmoParameters& GetGlobalGizmoParameters();
    CBaseObject* NewObject(const char* typeName, const char* fileName = "", const char* name = "", float x = 0.0f, float y = 0.0f, float z = 0.0f, bool modifyDoc = true);
    void DeleteObject(CBaseObject* obj);
    CBaseObject* CloneObject(CBaseObject* obj);
    void StartObjectCreation(const QString& type) { StartObjectCreation(type, QString()); }
    void StartObjectCreation(const QString& type, const QString& file);
    IObjectManager* GetObjectManager();
    // This will return a null pointer if CrySystem is not loaded before
    // Global Sandbox Settings are loaded from the registry before CrySystem
    // At that stage GetSettingsManager will return null and xml node in
    // memory will not be populated with Sandbox Settings.
    // After m_IEditor is created and CrySystem loaded, it is possible
    // to feed memory node with all necessary data needed for export
    // (gSettings.Load() and CXTPDockingPaneManager/CXTPDockingPaneLayout Sandbox layout management)
    CSettingsManager* GetSettingsManager();
    CSelectionGroup*    GetSelection();
    int ClearSelection();
    CBaseObject* GetSelectedObject();
    void SelectObject(CBaseObject* obj);
    void LockSelection(bool bLock);
    bool IsSelectionLocked();
    void PickObject(IPickObjectCallback* callback, const QMetaObject* targetClass = 0, const char* statusText = 0, bool bMultipick = false);

    void CancelPick();
    bool IsPicking();
    IDataBaseManager* GetDBItemManager(EDataBaseItemType itemType);
    CEntityPrototypeManager* GetEntityProtManager() { return m_pEntityManager; };
    CMaterialManager* GetMaterialManager() { return m_pMaterialManager; }
    IEditorParticleManager* GetParticleManager() override { return m_particleManager; }
    CMusicManager* GetMusicManager() { return m_pMusicManager; };
    CPrefabManager* GetPrefabManager() { return m_pPrefabManager; };
    CGameTokenManager* GetGameTokenManager() { return m_pGameTokenManager; };
    CLensFlareManager* GetLensFlareManager()    { return m_pLensFlareManager; };

    IBackgroundTaskManager* GetBackgroundTaskManager() override;
    IBackgroundScheduleManager* GetBackgroundScheduleManager() override;
    IEditorFileMonitor* GetFileMonitor() override;
    void RegisterEventLoopHook(IEventLoopHook* pHook) override;
    void UnregisterEventLoopHook(IEventLoopHook* pHook) override;
    IMissingAssetResolver* GetMissingAssetResolver() override { return m_pFileNameResolver; }
    IIconManager* GetIconManager();
    float GetTerrainElevation(float x, float y);
    CHeightmap* GetHeightmap();
    CVegetationMap* GetVegetationMap();
    Editor::EditorQtApplication* GetEditorQtApplication() { return m_QtApplication; }
    const QColor& GetColorByName(const QString& name) override;
    //////////////////////////////////////////////////////////////////////////
    // Special FG
    //////////////////////////////////////////////////////////////////////////
    CEditorFlowGraphModuleManager* GetFlowGraphModuleManager(){return m_pFlowGraphModuleManager; }
    CFlowGraphDebuggerEditor* GetFlowGraphDebuggerEditor(){return m_pFlowGraphDebuggerEditor; }
    CMaterialFXGraphMan*    GetMatFxGraphManager() { return m_pMatFxGraphManager; }
    CAIManager* GetAI();

    //////////////////////////////////////////////////////////////////////////
    CCustomActionsEditorManager* GetCustomActionManager();
    IMovieSystem* GetMovieSystem()
    {
        if (m_pSystem)
        {
            return m_pSystem->GetIMovieSystem();
        }
        return NULL;
    };

    CEquipPackLib* GetEquipPackLib() { return m_pEquipPackLib; }
    CPluginManager* GetPluginManager() { return m_pPluginManager; }
    CTerrainManager* GetTerrainManager() { return m_pTerrainManager; }
    CViewManager* GetViewManager();
    CViewport* GetActiveView();
    void SetActiveView(CViewport* viewport);

    CLevelIndependentFileMan* GetLevelIndependentFileMan() { return m_pLevelIndependentFileMan; }

    void UpdateViews(int flags, const AABB* updateRegion);
    void ResetViews();
    void ReloadTrackView();
    void UpdateSequencer(bool bOnlyKeys = false);
    Vec3 GetMarkerPosition() { return m_marker; };
    void SetMarkerPosition(const Vec3& pos) { m_marker = pos; };
    void    SetSelectedRegion(const AABB& box);
    void    GetSelectedRegion(AABB& box);
    CRuler* GetRuler() { return m_pRuler; }
    bool AddToolbarItem(uint8 iId, IUIEvent* pIHandler);
    void SetDataModified();
    int SelectRollUpBar(int rollupBarId);
    int AddRollUpPage(
        int rollbarId,
        const QString& pszCaption,
        QWidget* pwndTemplate,
        int iIndex = -1,
        bool bAutoExpand = true) override;
    void RemoveRollUpPage(int rollbarId, int iIndex);
    void RenameRollUpPage(int rollbarId, int iIndex, const char* nNewName);
    void ExpandRollUpPage(int rollbarId, int iIndex, bool bExpand = true);
    void EnableRollUpPage(int rollbarId, int iIndex, bool bEnable = true);
    int GetRollUpPageCount(int rollbarId);
    void SetOperationMode(EOperationMode mode);
    EOperationMode GetOperationMode();
    void SetEditMode(int editMode);
    int GetEditMode();

    //! A correct tool is one that corresponds to the previously set edit mode.
    bool HasCorrectEditTool() const;

    //! Returns the edit tool required for the edit mode specified.
    CEditTool* CreateCorrectEditTool();

    void SetEditTool(CEditTool* tool, bool bStopCurrentTool = true) override;
    void SetEditTool(const QString& sEditToolName, bool bStopCurrentTool = true) override;
    void ReinitializeEditTool() override;
    //! Returns current edit tool.
    CEditTool* GetEditTool() override;

    ITransformManipulator* ShowTransformManipulator(bool bShow);
    ITransformManipulator* GetTransformManipulator();
    void SetAxisConstraints(AxisConstrains axis);
    AxisConstrains GetAxisConstrains();
    void SetAxisVectorLock(bool bAxisVectorLock) { m_bAxisVectorLock = bAxisVectorLock; }
    bool IsAxisVectorLocked() { return m_bAxisVectorLock; }
    void SetTerrainAxisIgnoreObjects(bool bIgnore);
    bool IsTerrainAxisIgnoreObjects();
    void SetReferenceCoordSys(RefCoordSys refCoords);
    RefCoordSys GetReferenceCoordSys();
    XmlNodeRef FindTemplate(const QString& templateName);
    void AddTemplate(const QString& templateName, XmlNodeRef& tmpl);
    CBaseLibraryDialog* OpenDataBaseLibrary(EDataBaseItemType type, IDataBaseItem* pItem = NULL);
   
    const QtViewPane* OpenView(QString sViewClassName, bool reuseOpened = true) override;

    /**
     * Returns the top level widget which is showing the view pane with the specified name.
     * To access the child widget which actually implements the view pane use this instead:
     * QtViewPaneManager::FindViewPane<MyDialog>(name);
     */
    QWidget* FindView(QString viewClassName) override;

    bool CloseView(const char* sViewClassName);
    bool SetViewFocus(const char* sViewClassName);

    virtual QWidget* OpenWinWidget(WinWidgetId openId) override;
    virtual WinWidget::WinWidgetManager* GetWinWidgetManager() const override;

    // close ALL panels related to classId, used when unloading plugins.
    void CloseView(const GUID& classId);
    bool SelectColor(QColor &color, QWidget *parent = 0) override;
    void Update();
    SFileVersion GetFileVersion() { return m_fileVersion; };
    SFileVersion GetProductVersion() { return m_productVersion; };
    //! Get shader enumerator.
    CShaderEnum* GetShaderEnum();
    CUndoManager* GetUndoManager() { return m_pUndoManager; };
    void BeginUndo();
    void RestoreUndo(bool undo);
    void AcceptUndo(const QString& name);
    void CancelUndo();
    void SuperBeginUndo();
    void SuperAcceptUndo(const QString& name);
    void SuperCancelUndo();
    void SuspendUndo();
    void ResumeUndo();
    void Undo();
    void Redo();
    bool IsUndoRecording();
    bool IsUndoSuspended();
    void RecordUndo(IUndoObject* obj);
    bool FlushUndo(bool isShowMessage = false);
    bool ClearLastUndoSteps(int steps);
    //! Retrieve current animation context.
    CAnimationContext* GetAnimation();
    CTrackViewSequenceManager* GetSequenceManager() override;
    ITrackViewSequenceManager* GetSequenceManagerInterface() override;

    CToolBoxManager* GetToolBoxManager() { return m_pToolBoxManager; };
    IErrorReport* GetErrorReport() { return m_pErrorReport; }
    IErrorReport* GetLastLoadedLevelErrorReport() { return m_pLasLoadedLevelErrorReport; }
    void CommitLevelErrorReport() {SAFE_DELETE(m_pLasLoadedLevelErrorReport); m_pLasLoadedLevelErrorReport = new CErrorReport(*m_pErrorReport); }
    virtual IFileUtil* GetFileUtil() override { return m_pFileUtil;  }
    void Notify(EEditorNotifyEvent event);
    void NotifyExcept(EEditorNotifyEvent event, IEditorNotifyListener* listener);
    void RegisterNotifyListener(IEditorNotifyListener* listener);
    void UnregisterNotifyListener(IEditorNotifyListener* listener);
    //! Register document notifications listener.
    void RegisterDocListener(IDocListener* listener);
    //! Unregister document notifications listener.
    void UnregisterDocListener(IDocListener* listener);
    //! Retrieve interface to the source control.
    ISourceControl* GetSourceControl();
    //! Retrieve true if source control is provided and enabled in settings
    bool IsSourceControlAvailable() override;
    //! Only returns true if source control is both available AND currently connected and functioning
    bool IsSourceControlConnected() override;
    //! Retrieve interface to the source control.
    IAssetTagging* GetAssetTagging();
    //! Setup Material Editor mode
    void SetMatEditMode(bool bIsMatEditMode);
    CFlowGraphManager* GetFlowGraphManager() { return m_pFlowGraphManager; };
    CUIEnumsDatabase* GetUIEnumsDatabase() { return m_pUIEnumsDatabase; };
    void AddUIEnums();
    void GetMemoryUsage(ICrySizer* pSizer);
    void ReduceMemory();
    // Get Export manager
    IExportManager* GetExportManager();
    // Set current configuration spec of the editor.
    void SetEditorConfigSpec(ESystemConfigSpec spec, ESystemConfigPlatform platform);
    ESystemConfigSpec GetEditorConfigSpec() const;
    ESystemConfigPlatform GetEditorConfigPlatform() const;
    void ReloadTemplates();
    void AddErrorMessage(const QString& text, const QString& caption);
    IResourceSelectorHost* GetResourceSelectorHost() { return m_pResourceSelectorHost.get(); }
    virtual void ShowStatusText(bool bEnable);

    void OnObjectContextMenuOpened(QMenu* pMenu, const CBaseObject* pObject);
    virtual void RegisterObjectContextMenuExtension(TContextMenuExtensionFunc func) override;

    virtual void SetCurrentMissionTime(float time);
    virtual SSystemGlobalEnvironment* GetEnv() override;
    virtual IAssetBrowser* GetAssetBrowser() override; // Vladimir@Conffx
    virtual IBaseLibraryManager* GetMaterialManagerLibrary() override; // Vladimir@Conffx
    virtual IEditorMaterialManager* GetIEditorMaterialManager() override; // Vladimir@Conffx
    virtual IImageUtil* GetImageUtil() override;  // Vladimir@conffx
    virtual IEditorParticleUtils* GetParticleUtils() override; // Leroy@Conffx
    virtual SEditorSettings* GetEditorSettings() override;
    virtual ILogFile* GetLogFile() override { return m_pLogFile; }

    // Resource Manager
    virtual void SetAWSResourceManager(IAWSResourceManager* awsResourceManager) override { m_awsResourceManager = awsResourceManager; }
    virtual IAWSResourceManager* GetAWSResourceManager() override { return m_awsResourceManager; }

    // Console federation helper
    virtual void LaunchAWSConsole(QString destUrl) override;

    virtual bool ToProjectConfigurator(const QString& msg, const QString& caption, const QString& location) override;

    virtual QQmlEngine* GetQMLEngine() const;
    void UnloadPlugins(bool shuttingDown = false) override;
    void LoadPlugins() override;

    QMimeData* CreateQMimeData() const override;
    void DestroyQMimeData(QMimeData* data) const override;
    CBaseObject* BaseObjectFromEntityId(unsigned int id) override;

    bool IsLegacyUIEnabled() override;
    void SetLegacyUIEnabled(bool enabled) override;

protected:

    //////////////////////////////////////////////////////////////////////////
    // EditorEntityContextNotificationBus implementation
    void OnStartPlayInEditor() override;
    //////////////////////////////////////////////////////////////////////////
    AZStd::string LoadProjectIdFromProjectData();
    void InitMetrics();

    void DetectVersion();
    void RegisterTools();
    void SetMasterCDFolder();
    QRollupCtrl* GetRollUpControl(int rollupId)
    {
        return MainWindow::instance()->GetRollUpControl(rollupId);
    };
    
    void LogStartingShutdown();
    void LogEarlyShutdownExit();
    void ShutdownCrashLog();
    void LogBeginGameMode();
    void LogEndGameMode();

    void LoadSettings();
    void SaveSettings() const;

    //! List of all notify listeners.
    std::list<IEditorNotifyListener*> m_listeners;
    QMap<int, QWidget*> m_panelIds;

    EEditMode m_currEditMode;
    EOperationMode m_operationMode;
    ISystem* m_pSystem;
    IFileUtil* m_pFileUtil;
    CClassFactory* m_pClassFactory;
    CEditorCommandManager* m_pCommandManager;
    CObjectManager* m_pObjectManager;
    CPluginManager* m_pPluginManager;
    CViewManager*   m_pViewManager;
    CUndoManager* m_pUndoManager;
    Vec3 m_marker;
    AABB m_selectedRegion;
    AxisConstrains m_selectedAxis;
    RefCoordSys m_refCoordsSys;
    AxisConstrains m_lastAxis[16];
    RefCoordSys m_lastCoordSys[16];
    bool m_bAxisVectorLock;
    bool m_bUpdates;
    bool m_bTerrainAxisIgnoreObjects;
    SFileVersion m_fileVersion;
    SFileVersion m_productVersion;
    CXmlTemplateRegistry m_templateRegistry;
    CDisplaySettings* m_pDisplaySettings;
    CShaderEnum* m_pShaderEnum;
    _smart_ptr<CEditTool> m_pEditTool;
    CIconManager* m_pIconManager;
    std::unique_ptr<SGizmoParameters> m_pGizmoParameters;
    QString m_masterCDFolder;
    QString m_userFolder;
    bool m_bSelectionLocked;
    _smart_ptr<CEditTool> m_pPickTool;
    class CAxisGizmo* m_pAxisGizmo;
    CAIManager* m_pAIManager;
    CCustomActionsEditorManager* m_pCustomActionsManager;
    CEditorFlowGraphModuleManager* m_pFlowGraphModuleManager;
    CMaterialFXGraphMan* m_pMatFxGraphManager;
    CFlowGraphDebuggerEditor* m_pFlowGraphDebuggerEditor;
    CEquipPackLib* m_pEquipPackLib;
    CGameEngine* m_pGameEngine;
    CAnimationContext* m_pAnimationContext;
    CTrackViewSequenceManager* m_pSequenceManager;
    CToolBoxManager* m_pToolBoxManager;
    CEntityPrototypeManager* m_pEntityManager;
    CMaterialManager* m_pMaterialManager;
    CAlembicCompiler* m_pAlembicCompiler;
    IEditorParticleManager* m_particleManager;
    IEditorParticleUtils* m_particleEditorUtils;
    CMusicManager* m_pMusicManager;
    CPrefabManager* m_pPrefabManager;
    CGameTokenManager* m_pGameTokenManager;
    CLensFlareManager* m_pLensFlareManager;
    CErrorReport* m_pErrorReport;
    IMissingAssetResolver* m_pFileNameResolver;
    //! Contains the error reports for the last loaded level.
    CErrorReport* m_pLasLoadedLevelErrorReport;
    //! Global instance of error report class.
    CErrorsDlg* m_pErrorsDlg;
    //! Source control interface.
    ISourceControl* m_pSourceControl;
    //! AssetTagging provider interface
    IAssetTagging* m_pAssetTagging;
    CFlowGraphManager* m_pFlowGraphManager;

    CSelectionTreeManager* m_pSelectionTreeManager;

    CUIEnumsDatabase* m_pUIEnumsDatabase;
    //! Currently used ruler
    CRuler* m_pRuler;
    EditorScriptEnvironment* m_pScriptEnv;
    //! CConsole Synchronization
    CConsoleSynchronization* m_pConsoleSync;
    //! Editor Settings Manager
    CSettingsManager* m_pSettingsManager;

    CLevelIndependentFileMan* m_pLevelIndependentFileMan;

    //! Export manager for exporting objects and a terrain from the game to DCC tools
    CExportManager* m_pExportManager;
    CTerrainManager* m_pTerrainManager;
    CVegetationMap* m_pVegetationMap;
    std::unique_ptr<BackgroundTaskManager::CTaskManager> m_pBackgroundTaskManager;
    std::unique_ptr<BackgroundScheduleManager::CScheduleManager> m_pBackgroundScheduleManager;
    std::unique_ptr<CEditorFileMonitor> m_pEditorFileMonitor;
    std::unique_ptr<IResourceSelectorHost> m_pResourceSelectorHost;
    QString m_selectFileBuffer;
    QString m_levelNameBuffer;

    IAWSResourceManager* m_awsResourceManager;
    std::unique_ptr<WinWidget::WinWidgetManager> m_winWidgetManager;

    //! True if the editor is in material edit mode. Fast preview of materials.
    //! In this mode only very limited functionality is available.
    bool m_bMatEditMode;
    bool m_bShowStatusText;
    bool m_bInitialized;
    bool m_bExiting;
    static void CmdPy(IConsoleCmdArgs* pArgs);

    std::vector<TContextMenuExtensionFunc> m_objectContextMenuExtensions;

    Editor::EditorQtApplication* const m_QtApplication = nullptr;

    AssetDatabase::AssetDatabaseLocationListener* m_pAssetDatabaseLocationListener;
    AzAssetBrowserRequestHandler* m_pAssetBrowserRequestHandler;
    AssetEditorRequestsHandler* m_assetEditorRequestsHandler;
    AZStd::vector<AZStd::unique_ptr<AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler>> m_thumbnailRenderers;

    IAssetBrowser* m_pAssetBrowser; // Vladimir@Conffx
    IImageUtil* m_pImageUtil;  // Vladimir@conffx
    ILogFile* m_pLogFile;  // Vladimir@conffx

    CryMutex m_pluginMutex; // protect any pointers that come from plugins, such as the source control cached pointer.
    static const char* m_crashLogFileName;

    bool m_isLegacyUIEnabled;
};

