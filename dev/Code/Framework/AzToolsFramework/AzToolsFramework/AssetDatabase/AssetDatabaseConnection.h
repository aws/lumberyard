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
#ifndef AZTOOLSFRAMEWORK_Connection_H
#define AZTOOLSFRAMEWORK_Connection_H
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/bitset.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

// At the time of writing, AZStd::function does not support RVALUE-Refs.  We use std::function instead.
#include <functional>

namespace AzToolsFramework
{
    namespace SQLite
    {
        class Connection;
        class Statement;
    } // namespace SQLite

    namespace AssetDatabase
    {
        //! List all database version changes here with descriptive naming to explain what was changed in
        //! the database
        enum class DatabaseVersion : int
        {
            DatabaseDoesNotExist = -1,
            StartingVersion = 2,
            AddedIndices = 3, // the version where we added the indexes
            AddedJobLogTable = 4, // added job log table to the database
            // skipping up to 7 internally so that old tables are cleared.
            NewTables = 7, // the version where we tables refactored to scanfolder/source/job/product
            AddedOutputPrefixToScanFolders = 8,
            AddedJobKeyIndex = 9, // the version where we add jobKey index on job table
            AddedSourceGuidIndex = 10,
            AddedSourceDependencyTable = 11,
            AddedLegacySubIDsTable = 12, // the version where we added the table that contains a map of product -> previously known aliases
            AddedProductDependencyTable = 13,
            ClearAutoSucceedJobs = 14, // version bump to clear out AutoSucceed jobs from the database (they were never intended to be there in the first place)
            AddedFilesTable = 15,
            AddedAnalysisFingerprint = 16,
            AddedSourceDependencyType = 17,
            AddedFileModTimes = 18,
            AddedUnresolvedDependencyField = 19, // Add unresolved path field to dependency table
            AddedUnresolvedDependencyTypeField = 20,
            AddedTypeOfDependencyIndex = 21,
            AddedProductDependencyPlatform = 22,
            //Add all new versions before this
            DatabaseVersionCount,
            LatestVersion = DatabaseVersionCount - 1
        };

        //////////////////////////////////////////////////////////////////////////
        //DatabaseInfoEntry
        class DatabaseInfoEntry
        {
        public:
            DatabaseInfoEntry() = default;
            DatabaseInfoEntry(AZ::s64 rowID, DatabaseVersion version);

            auto GetColumns();

            AZ::s64 m_rowID = -1;
            DatabaseVersion m_version = DatabaseVersion::DatabaseDoesNotExist;
        };

        typedef AZStd::vector<DatabaseInfoEntry> DatabaseInfoEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //ScanFolderDatabaseEntry
        class ScanFolderDatabaseEntry
        {
        public:
            ScanFolderDatabaseEntry() = default;
            ScanFolderDatabaseEntry(AZ::s64 scanFolderID,
                const char* scanFolder,
                const char* displayName,
                const char* portableKey,
                const char* outputPrefix,
                int isRoot = 0);
            ScanFolderDatabaseEntry(const char* scanFolder,
                const char* displayName,
                const char* portableKey,
                const char* outputPrefix,
                int isRoot = 0);
            ScanFolderDatabaseEntry(const ScanFolderDatabaseEntry& other);
            ScanFolderDatabaseEntry(ScanFolderDatabaseEntry&& other);

            ScanFolderDatabaseEntry& operator=(ScanFolderDatabaseEntry&& other);
            ScanFolderDatabaseEntry& operator=(const ScanFolderDatabaseEntry& other);
            bool operator==(const ScanFolderDatabaseEntry& other) const;

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_scanFolderID = -1; // the database Primary Key row index, not to be used for any logic purpose
            AZStd::string m_scanFolder; // the actual local computer path to that scan folder.
            AZStd::string m_displayName; // a display name, blank means it should not show up in UIs
            AZStd::string m_portableKey; // a key that uniquely identifies a scan folder so that we can recognize the same one in other databases/computer
            AZStd::string m_outputPrefix;
            int m_isRoot;
        };

        typedef AZStd::vector<ScanFolderDatabaseEntry> ScanFolderDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //SourceDatabaseEntry
        class SourceDatabaseEntry
        {
        public:
            SourceDatabaseEntry() = default;
            SourceDatabaseEntry(AZ::s64 sourceID, AZ::s64 scanFolderPK, const char* sourceName, AZ::Uuid sourceGuid, const char* analysisFingerprint);
            SourceDatabaseEntry(AZ::s64 scanFolderPK, const char* sourceName, AZ::Uuid sourceGuid, const char* analysisFingerprint);

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_sourceID = -1;
            AZ::s64 m_scanFolderPK = -1;
            AZStd::string m_sourceName;
            AZ::Uuid m_sourceGuid = AZ::Uuid::CreateNull();
            AZStd::string m_analysisFingerprint;
        };

        typedef AZStd::vector<SourceDatabaseEntry> SourceDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        class BuilderInfoEntry
        {
        public:
            BuilderInfoEntry() = default;
            BuilderInfoEntry(AZ::s64 builderInfoID, const AZ::Uuid& builderUuid, const char* analysisFingerprint);
            AZ::s64 m_builderInfoID = -1;
            AZ::Uuid m_builderUuid = AZ::Uuid::CreateNull();
            AZStd::string m_analysisFingerprint;

            AZStd::string ToString() const;
            auto GetColumns();
        };

        using BuilderInfoEntryContainer = AZStd::vector<BuilderInfoEntry>;

        //////////////////////////////////////////////////////////////////////////
        //JobDatabaseEntry
        class JobDatabaseEntry
        {
        public:
            JobDatabaseEntry() = default;
            JobDatabaseEntry(AZ::s64 jobID, AZ::s64 sourcePK, const char* jobKey, AZ::u32 fingerprint, const char* platform, AZ::Uuid builderGuid, AssetSystem::JobStatus status, AZ::u64 jobRunKey, AZ::s64 firstFailLogTime = 0, const char* firstFailLogFile = nullptr, AZ::s64 lastFailLogTime = 0, const char* lastFailLogFile = nullptr, AZ::s64 lastLogTime = 0, const char* lastLogFile = nullptr);
            JobDatabaseEntry(AZ::s64 sourcePK, const char* jobKey, AZ::u32 fingerprint, const char* platform, AZ::Uuid builderGuid, AssetSystem::JobStatus status, AZ::u64 jobRunKey, AZ::s64 firstFailLogTime = 0, const char* firstFailLogFile = nullptr, AZ::s64 lastFailLogTime = 0, const char* lastFailLogFile = nullptr, AZ::s64 lastLogTime = 0, const char* lastLogFile = nullptr);
            JobDatabaseEntry(const JobDatabaseEntry& other);
            JobDatabaseEntry(JobDatabaseEntry&& other);

            JobDatabaseEntry& operator=(JobDatabaseEntry&& other);
            JobDatabaseEntry& operator=(const JobDatabaseEntry& other);
            bool operator==(const JobDatabaseEntry& other) const;

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_jobID = -1;
            AZ::s64 m_sourcePK = -1;
            AZStd::string m_jobKey;
            AZ::u32 m_fingerprint = 0;
            AZStd::string m_platform;
            AZ::Uuid m_builderGuid;
            AssetSystem::JobStatus m_status = AssetSystem::JobStatus::Queued;
            AZ::u64 m_jobRunKey = 0;
            AZ::s64 m_firstFailLogTime = 0;
            AZStd::string m_firstFailLogFile;
            AZ::s64 m_lastFailLogTime = 0;
            AZStd::string m_lastFailLogFile;
            AZ::s64 m_lastLogTime = 0;
            AZStd::string m_lastLogFile;
        };

        typedef AZStd::vector<JobDatabaseEntry> JobDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //SourceFileDependencyEntry
        class SourceFileDependencyEntry
        {
        public:
            /// This is also used as a bitset when making queries.
            enum TypeOfDependency : AZ::u32
            {
                DEP_SourceToSource  = 1 << 0, ///< source file depends on other source file
                DEP_JobToJob        = 1 << 1, ///< job depends on other job
                DEP_SourceOrJob     = (DEP_SourceToSource | DEP_JobToJob),
                DEP_SourceLikeMatch = 1 << 2, ///< Allow wildcard matches using LIKE
                DEP_Any             = 0xFFFFFFFF ///< convenience value - not allowed to write this to the actual DB.
            };
            
            SourceFileDependencyEntry() = default;
            SourceFileDependencyEntry(AZ::Uuid builderGuid, const char* source, const char* dependsOnSource, TypeOfDependency dependencyType);

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_sourceDependencyID = -1;
            AZ::Uuid m_builderGuid = AZ::Uuid::CreateNull();
            TypeOfDependency m_typeOfDependency = DEP_SourceToSource;
            AZStd::string m_source;
            AZStd::string m_dependsOnSource;
        };

        typedef AZStd::vector<SourceFileDependencyEntry> SourceFileDependencyEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //ProductDatabaseEntry
        class ProductDatabaseEntry
        {
        public:
            ProductDatabaseEntry() = default;
            ProductDatabaseEntry(AZ::s64 productID, AZ::s64 jobPK,  AZ::u32 subID, const char* productName,
                AZ::Data::AssetType assetType, AZ::Uuid legacyGuid = AZ::Uuid::CreateNull());
            ProductDatabaseEntry(AZ::s64 jobPK, AZ::u32 subID, const char* productName,
                AZ::Data::AssetType assetType, AZ::Uuid legacyGuid = AZ::Uuid::CreateNull());
            ProductDatabaseEntry(const ProductDatabaseEntry& entry);
            ProductDatabaseEntry(ProductDatabaseEntry&& other);

            ProductDatabaseEntry& operator=(ProductDatabaseEntry&& other);
            ProductDatabaseEntry& operator=(const ProductDatabaseEntry& other);
            bool operator==(const ProductDatabaseEntry& other) const;

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_productID = -1;
            AZ::s64 m_jobPK = -1;
            AZ::u32 m_subID = 0;
            AZStd::string m_productName;
            AZ::Data::AssetType m_assetType = AZ::Data::AssetType::CreateNull();
            AZ::Uuid m_legacyGuid = AZ::Uuid::CreateNull();//used only for backward compatibility with old product guid, is generated based on product name
        };
        typedef AZStd::vector<ProductDatabaseEntry> ProductDatabaseEntryContainer;

        class LegacySubIDsEntry
        {
        public:
            LegacySubIDsEntry() = default;
            LegacySubIDsEntry(AZ::s64 subIDsEntryID, AZ::s64 productPK, AZ::u32 subId); // loaded from db, and thus includes the PK
            LegacySubIDsEntry(AZ::s64 productPK, AZ::u32 subId);  // being synthesized does not include the PK.
            LegacySubIDsEntry(const LegacySubIDsEntry& entry) = default;
            LegacySubIDsEntry& operator=(const LegacySubIDsEntry& other) = default;

            auto GetColumns();

            AZ::s64 m_subIDsEntryID = -1; // the main ID of this table.
            AZ::s64 m_productPK = -1; // the foreign key to the Products table.
            AZ::u32 m_subID = 0; // the legacy subID.
        };

        typedef AZStd::vector<LegacySubIDsEntry> LegacySubIDsEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //ProductDependencyDatabaseEntry
        class ProductDependencyDatabaseEntry
        {
        public:
            enum DependencyType : AZ::u32
            {
                ProductDep_ProductFile = 0,
                ProductDep_SourceFile
            };

            ProductDependencyDatabaseEntry() = default;
            ProductDependencyDatabaseEntry(AZ::s64 productDependencyID, AZ::s64 productPK, AZ::Uuid dependencySourceGuid, AZ::u32 dependencySubID, AZStd::bitset<64> dependencyFlags, const AZStd::string& platform, const AZStd::string& unresolvedPath, DependencyType dependencyType);
            ProductDependencyDatabaseEntry(AZ::s64 productPK, AZ::Uuid dependencySourceGuid, AZ::u32 dependencySubID, AZStd::bitset<64> dependencyFlags, const AZStd::string& platform, const AZStd::string& unresolvedPath="", DependencyType dependencyType = DependencyType::ProductDep_ProductFile);
            
            bool operator == (const ProductDependencyDatabaseEntry& other) const;

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_productDependencyID = -1;
            AZ::s64 m_productPK = -1;
            AZ::Uuid m_dependencySourceGuid = AZ::Uuid::CreateNull();
            AZ::u32 m_dependencySubID = 0;
            AZStd::bitset<64> m_dependencyFlags = 0;
            AZStd::string m_unresolvedPath;
            AZStd::string m_platform;
            DependencyType m_dependencyType = DependencyType::ProductDep_ProductFile;
        };

        typedef AZStd::vector<ProductDependencyDatabaseEntry> ProductDependencyDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //CombinedDatabaseEntry
        class CombinedDatabaseEntry
            : public ScanFolderDatabaseEntry
            , public SourceDatabaseEntry
            , public JobDatabaseEntry
            , public ProductDatabaseEntry
        {
        public:
            // allow it to default move operators and etc.
            bool operator==(const CombinedDatabaseEntry& other) = delete; // its meaningless to compare these
            LegacySubIDsEntryContainer m_legacySubIDs;

            auto GetColumns();
        };

        typedef AZStd::vector<CombinedDatabaseEntry> CombinedDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //FileDatabaseEntry
        class FileDatabaseEntry
        {
        public:
            FileDatabaseEntry() = default;

            FileDatabaseEntry(const FileDatabaseEntry& other) = default;
            FileDatabaseEntry(FileDatabaseEntry&& other) = default;

            FileDatabaseEntry& operator=(FileDatabaseEntry&& other) = default;
            FileDatabaseEntry& operator=(const FileDatabaseEntry& other) = default;
            bool operator==(const FileDatabaseEntry& other) const;

            AZStd::string ToString() const;
            auto GetColumns();

            AZ::s64 m_fileID = -1;
            AZ::s64 m_scanFolderPK = -1;
            AZStd::string m_fileName;
            int m_isFolder;
            AZ::u64 m_modTime{};
        };

        typedef AZStd::vector<FileDatabaseEntry> FileDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //SourceAndScanFolderDatabaseEntry
        class SourceAndScanFolderDatabaseEntry
            : public ScanFolderDatabaseEntry
            , public SourceDatabaseEntry
        {
        public:
            auto GetColumns();
        };

        typedef AZStd::vector<SourceAndScanFolderDatabaseEntry> SourceAndScanFolderDatabaseEntryContainer;

        //////////////////////////////////////////////////////////////////////////
        //AssetDatabaseConnection
        //! The Connection class represents a read-only connection to the asset database specifically
        //! (as opposed to a sql connection). Things like the Asset Processor derive from this in order
        //! to provide read-write access and curate the database. You may have as many of these
        //! connections as you wish, each represents a single connection to the asset database for use by
        //! a specific thread.
        //! All general read only queries are added here. Since this is meant for tools, we only care
        //! about queries which return data here, no modifications allowed here. Currently the Asset
        //! Processor is the only program allowed to modify the database. As such it derives from this
        //! class and overrides ReadOnly to return false which allows it to write / curate the database.
        //! However most systems only need to read and be notified of changes, so when they derive
        //! they don't override anything but CreateStatements to add specific queries that only make
        //! sense for their specific purpose, otherwise that statement should be added to this class so
        //! it can be reused by any system that needs it. Note that if a system needs read only access
        //! and has no special query needs, then this class can be used directly.
        class AssetDatabaseConnection
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetDatabaseConnection, AZ::SystemAllocator, 0);

            AssetDatabaseConnection();
            virtual ~AssetDatabaseConnection();


            //Open / Close the database
            bool OpenDatabase();
            void CloseDatabase();

            //These only need to be overridden by derived class if write access is needed, such as by the
            //asset processor.
            virtual bool IsReadOnly() const
            {
                return true;
            }
            virtual bool PostOpenDatabase(); // used by Asset Processor to upgrade the database

            //returns the current version of the code used for reading the database
            //this is how we know we have to upgrade old databases to the new ones
            virtual DatabaseVersion CurrentDatabaseVersion() const
            {
                return DatabaseVersion::LatestVersion; // DO NOT alter this value, add to the enumeration instead
            }

            //////////////////////////////////////////////////////////////////////////
            //handlers
            using databaseInfoHandler = AZStd::function<bool(DatabaseInfoEntry& entry)>;
            using scanFolderHandler = AZStd::function<bool(ScanFolderDatabaseEntry& entry)>;
            using sourceHandler = AZStd::function<bool(SourceDatabaseEntry& entry)>;
            using combinedSourceScanFolderHandler = AZStd::function<bool(SourceAndScanFolderDatabaseEntry& entry)>;
            using jobHandler = AZStd::function<bool(JobDatabaseEntry& entry)>;
            using productHandler = AZStd::function<bool(ProductDatabaseEntry& entry)>;
            using combinedHandler = AZStd::function<bool(CombinedDatabaseEntry& entry)>;
            using jobInfoHandler = AZStd::function<bool(AzToolsFramework::AssetSystem::JobInfo& job)>;
            using sourceFileDependencyHandler = AZStd::function<bool(SourceFileDependencyEntry& entry)>;
            using legacySubIDsHandler = AZStd::function<bool(LegacySubIDsEntry& entry)>;
            using productDependencyHandler = AZStd::function<bool(ProductDependencyDatabaseEntry& entry)>;
            using combinedProductDependencyHandler = AZStd::function<bool(AZ::Data::AssetId& asset, ProductDependencyDatabaseEntry& entry)>;
            // note that AZStd::function cannot handle rvalue-refs at the time of writing this.
            using BuilderInfoHandler = std::function<bool(BuilderInfoEntry&&)>;
            using fileHandler = AZStd::function<bool(FileDatabaseEntry& entry)>;

            //////////////////////////////////////////////////////////////////
            //Query entire table
            bool QueryDatabaseInfoTable(databaseInfoHandler handler);
            DatabaseVersion QueryDatabaseVersion();//convenience function to return the version field in the database
            bool QueryScanFoldersTable(scanFolderHandler handler);
            bool QuerySourcesTable(sourceHandler handler);
            bool QueryJobsTable(jobHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductsTable(productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductDependenciesTable(combinedProductDependencyHandler handler);
            bool QueryBuilderInfoTable(const BuilderInfoHandler& handler);
            bool QueryFilesTable(fileHandler handler);

            //////////////////////////////////////////////////////////////////////////
            //Queries
            //NOTE: There are 2 variations of string equivalence in the database:
            //"By": Means the strings are exactly the same.
            //"Like": Raw LIKE sql query that takes SQL % wild card and _ wild space.
            enum LikeType
            {
                Raw,
                StartsWith,
                EndsWith,
                Matches,
            };
            AZStd::string GetLikeActualSearchTerm(const char* likeString, LikeType likeType);
            //NOTE: The return bool is always whether the database query succeeded. If you
            //succeeded in querying the database but it does not exist in the database it will return true.
            //NOTE: If a query returns true you are guaranteed to have at least one output. However, if it fails
            //you MAY still have some output as it could be the result of multiple internal queries.
            //NOTE: Also these functions are all ADDITIVE, meaning if you pass a container in
            //and the query results in outputs, they are added to whats already there, they never delete
            //before adding outputs.
            //NOTE: A connection is NON MUTATING. This class CAN NOT modify the database in any way whatsoever.
            //This class is to provide a flexible read only interface for people to use. If you need to
            //Write then you should derive from this class and and write your writing code there. The
            //derived class may wrap or even hide this classes interface, but should follow the same rules.
            //NOTE: Any query to do with a non specific job or product has a optional platform filter.
            //NOTE: All combined query also has an optional jobKey filter, except the specific product queries
            //NOTE: Any additional filtering is probably best done in code as permutations of the SQL will
            //most likely not result in any significant speed boost and could easily become unmanageable.

            //scan folder
            bool QueryScanFolderByScanFolderID(AZ::s64 scanfolderID, scanFolderHandler handler);
            bool QueryScanFolderBySourceID(AZ::s64 sourceID, scanFolderHandler handler);
            bool QueryScanFolderByJobID(AZ::s64 jobID, scanFolderHandler handler);
            bool QueryScanFolderByProductID(AZ::s64 productID, scanFolderHandler handler);

            bool QueryScanFolderByDisplayName(const char* displayname, scanFolderHandler handler);
            bool QueryScanFolderByPortableKey(const char* portableKey, scanFolderHandler handler);

            //source
            bool QuerySourceBySourceID(AZ::s64 sourceID, sourceHandler handler);
            bool QuerySourceByScanFolderID(AZ::s64 scanFolderID, sourceHandler handler);
            bool QuerySourceByJobID(AZ::s64 jobID, sourceHandler handler);
            bool QuerySourceByProductID(AZ::s64 productID, sourceHandler handler);
            bool QuerySourceBySourceGuid(AZ::Uuid sourceGuid, sourceHandler handler);

            bool QuerySourceBySourceName(const char* exactSourceName, sourceHandler handler);
            bool QuerySourceBySourceNameScanFolderID(const char* exactSourceName, AZ::s64 scanFolderID, sourceHandler handler);
            bool QuerySourceLikeSourceName(const char* likeSourceName, LikeType likeType, sourceHandler handler);
            bool QuerySourceAnalysisFingerprint(const char* exactSourceName, AZ::s64 scanFolderID, AZStd::string& result);
            bool QuerySourceAndScanfolder(combinedSourceScanFolderHandler handler);

            //job
            bool QueryJobByJobID(AZ::s64 jobID, jobHandler handler);
            bool QueryJobByJobKey(AZStd::string jobKey, jobHandler handler);
            bool QueryJobByJobRunKey(AZ::u64 jobRunKey, jobHandler handler);
            bool QueryJobByProductID(AZ::s64 jobID, jobHandler handler);
            bool QueryJobBySourceID(AZ::s64 sourceID, jobHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);

            //product
            bool QueryProductByProductID(AZ::s64 productID, productHandler handler);
            bool QueryProductByJobID(AZ::s64 jobID, productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductBySourceID(AZ::s64 sourceID, productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductBySourceGuidSubID(AZ::Uuid sourceGuid, AZ::u32 productSubID, productHandler handler);

            bool QueryProductByProductName(const char* exactProductName, productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductLikeProductName(const char* likeProductName, LikeType likeType, productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);

            bool QueryProductBySourceName(const char* exactSourceName, productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductLikeSourceName(const char* likeSourceName, LikeType likeType, productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryProductByJobIDSubID(AZ::s64 jobID, AZ::u32 subId, productHandler handler);

            // legacy SubIDs
            bool QueryLegacySubIdsByProductID(AZ::s64 productId, legacySubIDsHandler handler);

            //combined scan folder/source/job/product
            bool QueryCombined(combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any, bool includeLegacySubIDs = false);
            bool QueryCombinedBySourceID(AZ::s64 sourceID, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryCombinedByJobID(AZ::s64 jobID, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryCombinedByProductID(AZ::s64 productID, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryCombinedBySourceGuidProductSubId(AZ::Uuid sourceGuid, AZ::u32 productSubID, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);

            bool QueryCombinedBySourceName(const char* exactSourceName, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryCombinedLikeSourceName(const char* likeSourceName, LikeType likeType, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);

            bool QueryCombinedByProductName(const char* productName, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool QueryCombinedLikeProductName(const char* productName, LikeType likeType, combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);

            //jobinfo
            bool QueryJobInfoByJobID(AZ::s64 jobID, jobInfoHandler handler);
            bool QueryJobInfoByJobRunKey(AZ::u64 jobRunKey, jobInfoHandler handler);
            bool QueryJobInfoByJobKey(AZStd::string jobKey, jobInfoHandler handler);
            bool QueryJobInfoBySourceName(const char* sourceName, jobInfoHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, const char* platform = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);

            //SourceDependency
            /// direct query - look up table row by row ID
            bool QuerySourceDependencyBySourceDependencyId(AZ::s64 sourceDependencyID, sourceFileDependencyHandler handler);

            /** reverse dependencies (query sources which depend on 'dependsOnSource').
            *   Optional nullable 'dependentFilter' filters it to only resulting sources which are LIKE the filter.
            */
            bool QuerySourceDependencyByDependsOnSource(const char* dependsOnSource, const char* dependentFilter, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::TypeOfDependency dependencyType, sourceFileDependencyHandler handler);

            /** Attempt to match either DEP_SourcetoSource or DEP_JobToJob
            *   Then allow DEP_SourceLikeMatch with Wildcard characters
            *   Optional nullable 'dependentFilter' filters it to only resulting sources which are LIKE the filter.
            */
            bool QuerySourceDependencyByDependsOnSourceWildcard(const char* dependsOnSource, const char* dependentFilter, sourceFileDependencyHandler handler);

            /** forward dependencies (query everything 'sourceDependency' depends on)
            *   Optional nullable 'dependentFilter' filters it to only resulting dependendencies which are LIKE the filter.
            */
            bool QueryDependsOnSourceBySourceDependency(const char* sourceDependency, const char* dependencyFilter, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::TypeOfDependency dependencyType, sourceFileDependencyHandler handler);

            //ProductDependencies
            bool QueryProductDependencyByProductDependencyId(AZ::s64 productDependencyID, productDependencyHandler handler);
            bool QueryProductDependencyByProductId(AZ::s64 productID, productDependencyHandler handler);
            bool QueryDirectProductDependencies(AZ::s64 productID, productHandler handler);
            bool QueryAllProductDependencies(AZ::s64 productID, productHandler handler);
            bool QueryUnresolvedProductDependencies(productDependencyHandler handler);

            //FileInfo
            bool QueryFileByFileID(AZ::s64 fileID, fileHandler handler);
            bool QueryFilesByFileNameAndScanFolderID(const char* fileName, AZ::s64 scanfolderID, fileHandler handler);
            bool QueryFilesLikeFileName(const char* likeFileName, LikeType likeType, fileHandler handler);
            bool QueryFilesByScanFolderID(AZ::s64 scanFolderID, fileHandler handler);
            bool QueryFileByFileNameScanFolderID(const char* fileName, AZ::s64 scanFolderID, fileHandler handler);
            //////////////////////////////////////////////////////////////////////////

        protected:
            
            SQLite::Connection* m_databaseConnection;

            //override in your derived class when you need to add specific queries
            virtual void CreateStatements(); // when overriding call the base first then add additional statements

            //return the path to the database
            virtual AZStd::string GetAssetDatabaseFilePath();

            //validation
            bool ValidateDatabaseTable(const char* callName, const char* tableName);

            //boiler plate
            auto GetCombinedResultAsLambda(); // helper to wrap up GetCombinedResult in a lambda, since GetCombinedResult is a member function
            bool GetCombinedResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::combinedHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any, bool includeLegacyAssetIDs = false);

            // cache what tables have been validated.  it is assumed that AP is the only app with read-write access to the table, and only
            // one AP can be running on a branch at any given time.  Therefore once the table is validated, there is no reason to continue validating it
            // before every query, since validating it essentially must makes sure it exists.
            AZStd::unordered_set<AZStd::string> m_validatedTables;
        };

        namespace
        {
            //boiler plate
            bool GetDatabaseInfoResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::databaseInfoHandler handler);
            bool GetScanFolderResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::scanFolderHandler handler);
            bool GetSourceResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::sourceHandler handler);
            bool GetSourceAndScanfolderResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::combinedSourceScanFolderHandler handler);
            bool GetSourceDependencyResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::sourceFileDependencyHandler handler);
            bool GetJobResultSimple(const char* name, SQLite::Statement* statement, AssetDatabaseConnection::jobHandler handler);
            bool GetJobResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::jobHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool GetProductResultSimple(const char* name, SQLite::Statement* statement, AssetDatabaseConnection::productHandler handler);
            bool GetProductResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::productHandler handler, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), const char* jobKey = nullptr, AssetSystem::JobStatus status = AssetSystem::JobStatus::Any);
            bool GetLegacySubIDsResult(const char* callname, SQLite::Statement* statement, AssetDatabaseConnection::legacySubIDsHandler handler);
            bool GetProductDependencyResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::productDependencyHandler handler);
            bool GetCombinedDependencyResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::combinedProductDependencyHandler handler);
            bool GetFileResult(const char* callName, SQLite::Statement* statement, AssetDatabaseConnection::fileHandler handler);
        }
    } // namespace AssetDatabase
}// namespace AzToolsFramework

#endif // AZTOOLSFRAMEWORK_Connection_H