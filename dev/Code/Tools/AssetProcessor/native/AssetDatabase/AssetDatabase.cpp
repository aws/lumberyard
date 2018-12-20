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


#include "AssetDatabase.h"
#include <QMetaType>
#include <QString>
#include <QSet>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/IO/SystemFile.h>
#include <AzToolsFramework/SQLite/SQLiteConnection.h>
#include <AzToolsFramework/SQLite/SQLiteQuery.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/algorithm.h>
#include "native/assetprocessor.h"
#include <native/utilities/assetUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <QFileInfo>

namespace AssetProcessor
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::AssetSystem;
    using namespace AzToolsFramework::AssetDatabase;
    using namespace AzToolsFramework::SQLite;

    //tack on the namespace to avoid statement name collisions
    namespace
    {
        static const char* LOG_NAME = "AssetProcessor";

        //////////////////////////////////////////////////////////////////////////
        //tables
        static const char* CREATE_DATABASE_INFOTABLE = "AssetProcessor::CreateDatabaseInfoTable";
        static const char* CREATE_DATABASE_INFOTABLE_STATEMENT =
            "CREATE TABLE IF NOT EXISTS dbinfo( "
            "    rowID   INTEGER PRIMARY KEY, "
            "    version INTEGER NOT NULL);";

        static const char* CREATE_SCANFOLDERS_TABLE = "AssetProcessor::CreateScanFoldersTable";
        static const char* CREATE_SCANFOLDERS_TABLE_STATEMENT =
            "CREATE TABLE IF NOT EXISTS ScanFolders( "
            "   ScanFolderID    INTEGER PRIMARY KEY AUTOINCREMENT, "
            "   ScanFolder      TEXT NOT NULL collate nocase, "
            "   DisplayName     TEXT NOT NULL collate nocase, "
            "   PortableKey     TEXT NOT NULL collate nocase, "
            "   OutputPrefix    TEXT NOT NULL collate nocase, "
            "   IsRoot          INTEGER NOT NULL);";

        static const char* CREATE_SOURCES_TABLE = "AssetProcessor::CreateSourceTable";
        static const char* CREATE_SOURCES_TABLE_STATEMENT =
            "CREATE TABLE IF NOT EXISTS Sources("
            "    SourceID         INTEGER PRIMARY KEY AUTOINCREMENT, "
            "    ScanFolderPK     INTEGER NOT NULL, "
            "    SourceName       TEXT NOT NULL collate nocase, "
            "    SourceGuid       BLOB NOT NULL, "
            "    FOREIGN KEY (ScanFolderPK) REFERENCES "
            "       ScanFolders(ScanFolderID) ON DELETE CASCADE);";

        static const char* CREATE_JOBS_TABLE = "AssetProcessor::CreateJobsTable";
        static const char* CREATE_JOBS_TABLE_STATEMENT =
            "CREATE TABLE IF NOT EXISTS Jobs("
            "    JobID            INTEGER PRIMARY KEY AUTOINCREMENT, "
            "    SourcePK         INTEGER NOT NULL, "
            "    JobKey           TEXT NOT NULL collate nocase, "
            "    Fingerprint      INTEGER NOT NULL, "
            "    Platform         TEXT NOT NULL collate nocase, "
            "    BuilderGuid      BLOB NOT NULL, "
            "    Status           INTEGER NOT NULL, "
            "    JobRunKey        INTEGER NOT NULL, "
            "    FirstFailLogTime INTEGER NOT NULL, "
            "    FirstFailLogFile TEXT collate nocase, "
            "    LastFailLogTime  INTEGER NOT NULL, "
            "    LastFailLogFile  TEXT collate nocase, "
            "    LastLogTime      INTEGER NOT NULL, "
            "    LastLogFile      TEXT collate nocase, "
            "    FOREIGN KEY (SourcePK) REFERENCES "
            "       Sources(SourceID) ON DELETE CASCADE);";

        static const char* CREATEINDEX_JOBS_JOBRUNKEY = "AssetProcesser::CreateIndexJobsJobRunKey";
        static const char* CREATEINDEX_JOBS_JOBRUNKEY_STATEMENT =
            "CREATE INDEX IF NOT EXISTS Jobs_JobRunKey ON Jobs (JobRunKey);";

        static const char* CREATEINDEX_JOBS_JOBKEY = "AssetProcesser::CreateIndexJobsJobKey";
        static const char* CREATEINDEX_JOBS_JOBKEY_STATEMENT =
            "CREATE INDEX IF NOT EXISTS Jobs_JobKey ON Jobs (JobKey);";

        static const char* CREATE_PRODUCT_TABLE = "AssetProcessor::CreateProductTable";
        static const char* CREATE_PRODUCT_TABLE_STATEMENT =
            "CREATE TABLE IF NOT EXISTS Products( "
            "    ProductID      INTEGER PRIMARY KEY AUTOINCREMENT, "
            "    JobPK          INTEGER NOT NULL, "
            "    ProductName    TEXT NOT NULL collate nocase, "
            "    SubID          INTEGER NOT NULL, "
            "    AssetType      BLOB NOT NULL, "
            "    LegacyGuid     BLOB NOT NULL, "
            "    FOREIGN KEY (JobPK) REFERENCES "
            "       Jobs(JobID) ON DELETE CASCADE);";

        static const char* CREATE_LEGACYSUBIDS_TABLE = "AssetProcessor::CreateLegacySubIDsTable";
        static const char* CREATE_LEGACYSUBIDS_TABLE_STATEMENT =
            "CREATE TABLE IF NOT EXISTS LegacySubIDs( "
            "   LegacySubID             INTEGER PRIMARY KEY, "
            "   ProductPK               INTEGER NOT NULL, "
            "   SubID                   INTEGER NOT NULL, "
            "   FOREIGN KEY(ProductPK) REFERENCES "
            "      Products(ProductID) ON DELETE CASCADE);";

        static const char* CREATEINDEX_LEGACYSUBIDS_PRODUCTPK = "AssetProcesser::CreateIndexLegacySubIDs_ProductPK";
        static const char* CREATEINDEX_LEGACYSUBIDS_PRODUCTPK_STATEMENT =
            "CREATE INDEX IF NOT EXISTS LegacySubIDs_ProductPK ON LegacySubIDs (ProductPK);";

        static const char* CREATE_SOURCE_DEPENDENCY_TABLE = "AssetProcessor::CreateSourceDependencyTable";
        static const char* CREATE_SOURCE_DEPENDENCY_TABLE_STATEMENT =
            "CREATE TABLE IF NOT EXISTS SourceDependency("
            "    SourceDependencyID            INTEGER PRIMARY KEY AUTOINCREMENT, "
            "    BuilderGuid                   BLOB NOT NULL, "
            "    Source                        TEXT NOT NULL collate nocase, "
            "    DependsOnSource               TEXT NOT NULL collate nocase); ";

        static const char* CREATE_PRODUCT_DEPENDENCY_TABLE = "AssetProcessor::CreateProductDependencyTable";
        static const char* CREATE_PRODUCT_DEPENDENCY_TABLE_STATEMENT =
            "CREATE TABLE IF NOT EXISTS ProductDependencies("
            "    ProductDependencyID  INTEGER PRIMARY KEY AUTOINCREMENT, "
            "    ProductPK            INTEGER NOT NULL, "
            "    DependencySourceGuid BLOB NOT NULL, "
            "    DependencySubID      INTEGER NOT NULL, "
            "    DependencyFlags      INTEGER NOT NULL, "
            "    FOREIGN KEY (ProductPK) REFERENCES "
            "        Products(ProductID) ON DELETE CASCADE);";

        static const char* CREATE_FILES_TABLE = "AssetProcessor::CreateFilesTable";
        static const char* CREATE_FILES_TABLE_STATEMENT =
            "CREATE TABLE IF NOT EXISTS Files( "
            "    FileID         INTEGER PRIMARY KEY AUTOINCREMENT, "
            "    ScanFolderPK   INTEGER NOT NULL, "
            "    FileName       TEXT NOT NULL collate nocase, "
            "    IsFolder       INTEGER NOT NULL, "
            "    FOREIGN KEY (ScanFolderPK) REFERENCES "
            "       ScanFolders(ScanFolderID) ON DELETE CASCADE);";

        //////////////////////////////////////////////////////////////////////////
        //indices
        static const char* CREATEINDEX_DEPENDSONSOURCE_SOURCEDEPENDENCY = "AssetProcesser::CreateIndexDependsOnSource_SourceDependency";
        static const char* CREATEINDEX_DEPENDSONSOURCE_SOURCEDEPENDENCY_STATEMENT =
            "CREATE INDEX IF NOT EXISTS DependsOnSource_SourceDependency ON SourceDependency (DependsOnSource);";
        static const char* CREATEINDEX_BUILDERGUID_SOURCE_SOURCEDEPENDENCY = "AssetProcesser::CreateIndexBuilderGuid_Source_SourceDependency";
        static const char* CREATEINDEX_BUILDERGUID_SOURCE_SOURCEDEPENDENCY_STATEMENT =
            "CREATE INDEX IF NOT EXISTS BuilderGuid_Source_SourceDependency ON SourceDependency (BuilderGuid, Source);";
        static const char* CREATEINDEX_SCANFOLDERS_SOURCES = "AssetProcesser::CreateIndexScanFoldersSources";
        static const char* CREATEINDEX_SCANFOLDERS_SOURCES_STATEMENT =
            "CREATE INDEX IF NOT EXISTS ScanFolders_Sources ON Sources (ScanFolderPK);";
        static const char* DROPINDEX_SCANFOLDERS_SOURCES_STATEMENT =
            "DROP INDEX IF EXISTS ScanFolders_Sources_idx;";

        static const char* CREATEINDEX_SCANFOLDERS_SOURCES_SCANFOLDER = "AssetProcesser::CreateIndexScanFoldersSourcesScanFolder";
        static const char* CREATEINDEX_SCANFOLDERS_SOURCES_SCANFOLDER_STATEMENT =
            "CREATE INDEX IF NOT EXISTS IdxSources_SourceAndScanFolder ON Sources (ScanFolderPK, SourceName);";

        static const char* CREATEINDEX_SOURCES_JOBS = "AssetProcesser::CreateIndexSourcesJobs";
        static const char* CREATEINDEX_SOURCES_JOBS_STATEMENT =
            "CREATE INDEX IF NOT EXISTS Sources_Jobs ON Jobs (SourcePK);";
        static const char* DROPINDEX_SOURCES_JOBS_STATEMENT =
            "DROP INDEX IF EXISTS Sources_Jobs_idx;";

        static const char* CREATEINDEX_JOBS_PRODUCTS = "AssetProcesser::CreateIndexJobsProducts";
        static const char* CREATEINDEX_JOBS_PRODUCTS_STATEMENT =
            "CREATE INDEX IF NOT EXISTS Jobs_Products ON Products (JobPK);";
        static const char* DROPINDEX_JOBS_PRODUCTS_STATEMENT =
            "DROP INDEX IF EXISTS Jobs_Products_idx;";

        static const char* CREATEINDEX_SOURCE_NAME = "AssetProcessor::CreateIndexSourceName";
        static const char* CREATEINDEX_SOURCE_NAME_STATEMENT =
            "CREATE INDEX IF NOT EXISTS Sources_SourceName ON Sources (SourceName);";
        static const char* DROPINDEX_SOURCE_NAME_STATEMENT =
            "DROP INDEX IF EXISTS Sources_SourceName_idx;";

        static const char* CREATEINDEX_SOURCE_GUID = "AssetProcessor::CreateIndexSourceGuid";
        static const char* CREATEINDEX_SOURCE_GUID_STATEMENT =
            "CREATE INDEX IF NOT EXISTS Sources_SourceGuid ON Sources (SourceGuid);";

        static const char* CREATEINDEX_PRODUCT_NAME = "AssetProcessor::CreateIndexProductName";
        static const char* CREATEINDEX_PRODUCT_NAME_STATEMENT =
            "CREATE INDEX IF NOT EXISTS Products_ProductName ON Products (ProductName);";
        static const char* DROPINDEX_PRODUCT_NAME_STATEMENT =
            "DROP INDEX IF EXISTS Products_ProductName_idx;";

        static const char* CREATEINDEX_PRODUCT_SUBID = "AssetProcessor::CreateIndexProductSubID";
        static const char* CREATEINDEX_PRODUCT_SUBID_STATEMENT =
            "CREATE INDEX IF NOT EXISTS Products_SubID ON Products (SubID);";

        static const char* CREATEINDEX_PRODUCTDEPENDENCIES_PRODUCTPK = "AssetProcessor::CreateIndexProductDependenciesProductPK";
        static const char* CREATEINDEX_PRODUCTDEPENDENCIES_PRODUCTPK_STATEMENT =
            "CREATE INDEX IF NOT EXISTS ProductDependencies_ProductPK ON ProductDependencies (ProductPK);";

        static const char* CREATEINDEX_FILE_NAME = "AssetProcessor::CreateIndexFilesName";
        static const char* CREATEINDEX_FILE_NAME_STATEMENT =
            "CREATE INDEX IF NOT EXISTS Files_FileName ON Files (FileName);";

        static const char* CREATEINDEX_SCANFOLDERS_FILES = "AssetProcesser::CreateIndexScanFoldersFiles";
        static const char* CREATEINDEX_SCANFOLDERS_FILES_STATEMENT =
            "CREATE INDEX IF NOT EXISTS ScanFolders_Files ON Files (ScanFolderPK);";

        //////////////////////////////////////////////////////////////////////////
        //insert/set/update/delete
        static const char* SET_DATABASE_VERSION = "AssetProcessor::SetDatabaseVersion";
        static const char* SET_DATABASE_VERSION_STATEMENT =
            "INSERT OR REPLACE INTO dbinfo(rowID, version) "
            "VALUES (1, :ver);";

        static const auto s_SetDatabaseVersionQuery = MakeSqlQuery("dbinfo", SET_DATABASE_VERSION, SET_DATABASE_VERSION_STATEMENT, LOG_NAME,
            SqlParam<AZ::s32>(":ver"));

        static const char* INSERT_SCANFOLDER = "AssetProcessor::InsertScanFolder";
        static const char* INSERT_SCANFOLDER_STATEMENT =
            "INSERT INTO ScanFolders (ScanFolder, DisplayName, PortableKey, OutputPrefix, IsRoot) "
            "VALUES (:scanfolder, :displayname, :portablekey, :outputprefix, :isroot);";

        static const auto s_InsertScanfolderQuery = MakeSqlQuery("ScanFolders", INSERT_SCANFOLDER, INSERT_SCANFOLDER_STATEMENT, LOG_NAME,
            SqlParam<const char*>(":scanfolder"),
            SqlParam<const char*>(":displayname"),
            SqlParam<const char*>(":portablekey"),
            SqlParam<const char*>(":outputprefix"),
            SqlParam<AZ::s32>(":isroot"));

        static const char* UPDATE_SCANFOLDER = "AssetProcessor::UpdateScanFolder";
        static const char* UPDATE_SCANFOLDER_STATEMENT =
            "UPDATE ScanFolders SET "
            "ScanFolder =   :scanfolder, "
            "DisplayName =  :displayname, "
            "PortableKey =  :portablekey, "
            "OutputPrefix = :outputprefix, "
            "IsRoot = :isroot "
            "WHERE "
            "ScanFolderID = :scanfolderid;";

        static const auto s_UpdateScanfolderQuery = MakeSqlQuery("ScanFolders", UPDATE_SCANFOLDER, UPDATE_SCANFOLDER_STATEMENT, LOG_NAME,
            SqlParam<const char*>(":scanfolder"),
            SqlParam<const char*>(":displayname"),
            SqlParam<const char*>(":portablekey"),
            SqlParam<const char*>(":outputprefix"),
            SqlParam<AZ::s32>(":isroot"),
            SqlParam<AZ::s64>(":scanfolderid"));

        static const char* DELETE_SCANFOLDER = "AssetProcessor::RemoveScanFolder";
        static const char* DELETE_SCANFOLDER_STATEMENT =
            "DELETE FROM ScanFolders WHERE "
            "(ScanFolderID = :scanfolderid);";

        static const auto s_DeleteScanfolderQuery = MakeSqlQuery("ScanFolders", DELETE_SCANFOLDER, DELETE_SCANFOLDER_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":scanfolderid"));

        static const char* INSERT_SOURCE = "AssetProcessor::InsertSource";
        static const char* INSERT_SOURCE_STATEMENT =
            "INSERT INTO Sources (ScanFolderPK, SourceName, SourceGuid) "
            "VALUES (:scanfolderid, :sourcename, :sourceguid);";

        static const auto s_InsertSourceQuery = MakeSqlQuery("Sources", INSERT_SOURCE, INSERT_SOURCE_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":scanfolderid"),
            SqlParam<const char*>(":sourcename"),
            SqlParam<AZ::Uuid>(":sourceguid"));

        static const char* UPDATE_SOURCE = "AssetProcessor::UpdateSource";
        static const char* UPDATE_SOURCE_STATEMENT =
            "UPDATE Sources SET "
            "ScanFolderPK = :scanfolderpk, "
            "SourceName = :sourcename, "
            "SourceGuid = :sourceguid WHERE "
            "SourceID = :sourceid;";

        static const auto s_UpdateSourceQuery = MakeSqlQuery("Sources", UPDATE_SOURCE, UPDATE_SOURCE_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":scanfolderpk"),
            SqlParam<const char*>(":sourcename"),
            SqlParam<AZ::Uuid>(":sourceguid"),
            SqlParam<AZ::s64>(":sourceid"));

        static const char* DELETE_SOURCE = "AssetProcessor::DeleteSource";
        static const char* DELETE_SOURCE_STATEMENT =
            "DELETE FROM Sources WHERE "
            "SourceID = :sourceid;";

        static const auto s_DeleteSourceQuery = MakeSqlQuery("Sources", DELETE_SOURCE, DELETE_SOURCE_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":sourceid"));

        static const char* DELETE_SOURCE_BY_SCANFOLDERID = "AssetProcessor::DeleteSourceByScanFolderID";
        static const char* DELETE_SOURCE_BY_SCANFOLDERID_STATEMENT =
            "DELETE FROM Sources WHERE "
            "ScanFolderPK = :scanfolderid;";

        static const auto s_DeleteSourceByScanfolderidQuery = MakeSqlQuery("Sources", DELETE_SOURCE_BY_SCANFOLDERID, DELETE_SOURCE_BY_SCANFOLDERID_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":scanfolderid"));

        static const char* GET_HIGHEST_JOBRUNKEY = "AssetProcessor::GetHighestJobRunKey";
        static const char* GET_HIGHEST_JOBRUNKEY_STATEMENT =
            "SELECT JobRunKey FROM Jobs ORDER BY JobRunKey DESC LIMIT 1;";

        static const auto s_GetHighestJobrunkeyQuery = MakeSqlQuery("Jobs", GET_HIGHEST_JOBRUNKEY, GET_HIGHEST_JOBRUNKEY_STATEMENT, LOG_NAME);

        static const char* INSERT_JOB = "AssetProcessor::InsertJob";
        static const char* INSERT_JOB_STATEMENT =
            "INSERT INTO Jobs (SourcePK, JobKey, Fingerprint, Platform, BuilderGuid, Status, JobRunKey, FirstFailLogTime, FirstFailLogFile, LastFailLogTime, LastFailLogFile, LastLogTime, LastLogFile) "
            "VALUES (:sourceid, :jobkey, :fingerprint, :platform, :builderguid, :status, :jobrunkey, :firstfaillogtime, :firstfaillogfile, :lastfaillogtime, :lastfaillogfile, :lastlogtime, :lastlogfile);";

        static const auto s_InsertJobQuery = MakeSqlQuery("Jobs", INSERT_JOB, INSERT_JOB_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":sourceid"),
            SqlParam<const char*>(":jobkey"),
            SqlParam<AZ::u32>(":fingerprint"),
            SqlParam<const char*>(":platform"),
            SqlParam<AZ::Uuid>(":builderguid"),
            SqlParam<AZ::s32>(":status"),
            SqlParam<AZ::u64>(":jobrunkey"),
            SqlParam<AZ::s64>(":firstfaillogtime"),
            SqlParam<const char*>(":firstfaillogfile"),
            SqlParam<AZ::s64>(":lastfaillogtime"),
            SqlParam<const char*>(":lastfaillogfile"),
            SqlParam<AZ::s64>(":lastlogtime"),
            SqlParam<const char*>(":lastlogfile"));

        static const char* UPDATE_JOB = "AssetProcessor::UpdateJob";
        static const char* UPDATE_JOB_STATEMENT =
            "UPDATE Jobs SET "
            "SourcePK = :sourceid, "
            "JobKey = :jobkey, "
            "Fingerprint = :fingerprint, "
            "Platform = :platform, "
            "BuilderGuid = :builderguid, "
            "Status = :status, "
            "JobRunKey = :jobrunkey, "
            "FirstFailLogTime = :firstfaillogtime, "
            "FirstFailLogFile = :firstfaillogfile, "
            "LastFailLogTime = :lastfaillogtime, "
            "LastFailLogFile = :lastfaillogfile, "
            "LastLogTime = :lastlogtime, "
            "LastLogFile = :lastlogfile WHERE "
            "JobID = :jobid;";

        static const auto s_UpdateJobQuery = MakeSqlQuery("Jobs", UPDATE_JOB, UPDATE_JOB_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":sourceid"),
            SqlParam<const char*>(":jobkey"),
            SqlParam<AZ::u32>(":fingerprint"),
            SqlParam<const char*>(":platform"),
            SqlParam<AZ::Uuid>(":builderguid"),
            SqlParam<AZ::s32>(":status"),
            SqlParam<AZ::u64>(":jobrunkey"),
            SqlParam<AZ::s64>(":firstfaillogtime"),
            SqlParam<const char*>(":firstfaillogfile"),
            SqlParam<AZ::s64>(":lastfaillogtime"),
            SqlParam<const char*>(":lastfaillogfile"),
            SqlParam<AZ::s64>(":lastlogtime"),
            SqlParam<const char*>(":lastlogfile"),
            SqlParam<AZ::s64>(":jobid"));

        static const char* DELETE_JOB = "AssetProcessor::DeleteJob";
        static const char* DELETE_JOB_STATEMENT =
            "DELETE FROM Jobs WHERE "
            "JobID = :jobid;";

        static const auto s_DeleteJobQuery = MakeSqlQuery("Jobs", DELETE_JOB, DELETE_JOB_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":jobid"));

        static const char* INSERT_PRODUCT = "AssetProcessor::InsertProduct";
        static const char* INSERT_PRODUCT_STATEMENT =
            "INSERT INTO Products (JobPK, SubID, ProductName, AssetType, LegacyGuid) "
            "VALUES (:jobid, :subid, :productname, :assettype, :legacyguid);";

        static const auto s_InsertProductQuery = MakeSqlQuery("Products", INSERT_PRODUCT, INSERT_PRODUCT_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":jobid"),
            SqlParam<AZ::u32>(":subid"),
            SqlParam<const char*>(":productname"),
            SqlParam<AZ::Uuid>(":assettype"),
            SqlParam<AZ::Uuid>(":legacyguid"));

        static const char* UPDATE_PRODUCT = "AssetProcessor::UpdateProduct";
        static const char* UPDATE_PRODUCT_STATEMENT =
            "UPDATE Products SET "
            "JobPK = :jobid, "
            "SubID = :subid, "
            "ProductName = :productname, "
            "AssetType = :assettype, "
            "LegacyGuid = :legacyguid WHERE "
            "ProductID = :productid;";

        static const auto s_UpdateProductQuery = MakeSqlQuery("Products", UPDATE_PRODUCT, UPDATE_PRODUCT_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":jobid"),
            SqlParam<AZ::u32>(":subid"),
            SqlParam<const char*>(":productname"),
            SqlParam<AZ::Uuid>(":assetttype"),
            SqlParam<AZ::Uuid>(":legacyguid"),
            SqlParam<AZ::s64>(":productid"));

        static const char* DELETE_PRODUCT = "AssetProcessor::DeleteProduct";
        static const char* DELETE_PRODUCT_STATEMENT =
            "DELETE FROM Products WHERE "
            "ProductID = :productid;";

        static const auto s_DeleteProductQuery = MakeSqlQuery("Products", DELETE_PRODUCT, DELETE_PRODUCT_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":productid"));

        static const char* DELETE_PRODUCTS_BY_JOBID = "AssetProcessor::DeleteAllProductsByJobID";
        static const char* DELETE_PRODUCTS_BY_JOBID_STATEMENT =
            "DELETE FROM Products WHERE "
            "JobPK = :jobid;";

        static const auto s_DeleteProductsByJobidQuery = MakeSqlQuery("Products", DELETE_PRODUCTS_BY_JOBID, DELETE_PRODUCTS_BY_JOBID_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":jobid"));

        static const char* DELETE_PRODUCTS_BY_SOURCEID = "AssetProcessor::DeleteAllProductsBySourceID";
        static const char* DELETE_PRODUCTS_BY_SOURCEID_STATEMENT =
            "DELETE FROM Products "
            "WHERE EXISTS "
            "(SELECT * FROM Jobs WHERE "
            "Products.JobPK = Jobs.JobID AND "
            "Jobs.SourcePK = :sourceid);";

        static const auto s_DeleteProductsBySourceidQuery = MakeSqlQuery("Products", DELETE_PRODUCTS_BY_SOURCEID, DELETE_PRODUCTS_BY_SOURCEID_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":sourceid"));

        static const char* DELETE_PRODUCTS_BY_SOURCEID_PLATFORM = "AssetProcessor::DeleteProductsBySourceIDPlatform";
        static const char* DELETE_PRODUCTS_BY_SOURCEID_PLATFORM_STATEMENT =
            "DELETE FROM Products "
            "WHERE EXISTS "
            "(SELECT * FROM Jobs WHERE "
            "Products.JobPK = Jobs.JobID AND "
            "Jobs.SourcePK = :sourceid AND "
            "Jobs.Platform = :platform);";

        static const auto s_DeleteProductsBySourceidPlatformQuery = MakeSqlQuery("Products", DELETE_PRODUCTS_BY_SOURCEID_PLATFORM, DELETE_PRODUCTS_BY_SOURCEID_PLATFORM_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":sourceid"),
            SqlParam<const char*>(":platform"));

        static const char* INSERT_SOURCE_DEPENDENCY = "AssetProcessor::InsertSourceDependency";
        static const char* INSERT_SOURCE_DEPENDENCY_STATEMENT =
            "INSERT INTO SourceDependency (BuilderGuid, Source, DependsOnSource) "
            "VALUES (:builderGuid, :source, :dependsOnSource);";

        static const auto s_InsertSourceDependencyQuery = MakeSqlQuery("SourceDependency", INSERT_SOURCE_DEPENDENCY, INSERT_SOURCE_DEPENDENCY_STATEMENT, LOG_NAME,
            SqlParam<AZ::Uuid>(":builderGuid"),
            SqlParam<const char*>(":source"),
            SqlParam<const char*>(":dependsOnSource"));

        static const char* UPDATE_SOURCE_DEPENDENCY = "AssetProcessor::UpdateSourceDependency";
        static const char* UPDATE_SOURCE_DEPENDENCY_STATEMENT =
            "UPDATE SourceDependency SET "
            "DependsOnSource = :dependsOnSource, WHERE "
            "BuilderGuid = :builderGuid AND "
            "Source = :source AND "
            "SourceDependencyID = :sourceDependencyId;";

        static const auto s_UpdateSourceDependencyQuery = MakeSqlQuery("SourceDependency", UPDATE_SOURCE_DEPENDENCY, UPDATE_SOURCE_DEPENDENCY_STATEMENT, LOG_NAME,
            SqlParam<const char*>(":dependsOnSource"),
            SqlParam<AZ::Uuid>(":builderGuid"),
            SqlParam<const char*>(":source"),
            SqlParam<AZ::s64>(":sourceDependencyId"));

        static const char* DELETE_SOURCE_DEPENDENCY_SOURCEDEPENDENCYID = "AssetProcessor::DeleteSourceDependencBySourceDependencyId";
        static const char* DELETE_SOURCE_DEPENDENCY_SOURCEDEPENDENCYID_STATEMENT =
            "DELETE FROM SourceDependency WHERE "
            "SourceDependencyID = :sourceDependencyId;";

        static const auto s_DeleteSourceDependencySourcedependencyidQuery = MakeSqlQuery("SourceDependency", DELETE_SOURCE_DEPENDENCY_SOURCEDEPENDENCYID, DELETE_SOURCE_DEPENDENCY_SOURCEDEPENDENCYID_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":sourceDependencyId"));

        static const char* INSERT_NEW_LEGACYSUBID = "AssetProcessor::InsertLegacySubID";
        static const char* INSERT_NEW_LEGACYSUBID_STATEMENT =
            "INSERT INTO LegacySubIDs (ProductPK, SubID) "
            "VALUES (:productPK, :subID);";

        static const auto s_InsertNewLegacysubidQuery = MakeSqlQuery("LegacySubIDs", INSERT_NEW_LEGACYSUBID, INSERT_NEW_LEGACYSUBID_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":productPK"),
            SqlParam<AZ::u32>(":subID"));

        static const char* OVERWRITE_EXISTING_LEGACYSUBID = "AssetProcessor::OverwriteLegacySubID";
        static const char* OVERWRITE_EXISTING_LEGACYSUBID_STATEMENT =
            "UPDATE LegacySubIDs "
            "  SET "
            "     ProductPK = :productPK, "
            "     SubID = :subID "
            "  WHERE "
            "     LegacySubID = :legacySubID;";

        static const auto s_OverwriteExistingLegacysubidQuery = MakeSqlQuery("LegacySubIDs", OVERWRITE_EXISTING_LEGACYSUBID, OVERWRITE_EXISTING_LEGACYSUBID_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":productPK"),
            SqlParam<AZ::u32>(":subID"),
            SqlParam<AZ::s64>(":legacySubID"));

        static const char* DELETE_LEGACYSUBIDS_BY_PRIMARY_KEY = "AssetProcessor::DeleteLegacySubIDsByPrimaryKey";
        static const char* DELETE_LEGACYSUBIDS_BY_PRIMARY_KEY_STATEMENT =
            "DELETE FROM LegacySubIDs WHERE "
            "LegacySubID = :legacySubID;";

        static const auto s_DeleteLegacysubidsByPrimaryKeyQuery = MakeSqlQuery("LegacySubIDs", DELETE_LEGACYSUBIDS_BY_PRIMARY_KEY, DELETE_LEGACYSUBIDS_BY_PRIMARY_KEY_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":legacySubID"));

        static const char* DELETE_LEGACYSUBIDS_BY_PRODUCTID = "AssetProcessor::DeleteLegacySubIDsByProductID";
        static const char* DELETE_LEGACYSUBIDS_BY_PRODUCTID_STATEMENT =
            "DELETE FROM LegacySubIDs WHERE "
            "ProductPK = :productPK;";

        static const auto s_DeleteLegacysubidsByProductidQuery = MakeSqlQuery("LegacySubIDs", DELETE_LEGACYSUBIDS_BY_PRODUCTID, DELETE_LEGACYSUBIDS_BY_PRODUCTID_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":productPK"));

        static const char* INSERT_PRODUCT_DEPENDENCY = "AssetProcessor::InsertProductDependency";
        static const char* INSERT_PRODUCT_DEPENDENCY_STATEMENT =
            "INSERT INTO ProductDependencies (ProductPK, DependencySourceGuid, DependencySubID, DependencyFlags) "
            "VALUES (:productPK, :dependencySourceGuid, :dependencySubID, :dependencyFlags);";

        static const auto s_InsertProductDependencyQuery = MakeSqlQuery("ProductDependencies", INSERT_PRODUCT_DEPENDENCY, INSERT_PRODUCT_DEPENDENCY_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":productPK"),
            SqlParam<AZ::Uuid>(":dependencySourceGuid"),
            SqlParam<AZ::u32>(":dependencySubID"),
            SqlParam<AZ::s64>(":dependencyFlags"));

        static const char* UPDATE_PRODUCT_DEPENDENCY = "AssetProcessor::UpdateProductDependency";
        static const char* UPDATE_PRODUCT_DEPENDENCY_STATEMENT =
            "UPDATE ProductDependencies SET "
            "ProductPK = :productPK, "
            "DependencySourceGuid = :dependencySourceGuid, "
            "DependencySubID = :dependencySubID, "
            "DependencyFlags = :dependencyFlags, WHERE "
            "ProductDependencyID = :productDependencyID;";

        static const auto s_UpdateProductDependencyQuery = MakeSqlQuery("ProductDependencies", UPDATE_PRODUCT_DEPENDENCY, UPDATE_PRODUCT_DEPENDENCY_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":productPK"),
            SqlParam<AZ::Uuid>(":dependencySourceGuid"),
            SqlParam<AZ::u32>(":dependencySubID"),
            SqlParam<AZ::u64>(":dependencyFlags"),
            SqlParam<AZ::s64>(":productDependencyID"));

        static const char* DELETE_PRODUCT_DEPENDENCY_BY_PRODUCTID = "AssetProcessor::DeleteProductDependencyByProductId";
        static const char* DELETE_PRODUCT_DEPENDENCY_BY_PRODUCTID_STATEMENT =
            "DELETE FROM ProductDependencies WHERE "
            "ProductPK = :productpk;";

        static const auto s_DeleteProductDependencyByProductIdQuery = MakeSqlQuery("ProductDependencies", DELETE_PRODUCT_DEPENDENCY_BY_PRODUCTID, DELETE_PRODUCT_DEPENDENCY_BY_PRODUCTID_STATEMENT, LOG_NAME,
            SqlParam<AZ::s64>(":productpk"));

        static const char* DELETE_AUTO_SUCCEED_JOBS = "AssetProcessor::DeleteAutoSucceedJobs";
        static const char* DELETE_AUTO_SUCCEED_JOBS_STATEMENT =
            "DELETE FROM Jobs WHERE JobKey LIKE 'CreateJobs_success_'";

        static const char* INSERT_FILE = "AssetProcessor::InsertFile";
        static const char* INSERT_FILE_STATEMENT =
            "INSERT INTO Files (ScanFolderPK, FileName, IsFolder) "
            "VALUES (:scanfolderpk, :filename, :isfolder);";

        static const char* UPDATE_FILE = "AssetProcessor::UpdateFile";
        static const char* UPDATE_FILE_STATEMENT =
            "UPDATE Files SET "
            "ScanFolderPK = :scanfolderpk, "
            "FileName = :filename, "
            "IsFolder = :isfolder WHERE "
            "FileID = :fileid;";

        static const char* DELETE_FILE = "AssetProcessor::DeleteFile";
        static const char* DELETE_FILE_STATEMENT =
            "DELETE FROM Files WHERE "
            "FileID = :fileid;";
    }

    AssetDatabaseConnection::AssetDatabaseConnection()
    {
        qRegisterMetaType<ScanFolderDatabaseEntry>("ScanFolderEntry");
        qRegisterMetaType<SourceDatabaseEntry>("SourceEntry");
        qRegisterMetaType<JobDatabaseEntry>("JobDatabaseEntry");
        qRegisterMetaType<ProductDatabaseEntry>("ProductEntry");
        qRegisterMetaType<CombinedDatabaseEntry>("CombinedEntry");
        qRegisterMetaType<SourceDatabaseEntryContainer>("SourceEntryContainer");
        qRegisterMetaType<JobDatabaseEntryContainer>("JobDatabaseEntryContainer");
        qRegisterMetaType<ProductDatabaseEntryContainer>("ProductEntryContainer");
        qRegisterMetaType<CombinedDatabaseEntryContainer>("CombinedEntryContainer");
    }

    AssetDatabaseConnection::~AssetDatabaseConnection()
    {
        CloseDatabase();
    }

    bool AssetDatabaseConnection::DataExists()
    {
        AZStd::string dbFilePath = GetAssetDatabaseFilePath();
        return AZ::IO::SystemFile::Exists(dbFilePath.c_str());
    }

    void AssetDatabaseConnection::LoadData()
    {
        if ((!m_databaseConnection) || (!m_databaseConnection->IsOpen()))
        {
            OpenDatabase();
        }
    }

    void AssetDatabaseConnection::ClearData()
    {
        if ((m_databaseConnection) && (m_databaseConnection->IsOpen()))
        {
            CloseDatabase();
        }
        AZStd::string dbFilePath = GetAssetDatabaseFilePath();
        AZ::IO::SystemFile::Delete(dbFilePath.c_str());
        OpenDatabase();
    }


    bool AssetDatabaseConnection::PostOpenDatabase()
    {
        DatabaseVersion foundVersion = DatabaseVersion::DatabaseDoesNotExist;

        if (m_databaseConnection->DoesTableExist("dbinfo"))
        {
            foundVersion = QueryDatabaseVersion();
        }
        bool dropAllTables = true;

        // if its a future version, we don't want to drop tables and blow up, we'd rather just inform the user, and move on:
        if (foundVersion > CurrentDatabaseVersion())
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false,
                "The database in the Cache folder appears to be from a NEWER version of Asset Processor than this one.\n"
                "To prevent loss of data in the cache for the newer version, this Asset Processor will close.\n");
            return false;
        }

        if (foundVersion == DatabaseVersion::AddedOutputPrefixToScanFolders)
        {
            // execute statements to upgrade the database
            if (m_databaseConnection->ExecuteOneOffStatement(CREATEINDEX_JOBS_JOBKEY))
            {
                foundVersion = DatabaseVersion::AddedJobKeyIndex;
            }
        }

        // over here, check the version number, and perform upgrading if you need to
        if (foundVersion == DatabaseVersion::AddedJobKeyIndex)
        {
            if (
                (m_databaseConnection->ExecuteOneOffStatement(CREATEINDEX_SOURCE_GUID)) &&
                (m_databaseConnection->ExecuteOneOffStatement(CREATEINDEX_SCANFOLDERS_SOURCES_SCANFOLDER))
                )
            {
                foundVersion = DatabaseVersion::AddedSourceGuidIndex;
            }
        }

        if (foundVersion == DatabaseVersion::AddedSourceGuidIndex)
        {
            if (
                (m_databaseConnection->ExecuteOneOffStatement(CREATE_SOURCE_DEPENDENCY_TABLE)) &&
                (m_databaseConnection->ExecuteOneOffStatement(CREATEINDEX_DEPENDSONSOURCE_SOURCEDEPENDENCY)) &&
                (m_databaseConnection->ExecuteOneOffStatement(CREATEINDEX_BUILDERGUID_SOURCE_SOURCEDEPENDENCY))
                )
            {
                foundVersion = DatabaseVersion::AddedSourceDependencyTable;
            }
        }

        if (foundVersion == DatabaseVersion::AddedSourceDependencyTable)
        {
            // add the missing tables - nothing will have generated data for this before this point, so its okay to just make empty ones.
            if (
                (m_databaseConnection->ExecuteOneOffStatement(CREATE_LEGACYSUBIDS_TABLE)) &&
                (m_databaseConnection->ExecuteOneOffStatement(CREATEINDEX_LEGACYSUBIDS_PRODUCTPK))
                )
            {
                foundVersion = DatabaseVersion::AddedLegacySubIDsTable;
            }
        }

        if (foundVersion == DatabaseVersion::AddedLegacySubIDsTable)
        {
            if (m_databaseConnection->ExecuteOneOffStatement(CREATE_PRODUCT_DEPENDENCY_TABLE))
            {
                foundVersion = DatabaseVersion::AddedProductDependencyTable;
            }
        }

        if (foundVersion == DatabaseVersion::AddedProductDependencyTable)
        {
            if (m_databaseConnection->ExecuteOneOffStatement(DELETE_AUTO_SUCCEED_JOBS))
            {
                foundVersion = DatabaseVersion::ClearAutoSucceedJobs;
            }
        }

        if (foundVersion == DatabaseVersion::ClearAutoSucceedJobs)
        {
            if (m_databaseConnection->ExecuteOneOffStatement(CREATE_FILES_TABLE))
            {
                foundVersion = DatabaseVersion::AddedFilesTable;
            }
        }

        if (foundVersion == CurrentDatabaseVersion())
        {
            dropAllTables = false;
        }
        else
        {
            dropAllTables = true;
        }

        // example, if you know how to get from version 1 to version 2, and we're on version 1 and should be on version 2,
        // we can either drop all tables and recreate them, or we can write statements which upgrade the database.
        // if you know how to upgrade, write your modify statements here, then set dropAllTables to false.
        // otherwise it will re-create from scratch.

        if (dropAllTables)
        {
            // drop all tables by destroying the entire database.
            m_databaseConnection->Close();
            AZStd::string dbFilePath = GetAssetDatabaseFilePath();
            AZ::IO::SystemFile::Delete(dbFilePath.c_str());
            if (!m_databaseConnection->Open(dbFilePath, IsReadOnly()))
            {
                delete m_databaseConnection;
                m_databaseConnection = nullptr;
                AZ_Error(LOG_NAME, false, "Unable to open the asset database at %s\n", dbFilePath.c_str());
                return false;
            }

            CreateStatements();
            ExecuteCreateStatements();
        }

        // now that the database matches the schema, update it:
        SetDatabaseVersion(CurrentDatabaseVersion());

        return AzToolsFramework::AssetDatabase::AssetDatabaseConnection::PostOpenDatabase();
    }


    void AssetDatabaseConnection::ExecuteCreateStatements()
    {
        AZ_Assert(m_databaseConnection, "No connection!");
        for (const auto& element : m_createStatements)
        {
            m_databaseConnection->ExecuteOneOffStatement(element.c_str());
        }
    }

    void AssetDatabaseConnection::SetDatabaseVersion(DatabaseVersion ver)
    {
        AZ_Error(LOG_NAME, m_databaseConnection, "Fatal: attempt to work on a database connection that doesn't exist");
        AZ_Error(LOG_NAME, m_databaseConnection->IsOpen(), "Fatal: attempt to work on a database connection that isn't open");
        AZ_Error(LOG_NAME, m_databaseConnection->DoesTableExist("dbinfo"), "Fatal: dbinfo table does not exist");

        StatementAutoFinalizer autoFinal(*m_databaseConnection, SET_DATABASE_VERSION);
        Statement* statement = autoFinal.Get();
        AZ_Error(LOG_NAME, statement, "Statement not found: %s", SET_DATABASE_VERSION);

        statement->BindValueInt(statement->GetNamedParamIdx(":ver"), static_cast<int>(ver));
        Statement::SqlStatus result = statement->Step();
        AZ_Warning(LOG_NAME, result != SQLite::Statement::SqlOK, "Failed to execute SetDatabaseVersion.");
    }

    void AssetDatabaseConnection::CreateStatements()
    {
        AZ_Assert(m_databaseConnection, "No connection!");
        AZ_Assert(m_databaseConnection->IsOpen(), "Connection is not open");

        AzToolsFramework::AssetDatabase::AssetDatabaseConnection::CreateStatements();

        // ---------------------------------------------------------------------------------------------
        //                  Housekeeping
        // ---------------------------------------------------------------------------------------------
        m_databaseConnection->AddStatement("VACUUM", "VACUUM");
        m_databaseConnection->AddStatement("ANALYZE", "ANALYZE");

        // ---------------------------------------------------------------------------------------------
        //                  Database Info table
        // ----------------------------------------------------------------------------------------------
        m_databaseConnection->AddStatement(CREATE_DATABASE_INFOTABLE, CREATE_DATABASE_INFOTABLE_STATEMENT);
        m_createStatements.push_back(CREATE_DATABASE_INFOTABLE);

        AddStatement(m_databaseConnection, s_SetDatabaseVersionQuery);

        // ----------------------------------------------------------------------------------------------
        //                  ScanFolders table
        // ----------------------------------------------------------------------------------------------
        m_databaseConnection->AddStatement(CREATE_SCANFOLDERS_TABLE, CREATE_SCANFOLDERS_TABLE_STATEMENT);
        m_createStatements.push_back(CREATE_SCANFOLDERS_TABLE);

        AddStatement(m_databaseConnection, s_InsertScanfolderQuery);
        AddStatement(m_databaseConnection, s_UpdateScanfolderQuery);
        AddStatement(m_databaseConnection, s_DeleteScanfolderQuery);
        AddStatement(m_databaseConnection, s_DeleteSourceByScanfolderidQuery);

        // ---------------------------------------------------------------------------------------------
        //                  Source table
        // ---------------------------------------------------------------------------------------------
        m_databaseConnection->AddStatement(CREATE_SOURCES_TABLE, CREATE_SOURCES_TABLE_STATEMENT);
        m_createStatements.push_back(CREATE_SOURCES_TABLE);

        AddStatement(m_databaseConnection, s_InsertSourceQuery);
        AddStatement(m_databaseConnection, s_UpdateSourceQuery);
        AddStatement(m_databaseConnection, s_DeleteSourceQuery);

        // ---------------------------------------------------------------------------------------------
        //                  Jobs table
        // ---------------------------------------------------------------------------------------------
        m_databaseConnection->AddStatement(CREATE_JOBS_TABLE, CREATE_JOBS_TABLE_STATEMENT);
        m_createStatements.push_back(CREATE_JOBS_TABLE);

        AddStatement(m_databaseConnection, s_GetHighestJobrunkeyQuery);
        AddStatement(m_databaseConnection, s_InsertJobQuery);
        AddStatement(m_databaseConnection, s_UpdateJobQuery);
        AddStatement(m_databaseConnection, s_DeleteJobQuery);
        // ---------------------------------------------------------------------------------------------
        //                   Products table
        // ---------------------------------------------------------------------------------------------
        m_databaseConnection->AddStatement(CREATE_PRODUCT_TABLE, CREATE_PRODUCT_TABLE_STATEMENT);
        m_createStatements.push_back(CREATE_PRODUCT_TABLE);

        AddStatement(m_databaseConnection, s_InsertProductQuery);
        AddStatement(m_databaseConnection, s_UpdateProductQuery);
        AddStatement(m_databaseConnection, s_DeleteProductQuery);
        AddStatement(m_databaseConnection, s_DeleteProductsByJobidQuery);
        AddStatement(m_databaseConnection, s_DeleteProductsBySourceidQuery);
        AddStatement(m_databaseConnection, s_DeleteProductsBySourceidPlatformQuery);

        // ---------------------------------------------------------------------------------------------
        //                   Source Dependency table
        // ---------------------------------------------------------------------------------------------
        m_databaseConnection->AddStatement(CREATE_SOURCE_DEPENDENCY_TABLE, CREATE_SOURCE_DEPENDENCY_TABLE_STATEMENT);
        m_createStatements.push_back(CREATE_SOURCE_DEPENDENCY_TABLE);

        AddStatement(m_databaseConnection, s_InsertSourceDependencyQuery);
        AddStatement(m_databaseConnection, s_UpdateSourceDependencyQuery);
        AddStatement(m_databaseConnection, s_DeleteSourceDependencySourcedependencyidQuery);

        // ---------------------------------------------------------------------------------------------
        //                    Legacy SubIDs table
        // ---------------------------------------------------------------------------------------------
        m_databaseConnection->AddStatement(CREATE_LEGACYSUBIDS_TABLE, CREATE_LEGACYSUBIDS_TABLE_STATEMENT);
        m_databaseConnection->AddStatement(CREATEINDEX_LEGACYSUBIDS_PRODUCTPK, CREATEINDEX_LEGACYSUBIDS_PRODUCTPK_STATEMENT);
        m_createStatements.push_back(CREATE_LEGACYSUBIDS_TABLE);
        m_createStatements.push_back(CREATEINDEX_LEGACYSUBIDS_PRODUCTPK);

        AddStatement(m_databaseConnection, s_InsertNewLegacysubidQuery);
        AddStatement(m_databaseConnection, s_OverwriteExistingLegacysubidQuery);
        AddStatement(m_databaseConnection, s_DeleteLegacysubidsByPrimaryKeyQuery);
        AddStatement(m_databaseConnection, s_DeleteLegacysubidsByProductidQuery);

        // ---------------------------------------------------------------------------------------------
        //                   Product Dependency table
        // ---------------------------------------------------------------------------------------------
        m_databaseConnection->AddStatement(CREATE_PRODUCT_DEPENDENCY_TABLE, CREATE_PRODUCT_DEPENDENCY_TABLE_STATEMENT);
        m_createStatements.push_back(CREATE_PRODUCT_DEPENDENCY_TABLE);

        AddStatement(m_databaseConnection, s_InsertProductDependencyQuery);
        AddStatement(m_databaseConnection, s_UpdateProductDependencyQuery);
        AddStatement(m_databaseConnection, s_DeleteProductDependencyByProductIdQuery);

        // ---------------------------------------------------------------------------------------------
        //                  Files table
        // ---------------------------------------------------------------------------------------------
        m_databaseConnection->AddStatement(CREATE_FILES_TABLE, CREATE_FILES_TABLE_STATEMENT);
        m_createStatements.push_back(CREATE_FILES_TABLE);

        m_databaseConnection->AddStatement(INSERT_FILE, INSERT_FILE_STATEMENT);
        m_databaseConnection->AddStatement(UPDATE_FILE, UPDATE_FILE_STATEMENT);
        m_databaseConnection->AddStatement(DELETE_FILE, DELETE_FILE_STATEMENT);

        // ---------------------------------------------------------------------------------------------
        //                   Indices
        // ---------------------------------------------------------------------------------------------
        m_databaseConnection->AddStatement(CREATEINDEX_DEPENDSONSOURCE_SOURCEDEPENDENCY, CREATEINDEX_DEPENDSONSOURCE_SOURCEDEPENDENCY_STATEMENT);
        m_createStatements.push_back(CREATEINDEX_DEPENDSONSOURCE_SOURCEDEPENDENCY);

        m_databaseConnection->AddStatement(CREATEINDEX_BUILDERGUID_SOURCE_SOURCEDEPENDENCY, CREATEINDEX_BUILDERGUID_SOURCE_SOURCEDEPENDENCY_STATEMENT);
        m_createStatements.push_back(CREATEINDEX_BUILDERGUID_SOURCE_SOURCEDEPENDENCY);

        m_databaseConnection->AddStatement(CREATEINDEX_SCANFOLDERS_SOURCES_SCANFOLDER, CREATEINDEX_SCANFOLDERS_SOURCES_SCANFOLDER_STATEMENT);
        m_createStatements.push_back(CREATEINDEX_SCANFOLDERS_SOURCES_SCANFOLDER);

        m_databaseConnection->AddStatement(CREATEINDEX_SOURCES_JOBS, CREATEINDEX_SOURCES_JOBS_STATEMENT);
        m_createStatements.push_back(CREATEINDEX_SOURCES_JOBS);

        m_databaseConnection->AddStatement(CREATEINDEX_JOBS_PRODUCTS, CREATEINDEX_JOBS_PRODUCTS_STATEMENT);

        m_createStatements.push_back(CREATEINDEX_JOBS_PRODUCTS);

        m_databaseConnection->AddStatement(CREATEINDEX_JOBS_JOBRUNKEY, CREATEINDEX_JOBS_JOBRUNKEY_STATEMENT);
        m_createStatements.push_back(CREATEINDEX_JOBS_JOBRUNKEY);

        m_databaseConnection->AddStatement(CREATEINDEX_JOBS_JOBKEY, CREATEINDEX_JOBS_JOBKEY_STATEMENT);
        m_createStatements.push_back(CREATEINDEX_JOBS_JOBKEY);

        m_databaseConnection->AddStatement(CREATEINDEX_SOURCE_NAME, CREATEINDEX_SOURCE_NAME_STATEMENT);
        m_createStatements.push_back(CREATEINDEX_SOURCE_NAME);

        m_databaseConnection->AddStatement(CREATEINDEX_SOURCE_GUID, CREATEINDEX_SOURCE_GUID_STATEMENT);
        m_createStatements.push_back(CREATEINDEX_SOURCE_GUID);

        m_databaseConnection->AddStatement(CREATEINDEX_PRODUCT_NAME, CREATEINDEX_PRODUCT_NAME_STATEMENT);
        m_createStatements.push_back(CREATEINDEX_PRODUCT_NAME);

        m_databaseConnection->AddStatement(CREATEINDEX_PRODUCT_SUBID, CREATEINDEX_PRODUCT_SUBID_STATEMENT);
        m_createStatements.push_back(CREATEINDEX_PRODUCT_SUBID);

        m_databaseConnection->AddStatement(CREATEINDEX_PRODUCTDEPENDENCIES_PRODUCTPK, CREATEINDEX_PRODUCTDEPENDENCIES_PRODUCTPK_STATEMENT);
        m_createStatements.push_back(CREATEINDEX_PRODUCTDEPENDENCIES_PRODUCTPK);

        m_databaseConnection->AddStatement(CREATEINDEX_FILE_NAME, CREATEINDEX_FILE_NAME_STATEMENT);
        m_createStatements.push_back(CREATEINDEX_FILE_NAME);

        m_databaseConnection->AddStatement(CREATEINDEX_SCANFOLDERS_FILES, CREATEINDEX_SCANFOLDERS_FILES_STATEMENT);
        m_createStatements.push_back(CREATEINDEX_SCANFOLDERS_FILES);

        m_databaseConnection->AddStatement(DELETE_AUTO_SUCCEED_JOBS, DELETE_AUTO_SUCCEED_JOBS_STATEMENT);
    }

    void AssetDatabaseConnection::VacuumAndAnalyze()
    {
        if (m_databaseConnection)
        {
            m_databaseConnection->ExecuteOneOffStatement("VACUUM");
            m_databaseConnection->ExecuteOneOffStatement("ANALYZE");
        }
    }

    bool AssetDatabaseConnection::GetScanFolderByScanFolderID(AZ::s64 scanfolderID, ScanFolderDatabaseEntry& entry)
    {
        bool found = false;
        QueryScanFolderByScanFolderID( scanfolderID, 
            [&](ScanFolderDatabaseEntry& scanFolderEntry)
            {
                entry = scanFolderEntry;
                found = true;
                return false;//only one
            });
        return found;
    }

    bool AssetDatabaseConnection::GetScanFolderBySourceID(AZ::s64 sourceID, ScanFolderDatabaseEntry& entry)
    {
        bool found = false;
        QueryScanFolderBySourceID( sourceID, 
            [&](ScanFolderDatabaseEntry& scanFolderEntry)
        {
            entry = scanFolderEntry;
            found = true;
            return false;//only one
        });
        return found;
    }

    bool AssetDatabaseConnection::GetScanFolderByJobID(AZ::s64 jobID, ScanFolderDatabaseEntry& entry)
    {
        bool found = false;
        QueryScanFolderByJobID( jobID, 
            [&](ScanFolderDatabaseEntry& scanFolderEntry)
        {
            entry = scanFolderEntry;
            found = true;
            return false;  // return false because we only want one entry, no need to continue scanning rows
        });
        return found ;
    }

    bool AssetDatabaseConnection::GetScanFolderByProductID(AZ::s64 productID, ScanFolderDatabaseEntry& entry)
    {
        bool found = false;
        QueryScanFolderByProductID( productID, 
            [&](ScanFolderDatabaseEntry& scanFolderEntry)
        {
            entry = scanFolderEntry;
            found = true;
            return false; // stop after the first result
        });
        return found;
    }

    bool AssetDatabaseConnection::GetScanFolderByPortableKey(QString portableKey, ScanFolderDatabaseEntry& entry)
    {
        bool found = false;
        QueryScanFolderByPortableKey(portableKey.toUtf8().constData(),
            [&](ScanFolderDatabaseEntry& scanFolder)
            {
                entry = AZStd::move(scanFolder);
                found = true;
                return false; // stop after the first result
            });
        return found;
    }

    bool AssetDatabaseConnection::GetScanFolders(ScanFolderDatabaseEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QueryScanFoldersTable(
            [&](ScanFolderDatabaseEntry& scanFolder)
            {
                found = true;
                container.push_back();
                container.back() = AZStd::move(scanFolder);
                return true;  // return true to collect more rows since we are filling a container
            });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::SetScanFolder(ScanFolderDatabaseEntry& entry)
    {
        if (!ValidateDatabaseTable(INSERT_SCANFOLDER, "ScanFolders"))
        {
            AZ_Error(LOG_NAME, false, "Could not find ScanFolder table");
            return false;
        }

        ScanFolderDatabaseEntry existingEntry;

        if (entry.m_scanFolderID == -1)
        {
            //they didn't supply an id, add to database!

            //make sure the scan path is not already in the database
            if (GetScanFolderByPortableKey(entry.m_portableKey.c_str(), existingEntry))
            {
                //its in the database already, update the input entry id and try again:
                entry.m_scanFolderID = existingEntry.m_scanFolderID;
                return SetScanFolder(entry);
            }

            //its not in the database, add it
            // it is a single statement, do not wrap it in a transaction, this wastes a lot of time.
            if (!s_InsertScanfolderQuery.BindAndStep(*m_databaseConnection, entry.m_scanFolder.c_str(), entry.m_displayName.c_str(), entry.m_portableKey.c_str(), entry.m_outputPrefix.c_str(), entry.m_isRoot))
            {
                return false;
            }

            if (GetScanFolderByPortableKey(entry.m_portableKey.c_str(), existingEntry))
            {
                //its in the database already, update the input entry
                entry.m_scanFolderID = existingEntry.m_scanFolderID;
                return true;
            }

            AZ_Error(LOG_NAME, false, "Failed to read the new scan folder into the database.");
            return false;
        }
        else
        {
            //they supplied an id, see if it exists in the database
            if (!GetScanFolderByScanFolderID(entry.m_scanFolderID, existingEntry))
            {
                AZ_WarningOnce(LOG_NAME, false, "Failed to write the new scan folder into the database.");
                return false;
            }

            return s_UpdateScanfolderQuery.BindAndStep(*m_databaseConnection, entry.m_scanFolder.c_str(), entry.m_displayName.c_str(), entry.m_portableKey.c_str(), entry.m_outputPrefix.c_str(), entry.m_isRoot, entry.m_scanFolderID);
        }
    }

    bool AssetDatabaseConnection::RemoveScanFolder(AZ::s64 scanFolderID)
    {
        if (!ValidateDatabaseTable(DELETE_SCANFOLDER, "ScanFolders"))
        {
            return false;
        }

        ScopedTransaction transaction(m_databaseConnection);

        if (!s_DeleteScanfolderQuery.BindAndStep(*m_databaseConnection, scanFolderID))
        {
            return false;
        }

        transaction.Commit();

        return true;
    }

    bool AssetDatabaseConnection::RemoveScanFolders(ScanFolderDatabaseEntryContainer& container)
    {
        bool succeeded = true;
        for (auto& entry : container)
        {
            succeeded &= RemoveScanFolder(entry.m_scanFolderID);
            if (succeeded)
            {
                entry.m_scanFolderID = -1;//set it to default -1 as this is no longer exists
            }
        }
        return succeeded;
    }

    bool AssetDatabaseConnection::GetSourceBySourceID(AZ::s64 sourceID, SourceDatabaseEntry& entry)
    {
        bool found = false;
        QuerySourceBySourceID( sourceID,
            [&](SourceDatabaseEntry& source)
            {
                found = true;
                entry = AZStd::move(source);
                return false;  // return false in order to stop iterating any further - we are only populating one entry.
            });
        return found;
    }

    bool AssetDatabaseConnection::GetSourceBySourceGuid(AZ::Uuid sourceGuid, SourceDatabaseEntry& entry)
    {
        bool found = false;
        QuerySourceBySourceGuid(sourceGuid,
            [&](SourceDatabaseEntry& source)
        {
            found = true;
            entry = AZStd::move(source);
            return false;  // return false in order to stop iterating any further - we are only populating one entry.
        });
        return found;
    }

    bool AssetDatabaseConnection::GetSources(SourceDatabaseEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QuerySourcesTable(
            [&](SourceDatabaseEntry& source)
            {
                found = true;
                container.push_back();
                container.back() = AZStd::move(source);
                return true; // return true to continue iterating over additional results, we are populating a container
            });
        return  found && succeeded;
    }

    bool AssetDatabaseConnection::GetSourcesBySourceName(QString exactSourceName, SourceDatabaseEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QuerySourceBySourceName(AssetUtilities::NormalizeFilePath(exactSourceName).toUtf8().constData(),
            [&](SourceDatabaseEntry& source)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(source);
            return true;  // return true to continue iterating over additional results, we are populating a container
        });
        return  found && succeeded;
    }

    bool AssetDatabaseConnection::GetSourcesBySourceNameScanFolderId(QString exactSourceName, AZ::s64 scanFolderID, SourceDatabaseEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QuerySourceBySourceNameScanFolderID(exactSourceName.toUtf8().constData(),
            scanFolderID,
            [&](SourceDatabaseEntry& source)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(source);
            return true;  // return true to continue iterating over additional results, we are populating a container
        });
        return  found && succeeded;
    }

    bool AssetDatabaseConnection::GetSourcesLikeSourceName(QString likeSourceName, LikeType likeType, SourceDatabaseEntryContainer& container)
    {
        if (likeSourceName.isEmpty())
        {
            return false;
        }

        bool found = false;
        bool succeeded = QuerySourceLikeSourceName(likeSourceName.toUtf8().constData(), likeType,
            [&](SourceDatabaseEntry& source)
            {
                found = true;
                container.push_back();
                container.back() = AZStd::move(source);
                return true;  // return true to continue iterating over additional results, we are populating a container
            });
        return  found && succeeded;
    }

    bool AssetDatabaseConnection::GetSourceByJobID(AZ::s64 jobID, SourceDatabaseEntry& entry)
    {
        bool found = false;
        QuerySourceByJobID( jobID,
            [&](SourceDatabaseEntry& source)
        {
            found = true;
            entry = AZStd::move(source);
            return false; // stop after the first result
        });
        return found;
    }

    bool AssetDatabaseConnection::GetSourceByProductID(AZ::s64 productID, SourceDatabaseEntry& entry)
    {
        bool found = false;
        QuerySourceByProductID( productID,
            [&](SourceDatabaseEntry& source)
        {
            found = true;
            entry = AZStd::move(source);
            return false; // stop after the first result
        });
        return found;
    }

    bool AssetDatabaseConnection::GetSourcesByProductName(QString exactProductName, SourceDatabaseEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QueryCombinedByProductName(exactProductName.toUtf8().constData(),
                [&](CombinedDatabaseEntry& combined)
                {
                    found = true;
                    container.push_back();
                    container.back() = AZStd::move(combined);
                    return true; // return true to continue collecting all
                });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetSourcesLikeProductName(QString likeProductName, LikeType likeType, SourceDatabaseEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QueryCombinedLikeProductName(likeProductName.toUtf8().constData(), likeType,
                [&](CombinedDatabaseEntry& combined)
                {
                    found = true;
                    container.push_back();
                    container.back() = AZStd::move(combined);
                    return true;//all
                });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::SetSource(SourceDatabaseEntry& entry)
    {
        if (!ValidateDatabaseTable(INSERT_SOURCE, "Sources"))
        {
            AZ_Error(LOG_NAME, false, "Could not find Sources table");
            return false;
        }

        if (entry.m_sourceID == -1)
        {
            //they didn't supply an id, add to database

            //first make sure its not already in the database
            SourceDatabaseEntry existingEntry;
            if (GetSourceBySourceGuid(entry.m_sourceGuid, existingEntry))
            {
                // this source guid already exists.  note that the UUID is final, there is only ever one UUID for a source
                // if folders override each other, the UUID stays the same but the scanfolder field changes but its still considered the same source file.
                entry.m_sourceID = existingEntry.m_sourceID;
                return SetSource(entry); // now update the existing field
            }

            if (!s_InsertSourceQuery.BindAndStep(*m_databaseConnection, entry.m_scanFolderPK, entry.m_sourceName.c_str(), entry.m_sourceGuid))
            {
                return false;
            }

            //now that its in the database get the id
            if (GetSourceBySourceGuid(entry.m_sourceGuid, existingEntry))
            {
                entry.m_sourceID = existingEntry.m_sourceID;
                return true;
            }

            AZ_Error(LOG_NAME, false, "Failed to read the new source into the database.");
            return false;
        }
        else
        {
            //they supplied an id, see if it exists in the database
            SourceDatabaseEntry existingEntry;
            if (!GetSourceBySourceID(entry.m_sourceID, existingEntry))
            {
                //they supplied an id but is not in the database!
                AZ_Error(LOG_NAME, false, "Failed to write the source into the database.");
                return false;
            }

            // don't bother updating the database if all fields are equal.
            // note that we already looked it up by source ID
            if ((existingEntry.m_scanFolderPK == entry.m_scanFolderPK) &&
                (existingEntry.m_sourceGuid == entry.m_sourceGuid) &&
                (existingEntry.m_sourceName == entry.m_sourceName))
            {
                return true;
            }


            return s_UpdateSourceQuery.BindAndStep(*m_databaseConnection, entry.m_scanFolderPK, entry.m_sourceName.c_str(), entry.m_sourceGuid, entry.m_sourceID);
        }
    }

    // this must actually delete the source
    bool AssetDatabaseConnection::RemoveSource(AZ::s64 sourceID)
    {
        if (!ValidateDatabaseTable(DELETE_SOURCE, "Sources"))
        {
            AZ_Error(LOG_NAME, false, "Could not find Sources table");
            return false;
        }

        ScopedTransaction transaction(m_databaseConnection);

        if (!s_DeleteSourceQuery.BindAndStep(*m_databaseConnection, sourceID))
        {
            return false;
        }

        transaction.Commit();

        return true;
    }

    bool AssetDatabaseConnection::RemoveSources(SourceDatabaseEntryContainer& container)
    {
        bool succeeded = true;
        for (auto& entry : container)
        {
            succeeded &= RemoveSource(entry.m_sourceID);
            if (succeeded)
            {
                entry.m_sourceID = -1;//set it to -1 as it no longer exists
            }
        }
        return succeeded;
    }

    bool AssetDatabaseConnection::RemoveSourcesByScanFolderID(AZ::s64 scanFolderID)
    {
        bool found = false;
        bool succeeded = QuerySourceByScanFolderID(scanFolderID,
                [&](SourceDatabaseEntry& source)
                {
                    found = true;
                    succeeded &= RemoveSource(source.m_sourceID);
                    return true;//all
                });
        return found && succeeded;
    }

    AZ::s64 AssetDatabaseConnection::GetHighestJobRunKey()
    {
        if (!m_databaseConnection)
        {
            return 0;
        }

        StatementAutoFinalizer autoFinal;

        if (!s_GetHighestJobrunkeyQuery.Bind(*m_databaseConnection, autoFinal))
        {
            return 0;
        }

        Statement* statement = autoFinal.Get();

        if (statement->Step() == Statement::SqlError)
        {
            // this is okay, since the table may be empty.
            return 0;
        }

        return statement->GetColumnInt64(0);
    }

    bool AssetDatabaseConnection::GetJobs(JobDatabaseEntryContainer& container, AZ::Uuid builderGuid, QString jobKey, QString platform, JobStatus status)
    {
        bool found = false;
        bool succeeded = QueryJobsTable(
                [&](JobDatabaseEntry& job)
                {
                    found = true;
                    container.push_back();
                    container.back() = AZStd::move(job);
                    return true;//all
                },  builderGuid,
                jobKey.isEmpty() ? nullptr : jobKey.toUtf8().constData(),
                platform.isEmpty() ? nullptr : platform.toUtf8().constData(),
                status);
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetJobByJobID(AZ::s64 jobID, JobDatabaseEntry& entry)
    {
        bool found = false;
        QueryJobByJobID( jobID,
            [&](JobDatabaseEntry& job)
        {
            found = true;
            entry = AZStd::move(job);
            return false; // stop after the first result
        });
        return found;
    }

    bool AssetDatabaseConnection::GetJobByProductID(AZ::s64 productID, JobDatabaseEntry& entry)
    {
        bool found = false;
        QueryJobByProductID(productID,
            [&](JobDatabaseEntry& job)
        {
            found = true;
            entry = AZStd::move(job);
            return false; // stop after the first result
        });
        return found;
    }

    bool AssetDatabaseConnection::GetJobsBySourceID(AZ::s64 sourceID, JobDatabaseEntryContainer& container, AZ::Uuid builderGuid, QString jobKey, QString platform, JobStatus status)
    {
        bool found = false;
        bool succeeded = QueryJobBySourceID(sourceID,
            [&](JobDatabaseEntry& job)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(job);
            return true; // continue to fetch more rows.
        },  builderGuid,
            jobKey.isEmpty() ? nullptr : jobKey.toUtf8().constData(),
            platform.isEmpty() ? nullptr : platform.toUtf8().constData(),
            status);
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetJobsBySourceName(QString exactSourceName, JobDatabaseEntryContainer& container, AZ::Uuid builderGuid, QString jobKey, QString platform, JobStatus status)
    {
        bool found = false;
        bool succeeded = QuerySourceBySourceName(exactSourceName.toUtf8().constData(),
            [&](SourceDatabaseEntry& source)
        {
            succeeded = QueryJobBySourceID(source.m_sourceID,
                [&](JobDatabaseEntry& job)
                {
                    found = true;
                    container.push_back();
                    container.back() = AZStd::move(job);
                    return true;//all
                },  builderGuid,
                    jobKey.isEmpty() ? nullptr : jobKey.toUtf8().constData(),
                    platform.isEmpty() ? nullptr : platform.toUtf8().constData(),
                    status);
            return true; // continue to fetch more rows.
        });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetJobsLikeSourceName(QString likeSourceName, LikeType likeType, JobDatabaseEntryContainer& container, AZ::Uuid builderGuid, QString jobKey, QString platform, JobStatus status)
    {
        if (likeSourceName.isEmpty())
        {
            return false;
        }

        bool found = false;
        bool succeeded = QuerySourceLikeSourceName(likeSourceName.toUtf8().constData(), likeType,
            [&](SourceDatabaseEntry& source)
        {
            succeeded = QueryJobBySourceID(source.m_sourceID,
                [&](JobDatabaseEntry& job)
            {
                found = true;
                container.push_back();
                container.back() = AZStd::move(job);
                return true;//all
            },  builderGuid,
                jobKey.isEmpty() ? nullptr : jobKey.toUtf8().constData(),
                platform.isEmpty() ? nullptr : platform.toUtf8().constData(),
                status);
            return true; // continue to fetch more rows.
        });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetJobsByProductName(QString exactProductName, JobDatabaseEntryContainer& container, AZ::Uuid builderGuid, QString jobKey, QString platform, JobStatus status)
    {
        bool found = false;
        bool succeeded = QueryProductByProductName(exactProductName.toUtf8().constData(),
            [&](ProductDatabaseEntry& product)
        {
            succeeded = QueryJobByProductID(product.m_productID,
                [&](JobDatabaseEntry& job)
            {
                found = true;
                container.push_back();
                container.back() = AZStd::move(job);
                return true;//all
            });
            return true; // continue to fetch more rows.
        },  builderGuid,
            jobKey.isEmpty() ? nullptr : jobKey.toUtf8().constData(),
            platform.isEmpty() ? nullptr : platform.toUtf8().constData(),
            status);
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetJobsLikeProductName(QString likeProductName, LikeType likeType, JobDatabaseEntryContainer& container, AZ::Uuid builderGuid, QString jobKey, QString platform, JobStatus status)
    {
        bool found = false;
        bool succeeded = QueryProductLikeProductName(likeProductName.toUtf8().constData(), likeType,
            [&](ProductDatabaseEntry& product)
        {
            succeeded = QueryJobByProductID(product.m_productID,
                [&](JobDatabaseEntry& job)
            {
                found = true;
                container.push_back();
                container.back() = AZStd::move(job);
                return true; // continue to fetch more rows for the QueryJobByProductId call
            });
            return true; // continue to fetch more rows for the QueryProductLikeProductName call
        },  builderGuid,
            jobKey.isEmpty() ? nullptr : jobKey.toUtf8().constData(),
            platform.isEmpty() ? nullptr : platform.toUtf8().constData(),
            status);
        return found && succeeded;
    }

    bool AssetDatabaseConnection::SetJob(JobDatabaseEntry& entry)
    {
        if (!ValidateDatabaseTable("SetJob", "Jobs"))
        {
            AZ_Error(LOG_NAME, false, "Could not find Jobs table");
            return false;
        }

        if (entry.m_jobRunKey <= 0)
        {
            AZ_Error(LOG_NAME, false, "You must specify a valid Job Run Key for a job to make it into the database.\n");
            return false;
        }

        if (entry.m_jobID == -1)
        {
            //they didn't supply an id, add to database

            //make sure its not already in the database
            JobDatabaseEntryContainer existingJobs;
            if (GetJobsBySourceID(entry.m_sourcePK, existingJobs, entry.m_builderGuid, entry.m_jobKey.c_str(), entry.m_platform.c_str()))
            {
                //see if this job is already here
                for (const auto& existingjob : existingJobs)
                {
                    if (existingjob == entry)
                    {
                        //this job already exists
                        entry.m_jobID = existingjob.m_jobID;
                        return true;
                    }
                }
            }

            if (!s_InsertJobQuery.BindAndStep(*m_databaseConnection, entry.m_sourcePK, entry.m_jobKey.c_str(), entry.m_fingerprint, entry.m_platform.c_str(),
                entry.m_builderGuid, static_cast<int>(entry.m_status), entry.m_jobRunKey, entry.m_firstFailLogTime, entry.m_firstFailLogFile.c_str(),
                entry.m_lastFailLogTime, entry.m_lastFailLogFile.c_str(), entry.m_lastLogTime, entry.m_lastLogFile.c_str()))
            {
                return false;
            }

            //make sure its now in the database
            existingJobs.clear();
            if (GetJobsBySourceID(entry.m_sourcePK, existingJobs, entry.m_builderGuid, entry.m_jobKey.c_str(), entry.m_platform.c_str()))
            {
                //see if this job is already here
                for (const auto& existingjob : existingJobs)
                {
                    if (existingjob == entry)
                    {
                        //this job already exists
                        entry.m_jobID = existingjob.m_jobID;
                        return true;
                    }
                }
            }

            AZ_Warning(LOG_NAME, false, "Failed to read the new job from the database.");
            return false;
        }
        else
        {
            //they supplied an id, see if it exists in the database
            JobDatabaseEntry existingEntry;
            if (!GetJobByJobID(entry.m_jobID, existingEntry))
            {
                AZ_Error(LOG_NAME, false, "Failed to find the job in the database.");
                return false;
            }

            //its in the database already, if its not the same update the database
            if (existingEntry == entry)
            {
                return true;
            }

            return s_UpdateJobQuery.BindAndStep(*m_databaseConnection, entry.m_sourcePK, entry.m_jobKey.c_str(), entry.m_fingerprint, entry.m_platform.c_str(),
                entry.m_builderGuid, static_cast<int>(entry.m_status), entry.m_jobRunKey, entry.m_firstFailLogTime, entry.m_firstFailLogFile.c_str(),
                entry.m_lastFailLogTime, entry.m_lastFailLogFile.c_str(), entry.m_lastLogTime, entry.m_lastLogFile.c_str(), entry.m_jobID);
        }
    }

    // this must actually delete the job
    bool AssetDatabaseConnection::RemoveJob(AZ::s64 jobID)
    {
        if (!ValidateDatabaseTable(DELETE_JOB, "Jobs"))
        {
            AZ_Error(LOG_NAME, false, "Could not find Jobs table");
            return false;
        }

        ScopedTransaction transaction(m_databaseConnection);

        if(!s_DeleteJobQuery.BindAndStep(*m_databaseConnection, jobID))
        {
            return false;
        }

        transaction.Commit();

        return true;
    }

    bool AssetDatabaseConnection::RemoveJobs(JobDatabaseEntryContainer& container)
    {
        bool succeeded = true;
        for (auto& entry : container)
        {
            succeeded &= RemoveJob(entry.m_jobID);
            if (succeeded)
            {
                entry.m_jobID = -1; //set it to -1 as the id is no longer valid
            }
        }

        return succeeded;
    }

    bool AssetDatabaseConnection::RemoveJobByProductID(AZ::s64 productID)
    {
        JobDatabaseEntry job;
        bool succeeded = GetJobByProductID(productID, job);
        if (succeeded)
        {
            succeeded &= RemoveJob(job.m_jobID);
        }
        return succeeded;
    }

    bool AssetDatabaseConnection::GetProductByProductID(AZ::s64 productID, ProductDatabaseEntry& entry)
    {
        bool found = false;
        QueryProductByProductID(productID,
            [&](ProductDatabaseEntry& product)
            {
                found = true;
                entry = AZStd::move(product);
                return false; // stop after the first one is found.
            });
        return found;
    }

    bool AssetDatabaseConnection::GetProducts(ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid, QString jobKey, QString platform, JobStatus status)
    {
        bool found = false;
        bool succeeded = QueryProductsTable(
                [&](ProductDatabaseEntry& product)
            {
                found = true;
                container.push_back();
                container.back() = AZStd::move(product);
                return true; // continue fetching more results.
            }, builderGuid,
               jobKey.isEmpty() ? nullptr : jobKey.toUtf8().constData(),
               platform.isEmpty() ? nullptr : platform.toUtf8().constData(),
               status);
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetProductsByProductName(QString exactProductName, ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid, QString jobKey, QString platform, JobStatus status)
    {
        bool found = false;
        bool succeeded = QueryProductByProductName(exactProductName.toUtf8().constData(),
            [&](ProductDatabaseEntry& product)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(product);
            return true; // continue fetching more results.
        }, builderGuid,
           jobKey.isEmpty() ? nullptr : jobKey.toUtf8().constData(),
           platform.isEmpty() ? nullptr : platform.toUtf8().constData(),
           status);
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetProductsLikeProductName(QString likeProductName, LikeType likeType, ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid, QString jobKey, QString platform, JobStatus status)
    {
        bool found = false;
        
        if (likeProductName.isEmpty())
        {
            return false;
        }

        bool succeeded = QueryProductLikeProductName(likeProductName.toUtf8().constData(), likeType,
                [&](ProductDatabaseEntry& product)
            {
                found = true;
                container.push_back();
                container.back() = AZStd::move(product);
                return true; // continue fetching more results.
            }, builderGuid,
               jobKey.isEmpty() ? nullptr : jobKey.toUtf8().constData(),
               platform.isEmpty() ? nullptr : platform.toUtf8().constData(),
               status);
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetProductsBySourceName(QString exactSourceName, ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid, QString jobKey, QString platform, JobStatus status)
    {
        bool found = false;
        bool succeeded = QueryProductBySourceName(exactSourceName.toUtf8().constData(),
                [&](ProductDatabaseEntry& product)
            {
                found = true;
                container.push_back();
                container.back() = AZStd::move(product);
                return true; // continue fetching more results.
            }, builderGuid,
               jobKey.isEmpty() ? nullptr : jobKey.toUtf8().constData(),
               platform.isEmpty() ? nullptr : platform.toUtf8().constData(),
               status);
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetProductsLikeSourceName(QString likeSourceName, LikeType likeType, ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid, QString jobKey, QString platform, JobStatus status)
    {
        if (likeSourceName.isEmpty())
        {
            return false;
        }

        bool found = false;
        bool succeeded = QueryProductLikeSourceName(likeSourceName.toUtf8().constData(), likeType,
            [&](ProductDatabaseEntry& product)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(product);
            return true; // continue fetching more results.
        }, builderGuid,
            jobKey.isEmpty() ? nullptr : jobKey.toUtf8().constData(),
            platform.isEmpty() ? nullptr : platform.toUtf8().constData(),
            status);
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetProductsBySourceID(AZ::s64 sourceID, ProductDatabaseEntryContainer& container, AZ::Uuid builderGuid, QString jobKey, QString platform, JobStatus status)
    {
        bool found = false;
        bool succeeded = QueryCombinedBySourceID(sourceID,
            [&](CombinedDatabaseEntry& combined)
            {
                found = true;
                container.push_back();
                container.back() = AZStd::move(combined);
                return true; // continue fetching more results.
            }, builderGuid,
               jobKey.isEmpty() ? nullptr : jobKey.toUtf8().constData(),
               platform.isEmpty() ? nullptr : platform.toUtf8().constData(),
               status);
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetProductsByJobID(AZ::s64 jobID, ProductDatabaseEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QueryCombinedByJobID(jobID,
            [&](CombinedDatabaseEntry& combined)
            {
                found = true;
                container.push_back();
                container.back() = AZStd::move(combined);
                return true; // continue fetching more results.
            });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetProductByJobIDSubId(AZ::s64 jobID, AZ::u32 subID, AzToolsFramework::AssetDatabase::ProductDatabaseEntry& result)
    {
        bool found = false;
        QueryProductByJobIDSubID(jobID, subID, 
            [&](ProductDatabaseEntry& resultFromDB)
        {
            found = true;
            result = AZStd::move(resultFromDB);
            return false; // stop after the first result
        });
        return found;
    }

    //! For a given source, set the list of products for that source.
    //! Removes any data that's present and overwrites it with the new list
    //! Note that an empty list is in fact acceptable data, it means the source emitted no products
    bool AssetDatabaseConnection::SetProduct(ProductDatabaseEntry& entry)
    {
        if(!ValidateDatabaseTable(s_InsertProductQuery.m_statementName, s_InsertProductQuery.m_tableName))
        {
            AZ_Error(LOG_NAME, false, "Could not find %s table", s_InsertProductQuery.m_tableName);
            return false;
        }

        bool wasAlreadyInDatabase = false;
        ProductDatabaseEntry existingProductInDatabase;

        if (entry.m_productID == -1)
        {
            // they didn't set an id, add to database
            // but make sure its not already in the database before doing so:
            if (GetProductByJobIDSubId(entry.m_jobPK, entry.m_subID, existingProductInDatabase))
            {
                wasAlreadyInDatabase = true;
            }
        }
        else
        {
            if (GetProductByProductID(entry.m_productID, existingProductInDatabase))
            {
                wasAlreadyInDatabase = true;
            }
            else
            {
                // its not OK to be pushing specific IDs into the database that don't exist.
                AZ_Error(LOG_NAME, false, "Attempt to call SetProduct(...) with a database productID (%lli) that is not -1 but also doesn't exist.", entry.m_productID);
                return false;
            }
        }

        // we can early out if it was in the database and the database entry is identical to the new one:
        if (wasAlreadyInDatabase)
        {
            entry.m_productID = existingProductInDatabase.m_productID;
            if (entry == existingProductInDatabase)
            {
                return true;
            }
        }

        // if we get here, we need to either insert or update in the database.

        {
            // note, intentional scope created for the statement finalizer
            const char* statementToUse = wasAlreadyInDatabase ? UPDATE_PRODUCT : INSERT_PRODUCT;
            StatementAutoFinalizer autoFinalizer(*m_databaseConnection, statementToUse);
            Statement* statement = autoFinalizer.Get();
            AZ_Assert(statement, "Statement not found: %s", statementToUse);

            if (wasAlreadyInDatabase)
            {
                // the "update" version requires the prior product row Id:
                int productIdx = statement->GetNamedParamIdx(":productid");
                if (!productIdx)
                {
                    AZ_Error(LOG_NAME, false, "Could not find the Idx for :productid for %s ", statementToUse);
                    return false;
                }

                statement->BindValueInt64(productIdx, entry.m_productID);
            }

            int jobIdx = statement->GetNamedParamIdx(":jobid");
            if(!jobIdx)
            {
                AZ_Error(LOG_NAME, false, "Could not find the Idx for :jobid for %s ", statementToUse);
                return false;
            }
            statement->BindValueInt64(jobIdx, entry.m_jobPK);

            int subIdIdx = statement->GetNamedParamIdx(":subid");
            if(!subIdIdx)
            {
                AZ_Error(LOG_NAME, false, "Could not find the Idx for :subid for %s ", statementToUse);
                return false;
            }
            statement->BindValueInt(subIdIdx, entry.m_subID);
            
            int productNameIdx = statement->GetNamedParamIdx(":productname");
            if(!productNameIdx)
            {
                AZ_Error(LOG_NAME, false, "Could not find the Idx for :productname for %s ", statementToUse);
                return false;
            }
            statement->BindValueText(productNameIdx, entry.m_productName.c_str());

            int assetTypeIdx = statement->GetNamedParamIdx(":assettype");
            if(!assetTypeIdx)
            {
                AZ_Error(LOG_NAME, false, "Could not find the Idx for :assettype for %s ", statementToUse);
                return false;
            }
            statement->BindValueUuid(assetTypeIdx, entry.m_assetType);

            int legacyGuidIdx = statement->GetNamedParamIdx(":legacyguid");
            if(!legacyGuidIdx)
            {
                AZ_Error(LOG_NAME, false, "Could not find the Idx for :legacyguid for %s ", statementToUse);
                return false;
            }
            statement->BindValueUuid(legacyGuidIdx, entry.m_legacyGuid);

            if(statement->Step() == Statement::SqlError)
            {
                AZ_Error(LOG_NAME, false, "Failed to execute the %s statement", statementToUse);
                return false;
            }

            if (!wasAlreadyInDatabase)
            {
                // just read what the last inserted row ID is.
                // (this is why database access must only be one thread per connection, as this could otherwise
                // be mutated by the other thread.  its stored on the connection object, not a TLS variable)
                entry.m_productID = m_databaseConnection->GetLastRowID();
            }
        }
        return true;
    }

    bool AssetDatabaseConnection::SetProducts(ProductDatabaseEntryContainer& container)
    {
        if (container.empty())
        {
            return false;
        }
        
        bool succeeded = true;
        for (auto& entry : container)
        {
            succeeded &= SetProduct(entry);
        }
        return succeeded;
    }

    //! Clear the products for a given source.  This removes the entry entirely, not just sets it to empty.
    bool AssetDatabaseConnection::RemoveProduct(AZ::s64 productID)
    {
        if (!ValidateDatabaseTable("RemoveProduct", "Products"))
        {
            AZ_Error(LOG_NAME, false, "Could not find Products table");
            return false;
        }

        ScopedTransaction transaction(m_databaseConnection);

        if (!s_DeleteProductQuery.BindAndStep(*m_databaseConnection, productID))
        {
            return false;
        }

        bool wasEffective = (m_databaseConnection->GetNumAffectedRows() != 0);

        transaction.Commit();

        return wasEffective;
    }

    bool AssetDatabaseConnection::RemoveProducts(ProductDatabaseEntryContainer& container)
    {
        if (container.empty())
        {
            return false;
        }

        bool succeeded = true;
        for (auto& entry : container)
        {
            succeeded &= RemoveProduct(entry.m_productID);
            if (succeeded)
            {
                entry.m_productID = -1;
            }
        }
        return succeeded;
    }

    bool AssetDatabaseConnection::RemoveProductsByJobID(AZ::s64 jobID)
    {
        if (!ValidateDatabaseTable(DELETE_PRODUCTS_BY_JOBID, "Products"))
        {
            AZ_Error(LOG_NAME, false, "Could not find Jobs or Products table");
            return false;
        }

        ScopedTransaction transaction(m_databaseConnection);

        if (!s_DeleteProductsByJobidQuery.BindAndStep(*m_databaseConnection, jobID))
        {
            return false;
        }


        bool wasEffective = (m_databaseConnection->GetNumAffectedRows() != 0);

        transaction.Commit();

        return wasEffective;
    }

    bool AssetDatabaseConnection::RemoveProductsBySourceID(AZ::s64 sourceID, AZ::Uuid builderGuid, QString jobKey, QString platform, JobStatus status)
    {
        if ( (!builderGuid.IsNull()) || (jobKey != nullptr) || (status != AssetSystem::JobStatus::Any) )
        {
            //we have to do custom query the delete
            ProductDatabaseEntryContainer products;
            bool succeeded = GetProductsBySourceID(sourceID, products, builderGuid, jobKey, platform, status);
            if (succeeded)
            {
                succeeded &= RemoveProducts(products);
            }
            return succeeded;
        }

        if (!ValidateDatabaseTable("RemoveProductsBySourceID", "Jobs") ||
            !ValidateDatabaseTable("RemoveProductsBySourceID", "Products"))
        {
            AZ_Error(LOG_NAME, false, "Could not find Jobs or Products table");
            return false;
        }

        ScopedTransaction transaction(m_databaseConnection);

        if(!platform.isEmpty())
        {
            AZStd::string platformStr = platform.toUtf8().constData();
            
            if (!s_DeleteProductsBySourceidPlatformQuery.BindAndStep(*m_databaseConnection, sourceID, platformStr.c_str()))
            {
                return false;
            }
        }
        else if(!s_DeleteProductsBySourceidQuery.BindAndStep(*m_databaseConnection, sourceID))
        {
            return false;
        }


        bool wasEffective = (m_databaseConnection->GetNumAffectedRows() != 0);

        transaction.Commit();

        return wasEffective;
    }

    bool AssetDatabaseConnection::GetJobInfoByJobID(AZ::s64 jobID, JobInfo& entry)
    {
        bool found = false;
        QueryJobInfoByJobID(jobID,
            [&](JobInfo& jobInfo)
        {
            found = true;
            entry = AZStd::move(jobInfo);
            return false; // stop after the first result
        });
        return found;
    }

    bool AssetDatabaseConnection::GetJobInfoByJobKey(AZStd::string jobKey, JobInfoContainer& container)
    {
        bool found = false;
        bool succeeded = QueryJobInfoByJobKey(jobKey,
            [&](JobInfo& jobInfo)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(jobInfo);
            return true; // return true to keep iterating over further rows.
        });
        return found && succeeded;

    }

    bool AssetDatabaseConnection::GetJobInfoByJobRunKey(AZ::u64 jobRunKey, JobInfoContainer& container)
    {
        bool found = false;
        bool succeeded = QueryJobInfoByJobRunKey(jobRunKey,
            [&](JobInfo& jobInfo)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(jobInfo);
            return true; // return true to keep iterating over further rows.
        });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetJobInfoBySourceName(QString exactSourceName, JobInfoContainer& container, AZ::Uuid builderGuid, QString jobKey, QString platform, JobStatus status)
    {
        bool found = false;
        bool succeeded = QueryJobInfoBySourceName(exactSourceName.toUtf8().constData(),
            [&](JobInfo& jobInfo)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(jobInfo);
            return true; // return true to keep iterating over further rows.
        }, builderGuid,
            jobKey.isEmpty() ? nullptr : jobKey.toUtf8().constData(),
            platform.isEmpty() ? nullptr : platform.toUtf8().constData(),
            status);
        return found && succeeded;
    }

    bool AssetDatabaseConnection::SetSourceFileDependencies(SourceFileDependencyEntryContainer& container)
    {
        bool succeeded = true;
        for (auto& entry : container)
        {
            succeeded = succeeded && SetSourceFileDependency(entry);
        }
        return succeeded;
    }

    bool AssetDatabaseConnection::SetSourceFileDependency(SourceFileDependencyEntry& entry)
    {
        if (!ValidateDatabaseTable(INSERT_SOURCE_DEPENDENCY, "SourceDependency"))
        {
            AZ_Error(LOG_NAME, false, "Could not find Source Dependency table");
            return false;
        }

        if (entry.m_sourceDependencyID == -1)
        {
            //they didn't supply an id, add to database

            //first make sure its not already in the database
            SourceFileDependencyEntry existingEntry;
            if (GetSourceFileDependency(entry, existingEntry))
            {
                // We already have this entry in the database
                return true;
            }

            if (!s_InsertSourceDependencyQuery.BindAndStep(*m_databaseConnection, entry.m_builderGuid, entry.m_source.c_str(), entry.m_dependsOnSource.c_str()))
            {
                return false;
            }

            //now that its in the database get the id
            if (GetSourceFileDependency(entry, existingEntry))
            {
                entry.m_sourceDependencyID = existingEntry.m_sourceDependencyID;
                return true;
            }

            AZ_Error(LOG_NAME, false, "Failed to read the new source dependency into the database.");
            return false;
        }
        else
        {
            //they supplied an id, see if it exists in the database
            SourceFileDependencyEntry existingEntry;
            if (!GetSourceFileDependencyBySourceDependencyId(entry.m_sourceDependencyID, existingEntry))
            {
                //they supplied an id but is not in the database!
                AZ_Error(LOG_NAME, false, "Failed to write the source dependency into the database.");
                return false;
            }

            // don't bother updating the database if all fields are equal.
            if ((existingEntry.m_builderGuid == entry.m_builderGuid) &&
                (existingEntry.m_source == entry.m_source) &&
                (existingEntry.m_dependsOnSource == entry.m_dependsOnSource))
            {
                return true;
            }

            return s_UpdateSourceDependencyQuery.BindAndStep(*m_databaseConnection, entry.m_dependsOnSource.c_str(), entry.m_builderGuid, entry.m_source.c_str(), entry.m_sourceDependencyID);
        }
    }

    bool AssetDatabaseConnection::RemoveSourceFileDependencies(SourceFileDependencyEntryContainer& container)
    {
        bool succeeded = true;
        for (auto& entry : container)
        {
            succeeded = succeeded && RemoveSourceFileDependency(entry);
            if (succeeded)
            {
                entry.m_sourceDependencyID = -1;//set it to -1 as it no longer exists
            }
        }
        return succeeded;
    }

    bool AssetDatabaseConnection::RemoveSourceFileDependency(const SourceFileDependencyEntry& entry)
    {
        if (!ValidateDatabaseTable(DELETE_SOURCE_DEPENDENCY_SOURCEDEPENDENCYID, "SourceDependency"))
        {
            AZ_Error(LOG_NAME, false, "Could not find Source Dependency table");
            return false;
        }

        if (entry.m_sourceDependencyID == -1)
        {
            //they didn't supply an id, check to make sure that an entry exists in the database and then delete it
            SourceFileDependencyEntry existingEntry;
            if (GetSourceFileDependency(entry, existingEntry))
            {
                return RemoveSourceFileDependency(existingEntry);
            }

            return true; // no such entry exists in the database
        }
        else
        {
            //they supplied an id, see if it exists in the database
            SourceFileDependencyEntry existingEntry;
            if (!GetSourceFileDependencyBySourceDependencyId(entry.m_sourceDependencyID, existingEntry))
            {
                //they supplied an id but is not found in the database!
                AZ_Warning(LOG_NAME, false, "Could not find SourceDependencyid in the database.");
                return true;
            }

            ScopedTransaction transaction(m_databaseConnection);

            if (!s_DeleteSourceDependencySourcedependencyidQuery.BindAndStep(*m_databaseConnection, existingEntry.m_sourceDependencyID))
            {
                return false;
            }

            transaction.Commit();

            return true;
        }
    }

    bool AssetDatabaseConnection::GetSourceFileDependency(const SourceFileDependencyEntry& inputEntry, SourceFileDependencyEntry& databaseEntry)
    {
        bool found = false;
        QuerySourceDependency(inputEntry.m_builderGuid, inputEntry.m_source.c_str(), inputEntry.m_dependsOnSource.c_str(),
            [&](SourceFileDependencyEntry& entry)
        {
            found = true;
            databaseEntry = AZStd::move(entry);
            return false; // stop after the first result
        });
        return found;
    }

    bool AssetDatabaseConnection::GetSourceFileDependenciesByBuilderGUIDAndSource(const AZ::Uuid& builderGuid, const char* source, SourceFileDependencyEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QuerySourceDependencyByBuilderGUIDAndSource(builderGuid, source,
            [&](SourceFileDependencyEntry& entry)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(entry);
            return true; // return true to keep iterating over further rows.
        });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetSourceFileDependenciesByDependsOnSource(const QString& dependsOnSource, SourceFileDependencyEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QuerySourceDependencyByDependsOnSource(dependsOnSource.toUtf8().constData(),nullptr,
            [&](SourceFileDependencyEntry& entry)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(entry);
            return true; // return true to keep iterating over further rows.
        });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetDependsOnSourceBySource(const QString& source, AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QueryDependsOnSourceBySourceDependency(source.toUtf8().constData(),nullptr,
            [&](SourceFileDependencyEntry& entry)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(entry);
            return true; // return true to keep iterating over further rows.
        });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetSourceFileDependencyBySourceDependencyId(AZ::s64 sourceDependencyId, SourceFileDependencyEntry& sourceDependencyEntry)
    {
        bool found = false;
        QuerySourceDependencyBySourceDependencyId(sourceDependencyId,
            [&](SourceFileDependencyEntry& entry)
        {
            found = true;
            sourceDependencyEntry = AZStd::move(entry);
            return false; // stop after the first result
        });
        return found;
    }

    bool AssetDatabaseConnection::CreateOrUpdateLegacySubID(AzToolsFramework::AssetDatabase::LegacySubIDsEntry& entry)
    {
        ScopedTransaction transaction(m_databaseConnection);

        const char* statementName = INSERT_NEW_LEGACYSUBID;

        bool creatingNew = entry.m_subIDsEntryID == -1;

        if (!creatingNew)
        {
            statementName = OVERWRITE_EXISTING_LEGACYSUBID;
        }

        if (creatingNew)
        {
            if (!s_InsertNewLegacysubidQuery.BindAndStep(*m_databaseConnection, entry.m_productPK, entry.m_subID))
            {
                return false;
            }
        }
        else if (!s_OverwriteExistingLegacysubidQuery.BindAndStep(*m_databaseConnection, entry.m_productPK, entry.m_subID, entry.m_subIDsEntryID))
        {
            return false;
        }

        
        if (creatingNew)
        {
            AZ::s64 rowID = m_databaseConnection->GetLastRowID();
            entry.m_subIDsEntryID = rowID;
        }
        else
        {
            if (m_databaseConnection->GetNumAffectedRows() == 0)
            {
                // you specified an invalid key.
                AZ_Warning(LOG_NAME, false, "Failed to CreateOrUpdateLegacySubID in the database - invalid key specified.");
                return false;
            }
        }

        transaction.Commit();
        return true;
    }

    bool AssetDatabaseConnection::RemoveLegacySubID(AZ::s64 legacySubIDsEntryID)
    {
        if (!ValidateDatabaseTable("RemoveLegacySubID", "LegacySubIDs"))
        {
            AZ_Error(LOG_NAME, false, "Could not find LegacySubIDs table");
            return false;
        }

        ScopedTransaction transaction(m_databaseConnection);

        if (!s_DeleteLegacysubidsByPrimaryKeyQuery.BindAndStep(*m_databaseConnection, legacySubIDsEntryID))
        {
            return false;
        }

        transaction.Commit();

        return true;
    }

    bool AssetDatabaseConnection::RemoveLegacySubIDsByProductID(AZ::s64 productID)
    {
        if (!ValidateDatabaseTable("RemoveLegacySubIDsByProductID", "LegacySubIDs"))
        {
            AZ_Error(LOG_NAME, false, "Could not find LegacySubIDs table");
            return false;
        }

        ScopedTransaction transaction(m_databaseConnection);

        if (!s_DeleteLegacysubidsByProductidQuery.BindAndStep(*m_databaseConnection, productID))
        {
            return false;
        }

        transaction.Commit();

        return true;
    }

    // ProductDependencies
    bool AssetDatabaseConnection::GetProductDependencies(ProductDependencyDatabaseEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QueryProductDependenciesTable([&](AZ::Data::AssetId& assetId, ProductDependencyDatabaseEntry& entry)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(entry);
            return true; // return true to keep iterating over further rows.
        });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetProductDependencyByProductDependencyID(AZ::s64 productDependencyID, ProductDependencyDatabaseEntry& productDependencyEntry)
    {
        bool found = false;
        QueryProductDependencyByProductDependencyId(productDependencyID,
            [&](ProductDependencyDatabaseEntry& entry)
        {
            found = true;
            productDependencyEntry = AZStd::move(entry);
            return false; // stop after the first result
        });
        return found;
    }

    bool AssetDatabaseConnection::GetProductDependenciesByProductID(AZ::s64 productID, ProductDependencyDatabaseEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QueryProductDependencyByProductId(productID,
            [&](ProductDependencyDatabaseEntry& entry)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(entry);
            return true; // return true to keep iterating over further rows.
        });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetDirectProductDependencies(AZ::s64 productID, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QueryDirectProductDependencies(productID,
            [&](ProductDatabaseEntry& entry)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(entry);
            return true; // return true to keep iterating over further rows.
        });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetAllProductDependencies(AZ::s64 productID, AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QueryAllProductDependencies(productID,
            [&](ProductDatabaseEntry& entry)
        {
            found = true;
            container.push_back();
            container.back() = AZStd::move(entry);
            return true; // return true to keep iterating over further rows.
        });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::SetProductDependency(ProductDependencyDatabaseEntry& entry)
    {
        if (!ValidateDatabaseTable(s_InsertProductDependencyQuery.m_statementName, s_InsertProductDependencyQuery.m_tableName))
        {
            AZ_Error(LOG_NAME, false, "Could not find Products table");
            return false;
        }

        if (entry.m_productDependencyID == -1)
        {
            //they didn't set an id, add to database

            //make sure its not already in the database
            ProductDependencyDatabaseEntryContainer existingProductDependencies;
            if (GetProductDependenciesByProductID(entry.m_productPK, existingProductDependencies))
            {
                for (const auto& existingProductDependency : existingProductDependencies)
                {
                    if (existingProductDependency == entry)
                    {
                        //this product already exists
                        entry.m_productDependencyID = existingProductDependency.m_productDependencyID;
                        return true;
                    }
                }
            }

            if (!s_InsertProductDependencyQuery.BindAndStep(*m_databaseConnection, entry.m_productPK, entry.m_dependencySourceGuid, entry.m_dependencySubID, entry.m_dependencyFlags.to_ullong()))
            {
                return false;
            }

            //now read it from the database
            existingProductDependencies.clear();
            if (GetProductDependenciesByProductID(entry.m_productPK, existingProductDependencies))
            {
                for (const auto& existingProductDependency : existingProductDependencies)
                {
                    if (existingProductDependency == entry)
                    {
                        entry.m_productDependencyID = existingProductDependency.m_productDependencyID;
                        return true;
                    }
                }
            }

            return false;
        }
        else
        {
            //they supplied an id, see if it exists in the database
            ProductDependencyDatabaseEntry existingEntry;
            if (!GetProductDependencyByProductDependencyID(entry.m_productDependencyID, existingEntry))
            {
                AZ_Error(LOG_NAME, false, "Failed to write the product into the database.");
                return false;
            }

            //if the product is now different update it
            if (existingEntry == entry)
            {
                return true;
            }

            return s_UpdateProductDependencyQuery.BindAndStep(*m_databaseConnection, entry.m_productPK, entry.m_dependencySourceGuid, entry.m_dependencySubID, entry.m_dependencyFlags.to_ullong(), entry.m_productDependencyID);
        }
    }

    bool AssetDatabaseConnection::SetProductDependencies(const ProductDependencyDatabaseEntryContainer& container)
    {
        // first, collect all unique ProductPKs:
        ScopedTransaction transaction(m_databaseConnection);

        AZStd::unordered_set<AZ::s64> uniqueProductIds;
        for (const ProductDependencyDatabaseEntry& item : container)
        {
            uniqueProductIds.insert(item.m_productPK);
        }

        // unordered set eliminates dupes
        for (AZ::s64 productId : uniqueProductIds)
        {
            if (!RemoveProductDependencyByProductId(productId))
            {
                return false; // auto rollback will occur
            }
        }

        // now insert the new ones since we know there's no collisions:
                
        for (auto& entry : container)
        {

            if (!s_InsertProductDependencyQuery.BindAndStep(*m_databaseConnection, entry.m_productPK, entry.m_dependencySourceGuid, entry.m_dependencySubID, entry.m_dependencyFlags.to_ullong()))
            {
                return false;
            }
        }

        transaction.Commit();
        return true;
    }

    bool AssetDatabaseConnection::RemoveProductDependencyByProductId(AZ::s64 productID)
    {
        if (!ValidateDatabaseTable(DELETE_PRODUCT_DEPENDENCY_BY_PRODUCTID, "ProductDependencies"))
        {
            AZ_Error(LOG_NAME, false, "Could not find Product Dependency table");
            return false;
        }

        ScopedTransaction transaction(m_databaseConnection);

        if (!s_DeleteProductDependencyByProductIdQuery.BindAndStep(*m_databaseConnection, productID))
        {
            return false;
        }

        transaction.Commit();

        return true;
    }

    bool AssetDatabaseConnection::GetFileByFileID(AZ::s64 fileID, FileDatabaseEntry& entry)
    {
        bool found = false;
        bool succeeded = QueryFileByFileID(fileID,
                [&](FileDatabaseEntry& file)
                {
                    found = true;
                    entry = AZStd::move(file);
                    return false;//one
                });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetFileByFileNameAndScanFolderId(QString fileName, AZ::s64 scanFolderId, FileDatabaseEntry& entry)
    {
        bool found = false;
        bool succeeded = QueryFilesByFileNameAndScanFolderID(AssetUtilities::NormalizeFilePath(fileName).toUtf8().constData(), scanFolderId,
                [&](FileDatabaseEntry& file)
                {
                    found = true;
                    entry = AZStd::move(file);
                    return false;//one
                });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::GetFilesLikeFileName(QString likeFileName, LikeType likeType, FileDatabaseEntryContainer& container)
    {
        bool found = false;
        bool succeeded = QueryFilesLikeFileName(likeFileName.toUtf8().constData(), likeType,
            [&](FileDatabaseEntry& file)
        {
            found = true;
            container.push_back(file);
            return true;//all
        });
        return found && succeeded;
    }

    bool AssetDatabaseConnection::InsertFile(FileDatabaseEntry& entry)
    {
        //they didn't supply an id, add to database
        if (entry.m_fileID == -1)
        {
            if (!ValidateDatabaseTable(INSERT_FILE, "Files"))
            {
                AZ_Error(LOG_NAME, false, "Could not find Files table");
                return false;
            }

            //first make sure its not already in the database
            FileDatabaseEntry existingEntry;
            if (GetFileByFileNameAndScanFolderId(entry.m_fileName.c_str(), entry.m_scanFolderPK, existingEntry))
            {
                entry.m_fileID = existingEntry.m_fileID;
                return UpdateFile(entry); // now update the existing field
            }

            // it is a single statement, do not wrap it in a transaction, this wastes a lot of time.
            StatementAutoFinalizer autoFinal(*m_databaseConnection, INSERT_FILE);
            Statement* statement = autoFinal.Get();
            if (!statement)
            {
                AZ_Error(LOG_NAME, statement, "Could not get statement: %s", INSERT_FILE);
                return false;
            }

            int scanFolderPKIdx = statement->GetNamedParamIdx(":scanfolderpk");
            if (!scanFolderPKIdx)
            {
                AZ_Error(LOG_NAME, false, "could not find scanfolderpk in statement %s", INSERT_FILE);
                return false;
            }
            statement->BindValueInt64(scanFolderPKIdx, entry.m_scanFolderPK);

            int fileNameIdx = statement->GetNamedParamIdx(":filename");
            if (!fileNameIdx)
            {
                AZ_Error(LOG_NAME, false, "could not find filename in statement %s", INSERT_FILE);
                return false;
            }
            statement->BindValueText(fileNameIdx, entry.m_fileName.c_str());

            int isFolderIdx = statement->GetNamedParamIdx(":isfolder");
            if (!isFolderIdx)
            {
                AZ_Error(LOG_NAME, false, "could not find isfolder in statement %s", INSERT_FILE);
                return false;
            }
            statement->BindValueInt(isFolderIdx, entry.m_isFolder);
            
            if (statement->Step() == Statement::SqlError)
            {
                AZ_Warning(LOG_NAME, false, "Failed to write the new source into the database.");
                return false;
            }

            //now that its in the database get the id
            AZ::s64 rowID = m_databaseConnection->GetLastRowID();
            entry.m_fileID = rowID;

            return true;
        }

        return UpdateFile(entry);
    }

    bool AssetDatabaseConnection::UpdateFile(FileDatabaseEntry& entry) 
    {
        if (!ValidateDatabaseTable(UPDATE_FILE, "Files"))
        {
            AZ_Error(LOG_NAME, false, "Could not find Files table");
            return false;
        }

        //they supplied an id, see if it exists in the database
        FileDatabaseEntry existingEntry;
        if (!GetFileByFileID(entry.m_fileID, existingEntry))
        {
            //they supplied an id but is not in the database!
            AZ_Error(LOG_NAME, false, "Failed to write the file into the database.");
            return false;
        }

        // don't bother updating the database if all fields are equal.
        // note that we already looked it up by source ID
        if ((existingEntry.m_scanFolderPK == entry.m_scanFolderPK) &&
            (existingEntry.m_fileName == entry.m_fileName) &&
            (existingEntry.m_isFolder == entry.m_isFolder))
        {
            return false;
        }

        StatementAutoFinalizer autoFinal(*m_databaseConnection, UPDATE_FILE);
        Statement* statement = autoFinal.Get();
        if (!statement)
        {
            AZ_Error(LOG_NAME, statement, "Could not get statement: %s", UPDATE_FILE);
            return false;
        }

        int fileIdx = statement->GetNamedParamIdx(":fileid");
        if (!fileIdx)
        {
            AZ_Error(LOG_NAME, false, "could not find fileid in statement %s", UPDATE_FILE);
            return false;
        }
        statement->BindValueInt64(fileIdx, entry.m_fileID);

        int scanFolderPKIdx = statement->GetNamedParamIdx(":scanfolderpk");
        if (!scanFolderPKIdx)
        {
            AZ_Error(LOG_NAME, false, "could not find scanfolderpk in statement %s", UPDATE_FILE);
            return false;
        }
        statement->BindValueInt64(scanFolderPKIdx, entry.m_scanFolderPK);

        int fileNameIdx = statement->GetNamedParamIdx(":filename");
        if (!fileNameIdx)
        {
            AZ_Error(LOG_NAME, false, "could not find filename in statement %s", UPDATE_FILE);
            return false;
        }
        statement->BindValueText(fileNameIdx, entry.m_fileName.c_str());

        int isFolderIdx = statement->GetNamedParamIdx(":isfolder");
        if (!isFolderIdx)
        {
            AZ_Error(LOG_NAME, false, "could not find isfolder in statement %s", UPDATE_FILE);
            return false;
        }
        statement->BindValueInt(isFolderIdx, entry.m_isFolder);
        
        if (statement->Step() == Statement::SqlError)
        {
            AZ_Warning(LOG_NAME, false, "Failed to execute %s to update fingerprints (key %i)", UPDATE_FILE, entry.m_fileID);
            return false;
        }

        return true;
    }

    bool AssetDatabaseConnection::RemoveFile(AZ::s64 fileID)
    {
        if (!ValidateDatabaseTable(DELETE_FILE, "Files"))
        {
            AZ_Error(LOG_NAME, false, "Could not find Files table");
            return false;
        }

        ScopedTransaction transaction(m_databaseConnection);

        StatementAutoFinalizer autoFinal(*m_databaseConnection, DELETE_FILE);
        Statement* statement = autoFinal.Get();
        if (!statement)
        {
            AZ_Error(LOG_NAME, statement, "Could not get statement: %s", DELETE_FILE);
            return false;
        }

        int fileIDIdx = statement->GetNamedParamIdx(":fileid");
        if (!fileIDIdx)
        {
            AZ_Error(LOG_NAME, false, "could not find fileid in statement %s", DELETE_FILE);
            return false;
        }

        statement->BindValueInt64(fileIDIdx, fileID);

        if (statement->Step() == Statement::SqlError)
        {
            AZ_Warning(LOG_NAME, false, "Failed to RemoveFile from the database");
            return false;
        }

        transaction.Commit();

        return true;
    }
}    //namespace AssetProcessor

