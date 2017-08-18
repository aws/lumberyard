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

#include <SceneAPI/FbxSceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            class FbxSkinImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(FbxSkinImporter, "{22108E92-7037-442D-94E0-A2E92554A79F}", SceneCore::LoadingComponent);

                FbxSkinImporter();
                ~FbxSkinImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportSkin(FbxNodeEncounteredContext& context);
            protected:
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
                // Workaround for VS2013 - Delete the copy constructor and make it private
                // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
                FbxSkinImporter(const FbxSkinImporter&) = delete;
#endif
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ