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

#include <QCoreApplication>
#include <QDir>

#include "GameEngine.h"
#include "IEditorImpl.h"
#include "CryEditDoc.h"
#include "Objects/EntityScript.h"
#include "Objects/AIWave.h"
#include "Geometry/EdMesh.h"
#include "Mission.h"
#include "Terrain/SurfaceType.h"
#include "Terrain/Heightmap.h"
#include "Terrain/TerrainGrid.h"
#include "TerrainLighting.h"
#include "ViewManager.h"
#include "StartupLogoDialog.h"
#include "AI/AIManager.h"
#include "AI/CoverSurfaceManager.h"
#include "AI/NavDataGeneration/FlightNavRegion.h"
#include "Objects/ObjectLayerManager.h"
#include "Objects/AICoverSurface.h"
#include "Objects/SmartObject.h"
#include "AI/NavDataGeneration/Navigation.h"
#include "Material/MaterialManager.h"
#include "Particles/ParticleManager.h"
#include "HyperGraph/FlowGraphManager.h"
#include "HyperGraph/FlowGraphModuleManager.h"
#include "HyperGraph/FlowGraphDebuggerEditor.h"
#include "UIEnumsDatabase.h"
#include "EquipPackLib.h"
#include "Util/Ruler.h"
#include "CustomActions/CustomActionsEditorManager.h"
#include "Material/MaterialFXGraphMan.h"
#include "AnimationContext.h"
#include <IAgent.h>
#include <I3DEngine.h>
#include <IAISystem.h>
#include <IEntitySystem.h>
#include <IMovieSystem.h>
#include <IScriptSystem.h>
#include <ICryPak.h>
#include <IPhysics.h>
#include <IEditorGame.h>
#include <ITimer.h>
#include <IFlowSystem.h>
#include <ICryAnimation.h>
#include <IGame.h>
#include <IGameFramework.h>
#include <IDialogSystem.h>
#include <IDynamicResponseSystem.h>
#include <IDeferredCollisionEvent.h>
#include "platform_impl.h"
#include "Prefabs/PrefabManager.h"
#include "Prefabs/PrefabEvents.h"
#include <ITimeOfDay.h>
#include <LyShine/ILyShine.h>
#include <ParseEngineConfig.h>
#include <Core/EditorFilePathManager.h>
#include <CrySystemBus.h>
#include <MainThreadRenderRequestBus.h>

#include <LmbrCentral/Animation/SimpleAnimationComponentBus.h>

#include <AzCore/base.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Input/Buses/Requests/InputChannelRequestBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include "QtUtil.h"
#include "MainWindow.h"

#include <QMessageBox>
#include <QThread>

static const char defaultFileExtension[] = ".ly";
static const char oldFileExtension[] = ".cry";

// Implementation of System Callback structure.
struct SSystemUserCallback
    : public ISystemUserCallback
{
    SSystemUserCallback(IInitializeUIInfo* logo) : m_threadErrorHandler(this) { m_pLogo = logo; };
    virtual void OnSystemConnect(ISystem* pSystem)
    {
        ModuleInitISystem(pSystem, "Editor");
    }

    virtual bool OnError(const char* szErrorString)
    {
        // since we show a message box, we have to use the GUI thread
        if (QThread::currentThread() != qApp->thread())
        {
            bool result = false;
            QMetaObject::invokeMethod(&m_threadErrorHandler, "OnError", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, result), Q_ARG(const char*, szErrorString));
            return result;
        }

        if (szErrorString)
        {
            Log(szErrorString);
        }

        if (GetIEditor()->IsInTestMode())
        {
            exit(1);
        }

        char str[4096];

        if (szErrorString)
        {
            azsnprintf(str, 4096, "%s\r\nSave Level Before Exiting the Editor?", szErrorString);
        }
        else
        {
            azsnprintf(str, 4096, "Unknown Error\r\nSave Level Before Exiting the Editor?");
        }

        int res = IDNO;

        ICVar* pCVar = gEnv->pConsole ? gEnv->pConsole->GetCVar("sys_no_crash_dialog") : NULL;

        if (!pCVar || pCVar->GetIVal() == 0)
        {
            res = QMessageBox::critical(QApplication::activeWindow(), QObject::tr("Engine Error"), str, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        }

        if (res == QMessageBox::Yes || res == QMessageBox::No)
        {
            if (res == QMessageBox::Yes)
            {
                if (GetIEditor()->SaveDocument())
                {
                    QMessageBox::information(QApplication::activeWindow(), QObject::tr("Save"), QObject::tr("Level has been successfully saved!\r\nPress Ok to terminate Editor."));
                }
            }
        }

        return true;
    }

    virtual bool OnSaveDocument()
    {
        bool success = false;

        if (GetIEditor())
        {
            // Turn off save backup as we force a backup before reaching this point
            bool prevSaveBackup = gSettings.bBackupOnSave;
            gSettings.bBackupOnSave = false;

            success = GetIEditor()->SaveDocument();
            gSettings.bBackupOnSave = prevSaveBackup;
        }

        return success;
    }

    virtual bool OnBackupDocument()
    {
        CCryEditDoc* level = GetIEditor() ? GetIEditor()->GetDocument() : nullptr;
        if (level)
        {
            return level->BackupBeforeSave(true);
        }

        return false;
    }

    virtual void OnProcessSwitch()
    {
        if (GetIEditor()->IsInGameMode())
        {
            GetIEditor()->SetInGameMode(false);
        }
    }

    virtual void OnInitProgress(const char* sProgressMsg)
    {
        if (m_pLogo)
        {
            m_pLogo->SetInfoText(sProgressMsg);
        }
    }

    virtual int ShowMessage(const char* text, const char* caption, unsigned int uType)
    {
        const UINT kMessageBoxButtonMask = 0x000f;
        if (!GetIEditor()->IsInGameMode() && (uType == 0 || uType == MB_OK || !(uType & kMessageBoxButtonMask)))
        {
            static_cast<CEditorImpl*>(GetIEditor())->AddErrorMessage(text, caption);
            return IDOK;
        }
        return CryMessageBox(text, caption, uType);
    }

    virtual void GetMemoryUsage(ICrySizer* pSizer)
    {
        GetIEditor()->GetMemoryUsage(pSizer);
    }

    void OnSplashScreenDone()
    {
        m_pLogo = nullptr;
    }

private:
    IInitializeUIInfo* m_pLogo;
    ThreadedOnErrorHandler m_threadErrorHandler;
};

ThreadedOnErrorHandler::ThreadedOnErrorHandler(ISystemUserCallback* callback)
    : m_userCallback(callback)
{
    moveToThread(qApp->thread());
}

ThreadedOnErrorHandler::~ThreadedOnErrorHandler()
{
}

bool ThreadedOnErrorHandler::OnError(const char* error)
{
    return m_userCallback->OnError(error);
}

//! This class will be used by CSystem to find out whether the negotiation with the assetprocessor failed
class AssetProcessConnectionStatus
    : public AzFramework::AssetSystemConnectionNotificationsBus::Handler
{
public:
    AssetProcessConnectionStatus()
    {
        AzFramework::AssetSystemConnectionNotificationsBus::Handler::BusConnect();
    };
    ~AssetProcessConnectionStatus()
    {
        AzFramework::AssetSystemConnectionNotificationsBus::Handler::BusDisconnect();
    }

    //! Notifies listeners that connection to the Asset Processor failed
    void ConnectionFailed() override
    {
        m_connectionFailed = true;
    }

    void NegotiationFailed() override
    {
        m_negotiationFailed = true;
    }

    bool CheckConnectionFailed()
    {
        return m_connectionFailed;
    }

    bool CheckNegotiationFailed()
    {
        return m_negotiationFailed;
    }
private:
    bool m_connectionFailed = false;
    bool m_negotiationFailed = false;
};


// Implements EntitySystemSink for InGame mode.
struct SInGameEntitySystemListener
    : public IEntitySystemSink
{
    SInGameEntitySystemListener()
    {
    }

    ~SInGameEntitySystemListener()
    {
        // Remove all remaining entities from entity system.
        IEntitySystem* pEntitySystem = GetIEditor()->GetSystem()->GetIEntitySystem();

        for (std::set<int>::iterator it = m_entities.begin(); it != m_entities.end(); ++it)
        {
            EntityId entityId = *it;
            IEntity* pEntity = pEntitySystem->GetEntity(entityId);
            if (pEntity)
            {
                IEntity* pParent = pEntity->GetParent();
                if (pParent)
                {
                    // Childs of irremovable entity are also not deleted (Needed for vehicles weapons for example)
                    if (pParent->GetFlags() & ENTITY_FLAG_UNREMOVABLE)
                    {
                        continue;
                    }
                }
            }
            pEntitySystem->RemoveEntity(*it, true);
        }
    }

    virtual bool OnBeforeSpawn(SEntitySpawnParams& params)
    {
        return true;
    }

    virtual void OnSpawn(IEntity* e, SEntitySpawnParams& params)
    {
        //if (params.ed.ClassId!=0 && ed.ClassId!=PLAYER_CLASS_ID) // Ignore MainPlayer
        if (!(e->GetFlags() & ENTITY_FLAG_UNREMOVABLE))
        {
            m_entities.insert(e->GetId());
        }
    }

    virtual bool OnRemove(IEntity* e)
    {
        if (!(e->GetFlags() & ENTITY_FLAG_UNREMOVABLE))
        {
            m_entities.erase(e->GetId());
        }
        return true;
    }

    virtual void OnReused(IEntity* pEntity, SEntitySpawnParams& params)
    {
        CRY_ASSERT_MESSAGE(false, "Editor should not be receiving entity reused events from IEntitySystemSink, investigate this.");
    }

    virtual void OnEvent(IEntity* pEntity, SEntityEvent& event)
    {
    }

private:
    // Ids of all spawned entities.
    std::set<int> m_entities;
};

namespace
{
    SInGameEntitySystemListener* s_InGameEntityListener = 0;
};

// This class implements calls from the game to the editor
class CGameToEditorInterface
    : public IGameToEditorInterface
{
    virtual void SetUIEnums(const char* sEnumName, const char** sStringsArray, int nStringCount)
    {
        QStringList sStringsList;

        for (int i = 0; i < nStringCount; ++i)
        {
            stl::push_back_unique(sStringsList, sStringsArray[i]);
        }

        GetIEditor()->GetUIEnumsDatabase()->SetEnumStrings(sEnumName, sStringsList);
    }
};

CGameEngine::CGameEngine()
    : m_gameDll(0)
    , m_bIgnoreUpdates(false)
    , m_pEditorGame(0)
    , m_ePendingGameMode(ePGM_NotPending)
{
    m_pISystem = NULL;
    m_pNavigation = 0;
    m_bLevelLoaded = false;
    m_bInGameMode = false;
    m_bSimulationMode = false;
    m_bSimulationModeAI = false;
    m_bSyncPlayerPosition = true;
    m_hSystemHandle = 0;
    m_bUpdateFlowSystem = false;
    m_bJustCreated = false;
    m_pGameToEditorInterface = new CGameToEditorInterface;
    m_levelName = "Untitled";
    m_levelExtension = defaultFileExtension;
    m_playerViewTM.SetIdentity();
    GetIEditor()->RegisterNotifyListener(this);
}

CGameEngine::~CGameEngine()
{
    GetIEditor()->UnregisterNotifyListener(this);
    m_pISystem->GetIMovieSystem()->SetCallback(NULL);
    CEntityScriptRegistry::Instance()->Release();
    CEdMesh::ReleaseAll();

    if (m_pNavigation)
    {
        delete m_pNavigation;
    }

    if (m_pEditorGame)
    {
        m_pEditorGame->Shutdown();
    }

    if (m_gameDll)
    {
        CryFreeLibrary(m_gameDll);
    }

    delete m_pISystem;
    m_pISystem = NULL;

    if (m_hSystemHandle)
    {
        CryFreeLibrary(m_hSystemHandle);
    }

    delete m_pGameToEditorInterface;
    delete m_pSystemUserCallback;
}

static int ed_killmemory_size;
static int ed_indexfiles;

void KillMemory(IConsoleCmdArgs* /* pArgs */)
{
    while (true)
    {
        const int kLimit = 10000000;
        int size;

        if (ed_killmemory_size > 0)
        {
            size = ed_killmemory_size;
        }
        else
        {
            size = rand() * rand();
            size = size > kLimit ? kLimit : size;
        }

        uint8* alloc = new uint8[size];
    }
}

static void CmdGotoEditor(IConsoleCmdArgs* pArgs)
{
    // feature is mostly useful for QA purposes, this works with the game "goto" command
    // this console command actually is used by the game command, the editor command shouldn't be used by the user
    int iArgCount = pArgs->GetArgCount();

    CViewManager* pViewManager = GetIEditor()->GetViewManager();
    CViewport* pRenderViewport = pViewManager->GetGameViewport();
    if (!pRenderViewport)
    {
        return;
    }

    float x, y, z, wx, wy, wz;

    if (iArgCount == 7
        && azsscanf(pArgs->GetArg(1), "%f", &x) == 1
        && azsscanf(pArgs->GetArg(2), "%f", &y) == 1
        && azsscanf(pArgs->GetArg(3), "%f", &z) == 1
        && azsscanf(pArgs->GetArg(4), "%f", &wx) == 1
        && azsscanf(pArgs->GetArg(5), "%f", &wy) == 1
        && azsscanf(pArgs->GetArg(6), "%f", &wz) == 1)
    {
        Matrix34 tm = pRenderViewport->GetViewTM();

        tm.SetTranslation(Vec3(x, y, z));
        tm.SetRotation33(Matrix33::CreateRotationXYZ(DEG2RAD(Ang3(wx, wy, wz))));
        pRenderViewport->SetViewTM(tm);
    }
}

AZ::Outcome<void, AZStd::string> CGameEngine::Init(
    bool bPreviewMode,
    bool bTestMode,
    bool bShaderCacheGen,
    const char* sInCmdLine,
    IInitializeUIInfo* logo,
    HWND hwndForInputSystem)
{
    m_pSystemUserCallback = new SSystemUserCallback(logo);
    m_hSystemHandle = CryLoadLibraryDefName("CrySystem");

    if (!m_hSystemHandle)
    {
        auto errorMessage = AZStd::string::format("%s Loading Failed", CryLibraryDefName("CrySystem"));
        Error(errorMessage.c_str());
        return AZ::Failure(errorMessage);
    }

    PFNCREATESYSTEMINTERFACE pfnCreateSystemInterface =
        (PFNCREATESYSTEMINTERFACE)CryGetProcAddress(m_hSystemHandle, "CreateSystemInterface");


    // Locate the root path
    const char* calcRootPath = nullptr;
    EBUS_EVENT_RESULT(calcRootPath, AZ::ComponentApplicationBus, GetAppRoot);
    if (calcRootPath == nullptr)
    {
        // If the app root isnt available, default to the engine root
        EBUS_EVENT_RESULT(calcRootPath, AzToolsFramework::ToolsApplicationRequestBus, GetEngineRootPath);
    }
    const char* searchPath[] = { calcRootPath };
    CEngineConfig engineConfig(searchPath,AZ_ARRAY_SIZE(searchPath)); // read the engine config also to see what game is running, and what folder(s) there are.

    SSystemInitParams sip;
    engineConfig.CopyToStartupParams(sip);

    sip.connectToRemote = true; // editor always connects
    sip.waitForConnection = true; // editor REQUIRES connect.
    strncpy(sip.remoteIP, "127.0.0.1", sizeof(sip.remoteIP)); // editor ONLY connects to the local asset processor

    sip.bEditor = true;
    sip.bDedicatedServer = false;
    sip.bPreview = bPreviewMode;
    sip.bTestMode = bTestMode;
    sip.hInstance = nullptr;

    sip.pSharedEnvironment = AZ::Environment::GetInstance();

#ifdef AZ_PLATFORM_APPLE_OSX
    // Create a hidden QWidget. Would show a black window on macOS otherwise.
    auto window = new QWidget();
    QObject::connect(qApp, &QApplication::lastWindowClosed, window, &QWidget::deleteLater);
    sip.hWnd = (HWND)window->winId();
#else
    sip.hWnd = hwndForInputSystem;
#endif
    sip.hWndForInputSystem = hwndForInputSystem;

    sip.pLogCallback = &m_logFile;
    sip.sLogFileName = "@log@/Editor.log";
    sip.pUserCallback = m_pSystemUserCallback;
    sip.pValidator = GetIEditor()->GetErrorReport(); // Assign validator from Editor.

    // Calculate the branch token first based on the app root path if possible
    if (calcRootPath!=nullptr)
    {
        AZStd::string appRoot(calcRootPath);
        AZStd::string branchToken;
        AzFramework::StringFunc::AssetPath::CalculateBranchToken(appRoot, branchToken);
        azstrncpy(sip.branchToken, AZ_ARRAY_SIZE(sip.branchToken), branchToken.c_str(), branchToken.length());
    }

    if (sInCmdLine)
    {
        azstrncpy(sip.szSystemCmdLine, AZ_COMMAND_LINE_LEN, sInCmdLine, AZ_COMMAND_LINE_LEN);
        if (strstr(sInCmdLine, "-export"))
        {
            sip.bUnattendedMode = true;
        }
    }

    if (bShaderCacheGen)
    {
        sip.bSkipFont = true;
    }
    AssetProcessConnectionStatus    apConnectionStatus;

    m_pISystem = pfnCreateSystemInterface(sip);

    if (!m_pISystem)
    {
        AZStd::string errorMessage = "Could not initialize CSystem.  View the logs for more details.";

        gEnv = nullptr;
        Error("CreateSystemInterface Failed");
        return AZ::Failure(errorMessage);
    }

    if (apConnectionStatus.CheckNegotiationFailed())
    {
        auto errorMessage = AZStd::string::format("Negotiation with Asset Processor failed.\n"
            "Please ensure the Asset Processor is running on the same branch and try again.");
        gEnv = nullptr;
        return AZ::Failure(errorMessage);
    }

    if (apConnectionStatus.CheckConnectionFailed())
    {
        auto errorMessage = AZStd::string::format("Unable to connect to the local Asset Processor.\n\n"
                                                  "The Asset Processor is either not running locally or not accepting connections on port %d. "
                                                  "Check your remote_port settings in bootstrap.cfg or view the Asset Processor's \"Logs\" tab "
                                                  "for any errors.", sip.remotePort);
        gEnv = nullptr;
        return AZ::Failure(errorMessage);
    }

    // because we're the editor here, we also give tool aliases to the original, unaltered roots:
    string devAssetsFolder = engineConfig.m_rootFolder + "/" + engineConfig.m_gameFolder;
    if (gEnv && gEnv->pFileIO)
    {
        const char* engineRoot = nullptr;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(engineRoot, &AzToolsFramework::ToolsApplicationRequests::GetEngineRootPath);
        if (engineRoot != nullptr)
        {
            gEnv->pFileIO->SetAlias("@engroot@", engineRoot);
        }
        else
        {
            gEnv->pFileIO->SetAlias("@engroot@", engineConfig.m_rootFolder.c_str());
        }

        gEnv->pFileIO->SetAlias("@devroot@", engineConfig.m_rootFolder.c_str());
        gEnv->pFileIO->SetAlias("@devassets@", devAssetsFolder.c_str());
    }

    // No scripts should Execute prior to this point
    if (gEnv->pScriptSystem)
    {
        gEnv->pScriptSystem->PostInit();
    }

    if (gEnv->pAISystem)
    {
        gEnv->pAISystem->PostInit();
    }

    if (gEnv->pFilePathManager)
    {
        delete gEnv->pFilePathManager;
    }
    gEnv->pFilePathManager = new EditorFilePathManager();

    SetEditorCoreEnvironment(gEnv);

    m_pNavigation = new CNavigation(gEnv->pSystem);
    m_pNavigation->Init();

    if (gEnv
        && gEnv->p3DEngine
        && gEnv->p3DEngine->GetTimeOfDay())
    {
        gEnv->p3DEngine->GetTimeOfDay()->BeginEditMode();
    }

    if (gEnv && gEnv->pMovieSystem)
    {
        gEnv->pMovieSystem->EnablePhysicsEvents(m_bSimulationMode);
    }

    CLogFile::AboutSystem();
    REGISTER_CVAR(ed_killmemory_size, -1, VF_DUMPTODISK, "Sets the testing allocation size. -1 for random");
    REGISTER_CVAR(ed_indexfiles, 1, VF_DUMPTODISK, "Index game resource files, 0 - inactive, 1 - active");
    REGISTER_COMMAND("ed_killmemory", KillMemory, VF_NULL, "");
    REGISTER_COMMAND("ed_goto", CmdGotoEditor, VF_CHEAT, "Internal command, used by the 'GOTO' console command\n");

    // The editor needs to handle the quit command differently
    gEnv->pConsole->RemoveCommand("quit");
    REGISTER_COMMAND("quit", CGameEngine::HandleQuitRequest, VF_RESTRICTEDMODE, "Quit/Shutdown the engine");

    EBUS_EVENT(CrySystemEventBus, OnCryEditorInitialized);
    
    return AZ::Success();
}

bool CGameEngine::InitGame(const char* sGameDLL)
{
    if (sGameDLL)
    {
        char fullLibraryName[AZ_MAX_PATH_LEN] = { 0 };
        azsnprintf(fullLibraryName, AZ_MAX_PATH_LEN, "%s%s%s", CrySharedLibraryPrefix, sGameDLL, CrySharedLibraryExtension);
        CryComment("Loading Game DLL: %s", fullLibraryName);

        m_gameDll = CryLoadLibrary(fullLibraryName);

        if (!m_gameDll)
        {
            CryLogAlways("Failed to load Game DLL \"%s\"", fullLibraryName);
            CEntityScriptRegistry::Instance()->LoadScripts();

            return false;
        }

        IEditorGame::TEntryFunction CreateEditorGame =
            (IEditorGame::TEntryFunction)CryGetProcAddress(m_gameDll, "CreateEditorGame");

        if (!CreateEditorGame)
        {
            Error("CryGetProcAddress(%d, %s) failed!", m_gameDll, "CreateEditorGame");

            return false;
        }

        m_pEditorGame = CreateEditorGame();

        if (!m_pEditorGame)
        {
            Error("CreateEditorGame() failed!");

            return false;
        }
    }
    else
    {
        EditorGameRequestBus::BroadcastResult(m_pEditorGame, &EditorGameRequestBus::Events::CreateEditorGame);
        if (!m_pEditorGame)
        {
            Error("Unable to initialize editor game!");
            return false;
        }
    }

    if (m_pEditorGame && !m_pEditorGame->Init(m_pISystem, m_pGameToEditorInterface))
    {
        // Failed to initialize the legacy editor game, so set it to null
        m_pEditorGame = nullptr;
    }

    if (m_pEditorGame)
    {
        // Execute Editor.lua override file.
        if (IScriptSystem* pScriptSystem = m_pISystem->GetIScriptSystem())
        {
            pScriptSystem->ExecuteFile("Editor.Lua", false);
        }
        CStartupLogoDialog::SetText("Loading Entity Scripts...");
        CEntityScriptRegistry::Instance()->LoadScripts();
        CStartupLogoDialog::SetText("Loading Flowgraphs...");
        IEditor* pEditor = GetIEditor();

        if (pEditor->GetFlowGraphManager())
        {
            pEditor->GetFlowGraphManager()->Init();
        }

        CStartupLogoDialog::SetText("Loading Material Effects Flowgraphs...");
        if (GetIEditor()->GetMatFxGraphManager())
        {
            GetIEditor()->GetMatFxGraphManager()->Init();
        }

        CStartupLogoDialog::SetText("Initializing Flowgraph Debugger...");
        if (GetIEditor()->GetFlowGraphDebuggerEditor())
        {
            GetIEditor()->GetFlowGraphDebuggerEditor()->Init();
        }

        CStartupLogoDialog::SetText("Initializing Flowgraph Module Manager...");
        if (GetIEditor()->GetFlowGraphModuleManager())
        {
            GetIEditor()->GetFlowGraphModuleManager()->Init();
        }

        // Initialize prefab events after flowgraphmanager to avoid handling creation of flow node prototypes
        CPrefabManager* pPrefabManager = pEditor->GetPrefabManager();
        if (pPrefabManager)
        {
            CStartupLogoDialog::SetText("Initializing Prefab Events...");
            CPrefabEvents* pPrefabEvents = pPrefabManager->GetPrefabEvents();
            CRY_ASSERT(pPrefabEvents != NULL);
            const bool bResult = pPrefabEvents->Init();
            if (!bResult)
            {
                CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "CGameEngine::InitGame: Failed to init prefab events");
            }
        }

        if (pEditor->GetAI())
        {
            pEditor->GetAI()->Init(m_pISystem);
        }

        CCustomActionsEditorManager* pCustomActionsManager = pEditor->GetCustomActionManager();

        if (pCustomActionsManager)
        {
            pCustomActionsManager->Init(m_pISystem);
        }


        CStartupLogoDialog::SetText("Loading Equipment Packs...");
        // Load Equipment packs from disk and export to Game
        pEditor->GetEquipPackLib()->Reset();
        pEditor->GetEquipPackLib()->LoadLibs(true);
    }

    // in editor we do it later, bExecuteCommandLine was set to false
    m_pISystem->ExecuteCommandLine();

    return true;
}

void CGameEngine::SetLevelPath(const QString& path)
{
    QByteArray levelPath;
    levelPath.reserve(AZ_MAX_PATH_LEN);
    levelPath = Path::ToUnixPath(Path::RemoveBackslash(path)).toUtf8();
    AZ::IO::FileIOBase::GetInstance()->ConvertToAlias(levelPath.data(), levelPath.capacity());
    m_levelPath = levelPath;

    m_levelName = m_levelPath.mid(m_levelPath.lastIndexOf('/') + 1);

    // Store off if 
    if (QFileInfo(path + oldFileExtension).exists())
    {
        m_levelExtension = oldFileExtension;
    }
    else
    {
        m_levelExtension = defaultFileExtension;
    }

    QString relativeLevelPath = Path::GetRelativePath(path);
    if (gEnv->p3DEngine)
    {
        gEnv->p3DEngine->SetLevelPath(relativeLevelPath.toUtf8().data());
    }

    if (gEnv->pAISystem)
    {
        gEnv->pAISystem->SetLevelPath(relativeLevelPath.toUtf8().data());
    }
}

void CGameEngine::SetMissionName(const QString& mission)
{
    m_missionName = mission;
}

bool CGameEngine::LoadLevel(
    const QString& mission,
    bool bDeleteAIGraph,
    bool bReleaseResources)
{
    LOADING_TIME_PROFILE_SECTION(GetIEditor()->GetSystem());
    m_bLevelLoaded = false;
    m_missionName = mission;
    CLogFile::FormatLine("Loading map '%s' into engine...", m_levelPath.toUtf8().data());
    // Switch the current directory back to the Master CD folder first.
    // The engine might have trouble to find some files when the current
    // directory is wrong
    QDir::setCurrent(GetIEditor()->GetMasterCDFolder());

    QString pakFile = m_levelPath + "/level.pak";

    // Open Pak file for this level.
    if (!m_pISystem->GetIPak()->OpenPack(m_levelPath.toUtf8().data(), pakFile.toUtf8().data()))
    {
        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Level Pack File %s Not Found", pakFile.toUtf8().data());
    }

    // Initialize physics grid.
    if (bReleaseResources)
    {
        SSectorInfo si;

        GetIEditor()->GetHeightmap()->GetSectorsInfo(si);
        float terrainSize = si.sectorSize * si.numSectors;

        if (m_pISystem->GetIPhysicalWorld())
        {
            float fCellSize = terrainSize > 2048 ? terrainSize * (1.0f / 1024) : 2.0f;

            if (ICVar* pCvar = m_pISystem->GetIConsole()->GetCVar("e_PhysMinCellSize"))
            {
                fCellSize = max(fCellSize, (float)pCvar->GetIVal());
            }

            int log2PODGridSize = 0;

            if (fCellSize == 2.0f)
            {
                log2PODGridSize = 2;
            }
            else if (fCellSize == 4.0f)
            {
                log2PODGridSize = 1;
            }

            m_pISystem->GetIPhysicalWorld()->SetupEntityGrid(2, Vec3(0, 0, 0), terrainSize / fCellSize, terrainSize / fCellSize, fCellSize, fCellSize, log2PODGridSize);
        }

        // Resize proximity grid in entity system.
        if (bReleaseResources)
        {
            if (gEnv->pEntitySystem)
            {
                gEnv->pEntitySystem->ResizeProximityGrid(terrainSize, terrainSize);
            }
        }
    }

    // Load level in 3d engine.
    if (!gEnv->p3DEngine->InitLevelForEditor(m_levelPath.toUtf8().data(), m_missionName.toUtf8().data()))
    {
        CLogFile::WriteLine("ERROR: Can't load level !");
        QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("ERROR: Can't load level !"));
        return false;
    }

    // Audio: notify audio of level loading start?
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_REFRESH);

    if (gEnv->pAISystem && bDeleteAIGraph)
    {
        gEnv->pAISystem->FlushSystemNavigation();
    }

    m_bLevelLoaded = true;

    if (!bReleaseResources)
    {
        ReloadEnvironment();
    }

    if (GetIEditor()->GetMatFxGraphManager())
    {
        GetIEditor()->GetMatFxGraphManager()->ReloadFXGraphs();
    }

    return true;
}

bool CGameEngine::ReloadLevel()
{
    if (!LoadLevel(GetMissionName(), false, false))
    {
        return false;
    }

    return true;
}

bool CGameEngine::LoadAI(const QString& levelName, const QString& missionName)
{
    if (!gEnv->pAISystem)
    {
        return false;
    }

    if (!IsLevelLoaded())
    {
        return false;
    }

    float fStartTime = m_pISystem->GetITimer()->GetAsyncCurTime();
    CLogFile::FormatLine("Loading AI data %s, %s", levelName.toUtf8().data(), missionName.toUtf8().data());
    gEnv->pAISystem->FlushSystemNavigation();
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_CLEAR_AIGRAPH);
    gEnv->pAISystem->LoadLevelData(levelName.toUtf8().data(), missionName.toUtf8().data());
    CLogFile::FormatLine("Finished Loading AI data in %6.3f secs", m_pISystem->GetITimer()->GetAsyncCurTime() - fStartTime);

    return true;
}

bool CGameEngine::LoadMission(const QString& mission)
{
    if (!IsLevelLoaded())
    {
        return false;
    }

    if (mission != m_missionName)
    {
        m_missionName = mission;
        gEnv->p3DEngine->LoadMissionDataFromXMLNode(m_missionName.toUtf8().data());
    }

    return true;
}

bool CGameEngine::ReloadEnvironment()
{
    if (!gEnv->p3DEngine)
    {
        return false;
    }

    if (!IsLevelLoaded() && !m_bJustCreated)
    {
        return false;
    }

    if (!GetIEditor()->GetDocument())
    {
        return false;
    }

    XmlNodeRef env = XmlHelpers::CreateXmlNode("Environment");
    CXmlTemplate::SetValues(GetIEditor()->GetDocument()->GetEnvironmentTemplate(), env);

    // Notify mission that environment may be changed.
    GetIEditor()->GetDocument()->GetCurrentMission()->OnEnvironmentChange();
    //! Add lighting node to environment settings.
    GetIEditor()->GetDocument()->GetCurrentMission()->GetLighting()->Serialize(env, false);

    QString xmlStr = QString::fromLatin1(env->getXML());

    // Reload level data in engine.
    gEnv->p3DEngine->LoadEnvironmentSettingsFromXML(env);

    return true;
}

void CGameEngine::SwitchToInGame()
{
    if (gEnv->p3DEngine)
    {
        gEnv->p3DEngine->DisablePostEffects();
        gEnv->p3DEngine->ResetPostEffects();
    }

    GetIEditor()->Notify(eNotify_OnBeginGameMode);

    m_pISystem->SetThreadState(ESubsys_Physics, false);

    if (gEnv->p3DEngine)
    {
        gEnv->p3DEngine->ResetParticlesAndDecals();
    }

    CViewport* pGameViewport = GetIEditor()->GetViewManager()->GetGameViewport();

    if (GetIEditor()->GetObjectManager() && GetIEditor()->GetObjectManager()->GetLayersManager())
    {
        GetIEditor()->GetObjectManager()->GetLayersManager()->SetGameMode(true);
    }

    if (GetIEditor()->GetFlowGraphManager())
    {
        GetIEditor()->GetFlowGraphManager()->OnEnteringGameMode(true);
    }

    GetIEditor()->GetAI()->OnEnterGameMode(true);

    if (gEnv->pDynamicResponseSystem)
    {
        gEnv->pDynamicResponseSystem->Reset();
    }

    m_pISystem->GetIMovieSystem()->EnablePhysicsEvents(true);
    m_bInGameMode = true;

    if (m_pEditorGame)
    {
        m_pEditorGame->SetGameMode(true);
    }

    CRuler* pRuler = GetIEditor()->GetRuler();
    if (pRuler)
    {
        pRuler->SetActive(false);
    }

    GetIEditor()->GetAI()->SaveAndReloadActionGraphs();

    CCustomActionsEditorManager* pCustomActionsManager = GetIEditor()->GetCustomActionManager();
    if (pCustomActionsManager)
    {
        pCustomActionsManager->SaveAndReloadCustomActionGraphs();
    }

    gEnv->p3DEngine->GetTimeOfDay()->EndEditMode();
    HideLocalPlayer(false);
    if (m_pEditorGame)
    {
        m_pEditorGame->SetPlayerPosAng(m_playerViewTM.GetTranslation(), m_playerViewTM.TransformVector(FORWARD_DIRECTION));
    }
    gEnv->pSystem->GetViewCamera().SetMatrix(m_playerViewTM);

    IEntity* myPlayer = m_pEditorGame ? m_pEditorGame->GetPlayer() : nullptr;

    if (myPlayer)
    {
        myPlayer->InvalidateTM(ENTITY_XFORM_POS | ENTITY_XFORM_ROT);

        pe_player_dimensions dim;
        dim.heightEye = 0;
        if (myPlayer->GetPhysics())
        {
            myPlayer->GetPhysics()->GetParams(&dim);
        }

        if (pGameViewport)
        {
            myPlayer->SetPos(pGameViewport->GetViewTM().GetTranslation() - Vec3(0, 0, dim.heightEye));
            myPlayer->SetRotation(Quat(Ang3::GetAnglesXYZ(Matrix33(pGameViewport->GetViewTM()))));
        }
    }

    // Disable accelerators.
    GetIEditor()->EnableAcceleratos(false);
    // Reset physics state before switching to game.
    m_pISystem->GetIPhysicalWorld()->ResetDynamicEntities();
    // Reset mission script.
    GetIEditor()->GetDocument()->GetCurrentMission()->ResetScript();
    //! Send event to switch into game.
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_INGAME);

    // reset all agents in aisystem
    // (MATT) Needs to occur after ObjectManager because AI now needs control over AI enabling on start {2008/09/22}
    if (m_pISystem->GetAISystem())
    {
        m_pISystem->GetAISystem()->Reset(IAISystem::RESET_ENTER_GAME);
    }

    // When the player starts the game inside an area trigger, it will get
    // triggered.
    if (gEnv->pEntitySystem)
    {
        gEnv->pEntitySystem->ResetAreas();

        // Register in game entitysystem listener.
        s_InGameEntityListener = new SInGameEntitySystemListener;
        gEnv->pEntitySystem->AddSink(s_InGameEntityListener, IEntitySystem::OnSpawn | IEntitySystem::OnRemove, 0);

        SEntityEvent event;
        event.event = ENTITY_EVENT_RESET;
        event.nParam[0] = 1;
        gEnv->pEntitySystem->SendEventToAll(event);
        event.event = ENTITY_EVENT_LEVEL_LOADED;
        gEnv->pEntitySystem->SendEventToAll(event);
        event.event = ENTITY_EVENT_START_GAME;
        gEnv->pEntitySystem->SendEventToAll(event);
    }
    m_pISystem->GetIMovieSystem()->Reset(true, false);

    // Transition to runtime entity context.
    EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, StartPlayInEditor);

    // Constrain and hide the system cursor (important to do this last)
    AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                    &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                    AzFramework::SystemCursorState::ConstrainedAndHidden);
}

void CGameEngine::SwitchToInEditor()
{
    // Transition to editor entity context.
    EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, StopPlayInEditor);

    // Reset movie system
    for (int i = m_pISystem->GetIMovieSystem()->GetNumPlayingSequences(); --i >= 0;)
    {
        m_pISystem->GetIMovieSystem()->GetPlayingSequence(i)->Deactivate();
    }
    m_pISystem->GetIMovieSystem()->Reset(false, false);

    m_pISystem->SetThreadState(ESubsys_Physics, false);

    if (gEnv->p3DEngine)
    {
        // Reset 3d engine effects
        gEnv->p3DEngine->DisablePostEffects();
        gEnv->p3DEngine->ResetPostEffects();
        gEnv->p3DEngine->ResetParticlesAndDecals();
    }

    CViewport* pGameViewport = GetIEditor()->GetViewManager()->GetGameViewport();

    if (GetIEditor()->GetObjectManager() && GetIEditor()->GetObjectManager()->GetLayersManager())
    {
        GetIEditor()->GetObjectManager()->GetLayersManager()->SetGameMode(false);
    }

    if (GetIEditor()->GetFlowGraphManager())
    {
        GetIEditor()->GetFlowGraphManager()->OnEnteringGameMode(false);
    }

    GetIEditor()->GetAI()->OnEnterGameMode(false);

    m_pISystem->GetIMovieSystem()->EnablePhysicsEvents(m_bSimulationMode);
    gEnv->p3DEngine->GetTimeOfDay()->BeginEditMode();

    // reset all agents in aisystem
    if (m_pISystem->GetAISystem())
    {
        m_pISystem->GetAISystem()->Reset(IAISystem::RESET_EXIT_GAME);
    }

    // this has to be done before the RemoveSink() call, or else some entities may not be removed
    gEnv->p3DEngine->GetDeferredPhysicsEventManager()->ClearDeferredEvents();

    // Unregister ingame entitysystem listener, and kill all remaining entities.
    if (s_InGameEntityListener != nullptr)
    {
        gEnv->pEntitySystem->RemoveSink(s_InGameEntityListener);
        delete s_InGameEntityListener;
        s_InGameEntityListener = nullptr;
    }

    // Enable accelerators.
    GetIEditor()->EnableAcceleratos(true);
    // Reset mission script.
    GetIEditor()->GetDocument()->GetCurrentMission()->ResetScript();


    // reset movie-system
    m_pISystem->GetIPhysicalWorld()->ResetDynamicEntities();

    // reset UI system
    if (gEnv->pLyShine)
    {
        gEnv->pLyShine->Reset();
    }

    SEntityEvent event;
    event.event = ENTITY_EVENT_RESET;
    //When we switch back to editor from game, we are checking the value of simulation mode and
    //based on its value,we are setting the event parameter appropriately
    if (GetSimulationMode())
    {
        event.nParam[0] = 1;
    }
    else
    {
        event.nParam[0] = 0;
    }
    if (gEnv->pEntitySystem)
    {
        gEnv->pEntitySystem->SendEventToAll(event);
    }

    // [Anton] - order changed, see comments for CGameEngine::SetSimulationMode
    //! Send event to switch out of game.
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_OUTOFGAME);

    // Hide all drawing of character.
    HideLocalPlayer(true);
    m_bInGameMode = false;
    if (m_pEditorGame)
    {
        m_pEditorGame->SetGameMode(false);
    }

    IEntity* myPlayer = m_pEditorGame ? m_pEditorGame->GetPlayer() : nullptr;
    if (myPlayer)
    {
        // Move the camera to the entity position so it doesn't shift backward each time you drop in.
        CCamera& cam = gEnv->pSystem->GetViewCamera();
        Vec3 pos = myPlayer->GetPos();
        pos.z = cam.GetPosition().z;
        cam.SetPosition(pos);

        // Restore the previous editor camera.
        myPlayer->SetWorldTM(m_playerViewTM);
    }

    // Out of game in Editor mode.
    if (pGameViewport)
    {
        pGameViewport->SetViewTM(m_playerViewTM);
    }


    GetIEditor()->Notify(eNotify_OnEndGameMode);

    // Unconstrain the system cursor and make it visible (important to do this last)
    AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                    &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                    AzFramework::SystemCursorState::UnconstrainedAndVisible);
}

void CGameEngine::HandleQuitRequest(IConsoleCmdArgs* /*args*/)
{
    if (GetIEditor()->GetGameEngine()->IsInGameMode())
    {
        GetIEditor()->GetGameEngine()->RequestSetGameMode(false);
        gEnv->pConsole->ShowConsole(false);
    }
    else
    {
        MainWindow::instance()->GetActionManager()->GetAction(ID_APP_EXIT)->trigger();
    }
}

void CGameEngine::RequestSetGameMode(bool inGame)
{
    m_ePendingGameMode = inGame ? ePGM_SwitchToInGame : ePGM_SwitchToInEditor;

    if (m_ePendingGameMode == ePGM_SwitchToInGame)
    {
        EBUS_EVENT(LmbrCentral::SimpleAnimationComponentRequestBus, StartDefaultAnimations);
    }
    else if (m_ePendingGameMode == ePGM_SwitchToInEditor)
    {
        EBUS_EVENT(LmbrCentral::SimpleAnimationComponentRequestBus, StopAllAnimations);
    }
}

void CGameEngine::SetGameMode(bool bInGame)
{
    if (m_bInGameMode == bInGame)
    {
        return;
    }

    if (!GetIEditor()->GetDocument())
    {
        return;
    }

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_GAME_MODE_SWITCH_START, bInGame, 0);

    // Enables engine to know about that.
    gEnv->SetIsEditorGameMode(bInGame);

    // Ignore updates while changing in and out of game mode
    m_bIgnoreUpdates = true;
    LockResources();

    // Switching modes will destroy the current AzFramework::EntityConext which may contain
    // data the queued events hold on to, so execute all queued events before switching.
    ExecuteQueuedEvents();

    if (bInGame)
    {
        SwitchToInGame();
    }
    else
    {
        SwitchToInEditor();
    }

    GetIEditor()->GetObjectManager()->SendEvent(EVENT_PHYSICS_APPLYSTATE);

    // Enables engine to know about that.
    if (MainWindow::instance())
    {
        AzFramework::InputChannelRequestBus::Broadcast(&AzFramework::InputChannelRequests::ResetState);
        MainWindow::instance()->setFocus();
    }

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED, bInGame, 0);

    UnlockResources();
    m_bIgnoreUpdates = false;

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_GAME_MODE_SWITCH_END, bInGame, 0);
}

void CGameEngine::SetSimulationMode(bool enabled, bool bOnlyPhysics)
{
    if (m_bSimulationMode == enabled)
    {
        return;
    }

    m_pISystem->GetIMovieSystem()->EnablePhysicsEvents(enabled);

    if (!bOnlyPhysics)
    {
        LockResources();
    }

    if (enabled)
    {
        CRuler* pRuler = GetIEditor()->GetRuler();
        if (pRuler)
        {
            pRuler->SetActive(false);
        }

        GetIEditor()->Notify(eNotify_OnBeginSimulationMode);

        if (!bOnlyPhysics)
        {
            GetIEditor()->GetAI()->SaveAndReloadActionGraphs();

            CCustomActionsEditorManager* pCustomActionsManager = GetIEditor()->GetCustomActionManager();
            if (pCustomActionsManager)
            {
                pCustomActionsManager->SaveAndReloadCustomActionGraphs();
            }
        }
    }
    else
    {
        GetIEditor()->Notify(eNotify_OnEndSimulationMode);
    }

    m_bSimulationMode = enabled;

    // Enables engine to know about simulation mode.
    gEnv->SetIsEditorSimulationMode(enabled);

    m_pISystem->SetThreadState(ESubsys_Physics, false);

    if (!bOnlyPhysics)
    {
        m_bSimulationModeAI = enabled;
    }

    if (m_bSimulationMode)
    {
        if (!bOnlyPhysics)
        {
            if (m_pISystem->GetI3DEngine())
            {
                m_pISystem->GetI3DEngine()->ResetPostEffects();
            }

            // make sure FlowGraph is initialized
            if (m_pISystem->GetIFlowSystem())
            {
                m_pISystem->GetIFlowSystem()->Reset(false);
            }

            GetIEditor()->SetConsoleVar("ai_ignoreplayer", 1);
            //GetIEditor()->SetConsoleVar( "ai_soundperception",0 );
        }

        // [Anton] the order of the next 3 calls changed, since, EVENT_INGAME loads physics state (if any),
        // and Reset should be called before it
        m_pISystem->GetIPhysicalWorld()->ResetDynamicEntities();
        GetIEditor()->GetObjectManager()->SendEvent(EVENT_INGAME);

        if (!bOnlyPhysics && m_pISystem->GetAISystem())
        {
            // (MATT) Had to move this here because AI system now needs control over entities enabled on start {2008/09/22}
            // When turning physics on, emulate out of game event.
            m_pISystem->GetAISystem()->Reset(IAISystem::RESET_ENTER_GAME);
        }

        // Register in game entitysystem listener.
        if (gEnv->pEntitySystem)
        {
            s_InGameEntityListener = new SInGameEntitySystemListener;
            gEnv->pEntitySystem->AddSink(s_InGameEntityListener, IEntitySystem::OnSpawn | IEntitySystem::OnRemove, 0);
        }
    }
    else
    {
        if (!bOnlyPhysics)
        {
            GetIEditor()->SetConsoleVar("ai_ignoreplayer", 0);
            //GetIEditor()->SetConsoleVar( "ai_soundperception",1 );

            if (m_pISystem->GetI3DEngine())
            {
                m_pISystem->GetI3DEngine()->ResetPostEffects();
            }

            // When turning physics off, emulate out of game event.
            if (m_pISystem->GetAISystem())
            {
                m_pISystem->GetAISystem()->Reset(IAISystem::RESET_EXIT_GAME);
            }
        }

        m_pISystem->GetIPhysicalWorld()->ResetDynamicEntities();

        GetIEditor()->GetObjectManager()->SendEvent(EVENT_OUTOFGAME);


        // Unregister ingame entitysystem listener, and kill all remaining entities.
        if (gEnv->pEntitySystem)
        {
            gEnv->pEntitySystem->RemoveSink(s_InGameEntityListener);
            delete s_InGameEntityListener;
            s_InGameEntityListener = 0;
        }
    }

    if (!bOnlyPhysics && gEnv->pEntitySystem)
    {
        SEntityEvent event;
        event.event = ENTITY_EVENT_RESET;
        event.nParam[0] = (UINT_PTR)enabled;
        gEnv->pEntitySystem->SendEventToAll(event);
    }

    GetIEditor()->GetObjectManager()->SendEvent(EVENT_PHYSICS_APPLYSTATE);

    // Execute all queued events before switching modes.
    ExecuteQueuedEvents();

    // Transition back to editor entity context.
    // Symmetry is not critical. It's okay to call this even if we never called StartPlayInEditor
    // (bOnlyPhysics was true when we entered simulation mode).
    AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::StopPlayInEditor);

    if (m_bSimulationMode && !bOnlyPhysics)
    {
        if (gEnv->pEntitySystem)
        {
            SEntityEvent event;
            event.event = ENTITY_EVENT_LEVEL_LOADED;
            gEnv->pEntitySystem->SendEventToAll(event);

            event.event = ENTITY_EVENT_START_GAME;
            gEnv->pEntitySystem->SendEventToAll(event);
        }

        // Transition to runtime entity context.
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::StartPlayInEditor);
    }

    if (m_pISystem->GetIGame() && m_pISystem->GetIGame()->GetIGameFramework())
    {
        m_pISystem->GetIGame()->GetIGameFramework()->OnEditorSetGameMode(enabled + 2);
    }

    if (!bOnlyPhysics)
    {
        UnlockResources();
    }

    AzFramework::InputChannelRequestBus::Broadcast(&AzFramework::InputChannelRequests::ResetState);
}

CNavigation* CGameEngine::GetNavigation()
{
    return m_pNavigation;
}

void CGameEngine::LoadAINavigationData()
{
    if (!gEnv->pAISystem)
    {
        return;
    }

    CWaitProgress wait("Loading AI Navigation");

    // Inform AiPoints that their GraphNodes are invalid (so release references to them)
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_CLEAR_AIGRAPH);
    CLogFile::FormatLine("Loading AI navigation data for Level:%s Mission:%s", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    gEnv->pAISystem->FlushSystemNavigation();
    gEnv->pAISystem->LoadNavigationData(m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    // Inform AiPoints that they need to recreate GraphNodes for navigation NEEDED??
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_ATTACH_AIPOINTS);
}

void CGameEngine::GenerateAiAll()
{
    if (!gEnv->pAISystem)
    {
        return;
    }

    CWaitProgress wait("Generating All AI Annotations");

    wait.Step(1);

    // Inform AiPoints to release their GraphNodes as FlushSystemNavigation() clears all.
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_CLEAR_AIGRAPH);
    // TOxDO Mrz 20, 2008: <pvl> flush editor nav too?
    // UPDATE Aug 5, 2008: <pvl> yes, I'd say
    gEnv->pAISystem->FlushSystemNavigation();
    //m_pNavigation->FlushSystemNavigation();
    wait.Step(5);

    char fileName[1024];
    azsnprintf(fileName, 1024, "%s/net%s.bai", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    AILogProgress(" Now writing to %s.", fileName);
    m_pNavigation->GetGraph()->WriteToFile(fileName);
    wait.Step(60);

    // FIXxME Mrz 18, 2008: <pvl> should we just call ExportAIGraph() from here?
    // UPDATE Mrz 20, 2008: <pvl> there's no need actually - our caller
    // CCryEditApp::OnAiGenerateAll() calls that immediately after
    // this function returns
    // NOTE Mrz 20, 2008: <pvl> interestingly enough, road navigation gets
    // processed as part of ReconnectAllWaypointNodes() but it's not written
    // out there - let's do it here until I can find a better place
    // TODO Mrz 20, 2008: <pvl> it might be best that we invoke roads processing
    // from here as well and add the saving to it so that road nav region saves
    // itself after it's finished its preprocessing just as other regions do?
    char fileNameRoads[1024];
    azsnprintf(fileNameRoads, 1024, "%s\\roadnav%s.bai", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    m_pNavigation->ExportData(0, 0, fileNameRoads, 0, 0, 0);

    QString fileNameAreas;
    fileNameAreas = QStringLiteral("%1/areas%2.bai").arg(m_levelPath, m_missionName);
    m_pNavigation->WriteAreasIntoFile(fileNameAreas.toUtf8().data());

    wait.Step(65);
    GetIEditor()->GetGameEngine()->GenerateAICoverSurfaces();

    wait.Step(75);
    GenerateAINavigationMesh();
    wait.Step(85);
    gEnv->pAISystem->LoadNavigationData(m_levelPath.toUtf8().data(), m_missionName.toUtf8().data(), false, true);
    wait.Step(95);
    CLogFile::FormatLine("Validating SmartObjects");

    // quick temp code to validate smart objects on level export
    {
        if (CObjectClassDesc* pClass = GetIEditor()->GetObjectManager()->FindClass("SmartObject"))
        {
            QString error;
            CBaseObjectsArray objects;
            GetIEditor()->GetObjectManager()->GetObjects(objects);

            CBaseObjectsArray::iterator it, itEnd = objects.end();
            for (it = objects.begin(); it != itEnd; ++it)
            {
                CBaseObject* pBaseObject = *it;
                if (pBaseObject->GetClassDesc() == pClass)
                {
                    CSmartObject* pSOEntity = (CSmartObject*)pBaseObject;

                    if (!gEnv->pAISystem->GetSmartObjectManager()->ValidateSOClassTemplate(pSOEntity->GetIEntity()))
                    {
                        const Vec3 pos = pSOEntity->GetWorldPos();

                        IErrorReport* errorReport = GetIEditor()->GetErrorReport();

                        error = QStringLiteral("SmartObject '%1' at (%2, %3, %4) is invalid!").arg(pSOEntity->GetName())
                            .arg(pos.x, 0, 'f', 2).arg(pos.y, 0, 'f', 2).arg(pos.z, 0, 'f', 2);
                        CErrorRecord err(pSOEntity, CErrorRecord::ESEVERITY_WARNING, error.toUtf8().data());
                        errorReport->ReportError(err);
                    }
                }
            }
        }
    }

    wait.Step(100);
}

void CGameEngine::GenerateAiTriangulation()
{
    if (!gEnv->pAISystem)
    {
        return;
    }

    CWaitProgress wait("Generating AI Triangulation");

    // Inform AiPoints to release their GraphNodes as FlushSystemNavigation() clears all.
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_CLEAR_AIGRAPH);
    m_pNavigation->FlushSystemNavigation();

    CLogFile::FormatLine("Generating Triangulation for Level:%s Mission:%s", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    m_pNavigation->GenerateTriangulation(m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());

    // Inform AiPoints that they need to recreate GraphNodes ready for graph export
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_ATTACH_AIPOINTS);

    CLogFile::FormatLine("Generating Waypoints for Level:%s Mission:%s", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    m_pNavigation->ReconnectAllWaypointNodes();

    char fileName[1024];
    azsnprintf(fileName, 1024, "%s/net%s.bai", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    AILogProgress(" Now writing to %s.", fileName);
    m_pNavigation->GetGraph()->WriteToFile(fileName);

    QString fileNameAreas;
    fileNameAreas = QStringLiteral("%1/areas%2.bai").arg(m_levelPath, m_missionName);
    m_pNavigation->WriteAreasIntoFile(fileNameAreas.toUtf8().data());

    gEnv->pAISystem->LoadNavigationData(m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
}

void CGameEngine::GenerateAiWaypoint()
{
    if (!gEnv->pAISystem)
    {
        return;
    }

    CWaitProgress wait("Generating AI Waypoints");

    // Inform AiPoints that they need to create GraphNodes (if they haven't already) ready for graph export
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_ATTACH_AIPOINTS);

    CLogFile::FormatLine("Generating Waypoints for Level:%s Mission:%s", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    m_pNavigation->ReconnectAllWaypointNodes();

    char fileName[1024];
    azsnprintf(fileName, 1024, "%s/net%s.bai", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    AILogProgress(" Now writing to %s.", fileName);
    m_pNavigation->GetGraph()->WriteToFile(fileName);

    QString fileNameAreas;
    fileNameAreas = QStringLiteral("%1/areas%2.bai").arg(m_levelPath, m_missionName);
    m_pNavigation->WriteAreasIntoFile(fileNameAreas.toUtf8().data());

    gEnv->pAISystem->LoadNavigationData(m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
}

void CGameEngine::GenerateAiFlightNavigation()
{
    if (!gEnv->pAISystem)
    {
        return;
    }

    CWaitProgress wait("Generating AI Flight navigation");

    m_pNavigation->GetFlightNavRegion()->Clear();

    CLogFile::FormatLine("Generating Flight navigation for Level:%s Mission:%s", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    m_pNavigation->GenerateFlightNavigation(m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());

    char fileName[1024];
    azsnprintf(fileName, 1024, "%s/net%s.bai", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    AILogProgress(" Now writing to %s.", fileName);
    m_pNavigation->GetGraph()->WriteToFile(fileName);

    QString fileNameAreas;
    fileNameAreas = QStringLiteral("%1/areas%2.bai").arg(m_levelPath, m_missionName);
    m_pNavigation->WriteAreasIntoFile(fileNameAreas.toUtf8().data());

    gEnv->pAISystem->LoadNavigationData(m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
}

void CGameEngine::GenerateAiNavVolumes()
{
    if (!gEnv->pAISystem)
    {
        return;
    }

    CWaitProgress wait("Generating AI 3D navigation volumes");

    m_pNavigation->FlushSystemNavigation();

    CLogFile::FormatLine("Generating 3D navigation volumes for Level:%s Mission:%s", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    m_pNavigation->Generate3DVolumes(m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());

    char fileName[1024];
    azsnprintf(fileName, 1024, "%s/net%s.bai", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    AILogProgress(" Now writing to %s.", fileName);
    m_pNavigation->GetGraph()->WriteToFile(fileName);

    QString fileNameAreas;
    fileNameAreas = QStringLiteral("%1/areas%2.bai").arg(m_levelPath, m_missionName);
    m_pNavigation->WriteAreasIntoFile(fileNameAreas.toUtf8().data());

    gEnv->pAISystem->LoadNavigationData(m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
}

void CGameEngine::GenerateAINavigationMesh()
{
    if (!gEnv->pAISystem)
    {
        return;
    }

    CWaitProgress wait("Generating AI MNM navigation meshes");
    QString mnmFilename;

    gEnv->pAISystem->GetNavigationSystem()->ProcessQueuedMeshUpdates();

    mnmFilename = QStringLiteral("%1/mnmnav%2.bai").arg(m_levelPath, m_missionName);
    gEnv->pAISystem->GetNavigationSystem()->SaveToFile(mnmFilename.toUtf8().data());
}

void CGameEngine::ValidateAINavigation()
{
    if (!gEnv->pAISystem)
    {
        return;
    }

    CWaitProgress wait("Validating AI navigation");

    if (m_pNavigation->GetGraph())
    {
        if (m_pNavigation->GetGraph()->Validate("", true))
        {
            CLogFile::FormatLine("AI navigation validation OK");
        }
        else
        {
            CLogFile::FormatLine("AI navigation validation failed");
        }
    }
    else
    {
        CLogFile::FormatLine("No AI graph - cannot validate");
    }

    m_pNavigation->ValidateNavigation();
}

void CGameEngine::ClearAllAINavigation()
{
    if (!gEnv->pAISystem)
    {
        return;
    }

    CWaitProgress wait("Clearing all AI navigation data");

    wait.Step(1);
    // Inform AiPoints to release their GraphNodes.
    GetIEditor()->GetObjectManager()->SendEvent(EVENT_CLEAR_AIGRAPH);
    // Flush old navigation data
    gEnv->pAISystem->FlushSystemNavigation();
    m_pNavigation->FlushSystemNavigation();
    wait.Step(20);

    // Ensure file data is all cleared (prevents confusion if the old data were to be reloaded implicitly)
    char fileName[1024];
    azsnprintf(fileName, 1024, "%s/net%s.bai", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    AILogProgress(" Now writing empty data to %s.", fileName);
    m_pNavigation->GetGraph()->WriteToFile(fileName);
    wait.Step(60);
    gEnv->pAISystem->LoadNavigationData(m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    wait.Step(100);
}

void CGameEngine::Generate3DDebugVoxels()
{
    CWaitProgress wait("Generating 3D debug voxels");

    m_pNavigation->Generate3DDebugVoxels();
    CLogFile::FormatLine("Use ai_debugdrawvolumevoxels to view more/fewer voxels");

    char fileName[1024];
    azsnprintf(fileName, 1024, "%s/net%s.bai", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    AILogProgress(" Now writing to %s.", fileName);
    m_pNavigation->GetGraph()->WriteToFile(fileName);
    gEnv->pAISystem->LoadNavigationData(m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
}

void CGameEngine::GenerateAICoverSurfaces()
{
    if (!gEnv->pAISystem)
    {
        return;
    }

    CCoverSurfaceManager* coverSurfaceManager = GetIEditor()->GetAI()->GetCoverSurfaceManager();
    const CCoverSurfaceManager::SurfaceObjects& surfaceObjects = coverSurfaceManager->GetSurfaceObjects();
    uint32 surfaceObjectCount = surfaceObjects.size();

    if (surfaceObjectCount)
    {
        {
            CWaitProgress wait("Generating AI Cover Surfaces");

            wait.Step(0);

            float stepInc = 100.5f / (float)surfaceObjectCount;
            float step = 0.0f;

            GetIEditor()->GetAI()->GetCoverSurfaceManager()->ClearGameSurfaces();
            CCoverSurfaceManager::SurfaceObjects::const_iterator it = surfaceObjects.begin();
            CCoverSurfaceManager::SurfaceObjects::const_iterator end = surfaceObjects.end();

            for (; it != end; ++it)
            {
                CAICoverSurface* coverSurfaceObject = *it;
                coverSurfaceObject->Generate();
                step += stepInc;
                wait.Step((uint32)step);
            }
        }

        {
            CWaitProgress wait("Validating AI Cover Surfaces");

            wait.Step(0);

            float stepInc = 100.5f / (float)surfaceObjectCount;
            float step = 0.0f;

            CCoverSurfaceManager::SurfaceObjects::const_iterator it = surfaceObjects.begin();
            CCoverSurfaceManager::SurfaceObjects::const_iterator end = surfaceObjects.end();

            for (; it != end; ++it)
            {
                CAICoverSurface* coverSurfaceObject = *it;
                coverSurfaceObject->ValidateGenerated();

                step += stepInc;
                wait.Step((uint32)step);
            }
        }
    }

    char fileName[1024];
    azsnprintf(fileName, 1024, "%s\\cover%s.bai", m_levelPath.toUtf8().data(), m_missionName.toUtf8().data());
    AILogProgress("Now writing to %s.", fileName);
    GetIEditor()->GetAI()->GetCoverSurfaceManager()->WriteToFile(fileName);
}

void CGameEngine::ExportAiData(
    const char* navFileName,
    const char* areasFileName,
    const char* roadsFileName,
    const char* vertsFileName,
    const char* volumeFileName,
    const char* flightFileName)
{
    m_pNavigation->ExportData(
        navFileName,
        areasFileName,
        roadsFileName,
        vertsFileName,
        volumeFileName,
        flightFileName);
}

void CGameEngine::ResetResources()
{
    if (gEnv->pEntitySystem)
    {
        {
            /// Delete all entities.
            gEnv->pEntitySystem->Reset();
        }

        // In editor we are at the same time client and server.
        gEnv->bServer = true;
        gEnv->bMultiplayer = false;
        gEnv->bHostMigrating = false;
        gEnv->SetIsClient(true);
    }

    if (gEnv->p3DEngine)
    {
        gEnv->p3DEngine->UnloadLevel();
    }

    if (gEnv->pPhysicalWorld)
    {
        // Initialize default entity grid in physics
        m_pISystem->GetIPhysicalWorld()->SetupEntityGrid(2, Vec3(0, 0, 0), 128, 128, 4, 4, 1);
    }
}

void CGameEngine::SetPlayerEquipPack(const char* sEqipPackName)
{
    //TODO: remove if not used, mission related
}

void CGameEngine::SetPlayerViewMatrix(const Matrix34& tm, bool bEyePos)
{
    m_playerViewTM = tm;

    if (!m_pEditorGame)
    {
        return;
    }

    if (m_bSyncPlayerPosition)
    {
        const Vec3 oPosition(m_playerViewTM.GetTranslation());
        const Vec3 oDirection(m_playerViewTM.TransformVector(FORWARD_DIRECTION));
        m_pEditorGame->SetPlayerPosAng(oPosition, oDirection);
    }
}

void CGameEngine::HideLocalPlayer(bool bHide)
{
    if (m_pEditorGame)
    {
        m_pEditorGame->HidePlayer(bHide);
    }
}

void CGameEngine::SyncPlayerPosition(bool bEnable)
{
    m_bSyncPlayerPosition = bEnable;

    if (m_bSyncPlayerPosition)
    {
        SetPlayerViewMatrix(m_playerViewTM);
    }
}

void CGameEngine::SetCurrentMOD(const char* sMod)
{
    m_MOD = sMod;
}

QString CGameEngine::GetCurrentMOD() const
{
    return m_MOD;
}

void CGameEngine::Update()
{
    if (m_bIgnoreUpdates)
    {
        return;
    }

    switch (m_ePendingGameMode)
    {
    case ePGM_SwitchToInGame:
    {
        SetGameMode(true);
        m_ePendingGameMode = ePGM_NotPending;
        break;
    }

    case ePGM_SwitchToInEditor:
    {
        bool wasInSimulationMode = GetIEditor()->GetGameEngine()->GetSimulationMode();
        if (wasInSimulationMode)
        {
            GetIEditor()->GetGameEngine()->SetSimulationMode(false);
        }
        SetGameMode(false);
        if (wasInSimulationMode)
        {
            GetIEditor()->GetGameEngine()->SetSimulationMode(true);
        }
        m_ePendingGameMode = ePGM_NotPending;
        break;
    }
    }

    AZ::ComponentApplication* componentApplication = nullptr;
    EBUS_EVENT_RESULT(componentApplication, AZ::ComponentApplicationBus, GetApplication);

    if (m_bInGameMode)
    {
        CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();

        // if we're in editor mode, match the width, height and Fov, but alter no other parameters
        if (pRenderViewport)
        {
            int width = 640;
            int height = 480;
            pRenderViewport->GetDimensions(&width, &height);

            // Check for custom width and height cvars in use by Track View.
            // The backbuffer size maybe have been changed, so we need to make sure the viewport
            // is setup with the correct aspect ratio here so the captured output will look correct.
            ICVar* cVar = gEnv->pConsole->GetCVar("TrackViewRenderOutputCapturing");
            if (cVar && cVar->GetIVal() != 0)
            {
                const int customWidth = gEnv->pConsole->GetCVar("r_CustomResWidth")->GetIVal();
                const int customHeight = gEnv->pConsole->GetCVar("r_CustomResHeight")->GetIVal();
                if (customWidth != 0 && customHeight != 0)
                {
                    IEditor* editor = GetIEditor();
                    AZ_Assert(editor, "Expected valid Editor");
                    IRenderer* renderer = editor->GetRenderer();
                    AZ_Assert(renderer, "Expected valid Renderer");

                    int maxRes = renderer->GetMaxSquareRasterDimension();
                    width = clamp_tpl(customWidth, 32, maxRes);
                    height = clamp_tpl(customHeight, 32, maxRes);
                }
            }

            CCamera& cam = gEnv->pSystem->GetViewCamera();
            cam.SetFrustum(width, height, pRenderViewport->GetFOV(), cam.GetNearPlane(), cam.GetFarPlane(), cam.GetPixelAspectRatio());
        }

        if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
        {
            gEnv->pGame->GetIGameFramework()->PreUpdate(true, 0);
            componentApplication->Tick(gEnv->pTimer->GetFrameTime(ITimer::ETIMER_GAME));
            gEnv->pGame->GetIGameFramework()->PostUpdate(true, 0);
        }
        else if (gEnv->pSystem)
        {
            gEnv->pSystem->UpdatePreTickBus();
            componentApplication->Tick(gEnv->pTimer->GetFrameTime(ITimer::ETIMER_GAME));
            gEnv->pSystem->UpdatePostTickBus();
        }

        // TODO: still necessary after AVI recording removal?
        if (pRenderViewport)
        {
            // Make sure we at least try to update game viewport (Needed for AVI recording).
            pRenderViewport->Update();
        }
    }
    else
    {
        // [marco] check current sound and vis areas for music etc.
        // but if in game mode, 'cos is already done in the above call to game->update()
        unsigned int updateFlags = ESYSUPDATE_EDITOR;

        CRuler* pRuler = GetIEditor()->GetRuler();
        const bool bRulerNeedsUpdate = (pRuler && pRuler->HasQueuedPaths());

        if (!m_bSimulationMode)
        {
            updateFlags |= ESYSUPDATE_IGNORE_PHYSICS;
        }

        if (!m_bSimulationModeAI && !bRulerNeedsUpdate)
        {
            updateFlags |= ESYSUPDATE_IGNORE_AI;
        }

        bool bUpdateAIPhysics = GetSimulationMode() || m_bUpdateFlowSystem;

        if (bUpdateAIPhysics)
        {
            updateFlags |= ESYSUPDATE_EDITOR_AI_PHYSICS;
        }

        GetIEditor()->GetAnimation()->Update();
        GetIEditor()->GetSystem()->UpdatePreTickBus(updateFlags);
        componentApplication->Tick(gEnv->pTimer->GetFrameTime(ITimer::ETIMER_GAME));
        GetIEditor()->GetSystem()->UpdatePostTickBus(updateFlags);

        // Update flow system in simulation mode.
        if (bUpdateAIPhysics)
        {
            IFlowSystem* pFlowSystem = GetIFlowSystem();

            if (pFlowSystem)
            {
                pFlowSystem->Update();
            }

            IDialogSystem* pDialogSystem = gEnv->pGame ? (gEnv->pGame->GetIGameFramework() ? gEnv->pGame->GetIGameFramework()->GetIDialogSystem() : NULL) : NULL;

            if (pDialogSystem)
            {
                pDialogSystem->Update(gEnv->pTimer->GetFrameTime());
            }
        }
        else if (GetIEditor()->GetAI()->GetNavigationContinuousUpdateState())
        {
            GetIEditor()->GetAI()->NavigationContinuousUpdate();
        }

        GetIEditor()->GetAI()->NavigationDebugDisplay();
    }

    EBUS_EVENT(AzToolsFramework::AssetSystemRequestBus, UpdateQueuedEvents);

    m_pNavigation->Update();
}

void CGameEngine::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginNewScene:
    case eNotify_OnBeginSceneOpen:
    {
        ResetResources();
        if (m_pEditorGame)
        {
            m_pEditorGame->OnBeforeLevelLoad();
        }
    }
    break;
    case eNotify_OnBeginTerrainRebuild:
    {
        if (m_pEditorGame)
        {
            m_pEditorGame->OnBeforeLevelLoad();
        }
    }
    break;
    case eNotify_OnEndSceneOpen:
    case eNotify_OnEndTerrainRebuild:
    {
        if (m_pEditorGame)
        {
            // This method must be called so we will have a player to start game mode later.
            m_pEditorGame->OnAfterLevelLoad(m_levelName.toUtf8().data(), m_levelPath.toUtf8().data());
        }
    }
    case eNotify_OnEndNewScene: // intentional fall-through?
    {
        if (m_pEditorGame)
        {
            m_pEditorGame->OnAfterLevelInit(m_levelName.toUtf8().data(), m_levelPath.toUtf8().data());
            HideLocalPlayer(true);
            SetPlayerViewMatrix(m_playerViewTM);     // Sync initial player position
        }

        if (gEnv->p3DEngine)
        {
            gEnv->p3DEngine->PostLoadLevel();
        }
    }
    break;
    case eNotify_OnCloseScene:
    {
        if (m_pEditorGame)
        {
            m_pEditorGame->OnCloseLevel();
        }
    }
    break;
    case eNotify_OnSplashScreenDestroyed:
    {
        if (m_pSystemUserCallback != NULL)
        {
            m_pSystemUserCallback->OnSplashScreenDone();
        }
    }
    break;
    }
}

IEntity* CGameEngine::GetPlayerEntity()
{
    if (m_pEditorGame)
    {
        return m_pEditorGame->GetPlayer();
    }

    return 0;
}

IFlowSystem* CGameEngine::GetIFlowSystem() const
{
    if (m_pEditorGame)
    {
        return m_pEditorGame->GetIFlowSystem();
    }

    return NULL;
}

IGameTokenSystem* CGameEngine::GetIGameTokenSystem() const
{
    if (m_pEditorGame)
    {
        return m_pEditorGame->GetIGameTokenSystem();
    }

    return NULL;
}

IEquipmentSystemInterface* CGameEngine::GetIEquipmentSystemInterface() const
{
    if (m_pEditorGame)
    {
        return m_pEditorGame->GetIEquipmentSystemInterface();
    }

    return NULL;
}

void CGameEngine::LockResources()
{
    gEnv->p3DEngine->LockCGFResources();
}

void CGameEngine::UnlockResources()
{
    gEnv->p3DEngine->UnlockCGFResources();
}

bool CGameEngine::SupportsMultiplayerGameRules()
{
    return m_pEditorGame ? m_pEditorGame->SupportsMultiplayerGameRules() : false;
}

void CGameEngine::ToggleMultiplayerGameRules()
{
    if (m_pEditorGame)
    {
        m_pEditorGame->ToggleMultiplayerGameRules();
    }
}

bool CGameEngine::BuildEntitySerializationList(XmlNodeRef output)
{
    return m_pEditorGame ? m_pEditorGame->BuildEntitySerializationList(output) : true;
}

void CGameEngine::OnTerrainModified(const Vec2& modPosition, float modAreaRadius, bool fullTerrain)
{
    INavigationSystem* pNavigationSystem = gEnv->pAISystem ? gEnv->pAISystem->GetNavigationSystem() : nullptr;

    if (pNavigationSystem)
    {
        // Only report local modifications, not a change in the full terrain (probably happening during initialization)
        if (fullTerrain == false)
        {
            const Vec2 offset(modAreaRadius * 1.5f, modAreaRadius * 1.5f);
            AABB updateBox;
            updateBox.min = modPosition - offset;
            updateBox.max = modPosition + offset;
            const float terrainHeight1 = gEnv->p3DEngine->GetTerrainElevation(updateBox.min.x, updateBox.min.y);
            const float terrainHeight2 = gEnv->p3DEngine->GetTerrainElevation(updateBox.max.x, updateBox.max.y);
            const float terrainHeight3 = gEnv->p3DEngine->GetTerrainElevation(modPosition.x, modPosition.y);

            updateBox.min.z = min(terrainHeight1, min(terrainHeight2, terrainHeight3)) - (modAreaRadius * 2.0f);
            updateBox.max.z = max(terrainHeight1, max(terrainHeight2, terrainHeight3)) + (modAreaRadius * 2.0f);
            pNavigationSystem->WorldChanged(updateBox);
        }
    }
}

void CGameEngine::OnAreaModified(const AABB& modifiedArea)
{
    INavigationSystem* pNavigationSystem = gEnv->pAISystem ? gEnv->pAISystem->GetNavigationSystem() : nullptr;
    if (pNavigationSystem)
    {
        pNavigationSystem->WorldChanged(modifiedArea);
    }
}

void CGameEngine::ExecuteQueuedEvents()
{
    AZ::Data::AssetBus::ExecuteQueuedEvents();
    AZ::TickBus::ExecuteQueuedEvents();
    AZ::MainThreadRenderRequestBus::ExecuteQueuedEvents();
}

#include <GameEngine.moc>
