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
#ifdef MOTIONCANVAS_GEM_ENABLED

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneData/EMotionFX/Rules/MetaDataRule.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {

            template<typename T>
            bool MetaDataRule::SaveMetaDataToFile(const AZStd::string& sourceAssetFilename, const AZStd::string& groupName, const AZStd::string& metaDataString)
            {
                AZ_TraceContext("Meta data", sourceAssetFilename);

                if (sourceAssetFilename.empty())
                {
                    AZ_Error("EMotionFX", false, "Source asset filename is empty.");
                    return false;
                }

                // Load the manifest from disk.
                AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene> scene = AZ::SceneAPI::Events::AssetImportRequest::LoadScene(sourceAssetFilename, AZ::SceneAPI::Events::AssetImportRequest::AssetProcessor);
                if (!scene)
                {
                    AZ_Error("EMotionFX", false, "Unable to save meta data to manifest due to failed scene loading.");
                    return false;
                }

                AZ::SceneAPI::Containers::SceneManifest& manifest = scene->GetManifest();
                auto values = manifest.GetValueStorage();
                auto groupView = AZ::SceneAPI::Containers::MakeDerivedFilterView<T>(values);
                for (T& group : groupView)
                {
                    // Non-case sensitive group name comparison. Product filenames are lower case only and might mismatch casing of the entered group name.
                    if (AzFramework::StringFunc::Equal(group.GetName().c_str(), groupName.c_str()))
                    {
                        SaveMetaData(group, metaDataString);
                    }
                }
  
                const AZStd::string& manifestFilename = scene->GetManifestFilename();

                // Source Control: Checkout manifest file.
                using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
                bool checkoutResult = false;
                ApplicationBus::BroadcastResult(checkoutResult, &ApplicationBus::Events::RequestEditForFileBlocking, manifestFilename.c_str(), "Checking out manifest file.", [](int& current, int& max) {});
                if (!checkoutResult)
                {
                    AZ_Error("EMotionFX", false, "Unable to checkout meta data from source control. Cannot save manifest file '%s'.", manifestFilename.c_str());
                    return false;
                }

                return manifest.SaveToFile(manifestFilename.c_str());
            }

        } // SceneData
    } // SceneAPI
} // AZ
#endif //MOTIONCANVAS_GEM_ENABLED