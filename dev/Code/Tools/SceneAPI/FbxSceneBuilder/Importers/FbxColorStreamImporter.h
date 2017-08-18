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

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/FbxSceneBuilder/ImportContexts/FbxImportContexts.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            class MeshVertexColorData;
        }
    }

    namespace FbxSDKWrapper
    {
        class FbxMeshWrapper;
        class FbxVertexColorWrapper;
    }

    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            class FbxColorStreamImporter
                : public SceneCore::LoadingComponent
            {
            public:
                AZ_COMPONENT(FbxColorStreamImporter, "{96A25361-04FC-43EC-A443-C81E2E28F3BB}", SceneCore::LoadingComponent);

                FbxColorStreamImporter();
                ~FbxColorStreamImporter() override = default;

                static void Reflect(ReflectContext* context);

                Events::ProcessingResult ImportColorStreams(SceneNodeAppendedContext& context);

            protected:
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
                // Workaround for VS2013 - Delete the copy constructor and make it private
                // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
                FbxColorStreamImporter(const FbxColorStreamImporter&) = delete;
#endif
                AZStd::shared_ptr<SceneData::GraphData::MeshVertexColorData> BuildVertexColorData(const FbxSDKWrapper::FbxVertexColorWrapper& fbxVertexColors, size_t vertexCount, const std::shared_ptr<FbxSDKWrapper::FbxMeshWrapper>& fbxMesh);
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ