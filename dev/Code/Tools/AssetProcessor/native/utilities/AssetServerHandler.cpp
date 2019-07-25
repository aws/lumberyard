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

#include <native/utilities/AssetServerHandler.h>
#include <native/resourcecompiler/rcjob.h>
#include <native/utilities/assetUtils.h>
#include <AzToolsFramework/Archive/ArchiveAPI.h>
#include <QDir>

namespace AssetProcessor
{

    QString ComputeArchiveFilePath(const AssetProcessor::BuilderParams& builderParams)
    {
        QFileInfo fileInfo(builderParams.m_processJobRequest.m_sourceFile.c_str());
        QString assetServerAddress = QDir::toNativeSeparators(AssetUtilities::ServerAddress());
        if (!assetServerAddress.isEmpty())
        {
            QDir assetServerDir(assetServerAddress);
            QString archiveFileName = builderParams.GetServerKey() + ".zip";
            QString archiveFilePath = QDir(assetServerDir.filePath(fileInfo.path())).filePath(archiveFileName);
            return archiveFilePath;
        }

        return QString();
    }

    AssetServerHandler::AssetServerHandler()
    {
        AssetServerBus::Handler::BusConnect();
    }

    AssetServerHandler::~AssetServerHandler()
    {
        AssetServerBus::Handler::BusDisconnect();
    }

    bool AssetServerHandler::IsServerAddressValid()
    {
        QString address = AssetUtilities::ServerAddress();
        bool isValid = !address.isEmpty() && QDir(address).exists();
        return isValid;
    }

    bool AssetServerHandler::RetrieveJobResult(const AssetProcessor::BuilderParams& builderParams)
    {
        AssetBuilderSDK::JobCancelListener jobCancelListener(builderParams.m_rcJob->GetJobEntry().m_jobRunKey);
        AssetUtilities::QuitListener listener;
        listener.BusConnect();

        QString archiveAbsFilePath = ComputeArchiveFilePath(builderParams);
        if (archiveAbsFilePath.isEmpty())
        {
            return false;
        }

        if (!QFile::exists(archiveAbsFilePath))
        {
            // file does not exists on the server 
            return false;
        }

        if (listener.WasQuitRequested() || jobCancelListener.IsCancelled())
        {
            return false;
        }
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Extracting archive for job (%s, %s, %s) with fingerprint (%u).\n",
            builderParams.m_rcJob->GetJobEntry().m_pathRelativeToWatchFolder.toUtf8().data(), builderParams.m_rcJob->GetJobKey().toUtf8().data(),
            builderParams.m_rcJob->GetPlatformInfo().m_identifier.c_str(), builderParams.m_rcJob->GetOriginalFingerprint());
        bool success = false;
        AzToolsFramework::ArchiveCommands::Bus::BroadcastResult(success, &AzToolsFramework::ArchiveCommands::ExtractArchiveBlocking, archiveAbsFilePath.toUtf8().data(), builderParams.GetTempJobDirectory(), false);
        AZ_Warning(AssetProcessor::DebugChannel, success, "Extracting archive operation failed.\n");
        return success;
    }

    bool AssetServerHandler::StoreJobResult(const AssetProcessor::BuilderParams& builderParams)
    {
        AssetBuilderSDK::JobCancelListener jobCancelListener(builderParams.m_rcJob->GetJobEntry().m_jobRunKey);
        AssetUtilities::QuitListener listener;
        listener.BusConnect();
        QString archiveAbsFilePath = ComputeArchiveFilePath(builderParams);
        
        if (archiveAbsFilePath.isEmpty())
        {
            return false;
        }

        if (QFile::exists(archiveAbsFilePath))
        {
            // file already exists on the server 
            return true;
        }
        
        if (listener.WasQuitRequested() || jobCancelListener.IsCancelled())
        {
            return false;
        }

        bool success = false;

        AZ_TracePrintf(AssetProcessor::DebugChannel, " Creating archive for job (%s, %s, %s) with fingerprint (%u).\n",
            builderParams.m_rcJob->GetJobEntry().m_pathRelativeToWatchFolder.toUtf8().data(), builderParams.m_rcJob->GetJobKey().toUtf8().data(),
            builderParams.m_rcJob->GetPlatformInfo().m_identifier.c_str(), builderParams.m_rcJob->GetOriginalFingerprint());
        AzToolsFramework::ArchiveCommands::Bus::BroadcastResult(success, &AzToolsFramework::ArchiveCommands::CreateArchiveBlocking, archiveAbsFilePath.toUtf8().data(), builderParams.GetTempJobDirectory());
        AZ_Warning(AssetProcessor::DebugChannel, success, "Creating archive operation failed.\n");

        return success;
    }

}// AssetProcessor