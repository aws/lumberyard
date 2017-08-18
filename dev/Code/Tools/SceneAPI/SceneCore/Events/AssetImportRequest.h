#pragma once

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

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/EBus/ebus.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace Events
        {
            enum class LoadingResult
            {
                Ignored,
                AssetLoaded,
                ManifestLoaded,
                AssetFailure,
                ManifestFailure
            };

            class LoadingResultCombiner
            {
            public:
                inline LoadingResultCombiner();
                inline void operator=(LoadingResult rhs);
                inline ProcessingResult GetManifestResult() const;
                inline ProcessingResult GetAssetResult() const;

            private:
                ProcessingResult m_manifestResult;
                ProcessingResult m_assetResult;
            };

            class AssetImportRequest
                : public AZ::EBusTraits
            {
            public:
                enum RequestingApplication
                {
                    Generic,
                    Editor,
                    AssetProcessor
                };
                enum ManifestAction
                {
                    Update,
                    ConstructDefault
                };

                static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

                virtual ~AssetImportRequest() = 0;

                // Fills the given list with all available file extensions, excluding the extension for the manifest.
                SCENE_CORE_API virtual void GetSupportedFileExtensions(AZStd::unordered_set<AZStd::string>& extensions);
                // Gets the file extension for the manifest.
                SCENE_CORE_API virtual void GetManifestExtension(AZStd::string& result);

                // Before asset loading starts this is called to allow for any required initialization.
                SCENE_CORE_API virtual ProcessingResult PrepareForAssetLoading(Containers::Scene& scene, RequestingApplication requester);
                // Starts the loading of the asset at the given path in the given scene. Loading optimizations can be applied based on 
                //      the calling application.
                SCENE_CORE_API virtual LoadingResult LoadAsset(Containers::Scene& scene, const AZStd::string& path, RequestingApplication requester);
                // FinalizeAssetLoading can be used to do any work to complete loading, such as complete asynchronous loading
                //      or adjust the loaded content in the the SceneGraph. While manifest changes can be done here as well, it's
                //      recommended to wait for the UpdateManifest call.
                SCENE_CORE_API virtual void FinalizeAssetLoading(Containers::Scene& scene, RequestingApplication requester);
                // After all loading has completed, this call can be used to make adjustments to the manifest. Based on the given
                //      action this can mean constructing a new manifest or updating an existing manifest. This call is intended
                //      to deal with any default behavior of the manifest.
                SCENE_CORE_API virtual ProcessingResult UpdateManifest(Containers::Scene& scene, ManifestAction action,
                    RequestingApplication requester);

                // Utility function to load an asset and manifest from file by using the EBus functions above. If the given path
                //      is to a manifest file this function will attempt to find the matching source file.
                SCENE_CORE_API static AZStd::shared_ptr<Containers::Scene> LoadScene(const AZStd::string& assetFilePath,
                    RequestingApplication requester);
                // Utility function to load an asset and manifest from file by using the EBus functions above. This function assumes
                //      that the given path points to the asset (not the manifest) and that it has been checked to exist.
                SCENE_CORE_API static AZStd::shared_ptr<Containers::Scene> LoadSceneFromVerifiedPath(const AZStd::string& assetFilePath,
                    RequestingApplication requester);
            };

            using AssetImportRequestBus = AZ::EBus<AssetImportRequest>;

            inline AssetImportRequest::~AssetImportRequest()
            {
            }
        } // Events
    } // SceneAPI
} // AZ