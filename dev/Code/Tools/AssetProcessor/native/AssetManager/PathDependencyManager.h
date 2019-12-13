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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <native/AssetManager/assetProcessorManager.h>

class QString;

namespace AssetProcessor
{
    class PlatformConfiguration;
    class AssetDatabaseConnection;

    /// Handles resolving and saving product path dependencies
    class PathDependencyManager
    {
    public:
        PathDependencyManager(AssetProcessorManager* assetProcessorManager, PlatformConfiguration* platformConfig);

        /// Updates any unresolved wildcard dependencies that have been satisfied by the newly finished product
        void ProductFinished(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry, const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry);

        /// This function is responsible for looking up existing, unresolved dependencies that the current asset satisfies.
        /// These can be dependencies on either the source asset or one of the product assets
        void RetryDeferredDependencies(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry);

        /// This function is responsible for taking the path dependencies output by the current asset and trying to resolve them to AssetIds
        /// This does not look for dependencies that the current asset satisfies.
        void ResolveDependencies(AssetBuilderSDK::ProductPathDependencySet& pathDeps, AZStd::vector<AssetBuilderSDK::ProductDependency>& resolvedDeps, const AZStd::string& platform);

        /// Saves a product's unresolved dependencies to the database
        void SaveUnresolvedDependenciesToDatabase(AssetBuilderSDK::ProductPathDependencySet& unresolvedDependencies, const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry, const AZStd::string& platform);

        ///  Removes unresolved product dependencies from the product dependencies map
        void RemoveUnresolvedProductDependencies(const AZStd::vector<AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry>& unresolvedProductDependencies);

        // The two Ids needed for a ProductDependency entry, and platform. Used for saving ProductDependencies that are pending resolution
        struct DependencyProductIdInfo
        {
            AZ::s64 m_productId;
            AZ::s64 m_productDependencyId;
            AZStd::string m_platform;
        };
        using DependencyProductMap = AZStd::unordered_map<AZStd::string, AZStd::vector<DependencyProductIdInfo>>;
        // Avoid calling this directly, this is public to allow unit tests to access it.
        DependencyProductMap& GetDependencyProductMap(bool exactMatch, AssetBuilderSDK::ProductPathDependencyType dependencyType);
    private:

        /// Loads unresolved dependencies from the database
        void PopulateUnresolvedDependencies();

        /// Returns false if a path contains wildcards, true otherwise
        bool IsExactDependency(AZStd::string_view path);

        /// Removes /platform/project/ from the start of a product path
        AZStd::string StripPlatformAndProject(AZStd::string_view relativeProductPath);

        /// Prefixes the scanFolderId to the relativePath
        AZStd::string ToScanFolderPrefixedPath(int scanFolderId, const char* relativePath);

        /// Takes a path and breaks it into a database-prefixed relative path and scanFolder path
        /// This function can accept an absolute source path, an un-prefixed relative path, and a prefixed relative path
        /// The file returned will be the first one matched based on scanfolder priority
        bool ProcessInputPathToDatabasePathAndScanFolder(const char* dependencyPathSearch, QString& databaseName, QString& scanFolder);
        
        /// Updates the database with the now-resolved product dependencies
        /// dependencyPairs - the list of unresolved dependencies to update
        /// sourceEntry - database entry for the source file that resolved these dependencies
        /// products - database entry for the products the source file produced
        void UpdateResolvedDependencies(AZStd::vector<DependencyProductIdInfo>& dependencyPairs,
            const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry,
            const AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& products,
            bool removeSatisfiedDependencies = true);

        DependencyProductMap& GetDependencyProductMap(bool exactMatch, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry::DependencyType dependencyType);

        DependencyProductMap m_unresolvedSourcePathDependencyIds;
        DependencyProductMap m_unresolvedProductPathDependencyIds;
        DependencyProductMap m_unresolvedWildcardSourcePathDependencyIds;
        DependencyProductMap m_unresolvedWildcardProductPathDependencyIds;

        AssetProcessorManager* m_assetProcessorManager;
        AZStd::shared_ptr<AssetDatabaseConnection> m_stateData;
        PlatformConfiguration* m_platformConfig{};
    };

} // namespace AssetProcessor
