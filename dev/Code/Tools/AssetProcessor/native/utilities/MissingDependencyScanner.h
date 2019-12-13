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

#include <AzCore/std/string/regex.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>

namespace AZ
{
    namespace IO
    {
        class GenericStream;
    }
}

namespace AssetProcessor
{
    class AssetDatabaseConnection;
    class LineByLineDependencyScanner;
    class MissingDependency;
    class PotentialDependencies;
    class SpecializedDependencyScanner;

    typedef AZStd::set<MissingDependency> MissingDependencies;

    class MissingDependencyScanner
    {
    public:
        enum class ScannerMatchType
        {
            // The scanner to run only matches based on the file extension, such as a "json" scanner will only
            // scan files with the .json extension.
            ExtensionOnlyFirstMatch,
            // The scanners open each file and inspect the contents to see if they look like the format.
            // The first scanner found that matches the file will be used.
            // Example: If a file named "Medium.difficulty" is in XML format, the XML scanner will catch this and scan it.
            FileContentsFirstMatch,
            // All scanners that can scan the given file are used to scan it. Time consuming but thorough.
            Deep
        };

        MissingDependencyScanner();

        //! Scans the file at the fullPath for anything that looks like a missing dependency.
        //! Reporting is handled internally, no results are returned.
        //! Anything that matches a result in the given dependency list will not be reported as a missing dependency.
        //! The databaseConnection is used to query the database to transform relative paths into source or
        //! product assets that match those paths, as well as looking up products for UUIDs found in files.
        //! The matchtype is used to determine how to scan the given file, see the ScannerMatchType enum for more information.
        //! A specific scanner can be forced to be used, this will supercede the match type.
        void ScanFile(
            const AZStd::string& fullPath,
            int maxScanIteration,
            AZ::s64 productPK,
            const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencies,
            AssetDatabaseConnection* databaseConnection,
            ScannerMatchType matchType= ScannerMatchType::ExtensionOnlyFirstMatch,
            AZ::Crc32* forceScanner=nullptr);

        static const int DefaultMaxScanIteration = 100;

        void RegisterSpecializedScanner(AZStd::shared_ptr<SpecializedDependencyScanner> scanner);

    protected:
        bool RunScan(
            const AZStd::string& fullPath,
            int maxScanIteration,
            AZ::IO::GenericStream& fileStream,
            PotentialDependencies& potentialDependencies,
            ScannerMatchType matchType,
            AZ::Crc32* forceScanner);

        void ScanStringForMissingDependencies(
            const AZStd::string& scanString,
            const AZStd::regex& subIdRegex,
            const AZStd::regex& uuidRegex,
            const AZStd::regex& pathRegex,
            PotentialDependencies& potentialDependencies);

        void PopulateMissingDependencies(
            AZ::s64 productPK,
            AssetDatabaseConnection* databaseConnection,
            const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencies,
            MissingDependencies& missingDependencies,
            const PotentialDependencies& potentialDependencies);

        void ReportMissingDependencies(
            AZ::s64 productPK,
            AssetDatabaseConnection* databaseConnection,
            const MissingDependencies& missingDependencies);

        typedef AZStd::unordered_map<AZ::Crc32, AZStd::shared_ptr<SpecializedDependencyScanner>> DependencyScannerMap;
        DependencyScannerMap m_specializedScanners;
        AZStd::shared_ptr<LineByLineDependencyScanner> m_defaultScanner;
    };
}
