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

#include "FileStateCache.h"
#include <native/utilities/assetUtils.h>
#include <QDir>

namespace AssetProcessor
{

    bool FileStateCache::GetFileInfo(const QString& absolutePath, FileStateInfo* foundFileInfo) const
    {
        LockGuardType scopeLock(m_mapMutex);
        auto itr = m_fileInfoMap.find(PathToKey(absolutePath));

        if (itr != m_fileInfoMap.end())
        {
            *foundFileInfo = itr.value();
            return true;
        }

        return false;
    }

    bool FileStateCache::Exists(const QString& absolutePath) const
    {
        LockGuardType scopeLock(m_mapMutex);
        auto itr = m_fileInfoMap.find(PathToKey(absolutePath));

        return itr != m_fileInfoMap.end();
    }

    void FileStateCache::AddInfoSet(QSet<AssetFileInfo> infoSet)
    {
        LockGuardType scopeLock(m_mapMutex);
        for (const AssetFileInfo& info : infoSet)
        {
            m_fileInfoMap[PathToKey(info.m_filePath)] = FileStateInfo(info);
        }
    }

    void FileStateCache::AddFile(const QString& absolutePath)
    {
        QFileInfo fileInfo(absolutePath);
        LockGuardType scopeLock(m_mapMutex);

        AddOrUpdateFileInternal(fileInfo);

        if(fileInfo.isDir())
        {
            ScanFolder(absolutePath);
        }
    }

    void FileStateCache::UpdateFile(const QString& absolutePath)
    {
        QFileInfo fileInfo(absolutePath);
        LockGuardType scopeLock(m_mapMutex);

        AddOrUpdateFileInternal(fileInfo);
    }

    void FileStateCache::RemoveFile(const QString& absolutePath)
    {
        LockGuardType scopeLock(m_mapMutex);

        auto itr = m_fileInfoMap.find(PathToKey(absolutePath));

        if (itr != m_fileInfoMap.end())
        {
            bool isDirectory = itr.value().m_isDirectory;
            QString parentPath = itr.value().m_absolutePath;
            m_fileInfoMap.erase(itr);

            if (isDirectory)
            {
                for (itr = m_fileInfoMap.begin(); itr != m_fileInfoMap.end(); )
                {
                    if (itr.value().m_absolutePath.startsWith(parentPath))
                    {
                        itr = m_fileInfoMap.erase(itr);
                        continue;
                    }

                    itr++;
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////

    QString FileStateCache::PathToKey(const QString& absolutePath) const
    {
        QString normalized = AssetUtilities::NormalizeFilePath(absolutePath);

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_APPLE_OSX)
        // These file systems are not case-sensitive, so the cache should not be either
        return normalized.toLower();
#else
        return normalized;
#endif
    }

    void FileStateCache::AddOrUpdateFileInternal(QFileInfo fileInfo)
    {
        m_fileInfoMap[PathToKey(fileInfo.absoluteFilePath())] = FileStateInfo(fileInfo.absoluteFilePath(), fileInfo.lastModified(), fileInfo.size(), fileInfo.isDir());
    }

    void FileStateCache::ScanFolder(const QString& absolutePath)
    {
        QDir inputFolder(absolutePath);
        QFileInfoList entries = inputFolder.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Files);

        for (const QFileInfo& entry : entries)
        {
            AddOrUpdateFileInternal(entry);

            if (entry.isDir())
            {
                ScanFolder(entry.absoluteFilePath());
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////

    bool FileStatePassthrough::GetFileInfo(const QString& absolutePath, FileStateInfo* foundFileInfo) const
    {
        QFileInfo fileInfo(absolutePath);

        if (fileInfo.exists())
        {
            *foundFileInfo = FileStateInfo(fileInfo.absoluteFilePath(), fileInfo.lastModified(), fileInfo.size(), fileInfo.isDir());

            return true;
        }

        return false;
    }

    bool FileStatePassthrough::Exists(const QString& absolutePath) const
    {
        return QFile(absolutePath).exists();
    }

    bool FileStateInfo::operator==(const FileStateInfo& rhs) const
    {
        return m_absolutePath == rhs.m_absolutePath
            && m_modTime == rhs.m_modTime
            && m_fileSize == rhs.m_fileSize
            && m_isDirectory == rhs.m_isDirectory;
    }

} // namespace AssetProcessor