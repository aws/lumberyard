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

#include <StdAfx.h>
#include <AssetImporterDocument.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Export/MtlMaterialExporter.h>
#include <AzCore/IO/SystemFile.h>
#include <Util/PathUtil.h>
#include <GFxFramework/MaterialIO/IMaterial.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IPhysicsRule.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkeletonGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IAnimationGroup.h>
#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/Events/SceneSerializationBus.h>
#include <SceneAPI/SceneCore/Import/Utilities/FileFinder.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <IEditor.h>
#include <ISourceControl.h>
#include <QFile>
#include <QWidget>
#include <QMessageBox>
#include <QPushButton>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <ActionOutput.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

AssetImporterDocument::AssetImporterDocument()
{
}

bool AssetImporterDocument::LoadScene(const AZStd::string& sceneFullPath)
{
    namespace SceneEvents = AZ::SceneAPI::Events;
    SceneEvents::SceneSerializationBus::BroadcastResult(m_scene, &SceneEvents::SceneSerializationBus::Events::LoadScene, sceneFullPath, AZ::Uuid::CreateNull());
    return !!m_scene;
}

void AssetImporterDocument::SaveScene(AZStd::shared_ptr<AZ::ActionOutput>& output, AZ::SaveCompleteCallback onSaveComplete)
{
    if (!m_scene)
    {
        if (output)
        {
            output->AddError("No scene file was loaded.");
        }

        if (onSaveComplete)
        {
            onSaveComplete(false);
        }

        return;
    }

    m_saveRunner = AZStd::make_shared<AZ::AsyncSaveRunner>();

    // Add a no-op saver to put the FBX into source control. The benefit of doing it this way
    //  rather than invoking the SourceControlCommands bus directly is that we enable ourselves
    //  to have a single callback point for fbx & the manifest.
    auto fbxNoOpSaver = m_saveRunner->GenerateController();
    fbxNoOpSaver->AddSaveOperation(m_scene->GetSourceFilename(), nullptr);

    // Save the manifest
    SaveManifest();

    m_saveRunner->Run(output,
        [this, onSaveComplete](bool success)
        {
            if (onSaveComplete)
            {
                onSaveComplete(success);
            }

            m_saveRunner = nullptr;
    }, AZ::AsyncSaveRunner::ControllerOrder::Sequential);
}

void AssetImporterDocument::ClearScene()
{
    m_scene.reset();
}

void AssetImporterDocument::SaveManifest()
{
    // Create the save controller and add the save operation for the manifest job to it
    AZStd::shared_ptr<AZ::SaveOperationController> saveController = m_saveRunner->GenerateController();

    saveController->AddSaveOperation(m_scene->GetManifestFilename(),
        [this](const AZStd::string& fullPath, const AZStd::shared_ptr<AZ::ActionOutput>& actionOutput) -> bool
        {
            // Write the manifest
            AZ::IO::SystemFile file;
            file.Open(fullPath.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
            if (!file.IsOpen())
            {
                if (actionOutput)
                {
                    actionOutput->AddError("Unable to open manifest file for writing.", fullPath);
                }
                return false;
            }

            AZ::IO::SystemFileStream fileStream(&file, false);
            if (!fileStream.IsOpen())
            {
                if (actionOutput)
                {
                    actionOutput->AddError("Unable to create stream for manifest file.", fullPath);
                }
                return false;
            }

            if (!m_scene->GetManifest().SaveToStream(fileStream))
            {
                if (actionOutput)
                {
                    actionOutput->AddError("Unable to serialize manifest to file stream", fullPath);
                }
                return false;
            }

            return true;
        });
}

AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& AssetImporterDocument::GetScene()
{
    return m_scene;
}
