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

#include <AzCore/EBus/Ebus.h>
#include <native/AssetManager/assetScanFolderInfo.h>
#include <QString>
#include <QSet>
#include <QFileInfo>

namespace AssetProcessor
{
    struct FileStateInfo
    {
        FileStateInfo() = default;
        FileStateInfo(QString filePath, QDateTime modTime, AZ::u64 fileSize, bool isDirectory)
            : m_absolutePath(filePath), m_modTime(modTime), m_fileSize(fileSize), m_isDirectory(isDirectory) {}

        explicit FileStateInfo(const AssetFileInfo& assetFileInfo)
            : m_absolutePath(assetFileInfo.m_filePath), m_fileSize(assetFileInfo.m_fileSize), m_isDirectory(assetFileInfo.m_isDirectory), m_modTime(assetFileInfo.m_modTime)
        {

        }

        bool operator==(const FileStateInfo& rhs) const;

        QString m_absolutePath{};
        QDateTime m_modTime{};
        AZ::u64 m_fileSize{};
        bool m_isDirectory{};
    };

    class FileStateRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;
        static const bool LocklessDispatch = true; // FileStateCache is being fed information from outside the ebus so we have to do our own locking anyway

        /// Fetches info on the file/directory if it exists.  Returns true if it exists, false otherwise
        virtual bool GetFileInfo(const QString& absolutePath, FileStateInfo* foundFileInfo) const = 0;
        /// Convenience function to check if a file or directory exists.
        virtual bool Exists(const QString& absolutePath) const = 0;
    };

    using FileStateRequestBus = AZ::EBus<FileStateRequests>;

    class FileStateBase
        : public FileStateRequestBus::Handler
    {
    public:
        FileStateBase()
        {
            BusConnect();
        }

        virtual ~FileStateBase()
        {
            BusDisconnect();
        }

        /// Bulk adds file state to the cache
        virtual void AddInfoSet(QSet<AssetFileInfo> /*infoSet*/) {}

        /// Adds a single file to the cache.  This will query the OS for the current state
        virtual void AddFile(const QString& /*absolutePath*/) {}

        /// Updates a single file in the cache.  This will query the OS for the current state
        virtual void UpdateFile(const QString& /*absolutePath*/) {}

        /// Removes a file from the cache
        virtual void RemoveFile(const QString& /*absolutePath*/) {}
    };

    /// Caches file state information retrieved by the file scanner and file watcher
    /// Profiling has shown it is faster (at least on windows) compared to asking the OS for file information every time
    class FileStateCache final :
        public FileStateBase
    {
    public:

        // FileStateRequestBus implementation
        bool GetFileInfo(const QString& absolutePath, FileStateInfo* foundFileInfo) const override;
        bool Exists(const QString& absolutePath) const override;

        void AddInfoSet(QSet<AssetFileInfo> infoSet) override;
        void AddFile(const QString& absolutePath) override;
        void UpdateFile(const QString& absolutePath) override;
        void RemoveFile(const QString& absolutePath) override;

    private:

        /// Handles converting a file path into a uniform format for use as a map key
        QString PathToKey(const QString& absolutePath) const;

        /// Add/Update a single file
        void AddOrUpdateFileInternal(QFileInfo fileInfo);

        /// Recursively collects all the files contained in the directory specified by absolutePath
        void ScanFolder(const QString& absolutePath);

        mutable AZStd::recursive_mutex m_mapMutex;
        QHash<QString, FileStateInfo> m_fileInfoMap;

        using LockGuardType = AZStd::lock_guard<decltype(m_mapMutex)>;
    };

    /// Pass through version of the FileStateCache which does not cache anything.  Every request is redirected to the OS
    class FileStatePassthrough final :
        public FileStateBase
    {
    public:
        // FileStateRequestBus implementation
        bool GetFileInfo(const QString& absolutePath, FileStateInfo* foundFileInfo) const override;
        bool Exists(const QString& absolutePath) const override;
    };
} // namespace AssetProcessor