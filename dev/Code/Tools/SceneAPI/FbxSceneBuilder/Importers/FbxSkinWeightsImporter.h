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
    namespace FbxSDKWrapper
    {
        class FbxMeshWrapper;
    }

    namespace SceneData
    {
        namespace GraphData
        {
            class SkinWeightData;
        }
    }

    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            class FbxSkinWeightsImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(FbxSkinWeightsImporter, "{95FCD291-5E1F-4591-90AD-AB5EA2599C3E}", SceneCore::LoadingComponent);

                FbxSkinWeightsImporter();
                ~FbxSkinWeightsImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportSkinWeights(SceneNodeAppendedContext& context);

            protected:
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
                // Workaround for VS2013 - Delete the copy constructor and make it private
                // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
                FbxSkinWeightsImporter(const FbxSkinWeightsImporter&) = delete;
#endif
                AZStd::shared_ptr<SceneData::GraphData::SkinWeightData> BuildSkinWeightData(
                    const std::shared_ptr<const FbxSDKWrapper::FbxMeshWrapper>& fbxMesh, int skinIndex);

                static const AZStd::string s_skinWeightName;
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ