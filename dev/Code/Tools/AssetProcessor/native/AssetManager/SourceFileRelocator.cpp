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

#include "SourceFileRelocator.h"
#include "FileStateCache.h"
#include "utilities/assetUtils.h"
#include <AzCore/IO/SystemFile.h>
#include <QDir>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzToolsFramework/Debug/TraceContext.h>

namespace AssetProcessor
{
    SourceFileRelocator::SourceFileRelocator(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> stateData, PlatformConfiguration* platformConfiguration)
        : m_stateData(AZStd::move(stateData))
        , m_platformConfig(platformConfiguration)
    {
        AZ::Interface<ISourceFileRelocation>::Register(this);
    }

    SourceFileRelocator::~SourceFileRelocator()
    {
        AZ::Interface<ISourceFileRelocation>::Unregister(this);
    }

    AZ::Outcome<void, AZStd::string> SourceFileRelocator::GetScanFolderAndRelativePath(const AZStd::string& normalizedSource, bool allowNonexistentPath, const ScanFolderInfo*& scanFolderInfo, AZStd::string& relativePath) const
    {
        scanFolderInfo = nullptr;
        bool isRelative = AzFramework::StringFunc::Path::IsRelative(normalizedSource.c_str());
        
        if (isRelative)
        {
            // Relative paths can match multiple files/folders, search each scan folder for a valid match
            QString matchedPath;
            QString tempRelativeName(normalizedSource.c_str());

            for(int i = 0; i < m_platformConfig->GetScanFolderCount(); ++i)
            {
                const AssetProcessor::ScanFolderInfo* scanFolderInfoCheck = &m_platformConfig->GetScanFolderAt(i);

                if ((!scanFolderInfoCheck->RecurseSubFolders()) && (tempRelativeName.contains('/')))
                {
                    // the name is a deeper relative path, but we don't recurse this scan folder, so it can't win
                    continue;
                }
                QDir rooted(scanFolderInfoCheck->ScanPath());
                QString absolutePath = rooted.absoluteFilePath(tempRelativeName);
                bool fileExists = false;
                AssetProcessor::FileStateRequestBus::BroadcastResult(fileExists, &AssetProcessor::FileStateRequestBus::Events::Exists, absolutePath);

                if (fileExists)
                {
                    if (matchedPath.isEmpty())
                    {
                        matchedPath = AssetUtilities::NormalizeFilePath(absolutePath);
                        scanFolderInfo = scanFolderInfoCheck;
                    }
                    else
                    {
                        return AZ::Failure(AZStd::string::format("Relative path matched multiple files/folders.  Please narrow your query.\nMatch 1: %s\nMatch 2: %s\n",
                            matchedPath.toUtf8().constData(),
                            absolutePath.toUtf8().constData()));
                    }
                }
            }

            if(allowNonexistentPath && !scanFolderInfo)
            {
                // We didn't find a match, so assume the path refers to a folder in the highest priority scanfolder
                scanFolderInfo = &m_platformConfig->GetScanFolderAt(0);
            }
        }
        else
        {
            scanFolderInfo = m_platformConfig->GetScanFolderForFile(normalizedSource.c_str());
        }

        if (!scanFolderInfo)
        {
            return AZ::Failure(AZStd::string::format("Path %s does not exist within a scan folder.\n", normalizedSource.c_str()));
        }

        if (isRelative)
        {
            relativePath = normalizedSource;
        }
        else
        {
            QString relativePathQString;
            if (!PlatformConfiguration::ConvertToRelativePath(normalizedSource.c_str(), scanFolderInfo, relativePathQString, true))
            {
                return AZ::Failure(AZStd::string::format("Failed to convert path to relative path. %s\n", normalizedSource.c_str()));
            }

            relativePath = relativePathQString.toUtf8().constData();
        }

        return AZ::Success();
    }

    AZStd::string SourceFileRelocator::RemoveDatabasePrefix(const ScanFolderInfo* scanFolder, AZStd::string sourceName)
    {
        if (!scanFolder->GetOutputPrefix().isEmpty())
        {
            return sourceName.substr(scanFolder->GetOutputPrefix().size() + 1); //+1 to remove the slash after the prefix
        }

        return sourceName;
    }

    void SourceFileRelocator::GetSources(QStringList pathMatches, const ScanFolderInfo* scanFolderInfo,
        SourceFileRelocationContainer& sources) const
    {
        for (auto& file : pathMatches)
        {
            QString databaseSourceName;

            PlatformConfiguration::ConvertToRelativePath(file, scanFolderInfo, databaseSourceName);
            m_stateData->QuerySourceBySourceNameScanFolderID(databaseSourceName.toUtf8().constData(), scanFolderInfo->ScanFolderID(), [this, &sources, &scanFolderInfo](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                {
                    sources.emplace_back(entry, GetProductMapForSource(entry.m_sourceID), RemoveDatabasePrefix(scanFolderInfo, entry.m_sourceName), scanFolderInfo);
                    return true;
                });
        }
    }

    AZStd::unordered_map<int, AzToolsFramework::AssetDatabase::ProductDatabaseEntry> SourceFileRelocator::GetProductMapForSource(AZ::s64 sourceId) const
    {
        AZStd::unordered_map<int, AzToolsFramework::AssetDatabase::ProductDatabaseEntry> products;

        m_stateData->QueryProductBySourceID(sourceId, [&products](const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry)
            {
                products.emplace(entry.m_subID, entry);
                return true;
            });

        return products;
    }

    AZ::Outcome<void, AZStd::string> SourceFileRelocator::GetSourcesByPath(const AZStd::string& normalizedSource, SourceFileRelocationContainer& sources, const ScanFolderInfo*& scanFolderInfoOut) const
    {
        bool isWildcard = normalizedSource.find('*') != AZStd::string_view::npos || normalizedSource.find('?') != AZStd::string_view::npos;

        if (isWildcard)
        {
            bool isRelative = AzFramework::StringFunc::Path::IsRelative(normalizedSource.c_str());
            scanFolderInfoOut = nullptr;

            if (isRelative)
            {
                // For relative wildcard paths, we'll test the source path in each scan folder to see if we find any matches
                bool foundMatch = false;
                bool sourceContainsSlash = normalizedSource.find('/') != normalizedSource.npos;

                for (int i = 0; i < m_platformConfig->GetScanFolderCount(); ++i)
                {
                    const ScanFolderInfo* scanFolderInfo = &m_platformConfig->GetScanFolderAt(i);

                    if(!scanFolderInfo->RecurseSubFolders() && sourceContainsSlash)
                    {
                        continue;
                    }

                    auto pathMatches = m_platformConfig->FindWildcardMatches(scanFolderInfo->ScanPath(), normalizedSource.c_str(), false);

                    if (!pathMatches.isEmpty())
                    {
                        if (foundMatch)
                        {
                            return AZ::Failure(AZStd::string::format("Wildcard query %s matched files in multiple scanfolders.  Files can only be moved from one scanfolder at a time.  Please narrow your query.\nMatch 1: %s\nMatch 2: %s\n",
                                normalizedSource.c_str(),
                                scanFolderInfoOut->ScanPath().toUtf8().constData(),
                                scanFolderInfo->ScanPath().toUtf8().constData()));
                        }

                        foundMatch = true;
                        scanFolderInfoOut = scanFolderInfo;
                    }

                    GetSources(pathMatches, scanFolderInfo, sources);
                }
            }
            else
            {
                if (!normalizedSource.ends_with('*') && AZ::IO::FileIOBase::GetInstance()->IsDirectory(normalizedSource.c_str()))
                {
                    return AZ::Failure(AZStd::string("Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory.\n"));
                }

                // Absolute path: just look up the scanfolder and convert to a relative path
                AZStd::string pathOnly;
                QString databaseName;
                AzFramework::StringFunc::Path::GetFullPath(normalizedSource.c_str(), pathOnly);

                scanFolderInfoOut = m_platformConfig->GetScanFolderForFile(pathOnly.c_str());

                if (!scanFolderInfoOut)
                {
                    return AZ::Failure(AZStd::string::format("Path %s does not exist in a scanfolder\n", pathOnly.c_str()));
                }

                PlatformConfiguration::ConvertToRelativePath(normalizedSource.c_str(), scanFolderInfoOut, databaseName, false);

                auto pathMatches = m_platformConfig->FindWildcardMatches(scanFolderInfoOut->ScanPath(), databaseName, false);
                GetSources(pathMatches, scanFolderInfoOut, sources);
            }
        }
        else // Non-wildcard search
        {
            AZStd::string relativePath;
            auto result = GetScanFolderAndRelativePath(normalizedSource, false, scanFolderInfoOut, relativePath);

            if (!result.IsSuccess())
            {
                return result;
            }

            AZStd::string absoluteSourcePath;
            AzFramework::StringFunc::Path::Join(scanFolderInfoOut->ScanPath().toUtf8().constData(), relativePath.c_str(), absoluteSourcePath);

            if (AZ::IO::FileIOBase::GetInstance()->IsDirectory(absoluteSourcePath.c_str()))
            {
                return AZ::Failure(AZStd::string("Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory.\n"));
            }
            else
            {
                m_stateData->QuerySourceBySourceNameScanFolderID(relativePath.c_str(), scanFolderInfoOut->ScanFolderID(), [this, &sources, &scanFolderInfoOut, &relativePath](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry) -> bool
                    {
                        sources.emplace_back(entry, GetProductMapForSource(entry.m_sourceID), RemoveDatabasePrefix(scanFolderInfoOut, entry.m_sourceName), scanFolderInfoOut);
                        return true;
                    });
            }
        }

        return AZ::Success();
    }

    void SourceFileRelocator::PopulateDependencies(SourceFileRelocationContainer& relocationContainer) const
    {
        for (auto& relocationInfo : relocationContainer)
        {
            m_stateData->QuerySourceDependencyByDependsOnSource(relocationInfo.m_sourceEntry.m_sourceName.c_str(),
                nullptr,
                AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any, [&relocationInfo](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& dependencyEntry)
                {
                    relocationInfo.m_sourceDependencyEntries.push_back(dependencyEntry);
                    relocationInfo.m_hasPathDependencies |= !dependencyEntry.m_fromAssetId;
                    return true;
                });

            m_stateData->QueryProductDependenciesThatDependOnProductBySourceId(relocationInfo.m_sourceEntry.m_sourceID, [this, &relocationInfo](const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry)
                {
                    AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;

                    m_stateData->QuerySourceByProductID(entry.m_productPK, [&sourceEntry](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                        {
                            sourceEntry = entry;
                            return false;
                        });

                    // Don't count self-referencing product dependencies as there shouldn't be a case where a file has a hardcoded reference to itself.
                    if(sourceEntry.m_sourceID != relocationInfo.m_sourceEntry.m_sourceID)
                    {
                        relocationInfo.m_productDependencyEntries.push_back(entry);
                        relocationInfo.m_hasPathDependencies |= !entry.m_fromAssetId;
                    }

                    return true;
                });
        }
    }

    void SourceFileRelocator::MakePathRelative(const AZStd::string& parentPath, const AZStd::string& childPath, AZStd::string& parentRelative, AZStd::string& childRelative)
    {
        auto parentItr = parentPath.begin();
        auto childItr = childPath.begin();

        while (*parentItr == *childItr && parentItr != parentPath.end() && childItr != childPath.end())
        {
            ++parentItr;
            ++childItr;
        }

        parentRelative = AZStd::string(parentItr, parentPath.end());
        childRelative = AZStd::string(childItr, childPath.end());
    }

    AZ::Outcome<AZStd::string, AZStd::string> SourceFileRelocator::HandleWildcard(AZStd::string_view absFile, AZStd::string_view absSearch, AZStd::string destination)
    {
        AZStd::string searchAsRegex = absSearch;
        AZStd::regex specialCharacters(R"([.?^$+(){}[\]-])");

        // Escape the regex special characters
        searchAsRegex = AZStd::regex_replace(searchAsRegex, specialCharacters, R"(\$0)");
        // Replace * with .*
        searchAsRegex = AZStd::regex_replace(searchAsRegex, AZStd::regex(R"(\*)"), R"((.*))");

        AZStd::smatch result;

        // Match absSearch against absFile to find what each * expands to
        if(AZStd::regex_search(absFile.begin(), absFile.end(), result, AZStd::regex(searchAsRegex, AZStd::regex::icase)))
        {
            // For each * expansion, replace the * in the destination with the expanded result
            for (size_t i = 1; i < result.size(); ++i)
            {
                auto matchedString = result[i].str();

                // Only the last match can match across directory levels
                if(matchedString.find('/') != matchedString.npos && i < result.size() - 1)
                {
                    return AZ::Failure(AZStd::string("Wildcard cannot match across directory levels.  Please simplify your search or put a wildcard at the end of the search to match across directories.\n"));
                }

                destination.replace(destination.find('*'), 1, result[i].str().c_str());
            }
        }

        return AZ::Success(destination);
    }

    void SourceFileRelocator::FixDestinationMissingFilename(AZStd::string& destination, const AZStd::string& source) const
    {
        if (destination.ends_with(AZ_CORRECT_DATABASE_SEPARATOR))
        {
            size_t lastSlash = source.find_last_of(AZ_CORRECT_DATABASE_SEPARATOR);

            if (lastSlash == source.npos)
            {
                lastSlash = 0;
            }
            else
            {
                ++lastSlash; // Skip the slash itself
            }

            AZStd::string filename = source.substr(lastSlash);
            destination = destination + filename;
        }
    }

    AZ::Outcome<void, AZStd::string> SourceFileRelocator::ComputeDestination(SourceFileRelocationContainer& relocationContainer, const ScanFolderInfo* sourceScanFolder, const AZStd::string& source, AZStd::string destination, const ScanFolderInfo*& destinationScanFolderOut) const
    {
        if(destination.find("..") != AZStd::string::npos)
        {
            return AZ::Failure(AZStd::string("Destination cannot contain any path navigation.  Please specify an absolute or relative path that does not contain ..\n"));
        }

        FixDestinationMissingFilename(destination, source);

        if(destination.find_first_of("<|>?\"") != destination.npos)
        {
            return AZ::Failure(AZStd::string("Destination string contains invalid characters.\n"));
        }

        if (!AzFramework::StringFunc::Path::IsRelative(destination.c_str()))
        {
            destinationScanFolderOut = m_platformConfig->GetScanFolderForFile(destination.c_str());

            if (!destinationScanFolderOut)
            {
                return AZ::Failure(AZStd::string("Destination must exist within a scanfolder.\n"));
            }
        }

        AZ::s64 sourceWildcardCount = std::count(source.begin(), source.end(), '*');
        AZ::s64 destinationWildcardCount = std::count(destination.begin(), destination.end(), '*');

        if(sourceWildcardCount != destinationWildcardCount)
        {
            return AZ::Failure(AZStd::string("Source and destination paths must have the same number of wildcards.\n"));
        }

        AZStd::string lastError;

        for (SourceFileRelocationInfo& relocationInfo : relocationContainer)
        {
            AZStd::string oldAbsolutePath, selectionSourceAbsolutePath;
            AzFramework::StringFunc::Path::ConstructFull(sourceScanFolder->ScanPath().toUtf8().constData(), relocationInfo.m_oldRelativePath.c_str(), oldAbsolutePath, true);

            if (AzFramework::StringFunc::Path::IsRelative(source.c_str()))
            {
                AzFramework::StringFunc::Path::Join(sourceScanFolder->ScanPath().toUtf8().constData(), source.c_str(), selectionSourceAbsolutePath, false, true, false);
            }
            else
            {
                selectionSourceAbsolutePath = source;
            }

            AZStd::replace(selectionSourceAbsolutePath.begin(), selectionSourceAbsolutePath.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);
            AZStd::replace(oldAbsolutePath.begin(), oldAbsolutePath.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);

            auto result = HandleWildcard(oldAbsolutePath, selectionSourceAbsolutePath, destination);

            if(!result.IsSuccess())
            {
                lastError = result.TakeError();
                continue;
            }

            AZStd::string newDestinationPath = AssetUtilities::NormalizeFilePath(result.TakeValue().c_str()).toUtf8().constData();

            if (!AzFramework::StringFunc::Path::IsRelative(newDestinationPath.c_str()))
            {
                QString relativePath;
                QString scanFolderName;
                m_platformConfig->ConvertToRelativePath(newDestinationPath.c_str(), relativePath, scanFolderName, false);

                relocationInfo.m_newAbsolutePath = newDestinationPath;
                relocationInfo.m_newRelativePath = relativePath.toUtf8().constData();
            }
            else
            {
                if (!destinationScanFolderOut)
                {
                    AZStd::string relativePath;

                    auto outcome = GetScanFolderAndRelativePath(destination, true, destinationScanFolderOut, relativePath);

                    if (!outcome.IsSuccess())
                    {
                        return AZ::Failure(outcome.TakeError());
                    }
                }

                relocationInfo.m_newRelativePath = newDestinationPath;
                AzFramework::StringFunc::Path::ConstructFull(destinationScanFolderOut->ScanPath().toUtf8().constData(), relocationInfo.m_newRelativePath.c_str(), relocationInfo.m_newAbsolutePath, true);
            }

            relocationInfo.m_newRelativePath = AssetUtilities::NormalizeFilePath(relocationInfo.m_newRelativePath.c_str()).toUtf8().constData();

            relocationInfo.m_newUuid = AssetUtilities::CreateSafeSourceUUIDFromName(relocationInfo.m_newRelativePath.c_str());
        }

        if(!destinationScanFolderOut)
        {
            return AZ::Failure(lastError);
        }

        return AZ::Success();
    }

    AZStd::string BuildTaskFailureReport(const FileUpdateTasks& updateTasks)
    {
        AZStd::string report;

        for(const FileUpdateTask& task : updateTasks)
        {
            if(!task.m_succeeded)
            {
                if(report.empty())
                {
                    report.append("UPDATE FAILURE REPORT:\nThe following files have a dependency on file(s) that were moved which could not be updated automatically:\n");
                }

                report.append(AZStd::string::format("\t%s\n\t\t%s -> %s\n", task.m_absPathFileToUpdate.c_str(), task.m_oldStrings[0].c_str(), task.m_newStrings[0].c_str()));
            }
        }

        return report;
    }

    AZStd::string SourceFileRelocator::BuildReport(const SourceFileRelocationContainer& relocationEntries, const FileUpdateTasks& updateTasks, bool isMove) const
    {
        AZStd::string report;

        report.append("FILE REPORT:\n");

        for (const SourceFileRelocationInfo& relocationInfo : relocationEntries)
        {
            if (isMove)
            {
                report.append(AZStd::string::format(
                    "SOURCEID: %d, CURRENT PATH: %s, NEW PATH: %s, CURRENT GUID: %s, NEW GUID: %s\n",
                    relocationInfo.m_sourceEntry.m_sourceID,
                    relocationInfo.m_oldRelativePath.c_str(),
                    relocationInfo.m_newRelativePath.c_str(),
                    relocationInfo.m_sourceEntry.m_sourceGuid.ToString<AZStd::string>().c_str(),
                    relocationInfo.m_newUuid.ToString<AZStd::string>().c_str()));
            }
            else
            {
                report.append(AZStd::string::format(
                    "SOURCEID: %d, CURRENT PATH: %s, CURRENT GUID: %s\n",
                    relocationInfo.m_sourceEntry.m_sourceID,
                    relocationInfo.m_oldRelativePath.c_str(),
                    relocationInfo.m_sourceEntry.m_sourceGuid.ToString<AZStd::string>().c_str()));
            }

            if (!relocationInfo.m_sourceDependencyEntries.empty())
            {
                report.append("\tThe following files have a source/job dependency on this file and will break:\n");

                for (const auto& sourceDependency : relocationInfo.m_sourceDependencyEntries)
                {
                    report = AZStd::string::format("%s\t\tPATH: %s, TYPE: %d, %s\n", report.c_str(), sourceDependency.m_dependsOnSource.c_str(), sourceDependency.m_typeOfDependency, sourceDependency.m_fromAssetId ? "AssetId-based" : "Path-based");
                }
            }

            if (!relocationInfo.m_productDependencyEntries.empty())
            {
                report.append("\tThe following files have a product dependency on one or more of the products generated by this file and will break:\n");

                for (const auto& productDependency : relocationInfo.m_productDependencyEntries)
                {
                    AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
                    AzToolsFramework::AssetDatabase::ProductDatabaseEntry productEntry;
                    AzToolsFramework::AssetDatabase::ProductDatabaseEntry thisFilesProductEntry;

                    m_stateData->QuerySourceByProductID(productDependency.m_productPK, [&sourceEntry](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                        {
                            sourceEntry = entry;
                            return false;
                        });

                    m_stateData->QueryProductByProductID(productDependency.m_productPK, [&productEntry](const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry)
                        {
                            productEntry = entry;
                            return false;
                        });

                    m_stateData->QueryCombinedBySourceGuidProductSubId(productDependency.m_dependencySourceGuid, productDependency.m_dependencySubID, [&thisFilesProductEntry, &productDependency](const AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& entry)
                        {
                            thisFilesProductEntry = static_cast<AzToolsFramework::AssetDatabase::ProductDatabaseEntry>(entry);
                            return false;
                        }, AZ::Uuid::CreateNull(), nullptr, productDependency.m_platform.c_str());

                    report = AZStd::string::format("%s\t\tPATH: %s, DEPENDS ON PRODUCT: %s, ASSETID: %s, TYPE: %d, %s\n", report.c_str(), productEntry.m_productName.c_str(), thisFilesProductEntry.m_productName.c_str(), AZ::Data::AssetId(sourceEntry.m_sourceGuid, productEntry.m_subID).ToString<AZStd::string>().c_str(), productDependency.m_dependencyType, productDependency.m_fromAssetId ? "AssetId-based" : "Path-based");
                }
            }
        }

        report.append(BuildTaskFailureReport(updateTasks));

        return report;
    }

    AZ::Outcome<RelocationSuccess, MoveFailure> SourceFileRelocator::Move(const AZStd::string& source, const AZStd::string& destination, bool previewOnly, bool allowDependencyBreaking, bool removeEmptyFolders, bool updateReferences)
    {
        AZStd::string normalizedSource = source;
        AZStd::string normalizedDestination = destination;

        // Just make sure we have uniform slashes, don't normalize because we need to keep slashes at the end of the path and wildcards, etc, which tend to get stripped out by normalize functions
        AZStd::replace(normalizedSource.begin(), normalizedSource.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);
        AZStd::replace(normalizedDestination.begin(), normalizedDestination.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);

        SourceFileRelocationContainer relocationContainer;
        const ScanFolderInfo* sourceScanFolderInfo{};
        const ScanFolderInfo* destinationScanFolderInfo{};

        auto result = GetSourcesByPath(normalizedSource, relocationContainer, sourceScanFolderInfo);

        if (!result.IsSuccess())
        {
            return AZ::Failure(MoveFailure(result.TakeError(), false));
        }

        // If no files were found, just early out
        if(relocationContainer.empty())
        {
            return AZ::Success(RelocationSuccess());
        }

        PopulateDependencies(relocationContainer);
        result = ComputeDestination(relocationContainer, sourceScanFolderInfo, normalizedSource, normalizedDestination, destinationScanFolderInfo);

        if(!result.IsSuccess())
        {
            return AZ::Failure(MoveFailure(result.TakeError(), false));
        }

        int errorCount = 0;
        FileUpdateTasks updateTasks;

        if(!previewOnly)
        {
            if(!updateReferences && !allowDependencyBreaking)
            {
                for (const SourceFileRelocationInfo& relocationInfo : relocationContainer)
                {
                    if(!relocationInfo.m_productDependencyEntries.empty() || !relocationInfo.m_sourceDependencyEntries.empty())
                    {
                        return AZ::Failure(MoveFailure(AZStd::string("Move failed.  There are files that have dependencies that may break as a result of being moved/renamed.\n"), true));
                    }
                }
            }

            bool sourceControlEnabled = false;
            AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(sourceControlEnabled, &AzToolsFramework::SourceControlConnectionRequestBus::Events::IsActive);

            if (sourceControlEnabled)
            {
                errorCount = DoSourceControlMoveFiles(normalizedSource, normalizedDestination, relocationContainer, sourceScanFolderInfo, destinationScanFolderInfo, removeEmptyFolders);
            }
            else
            {
                errorCount = DoMoveFiles(relocationContainer, removeEmptyFolders);
            }

            if (updateReferences)
            {
                updateTasks = UpdateReferences(relocationContainer, sourceControlEnabled);
            }
        }

        int relocationCount = aznumeric_caster(relocationContainer.size());
        int updateTotalCount = aznumeric_caster(updateTasks.size());
        int updateSuccessCount = 0;

        for (const auto& task : updateTasks)
        {
            if (task.m_succeeded)
            {
                ++updateSuccessCount;
            }
        }

        int updateFailureCount = updateTotalCount - updateSuccessCount;

        return AZ::Success(RelocationSuccess(
            relocationCount - errorCount, errorCount, relocationCount,
            updateSuccessCount, updateFailureCount, updateTotalCount,
            AZStd::move(relocationContainer),
            AZStd::move(updateTasks)));
    }

    AZ::Outcome<RelocationSuccess, AZStd::string> SourceFileRelocator::Delete(const AZStd::string& source, bool previewOnly, bool allowDependencyBreaking, bool removeEmptyFolders)
    {
        AZStd::string normalizedSource = AssetUtilities::NormalizeFilePath(source.c_str()).toUtf8().constData();

        SourceFileRelocationContainer relocationContainer;
        const ScanFolderInfo* scanFolderInfo{};

        auto result = GetSourcesByPath(normalizedSource, relocationContainer, scanFolderInfo);

        if (!result.IsSuccess())
        {
            return AZ::Failure(result.TakeError());
        }

        // If no files were found, just early out
        if (relocationContainer.empty())
        {
            return AZ::Success(RelocationSuccess());
        }

        PopulateDependencies(relocationContainer);

        int errorCount = 0;

        if (!previewOnly)
        {
            if (!allowDependencyBreaking)
            {
                for (const SourceFileRelocationInfo& relocationInfo : relocationContainer)
                {
                    if (!relocationInfo.m_productDependencyEntries.empty() || !relocationInfo.m_sourceDependencyEntries.empty())
                    {
                        return AZ::Failure(AZStd::string("Delete failed.  There are files that have dependencies that may break as a result of being deleted.\n"));
                    }
                }
            }

            bool sourceControlEnabled = false;
            AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(sourceControlEnabled, &AzToolsFramework::SourceControlConnectionRequestBus::Events::IsActive);

            if (sourceControlEnabled)
            {
                errorCount = DoSourceControlDeleteFiles(normalizedSource, relocationContainer, scanFolderInfo, removeEmptyFolders);
            }
            else
            {
                errorCount = DoDeleteFiles(relocationContainer, removeEmptyFolders);
            }
        }

        int relocationCount = aznumeric_caster(relocationContainer.size());
        return AZ::Success(RelocationSuccess(
            relocationCount - errorCount, errorCount, relocationCount,
            0, 0, 0,
            AZStd::move(relocationContainer),
            {}));
    }

    void RemoveEmptyFolders(const SourceFileRelocationContainer& relocationContainer)
    {
        for (const SourceFileRelocationInfo& info : relocationContainer)
        {
            AZStd::string oldParentFolder;
            AzFramework::StringFunc::Path::GetFullPath(info.m_oldAbsolutePath.c_str(), oldParentFolder);

            // Not checking the return value since non-empty folders will fail, we just want to delete empty folders.
            AZ::IO::SystemFile::DeleteDir(oldParentFolder.c_str());
        }
    }

    int SourceFileRelocator::DoMoveFiles(SourceFileRelocationContainer& relocationContainer, bool removeEmptyFolders)
    {
        int errorCount = 0;

        for (SourceFileRelocationInfo& relocationInfo : relocationContainer)
        {
            const char* oldPath = relocationInfo.m_oldAbsolutePath.c_str();
            const char* newPath = relocationInfo.m_newAbsolutePath.c_str();

            AZ_Error("SourceFileRelocator", !relocationInfo.m_oldAbsolutePath.empty(), "Current absolute path of file is missing: %s", oldPath);
            AZ_Error("SourceFileRelocator", !relocationInfo.m_newAbsolutePath.empty(), "Destination absolute path of file is missing: %s", oldPath);

            bool oldExists = AZ::IO::LocalFileIO::GetInstance()->Exists(oldPath);
            bool newExists = AZ::IO::LocalFileIO::GetInstance()->Exists(newPath);
            bool isReadOnly = AZ::IO::LocalFileIO::GetInstance()->IsReadOnly(oldPath);

            if (!oldExists)
            {
                AZ_Error("SourceFileRelocator", false, "File %s does not exist, cannot move non-existant file.", oldPath);
                errorCount++;
                continue;
            }

            if (newExists)
            {
                AZ_Error("SourceFileRelocator", false, "Destination file %s already exists, cannot overwrite existing file.", newPath);
                errorCount++;
                continue;
            }

            if(isReadOnly)
            {
                AZ_Error("SourceFileRelocator", false, "File %s is marked readonly and will not be moved.", oldPath);
                errorCount++;
                continue;
            }

            AZStd::string newParentFolder;
            AzFramework::StringFunc::Path::GetFullPath(newPath, newParentFolder);

            if (!AZ::IO::SystemFile::CreateDir(newParentFolder.c_str()))
            {
                AZ_Error("SourceFileRelocator", false, "Failed to create directory tree");
            }

            if (!AZ::IO::SystemFile::Rename(oldPath, newPath))
            {
                AZ_Error("SourceFileRelocator", false, "Failed to move/rename file from %s to %s", oldPath, newPath);
                errorCount++;
            }

            relocationInfo.m_operationSucceeded = true;
        }

        if (removeEmptyFolders)
        {
            RemoveEmptyFolders(relocationContainer);
        }

        return errorCount;
    }

    int SourceFileRelocator::DoDeleteFiles(SourceFileRelocationContainer& relocationContainer, bool removeEmptyFolders)
    {
        int errorCount = 0;

        for (SourceFileRelocationInfo& relocationInfo : relocationContainer)
        {
            const char* oldPath = relocationInfo.m_oldAbsolutePath.c_str();

            AZ_Error("SourceFileRelocator", !relocationInfo.m_oldAbsolutePath.empty(), "Current absolute path of file is missing: %s", oldPath);

            bool oldExists = AZ::IO::LocalFileIO::GetInstance()->Exists(oldPath);

            if (!oldExists)
            {
                AZ_Error("SourceFileRelocator", false, "File %s does not exist, cannot move non-existant file", oldPath);
                errorCount++;
                continue;
            }

            if (!AZ::IO::SystemFile::Delete(oldPath))
            {
                AZ_Error("SourceFileRelocator", false, "Failed to delete file %s", oldPath);
                errorCount++;
            }

            relocationInfo.m_operationSucceeded = true;
        }

        if (removeEmptyFolders)
        {
            RemoveEmptyFolders(relocationContainer);
        }

        return errorCount;
    }

    void HandleSourceControlResult(SourceFileRelocationContainer& relocationContainer, AZStd::binary_semaphore& waitSignal, int& errorCount,
        AzToolsFramework::SourceControlFlags checkFlag, bool checkNewPath, bool /*success*/, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
    {
        for (SourceFileRelocationInfo& entry : relocationContainer)
        {
            bool found = false;

            for (const AzToolsFramework::SourceControlFileInfo& scInfo : info)
            {
                const char* checkPath = checkNewPath ? entry.m_newAbsolutePath.c_str() : entry.m_oldAbsolutePath.c_str();

                if (AssetUtilities::NormalizeFilePath(scInfo.m_filePath.c_str()) == AssetUtilities::NormalizeFilePath(checkPath))
                {
                    found = true;
                    entry.m_operationSucceeded = scInfo.HasFlag(checkFlag);
                    break;
                }
            }

            if(!entry.m_operationSucceeded)
            {
                ++errorCount;

                if (!found)
                {
                    AZ_Printf("SourceFileRelocator", "Error: file is not tracked by source control %s\n", entry.m_oldAbsolutePath.c_str());
                }
                else
                {
                    AZ_Printf("SourceFileRelocator", "Error: operation failed for file %s\n", entry.m_oldAbsolutePath.c_str());
                }
            }
        }

        waitSignal.release();
    }

    void AdjustWildcardForPerforce(AZStd::string& path)
    {
        if (path.ends_with('*'))
        {
            path.replace(path.length() - 1, 1, "...");
        }
    }

    AZStd::string ToAbsolutePath(AZStd::string& normalizedPath, const ScanFolderInfo* scanFolderInfo)
    {
        AZStd::string absolutePath;

        if (AzFramework::StringFunc::Path::IsRelative(normalizedPath.c_str()))
        {
            AzFramework::StringFunc::Path::ConstructFull(scanFolderInfo->ScanPath().toUtf8().constData(), normalizedPath.c_str(), absolutePath, false);
        }
        else
        {
            absolutePath = normalizedPath;
        }

        return absolutePath;
    }

    int SourceFileRelocator::DoSourceControlMoveFiles(AZStd::string normalizedSource, AZStd::string normalizedDestination, SourceFileRelocationContainer& relocationContainer, const ScanFolderInfo* sourceScanFolderInfo, const ScanFolderInfo* destinationScanFolderInfo, bool removeEmptyFolders) const
    {
        using namespace AzToolsFramework;
        AZ_Printf("SourceFileRelocator", "Using source control to move files\n");

        AZ_Assert(sourceScanFolderInfo, "sourceScanFolderInfo cannot be null");
        AZ_Assert(destinationScanFolderInfo, "destinationScanFolderInfo cannot be null");

        for(const auto& relocationInfo : relocationContainer)
        {
            const char* newPath = relocationInfo.m_newAbsolutePath.c_str();

            bool newExists = AZ::IO::LocalFileIO::GetInstance()->Exists(newPath);

            if (newExists)
            {
                AZ_Printf("SourceFileRelocator", "Destination file %s already exists, cannot overwrite existing file\n", newPath);
            }
        }

        FixDestinationMissingFilename(normalizedDestination, normalizedSource);
        AdjustWildcardForPerforce(normalizedSource);
        AdjustWildcardForPerforce(normalizedDestination);

        AZStd::string absoluteSource = ToAbsolutePath(normalizedSource, sourceScanFolderInfo);
        AZStd::string absoluteDestination = ToAbsolutePath(normalizedDestination, destinationScanFolderInfo);

        AZ_Printf("SourceFileRelocator", "From: %s, To: %s\n", absoluteSource.c_str(), absoluteDestination.c_str());

        AZStd::binary_semaphore waitSignal;
        int errorCount = 0;

        AzToolsFramework::SourceControlResponseCallbackBulk callback = AZStd::bind(&HandleSourceControlResult,
            AZStd::ref (relocationContainer),
            AZStd::ref(waitSignal),
            AZStd::ref(errorCount),
            static_cast<AzToolsFramework::SourceControlFlags>(SCF_OpenByUser), // If a file is moved from A -> B and then again from B -> A, the result is just an "edit", so we're just going to assume success if the file is checked out, regardless of state
            true,
            AZStd::placeholders::_1,
            AZStd::placeholders::_2);

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestRenameBulk,
            absoluteSource.c_str(), absoluteDestination.c_str(), callback);

        while (!waitSignal.try_acquire_for(AZStd::chrono::milliseconds(200)))
        {
            AZ::TickBus::ExecuteQueuedEvents();
        }

        if (removeEmptyFolders)
        {
            RemoveEmptyFolders(relocationContainer);
        }

        return errorCount;
    }

    int SourceFileRelocator::DoSourceControlDeleteFiles(AZStd::string normalizedSource, SourceFileRelocationContainer& relocationContainer, const ScanFolderInfo* sourceScanFolderInfo, bool removeEmptyFolders) const
    {
        using namespace AzToolsFramework;
        AZ_Printf("SourceFileRelocator", "Using source control to delete files\n");

        AdjustWildcardForPerforce(normalizedSource);
        AZStd::string absoluteSource = ToAbsolutePath(normalizedSource, sourceScanFolderInfo);

        AZ_Printf("SourceFileRelocator", "Delete %s\n", absoluteSource.c_str());

        AZStd::binary_semaphore waitSignal;
        int errorCount = 0;

        AzToolsFramework::SourceControlResponseCallbackBulk callback = AZStd::bind(&HandleSourceControlResult, relocationContainer, AZStd::ref(waitSignal), AZStd::ref(errorCount),
            SCF_PendingDelete, false, AZStd::placeholders::_1, AZStd::placeholders::_2);

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestDeleteBulk,
            absoluteSource.c_str(), callback);

        while (!waitSignal.try_acquire_for(AZStd::chrono::milliseconds(200)))
        {
            AZ::TickBus::ExecuteQueuedEvents();
        }

        if (removeEmptyFolders)
        {
            RemoveEmptyFolders(relocationContainer);
        }

        return errorCount;
    }

    AZStd::string FileToString(const char* fullPath)
    {
        AZ::IO::FileIOStream fileStream(fullPath, AZ::IO::OpenMode::ModeRead);

        if (!fileStream.IsOpen())
        {
            AZ_Error("SourceFileRelocator", false, "Failed to open file for read %s", fullPath);
            return {};
        }

        AZ::IO::SizeType length = fileStream.GetLength();

        if (length == 0)
        {
            return {};
        }

        AZStd::vector<char> charBuffer;
        charBuffer.resize_no_construct(length);
        fileStream.Read(length, charBuffer.data());

        return AZStd::string(charBuffer.data(), charBuffer.data() + length);
    }

    void StringToFile(const char* fullPath, const AZStd::string& str)
    {
        AZ::IO::FileIOStream fileStream(fullPath, AZ::IO::OpenMode::ModeWrite);

        if(!fileStream.IsOpen())
        {
            AZ_Error("SourceFileRelocator", false, "Failed to open file for write %s", fullPath);
            return;
        }

        fileStream.Write(str.size(), str.data());
    }

    bool ReplaceAll(AZStd::string& str, const AZStd::string& oldStr, const AZStd::string& newStr)
    {
        return AzFramework::StringFunc::Replace(str, oldStr.c_str(), newStr.c_str());
    }

    bool SourceFileRelocator::UpdateFileReferences(const FileUpdateTask& updateTask)
    {
        const char* fullPath = updateTask.m_absPathFileToUpdate.c_str();
        AZStd::string fileAsString = FileToString(fullPath);

        if(fileAsString.empty())
        {
            return false;
        }

        bool didReplace = false;

        for (int i = 0; i < updateTask.m_oldStrings.size(); ++i)
        {
            didReplace = ReplaceAll(fileAsString, updateTask.m_oldStrings[i], updateTask.m_newStrings[i]);

            if (didReplace)
            {
                break;
            }
        }

        if (didReplace)
        {
            StringToFile(fullPath, fileAsString);
        }

        AZ_TracePrintf("SourceFileRelocator", "Updated %s - %s\n", fullPath, didReplace ? "SUCCESS" : "FAIL");

        return didReplace;
    }

    bool SourceFileRelocator::ComputeProductDependencyUpdatePaths(const SourceFileRelocationInfo& relocationInfo, const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& productDependency,
        AZStd::vector<AZStd::string>& oldPaths, AZStd::vector<AZStd::string>& newPaths, AZStd::string& absPathFileToUpdate) const
    {
        AZStd::string sourceName;
        AZStd::string scanPath;

        // Look up the source file and scanfolder of the product (productPK) that references this file
        m_stateData->QuerySourceByProductID(productDependency.m_productPK, [this, &sourceName, &scanPath](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
        {
            sourceName = entry.m_sourceName;
                        
            m_stateData->QueryScanFolderByScanFolderID(entry.m_scanFolderPK, [&scanPath](AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry)
            {
                scanPath = entry.m_scanFolder;
                return false;
            });

            return false;
        });

        // Find the product this dependency refers to
        auto iterator = relocationInfo.m_products.find(productDependency.m_dependencySubID);

        if(iterator == relocationInfo.m_products.end())
        {
            AZ_Warning("SourceFileRelocator", false, "Can't automatically update references to product, failed to find product with subId %d in product list for file %s", productDependency.m_dependencySubID, relocationInfo.m_oldAbsolutePath.c_str());
            return false;
        }

        // See if the product filename and source file name are the same (not including extension), if so, we can proceed.
        // The names need to be the same because if the source is renamed, we have no way of knowing how that will affect the product name.
        const AZStd::string& productName = iterator->second.m_productName;
        AZStd::string productFileName, sourceFileName;

        AzFramework::StringFunc::Path::GetFileName(productName.c_str(), productFileName);
        AzFramework::StringFunc::Path::GetFileName(relocationInfo.m_oldAbsolutePath.c_str(), sourceFileName);

        if(!AzFramework::StringFunc::Equal(sourceFileName.c_str(), productFileName.c_str(), false))
        {
            AZ_Warning("SourceFileRelocator", false, "Can't automatically update references to product because product name (%s) is different from source name (%s)", productFileName.c_str(), sourceFileName.c_str());
            return false;
        }

        // The names are the same, so just take the source path and replace the extension
        // We're computing the old path as well because the productName includes the platform and gamename which shouldn't be included in hardcoded references
        AZStd::string productExtension;
        AZStd::string oldProductPath = relocationInfo.m_oldRelativePath;
        AZStd::string newProductPath = relocationInfo.m_newRelativePath;

        // Product paths should be all lowercase
        AZStd::to_lower(oldProductPath.begin(), oldProductPath.end());
        AZStd::to_lower(newProductPath.begin(), newProductPath.end());

        AzFramework::StringFunc::Path::GetExtension(productName.c_str(), productExtension);
        AzFramework::StringFunc::Path::ReplaceExtension(oldProductPath, productExtension.c_str());
        AzFramework::StringFunc::Path::ReplaceExtension(newProductPath, productExtension.c_str());

        // This is the full path to the file we need to fix up
        AzFramework::StringFunc::Path::ConstructFull(scanPath.c_str(), sourceName.c_str(), absPathFileToUpdate);

        oldPaths.push_back(oldProductPath);
        oldPaths.push_back(relocationInfo.m_oldRelativePath); // If we fail to find a reference to the product, we'll try to find a reference to the source

        newPaths.push_back(newProductPath);
        newPaths.push_back(relocationInfo.m_newRelativePath);

        return true;
    }

    FileUpdateTasks SourceFileRelocator::UpdateReferences(const SourceFileRelocationContainer& relocationContainer, bool useSourceControl) const
    {
        FileUpdateTasks updateTasks;
        AZStd::unordered_set<AZStd::string> filesToEdit;
        AZStd::unordered_map<AZStd::string, AZStd::string> movedFileMap;

        // Make a record of all moved files.  We might need to edit some of them and if they've already moved,
        // we need to make sure we edit the files at the new location
        for(const SourceFileRelocationInfo& relocationInfo : relocationContainer)
        {
            if (relocationInfo.m_operationSucceeded)
            {
                movedFileMap.insert(AZStd::make_pair(
                    AssetUtilities::NormalizeFilePath(relocationInfo.m_oldAbsolutePath.c_str()).toUtf8().constData(),
                    AssetUtilities::NormalizeFilePath(relocationInfo.m_newAbsolutePath.c_str()).toUtf8().constData()));
            }
        }

        auto pathFixupFunc = [&movedFileMap](const char* filePath) -> AZStd::string
        {
            auto iterator = movedFileMap.find(AssetUtilities::NormalizeFilePath(filePath).toUtf8().constData());

            if (iterator != movedFileMap.end())
            {
                return iterator->second;
            }

            return filePath;
        };

        // Gather a list of all the files to edit and the edits that need to be made
        for(const SourceFileRelocationInfo& relocationInfo : relocationContainer)
        {
            for (const auto& sourceDependency : relocationInfo.m_sourceDependencyEntries)
            {
                AZStd::string fullPath = m_platformConfig->FindFirstMatchingFile(sourceDependency.m_source.c_str()).toUtf8().constData();

                fullPath = pathFixupFunc(fullPath.c_str());

                updateTasks.emplace(AZStd::vector<AZStd::string>{relocationInfo.m_oldRelativePath}, AZStd::vector<AZStd::string>{relocationInfo.m_newRelativePath}, fullPath, sourceDependency.m_fromAssetId);
                filesToEdit.insert(AZStd::move(fullPath));
            }

            for (const auto& productDependency : relocationInfo.m_productDependencyEntries)
            {
                AZStd::string fullPath;
                AZStd::vector<AZStd::string> oldPaths, newPaths;

                if (ComputeProductDependencyUpdatePaths(relocationInfo, productDependency, oldPaths, newPaths, fullPath))
                {
                    fullPath = pathFixupFunc(fullPath.c_str());

                    updateTasks.emplace(oldPaths, newPaths, fullPath, productDependency.m_fromAssetId);
                    filesToEdit.insert(AZStd::move(fullPath));
                }
            }
        }

        // No work to do? Early out
        if(filesToEdit.empty())
        {
            return {};
        }

        if (useSourceControl)
        {
            // Mark all the files for edit
            AZStd::binary_semaphore waitSignal;

            AzToolsFramework::SourceControlResponseCallbackBulk callback = [&waitSignal](bool /*success*/, const AZStd::vector<AzToolsFramework::SourceControlFileInfo>& /*info*/)
            {
                waitSignal.release();
            };

            AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEditBulk, filesToEdit, callback);

            // Wait for the edit command to finish before trying to actually edit the files
            while (!waitSignal.try_acquire_for(AZStd::chrono::milliseconds(200)))
            {
                AZ::TickBus::ExecuteQueuedEvents();
            }
        }

        // Update all the files
        for (FileUpdateTask& updateTask : updateTasks)
        {
            updateTask.m_succeeded = UpdateFileReferences(updateTask);
        }

        return updateTasks;
    }
} // namespace AssetProcessor
