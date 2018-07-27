#ifndef ASSETPROCESSOR_ASSETDATABASE_H
#define ASSETPROCESSOR_ASSETDATABASE_H

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


#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>

#include <QtCore/QSet>
#include <QtCore/QString>

#include "AzToolsFramework/API/EditorAssetSystemAPI.h"

class QStringList;

namespace AssetProcessor
{
    //! the Asset Processor's database manager's job is to create and modify the actual underlying
    //! SQL database. All queries to make changes to the database go through here.  This includes
    //! connecting to existing database and altering or creating database tables, etc.
    class AssetDatabaseConnection
        : public AzToolsFramework::AssetDatabase::AssetDatabaseConnection
    {
    public:
        AZ_CLASS_ALLOCATOR(AssetDatabaseConnection, AZ::SystemAllocator, 0);

        AssetDatabaseConnection();
        ~AssetDatabaseConnection();

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetDatabase::Connection
    public:
        bool IsReadOnly() const override
        { 
            return false;// return false, we actually curate/write to this database.
        } 
        void VacuumAndAnalyze();

    protected:
        void CreateStatements() override;
        bool PostOpenDatabase() override;
        //////////////////////////////////////////////////////////////////////////

    public:
        bool DataExists();
        void LoadData();
        void ClearData();

        //////////////////////////////////////////////////////////////////////////
        //Queries
        //NOTE: When passing in a structure to the Set<> functions, a default constructed structure has -1 for
        //the table generated id and is filled in by the query, that is why it is passed by non const reference.
        //For instance when SetSource(scanFolderEntry); is called it evaluates the query and fills in the
        //main m_sourceID from the query. If you pass in a structure with the id already filled in, i.e. not -1,
        //it is interpreted as an update to the database only if the contents differ in any way from whats
        //already in he database. Obviously if it is filled in and does not exist it returns false.
        //NOTE: The return bool for these queries only return true if both the query succeeded AND you got a result

        //////////////////////////////////////////////////////////////////////////
        //scan folders
        bool GetScanFolders(AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntryContainer& container);
        bool GetScanFolderByScanFolderID(AZ::s64 scanFolderID, AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry);
        bool GetScanFolderBySourceID(AZ::s64 sourceID, AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry);
        bool GetScanFolderByJobID(AZ::s64 jobID, AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry);
        bool GetScanFolderByProductID(AZ::s64 productID, AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry);

        bool GetScanFolderByPortableKey(QString portableKey, AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry);
        bool SetScanFolder(AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry); //on success sets scanfolderID, if already exists updates it
        bool RemoveScanFolder(AZ::s64 scanFolderID);
        bool RemoveScanFolders(AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntryContainer& container);

        //sources
        bool GetSources(AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer& container);

        bool GetSourceBySourceGuid(AZ::Uuid sourceGuid, AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry);

        bool GetSourceBySourceID(AZ::s64 sourceID, AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry);
        bool GetSourcesBySourceName(QString exactSourceName, AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer& source);
        bool GetSourcesBySourceNameScanFolderId(QString exactSourceName, AZ::s64 scanFolderID, AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer& source);
        bool GetSourcesLikeSourceName(QString likeSourceName, LikeType likeType, AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer& container);

        bool GetSourceByJobID(AZ::s64 jobID, AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry);

        bool GetSourceByProductID(AZ::s64 productID, AzToolsFramework::AssetDatabase::SourceDatabaseEntry& source);
        bool GetSourcesByProductName(QString exactProductName, AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer& container);
        bool GetSourcesLikeProductName(QString likeProductName, LikeType likeType, AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer& container);

        bool SetSource(AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry); //on success sets sourceID, if it already exists updates it
        bool RemoveSource(AZ::s64 sourceID);
        bool RemoveSources(AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer& container);
        bool RemoveSourcesByScanFolderID(AZ::s64 scanFolderID);

        //jobs
        
        // used to initialize the predictor for job Run Keys
        AZ::s64 GetHighestJobRunKey(); 
        bool GetJobs(AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetJobByJobID(AZ::s64 jobID, AzToolsFramework::AssetDatabase::JobDatabaseEntry& entry);
        bool GetJobByProductID(AZ::s64 productID, AzToolsFramework::AssetDatabase::JobDatabaseEntry& entry);

        bool GetJobsBySourceID(AZ::s64 sourceID, AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetJobsBySourceName(QString exactSourceName, AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetJobsLikeSourceName(QString likeSourceName, LikeType likeType, AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);

        bool GetJobsByProductName(QString exactProductName, AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetJobsLikeProductName(QString likeProductName, LikeType likeType, AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        
        bool SetJob(AzToolsFramework::AssetDatabase::JobDatabaseEntry& entry); //on success sets jobID, if it already exists updates it
        bool RemoveJob(AZ::s64 jobID);
        bool RemoveJobs(AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer& container);
        bool RemoveJobByProductID(AZ::s64 productID);

        //products
        bool GetProducts(AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetProductsByJobID(AZ::s64 jobID, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container);
        // note that the pair of (JobID, SubID) uniquely identifies a single job, and thus the result is always only one entry:
        bool GetProductByJobIDSubId(AZ::s64 jobID, AZ::u32 subID, AzToolsFramework::AssetDatabase::ProductDatabaseEntry& result);
        
        bool GetProductByProductID(AZ::s64 productID, AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry);
        bool GetProductsByProductName(QString exactProductName, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetProductsLikeProductName(QString likeProductName, LikeType likeType, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        
        bool GetProductsBySourceID(AZ::s64 sourceID, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetProductsBySourceName(QString exactSourceName, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        bool GetProductsLikeSourceName(QString likeSourceName, LikeType likeType, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);
        
        bool SetProduct(AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry); //on success sets productID, if it already exists updates it
        bool SetProducts(AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container); //on success sets productID, if it already exists updates it
        bool RemoveProducts(AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container);
        bool RemoveProduct(AZ::s64 productID);
        bool RemoveProductsByJobID(AZ::s64 jobID);
        bool RemoveProductsBySourceID(AZ::s64 sourceID, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);

        //jobinfo
        bool GetJobInfoByJobID(AZ::s64 jobID, AzToolsFramework::AssetSystem::JobInfo& jobInfo);
        bool GetJobInfoByJobKey(AZStd::string jobKey, AzToolsFramework::AssetSystem::JobInfoContainer& container);
        bool GetJobInfoByJobRunKey(AZ::u64 jobRunKey, AzToolsFramework::AssetSystem::JobInfoContainer& container);
        bool GetJobInfoBySourceName(QString exactSourceName, AzToolsFramework::AssetSystem::JobInfoContainer& container, AZ::Uuid builderGuid = AZ::Uuid::CreateNull(), QString jobKey = QString(), QString platform = QString(), AzToolsFramework::AssetSystem::JobStatus status = AzToolsFramework::AssetSystem::JobStatus::Any);

        //SourceFileDependency
        bool SetSourceFileDependency(AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& entry);
        bool SetSourceFileDependencies(AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& container);

        //! Finds all records where other files depend on 'dependsOnSource'
        bool GetSourceFileDependenciesByDependsOnSource(const QString& dependsOnSource, AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& container);
        //! Finds all records where 'source' is dependent on another file
        bool GetDependsOnSourceBySource(const QString& source, AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& container);

        bool GetSourceFileDependenciesByBuilderGUIDAndSource(const AZ::Uuid& builderGuid, const char* source, AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& container);
        bool GetSourceFileDependency(const AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& inputEntry, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& databaseEntry);
        bool GetSourceFileDependencyBySourceDependencyId(AZ::s64 sourceDependencyId, AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& sourceDependencyEntry);
        bool RemoveSourceFileDependencies(AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& container);
        bool RemoveSourceFileDependency(const AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& entry);

        bool CreateOrUpdateLegacySubID(AzToolsFramework::AssetDatabase::LegacySubIDsEntry& entry);  // create or overwrite operation.
        bool RemoveLegacySubID(AZ::s64 legacySubIDsEntryID);
        bool RemoveLegacySubIDsByProductID(AZ::s64 productID);

        //ProductDependencies
        bool GetProductDependencies(AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& container);
        bool GetProductDependencyByProductDependencyID(AZ::s64 productDependencyID, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& productDependencyEntry);
        bool GetProductDependenciesByProductID(AZ::s64 productID, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& container);
        bool GetDirectProductDependencies(AZ::s64 productID, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container);
        bool GetAllProductDependencies(AZ::s64 productID, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container);
        bool SetProductDependency(AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry);
        bool SetProductDependencies(AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& container);
        bool RemoveProductDependencyByProductId(AZ::s64 productID);
 
    protected:
        void SetDatabaseVersion(AzToolsFramework::AssetDatabase::DatabaseVersion ver);
        void ExecuteCreateStatements();

    private:
        AZStd::vector<AZStd::string> m_createStatements; // contains all statements required to create the tables
    };
}//namespace EditorFramework

#endif // ASSETPROCESSOR_ASSETDATABASE_H