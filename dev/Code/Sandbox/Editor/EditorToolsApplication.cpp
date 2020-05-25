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

#include "StdAfx.h"

#include "EditorToolsApplication.h"

#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/MaterialBrowser/MaterialBrowserComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <ParseEngineConfig.h>
#include <Crates/Crates.h>
#include <Engines/EnginesAPI.h> // For LyzardSDK
#include <QMessageBox>

#include <CryEdit.h>
#include <CryEditDoc.h>
#include <DisplaySettingsPythonFuncs.h>
#include <GameEngine.h>
#include <Material/MaterialPythonFuncs.h>
#include <Modelling/ModellingMode.h>
#include <PythonEditorFuncs.h>
#include <TrackView/TrackViewPythonFuncs.h>
#include <VegetationTool.h>
#include <ViewPane.h>
#include <ViewportTitleDlg.h>
#ifdef LY_TERRAIN_EDITOR
#include <TerrainModifyTool.h>
#include <Terrain/PythonTerrainFuncs.h>
#include <Terrain/PythonTerrainLayerFuncs.h>
#include <TerrainTexturePainter.h>
#include <TerrainHoleTool.h>
#include <TerrainTexture.h>
#endif

namespace EditorInternal
{
    EditorToolsApplication::EditorToolsApplication(int* argc, char*** argv)
        : ToolsApplication(argc, argv)
    {
        EditorToolsApplicationRequests::Bus::Handler::BusConnect();
    }

    EditorToolsApplication::~EditorToolsApplication()
    {
        EditorToolsApplicationRequests::Bus::Handler::BusDisconnect();
        Stop();
    }

    bool EditorToolsApplication::OnFailedToFindConfiguration(const char* configFilePath)
    {
        bool overrideAssetRoot = (this->m_assetRoot[0] != '\0');
        const char* sourcePaths[] = { this->m_assetRoot };

        CEngineConfig engineCfg(sourcePaths, overrideAssetRoot ? 1 : 0);

        char msg[4096] = { 0 };
        azsnprintf(msg, sizeof(msg),
            "Application descriptor file not found:\n"
            "%s\n"
            "Generate this file and commit it to source control by running:\n"
            "Bin64\\lmbr.exe projects populate-appdescriptors -projects %s",
            configFilePath, engineCfg.m_gameDLL.c_str());
        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QObject::tr("File not found"), msg);

        // flag that we failed, so that the application can exit out
        m_StartupAborted = true;

        // indicate that we don't want AzFramework::Application::Start to continue startup
        return false;
    }

    bool EditorToolsApplication::IsStartupAborted() const
    {
        return m_StartupAborted;
    }


    void EditorToolsApplication::RegisterCoreComponents()
    {
        AzToolsFramework::ToolsApplication::RegisterCoreComponents();

        RegisterComponentDescriptor(AZ::CratesHandler::CreateDescriptor());

        // Expose Legacy Python Bindings to Behavior Context for EditorPythonBindings Gem
        RegisterComponentDescriptor(AzToolsFramework::CryEditPythonHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::CryEditDocFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::DisplaySettingsPythonFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::MainWindowEditorFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::ModellingModeFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::ObjectManagerFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::MaterialPythonFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::PythonEditorFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::TrackViewFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::VegetationToolFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::ViewPanePythonFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::ViewportTitleDlgPythonFuncsHandler::CreateDescriptor());
#ifdef LY_TERRAIN_EDITOR
        RegisterComponentDescriptor(AzToolsFramework::TerrainModifyPythonFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::TerrainPythonFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::TerrainLayerPythonFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::TerrainPainterPythonFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::TerrainHoleToolPythonFuncsHandler::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::TerrainTexturePythonFuncsHandler::CreateDescriptor());
#endif
    }


    AZ::ComponentTypeList EditorToolsApplication::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = AzToolsFramework::ToolsApplication::GetRequiredSystemComponents();

        components.emplace_back(azrtti_typeid<AZ::CratesHandler>());
        components.emplace_back(azrtti_typeid<AzToolsFramework::Thumbnailer::ThumbnailerComponent>());
        components.emplace_back(azrtti_typeid<AzToolsFramework::AssetBrowser::AssetBrowserComponent>());
        components.emplace_back(azrtti_typeid<AzToolsFramework::MaterialBrowser::MaterialBrowserComponent>());

        return components;
    }


    void EditorToolsApplication::StartCommon(AZ::Entity* systemEntity)
    {
        AzToolsFramework::ToolsApplication::StartCommon(systemEntity);
        if (systemEntity->GetState() != AZ::Entity::ES_ACTIVE)
        {
            m_StartupAborted = true;
            return;
        }

        AZ::ModuleManagerRequestBus::Broadcast(&AZ::ModuleManagerRequestBus::Events::LoadDynamicModule, "LyzardEngines", AZ::ModuleInitializationSteps::ActivateEntity, true);
        AZ::ModuleManagerRequestBus::Broadcast(&AZ::ModuleManagerRequestBus::Events::LoadDynamicModule, "LyzardGems", AZ::ModuleInitializationSteps::ActivateEntity, true);
        AZ::ModuleManagerRequestBus::Broadcast(&AZ::ModuleManagerRequestBus::Events::LoadDynamicModule, "LyzardProjects", AZ::ModuleInitializationSteps::ActivateEntity, true);
    }

    bool EditorToolsApplication::Start()
    {
        char descriptorPath[AZ_MAX_PATH_LEN] = { 0 };
        char appRootOverride[AZ_MAX_PATH_LEN] = { 0 };
        {
            AzFramework::Application::StartupParameters params;

            AZ_Assert(m_argC != nullptr, "Missing Command Line Data");
            AZ_Assert(m_argV != nullptr, "Missing Command Line Data");

            if (!GetOptionalAppRootArg(*m_argC, *m_argV, appRootOverride, AZ_ARRAY_SIZE(appRootOverride)))
            {
                QString currentRoot;
                QDir pathCheck(qApp->applicationDirPath());
                do
                {
                    if (pathCheck.exists(QString("engine.json")))
                    {
                        currentRoot = pathCheck.absolutePath();
                        break;
                    }
                } while (pathCheck.cdUp());
                if (currentRoot.isEmpty())
                {
                    return false;
                }
                // CEngineConfig has a default depth of 3 to look for bootstrap.cfg
                // file. When the editor is in an App Bundle then it will be 3 levels
                // deeper. So make the max_depth we are willing to search to be 6, 3
                // for the default + 3 for app bundle location.
                const int max_depth = 6;
                CEngineConfig engineCfg(nullptr, 0, max_depth);

                azstrcat(descriptorPath, AZ_MAX_PATH_LEN, engineCfg.m_gameFolder);
                azstrcat(descriptorPath, AZ_MAX_PATH_LEN, "/Config/Editor.xml");

                azstrncpy(appRootOverride, AZ_ARRAY_SIZE(appRootOverride), currentRoot.toUtf8().data(), currentRoot.length());
            }
            else
            {
                const char* sourcePaths[] = { appRootOverride };
                CEngineConfig engineCfg(sourcePaths,AZ_ARRAY_SIZE(sourcePaths));
                azstrcat(descriptorPath, AZ_MAX_PATH_LEN, engineCfg.m_rootFolder);
                azstrcat(descriptorPath, AZ_MAX_PATH_LEN, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
                azstrcat(descriptorPath, AZ_MAX_PATH_LEN, engineCfg.m_gameFolder);
                azstrcat(descriptorPath, AZ_MAX_PATH_LEN, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "Config" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING  "Editor.xml");
            }

            params.m_appRootOverride = appRootOverride;

            // Must be done before creating QApplication, otherwise asserts when we alloc
            AzToolsFramework::ToolsApplication::Start(descriptorPath, params);
            if (IsStartupAborted())
            {
                return false;
            }
        }
        return true;
    }

    bool EditorToolsApplication::GetOptionalAppRootArg(int argc, char* argv[], char destinationRootArgBuffer[], size_t destinationRootArgBufferSize) const
    {
        const char* appRootArg = nullptr;
        bool isAppRootArg = false;
        for (int index = 0; index < argc; index++)
        {
            if (isAppRootArg)
            {
                appRootArg = argv[index];
                isAppRootArg = false;
            }
            else if (azstricmp("--app-root", argv[index]) == 0)
            {
                isAppRootArg = true;
            }
        }
        if (appRootArg)
        {
            azstrncpy(destinationRootArgBuffer, destinationRootArgBufferSize, appRootArg, strlen(appRootArg));
            const char lastChar = destinationRootArgBuffer[strlen(destinationRootArgBuffer) - 1];
            bool needsTrailingPathDelim = (lastChar != AZ_CORRECT_FILESYSTEM_SEPARATOR) && (lastChar != AZ_WRONG_FILESYSTEM_SEPARATOR);
            if (needsTrailingPathDelim)
            {
                azstrncat(destinationRootArgBuffer, destinationRootArgBufferSize, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING, 1);
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    void EditorToolsApplication::QueryApplicationType(AzFramework::ApplicationTypeQuery& appType) const
    { 
        appType.m_maskValue = AzFramework::ApplicationTypeQuery::Masks::Editor | AzFramework::ApplicationTypeQuery::Masks::Tool;
    };

    void EditorToolsApplication::CreateReflectionManager()
    {
        ToolsApplication::CreateReflectionManager();

        GetSerializeContext()->CreateEditContext();
    }

    void EditorToolsApplication::Reflect(AZ::ReflectContext* context)
    {
        ToolsApplication::Reflect(context);

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EditorToolsApplicationRequestBus>("EditorToolsApplicationRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor")
                ->Event("OpenLevel", &EditorToolsApplicationRequests::OpenLevel)
                ->Event("OpenLevelNoPrompt", &EditorToolsApplicationRequests::OpenLevelNoPrompt)
                ->Event("CreateLevel", &EditorToolsApplicationRequests::CreateLevel)
                ->Event("CreateLevelNoPrompt", &EditorToolsApplicationRequests::CreateLevelNoPrompt)
                ->Event("GetGameFolder", &EditorToolsApplicationRequests::GetGameFolder)
                ->Event("GetCurrentLevelName", &EditorToolsApplicationRequests::GetCurrentLevelName)
                ->Event("GetCurrentLevelPath", &EditorToolsApplicationRequests::GetCurrentLevelPath)
                ->Event("Exit", &EditorToolsApplicationRequests::Exit)
                ->Event("ExitNoPrompt", &EditorToolsApplicationRequests::ExitNoPrompt)
                ;
        }
    }

    AZStd::string EditorToolsApplication::GetGameFolder() const
    {
        return Path::GetEditingGameDataFolder();
    }

    bool EditorToolsApplication::OpenLevel(AZStd::string_view levelName)
    {
        AZStd::string levelPath = levelName;

        if (!AZ::IO::SystemFile::Exists(levelPath.c_str()))
        {
            // now let's check if they pre-pended directories (Samples/SomeLevelName)
            AZStd::string levelFileName;
            {
                AZStd::size_t found = levelPath.find_last_of("/\\");
                if (found == AZStd::string::npos)
                {
                    levelFileName = levelPath.substr(found + 1);
                }
                else
                {
                    levelFileName = levelPath;
                }
            }

            // if the input path can't be found, let's automatically add on the game folder and the levels
            AzFramework::StringFunc::Path::Join(levelPath.c_str(), levelFileName.c_str(), levelPath);
            AzFramework::StringFunc::Path::Join(DefaultLevelFolder, levelPath.c_str(), levelPath);
            AzFramework::StringFunc::Path::Join(GetGameFolder().c_str(), levelPath.c_str(), levelPath);

            // make sure the level path includes the cry extension, if needed
            if (!levelFileName.ends_with(OldFileExtension) && !levelFileName.ends_with(DefaultFileExtension))
            {
                AZStd::size_t levelPathLength = levelPath.length();
                levelPath += OldFileExtension;

                // Check if there is a .cry file, otherwise assume it is a new .ly file
                if (!AZ::IO::SystemFile::Exists(levelPath.c_str()))
                {
                    levelPath.replace(levelPathLength, sizeof(OldFileExtension) - 1, DefaultFileExtension);
                }
            }

            if (!AZ::IO::SystemFile::Exists(levelPath.c_str()))
            {
                return false;
            }
        }
        
        auto previousDocument = GetIEditor()->GetDocument();
        QString previousPathName = (previousDocument != nullptr) ? previousDocument->GetLevelPathName() : "";
        auto newDocument = CCryEditApp::instance()->OpenDocumentFile(levelPath.c_str());

        // the underlying document pointer doesn't change, so we can't check that; use the path name's instead
        return newDocument && !newDocument->IsLevelLoadFailed() && (newDocument->GetLevelPathName() != previousPathName);
    }

    bool EditorToolsApplication::OpenLevelNoPrompt(AZStd::string_view levelName)
    {
        GetIEditor()->GetDocument()->SetModifiedFlag(false);
        return OpenLevel(levelName);
    }

    int EditorToolsApplication::CreateLevel(AZStd::string_view levelName, int resolution, int unitSize, bool bUseTerrain)
    {
        // Clang warns about a temporary being created in a function's arguement list, so fullyQualifiedLevelName before the call
        QString fullyQualifiedLevelName;
        return CCryEditApp::instance()->CreateLevel(QString::fromUtf8(levelName.data(), static_cast<int>(levelName.size())), resolution, unitSize, bUseTerrain, fullyQualifiedLevelName,
            CCryEditApp::TerrainTextureExportSettings(CCryEditApp::TerrainTextureExportTechnique::PromptUser));
    }

    int EditorToolsApplication::CreateLevelNoPrompt(AZStd::string_view levelName, int heightmapResolution, int heightmapUnitSize, int terrainExportTextureSize, bool useTerrain)
    {
        // Clang warns about a temporary being created in a function's arguement list, so fullyQualifiedLevelName before the call
        QString fullyQualifiedLevelName;
        return CCryEditApp::instance()->CreateLevel(QString::fromUtf8(levelName.data(), static_cast<int>(levelName.size())), heightmapResolution, heightmapUnitSize, useTerrain, fullyQualifiedLevelName,
            CCryEditApp::TerrainTextureExportSettings(CCryEditApp::TerrainTextureExportTechnique::UseDefault, terrainExportTextureSize));
    }

    AZStd::string EditorToolsApplication::GetCurrentLevelName() const
    {
        return AZStd::string(GetIEditor()->GetGameEngine()->GetLevelName().toUtf8().data());
    }

    AZStd::string EditorToolsApplication::GetCurrentLevelPath() const
    {
        return AZStd::string(GetIEditor()->GetGameEngine()->GetLevelPath().toUtf8().data());
    }

    void EditorToolsApplication::Exit()
    {
        // Adding a single-shot QTimer to PyExit delays the QApplication::closeAllWindows call until
        // all the events in the event queue have been processed. Calling QApplication::closeAllWindows instead
        // of MainWindow::close ensures the Metal render window is cleaned up on macOS.
        QTimer::singleShot(0, qApp, &QApplication::closeAllWindows);
    }

    void EditorToolsApplication::ExitNoPrompt()
    {
        // Set the level to "unmodified" so that it doesn't prompt to save on exit.
        GetIEditor()->GetDocument()->SetModifiedFlag(false);
        Exit();
    }

}

