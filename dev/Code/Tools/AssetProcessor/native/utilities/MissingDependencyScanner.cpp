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

#include "MissingDependencyScanner.h"
#include "LineByLineDependencyScanner.h"
#include "PotentialDependencies.h"
#include "SpecializedDependencyScanner.h"
#include "native/AssetDatabase/AssetDatabase.h"
#include "native/assetprocessor.h"
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/FileTag/FileTagBus.h>
#include <AzFramework/FileTag/FileTag.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/SQLite/SQLiteBoundColumnSet.h>

namespace AssetProcessor
{
    class MissingDependency
    {
    public:
        MissingDependency(const AZ::Data::AssetId& assetId, const PotentialDependencyMetaData& metaData) :
            m_assetId(assetId),
            m_metaData(metaData)
        {

        }

        // Allows MissingDependency to be in a sorted container, which stabilizes log output.
        bool operator<(const MissingDependency& rhs) const
        {
            return m_assetId < rhs.m_assetId;
        }

        AZ::Data::AssetId m_assetId;
        PotentialDependencyMetaData m_metaData;
    };

    MissingDependencyScanner::MissingDependencyScanner()
    {
        m_defaultScanner = AZStd::make_shared<LineByLineDependencyScanner>();
    }

    void MissingDependencyScanner::ScanFile(
        const AZStd::string& fullPath,
        int maxScanIteration,
        AZ::s64 productPK,
        const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencies,
        AssetDatabaseConnection* databaseConnection,
        ScannerMatchType matchType,
        AZ::Crc32* forceScanner)
    {
        using namespace AzFramework::FileTag;
        AZ_Printf(AssetProcessor::ConsoleChannel, "Scanning for missing dependencies:\t%s\n", fullPath.c_str());

        AZStd::vector<AZStd::vector<AZStd::string>> blackListTagsList = {
        {
            FileTags[static_cast<unsigned int>(FileTagsIndex::EditorOnly)]
        },
        {
            FileTags[static_cast<unsigned int>(FileTagsIndex::Shader)]
        }};
        
        for (const AZStd::vector<AZStd::string>& tags : blackListTagsList)
        {
            bool shouldIgnore = false;
            QueryFileTagsEventBus::EventResult(shouldIgnore, FileTagType::BlackList,
                &QueryFileTagsEventBus::Events::Match, fullPath.c_str(), tags);
            if (shouldIgnore)
            {
                AZ_Printf(AssetProcessor::ConsoleChannel, "\tFile matches EditorOnly or Shader tag, ignoring for missing dependencies search.\n");
                return;
            }
        }

        AZ::IO::FileIOStream fileStream;
        if (!fileStream.Open(fullPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "File at path %s could not be opened.", fullPath.c_str());
            return;
        }

        PotentialDependencies potentialDependencies;
        bool scanSuccessful = RunScan(fullPath, maxScanIteration, fileStream, potentialDependencies, matchType, forceScanner);
        fileStream.Close();

        if (!scanSuccessful)
        {
            // RunScan will report an error on what caused the scan to fail.
            return;
        }

        MissingDependencies missingDependencies;
        PopulateMissingDependencies(productPK, databaseConnection, dependencies, missingDependencies, potentialDependencies);

        ReportMissingDependencies(productPK, databaseConnection, missingDependencies);
    }

    void MissingDependencyScanner::RegisterSpecializedScanner(AZStd::shared_ptr<SpecializedDependencyScanner> scanner)
    {
    m_specializedScanners.insert(AZStd::pair<AZ::Crc32, AZStd::shared_ptr<SpecializedDependencyScanner>>(scanner->GetScannerCRC(), scanner));
    }

    bool MissingDependencyScanner::RunScan(
        const AZStd::string& fullPath,
        int maxScanIteration,
        AZ::IO::GenericStream& fileStream,
        PotentialDependencies& potentialDependencies,
        ScannerMatchType matchType,
        AZ::Crc32* forceScanner)
    {
        // If a scanner is given to specifically use, then use that scanner and only that scanner.
        if (forceScanner)
        {
            AZ_Printf(
                AssetProcessor::ConsoleChannel,
                "\tForcing scanner with CRC %d\n",
                *forceScanner);
            DependencyScannerMap::iterator scannerToUse = m_specializedScanners.find(*forceScanner);
            if (scannerToUse != m_specializedScanners.end())
            {
                scannerToUse->second->ScanFileForPotentialDependencies(fileStream, potentialDependencies, maxScanIteration);
                return true;
            }
            else
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Attempted to force dependency scan using CRC %d, which is not registered.",
                    *forceScanner);
                return false;
            }
        }

        // Check if a specialized scanner should be used, based on the given scanner matching type rule.
        for (const AZStd::pair<AZ::Crc32, AZStd::shared_ptr<SpecializedDependencyScanner>>&
            scanner : m_specializedScanners)
        {
            switch (matchType)
            {
            case ScannerMatchType::ExtensionOnlyFirstMatch:
                if (scanner.second->DoesScannerMatchFileExtension(fullPath))
                {
                    return scanner.second->ScanFileForPotentialDependencies(fileStream, potentialDependencies, maxScanIteration);
                }
                break;
            case ScannerMatchType::FileContentsFirstMatch:
                if (scanner.second->DoesScannerMatchFileData(fileStream))
                {
                    return scanner.second->ScanFileForPotentialDependencies(fileStream, potentialDependencies, maxScanIteration);
                }
                break;
            case ScannerMatchType::Deep:
                // A deep scan has every matching scanner scan the file, and uses the default scan.
                if (scanner.second->DoesScannerMatchFileData(fileStream))
                {
                    scanner.second->ScanFileForPotentialDependencies(fileStream, potentialDependencies, maxScanIteration);
                }
                break;
            default:
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Scan match type %d is not available.", matchType);
                break;
            };
        }

        // No specialized scanner was found (or a deep scan is being performed), so use the default scanner.
        return m_defaultScanner->ScanFileForPotentialDependencies(fileStream, potentialDependencies, maxScanIteration);
    }

    void MissingDependencyScanner::PopulateMissingDependencies(
        AZ::s64 productPK,
        AssetDatabaseConnection* databaseConnection,
        const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencies,
        MissingDependencies& missingDependencies,
        const PotentialDependencies& potentialDependencies)
    {
        // If a file references itself, don't report it.
        AzToolsFramework::AssetDatabase::SourceDatabaseEntry fileWithPotentialMissingDependencies;
        databaseConnection->GetSourceByProductID(productPK, fileWithPotentialMissingDependencies);

        AZStd::map<AZ::Uuid, PotentialDependencyMetaData> uuids(potentialDependencies.m_uuids);
        AZStd::map<AZ::Data::AssetId, PotentialDependencyMetaData> assetIds(potentialDependencies.m_assetIds);

        // Check the existing product dependency list for the file that is being scanned, remove
        // any potential UUIDs that match dependencies already being emitted.
        for (const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry&
            existingDependency : dependencies)
        {
            AZStd::map<AZ::Uuid, PotentialDependencyMetaData>::iterator matchingDependency =
                uuids.find(existingDependency.m_dependencySourceGuid);
            if (matchingDependency != uuids.end())
            {
                uuids.erase(matchingDependency);
            }
        }

        // Remove all UUIDs that don't match an asset in the database.
        for (AZStd::map<AZ::Uuid, PotentialDependencyMetaData>::iterator uuidIter = uuids.begin();
            uuidIter != uuids.end();
            ++uuidIter)
        {
            if (fileWithPotentialMissingDependencies.m_sourceGuid == uuidIter->first)
            {
                // This product references itself, or the source it comes from. Don't report it as a missing dependency.
                continue;
            }

            AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
            if (!databaseConnection->GetSourceBySourceGuid(uuidIter->first, sourceEntry))
            {
                // The UUID isn't in the asset database, don't add it to the list of missing dependencies.
                continue;
            }

            AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer jobs;
            if (!databaseConnection->GetJobsBySourceID(sourceEntry.m_sourceID, jobs))
            {
                // No jobs existed for that source asset, so there are no products for this asset.
                // With no products, there is no way there can be a missing product dependency.
                continue;
            }

            // The dependency only referenced the source UUID, so add all products as missing dependencies.
            for (const AzToolsFramework::AssetDatabase::JobDatabaseEntry& job : jobs)
            {
                AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
                if (!databaseConnection->GetProductsByJobID(job.m_jobID, products))
                {
                    continue;
                }
                for (const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product : products)
                {
                    // This match was for a UUID with no product ID, so add all products as missing dependencies.
                    MissingDependency missingDependency(
                        AZ::Data::AssetId(uuidIter->first, product.m_subID),
                        uuidIter->second);
                    missingDependencies.insert(missingDependency);
                }
            }
        }

        // Validate the asset ID list, removing anything that is already a dependency, or does not exist in the asset database.
        for (AZStd::map<AZ::Data::AssetId, PotentialDependencyMetaData>::iterator assetIdIter = assetIds.begin();
            assetIdIter != assetIds.end();
            ++assetIdIter)
        {
            bool foundUUID = false;
            // Strip out all existing, matching dependencies
            for (const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry&
                existingDependency : dependencies)
            {
                if (existingDependency.m_dependencySourceGuid == assetIdIter->first.m_guid &&
                    existingDependency.m_dependencySubID == assetIdIter->first.m_subId)
                {
                    foundUUID = true;
                    break;
                }
            }

            // There is already a dependency with this UUID, so it's not a missing dependency.
            if (foundUUID)
            {
                continue;
            }

            AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
            if (!databaseConnection->GetSourceBySourceGuid(assetIdIter->first.m_guid, sourceEntry))
            {
                // The UUID isn't in the asset database. Don't report it as a missing dependency
                // because UUIDs are used for tracking many things that are not assets.
                continue;
            }

            AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer jobs;
            if (!databaseConnection->GetJobsBySourceID(sourceEntry.m_sourceID, jobs))
            {
                // No jobs existed for that source asset, so there are no products for this asset.
                // With no products, there is no way there can be a missing product dependency.
                continue;
            }

            // Check if any products exist for the given job, and those products have a sub ID that matches
            // the expected sub ID.
            bool foundMatchingProduct = false;
            for (const AzToolsFramework::AssetDatabase::JobDatabaseEntry& job : jobs)
            {
                AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
                if (!databaseConnection->GetProductsByJobID(job.m_jobID, products))
                {
                    continue;
                }
                for (const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product : products)
                {
                    if (product.m_subID == assetIdIter->first.m_subId)
                    {
                        // This product references itself. Don't report it as a missing dependency.
                        // If the product references a different product of the same source and that isn't
                        // a dependency, then do report that.
                        if (productPK == product.m_productID)
                        {
                            continue;
                        }

                        MissingDependency missingDependency(
                            assetIdIter->first,
                            assetIdIter->second);
                        missingDependencies.insert(missingDependency);
                        foundMatchingProduct = true;
                        break;
                    }
                }
                if (foundMatchingProduct)
                {
                    break;
                }
            }
        }

        for (const PotentialDependencyMetaData& path : potentialDependencies.m_paths)
        {
            AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer searchSources;
            QString searchName(path.m_sourceString.c_str());
            // The paths in the file may have had slashes in either direction, or double slashes.
            searchName.replace(AZ_WRONG_DATABASE_SEPARATOR_STRING, AZ_CORRECT_DATABASE_SEPARATOR_STRING);
            searchName.replace(AZ_DOUBLE_CORRECT_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR_STRING);
            
            if (databaseConnection->GetSourcesBySourceName(searchName, searchSources))
            {
                // A source matched the path, look up products and add them as resolved path dependencies.
                for (const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& source : searchSources)
                {
                    if (fileWithPotentialMissingDependencies.m_sourceGuid == source.m_sourceGuid)
                    {
                        // This product references itself, or the source it comes from. Don't report it as a missing dependency.
                        continue;
                    }

                    bool dependencyExistsForSource = false;
                    for (const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry&
                        existingDependency : dependencies)
                    {
                        if (existingDependency.m_dependencySourceGuid == source.m_sourceGuid)
                        {
                            dependencyExistsForSource = true;
                            break;
                        }
                    }

                    if (dependencyExistsForSource)
                    {
                        continue;
                    }

                    AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer jobs;
                    if (!databaseConnection->GetJobsBySourceID(source.m_sourceID, jobs))
                    {
                        // No jobs exist for this source, which means there is no matching product dependency.
                        continue;
                    }

                    for (const AzToolsFramework::AssetDatabase::JobDatabaseEntry& job : jobs)
                    {
                        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
                        if (!databaseConnection->GetProductsByJobID(job.m_jobID, products))
                        {
                            // No products, no product dependencies.
                            continue;
                        }

                        for (const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product : products)
                        {
                            MissingDependency missingDependency(
                                AZ::Data::AssetId(source.m_sourceGuid, product.m_subID),
                                path);
                            missingDependencies.insert(missingDependency);
                        }
                    }
                }
            }
            else
            {
                // Product paths in the asset database include the platform and additional pathing information that
                // makes this check more complex than the source path check.
                // Examples:
                //      pc/usersettings.xml
                //      pc/ProjectName/file.xml
                // Taking all results from this EndsWith check can lead to an over-emission of potential missing dependencies.
                // For example, if a file has a comment like "Something about .dds files", then EndsWith would return
                // every single dds file in the database.
                AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
                if (!databaseConnection->GetProductsLikeProductName(searchName, AssetDatabaseConnection::LikeType::EndsWith, products))
                {
                    continue;
                }
                for (const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product : products)
                {
                    if (productPK == product.m_productID)
                    {
                        // Don't report if a file has a reference to itself.
                        continue;
                    }
                    // Cull the platform from the product path to perform a more confident comparison against the given path.
                    QString culledProductPath = QString(product.m_productName.c_str());
                    culledProductPath = culledProductPath.remove(0,culledProductPath.indexOf(AZ_CORRECT_DATABASE_SEPARATOR_STRING)+1);

                    // This first check will catch paths that include the project name, as well as references to assets that include a scan folder in the path.
                    if (culledProductPath.compare(searchName, Qt::CaseInsensitive) != 0)
                    {
                        int nextFolderIndex = culledProductPath.indexOf(AZ_CORRECT_DATABASE_SEPARATOR_STRING);
                        if (nextFolderIndex == -1)
                        {
                            continue;
                        }
                        // Perform a second check with the scan folder removed. Many asset references are relevant to scan folder roots.
                        // For example, a material may have a relative path reference to a texture as "textures/SomeTexture.dds".
                        // This relative path resolves in many systems based on scan folder root, so if this file is in "platform/project/textures/SomeTexture.dds",
                        // this check is intended to find that reference.
                        culledProductPath = culledProductPath.remove(0, nextFolderIndex+1);
                        if (culledProductPath.compare(searchName, Qt::CaseInsensitive) != 0)
                        {
                            continue;
                        }
                    }

                    AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer productSources;
                    if (!databaseConnection->GetSourcesByProductName(QString(product.m_productName.c_str()), productSources))
                    {
                        AZ_Error(
                            AssetProcessor::ConsoleChannel,
                            false,
                            "Product %s does not have a matching source. Your database may be corrupted.", product.m_productName.c_str());
                        continue;
                    }

                    for (const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& source : productSources)
                    {
                        bool dependencyExistsForProduct = false;
                        for (const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry&
                            existingDependency : dependencies)
                        {
                            if (existingDependency.m_dependencySourceGuid == source.m_sourceGuid &&
                                existingDependency.m_dependencySubID == product.m_subID)
                            {
                                dependencyExistsForProduct = true;
                                break;
                            }
                        }
                        if (!dependencyExistsForProduct)
                        {
                            AZ::Data::AssetId assetId(source.m_sourceGuid, product.m_subID);
                            MissingDependency missingDependency(
                                AZ::Data::AssetId(source.m_sourceGuid, product.m_subID),
                                path);

                            missingDependencies.insert(missingDependency);
                        }
                    }
                }

            }
        }
    }

    void MissingDependencyScanner::ReportMissingDependencies(
        AZ::s64 productPK,
        AssetDatabaseConnection* databaseConnection,
        const MissingDependencies& missingDependencies)
    {
        using namespace AzFramework::FileTag;
        AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
        databaseConnection->GetSourceByProductID(productPK, sourceEntry);

        AZStd::vector<AZStd::string> tags{ 
            FileTags[static_cast<unsigned int>(FileTagsIndex::Ignore)],
            FileTags[static_cast<unsigned int>(FileTagsIndex::ProductDependency)] };

        for (const MissingDependency& missingDependency : missingDependencies)
        {
            bool shouldIgnore = false;
            QueryFileTagsEventBus::EventResult(shouldIgnore, FileTagType::BlackList,
                &QueryFileTagsEventBus::Events::Match, missingDependency.m_metaData.m_sourceString.c_str(), tags);
            if (!shouldIgnore)
            {
                AZStd::string assetIdStr = missingDependency.m_assetId.ToString<AZStd::string>();
                AZ_Printf(
                    AssetProcessor::ConsoleChannel,
                    "\t\tMissing dependency: String \"%s\" matches asset: %s\n",
                    missingDependency.m_metaData.m_sourceString.c_str(),
                    assetIdStr.c_str());

                AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry missingDependencyEntry(
                    productPK,
                    missingDependency.m_metaData.m_scanner->GetName(),
                    missingDependency.m_metaData.m_scanner->GetVersion(),
                    sourceEntry.m_analysisFingerprint,
                    missingDependency.m_assetId.m_guid,
                    missingDependency.m_assetId.m_subId,
                    missingDependency.m_metaData.m_sourceString);

                databaseConnection->SetMissingProductDependency(missingDependencyEntry);
            }
        }
    }
}
