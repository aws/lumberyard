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

#include "PathDependencyManager.h"
#include <native/AssetDatabase/AssetDatabase.h>
#include <assetprocessor.h>
#include <AzCore/std/string/wildcard.h>
#include <utilities/PlatformConfiguration.h>
#include <utilities/assetUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <IResourceCompilerHelper.h>

namespace AssetProcessor
{
    void SanitizeForDatabase(AZStd::string& str)
    {
        // Not calling normalize because wildcards should be preserved.
        AZStd::to_lower(str.begin(), str.end());
        AZStd::replace(str.begin(), str.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);
        AzFramework::StringFunc::Replace(str, AZ_DOUBLE_CORRECT_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR_STRING);
    }

    PathDependencyManager::PathDependencyManager(AssetProcessorManager* assetProcessorManager, PlatformConfiguration* platformConfig)
        : m_assetProcessorManager(assetProcessorManager), m_stateData(assetProcessorManager->GetDatabaseConnection()), m_platformConfig(platformConfig)
    {
        PopulateUnresolvedDependencies();
    }

    void PathDependencyManager::PopulateUnresolvedDependencies()
    {
        AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
        if (m_stateData->GetUnresolvedProductDependencies(dependencyContainer))
        {
            for (const auto& unresolvedDep : dependencyContainer)
            {
                DependencyProductIdInfo idPair;
                idPair.m_productDependencyId = unresolvedDep.m_productDependencyID;
                idPair.m_productId = unresolvedDep.m_productPK;
                idPair.m_platform = unresolvedDep.m_platform;
                AZStd::string path = unresolvedDep.m_unresolvedPath;
                AZStd::to_lower(path.begin(), path.end());
                bool isExactDependency = IsExactDependency(path);

                auto& dependencyMap = GetDependencyProductMap(isExactDependency, unresolvedDep.m_dependencyType);
                dependencyMap[path].push_back(AZStd::move(idPair));
            }
        }
    }

    void PathDependencyManager::SaveUnresolvedDependenciesToDatabase(AssetBuilderSDK::ProductPathDependencySet& unresolvedDependencies, const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry, const AZStd::string& platform)
    {
        using namespace AzToolsFramework::AssetDatabase;

        AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
        for (const auto& unresolvedPathDep : unresolvedDependencies)
        {
            auto dependencyType = unresolvedPathDep.m_dependencyType == AssetBuilderSDK::ProductPathDependencyType::SourceFile ?
                ProductDependencyDatabaseEntry::ProductDep_SourceFile :
                ProductDependencyDatabaseEntry::ProductDep_ProductFile;

            ProductDependencyDatabaseEntry placeholderDependency(
                productEntry.m_productID,
                AZ::Uuid::CreateNull(),
                0,
                AZStd::bitset<64>(),
                platform,
                // Use a string that will make it easy to route errors back here correctly. An empty string can be a symptom of many
                // other problems. This string says that something went wrong in this function.
                AZStd::string("INVALID_PATH"),
                dependencyType);
            
            AZStd::string path = AssetUtilities::NormalizeFilePath(unresolvedPathDep.m_dependencyPath.c_str()).toUtf8().constData();
            bool isExactDependency = IsExactDependency(path);

            if (isExactDependency && unresolvedPathDep.m_dependencyType == AssetBuilderSDK::ProductPathDependencyType::SourceFile)
            {
                QString relativePath, scanFolder;

                if (!AzFramework::StringFunc::Path::IsRelative(path.c_str()))
                {
                    if (m_platformConfig->ConvertToRelativePath(QString::fromUtf8(path.c_str()), relativePath, scanFolder, true))
                    {
                        auto* scanFolderInfo = m_platformConfig->GetScanFolderByPath(scanFolder);

                        path = ToScanFolderPrefixedPath(scanFolderInfo->ScanFolderID(), relativePath.toUtf8().constData());

                    }
                }
            }

            SanitizeForDatabase(path);
            auto& dependencyMap = GetDependencyProductMap(isExactDependency, unresolvedPathDep.m_dependencyType);

            // It's possible this job has been re-run for some reason, such as if the source file was modified while this
            // path was waiting to resolve. Check if the product dependency map already has this unmet path dependency, and remove it if so.
            DependencyProductMap::iterator existingDependency(dependencyMap.find(path));
            if (existingDependency != dependencyMap.end())
            {
                for(auto itr = existingDependency->second.begin(); itr != existingDependency->second.end(); ++itr)
                {
                    if (itr->m_productId == productEntry.m_productID)
                    {
                        AZ_TracePrintf(AssetProcessor::DebugChannel,
                            "Asset %s (product key %d) already has reported a missing path dependency on %s, removing old entry\n",
                            productEntry.m_productName.c_str(),
                            productEntry.m_productID,
                            path.c_str());

                        existingDependency->second.erase(itr);

                        break;
                    }
                }
            }

            placeholderDependency.m_unresolvedPath = path;
            dependencyContainer.push_back(placeholderDependency);
        }

        if(!m_stateData->UpdateProductDependencies(dependencyContainer))
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to save unresolved dependencies to database for product %d (%s)",
                productEntry.m_productID, productEntry.m_productName.c_str());
        }

        for(const auto& entry : dependencyContainer)
        {
            AZStd::string path = entry.m_unresolvedPath;
            bool isExactDependency = IsExactDependency(path);
            auto& dependencyMap = GetDependencyProductMap(isExactDependency, entry.m_dependencyType);

            // Record the productDependencyId, productId, and job platform so we can rewrite once it is resolved
            DependencyProductIdInfo dependencyIds;
            dependencyIds.m_productDependencyId = entry.m_productDependencyID;
            dependencyIds.m_productId = productEntry.m_productID;
            dependencyIds.m_platform = platform;
            dependencyMap[path].emplace_back(AZStd::move(dependencyIds));
        }
    }

    void PathDependencyManager::RemoveUnresolvedProductDependencies(const AZStd::vector<AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry>& unresolvedProductDependencies)
    {
        for (const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& productDependencyEntry : unresolvedProductDependencies)
        {
            if (productDependencyEntry.m_unresolvedPath.empty())
            {
                // this is not an unresolved dependency
                continue;
            }

            AZStd::string dependencyName(productDependencyEntry.m_unresolvedPath);
            SanitizeForDatabase(dependencyName);
            bool isExactDependency = IsExactDependency(dependencyName);

            auto& dependencyMap = GetDependencyProductMap(isExactDependency, productDependencyEntry.m_dependencyType);

            auto dependencyIter = dependencyMap.find(dependencyName);

            if (dependencyIter != dependencyMap.end())
            {
                for (auto iter = dependencyIter->second.begin(); iter != dependencyIter->second.end(); iter++)
                {
                    if (iter->m_productId == productDependencyEntry.m_productPK &&
                        iter->m_productDependencyId == productDependencyEntry.m_productDependencyID &&
                        iter->m_platform == productDependencyEntry.m_platform)
                    {
                        dependencyIter->second.erase(iter);
                        if (dependencyIter->second.empty())
                        {
                            dependencyMap.erase(dependencyIter);
                        }
                        break;
                    }
                }

            }
        }
    }

    bool PathDependencyManager::IsExactDependency(AZStd::string_view path)
    {
        return path.find('*') == AZStd::string_view::npos;
    }

    void PathDependencyManager::ProductFinished(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry, const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry)
    {
        for (auto& pair : m_unresolvedWildcardProductPathDependencyIds)
        {
            const AZStd::string& filter = pair.first;
            AZStd::string_view productName = productEntry.m_productName;

            // strip path of /<platform>/<project>/
            AZStd::string strippedPath = StripPlatformAndProject(productName);
            SanitizeForDatabase(strippedPath);

            if (AZStd::wildcard_match(filter, strippedPath))
            {
                UpdateResolvedDependencies(pair.second, sourceEntry, { productEntry }, false);
            }
        }

        for (auto& pair : m_unresolvedWildcardSourcePathDependencyIds)
        {
            const AZStd::string& filter = pair.first;
            AZStd::string sourceName(sourceEntry.m_sourceName);
            SanitizeForDatabase(sourceName);

            if (AZStd::wildcard_match(filter, sourceName))
            {
                UpdateResolvedDependencies(pair.second, sourceEntry, { productEntry }, false);
            }
        }
    }

    AZStd::string PathDependencyManager::StripPlatformAndProject(AZStd::string_view productName)
    {
        auto nextSlash = productName.find('/'); // platform/
        nextSlash = productName.find('/', nextSlash + 1) + 1; // project/
        return productName.substr(nextSlash, productName.size() - nextSlash);
    }

    PathDependencyManager::DependencyProductMap& PathDependencyManager::GetDependencyProductMap(bool exactMatch, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry::DependencyType dependencyType)
    {
        if (dependencyType == AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry::DependencyType::ProductDep_ProductFile)
        {
            if (exactMatch)
            {
                return m_unresolvedProductPathDependencyIds;
            }
            else
            {
                return m_unresolvedWildcardProductPathDependencyIds;
            }
        }
        else
        {
            if (exactMatch)
            {
                return m_unresolvedSourcePathDependencyIds;
            }
            else
            {
                return m_unresolvedWildcardSourcePathDependencyIds;
            }
        }
    }

    PathDependencyManager::DependencyProductMap& PathDependencyManager::GetDependencyProductMap(bool exactMatch, AssetBuilderSDK::ProductPathDependencyType dependencyType)
    {
        return GetDependencyProductMap(exactMatch, dependencyType == AssetBuilderSDK::ProductPathDependencyType::SourceFile
            ? AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry::DependencyType::ProductDep_SourceFile
            : AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry::DependencyType::ProductDep_ProductFile);
    }

    void PathDependencyManager::RetryDeferredDependencies(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
    {
        if (m_unresolvedSourcePathDependencyIds.empty() && m_unresolvedProductPathDependencyIds.empty())
        {
            // No unresolved dependencies, don't bother continuing
            return;
        }

        // Gather a list of all the products this source file produced
        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
        if (!m_stateData->GetProductsBySourceName(sourceEntry.m_sourceName.c_str(), products))
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Source %s did not have any products", sourceEntry.m_sourceName.c_str());
            return;
        }

        // Check for dependencies on any of the products
        if (!m_unresolvedProductPathDependencyIds.empty())
        {
            for (const auto& productEntry : products)
            {
                const AZStd::string& productName = productEntry.m_productName;

                // strip path of /<platform>/<project>/
                AZStd::string strippedPath = StripPlatformAndProject(productName);
                SanitizeForDatabase(strippedPath);

                auto unresolvedIter = m_unresolvedProductPathDependencyIds.find(strippedPath);

                if (unresolvedIter != m_unresolvedProductPathDependencyIds.end())
                {
                    UpdateResolvedDependencies(unresolvedIter->second, sourceEntry, { productEntry });
                    if (unresolvedIter->second.empty())
                    {
                        m_unresolvedProductPathDependencyIds.erase(unresolvedIter);
                    }
                }
            }
        }

        AZStd::string sourceNameWithScanFolder = ToScanFolderPrefixedPath(sourceEntry.m_scanFolderPK, sourceEntry.m_sourceName.c_str());

        // Check for dependencies on the source file
        SanitizeForDatabase(sourceNameWithScanFolder);
        auto unresolvedIter = m_unresolvedSourcePathDependencyIds.find(sourceNameWithScanFolder);

        if(unresolvedIter == m_unresolvedSourcePathDependencyIds.end())
        {
            AZStd::string santizedSourceName = sourceEntry.m_sourceName;
            SanitizeForDatabase(santizedSourceName);
            unresolvedIter = m_unresolvedSourcePathDependencyIds.find(santizedSourceName);
        }

        if (unresolvedIter != m_unresolvedSourcePathDependencyIds.end())
        {
            UpdateResolvedDependencies(unresolvedIter->second, sourceEntry, products);
            if (unresolvedIter->second.empty())
            {
                m_unresolvedSourcePathDependencyIds.erase(unresolvedIter);
            }
        }
    }

    void PathDependencyManager::UpdateResolvedDependencies(AZStd::vector<DependencyProductIdInfo>& dependencyInfoList,
        const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry,
        const AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& products,
        bool removeSatisfiedDependencies)
    {
        AZStd::vector<AZ::Data::AssetId> dependencyAssetIds;
        // Loop through all the assets that depend on this asset and update their dependencies
        auto dependencyInfo = dependencyInfoList.begin();
        while (dependencyInfo != dependencyInfoList.end())
        {
            AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;
            // If we're not removing the entry, we're not allowed to overwrite the existing one, it needs to stay in the database
            AZ::s64 dependencyId = removeSatisfiedDependencies ? dependencyInfo->m_productDependencyId : AzToolsFramework::AssetDatabase::InvalidEntryId;

            AZ_Assert(dependencyInfo->m_productDependencyId != AzToolsFramework::AssetDatabase::InvalidEntryId, "UpdateResolvedDependencies found an unresolved path dependency entry with an unset productDependencyId (%d)"
                "Recorded dependencies should always have a valid database ID.  Dependent on %s",
                dependencyInfo->m_productDependencyId, sourceEntry.m_sourceName.c_str());

            bool keepDependencyInfo = false;
            for (const auto& productEntry : products)
            {
                AzToolsFramework::AssetDatabase::JobDatabaseEntry jobEntry;
                if (!m_stateData->GetJobByJobID(productEntry.m_jobPK, jobEntry))
                {
                    AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to get job entry for product %s", productEntry.ToString().c_str());
                }

                if (jobEntry.m_platform != dependencyInfo->m_platform)
                {
                    keepDependencyInfo = true;
                    continue;
                }
                dependencyContainer.push_back();
                auto& entry = dependencyContainer.back();

                entry.m_productDependencyID = dependencyId;
                entry.m_productPK = dependencyInfo->m_productId;
                entry.m_dependencySourceGuid = sourceEntry.m_sourceGuid;
                entry.m_dependencySubID = productEntry.m_subID;
                entry.m_platform = dependencyInfo->m_platform;

                // There's only 1 database entry for this dependency, which we'll update with the first entry.
                // If this is a source dependency that resolves to multiple products, we'll need to add new entries for the rest of them (which happens when ID = AzToolsFramework::AssetDatabase::InvalidEntryId)
                dependencyId = AzToolsFramework::AssetDatabase::InvalidEntryId;
            }

            if (!m_stateData->UpdateProductDependencies(dependencyContainer))
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to update the placeholder product dependency");
            }

            for (const auto& dependency : dependencyContainer)
            {
                AzToolsFramework::AssetDatabase::ProductDatabaseEntry productEntry;
                if (!m_stateData->GetProductByProductID(dependency.m_productPK, productEntry))
                {
                    AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to get existing product with productId %i from the database", dependency.m_productPK);
                }

                AzToolsFramework::AssetDatabase::SourceDatabaseEntry dependentSource;
                if (!m_stateData->GetSourceByJobID(productEntry.m_jobPK, dependentSource))
                {
                    AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to get existing product from job ID of product %i from the database", dependency.m_productPK);
                }
                m_assetProcessorManager->EmitResolvedDependency(AZ::Data::AssetId(dependentSource.m_sourceGuid, productEntry.m_subID), dependency);
            }

            if (keepDependencyInfo || !removeSatisfiedDependencies)
            {
                dependencyInfo++;
            }
            else
            {
                dependencyInfo = dependencyInfoList.erase(dependencyInfo);
            }
        }
    }

    void CleanupPathDependency(AssetBuilderSDK::ProductPathDependency& pathDependency)
    {
        if(pathDependency.m_dependencyType == AssetBuilderSDK::ProductPathDependencyType::SourceFile)
        {
            // Nothing to cleanup if the dependency type was already pointing at source.
            return;
        }
        // Many workflows use source and product extensions for textures interchangeably, assuming that a later system will clean up the path.
        // Multiple systems use the AZ Serialization system to reference assets and collect these asset references. Not all of these systems
        // check if the references are to source or product asset types.
        // Instead requiring each of these systems to handle this (and failing in hard to track down ways later when they don't), check here, and clean things up.
        const AZStd::vector<AZStd::string> sourceImageExtensions = { ".tif", ".tiff", ".bmp", ".gif", ".jpg", ".jpeg", ".tga", ".png" };
        for (const AZStd::string& sourceImageExtension : sourceImageExtensions)
        {
            if (AzFramework::StringFunc::Path::IsExtension(pathDependency.m_dependencyPath.c_str(), sourceImageExtension.c_str()))
            {
                // This was a source format image reported initially as a product file dependency. Fix that to be a source file dependency.
                pathDependency.m_dependencyType = AssetBuilderSDK::ProductPathDependencyType::SourceFile;
                break;
            }
        }
    }

    void PathDependencyManager::ResolveDependencies(AssetBuilderSDK::ProductPathDependencySet& pathDeps, AZStd::vector<AssetBuilderSDK::ProductDependency>& resolvedDeps, const AZStd::string& platform)
    {
        constexpr int productDependencyFlags = 0;
        const QString gameName = AssetUtilities::ComputeGameName();

        auto pathIter = pathDeps.begin();
        while (pathIter != pathDeps.end())
        {
            AssetBuilderSDK::ProductPathDependency cleanedupDependency(*pathIter);
            CleanupPathDependency(cleanedupDependency);
            AZStd::string dependencyPathSearch = cleanedupDependency.m_dependencyPath;
            bool isExactDependency = !AzFramework::StringFunc::Replace(dependencyPathSearch, '*', '%');
            SanitizeForDatabase(dependencyPathSearch);

            if (cleanedupDependency.m_dependencyType == AssetBuilderSDK::ProductPathDependencyType::ProductFile)
            {
                AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer productInfoContainer;
                QString productNameWithPlatform = QString("%1%2%3").arg(platform.c_str(), AZ_CORRECT_DATABASE_SEPARATOR_STRING, dependencyPathSearch.c_str());
                QString productNameWithPlatformAndGameName = QString("%1%2%3%2%4").arg(platform.c_str(), AZ_CORRECT_DATABASE_SEPARATOR_STRING, gameName, dependencyPathSearch.c_str());

                if (isExactDependency)
                {
                    m_stateData->GetProductsByProductName(productNameWithPlatformAndGameName, productInfoContainer);
                    // Not all products will be in the game subfolder.
                    // Items in dev, like bootstrap.cfg, end up in just the root platform folder.
                    // These two checks search for products in both location.
                    // Example: If a path dependency was just "bootstrap.cfg" in SamplesProject on PC, this would search both
                    //  "cache/SamplesProject/pc/bootstrap.cfg" and "cache/SamplesProject/pc/SamplesProject/bootstrap.cfg".
                    m_stateData->GetProductsByProductName(productNameWithPlatform, productInfoContainer);

                }
                else
                {
                    m_stateData->GetProductsLikeProductName(productNameWithPlatformAndGameName, AzToolsFramework::AssetDatabase::AssetDatabaseConnection::LikeType::Raw, productInfoContainer);
                }

                // See if path matches any product files
                if (!productInfoContainer.empty())
                {
                    AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceDatabaseEntry;

                    for (const auto& productDatabaseEntry : productInfoContainer)
                    {
                        if (m_stateData->GetSourceByJobID(productDatabaseEntry.m_jobPK, sourceDatabaseEntry))
                        {
                            resolvedDeps.emplace_back(AZ::Data::AssetId(sourceDatabaseEntry.m_sourceGuid, productDatabaseEntry.m_subID), productDependencyFlags);
                        }
                        else
                        {
                            AZ_Error(AssetProcessor::ConsoleChannel, false, "Source for JobID %i not found (from product %s)", productDatabaseEntry.m_jobPK, dependencyPathSearch.c_str());
                        }

                        // For exact dependencies we expect that there is only 1 match.  Even if we processed more than 1, the results could be inconsistent since the other assets may not be finished processing yet
                        if (isExactDependency)
                        {
                            break;
                        }
                    }

                    // Wildcard dependencies never get removed since they can be fulfilled by a future product
                    if (isExactDependency)
                    {
                        pathIter = pathDeps.erase(pathIter);
                        continue;
                    }
                }
            }
            else
            {
                // See if path matches any source files
                AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer sourceInfoContainer;

                if (isExactDependency)
                {
                    QString databaseName;
                    QString scanFolder;

                    if (ProcessInputPathToDatabasePathAndScanFolder(dependencyPathSearch.c_str(), databaseName, scanFolder))
                    {
                        m_stateData->GetSourcesBySourceNameScanFolderId(databaseName, m_platformConfig->GetScanFolderByPath(scanFolder)->ScanFolderID(), sourceInfoContainer);
                    }
                }
                else
                {
                    m_stateData->GetSourcesLikeSourceName(dependencyPathSearch.c_str(), AzToolsFramework::AssetDatabase::AssetDatabaseConnection::LikeType::Raw, sourceInfoContainer);
                }

                if (!sourceInfoContainer.empty())
                {
                    bool productsAvailable = false;
                    for (const auto& sourceDatabaseEntry : sourceInfoContainer)
                    {
                        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer productInfoContainer;

                        if (m_stateData->GetProductsBySourceID(sourceDatabaseEntry.m_sourceID, productInfoContainer, AZ::Uuid::CreateNull(), "", platform.c_str()))
                        {
                            productsAvailable = true;
                            // Add a dependency on every product of this source file
                            for (const auto& productDatabaseEntry : productInfoContainer)
                            {
                                resolvedDeps.emplace_back(AZ::Data::AssetId(sourceDatabaseEntry.m_sourceGuid, productDatabaseEntry.m_subID), productDependencyFlags);
                            }
                        }

                        // For exact dependencies we expect that there is only 1 match.  Even if we processed more than 1, the results could be inconsistent since the other assets may not be finished processing yet
                        if (isExactDependency)
                        {
                            break;
                        }
                    }

                    if (isExactDependency && productsAvailable)
                    {
                        pathIter = pathDeps.erase(pathIter);
                        continue;
                    }
                }
            }
            pathIter->m_dependencyPath = cleanedupDependency.m_dependencyPath;
            pathIter->m_dependencyType = cleanedupDependency.m_dependencyType;
            pathIter++;
        }
    }

    bool PathDependencyManager::ProcessInputPathToDatabasePathAndScanFolder(const char* dependencyPathSearch, QString& databaseName, QString& scanFolder)
    {
        if (!AzFramework::StringFunc::Path::IsRelative(dependencyPathSearch))
        {
            // absolute paths just get converted directly
            return m_platformConfig->ConvertToRelativePath(QString::fromUtf8(dependencyPathSearch), databaseName, scanFolder);
        }
        else
        {
            // relative paths get the first matching asset, and then they get the usual call.
            QString absolutePath = m_platformConfig->FindFirstMatchingFile(QString::fromUtf8(dependencyPathSearch));
            if (!absolutePath.isEmpty())
            {
                return m_platformConfig->ConvertToRelativePath(absolutePath, databaseName, scanFolder);
            }
        }

        return false;
    }

    AZStd::string PathDependencyManager::ToScanFolderPrefixedPath(int scanFolderId, const char* relativePath)
    {
        static constexpr char ScanFolderSeparator = '$';

        return AZStd::string::format("%c%d%c%s", ScanFolderSeparator, scanFolderId, ScanFolderSeparator, relativePath);
    }

} // namespace AssetProcessor
