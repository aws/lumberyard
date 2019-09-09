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

// Description : CEditorImpl class implementation.


#include "StdAfx.h"
#include "IEditorImpl.h"
#include "CryEdit.h"
#include "CryEditDoc.h"
#include "Dialogs/ErrorsDlg.h"
#include "Plugin.h"
#include "IResourceCompilerHelper.h"
#include "PluginManager.h"
#include "IconManager.h"
#include "ViewManager.h"
#include "ViewPane.h"
#include "Objects/GizmoManager.h"
#include "Objects/AxisGizmo.h"
#include "DisplaySettings.h"
#include "ShaderEnum.h"
#include "KeyboardCustomizationSettings.h"
#include "HyperGraph/FlowGraphManager.h"
#include "Export/ExportManager.h"
#include "HyperGraph/FlowGraphModuleManager.h"
#include "HyperGraph/FlowGraphDebuggerEditor.h"
#include "Material/MaterialFXGraphMan.h"
#include "EquipPackLib.h"
#include "CustomActions/CustomActionsEditorManager.h"
#include "AI/AIManager.h"
#include "Undo/Undo.h"
#include "Material/MaterialManager.h"
#include "Material/MaterialPickTool.h"
#include "EntityPrototypeManager.h"
#include "TrackView/TrackViewSequenceManager.h"
#include "AnimationContext.h"
#include "GameEngine.h"
#include "ToolBox.h"
#include "MainWindow.h"
#include "BaseLibraryDialog.h"
#include "Material/Material.h"
#include "Alembic/AlembicCompiler.h"
#include "EntityPrototype.h"
#include "Particles/ParticleManager.h"
#include "Prefabs/PrefabManager.h"
#include "GameTokens/GameTokenManager.h"
#include "LensFlareEditor/LensFlareManager.h"
#include "DataBaseDialog.h"
#include "UIEnumsDatabase.h"
#include "Util/Ruler.h"
#include "Script/ScriptEnvironment.h"
#include "RenderHelpers/AxisHelper.h"
#include "PickObjectTool.h"
#include "ObjectCreateTool.h"
#include "TerrainModifyTool.h"
#include "RotateTool.h"
#include "NullEditTool.h"
#include "VegetationMap.h"
#include "VegetationTool.h"
#include "TerrainTexturePainter.h"
#include "EditMode/ObjectMode.h"
#include "EditMode/VertexMode.h"
#include "Modelling/ModellingMode.h"
#include "Terrain/TerrainManager.h"
#include "Terrain/SurfaceType.h"
#include <I3DEngine.h>
#include <IConsole.h>
#include <IEntitySystem.h>
#include <IMovieSystem.h>
#include <ISourceControl.h>
#include <IAssetTagging.h>
#include "Util/BoostPythonHelpers.h"
#include "Objects/ObjectLayerManager.h"
#include "BackgroundTaskManager.h"
#include "BackgroundScheduleManager.h"
#include "EditorFileMonitor.h"
#include <IEditorGame.h>
#include "EditMode/VertexSnappingModeTool.h"
#include "AssetResolver/AssetResolver.h"
#include "Mission.h"
#include "MainStatusBar.h"
#include <LyMetricsProducer/LyMetricsAPI.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/platform/FileSystem.h>
#include <WinWidget/WinWidgetManager.h>
#include <AWSNativeSDKInit/AWSNativeSDKInit.h>

#include "EditorParticleUtils.h" // Leroy@Conffx
#include "IEditorParticleUtils.h" // Leroy@Conffx

#include <Serialization/Serializer.h>
#include "SettingsBlock.h"
#include "ResourceSelectorHost.h"
#include "Util/FileUtil_impl.h"
#include "Util/ImageUtil_impl.h" // Vladimir@Conffx
#include "AssetBrowser/AssetBrowserImpl.h"
#include "LogFileImpl.h"  // Vladimir@Conffx
#include "Controls/QRollupCtrl.h"
#include "Controls/RollupBar.h"

#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/Application/ToolsApplication.h>

#include <Editor/AssetDatabase/AssetDatabaseLocationListener.h>
#include <Editor/AzAssetBrowser/AzAssetBrowserRequestHandler.h>
#include <Editor/Thumbnails/TextureThumbnailRenderer.h>
#include <Editor/Thumbnails/StaticMeshThumbnailRenderer.h>
#include <Editor/Thumbnails/MaterialThumbnailRenderer.h>
#include <Editor/AssetEditor/AssetEditorRequestsHandler.h>

#include "aws/core/utils/crypto/Factories.h"
#include <AzCore/JSON/document.h>
#include <AzCore/Math/Uuid.h>

#include <QDir>

#include <CloudCanvas/ICloudCanvas.h>

// even in Release mode, the editor will return its heap, because there's no Profile build configuration for the editor
#ifdef _RELEASE
#undef _RELEASE
#endif
#include <CrtDebugStats.h>
#include "Settings.h"

#include "Core/QtEditorApplication.h"
#include "../Plugins/EditorCommon/EditorCommonAPI.h"
#include "QtViewPaneManager.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/IO/FileOperations.h>

static CCryEditDoc * theDocument;
#include <QMimeData>
#include <QColorDialog>
#include <QMessageBox>
#include <QProcess>

#if defined(EXTERNAL_CRASH_REPORTING)
#include <ToolsCrashHandler.h>
#endif
#ifndef VERIFY
#define VERIFY(EXPRESSION) { auto e = EXPRESSION; assert(e); }
#endif

#undef GetCommandLine

namespace
{
    bool SelectionContainsComponentEntities()
    {
        bool result = false;
        CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
        if (pSelection)
        {
            CBaseObject* selectedObj = nullptr;
            for (int selectionCounter = 0; selectionCounter < pSelection->GetCount(); ++selectionCounter)
            {
                selectedObj = pSelection->GetObject(selectionCounter);
                if (selectedObj->GetType() == OBJTYPE_AZENTITY)
                {
                    result = true;
                    break;
                }
            }
        }
        return result;
    }
}

const char* CEditorImpl::m_crashLogFileName = "SessionStatus/editor_statuses.json";

CEditorImpl::CEditorImpl()
    : m_currEditMode(eEditModeSelect)
    , m_operationMode(eOperationModeNone)
    , m_pSystem(nullptr)
    , m_pFileUtil(nullptr)
    , m_pClassFactory(nullptr)
    , m_pCommandManager(nullptr)
    , m_pObjectManager(nullptr)
    , m_pPluginManager(nullptr)
    , m_pViewManager(nullptr)
    , m_pUndoManager(nullptr)
    , m_marker(0, 0, 0)
    , m_selectedAxis(AXIS_TERRAIN)
    , m_refCoordsSys(COORDS_LOCAL)
    , m_bAxisVectorLock(false)
    , m_bUpdates(true)
    , m_bTerrainAxisIgnoreObjects(false)
    , m_pDisplaySettings(nullptr)
    , m_pShaderEnum(nullptr)
    , m_pIconManager(nullptr)
    , m_bSelectionLocked(true)
    , m_pPickTool(nullptr)
    , m_pAxisGizmo(nullptr)
    , m_pAIManager(nullptr)
    , m_pCustomActionsManager(nullptr)
    , m_pFlowGraphModuleManager(nullptr)
    , m_pMatFxGraphManager(nullptr)
    , m_pFlowGraphDebuggerEditor(nullptr)
    , m_pEquipPackLib(nullptr)
    , m_pGameEngine(nullptr)
    , m_pAnimationContext(nullptr)
    , m_pSequenceManager(nullptr)
    , m_pToolBoxManager(nullptr)
    , m_pEntityManager(nullptr)
    , m_pMaterialManager(nullptr)
    , m_particleManager(nullptr)
    , m_particleEditorUtils(nullptr)
    , m_pMusicManager(nullptr)
    , m_pPrefabManager(nullptr)
    , m_pGameTokenManager(nullptr)
    , m_pLensFlareManager(nullptr)
    , m_pErrorReport(nullptr)
    , m_pFileNameResolver(nullptr)
    , m_pLasLoadedLevelErrorReport(nullptr)
    , m_pErrorsDlg(nullptr)
    , m_pSourceControl(nullptr)
    , m_pAssetTagging(nullptr)
    , m_pFlowGraphManager(nullptr)
    , m_pSelectionTreeManager(nullptr)
    , m_pUIEnumsDatabase(nullptr)
    , m_pRuler(nullptr)
    , m_pScriptEnv(nullptr)
    , m_pConsoleSync(nullptr)
    , m_pSettingsManager(nullptr)
    , m_pLevelIndependentFileMan(nullptr)
    , m_pExportManager(nullptr)
    , m_pTerrainManager(nullptr)
    , m_pVegetationMap(nullptr)
    , m_awsResourceManager(nullptr)
    , m_bMatEditMode(false)
    , m_bShowStatusText(true)
    , m_bInitialized(false)
    , m_bExiting(false)
    , m_QtApplication(static_cast<Editor::EditorQtApplication*>(qApp))
    , m_pAssetBrowser(nullptr)
    , m_pImageUtil(nullptr)
    , m_pLogFile(nullptr)
    , m_isLegacyUIEnabled(false)
{
    // note that this is a call into EditorCore.dll, which stores the g_pEditorPointer for all shared modules that share EditorCore.dll
    // this means that they don't need to do SetIEditor(...) themselves and its available immediately
    SetIEditor(this);

    m_pFileUtil = new CFileUtil_impl(); // Vladimir@Conffx
    m_pAssetBrowser = new CAssetBrowserImpl(); // Vladimir@Conffx
    m_pLogFile = new CLogFileImpl(); // Vladimir@Conffx
    m_pLevelIndependentFileMan = new CLevelIndependentFileMan;
    SetMasterCDFolder();
    gSettings.Load();

    // retrieve this after the settings have been loaded
    // this will end up being overridden if the CryEntityRemoval gem is present
    // if the new viewport interaction model is enabled we must also disable the legacy ui
    // it is not compatible with any of the legacy system (e.g. CryDesigner)
    m_isLegacyUIEnabled = gSettings.enableLegacyUI && !gSettings.newViewportInteractionModel;
    m_isNewViewportInteractionModelEnabled = gSettings.newViewportInteractionModel;

    m_pErrorReport = new CErrorReport;
    m_pFileNameResolver = new CMissingAssetResolver;
    m_pClassFactory = CClassFactory::Instance();
    m_pCommandManager = new CEditorCommandManager;
    CRegistrationContext regCtx;
    regCtx.pCommandManager = m_pCommandManager;
    regCtx.pClassFactory = m_pClassFactory;
    m_pEditorFileMonitor.reset(new CEditorFileMonitor());
    m_pBackgroundTaskManager.reset(new BackgroundTaskManager::CTaskManager);
    m_pBackgroundScheduleManager.reset(new BackgroundScheduleManager::CScheduleManager);
    m_pUIEnumsDatabase = new CUIEnumsDatabase;
    m_pDisplaySettings = new CDisplaySettings;
    m_pShaderEnum = new CShaderEnum;
    m_pDisplaySettings->LoadRegistry();
    m_pPluginManager = new CPluginManager;
    m_pTerrainManager = new CTerrainManager();
    m_pVegetationMap = new CVegetationMap();
    m_pObjectManager = new CObjectManager;
    m_pViewManager = new CViewManager;
    m_pIconManager = new CIconManager;
    m_pUndoManager = new CUndoManager;
    m_pAIManager = new CAIManager;
    m_pCustomActionsManager = new CCustomActionsEditorManager;
    m_pEquipPackLib = new CEquipPackLib;
    m_pToolBoxManager = new CToolBoxManager;
    m_pMaterialManager = new CMaterialManager(regCtx);
    m_pAlembicCompiler = new CAlembicCompiler();
    m_pSequenceManager = new CTrackViewSequenceManager;
    m_pAnimationContext = new CAnimationContext;
    m_pEntityManager = new CEntityPrototypeManager;
    m_particleManager = new CEditorParticleManager;
    m_pPrefabManager = new CPrefabManager;
    m_pGameTokenManager = new CGameTokenManager;
    m_pFlowGraphManager = new CFlowGraphManager;

    m_pImageUtil = new CImageUtil_impl();
    m_particleEditorUtils = CreateEditorParticleUtils();
    m_pLensFlareManager = new CLensFlareManager;
    m_pFlowGraphModuleManager = new CEditorFlowGraphModuleManager;
    m_pFlowGraphDebuggerEditor  = new CFlowGraphDebuggerEditor;
    m_pMatFxGraphManager = new CMaterialFXGraphMan;
    m_pScriptEnv = new EditorScriptEnvironment();
    m_pResourceSelectorHost.reset(CreateResourceSelectorHost());
    m_pRuler = new CRuler;
    m_selectedRegion.min = Vec3(0, 0, 0);
    m_selectedRegion.max = Vec3(0, 0, 0);
    ZeroStruct(m_lastAxis);
    m_lastAxis[eEditModeSelect] = AXIS_TERRAIN;
    m_lastAxis[eEditModeSelectArea] = AXIS_TERRAIN;
    m_lastAxis[eEditModeMove] = AXIS_TERRAIN;
    m_lastAxis[eEditModeRotate] = AXIS_Z;
    m_lastAxis[eEditModeScale] = AXIS_XY;
    ZeroStruct(m_lastCoordSys);
    m_lastCoordSys[eEditModeSelect] = COORDS_LOCAL;
    m_lastCoordSys[eEditModeSelectArea] = COORDS_LOCAL;
    m_lastCoordSys[eEditModeMove] = COORDS_WORLD;
    m_lastCoordSys[eEditModeRotate] = COORDS_WORLD;
    m_lastCoordSys[eEditModeScale] = COORDS_WORLD;
    DetectVersion();
    RegisterTools();

    m_winWidgetManager.reset(new WinWidget::WinWidgetManager);

    m_pAssetDatabaseLocationListener = nullptr;
    m_pAssetBrowserRequestHandler = nullptr;
    m_assetEditorRequestsHandler = nullptr;

    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();

    AZ::IO::SystemFile::CreateDir("SessionStatus");
    QFile::setPermissions(m_crashLogFileName, QFileDevice::ReadOther | QFileDevice::WriteOther);
}

void CEditorImpl::Initialize()
{
#if defined(EXTERNAL_CRASH_REPORTING)
    CrashHandler::ToolsCrashHandler::InitCrashHandler("Editor", {});
#endif

    // Must be set before QApplication is initialized, so that we support HighDpi monitors, like the Retina displays
    // on Windows 10
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // Prevents (native) sibling widgets from causing problems with docked QOpenGLWidgets on Windows
    // The problem is due to native widgets ending up with pixel formats that are incompatible with the GL pixel format
    // (generally due to a lack of an alpha channel). This blocks the creation of a shared GL context.
    // And on macOS it prevents all kinds of bugs related to native widgets, specially regarding toolbars (duplicate toolbars, artifacts, crashes).
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    // Activate QT immediately so that its available as soon as CEditorImpl is (and thus GetIEditor())
    InitializeEditorCommon(GetIEditor());

    LoadSettings();
}

//The only purpose of that function is to be called at the very begining of the shutdown sequence so that we can instrument and track
//how many crashes occur while shutting down
void CEditorImpl::OnBeginShutdownSequence()
{
    LogStartingShutdown();
}

void CEditorImpl::OnEarlyExitShutdownSequence()
{
    LogEarlyShutdownExit();
}

void CEditorImpl::Uninitialize()
{
    SaveSettings();

    if (m_pSystem)
    {
        UninitializeEditorCommonISystem(m_pSystem);
    }
    UninitializeEditorCommon();
    ShutdownCrashLog();
}

void CEditorImpl::UnloadPlugins(bool shuttingDown/* = false*/)
{
    CryAutoLock<CryMutex> lock(m_pluginMutex);

    // Flush core buses. We're about to unload DLLs and need to ensure we don't have module-owned functions left behind.
    AZ::Data::AssetBus::ExecuteQueuedEvents();
    AZ::TickBus::ExecuteQueuedEvents();

    LyMetrics_Shutdown();

    // first, stop anyone from accessing plugins that provide things like source control.
    // note that m_psSourceControl is re-queried
    m_pSourceControl = nullptr;
    m_pAssetTagging = nullptr;

    // Send this message to ensure that any widgets queued for deletion will get deleted before their
    // plugin containing their vtable is unloaded. If not, access violations can occur
    QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);

    GetPluginManager()->ReleaseAllPlugins();

#ifdef DEPRECATED_QML_SUPPORT
    m_QtApplication->UninitializeQML(); // destroy QML first since it will hang onto memory inside the DLLs
#endif // #ifdef DEPRECATED_QML_SUPPORT

    GetPluginManager()->UnloadAllPlugins();

    if (!shuttingDown)
    {
#ifdef DEPRECATED_QML_SUPPORT
        // since we mean to continue, so need to bring QML back up again in case someone needs it.
        m_QtApplication->InitializeQML();
#endif // #ifdef DEPRECATED_QML_SUPPORT
    }
}

void CEditorImpl::LoadPlugins()
{
    CryAutoLock<CryMutex> lock(m_pluginMutex);

#ifdef DEPRECATED_QML_SUPPORT
    // plugins require QML, so make sure its present:

    m_QtApplication->InitializeQML();
#endif // #ifdef DEPRECATED_QML_SUPPORT

    static const char* editor_plugins_folder = "EditorPlugins";

    // Build, verify, and set the engine root's editor plugin folder
    QString editorPluginPathStr;
    const char* engineRoot = nullptr;
    EBUS_EVENT_RESULT(engineRoot, AzToolsFramework::ToolsApplicationRequestBus, GetEngineRootPath);
    if (engineRoot != nullptr)
    {
        AZStd::string_view binFolderName;
        AZ::ComponentApplicationBus::BroadcastResult(binFolderName, &AZ::ComponentApplicationRequests::GetBinFolder);

        QDir testDir;
        testDir.setPath(QString("%1/%2").arg(engineRoot, binFolderName.data()));
        if (testDir.exists() && testDir.cd(QString(editor_plugins_folder)))
        {
            editorPluginPathStr = testDir.absolutePath();
        }
    }
    // If no editor plugin path was found based on the root engine path, then fallback to the current editor.exe path
    if (editorPluginPathStr.isEmpty())
    {
        editorPluginPathStr = QString("%1/%2").arg(qApp->applicationDirPath(),QString(editor_plugins_folder));
    }

    QString pluginSearchPath = QDir::toNativeSeparators(QString("%1/*" AZ_DYNAMIC_LIBRARY_EXTENSION).arg(editorPluginPathStr));

    GetPluginManager()->LoadPlugins(pluginSearchPath.toUtf8().data());

    InitMetrics();
}

#ifdef DEPRECATED_QML_SUPPORT
QQmlEngine* CEditorImpl::GetQMLEngine() const
{
    if (!m_QtApplication)
    {
        CryFatalError("Attempt to get the QML engine when there isn't a Qt Application created.");
        return nullptr;
    }

    QQmlEngine* pEngine = m_QtApplication->GetQMLEngine();
    if (!pEngine)
    {
        CryFatalError("Attempt to get the QML engine when there isn't a QML engine in existence yet or it has already been destroyed.");
        return nullptr;
    }

    return pEngine;
}
#endif // #ifdef DEPRECATED_QML_SUPPORT

CEditorImpl::~CEditorImpl()
{
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();

    gSettings.Save();
    m_bExiting = true; // Can't save level after this point (while Crash)
    SAFE_DELETE(m_pScriptEnv);
    SAFE_RELEASE(m_pSourceControl);
    SAFE_DELETE(m_pGameTokenManager);

    SAFE_DELETE(m_pMatFxGraphManager);
    SAFE_DELETE(m_pFlowGraphModuleManager);

    if (m_pFlowGraphDebuggerEditor)
    {
        m_pFlowGraphDebuggerEditor->Shutdown();
        SAFE_DELETE(m_pFlowGraphDebuggerEditor);
    }

    SAFE_DELETE(m_particleManager)
    SAFE_DELETE(m_pEntityManager)
    SAFE_DELETE(m_pMaterialManager)
    SAFE_DELETE(m_pAlembicCompiler)
    SAFE_DELETE(m_pEquipPackLib)
    SAFE_DELETE(m_pIconManager)
    SAFE_DELETE(m_pViewManager)
    SAFE_DELETE(m_pObjectManager) // relies on prefab manager
    SAFE_DELETE(m_pPrefabManager); // relies on flowgraphmanager
    SAFE_DELETE(m_pFlowGraphManager);
    SAFE_DELETE(m_pVegetationMap);
    SAFE_DELETE(m_pTerrainManager);
    // AI should be destroyed after the object manager, as the objects may
    // refer to AI components.
    SAFE_DELETE(m_pAIManager)
    SAFE_DELETE(m_pCustomActionsManager)

    // some plugins may be exporter - this must be above plugin manager delete.
    SAFE_DELETE(m_pExportManager);

    SAFE_DELETE(m_pPluginManager)
    SAFE_DELETE(m_pAnimationContext) // relies on undo manager
    SAFE_DELETE(m_pUndoManager)

    if (m_pDisplaySettings)
    {
        m_pDisplaySettings->SaveRegistry();
    }

    SAFE_DELETE(m_pDisplaySettings)
    SAFE_DELETE(m_pRuler)
    SAFE_DELETE(m_pShaderEnum)
    SAFE_DELETE(m_pToolBoxManager)
    SAFE_DELETE(m_pCommandManager)
    SAFE_DELETE(m_pClassFactory)
    SAFE_DELETE(m_pLasLoadedLevelErrorReport)
    SAFE_DELETE(m_pUIEnumsDatabase)

    SAFE_DELETE(m_pSettingsManager);

    SAFE_DELETE(m_pAssetDatabaseLocationListener);
    SAFE_DELETE(m_pAssetBrowserRequestHandler);
    SAFE_DELETE(m_assetEditorRequestsHandler);

    // Game engine should be among the last things to be destroyed, as it
    // destroys the engine.
    SAFE_DELETE(m_pErrorsDlg);
    SAFE_DELETE(m_pLevelIndependentFileMan);
    SAFE_DELETE(m_pFileNameResolver);
    SAFE_DELETE(m_pGameEngine);
    // The error report must be destroyed after the game, as the engine
    // refers to the error report and the game destroys the engine.
    SAFE_DELETE(m_pErrorReport);

    SAFE_DELETE(m_pAssetBrowser); // Vladimir@Conffx
    SAFE_DELETE(m_pFileUtil); // Vladimir@Conffx
    SAFE_DELETE(m_pImageUtil); // Vladimir@Conffx
    SAFE_DELETE(m_particleEditorUtils); // Leroy@Conffx
    SAFE_DELETE(m_pLogFile); // Vladimir@Conffx
}

void CEditorImpl::SetMasterCDFolder()
{
    QString szFolder = qApp->applicationDirPath();

    // Remove Bin32/Bin64 folder/
    AZStd::string_view binFolderName;
    AZ::ComponentApplicationBus::BroadcastResult(binFolderName, &AZ::ComponentApplicationRequests::GetBinFolder);

    szFolder.remove(QRegularExpression(R"((\\|/)Bin32.*)"));

    szFolder.remove(QRegularExpression(QStringLiteral("(\\\\|/)%1.*").arg(QRegularExpression::escape(binFolderName.data()))));


    m_masterCDFolder = QDir::toNativeSeparators(szFolder);

    if (!m_masterCDFolder.isEmpty())
    {
        if (m_masterCDFolder[m_masterCDFolder.length() - 1] != '\\')
        {
            m_masterCDFolder += '\\';
        }
    }

    QDir::setCurrent(szFolder);
}

void CEditorImpl::SetGameEngine(CGameEngine* ge)
{
    m_pAssetDatabaseLocationListener = new AssetDatabase::AssetDatabaseLocationListener();
    m_pAssetBrowserRequestHandler = new AzAssetBrowserRequestHandler();
    m_assetEditorRequestsHandler = aznew AssetEditorRequestsHandler();

    m_pSystem = ge->GetSystem();
    m_pGameEngine = ge;

    InitializeEditorCommonISystem(m_pSystem);

    m_templateRegistry.LoadTemplates("Editor");
    m_pObjectManager->LoadClassTemplates("Editor");
    m_pObjectManager->RegisterCVars();

    m_pMaterialManager->Set3DEngine();
    m_pAnimationContext->Init();

    m_thumbnailRenderers.push_back(AZStd::make_unique<TextureThumbnailRenderer>());
    m_thumbnailRenderers.push_back(AZStd::make_unique<StaticMeshThumbnailRenderer>());
    m_thumbnailRenderers.push_back(AZStd::make_unique<MaterialThumbnailRenderer>());
}

void CEditorImpl::RegisterTools()
{
    CRegistrationContext rc;

    rc.pCommandManager = m_pCommandManager;
    rc.pClassFactory = m_pClassFactory;
    CTerrainModifyTool::RegisterTool(rc);
    CVegetationTool::RegisterTool(rc);
    CTerrainTexturePainter::RegisterTool(rc);
    CObjectMode::RegisterTool(rc);
    CSubObjectModeTool::RegisterTool(rc);
    CMaterialPickTool::RegisterTool(rc);
    CModellingModeTool::RegisterTool(rc);
    CVertexSnappingModeTool::RegisterTool(rc);
    CRotateTool::RegisterTool(rc);
    NullEditTool::RegisterTool(rc);
}

void CEditorImpl::ExecuteCommand(const char* sCommand, ...)
{
    va_list args;
    va_start(args, sCommand);
    ExecuteCommand(QString::asprintf(sCommand, args));
    va_end(args);
}

void CEditorImpl::ExecuteCommand(const QString& command)
{
    m_pCommandManager->Execute(command.toUtf8().data());
}

void CEditorImpl::Update()
{
    if (!m_bUpdates)
    {
        return;
    }

    // Make sure this is not called recursively
    m_bUpdates = false;

    FUNCTION_PROFILER(GetSystem(), PROFILE_EDITOR);
    m_pRuler->Update();

    m_pFileNameResolver->PumpEvents();

    //@FIXME: Restore this latter.
    //if (GetGameEngine() && GetGameEngine()->IsLevelLoaded())
    {
        m_pObjectManager->Update();
    }
    if (IsInPreviewMode())
    {
        SetModifiedFlag(FALSE);
        SetModifiedModule(eModifiedNothing);
    }

    if (m_pGameEngine != NULL)
    {
        IEditorGame* pEditorGame = m_pGameEngine->GetIEditorGame();
        if (pEditorGame != NULL)
        {
            IEditorGame::HelpersDrawMode::EType helpersDrawMode = IEditorGame::HelpersDrawMode::Hide;
            if (m_pDisplaySettings->IsDisplayHelpers())
            {
                helpersDrawMode = IEditorGame::HelpersDrawMode::Show;
            }
            pEditorGame->UpdateHelpers(helpersDrawMode);
        }
    }

    m_bUpdates = true;
}

ISystem* CEditorImpl::GetSystem()
{
    return m_pSystem;
}

I3DEngine* CEditorImpl::Get3DEngine()
{
    if (gEnv)
    {
        return gEnv->p3DEngine;
    }
    return nullptr;
}

IRenderer*  CEditorImpl::GetRenderer()
{
    if (gEnv)
    {
        return gEnv->pRenderer;
    }
    return nullptr;
}

IGame*  CEditorImpl::GetGame()
{
    if (gEnv)
    {
        return gEnv->pGame;
    }
    return nullptr;
}

IEditorClassFactory* CEditorImpl::GetClassFactory()
{
    return m_pClassFactory;
}

CCryEditDoc* CEditorImpl::GetDocument() const
{
    return theDocument;
}

void CEditorImpl::SetDocument(CCryEditDoc* pDoc)
{
    theDocument = pDoc;
}

void CEditorImpl::SetModifiedFlag(bool modified)
{
    if (GetDocument() && GetDocument()->IsDocumentReady())
    {
        GetDocument()->SetModifiedFlag(modified);

        if (modified)
        {
            GetDocument()->SetLevelExported(false);
        }
    }
}

void CEditorImpl::SetModifiedModule(EModifiedModule eModifiedModule, bool boSet)
{
    if (GetDocument())
    {
        GetDocument()->SetModifiedModules(eModifiedModule, boSet);
    }
}

bool CEditorImpl::IsLevelExported() const
{
    CCryEditDoc* pDoc = GetDocument();

    if (pDoc)
    {
        return pDoc->IsLevelExported();
    }

    return false;
}

bool CEditorImpl::SetLevelExported(bool boExported)
{
    if (GetDocument())
    {
        GetDocument()->SetLevelExported(boExported);
        return true;
    }
    return false;
}

bool CEditorImpl::IsModified()
{
    if (GetDocument())
    {
        return GetDocument()->IsModified();
    }
    return false;
}

bool CEditorImpl::SaveDocument()
{
    if (m_bExiting)
    {
        return false;
    }

    if (GetDocument())
    {
        return GetDocument()->Save();
    }
    else
    {
        return false;
    }
}

QString CEditorImpl::GetMasterCDFolder()
{
    return m_masterCDFolder;
}

QString CEditorImpl::GetLevelFolder()
{
    return GetGameEngine()->GetLevelPath();
}

QString CEditorImpl::GetLevelName()
{
    m_levelNameBuffer = GetGameEngine()->GetLevelName();
    return m_levelNameBuffer;
}

QString CEditorImpl::GetLevelDataFolder()
{
    return Path::AddPathSlash(Path::AddPathSlash(GetGameEngine()->GetLevelPath()) + "LevelData");
}

QString CEditorImpl::GetSearchPath(EEditorPathName path)
{
    return gSettings.searchPaths[path][0];
}

QString CEditorImpl::GetResolvedUserFolder()
{
    m_userFolder = Path::GetResolvedUserSandboxFolder();
    return m_userFolder;
}

void CEditorImpl::SetDataModified()
{
    GetDocument()->SetModifiedFlag(TRUE);
}

void CEditorImpl::SetStatusText(const QString& pszString)
{
    if (m_bShowStatusText && !m_bMatEditMode && GetMainStatusBar())
    {
        GetMainStatusBar()->SetStatusText(pszString);
    }
}

IMainStatusBar* CEditorImpl::GetMainStatusBar()
{
    return MainWindow::instance()->StatusBar();
}

int CEditorImpl::SelectRollUpBar(int rollupBarId)
{
    return MainWindow::instance()->SelectRollUpBar(rollupBarId);
}

int CEditorImpl::AddRollUpPage(
    int rollbarId,
    const QString& pszCaption,
    QWidget* pwndTemplate,
    int iIndex /*= -1*/,
    bool bAutoExpand /*= true*/)
{
    if (!GetRollUpControl(rollbarId))
    {
        return 0;
    }

    // Preserve Focused window.
    QWidget* hFocusWnd = qApp->focusWidget();
    int ndx = GetRollUpControl(rollbarId)->insertItem(iIndex, pwndTemplate, pszCaption);
    if (!bAutoExpand)
    {
        GetRollUpControl(rollbarId)->setIndexVisible(ndx, false);
    }

    int id = 1;
    assert(m_panelIds.key(pwndTemplate, -1) == -1);
    if (!m_panelIds.isEmpty())
    {
        id = m_panelIds.lastKey() + 1;
    }
    m_panelIds.insert(id, pwndTemplate);

    // Make sure focus stay in main wnd.
    if (hFocusWnd && qApp->focusWidget() != hFocusWnd)
    {
        hFocusWnd->setFocus();
    }
    return id;
}

void CEditorImpl::RemoveRollUpPage(int rollbarId, int iIndex)
{
    if (GetRollUpControl(rollbarId))
    {
        QWidget* w = m_panelIds.value(iIndex);
        m_panelIds.remove(iIndex);
        GetRollUpControl(rollbarId)->removeItem(w);
        w->deleteLater();
    }
}

void CEditorImpl::RenameRollUpPage(int rollbarId, int iIndex, const char* pNewName)
{
    if (GetRollUpControl(rollbarId))
    {
        GetRollUpControl(rollbarId)->setItemText(GetRollUpControl(rollbarId)->indexOf(m_panelIds.value(iIndex)), pNewName);
    }
}

void CEditorImpl::ExpandRollUpPage(int rollbarId, int iIndex, bool bExpand)
{
    // Preserve Focused window.
    QWidget* hFocusWnd = qApp->focusWidget();

    if (GetRollUpControl(rollbarId))
    {
        GetRollUpControl(rollbarId)->setWidgetVisible(m_panelIds.value(iIndex), bExpand);
    }

    // Preserve Focused window.
    if (hFocusWnd && qApp->focusWidget() != hFocusWnd)
    {
        hFocusWnd->setFocus();
    }
}

void CEditorImpl::EnableRollUpPage(int rollbarId, int iIndex, bool bEnable)
{
    // Preserve Focused window.
    QWidget* hFocusWnd = qApp->focusWidget();

    if (GetRollUpControl(rollbarId))
    {
        GetRollUpControl(rollbarId)->setItemEnabled(GetRollUpControl(rollbarId)->indexOf(m_panelIds.value(iIndex)), bEnable);
    }

    // Preserve Focused window.
    if (hFocusWnd && qApp->focusWidget() != hFocusWnd)
    {
        hFocusWnd->setFocus();
    }
}

int CEditorImpl::GetRollUpPageCount(int rollbarId)
{
    if (GetRollUpControl(rollbarId))
    {
        return GetRollUpControl(rollbarId)->count();
    }
    return 0;
}

int CEditorImpl::GetEditMode()
{
    return m_currEditMode;
}

void CEditorImpl::SetEditMode(int editMode)
{
    bool isEditorInGameMode = false;
    EBUS_EVENT_RESULT(isEditorInGameMode, AzToolsFramework::EditorEntityContextRequestBus, IsEditorRunningGame);

    if (isEditorInGameMode)
    {
        if (editMode != eEditModeSelect)
        {
            if (SelectionContainsComponentEntities())
            {
                return;
            }
        }
    }

    if ((EEditMode)editMode == eEditModeRotate)
    {
        if (GetEditTool() && GetEditTool()->IsCircleTypeRotateGizmo())
        {
            editMode = eEditModeRotateCircle;
        }
    }

    EEditMode newEditMode = (EEditMode)editMode;
    if (m_currEditMode == newEditMode)
    {
        return;
    }

    m_currEditMode = newEditMode;
    AABB box(Vec3(0, 0, 0), Vec3(0, 0, 0));
    SetSelectedRegion(box);

    if (GetEditTool() && !GetEditTool()->IsNeedMoveTool())
    {
        SetEditTool(0, true);
    }

    Notify(eNotify_OnEditModeChange);
}

void CEditorImpl::SetOperationMode(EOperationMode mode)
{
    m_operationMode = mode;
    gSettings.operationMode = mode;
}

EOperationMode CEditorImpl::GetOperationMode()
{
    return m_operationMode;
}

bool CEditorImpl::HasCorrectEditTool() const
{
    if (!m_pEditTool)
    {
        return false;
    }

    switch (m_currEditMode)
    {
    case eEditModeRotate:
        return qobject_cast<CRotateTool*>(m_pEditTool) != nullptr;
    default:
        return qobject_cast<CObjectMode*>(m_pEditTool) != nullptr && qobject_cast<CRotateTool*>(m_pEditTool) == nullptr;
    }
}

CEditTool* CEditorImpl::CreateCorrectEditTool()
{
    if (m_currEditMode == eEditModeRotate)
    {
        CBaseObject* selectedObj = nullptr;
        CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
        if (pSelection && pSelection->GetCount() > 0)
        {
            selectedObj = pSelection->GetObject(0);
        }

        return (new CRotateTool(selectedObj));
    }

    return (new CObjectMode);
}

void CEditorImpl::SetEditTool(CEditTool* tool, bool bStopCurrentTool)
{
    CViewport* pViewport = GetIEditor()->GetActiveView();
    if (pViewport)
    {
        pViewport->SetCurrentCursor(STD_CURSOR_DEFAULT);
    }

    if (!tool)
    {
        if (HasCorrectEditTool())
        {
            return;
        }
        else
        {
            tool = CreateCorrectEditTool();
        }
    }

    if (!tool->Activate(m_pEditTool))
    {
        return;
    }

    if (bStopCurrentTool)
    {
        if (m_pEditTool && m_pEditTool != tool)
        {
            m_pEditTool->EndEditParams();
            SetStatusText("Ready");
        }
    }

    m_pEditTool = tool;
    if (m_pEditTool)
    {
        m_pEditTool->BeginEditParams(this, 0);
    }

    // Make sure pick is aborted.
    if (tool != m_pPickTool)
    {
        m_pPickTool = nullptr;
    }
    Notify(eNotify_OnEditToolChange);
}

void CEditorImpl::ReinitializeEditTool()
{
    if (m_pEditTool)
    {
        m_pEditTool->EndEditParams();
        m_pEditTool->BeginEditParams(this, 0);
    }
}

void CEditorImpl::SetEditTool(const QString& sEditToolName, bool bStopCurrentTool)
{
    CEditTool* pTool = GetEditTool();
    if (pTool && pTool->GetClassDesc())
    {
        // Check if already selected.
        if (QString::compare(pTool->GetClassDesc()->ClassName(), sEditToolName, Qt::CaseInsensitive) == 0)
        {
            return;
        }
    }

    IClassDesc* pClass = GetIEditor()->GetClassFactory()->FindClass(sEditToolName.toUtf8().data());
    if (!pClass)
    {
        Warning("Editor Tool %s not registered.", sEditToolName.toUtf8().data());
        return;
    }
    if (pClass->SystemClassID() != ESYSTEM_CLASS_EDITTOOL)
    {
        Warning("Class name %s is not a valid Edit Tool class.", sEditToolName.toUtf8().data());
        return;
    }

    QScopedPointer<QObject> o(pClass->CreateQObject());
    if (CEditTool* pEditTool = qobject_cast<CEditTool*>(o.data()))
    {
        GetIEditor()->SetEditTool(pEditTool);
        o.take();
        return;
    }
    else
    {
        Warning("Class name %s is not a valid Edit Tool class.", sEditToolName.toUtf8().data());
        return;
    }
}

CEditTool* CEditorImpl::GetEditTool()
{
    if (m_isNewViewportInteractionModelEnabled)
    {
        return nullptr;
    }
    
    return m_pEditTool;
}

ITransformManipulator* CEditorImpl::ShowTransformManipulator(bool bShow)
{
    if (bShow)
    {
        if (!m_pAxisGizmo)
        {
            m_pAxisGizmo = new CAxisGizmo;
            m_pAxisGizmo->AddRef();
            GetObjectManager()->GetGizmoManager()->AddGizmo(m_pAxisGizmo);
        }
        return m_pAxisGizmo;
    }
    else
    {
        // Hide gizmo.
        if (m_pAxisGizmo)
        {
            GetObjectManager()->GetGizmoManager()->RemoveGizmo(m_pAxisGizmo);
            m_pAxisGizmo->Release();
        }
        m_pAxisGizmo = 0;
    }
    return 0;
}

ITransformManipulator* CEditorImpl::GetTransformManipulator()
{
    return m_pAxisGizmo;
}

void CEditorImpl::SetAxisConstraints(AxisConstrains axisFlags)
{
    m_selectedAxis = axisFlags;
    m_lastAxis[m_currEditMode] = m_selectedAxis;
    m_pViewManager->SetAxisConstrain(axisFlags);
    SetTerrainAxisIgnoreObjects(false);

    // Update all views.
    UpdateViews(eUpdateObjects, NULL);
}

AxisConstrains CEditorImpl::GetAxisConstrains()
{
    return m_selectedAxis;
}

void CEditorImpl::SetTerrainAxisIgnoreObjects(bool bIgnore)
{
    m_bTerrainAxisIgnoreObjects = bIgnore;
}

bool CEditorImpl::IsTerrainAxisIgnoreObjects()
{
    return m_bTerrainAxisIgnoreObjects;
}

void CEditorImpl::SetReferenceCoordSys(RefCoordSys refCoords)
{
    m_refCoordsSys = refCoords;
    m_lastCoordSys[m_currEditMode] = m_refCoordsSys;

    // Update all views.
    UpdateViews(eUpdateObjects, NULL);

    // Update the construction plane infos.
    CViewport* pViewport = GetActiveView();
    if (pViewport)
    {
        //Pre and Post widget rendering calls are made here to make sure that the proper camera state is set.
        //MakeConstructionPlane will make a call to ViewToWorldRay which needs the correct camera state 
        //in the CRenderViewport to be set. 
        pViewport->PreWidgetRendering();

        pViewport->MakeConstructionPlane(GetIEditor()->GetAxisConstrains());

        pViewport->PostWidgetRendering();
    }

    Notify(eNotify_OnRefCoordSysChange);
}

RefCoordSys CEditorImpl::GetReferenceCoordSys()
{
    return m_refCoordsSys;
}

CBaseObject* CEditorImpl::NewObject(const char* typeName, const char* fileName, const char* name, float x, float y, float z, bool modifyDoc)
{
    CUndo undo("Create new object");

    IEditor* editor = GetIEditor();
    if (modifyDoc)
    {
        editor->SetModifiedFlag();
        editor->SetModifiedModule(eModifiedBrushes);
    }
    CBaseObject* object = editor->GetObjectManager()->NewObject(typeName, 0, fileName, name);
    if (!object)
    {
        return nullptr;
    }

    object->SetPos(Vec3(x, y, z));

    return object;
}

const SGizmoParameters& CEditorImpl::GetGlobalGizmoParameters()
{
    if (!m_pGizmoParameters.get())
    {
        m_pGizmoParameters.reset(new SGizmoParameters());
    }

    m_pGizmoParameters->axisConstraint = m_selectedAxis;
    m_pGizmoParameters->referenceCoordSys = m_refCoordsSys;
    m_pGizmoParameters->axisGizmoScale = gSettings.gizmo.axisGizmoSize;
    m_pGizmoParameters->axisGizmoText = gSettings.gizmo.axisGizmoText;

    return *m_pGizmoParameters;
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::DeleteObject(CBaseObject* obj)
{
    SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedBrushes);
    GetObjectManager()->DeleteObject(obj);
}

CBaseObject* CEditorImpl::CloneObject(CBaseObject* obj)
{
    SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedBrushes);
    return GetObjectManager()->CloneObject(obj);
}

void CEditorImpl::StartObjectCreation(const QString& type, const QString& file)
{
    if (!GetDocument()->IsDocumentReady())
    {
        return;
    }

    CObjectCreateTool* pTool = new CObjectCreateTool;
    GetIEditor()->SetEditTool(pTool);
    pTool->StartCreation(type, file);
}

CBaseObject* CEditorImpl::GetSelectedObject()
{
    CBaseObject* obj = nullptr;
    if (m_pObjectManager->GetSelection()->GetCount() != 1)
    {
        return nullptr;
    }
    return m_pObjectManager->GetSelection()->GetObject(0);
}

void CEditorImpl::SelectObject(CBaseObject* obj)
{
    GetObjectManager()->SelectObject(obj);
}

IObjectManager* CEditorImpl::GetObjectManager()
{
    return m_pObjectManager;
};

CSettingsManager* CEditorImpl::GetSettingsManager()
{
    // Do not go any further before XML class is ready to use
    if (!gEnv)
    {
        return nullptr;
    }

    if (!GetISystem())
    {
        return nullptr;
    }

    if (!m_pSettingsManager)
    {
        m_pSettingsManager = new CSettingsManager(eSettingsManagerMemoryStorage);
    }

    return m_pSettingsManager;
}

CSelectionGroup* CEditorImpl::GetSelection()
{
    return m_pObjectManager->GetSelection();
}

int CEditorImpl::ClearSelection()
{
    if (GetSelection()->IsEmpty())
    {
        return 0;
    }
    QString countString = GetCommandManager()->Execute("general.clear_selection");
    return countString.toInt();
}

void CEditorImpl::LockSelection(bool bLock)
{
    // Selection must be not empty to enable selection lock.
    if (!GetSelection()->IsEmpty())
    {
        m_bSelectionLocked = bLock;
    }
    else
    {
        m_bSelectionLocked = false;
    }
}

bool CEditorImpl::IsSelectionLocked()
{
    return m_bSelectionLocked;
}

void CEditorImpl::PickObject(IPickObjectCallback* callback, const QMetaObject* targetClass, const char* statusText, bool bMultipick)
{
    m_pPickTool = new CPickObjectTool(callback, targetClass);

    static_cast<CPickObjectTool*>(m_pPickTool.get())->SetMultiplePicks(bMultipick);
    if (statusText)
    {
        m_pPickTool.get()->SetStatusText(statusText);
    }

    SetEditTool(m_pPickTool);
}

void CEditorImpl::CancelPick()
{
    SetEditTool(0);
    m_pPickTool = 0;
}

bool CEditorImpl::IsPicking()
{
    if (GetEditTool() == m_pPickTool && m_pPickTool != 0)
    {
        return true;
    }
    return false;
}

CViewManager* CEditorImpl::GetViewManager()
{
    return m_pViewManager;
}

CViewport* CEditorImpl::GetActiveView()
{
    MainWindow* mainWindow = MainWindow::instance();
    if (mainWindow)
    {
        CLayoutViewPane* viewPane = mainWindow->GetActiveView();
        if (viewPane)
        {
            return qobject_cast<QtViewport*>(viewPane->GetViewport());
        }
    }
    return nullptr;
}

void CEditorImpl::SetActiveView(CViewport* viewport)
{
    m_pViewManager->SelectViewport(viewport);
}

void CEditorImpl::UpdateViews(int flags, const AABB* updateRegion)
{
    AABB prevRegion = m_pViewManager->GetUpdateRegion();
    if (updateRegion)
    {
        m_pViewManager->SetUpdateRegion(*updateRegion);
    }
    m_pViewManager->UpdateViews(flags);
    if (updateRegion)
    {
        m_pViewManager->SetUpdateRegion(prevRegion);
    }
}

void CEditorImpl::ReloadTrackView()
{
    Notify(eNotify_OnReloadTrackView);
}

void CEditorImpl::UpdateSequencer(bool bOnlyKeys)
{
    if (bOnlyKeys)
    {
        Notify(eNotify_OnUpdateSequencerKeys);
    }
    else
    {
        Notify(eNotify_OnUpdateSequencer);
    }
}

void CEditorImpl::ResetViews()
{
    m_pViewManager->ResetViews();

    m_pDisplaySettings->SetRenderFlags(m_pDisplaySettings->GetRenderFlags());
}

IIconManager* CEditorImpl::GetIconManager()
{
    return m_pIconManager;
}

IBackgroundTaskManager* CEditorImpl::GetBackgroundTaskManager()
{
    return m_pBackgroundTaskManager.get();
}

IBackgroundScheduleManager* CEditorImpl::GetBackgroundScheduleManager()
{
    return m_pBackgroundScheduleManager.get();
}

IEditorFileMonitor* CEditorImpl::GetFileMonitor()
{
    return m_pEditorFileMonitor.get();
}

void CEditorImpl::RegisterEventLoopHook(IEventLoopHook* pHook)
{
    CCryEditApp::instance()->RegisterEventLoopHook(pHook);
}

void CEditorImpl::UnregisterEventLoopHook(IEventLoopHook* pHook)
{
    CCryEditApp::instance()->UnregisterEventLoopHook(pHook);
}

void CEditorImpl::LaunchAWSConsole(QString destUrl)
{
    CCryEditApp::instance()->OnAWSLaunchConsolePage(destUrl.toStdString().c_str());
}

bool CEditorImpl::ToProjectConfigurator(const QString& msg, const QString& caption, const QString& location)
{
    return CCryEditApp::instance()->ToProjectConfigurator(msg, caption, location);
}

float CEditorImpl::GetTerrainElevation(float x, float y)
{
    I3DEngine* engine = m_pSystem->GetI3DEngine();
    if (!engine)
    {
        return 0;
    }
    return engine->GetTerrainElevation(x, y);
}

CHeightmap* CEditorImpl::GetHeightmap()
{
    assert(m_pTerrainManager);
    return m_pTerrainManager->GetHeightmap();
}

CVegetationMap* CEditorImpl::GetVegetationMap()
{
    return m_pVegetationMap;
}

const QColor& CEditorImpl::GetColorByName(const QString& name)
{
    return m_QtApplication->GetColorByName(name);
}

void CEditorImpl::SetSelectedRegion(const AABB& box)
{
    m_selectedRegion = box;
}

void CEditorImpl::GetSelectedRegion(AABB& box)
{
    box = m_selectedRegion;
}

const QtViewPane* CEditorImpl::OpenView(QString sViewClassName, bool reuseOpened)
{
    auto openMode = reuseOpened ? QtViewPane::OpenMode::None : QtViewPane::OpenMode::MultiplePanes;
    return QtViewPaneManager::instance()->OpenPane(sViewClassName, openMode);
}

QWidget* CEditorImpl::OpenWinWidget(WinWidgetId openId)
{
    if (m_winWidgetManager)
    {
        return m_winWidgetManager->OpenWinWidget(openId);
    }
    return nullptr;
}

WinWidget::WinWidgetManager* CEditorImpl::GetWinWidgetManager() const
{
    return m_winWidgetManager.get();
}

QWidget* CEditorImpl::FindView(QString viewClassName)
{
    return QtViewPaneManager::instance()->GetView(viewClassName);
}

// Intended to give a window focus only if it is currently open
bool CEditorImpl::SetViewFocus(const char* sViewClassName)
{
    QWidget* findWindow = FindView(sViewClassName);
    if (findWindow)
    {
        findWindow->setFocus(Qt::OtherFocusReason);
        return true;
    }
    return false;
}

bool CEditorImpl::CloseView(const char* sViewClassName)
{
    return QtViewPaneManager::instance()->ClosePane(sViewClassName);
}

void CEditorImpl::CloseView(const GUID& classId)
{
    IClassDesc* found = GetClassFactory()->FindClass(classId);
    if (found)
    {
        CloseView(found->ClassName().toUtf8().data());
    }
}

IDataBaseManager* CEditorImpl::GetDBItemManager(EDataBaseItemType itemType)
{
    switch (itemType)
    {
    case EDB_TYPE_MATERIAL:
        return m_pMaterialManager;
    case EDB_TYPE_ENTITY_ARCHETYPE:
        return m_pEntityManager;
    case EDB_TYPE_PREFAB:
        return m_pPrefabManager;
    case EDB_TYPE_GAMETOKEN:
        return m_pGameTokenManager;
    case EDB_TYPE_PARTICLE:
        return m_particleManager;
    }
    return 0;
}

CBaseLibraryDialog* CEditorImpl::OpenDataBaseLibrary(EDataBaseItemType type, IDataBaseItem* pItem)
{
    if (pItem)
    {
        type = pItem->GetType();
    }

    const QtViewPane* pane = nullptr;
    if (type == EDB_TYPE_MATERIAL)
    {
        pane = QtViewPaneManager::instance()->OpenPane(LyViewPane::MaterialEditor);

        // This is a workaround for a timing issue where the material editor
        // gets in a bad state while it is being polished for the first time
        // while loading a material at the same time, so delay the setting
        // of the material until the next event queue check
        QTimer::singleShot(0, [this, type, pItem] {
                IDataBaseManager* pManager = GetDBItemManager(type);
                if (pManager)
                {
                    pManager->SetSelectedItem(pItem);
                }
            });
    }
    else
    {
        pane = QtViewPaneManager::instance()->OpenPane(LyViewPane::DatabaseView);

        IDataBaseManager* pManager = GetDBItemManager(type);
        if (pManager)
        {
            pManager->SetSelectedItem(pItem);
        }
    }

    if (!pane)
    {
        return nullptr;
    }

    if (auto dlgDB = qobject_cast<CDataBaseDialog*>(pane->Widget()))
    {
        CDataBaseDialogPage* pPage = dlgDB->SelectDialog(type, pItem);
        if (pPage)
        {
            return qobject_cast<CBaseLibraryDialog*>(pPage);
        }
    }
    return nullptr;
}

bool CEditorImpl::SelectColor(QColor& color, QWidget* parent)
{
    QColorDialog dlg(color, parent);
    if (dlg.exec() == QDialog::Accepted)
    {
        color = dlg.currentColor();
        return true;
    }
    return false;
}

void CEditorImpl::SetInGameMode(bool inGame)
{
    static bool bWasInSimulationMode(false);

    if (inGame)
    {
        bWasInSimulationMode = GetIEditor()->GetGameEngine()->GetSimulationMode();
        GetIEditor()->GetGameEngine()->SetSimulationMode(false);
        GetIEditor()->GetCommandManager()->Execute("general.enter_game_mode");
    }
    else
    {
        GetIEditor()->GetCommandManager()->Execute("general.exit_game_mode");
        GetIEditor()->GetGameEngine()->SetSimulationMode(bWasInSimulationMode);
    }
}

bool CEditorImpl::IsInGameMode()
{
    if (m_pGameEngine)
    {
        return m_pGameEngine->IsInGameMode();
    }
    return false;
}

bool CEditorImpl::IsInSimulationMode()
{
    if (m_pGameEngine)
    {
        return m_pGameEngine->GetSimulationMode();
    }
    return false;
}

bool CEditorImpl::IsInTestMode()
{
    return CCryEditApp::instance()->IsInTestMode();
}

bool CEditorImpl::IsInConsolewMode()
{
    return CCryEditApp::instance()->IsInConsoleMode();
}

bool CEditorImpl::IsInLevelLoadTestMode()
{
    return CCryEditApp::instance()->IsInLevelLoadTestMode();
}

bool CEditorImpl::IsInPreviewMode()
{
    return CCryEditApp::instance()->IsInPreviewMode();
}

void CEditorImpl::EnableAcceleratos(bool bEnable)
{
    KeyboardCustomizationSettings::EnableShortcutsGlobally(bEnable);
}

static AZStd::string SafeGetStringFromDocument(rapidjson::Document& projectCfg, const char* memberName)
{
    if (projectCfg.HasMember(memberName) && projectCfg[memberName].IsString())
    {
        return projectCfg[memberName].GetString();
    }

    return "";
}

AZStd::string CEditorImpl::LoadProjectIdFromProjectData()
{
    const char* MissingProjectId = "";

    // get the full path of the project.json
    AZStd::string fullPath;
    AZStd::string relPath("project.json");
    bool fullPathFound = false;

    using namespace AzToolsFramework;
    AssetSystemRequestBus::BroadcastResult(fullPathFound, &AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, relPath, fullPath);

    if (!fullPathFound)
    {
        return MissingProjectId;
    }

    QFile file(fullPath.c_str());
    if (!file.open(QIODevice::ReadOnly))
    {
        return MissingProjectId;
    }

    // Read the project.json file using its full path
    QByteArray fileContents = file.readAll();
    file.close();

    rapidjson::Document projectCfg;
    projectCfg.Parse(fileContents);

    if (!projectCfg.IsObject())
    {
        return MissingProjectId;
    }

    AZStd::string projectId = SafeGetStringFromDocument(projectCfg, "project_id");

    // if we don't have a valid projectId by now, it's not happening
    if (projectId.empty() || projectId[0] == '\0')
    {
        return MissingProjectId;
    }

    // get the project Id and project name from the project.json file
    QString projectName(SafeGetStringFromDocument(projectCfg, "project_name").data());

    QFileInfo fileInfo(fullPath.c_str());
    QDir folderDirectory = fileInfo.dir();

    // get the project name from the folder directory
    QString editorProjectName = folderDirectory.dirName();

    // if the project name in the file doesn't match the directory name, it probably means that this is
    // a copied project, and not safe to put any plain text into the projectId string
    if (editorProjectName.compare(projectName, Qt::CaseInsensitive) != 0)
    {
        return projectId;
    }

    // get the project Id generated by using the project name from the folder directory
    QByteArray editorProjectNameUtf8 = editorProjectName.toUtf8();
    AZ::Uuid id = AZ::Uuid::CreateName(editorProjectNameUtf8.constData());

    // The projects that Lumberyard ships with had their project IDs hand-generated based on the name of the level.
    // Therefore, if the UUID from the project name is the same as the UUID in the file, it's one of our projects
    // and we can therefore send the name back, making it easier for Metrics to determine which level it was.
    // We are checking to see if this is a project we ship with Lumberyard, and therefore we can unobfuscate non-customer information.
    if (id != AZ::Uuid(projectId.data()))
    {
        return projectId;
    }

    QByteArray projectNameUtf8 = projectName.toUtf8();

    projectId += " [";
    projectId += projectNameUtf8.constData();
    projectId += "]";

    return projectId;
}

void CEditorImpl::InitMetrics()
{
    AZStd::string projectId = LoadProjectIdFromProjectData();

    static char statusFilePath[_MAX_PATH + 1];
    {
        azstrcpy(statusFilePath, _MAX_PATH, AZ::IO::FileIOBase::GetInstance()->GetAlias("@devroot@"));
        azstrcat(statusFilePath, _MAX_PATH, &(Aws::FileSystem::PATH_DELIM));
        azstrcat(statusFilePath, _MAX_PATH, m_crashLogFileName);
    }

    AWSNativeSDKInit::InitializationManager::InitAwsApi();
    const bool metricsInitAwsApi = false;

    // LY_METRICS_BUILD_TIME is defined by the build system
    LyMetrics_Initialize("Editor.exe", 2, metricsInitAwsApi, projectId.data(), statusFilePath, LY_METRICS_BUILD_TIME);
}

void CEditorImpl::DetectVersion()
{
#if defined(AZ_PLATFORM_WINDOWS)
    char exe[_MAX_PATH];
    DWORD dwHandle;
    UINT len;

    char ver[1024 * 8];

    GetModuleFileName(NULL, exe, _MAX_PATH);

    int verSize = GetFileVersionInfoSize(exe, &dwHandle);
    if (verSize > 0)
    {
        GetFileVersionInfo(exe, dwHandle, 1024 * 8, ver);
        VS_FIXEDFILEINFO* vinfo;
        VerQueryValue(ver, "\\", (void**)&vinfo, &len);

        m_fileVersion.v[0] = vinfo->dwFileVersionLS & 0xFFFF;
        m_fileVersion.v[1] = vinfo->dwFileVersionLS >> 16;
        m_fileVersion.v[2] = vinfo->dwFileVersionMS & 0xFFFF;
        m_fileVersion.v[3] = vinfo->dwFileVersionMS >> 16;

        m_productVersion.v[0] = vinfo->dwProductVersionLS & 0xFFFF;
        m_productVersion.v[1] = vinfo->dwProductVersionLS >> 16;
        m_productVersion.v[2] = vinfo->dwProductVersionMS & 0xFFFF;
        m_productVersion.v[3] = vinfo->dwProductVersionMS >> 16;
    }
#else
    // This requires the application version to be set using QCoreApplication::setApplicationVersion, which isn't done yet.
    const QString version = qApp->applicationVersion();
    if (!version.isEmpty())
    {
        QByteArray versionBytes = version.toUtf8();
        m_fileVersion.Set(versionBytes.data());
        m_productVersion.Set(versionBytes.data());
    }
#endif
}

XmlNodeRef CEditorImpl::FindTemplate(const QString& templateName)
{
    return m_templateRegistry.FindTemplate(templateName);
}

void CEditorImpl::AddTemplate(const QString& templateName, XmlNodeRef& tmpl)
{
    m_templateRegistry.AddTemplate(templateName, tmpl);
}

CShaderEnum* CEditorImpl::GetShaderEnum()
{
    return m_pShaderEnum;
}

bool CEditorImpl::ExecuteConsoleApp(const QString& CommandLine, QString& OutputText, bool bNoTimeOut, bool bShowWindow)
{
    CLogFile::FormatLine("Executing console application '%s'", CommandLine.toUtf8().data());

    QProcess process;
    if (bShowWindow)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        process.start(QString("cmd.exe /C %1").arg(CommandLine));
#elif defined(AZ_PLATFORM_APPLE_OSX)
        process.start(QString("/usr/bin/osascript -e 'tell application \"Terminal\" to do script \"%1\"'").arg(QString(CommandLine).replace("\"", "\\\"")));
#else
#error "Needs to be implemented"
#endif
    }
    else
    {
        process.start(CommandLine);
    }

    if (!process.waitForStarted())
    {
        return false;
    }

    // Wait for the process to finish
    process.waitForFinished();
    if (!bShowWindow)
    {
        OutputText = process.readAllStandardOutput();
    }

    return true;
}

void CEditorImpl::BeginUndo()
{
    if (m_pUndoManager)
    {
        m_pUndoManager->Begin();
    }
}

void CEditorImpl::RestoreUndo(bool undo)
{
    if (m_pPrefabManager)
    {
        m_pPrefabManager->SetSkipPrefabUpdate(true);
    }
    if (m_pUndoManager)
    {
        m_pUndoManager->Restore(undo);
    }
    if (m_pPrefabManager)
    {
        m_pPrefabManager->SetSkipPrefabUpdate(false);
    }
}

void CEditorImpl::AcceptUndo(const QString& name)
{
    if (m_pUndoManager)
    {
        m_pUndoManager->Accept(name);
    }
}

void CEditorImpl::CancelUndo()
{
    if (m_pUndoManager)
    {
        m_pUndoManager->Cancel();
    }
}

void CEditorImpl::SuperBeginUndo()
{
    if (m_pUndoManager)
    {
        m_pUndoManager->SuperBegin();
    }
}

void CEditorImpl::SuperAcceptUndo(const QString& name)
{
    if (m_pUndoManager)
    {
        m_pUndoManager->SuperAccept(name);
    }
}

void CEditorImpl::SuperCancelUndo()
{
    if (m_pUndoManager)
    {
        m_pUndoManager->SuperCancel();
    }
}

void CEditorImpl::SuspendUndo()
{
    if (m_pUndoManager)
    {
        m_pUndoManager->Suspend();
    }
}

void CEditorImpl::ResumeUndo()
{
    if (m_pUndoManager)
    {
        m_pUndoManager->Resume();
    }
}

void CEditorImpl::Undo()
{
    if (m_pUndoManager)
    {
        AzToolsFramework::EditorMetricsEventBusSelectionChangeHelper selectionChangeMetricsHelper;

        m_pUndoManager->Undo();
    }
}

void CEditorImpl::Redo()
{
    if (m_pUndoManager)
    {
        AzToolsFramework::EditorMetricsEventBusSelectionChangeHelper selectionChangeMetricsHelper;

        m_pUndoManager->Redo();
    }
}

bool CEditorImpl::IsUndoRecording()
{
    if (m_pUndoManager)
    {
        return m_pUndoManager->IsUndoRecording();
    }
    return false;
}

bool CEditorImpl::IsUndoSuspended()
{
    if (m_pUndoManager)
    {
        return m_pUndoManager->IsUndoSuspended();
    }
    return false;
}

void CEditorImpl::RecordUndo(IUndoObject* obj)
{
    if (m_pUndoManager)
    {
        m_pUndoManager->RecordUndo(obj);
    }
}

bool CEditorImpl::FlushUndo(bool isShowMessage)
{
    if (isShowMessage && m_pUndoManager && m_pUndoManager->IsHaveUndo() && QMessageBox::question(AzToolsFramework::GetActiveWindow(), QObject::tr("Flush Undo"), QObject::tr("After this operation undo will not be available! Are you sure you want to continue?")) != QMessageBox::Yes)
    {
        return false;
    }

    if (m_pUndoManager)
    {
        m_pUndoManager->Flush();
    }
    return true;
}

bool CEditorImpl::ClearLastUndoSteps(int steps)
{
    if (!m_pUndoManager || !m_pUndoManager->IsHaveUndo())
    {
        return false;
    }

    m_pUndoManager->ClearUndoStack(steps);
    return true;
}

bool CEditorImpl::ClearRedoStack()
{
    if (!m_pUndoManager || !m_pUndoManager->IsHaveRedo())
    {
        return false;
    }

    m_pUndoManager->ClearRedoStack();
    return true;
}

void CEditorImpl::SetConsoleVar(const char* var, float value)
{
    ICVar* ivar = GetSystem()->GetIConsole()->GetCVar(var);
    if (ivar)
    {
        ivar->Set(value);
    }
}

float CEditorImpl::GetConsoleVar(const char* var)
{
    ICVar* ivar = GetSystem()->GetIConsole()->GetCVar(var);
    if (ivar)
    {
        return ivar->GetFVal();
    }
    return 0;
}

CAIManager* CEditorImpl::GetAI()
{
    return m_pAIManager;
}

CCustomActionsEditorManager* CEditorImpl::GetCustomActionManager()
{
    return m_pCustomActionsManager;
}

CAnimationContext* CEditorImpl::GetAnimation()
{
    return m_pAnimationContext;
}

CTrackViewSequenceManager* CEditorImpl::GetSequenceManager()
{
    return m_pSequenceManager;
}

ITrackViewSequenceManager* CEditorImpl::GetSequenceManagerInterface()
{
    return GetSequenceManager();
}

void CEditorImpl::RegisterDocListener(IDocListener* listener)
{
    CCryEditDoc* doc = GetDocument();
    if (doc)
    {
        doc->RegisterListener(listener);
    }
}

void CEditorImpl::UnregisterDocListener(IDocListener* listener)
{
    CCryEditDoc* doc = GetDocument();
    if (doc)
    {
        doc->UnregisterListener(listener);
    }
}

// Confetti Start: Leroy Sikkes
void CEditorImpl::Notify(EEditorNotifyEvent event)
{
    NotifyExcept(event, nullptr);
}

void CEditorImpl::NotifyExcept(EEditorNotifyEvent event, IEditorNotifyListener* listener)
{
    if (m_bExiting)
    {
        return;
    }

    std::list<IEditorNotifyListener*>::iterator it = m_listeners.begin();
    while (it != m_listeners.end())
    {
        if (*it == listener)
        {
            it++;
            continue; // skip "except" listener
        }

        (*it++)->OnEditorNotifyEvent(event);
    }

    if (event == eNotify_OnBeginNewScene)
    {
        if (m_pAxisGizmo)
        {
            m_pAxisGizmo->Release();
        }
        m_pAxisGizmo = 0;
    }
    else if (event == eNotify_OnDisplayRenderUpdate)
    {
        IEditorGame* pEditorGame = m_pGameEngine ? m_pGameEngine->GetIEditorGame() : NULL;
        if (pEditorGame != NULL)
        {
            pEditorGame->OnDisplayRenderUpdated(m_pDisplaySettings->IsDisplayHelpers());
        }
    }

    if (event == eNotify_OnInit)
    {
        REGISTER_COMMAND("py", CmdPy, 0, "Execute a Python code snippet.");
    }

    GetPluginManager()->NotifyPlugins(event);

    if (event == eNotify_OnEndGameMode)
    {
        LogEndGameMode();
    }
}
// Confetti end: Leroy Sikkes

void CEditorImpl::RegisterNotifyListener(IEditorNotifyListener* listener)
{
    listener->m_bIsRegistered = true;
    stl::push_back_unique(m_listeners, listener);
}

void CEditorImpl::UnregisterNotifyListener(IEditorNotifyListener* listener)
{
    m_listeners.remove(listener);
    listener->m_bIsRegistered = false;
}

ISourceControl* CEditorImpl::GetSourceControl()
{
    CryAutoLock<CryMutex> lock(m_pluginMutex);

    if (m_pSourceControl)
    {
        return m_pSourceControl;
    }

    IEditorClassFactory* classFactory = GetIEditor() ? GetIEditor()->GetClassFactory() : nullptr;
    if (classFactory)
    {
        std::vector<IClassDesc*> classes;
        classFactory->GetClassesBySystemID(ESYSTEM_CLASS_SCM_PROVIDER, classes);
        for (int i = 0; i < classes.size(); i++)
        {
            IClassDesc* pClass = classes[i];
            ISourceControl* pSCM = NULL;
            HRESULT hRes = pClass->QueryInterface(__uuidof(ISourceControl), (void**)&pSCM);
            if (!FAILED(hRes) && pSCM)
            {
                m_pSourceControl = pSCM;
                return m_pSourceControl;
            }
        }
    }

    return 0;
}

bool CEditorImpl::IsSourceControlAvailable()
{
    if ((gSettings.enableSourceControl) && (GetSourceControl()))
    {
        return true;
    }

    return false;
}

bool CEditorImpl::IsSourceControlConnected()
{
    if ((gSettings.enableSourceControl) && (GetSourceControl()) && (GetSourceControl()->GetConnectivityState() == ISourceControl::Connected))
    {
        return true;
    }

    return false;
}

IAssetTagging* CEditorImpl::GetAssetTagging()
{
    CryAutoLock<CryMutex> lock(m_pluginMutex);

    if (m_pAssetTagging)
    {
        return m_pAssetTagging;
    }

    std::vector<IClassDesc*> classes;
    GetIEditor()->GetClassFactory()->GetClassesBySystemID(ESYSTEM_CLASS_ASSET_TAGGING, classes);
    for (int i = 0; i < classes.size(); i++)
    {
        IClassDesc* pClass = classes[i];
        IAssetTagging* pAssetTagging = NULL;
        HRESULT hRes = pClass->QueryInterface(__uuidof(IAssetTagging), (void**)&pAssetTagging);
        if (!FAILED(hRes) && pAssetTagging)
        {
            m_pAssetTagging = pAssetTagging;
            return m_pAssetTagging;
        }
    }

    return 0;
}

void CEditorImpl::SetMatEditMode(bool bIsMatEditMode)
{
    m_bMatEditMode = bIsMatEditMode;
}

void CEditorImpl::ShowStatusText(bool bEnable)
{
    m_bShowStatusText = bEnable;
}

void CEditorImpl::GetMemoryUsage(ICrySizer* pSizer)
{
    SIZER_COMPONENT_NAME(pSizer, "Editor");

    if (GetDocument())
    {
        SIZER_COMPONENT_NAME(pSizer, "Document");

        GetDocument()->GetMemoryUsage(pSizer);
    }

    if (m_pVegetationMap)
    {
        m_pVegetationMap->GetMemoryUsage(pSizer);
    }
}

void CEditorImpl::ReduceMemory()
{
    GetIEditor()->GetUndoManager()->ClearRedoStack();
    GetIEditor()->GetUndoManager()->ClearUndoStack();
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_FREE_GAME_DATA);
    gEnv->pRenderer->FreeResources(FRR_TEXTURES);

#if defined(AZ_PLATFORM_WINDOWS)
    HANDLE hHeap = GetProcessHeap();

    if (hHeap)
    {
        uint64 maxsize = (uint64)HeapCompact(hHeap, 0);
        CryLogAlways("Max Free Memory Block = %I64d Kb", maxsize / 1024);
    }
#endif
}

IExportManager* CEditorImpl::GetExportManager()
{
    if (!m_pExportManager)
    {
        m_pExportManager = new CExportManager();
    }

    return m_pExportManager;
}

void CEditorImpl::AddUIEnums()
{
    // Spec settings for shadow casting lights
    string SpecString[4];
    QStringList types;
    types.push_back("Never=0");
    SpecString[0].Format("VeryHigh Spec=%d", CONFIG_VERYHIGH_SPEC);
    types.push_back(SpecString[0].c_str());
    SpecString[1].Format("High Spec=%d", CONFIG_HIGH_SPEC);
    types.push_back(SpecString[1].c_str());
    SpecString[2].Format("Medium Spec=%d", CONFIG_MEDIUM_SPEC);
    types.push_back(SpecString[2].c_str());
    SpecString[3].Format("Low Spec=%d", CONFIG_LOW_SPEC);
    types.push_back(SpecString[3].c_str());
    m_pUIEnumsDatabase->SetEnumStrings("CastShadows", types);

    // Power-of-two percentages
    string percentStringPOT[5];
    types.clear();
    percentStringPOT[0].Format("Default=%d", 0);
    types.push_back(percentStringPOT[0].c_str());
    percentStringPOT[1].Format("12.5=%d", 1);
    types.push_back(percentStringPOT[1].c_str());
    percentStringPOT[2].Format("25=%d", 2);
    types.push_back(percentStringPOT[2].c_str());
    percentStringPOT[3].Format("50=%d", 3);
    types.push_back(percentStringPOT[3].c_str());
    percentStringPOT[4].Format("100=%d", 4);
    types.push_back(percentStringPOT[4].c_str());
    m_pUIEnumsDatabase->SetEnumStrings("ShadowMinResPercent", types);
}

void CEditorImpl::SetEditorConfigSpec(ESystemConfigSpec spec, ESystemConfigPlatform platform)
{
    gSettings.editorConfigSpec = spec;
    if (m_pSystem->GetConfigSpec(true) != spec || m_pSystem->GetConfigPlatform() != platform)
    {
        m_pSystem->SetConfigSpec(spec, platform, true);
        gSettings.editorConfigSpec = m_pSystem->GetConfigSpec(true);
        GetObjectManager()->SendEvent(EVENT_CONFIG_SPEC_CHANGE);
        AzToolsFramework::EditorEvents::Bus::Broadcast(&AzToolsFramework::EditorEvents::OnEditorSpecChange);
        if (m_pVegetationMap)
        {
            m_pVegetationMap->UpdateConfigSpec();
        }
    }
}

ESystemConfigSpec CEditorImpl::GetEditorConfigSpec() const
{
    return (ESystemConfigSpec)gSettings.editorConfigSpec;
}

ESystemConfigPlatform CEditorImpl::GetEditorConfigPlatform() const
{
    return m_pSystem->GetConfigPlatform();
}

void CEditorImpl::InitFinished()
{
    SProjectSettingsBlock::Load();

    if (!m_bInitialized)
    {
        m_bInitialized = true;
        Notify(eNotify_OnInit);

        // Let system wide listeners know about this as well.
        GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_EDITOR_ON_INIT, 0, 0);
    }
}

void CEditorImpl::ReloadTemplates()
{
    m_templateRegistry.LoadTemplates("Editor");
}

void CEditorImpl::AddErrorMessage(const QString& text, const QString& caption)
{
    if (!m_pErrorsDlg)
    {
        m_pErrorsDlg = new CErrorsDlg(GetEditorMainWindow());
        m_pErrorsDlg->show();
    }

    m_pErrorsDlg->AddMessage(text, caption);
}

void CEditorImpl::CmdPy(IConsoleCmdArgs* pArgs)
{
    // Execute the given script command.
    QString scriptCmd = pArgs->GetCommandLine();

    scriptCmd = scriptCmd.right(scriptCmd.length() - 2); // The part of the text after the 'py'
    scriptCmd = scriptCmd.trimmed();
    PyScript::AcquirePythonLock();
    PyRun_SimpleString(scriptCmd.toUtf8().data());
    PyErr_Print();
    PyScript::ReleasePythonLock();
}

void CEditorImpl::OnObjectContextMenuOpened(QMenu* pMenu, const CBaseObject* pObject)
{
    for (auto it : m_objectContextMenuExtensions)
    {
        it(pMenu, pObject);
    }
}

void CEditorImpl::RegisterObjectContextMenuExtension(TContextMenuExtensionFunc func)
{
    m_objectContextMenuExtensions.push_back(func);
}

void CEditorImpl::SetCurrentMissionTime(float time)
{
    if (CMission* pMission = GetIEditor()->GetDocument()->GetCurrentMission())
    {
        pMission->SetTime(time);
    }
}
// Vladimir@Conffx
SSystemGlobalEnvironment* CEditorImpl::GetEnv()
{
    assert(gEnv);
    return gEnv;
}

// Leroy@Conffx
IEditorParticleUtils* CEditorImpl::GetParticleUtils()
{
    return m_particleEditorUtils;
}

// Leroy@Conffx
SEditorSettings* CEditorImpl::GetEditorSettings()
{
    return &gSettings;
}

// Vladimir@Conffx
IAssetBrowser* CEditorImpl::GetAssetBrowser()
{
    return m_pAssetBrowser;
}

// Vladimir@Conffx
IBaseLibraryManager* CEditorImpl::GetMaterialManagerLibrary()
{
    return m_pMaterialManager;
}

// Vladimir@Conffx
IEditorMaterialManager* CEditorImpl::GetIEditorMaterialManager()
{
    return m_pMaterialManager;
}

IImageUtil* CEditorImpl::GetImageUtil()
{
    return m_pImageUtil;
}

QMimeData* CEditorImpl::CreateQMimeData() const
{
    return new QMimeData();
}

void CEditorImpl::DestroyQMimeData(QMimeData* data) const
{
    delete data;
}

CBaseObject* CEditorImpl::BaseObjectFromEntityId(EntityId id)
{
    return CEntityObject::FindFromEntityId(id);
}

bool CEditorImpl::IsLegacyUIEnabled()
{
    return m_isLegacyUIEnabled;
}

void CEditorImpl::SetLegacyUIEnabled(bool enabled)
{
    m_isLegacyUIEnabled = enabled;
}

bool CEditorImpl::IsNewViewportInteractionModelEnabled() const
{
    return m_isNewViewportInteractionModelEnabled;
}

void CEditorImpl::OnStartPlayInEditor()
{
    if (SelectionContainsComponentEntities())
    {
        SetEditMode(eEditModeSelect);
    }
    LogBeginGameMode();
}

void CEditorImpl::InitializeCrashLog()
{
    LyMetrics_InitializeCurrentProcessStatus(m_crashLogFileName, LY_METRICS_BUILD_TIME);

#if defined(WIN32) || defined(WIN64)
    if (::IsDebuggerPresent())
    {
        LyMetrics_UpdateCurrentProcessStatus(EESS_DebuggerAttached);
    }
#endif
}

void CEditorImpl::ShutdownCrashLog()
{
    LyMetrics_UpdateCurrentProcessStatus(EESS_EditorShutdown);
}

void CEditorImpl::LogStartingShutdown()
{
    LyMetrics_UpdateCurrentProcessStatus(EESS_StartingEditorShutdown);
}

void CEditorImpl::LogEarlyShutdownExit()
{
    LyMetrics_UpdateCurrentProcessStatus(EESS_EarlyShutdownExit);
}

void CEditorImpl::LogBeginGameMode()
{
    LyMetrics_UpdateCurrentProcessStatus(EESS_InGame);
}

void CEditorImpl::LogEndGameMode()
{
    EEditorSessionStatus sessionStatus = EESS_EditorOpened;

#if defined(WIN32) || defined(WIN64)
    if (::IsDebuggerPresent())
    {
        sessionStatus = EESS_DebuggerAttached;
    }
#endif

    LyMetrics_UpdateCurrentProcessStatus(sessionStatus);
}

namespace
{
    const std::vector<std::pair<EEditMode, QString>> s_editModeNames = {
        { eEditModeSelect, QStringLiteral("Select") },
        { eEditModeSelectArea, QStringLiteral("SelectArea") },
        { eEditModeMove, QStringLiteral("Move") },
        { eEditModeRotate, QStringLiteral("Rotate") },
        { eEditModeScale, QStringLiteral("Scale") }
    };
}

void CEditorImpl::LoadSettings()
{
    QSettings settings(QStringLiteral("Amazon"), QStringLiteral("Lumberyard"));

    settings.beginGroup(QStringLiteral("Editor"));
    settings.beginGroup(QStringLiteral("CoordSys"));

    for (const auto& editMode : s_editModeNames)
    {
        if (settings.contains(editMode.second))
        {
            m_lastCoordSys[editMode.first] = static_cast<RefCoordSys>(settings.value(editMode.second).toInt());
        }
    }

    settings.endGroup(); // CoordSys
    settings.endGroup(); // Editor
}

void CEditorImpl::SaveSettings() const
{
    QSettings settings(QStringLiteral("Amazon"), QStringLiteral("Lumberyard"));

    settings.beginGroup(QStringLiteral("Editor"));
    settings.beginGroup(QStringLiteral("CoordSys"));

    for (const auto& editMode : s_editModeNames)
    {
        settings.setValue(editMode.second, static_cast<int>(m_lastCoordSys[editMode.first]));
    }

    settings.endGroup(); // CoordSys
    settings.endGroup(); // Editor
}
