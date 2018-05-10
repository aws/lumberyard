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

#include <SceneAPI/SceneCore/Components/ExportingComponent.h>


namespace EMotionFX
{
    namespace Pipeline
    {
        struct ActorGroupExportContext;

        class ActorGroupExporter
            : public AZ::SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(ActorGroupExporter, "{9E21DF50-202B-44CB-A9E7-F429D3B5E5BE}", AZ::SceneAPI::SceneCore::ExportingComponent);

            ActorGroupExporter();
            ~ActorGroupExporter() override = default;

            static void Reflect(AZ::ReflectContext* context);

            AZ::SceneAPI::Events::ProcessingResult ProcessContext(ActorGroupExportContext& context) const;

        private:
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
            // Workaround for VS2013 - Delete the copy constructor and make it private
            // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
            ActorGroupExporter(const ActorGroupExporter&) = delete;
#endif
        };
    } // namespace Pipeline
} // namespace EMotionFX