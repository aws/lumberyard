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

#pragma once

#include <IConvertor.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#ifdef MOTIONCANVAS_GEM_ENABLED
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#endif


namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
            class SceneManifest;
        }
        namespace Import
        {
            class IImportersList;
        }
    }
    namespace RC
    {
        // Used, among other things, to make sure a valid SerializeContext is available.
        class RCToolApplication : public AzToolsFramework::ToolsApplication
        {
        public:
            RCToolApplication();
            void AddSystemComponents(AZ::Entity* systemEntity) override;

            AZ::ComponentTypeList GetRequiredSystemComponents() const override;
            void ResolveModulePath(AZ::OSString& modulePath) override;
        };

        class ISceneConfig;
        class SceneCompiler
            : public ICompiler
        {
        public:
            explicit SceneCompiler(const AZStd::shared_ptr<ISceneConfig>& config);

            void Release() override;

            void BeginProcessing(const IConfig* config) override;
            bool Process() override;
            void EndProcessing() override;

            IConvertContext* GetConvertContext() override;

        protected:
            virtual bool PrepareForExporting(RCToolApplication& application);
            virtual bool LoadAndExportScene();
            virtual bool ExportScene(const AZ::SceneAPI::Containers::Scene& scene, int platformId);

            ConvertContext m_context;
            AZStd::shared_ptr<ISceneConfig> m_config;

        private:
#if defined(MOTIONCANVAS_GEM_ENABLED)
            void InitMotionCanvasSystem();
            void ShutdownMotionCanvasSystem();

            AZStd::unique_ptr<CommandSystem::CommandManager>    m_commandManager;
            bool                                                m_motionCanvasInited = false;
#endif
        };
    } // namespace RC
} // namespace AZ