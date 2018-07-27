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
#include "native/AssetManager/assetScannerWorker.h"
#include "native/AssetManager/assetScanner.h"
#include "native/utilities/assetUtils.h"
#include "native/utilities/PlatformConfiguration.h"
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>

using namespace AssetProcessor;

AssetScannerWorker::AssetScannerWorker(PlatformConfiguration* config, QObject* parent)
    : QObject(parent)
    , m_platformConfiguration(config)
{
}

void AssetScannerWorker::StartScan()
{
    // this must be called from the thread operating it and not the main thread.
    Q_ASSERT(QThread::currentThread() == this->thread());

    m_fileList.clear();
    m_doScan = true;

    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Scanning file system for changes...\n");

    Q_EMIT ScanningStateChanged(AssetProcessor::AssetScanningStatus::Started);
    Q_EMIT ScanningStateChanged(AssetProcessor::AssetScanningStatus::InProgress);

    for (int idx = 0; idx < m_platformConfiguration->GetScanFolderCount(); idx++)
    {
        ScanFolderInfo scanFolderInfo = m_platformConfiguration->GetScanFolderAt(idx);
        ScanForSourceFiles(scanFolderInfo);
    }
    // we want not to emit any signals until we're finished scanning
    // so that we don't interleave directory tree walking (IO access to the file table)
    // with file access (IO access to file data) caused by sending signals to other classes.

    if (!m_doScan)
    {
        m_fileList.clear();
        Q_EMIT ScanningStateChanged(AssetProcessor::AssetScanningStatus::Stopped);
        return;
    }
    else
    {
        EmitFiles();
    }

    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "File system scan done.\n");

    Q_EMIT ScanningStateChanged(AssetProcessor::AssetScanningStatus::Completed);
}

// note:  Call this directly from the main thread!
// do not queue this call.
// Join the thread if you intend to wait until its stopped
void AssetScannerWorker::StopScan()
{
    m_doScan = false;
}

void AssetScannerWorker::ScanForSourceFiles(ScanFolderInfo scanFolderInfo)
{
    if (!m_doScan)
    {
        return;
    }

    QDir dir(scanFolderInfo.ScanPath());

    QFileInfoList entries;

    //Only scan sub folders if recurseSubFolders flag is set
    if (!scanFolderInfo.RecurseSubFolders())
    {
        entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files);
    }
    else
    {
        entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files);
    }

    for (const QFileInfo& entry : entries)
    {
        if (!m_doScan) // scan was cancelled!
        {
            return;
        }

        QString absPath = entry.absoluteFilePath();

        // Filtering out excluded files
        if (m_platformConfiguration->IsFileExcluded(absPath))
        {
            continue;
        }

        if (entry.isDir())
        {
            //Entry is a directory
            ScanFolderInfo tempScanFolderInfo(absPath, "", "", "", false, true);
            ScanForSourceFiles(tempScanFolderInfo);
        }
        else
        {
            // Filtering out metadata files as well, there is no need to send both the source file and the metadafiles 
            // to the apm for analysis, just sending the source file should be enough
            bool isMetaFile = false;
            for (int idx = 0; idx < m_platformConfiguration->MetaDataFileTypesCount(); idx++)
            {
                QPair<QString, QString> metaInfo = m_platformConfiguration->GetMetaDataFileTypeAt(idx);
                if (absPath.endsWith("." + metaInfo.first, Qt::CaseInsensitive))
                {
                    // its a meta file
                    isMetaFile = true;
                    break;
                }
            }

            if (isMetaFile)
            {
                continue;
            }

            //Entry is a file
            m_fileList.insert(absPath);
        }
    }
}

void AssetScannerWorker::EmitFiles()
{
    //Loop over all source asset files and send them up the chain:
    for (const QString& fileEntries : m_fileList)
    {
        if (!m_doScan)
        {
            break;
        }
        Q_EMIT FileOfInterestFound(fileEntries);
    }
    m_fileList.clear();
}


#include <native/AssetManager/assetScannerWorker.moc>
