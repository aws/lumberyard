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

#include <native/FileProcessor/FileProcessor.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <native/AssetManager/assetScanFolderInfo.h>
#include <native/utilities/assetUtils.h>
#include <native/utilities/PlatformConfiguration.h>

#include <QSet>
#include <QDir>
#include <QFileInfo>

namespace FileProcessorPrivate
{
    bool FinishedScanning(AssetProcessor::AssetScanningStatus status)
    {
        return status == AssetProcessor::AssetScanningStatus::Completed ||
               status == AssetProcessor::AssetScanningStatus::Stopped;
    }

    QString GenerateUniqueFileKey(AZ::s64 scanFolder, const char* fileName)
    {
        return QString("%1:%2").arg(scanFolder).arg(fileName);
    }
}

namespace AssetProcessor
{
    using namespace FileProcessorPrivate;

    FileProcessor::FileProcessor(PlatformConfiguration* config)
        : m_platformConfig(config)
    {
        m_connection = AZStd::shared_ptr<AssetDatabaseConnection>(aznew AssetDatabaseConnection());
        m_connection->OpenDatabase();

        QDir cacheRootDir;
        if (!AssetUtilities::ComputeProjectCacheRoot(cacheRootDir))
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to compute cache root folder");
        }
        m_normalizedCacheRootPath = AssetUtilities::NormalizeDirectoryPath(cacheRootDir.absolutePath());

        //query all current files from Files table
        auto filesFunction = [this](AzToolsFramework::AssetDatabase::FileDatabaseEntry& entry)
            {
                QString uniqueKey = GenerateUniqueFileKey(entry.m_scanFolderPK, entry.m_fileName.c_str());
                m_filesInDatabase[uniqueKey] = entry.m_fileID;
                return true;
            };
        m_connection->QueryFilesTable(filesFunction);
    }

    FileProcessor::~FileProcessor() = default;

    void FileProcessor::OnAssetScannerStatusChange(AssetScanningStatus status)
    {
        //when AssetScanner finished processing, synchronize Files table
        if (FileProcessorPrivate::FinishedScanning(status))
        {
            QMetaObject::invokeMethod(this, "Sync", Qt::QueuedConnection);
        }
    }

    void FileProcessor::AssessFilesFromScanner(QSet<QString> files)
    {
        for (const QString& fileName : files)
        {
            m_filesInAssetScanner.append(fileName);
        }
    }

    void FileProcessor::AssessFoldersFromScanner(QSet<QString> folders)
    {
        for (const QString& folderName : folders)
        {
            m_filesInAssetScanner.append(folderName);
        }
    }

    void FileProcessor::AssessAddedFile(QString filePath)
    {
        using namespace AzToolsFramework;

        if (m_shutdownSignalled)
        {
            return;
        }

        QString relativeFileName;
        QString scanFolderPath;
        if (!GetRelativePath(filePath, relativeFileName, scanFolderPath))
        {
            return;
        }

        const ScanFolderInfo* scanFolderInfo = m_platformConfig->GetScanFolderByPath(scanFolderPath);
        if (!scanFolderInfo)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to find the scan folder for file %s", filePath);
            return;
        }

        AssetDatabase::FileDatabaseEntry file;
        file.m_scanFolderPK = scanFolderInfo->ScanFolderID();
        file.m_fileName = relativeFileName.toUtf8().constData();
        file.m_isFolder = QFileInfo(filePath).isDir();
        if (m_connection->InsertFile(file))
        {
            AssetSystem::FileInfosNotificationMessage message;
            message.m_type = AssetSystem::FileInfosNotificationMessage::FileAdded;
            message.m_fileID = file.m_fileID;
            ConnectionBus::Broadcast(&ConnectionBusTraits::Send, 0, message);
        }

        if (file.m_isFolder)
        {
            QDir folder(filePath);
            for (const QFileInfo& subFile : folder.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot))
            {
                AssessAddedFile(subFile.absoluteFilePath());
            }
        }
    }

    void FileProcessor::AssessDeletedFile(QString filePath)
    {
        using namespace AzToolsFramework;
        
        if (m_shutdownSignalled)
        {
            return;
        }

        QString relativeFileName;
        QString scanFolderPath;
        if (!GetRelativePath(filePath, relativeFileName, scanFolderPath))
        {
            return;
        }

        const ScanFolderInfo* scanFolderInfo = m_platformConfig->GetScanFolderByPath(scanFolderPath);
        if (!scanFolderInfo)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to find the scan folder for file %s", filePath);
            return;
        }

        AssetDatabase::FileDatabaseEntry file;
        if (m_connection->GetFileByFileNameAndScanFolderId(relativeFileName, scanFolderInfo->ScanFolderID(), file) && DeleteFileRecursive(file))
        {
            AssetSystem::FileInfosNotificationMessage message;
            message.m_type = AssetSystem::FileInfosNotificationMessage::FileRemoved;
            message.m_fileID = file.m_fileID;
            ConnectionBus::Broadcast(&ConnectionBusTraits::Send, 0, message);
        }
    }

    void FileProcessor::Sync()
    {
        using namespace AzToolsFramework;

        if (m_shutdownSignalled)
        {
            return;
        }

        //first collect all fileIDs in Files table
        QSet<AZ::s64> missingFileIDs;
        for (AZ::s64 fileID : m_filesInDatabase.values())
        {
            missingFileIDs.insert(fileID);
        }

        for (QString fullFileName : m_filesInAssetScanner)
        {
            bool isDir = QFileInfo(fullFileName).isDir();
            QString scanFolderName;
            QString relativeFileName;

            if (!m_platformConfig->ConvertToRelativePath(fullFileName, relativeFileName, scanFolderName))
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to convert full path to relative for file %s", fullFileName);
                continue;
            }

            const ScanFolderInfo* scanFolderInfo = m_platformConfig->GetScanFolderForFile(fullFileName);
            if (!scanFolderInfo)
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to find the scan folder for file %s", fullFileName);
                continue;
            }

            AssetDatabase::FileDatabaseEntry file;
            file.m_scanFolderPK = scanFolderInfo->ScanFolderID();
            file.m_fileName = relativeFileName.toUtf8().constData(),
            file.m_isFolder = isDir;

            //when file is found by AssetScanner, remove it from the "missing" set
            QString uniqueKey = GenerateUniqueFileKey(file.m_scanFolderPK, relativeFileName.toUtf8().constData());
            if (m_filesInDatabase.contains(uniqueKey))
            {
                // found it, its not missing anymore.  (Its also already in the db)
                missingFileIDs.remove(m_filesInDatabase[uniqueKey]);
            }
            else
            {
                // its a new file we were previously unaware of.
                m_connection->InsertFile(file);
            }
        }

        // remove remaining files from the database as they no longer exist on hard drive
        for (AZ::s64 fileID : missingFileIDs)
        {
            m_connection->RemoveFile(fileID);
        }

        AssetSystem::FileInfosNotificationMessage message;
        ConnectionBus::Broadcast(&ConnectionBusTraits::Send, 0, message);
        
        // clear memory, we dont have a use for this map anymore.
        QMap<QString, AZ::s64> emptyMap;
        m_filesInDatabase.swap(emptyMap);
    }

    // note that this function normalizes the path and also returns true only if the file is 'relevant'
    // meaning something we care about tracking (ignore list/ etc taken into account).
    bool FileProcessor::GetRelativePath(QString& filePath, QString& relativeFileName, QString& scanFolderPath) const
    {
        filePath = AssetUtilities::NormalizeFilePath(filePath);
        if (filePath.startsWith(m_normalizedCacheRootPath, Qt::CaseInsensitive))
        {
            // modifies/adds to the cache are irrelevant.  Deletions are all we care about
            return false;
        } 

        if (m_platformConfig->IsFileExcluded(filePath))
        {
            return false; // we don't care about this kind of file.
        }

        if (!m_platformConfig->ConvertToRelativePath(filePath, relativeFileName, scanFolderPath))
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to convert full path to relative for file %s", filePath);
            return false;
        }
        return true;
    }

    bool FileProcessor::DeleteFileRecursive(const AzToolsFramework::AssetDatabase::FileDatabaseEntry& file) const
    {
        using namespace AzToolsFramework;

        if (m_shutdownSignalled)
        {
            return false;
        }

        if (file.m_isFolder)
        {
            AssetDatabase::FileDatabaseEntryContainer container;
            AZStd::string searchStr = file.m_fileName + AZ_CORRECT_DATABASE_SEPARATOR;
            m_connection->GetFilesLikeFileName(
                searchStr.c_str(),
                AssetDatabaseConnection::LikeType::StartsWith,
                container);
            for (const auto& subFile : container)
            {
                DeleteFileRecursive(subFile);
            }
        }

        return m_connection->RemoveFile(file.m_fileID);
    }

    void FileProcessor::QuitRequested()
    {
        m_shutdownSignalled = true;
        Q_EMIT ReadyToQuit(this);
    }

} // namespace AssetProcessor

#include <native/FileProcessor/FileProcessor.moc>
