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
#include "native/utilities/PlatformConfiguration.h"
#include "native/utilities/assetUtils.h"
#include "native/assetManager/assetScanFolderInfo.h"
#include "native/assetprocessor.h"
#include "native/utilities/assetUtils.h"
#include <AzCore/Debug/Trace.h>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QJsonValue>

#include <QSettings>
#include <QStringList>
#include <QDir>

namespace
{
    // the starting order in the file for gems.
    const int g_gemStartingOrder = 100;

    const char* s_platformNames[] = {
        "pc",
        "mac",
        "linux",
        "ios",
        "android",
        "durango", // ACCEPTED_USE
        "orbis" // ACCEPTED_USE
    };

    const unsigned int numPlatforms = (sizeof(s_platformNames) / sizeof(const char*));

    const char* s_rendererNames[] = {
        "D3D11",
        "GL4",
        "GLES3",
        "METAL",
        "DURANGO", // ACCEPTED_USE
        "ORBIS" // ACCEPTED_USE
    };

    const unsigned int numRenderers = (sizeof(s_rendererNames) / sizeof(const char*));

    unsigned int GetCrc(const char* name)
    {
        return AssetUtilities::ComputeCRC32(name);
    }

    // Path to the user preferences file
    const char* s_preferencesFileName = "SetupAssistantUserPreferences.ini";
}

using namespace AssetProcessor;

PlatformConfiguration::PlatformConfiguration(QObject* pParent)
    : QObject(pParent)
    , m_minJobs(1)
    , m_maxJobs(8)
{
    for (unsigned int platformIndex = 0; platformIndex < numPlatforms; ++platformIndex)
    {
        unsigned int crc = GetCrc(s_platformNames[platformIndex]);
        m_platformsByCrc[crc] = s_platformNames[platformIndex];
    }

    for (unsigned int rendererIndex = 0; rendererIndex < numRenderers; ++rendererIndex)
    {
        unsigned int crc = GetCrc(s_rendererNames[rendererIndex]);
        m_renderersByCrc[crc] = s_rendererNames[rendererIndex];
    }
}

PlatformConfiguration::~PlatformConfiguration()
{
}

void PlatformConfiguration::PopulateEnabledPlatforms(QString fileSource)
{
    QStringList platforms = AssetUtilities::ReadPlatformsFromCommandLine();

    if (!platforms.isEmpty())
    {
        for (QString platform : platforms)
        {
            if (AZStd::find(m_platforms.begin(), m_platforms.end(), platform) == m_platforms.end())
            {
                m_platforms.push_back(platform);
            }
        }

        //platforms specified in the commandline will always override platforms specified in the config file 
        return;
    }

    //if no platforms is specified in the command line we will always insert the current platform by default;
    if (AZStd::find(m_platforms.begin(), m_platforms.end(), CURRENT_PLATFORM) == m_platforms.end())
    {
        m_platforms.push_back(CURRENT_PLATFORM);
    }

    ReadPlatformsFromConfigFile(fileSource);
}

void PlatformConfiguration::ReadPlatformsFromConfigFile(QString iniPath)
{
    if (QFile::exists(iniPath))
    {
        QSettings loader(iniPath, QSettings::IniFormat);

        // Read in enabled platforms
        loader.beginGroup("Platforms");
        QStringList keys = loader.allKeys();
        for (int idx = 0; idx < keys.count(); idx++)
        {
            QString val = loader.value(keys[idx]).toString();
            QString platform = keys[idx].toLower();
            if (val.toLower() == "enabled")
            {
                if (std::find(m_platforms.begin(), m_platforms.end(), platform) == m_platforms.end())
                {
                    m_platforms.push_back(platform);
                }
            }
            else
            {
                // disable platform
                std::remove(m_platforms.begin(), m_platforms.end(), platform);
            }
        }
        loader.endGroup();
    }
}

bool PlatformConfiguration::ReadRecognizersFromConfigFile(QString iniPath)
{
    QDir assetRoot;
    AssetUtilities::ComputeAssetRoot(assetRoot);
    const QString normalizedRoot = AssetUtilities::NormalizeDirectoryPath(assetRoot.absolutePath());
    const QString gameName = AssetUtilities::ComputeGameName();

    QDir engineRoot;
    AssetUtilities::ComputeEngineRoot(engineRoot);
    const QString normalizedEngineRoot = AssetUtilities::NormalizeDirectoryPath(engineRoot.absolutePath());


    if (QFile::exists(iniPath))
    {
        QSettings loader(iniPath, QSettings::IniFormat);

        loader.beginGroup("Jobs");
        m_minJobs = loader.value("minJobs", m_minJobs).toInt();
        m_maxJobs = loader.value("maxJobs", m_maxJobs).toInt();
        loader.endGroup();

        // Read in scan folders and RC flags per asset/platform
        QStringList groups = loader.childGroups();
        for (QString group : groups)
        {
            if (group.startsWith("ScanFolder ", Qt::CaseInsensitive))
            {
                // its a scan folder!
                loader.beginGroup(group);
                QString watchFolder = loader.value("watch", QString()).toString();
                int order = loader.value("order", 0).toInt();

                watchFolder.replace("@root@", normalizedRoot, Qt::CaseInsensitive);
                watchFolder.replace("@GAMENAME@", gameName, Qt::CaseInsensitive);
                watchFolder.replace("@engineroot@", normalizedEngineRoot, Qt::CaseInsensitive);
                watchFolder = AssetUtilities::NormalizeDirectoryPath(watchFolder);

                if (watchFolder.endsWith(QDir::separator()))
                {
                    watchFolder = watchFolder.left(watchFolder.length() - 1);
                }

                QString outputPrefix = loader.value("output", QString()).toString();
                if (!outputPrefix.isEmpty() && (outputPrefix.endsWith('/') || outputPrefix.endsWith('\\')))
                {
                    outputPrefix = outputPrefix.left(outputPrefix.length() - 1);
                }
                QString scanFolderDisplayName = !outputPrefix.isEmpty() ? outputPrefix : group.split(" ", QString::SkipEmptyParts)[1];
                QString scanFolderPortableKey = QString("from-ini-file-%1").arg(scanFolderDisplayName);

                bool recurse = (loader.value("recursive", 1).toInt() == 1);
                bool isRoot = (watchFolder.compare(normalizedRoot, Qt::CaseInsensitive) == 0);
                recurse &= !isRoot; // root never recurses

                if (!watchFolder.isEmpty())
                {
                    AddScanFolder(ScanFolderInfo(watchFolder, scanFolderDisplayName, scanFolderPortableKey, outputPrefix, isRoot, recurse, order));
                }

                loader.endGroup();
            }

            if (group.startsWith("Exclude ", Qt::CaseInsensitive))
            {
                loader.beginGroup(group);

                ExcludeAssetRecognizer excludeRecognizer;
                excludeRecognizer.m_name = group.mid(8); // chop off the "Exclude " and you're left with the remainder name
                QString pattern = loader.value("pattern", QString()).toString();
                AssetBuilderSDK::AssetBuilderPattern::PatternType patternType = AssetBuilderSDK::AssetBuilderPattern::Regex;
                if (pattern.isEmpty())
                {
                    patternType = AssetBuilderSDK::AssetBuilderPattern::Wildcard;
                    pattern = loader.value("glob", QString()).toString();
                }
                AZStd::string excludePattern(pattern.toUtf8().data());
                excludeRecognizer.m_patternMatcher = AssetUtilities::FilePatternMatcher(excludePattern, patternType);
                m_excludeAssetRecognizers[excludeRecognizer.m_name] = excludeRecognizer;
                loader.endGroup();
            }

            if (group.startsWith("RC ", Qt::CaseInsensitive))
            {
                loader.beginGroup(group);

                AssetRecognizer rec;
                rec.m_name = group.mid(3); // chop off the "RC " and you're left with the remainder name
                if (!ReadRecognizerFromConfig(rec, loader))
                {
                    return false;
                }
                if (!rec.m_platformSpecs.empty())
                {
                    m_assetRecognizers[rec.m_name] = rec;
                }

                loader.endGroup();
            }
        }
    }
    return true;
}

void PlatformConfiguration::ReadMetaDataFromConfigFile(QString iniPath)
{
    if (QFile::exists(iniPath))
    {
        QSettings loader(iniPath, QSettings::IniFormat);

        //Read in Metadata types
        loader.beginGroup("MetaDataTypes");
        QStringList fileExtensions = loader.allKeys();
        for (int idx = 0; idx < fileExtensions.count(); idx++)
        {
            QString fileType = AssetUtilities::NormalizeFilePath(fileExtensions[idx]);
            QString extensionType = loader.value(fileType, QString()).toString();

            AddMetaDataType(fileType, extensionType);

            // Check if the Metadata 'file type' is a real file
            QString fullPath = FindFirstMatchingFile(fileType);
            if (!fullPath.isEmpty())
            {
                m_metaDataRealFiles.insert(fileType.toLower());
            }
        }

        loader.endGroup();
    }
}

QString PlatformConfiguration::PlatformAt(int pos) const
{
    return m_platforms[pos];
}

QPair<QString, QString> PlatformConfiguration::GetMetaDataFileTypeAt(int pos) const
{
    return m_metaDataFileTypes[pos];
}

bool PlatformConfiguration::IsMetaDataTypeRealFile(QString relativeName) const
{
    return m_metaDataRealFiles.find(relativeName.toLower()) != m_metaDataRealFiles.end();
}

QString PlatformConfiguration::PlatformName(unsigned int platformCrc) const
{
    auto it = m_platformsByCrc.find(platformCrc);
    if (it != m_platformsByCrc.end())
    {
        return *it;
    }
    return "";
}

QString PlatformConfiguration::RendererName(unsigned int rendererCrc) const
{
    auto it = m_renderersByCrc.find(rendererCrc);
    if (it != m_renderersByCrc.end())
    {
        return *it;
    }
    return "";
}


void PlatformConfiguration::EnablePlatform(QString platform, bool enable)
{
    if (enable)
    {
        m_platforms.push_back(platform.toLower());
    }
    else
    {
        auto platformIt = std::find(m_platforms.begin(), m_platforms.end(), platform.toLower());
        if (platformIt != m_platforms.end())
        {
            m_platforms.erase(platformIt);
        }
    }
}

bool PlatformConfiguration::GetMatchingRecognizers(QString fileName, RecognizerPointerContainer& output) const
{
    bool foundAny = false;
    if (IsFileExcluded(fileName))
    {
        //if the file is excluded than return false;
        return false;
    }
    for (const AssetRecognizer& recognizer : m_assetRecognizers)
    {
        if (recognizer.m_patternMatcher.MatchesPath(fileName))
        {
            // found a match
            output.push_back(&recognizer);
            foundAny = true;
        }
    }
    return foundAny;
}

int PlatformConfiguration::GetScanFolderCount() const
{
    return m_scanFolders.count();
}

AssetProcessor::ScanFolderInfo& PlatformConfiguration::GetScanFolderAt(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < m_scanFolders.count());
    return m_scanFolders[index];
}

void PlatformConfiguration::AddScanFolder(const AssetProcessor::ScanFolderInfo& source, bool isUnitTesting)
{
    if (isUnitTesting)
    {
        //using a bool instead of using #define UNIT_TEST because the user can also run batch processing in unittest
        m_scanFolders.push_back(source);
        return;
    }

    // Find and remove any previous matching entry, last entry wins
    auto it = std::find_if(m_scanFolders.begin(), m_scanFolders.end(), [&source](const ScanFolderInfo& info)
            {
                return info.ScanPath() == source.ScanPath();
            });
    if (it != m_scanFolders.end())
    {
        m_scanFolders.erase(it);
    }

    m_scanFolders.push_back(source);

    qSort(m_scanFolders.begin(), m_scanFolders.end(), [](const ScanFolderInfo& a, const ScanFolderInfo& b)
        {
            return a.GetOrder() < b.GetOrder();
        });
}

void PlatformConfiguration::AddRecognizer(const AssetRecognizer& source)
{
    m_assetRecognizers.insert(source.m_name, source);
}

void PlatformConfiguration::RemoveRecognizer(QString name)
{
    auto found = m_assetRecognizers.find(name);
    m_assetRecognizers.erase(found);
}

void PlatformConfiguration::AddMetaDataType(const QString& type, const QString& extension)
{
    QPair<QString, QString> key = qMakePair(type.toLower(), extension.toLower());
    if (!m_metaDataFileTypes.contains(key))
    {
        m_metaDataFileTypes.push_back(key);
    }
}

bool PlatformConfiguration::ConvertToRelativePath(QString fullFileName, QString& relativeName, QString& scanFolderName) const
{
    const ScanFolderInfo* info = GetScanFolderForFile(fullFileName);
    if (info)
    {
        QString relPath; // empty string.
        if (fullFileName.length() > info->ScanPath().length())
        {
            relPath = fullFileName.right(fullFileName.length() - info->ScanPath().length() - 1); // also eat the slash, hence -1
        }
        scanFolderName = info->ScanPath();

        if (info->GetOutputPrefix().isEmpty())
        {
            relativeName = relPath;
        }
        else
        {
            relativeName = info->GetOutputPrefix();

            if (!relPath.isEmpty())
            {
                relativeName += '/';
                relativeName += relPath;
            }
        }

        return true;
    }
    // did not find it.
    return false;
}

QString PlatformConfiguration::GetOverridingFile(QString relativeName, QString scanFolderName) const
{
    for (int pathIdx = 0; pathIdx < m_scanFolders.size(); ++pathIdx)
    {
        AssetProcessor::ScanFolderInfo scanFolderInfo = m_scanFolders[pathIdx];

        if (scanFolderName.compare(scanFolderInfo.ScanPath(), Qt::CaseInsensitive) == 0)
        {
            // we have found the root of the existing file.
            // nothing can override it.
            return QString();
        }
        QString tempRelativeName(relativeName);
        //if relative path starts with the output prefix than remove it first
        if (!scanFolderInfo.GetOutputPrefix().isEmpty() && tempRelativeName.startsWith(scanFolderInfo.GetOutputPrefix(), Qt::CaseInsensitive))
        {
            tempRelativeName = tempRelativeName.right(tempRelativeName.length() - scanFolderInfo.GetOutputPrefix().length() - 1); // also eat the slash, hence -1
        }

        if ((!scanFolderInfo.RecurseSubFolders()) && (tempRelativeName.contains('/')))
        {
            // the name is a deeper relative path, but we don't recurse this scan folder, so it can't win
            continue;
        }

        QString checkForReplacement = QDir(scanFolderInfo.ScanPath()).absoluteFilePath(tempRelativeName);


        if (QFile::exists(checkForReplacement))
        {
            // we have found a file in an earlier scan folder that would override this file
            return AssetUtilities::NormalizeFilePath(checkForReplacement);
        }
    }

    // we found it nowhere.
    return QString();
}

QString PlatformConfiguration::FindFirstMatchingFile(QString relativeName) const
{
    if (relativeName.isEmpty())
    {
        return QString();
    }

    for (int pathIdx = 0; pathIdx < m_scanFolders.size(); ++pathIdx)
    {
        AssetProcessor::ScanFolderInfo scanFolderInfo = m_scanFolders[pathIdx];

        QString tempRelativeName(relativeName);

        //if relative path starts with the output prefix than remove it first
        if (!scanFolderInfo.GetOutputPrefix().isEmpty() && tempRelativeName.startsWith(scanFolderInfo.GetOutputPrefix(), Qt::CaseInsensitive))
        {
            tempRelativeName = tempRelativeName.right(tempRelativeName.length() - scanFolderInfo.GetOutputPrefix().length() - 1); // also eat the slash, hence -1
        }
        if ((!scanFolderInfo.RecurseSubFolders()) && (tempRelativeName.contains('/')))
        {
            // the name is a deeper relative path, but we don't recurse this scan folder, so it can't win
            continue;
        }
        QDir rooted(scanFolderInfo.ScanPath());
        QString fullPath = rooted.absoluteFilePath(tempRelativeName);
        if (QFile::exists(fullPath))
        {
            // the problem here is that on case insensitive file systems, the file "BLAH.TXT" will also respond as existing if we ask for "blah.txt"
            // but we want to return the actual real casing of the file itself.
            // to do this, we use the directory information for the directory the file is in
            // we know that this always succeeds since we just verified that the file exists.
            QFileInfo newInfo(fullPath);
            QStringList searchPattern;
            searchPattern << newInfo.fileName();
            QStringList actualCasing = newInfo.absoluteDir().entryList(searchPattern, QDir::Files);

            if (actualCasing.isEmpty())
            {
                return tempRelativeName;
            }

            return AssetUtilities::NormalizeFilePath(newInfo.absoluteDir().absoluteFilePath(actualCasing[0]));
        }
    }
    return QString();
}

const AssetProcessor::ScanFolderInfo* PlatformConfiguration::GetScanFolderForFile(const QString& fullFileName) const
{
    QString normalized = AssetUtilities::NormalizeFilePath(fullFileName);
    for (int pathIdx = 0; pathIdx < m_scanFolders.size(); ++pathIdx)
    {
        QString scanFolderName = m_scanFolders[pathIdx].ScanPath();
        if (normalized.startsWith(scanFolderName, Qt::CaseInsensitive))
        {
            if (normalized.compare(scanFolderName, Qt::CaseInsensitive) == 0)
            {
                // if its an exact match, we're basically done
                return &m_scanFolders[pathIdx];
            }
            else
            {
                if (normalized.length() > scanFolderName.length())
                {
                    QChar examineChar = normalized[scanFolderName.length()]; // it must be a slash or its just a scan folder that starts with the same thing by coincidence.
                    if (examineChar != QChar('/'))
                    {
                        continue;
                    }
                    QString relPath = normalized.right(normalized.length() - scanFolderName.length() - 1); // also eat the slash, hence -1
                    if (!m_scanFolders[pathIdx].RecurseSubFolders())
                    {
                        // we only allow things that are in the root for nonrecursive folders
                        if (relPath.contains('/'))
                        {
                            continue;
                        }
                    }
                    return &m_scanFolders[pathIdx];
                }
            }
        }
    }
    return nullptr; // not found.
}

int PlatformConfiguration::GetMinJobs() const
{
    return m_minJobs;
}

int PlatformConfiguration::GetMaxJobs() const
{
    return m_maxJobs;
}


bool PlatformConfiguration::ReadRecognizerFromConfig(AssetRecognizer& target, QSettings& loader)
{
    AssetBuilderSDK::AssetBuilderPattern::PatternType patternType = AssetBuilderSDK::AssetBuilderPattern::Regex;
    QString pattern = loader.value("pattern", QString()).toString();
    if (pattern.isEmpty())
    {
        patternType = AssetBuilderSDK::AssetBuilderPattern::Wildcard;
        pattern = loader.value("glob", QString()).toString();
    }

    if (pattern.isEmpty())
    {
        // no pattern present
        AZ_Warning(AssetProcessor::ConsoleChannel, false, "No pattern was found in config %s while reading platform config.\n", target.m_name.toUtf8().data());
        return false;
    }

    target.m_patternMatcher = AssetUtilities::FilePatternMatcher(AZStd::string(pattern.toUtf8().data()), patternType);


    if (!target.m_patternMatcher.IsValid())
    {
        // no pattern present
        AZ_Warning(AssetProcessor::ConsoleChannel, false, "Invalid pattern in %s while reading platform config : %s \n",
            target.m_name.toUtf8().data(),
            target.m_patternMatcher.GetErrorString().c_str());
        return false;
    }

    // Qt actually has a custom INI parse which treats commas as string lists.
    // so if we want the original (with commas) we have to check for that
    QVariant paramValue = loader.value("params", QString());
    QString defaultParams = paramValue.type() == QVariant::StringList ? paramValue.toStringList().join(",") : paramValue.toString();

    for (int platformIndex = 0; platformIndex < PlatformCount(); ++platformIndex)
    {
        QString platform = PlatformAt(platformIndex);
        AssetPlatformSpec spec;
        spec.m_extraRCParams = defaultParams;
        paramValue = loader.value(platform, QString());

        QString overrideParams = paramValue.type() == QVariant::StringList ? paramValue.toStringList().join(",") : paramValue.toString();
        if (!overrideParams.isEmpty())
        {
            spec.m_extraRCParams = overrideParams;
        }
        target.m_platformSpecs[platform] = spec;
    }

    target.m_version = loader.value("version", QString()).toString();
    target.m_testLockSource = loader.value("lockSource", false).toBool();
    target.m_isCritical = loader.value("critical", false).toBool();
    target.m_supportsCreateJobs = loader.value("supportsCreateJobs", false).toBool();
    target.m_priority = loader.value("priority", 0).toInt();
    QString assetTypeString = loader.value("productAssetType", QString()).toString();
    if (!assetTypeString.isEmpty())
    {
        target.m_productAssetType = AZ::Uuid(assetTypeString.toUtf8().data());
        if (target.m_productAssetType.IsNull())
        {
            // you specified a UUID and it did not read.  A warning will have already been issued
            return false;
        }
    }


    return true;
}

void PlatformConfiguration::ReadGemsConfigFile(QString gemsFile, QStringList& gemConfigFiles)
{
    if (QFile::exists(gemsFile))
    {
        // load gems
        ReadGems(gemsFile, gemConfigFiles);
    }
}

int PlatformConfiguration::ReadGems(QString gemsConfigName, QStringList& gemConfigFiles)
{
    QDir assetRoot;
    AssetUtilities::ComputeAssetRoot(assetRoot);

    QDir engineRoot;
    AssetUtilities::ComputeEngineRoot(engineRoot);

    // The set of paths to search for each Gem
    QStringList searchPaths;
    // Always search the asset root
    searchPaths.append(AssetUtilities::NormalizeDirectoryPath(assetRoot.absolutePath()));

    // If the asset root and engine root is not the same, then add the engine root separately
    if (!(assetRoot == engineRoot))
    {
        searchPaths.append(AssetUtilities::NormalizeDirectoryPath(engineRoot.absolutePath()));
    }

    /* Parse SetupAssistantUserPreferences.ini, which contains Gems search paths. Looks like:
     * [General]
     * SDKEnginePathValid=true
     * SDKSearchPath3rdParty=F:/lyengine/3rdParty
     *
     * [Gems]
     * SearchPaths\size=2
     * SearchPaths\1\Path=E:\\Gems
     * SearchPaths\2\Path=C:\\Users\\colden\\Gems
     */
    if (engineRoot.exists(s_preferencesFileName))
    {
        QSettings userPreferences{ engineRoot.absoluteFilePath(s_preferencesFileName), QSettings::IniFormat };
        int arraySize = userPreferences.beginReadArray("Gems/SearchPaths");
        for (int i = 0; i < arraySize; ++i)
        {
            userPreferences.setArrayIndex(i);
            // Check that the path actually exists, then save it to the search paths
            QString path = AssetUtilities::NormalizeDirectoryPath(userPreferences.value("Path").toString());
            QDir dir{ path };
            if (dir.exists() && !searchPaths.contains(path))
            {
                searchPaths.append(path);
            }
        }
        userPreferences.endArray();
    }

    QFile sourceFile(gemsConfigName);
    if (!sourceFile.open(QFile::ReadOnly))
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Failed to open Gems configuration file : %s.\n", gemsConfigName.toUtf8().data());
        return 0;
    }

    QJsonParseError perror;
    QJsonDocument doc = QJsonDocument::fromJson(sourceFile.readAll(), &perror);
    if ((perror.error != QJsonParseError::NoError) || (!doc.isObject()))
    {
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Error parsing %s ... all gems disabled.  Error follows.\n", gemsConfigName.toUtf8().data());
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Error is at offset: %d and is %s.\n", perror.offset, perror.errorString().toUtf8().data());
        return 0;
    }

    QJsonValue val = doc.object().value("Gems");
    if (!val.isArray())
    {
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Error parsing %s ... \"Gems\" tag not found or is not an array... gems disabled.\n", gemsConfigName.toUtf8().data());
        return 0;
    }

    QJsonArray gemArray = val.toArray();

    int gemOrder = g_gemStartingOrder;

    /* GEMS.json contains a series of structures which look like this:
    * {
    * "GemListFormatVersion": 2,
    * "Gems": [
    *    {
    *         "Path": "Gems/Camera",
    *         "Uuid": "0e70f458e7d24fd6be29ab38ce2ebb84",
    *         "Version": "0.1.0",
    *         "_comment": "Camera"
    *     },
    *     {
    *         "Path": "Gems/LightningArc",
    *         "Uuid": "4c28210b23544635aa15be668dbff15d",
    *         "Version": "1.0.0",
    *         "_comment": "LightningArc"
    *       },
    *  etc...
    *
    * We care about the PATH only, but we'll validate that a legitimite Uuid and Version tag exists too...
    */

    int readGems = 0;
    for (const QJsonValue& gemElement : gemArray)
    {
        if (!gemElement.isObject())
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Error parsing %s  ... \"Gems\" element has a non-object element in its array. skipping.\n", gemsConfigName.toUtf8().data());
            continue;
        }
        QJsonObject gemObj = gemElement.toObject();
        QString gemGuid = gemObj.value("Uuid").toString();
        QString gemVersion = gemObj.value("Version").toString();
        QString gemPath = gemObj.value("Path").toString();
        QString folderName = gemPath.split("/").last();

        if ((gemGuid.isEmpty()) || (gemVersion.isEmpty()) || (gemPath.isEmpty()))
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Error parsing %s  ... gem has a missing Guid or Version or Path tag.  skipping!.\n", gemsConfigName.toUtf8().data());
            continue;
        }

        for (const QString& searchPath : searchPaths)
        {
            QDir gemDir(searchPath + "/" + gemPath);
            if (gemDir.exists())
            {
                QString gemFolder = searchPath + "/" + gemPath + "/Assets";
                AZ_TracePrintf(AssetProcessor::DebugChannel, "Adding GEM assets folder for monitoring / scanning: %s.\n", gemFolder.toUtf8().data());
                QString portableKey = QString("gemassets-%1").arg(gemGuid);
                AddScanFolder(ScanFolderInfo(gemFolder, gemPath + "/Assets", portableKey, "", false, true, gemOrder++));

                // add the gem folder itself for the root metadata (non-recursive)
                gemFolder = searchPath + "/" + gemPath;
                portableKey = QString("rootgem-%1").arg(gemGuid);
                AZ_TracePrintf(AssetProcessor::DebugChannel, "Adding GEM folder for gem.json: %s.\n", gemFolder.toUtf8().data());
                AddScanFolder(ScanFolderInfo(gemFolder, folderName, portableKey, gemPath, true, false, gemOrder++));

                QString gemConfigPath = gemFolder + "/AssetProcessorGemConfig.ini";
                if (QFile::exists(gemConfigPath))
                {
                    gemConfigFiles.push_back(gemConfigPath);
                }

                ++readGems;

                // Stop searching paths once found
                break;
            }
        }
    }
    return readGems;
}


const RecognizerContainer& PlatformConfiguration::GetAssetRecognizerContainer() const
{
    return this->m_assetRecognizers;
}

const ExcludeRecognizerContainer& PlatformConfiguration::GetExcludeAssetRecognizerContainer() const
{
    return this->m_excludeAssetRecognizers;
}

void AssetProcessor::PlatformConfiguration::AddExcludeRecognizer(const ExcludeAssetRecognizer& recogniser)
{
    m_excludeAssetRecognizers.insert(recogniser.m_name, recogniser);
}

void AssetProcessor::PlatformConfiguration::RemoveExcludeRecognizer(QString name)
{
    auto found = m_excludeAssetRecognizers.find(name);
    if (found != m_excludeAssetRecognizers.end())
    {
        m_excludeAssetRecognizers.erase(found);
    }
}

bool AssetProcessor::PlatformConfiguration::IsFileExcluded(QString fileName) const
{
    for (const ExcludeAssetRecognizer& excludeRecognizer : m_excludeAssetRecognizers)
    {
        if (excludeRecognizer.m_patternMatcher.MatchesPath(fileName))
        {
            return true;
        }
    }
    return false;
}

#include <native/utilities/PlatformConfiguration.moc>
