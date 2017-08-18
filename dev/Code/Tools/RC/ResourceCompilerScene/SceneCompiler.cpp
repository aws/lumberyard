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

#include "stdafx.h"
#include <IRCLog.h>
#include <ISceneConfig.h>
#include <ISystem.h>
#include <SceneCompiler.h>
#include <RCUtil.h>

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/string.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/IO/StreamerComponent.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/SourceControl/PerforceComponent.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <RC/ResourceCompilerScene/Cgf/CgfExporter.h>
#include <RC/ResourceCompilerScene/Chr/ChrExporter.h>
#include <RC/ResourceCompilerScene/Skin/SkinExporter.h>
#include <RC/ResourceCompilerScene/Caf/CafExporter.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Common/ContainerSettingsExporter.h>
#include <RC/ResourceCompilerScene/Common/MeshExporter.h>
#include <RC/ResourceCompilerScene/Common/MaterialExporter.h>
#include <RC/ResourceCompilerScene/Common/WorldMatrixExporter.h>
#include <RC/ResourceCompilerScene/Common/ColorStreamExporter.h>
#include <RC/ResourceCompilerScene/Common/UVStreamExporter.h>
#include <RC/ResourceCompilerScene/Common/SkeletonExporter.h>
#include <RC/ResourceCompilerScene/Common/SkinWeightExporter.h>
#include <RC/ResourceCompilerScene/Common/AnimationExporter.h>
#include <RC/ResourceCompilerScene/Common/BlendShapeExporter.h>
#include <RC/ResourceCompilerScene/Cgf/CgfGroupExporter.h>
#include <RC/ResourceCompilerScene/Cgf/CgfLodExporter.h>
#include <RC/ResourceCompilerScene/Chr/ChrGroupExporter.h>
#include <RC/ResourceCompilerScene/Skin/SkinGroupExporter.h>
#include <RC/ResourceCompilerScene/Skin/SkinLodExporter.h>
#include <RC/ResourceCompilerScene/Caf/CafGroupExporter.h>

// TODO: Including headers directly from a Gem is not modular, and can be removed once the pipeline supports Gem-based code. 
// This is contained within #ifdef MOTIONCANVAS_GEM_ENABLED, so it won't cause compile errors when the Gem isn't present.
#if defined(MOTIONCANVAS_GEM_ENABLED)
#include <RC/ResourceCompilerScene/MotionCanvasPipeline/MotionExporter.h>
#include <RC/ResourceCompilerScene/MotionCanvasPipeline/MotionGroupExporter.h>
#include <RC/ResourceCompilerScene/MotionCanvasPipeline/MotionDataBuilder.h>
#include <RC/ResourceCompilerScene/MotionCanvasPipeline/ActorExporter.h>
#include <RC/ResourceCompilerScene/MotionCanvasPipeline/ActorGroupExporter.h>
#include <RC/ResourceCompilerScene/MotionCanvasPipeline/ActorBuilder.h>
#endif

#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Components/Utilities/EntityConstructor.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

#if defined(MOTIONCANVAS_GEM_ENABLED)
    #include <EMotionFX/Source/EMotionFXManager.h>
    #include <MCore/Source/MCoreSystem.h>
#endif

namespace AZ
{
    namespace RC
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneContainers = AZ::SceneAPI::Containers;

        RCToolApplication::RCToolApplication()
        {
            RegisterComponentDescriptor(AnimationExporter::CreateDescriptor());
            RegisterComponentDescriptor(BlendShapeExporter::CreateDescriptor());
            RegisterComponentDescriptor(ColorStreamExporter::CreateDescriptor());
            RegisterComponentDescriptor(ContainerSettingsExporter::CreateDescriptor());
            RegisterComponentDescriptor(MeshExporter::CreateDescriptor());
            RegisterComponentDescriptor(SkeletonExporter::CreateDescriptor());
            RegisterComponentDescriptor(SkinWeightExporter::CreateDescriptor());
            RegisterComponentDescriptor(UVStreamExporter::CreateDescriptor());
            RegisterComponentDescriptor(WorldMatrixExporter::CreateDescriptor());
        }

        void RCToolApplication::AddSystemComponents(AZ::Entity* systemEntity)
        {
            AZ::ComponentApplication::AddSystemComponents(systemEntity);
            EnsureComponentAdded<AZ::MemoryComponent>(systemEntity);
        }

        AZ::ComponentTypeList RCToolApplication::GetRequiredSystemComponents() const
        {
            AZ::ComponentTypeList components = AzToolsFramework::ToolsApplication::GetRequiredSystemComponents();
            
            auto removed = AZStd::remove_if(components.begin(), components.end(), 
                [](const Uuid& id) -> bool
                {
                    return id == azrtti_typeid<AzFramework::TargetManagementComponent>()
                        || id == azrtti_typeid<AzToolsFramework::PerforceComponent>()
                        || id == azrtti_typeid<AZ::UserSettingsComponent>()
                        || id == azrtti_typeid<AZ::StreamerComponent>();
                });
            components.erase(removed, components.end());
            
            return components;
        }

        void RCToolApplication::ResolveModulePath(AZ::OSString& modulePath)
        {
            modulePath = AZ::OSString("../") + modulePath;  // first, pre-pend the .. to it.
            AzToolsFramework::ToolsApplication::ResolveModulePath(modulePath); // the base class will prepend the path to executable.
        }

        SceneCompiler::SceneCompiler(const AZStd::shared_ptr<ISceneConfig>& config)
            : m_config(config)
        {
        }

        void SceneCompiler::Release()
        {
#if defined(MOTIONCANVAS_GEM_ENABLED)
            ShutdownMotionCanvasSystem();
#endif
            delete this;
        }

        void SceneCompiler::BeginProcessing(const IConfig* /*config*/)
        {
#if defined(MOTIONCANVAS_GEM_ENABLED)
            InitMotionCanvasSystem();
#endif
        }

        bool SceneCompiler::Process()
        {
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Starting scene processing.\n");
            
            RCToolApplication application;
            if (!PrepareForExporting(application))
            {
                return false;
            }

            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Activating scene modules.\n");
            m_config->ActivateSceneModules();

            bool result = LoadAndExportScene();
            
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Deactivating scene modules.\n");
            m_config->DeactivateSceneModules();

            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Finished scene processing.\n");
            return true;
        }

        void SceneCompiler::EndProcessing()
        {
        }

        IConvertContext* SceneCompiler::GetConvertContext()
        {
            return &m_context;
        }

        bool SceneCompiler::PrepareForExporting(RCToolApplication& application)
        {
            // Not all Gems shutdown properly and leak memory, but this shouldn't
            //      prevent this builder from completing.
            AZ::AllocatorManager::Instance().SetAllocatorLeaking(true);
            
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Initializing tools application environment.\n");
            AZ::ComponentApplication::Descriptor descriptor;
            descriptor.m_useExistingAllocator = true;
            application.Start(descriptor);

            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Connecting to asset processor.\n");
            if (!ResourceCompilerUtil::ConnectToAssetProcessor(m_context.config->GetAsInt("port", 0, 0), "RC Scene Compiler"))
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to connect to Asset Processor on port %i.\n", m_context.config->GetAsInt("port", 0, 0));
                return false;
            }

            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Initializing serialize and edit context.\n");
            m_config->ReflectSceneModules(application.GetSerializeContext());

            const char* gameFolder = m_context.pRC->GetSystemEnvironment()->pFileIO->GetAlias("@devassets@");
            char configPath[2048];
            sprintf_s(configPath, "%s/config/editor.xml", gameFolder);
            AZ_TraceContext("Gem config file", configPath);
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Loading Gems.\n");
            if (!application.ReflectModulesFromAppDescriptor(configPath))
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to load Gems for scene processing.\n");
                return false;
            }

            return true;
        }

        bool SceneCompiler::LoadAndExportScene()
        {
            const string platformName = m_context.config->GetAsString("p", "<unknown>", "<invalid>");
            int platformId = m_context.config->GetAsInt("pi", AssetBuilderSDK::AllPlatforms, -1);
            AZ_TraceContext("Platform", platformName.c_str());
            AZ_TraceContext("Platform ID", platformId);
            if (platformId == -1)
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Invalid target platform provided.\n");
                return false;
            }

            AZStd::string sourcePath = m_context.GetSourcePath().c_str();
            AZ_TraceContext("Source", sourcePath);
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Loading source files.\n");
            AZStd::shared_ptr<SceneContainers::Scene> scene =
                SceneEvents::AssetImportRequest::LoadScene(sourcePath, SceneEvents::AssetImportRequest::RequestingApplication::AssetProcessor);
            if (!scene)
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to load asset.\n");
                return false;
            }

            AZ_TraceContext("Manifest", scene->GetManifestFilename());
            if (scene->GetManifest().IsEmpty())
            {
                AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "No manifest loaded and not enough information to create a default manifest.\n");
                return true;
            }

            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Exporting loaded data to engine specific formats.\n");
            if (!ExportScene(*scene, azlossy_caster(platformId)))
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to convert and export scene\n");
                return false;
            }
            return true;
        }

        bool SceneCompiler::ExportScene(const AZ::SceneAPI::Containers::Scene& scene, int platformId)
        {
            AZ_TraceContext("Output folder", m_context.GetOutputFolder().c_str());
            AZ_Assert(m_context.pRC->GetAssetWriter() != nullptr, "Invalid IAssetWriter initialization.");
            if (!m_context.pRC->GetAssetWriter())
            {
                return false;
            }

            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Creating export entity.\n");
            AZ::SceneAPI::SceneCore::EntityConstructor::EntityPointer exporter = AZ::SceneAPI::SceneCore::EntityConstructor::BuildEntity(
                "Scene Exporters", AZ::SceneAPI::SceneCore::ExportingComponent::TYPEINFO_Uuid());

            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Registering export processors.\n");
            // Register additional processors. Will be automatically unregistered when leaving scope.
            //      These have not yet been converted to components as they need special attention due
            //      to the arguments they currently need.
            CgfExporter cgfProcessor(&m_context);

            CgfGroupExporter meshGroupExporter(m_context.pRC->GetAssetWriter());
            CgfLodExporter meshLodExporter(m_context.pRC->GetAssetWriter());
            MaterialExporter materialExporter(&m_context);
            
#if defined(MOTIONCANVAS_GEM_ENABLED)
            MotionCanvasPipeline::MotionExporter motionExporter;
            MotionCanvasPipeline::MotionGroupExporter motionGroupExporter;
            MotionCanvasPipeline::MotionDataBuilder motionDataBuilder;
            MotionCanvasPipeline::ActorExporter actorExporter;
            MotionCanvasPipeline::ActorGroupExporter actorGroupExporter;
            MotionCanvasPipeline::ActorBuilder actorBuilder(&m_context);
#else
            ChrExporter chrProcessor(&m_context);
            SkinExporter skinProcessor(&m_context);

            CafExporter animationProcessor(&m_context);
            CafGroupExporter cafGroupExporter(m_context.pRC->GetAssetWriter(), &m_context);
            AnimationExporter animationExporter;
            ChrGroupExporter skeletonGroupExporter(m_context.pRC->GetAssetWriter(), &m_context);
            SkinGroupExporter skinGroupExporter(m_context.pRC->GetAssetWriter(), &m_context);
            SkinLodExporter skinLodExporter(m_context.pRC->GetAssetWriter(), &m_context);
#endif

            SceneEvents::ProcessingResultCombiner result;
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Preparing for export.\n");
            result += SceneEvents::Process<SceneEvents::PreExportEventContext>(scene, platformId);
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Exporting...\n");
            result += SceneEvents::Process<SceneEvents::ExportEventContext>(m_context.GetOutputFolder().c_str(), scene, platformId);
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Finalizing export process.\n");
            result += SceneEvents::Process<SceneEvents::PostExportEventContext>(m_context.GetOutputFolder().c_str(), platformId);

            switch (result.GetResult())
            {
            case SceneEvents::ProcessingResult::Success:
                return true;
            case SceneEvents::ProcessingResult::Ignored:
                AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "Nothing found to convert and export.\n");
                return true;
            case SceneEvents::ProcessingResult::Failure:
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failure during conversion and exporting.\n");
                return false;
            default:
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow,
                    "Unexpected result from conversion and exporting (%i).\n", result.GetResult());
                return false;
            }
        }

#if defined(MOTIONCANVAS_GEM_ENABLED)

        void SceneCompiler::InitMotionCanvasSystem()
        {
            if (!m_motionCanvasInited)
            {
                MCore::Initializer::InitSettings coreSettings;
                if (!MCore::Initializer::Init(&coreSettings))
                {
                    AZ_Error("EmotionFX Animation", false, "Failed to initialize EmotionFX SDK Core");
                    return;
                }

                // Initialize EmotionFX runtime.
                EMotionFX::Initializer::InitSettings emfxSettings;
                emfxSettings.mUnitType = MCore::Distance::UNITTYPE_METERS;

                if (!EMotionFX::Initializer::Init(&emfxSettings))
                {
                    AZ_Error("EmotionFX Animation", false, "Failed to initialize EmotionFX SDK Runtime");
                    return;
                }

                // Initialize the EMotionFX command system.
                m_commandManager = AZStd::make_unique<CommandSystem::CommandManager>();
                m_motionCanvasInited = true;
            }
        }

        void SceneCompiler::ShutdownMotionCanvasSystem()
        {
            if (m_motionCanvasInited)
            {
                m_motionCanvasInited = false;
                delete m_commandManager.release();
                EMotionFX::Initializer::Shutdown();
                MCore::Initializer::Shutdown();
            }
        }

#endif // MOTIONCANVAS_GEM_ENABLED
    } // namespace RC

} // namespace AZ
