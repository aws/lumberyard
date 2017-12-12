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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Import/ManifestImportRequestHandler.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Import
        {
            const char* ManifestImportRequestHandler::s_extension = ".assetinfo";

            void ManifestImportRequestHandler::Activate()
            {
                BusConnect();
            }

            void ManifestImportRequestHandler::Deactivate()
            {
                BusDisconnect();
            }

            void ManifestImportRequestHandler::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<ManifestImportRequestHandler, BehaviorComponent>()->Version(1);
                }
            }

            void ManifestImportRequestHandler::GetManifestExtension(AZStd::string& result)
            {
                result = s_extension;
            }

            Events::LoadingResult ManifestImportRequestHandler::LoadAsset(Containers::Scene& scene, const AZStd::string& path, 
                const Uuid& /*guid*/, RequestingApplication /*requester*/)
            {
                AZStd::string manifestPath = path + s_extension;
                
                IO::SystemFile file;
                file.Open(manifestPath.c_str(), IO::SystemFile::SF_OPEN_READ_ONLY);
                scene.SetManifestFilename(AZStd::move(manifestPath));
                if (!file.IsOpen())
                {
                    // If there's no manifest file, the default settings will do.
                    return Events::LoadingResult::Ignored;
                }

                IO::SystemFileStream fileStream(&file, false);
                if (!fileStream.IsOpen())
                {
                    AZ_TracePrintf(Utilities::ErrorWindow, "Unable to open existing manifest file.\n");
                    return Events::LoadingResult::ManifestFailure;
                }

                return scene.GetManifest().LoadFromStream(fileStream) ? Events::LoadingResult::ManifestLoaded : Events::LoadingResult::ManifestFailure;
            }
        } // namespace Import
    } // namespace SceneAPI
} // namespace AZ