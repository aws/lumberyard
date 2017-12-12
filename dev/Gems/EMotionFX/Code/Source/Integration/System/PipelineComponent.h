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
#if defined(EMOTIONFXANIMATION_EDITOR)
#pragma once

#include <SceneAPI/SceneCore/Components/SceneSystemComponent.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        class PipelineComponent 
            : public AZ::SceneAPI::SceneCore::SceneSystemComponent
        {
        public:
            AZ_COMPONENT(PipelineComponent, "{F74E0D7C-BF22-4BC0-897A-2D80DA960DB0}", AZ::SceneAPI::SceneCore::SceneSystemComponent);

            PipelineComponent();
            ~PipelineComponent() override = default;

            void Activate() override;
            void Deactivate() override;

            static void Reflect(AZ::ReflectContext* context);

        private:
            bool                                                m_EMotionFXInited;
            AZStd::unique_ptr<CommandSystem::CommandManager>    m_commandManager;

#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
            // Workaround for VS2013 - Delete the copy constructor and make it private
            // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
            PipelineComponent(const PipelineComponent&) = delete;
#endif
        };
    } // Pipeline
} // EMotionFX
#endif