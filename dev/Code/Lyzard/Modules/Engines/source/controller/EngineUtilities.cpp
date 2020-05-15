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

#include "StdAfx.h"

#include "EngineUtilities.h"

#include <QString>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QFileInfo::d_ptr': class 'QSharedDataPointer<QFileInfoPrivate>' needs to have dll-interface to be used by clients of class 'QFileInfo'
#include <QDir>
AZ_POP_DISABLE_WARNING

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace AzToolsFramework
{
    const char* EngineConfiguration::s_fileName = "engine.json";

    // Internally maintain a cache of the engine json files
    namespace Cache
    {
        // Maintain a cache of engine configurations that have been read
        static QMap<QString, QMap<QString, QString>>  s_engineConfigurationCache;
        static QMutex                                 s_engineConfigurationCacheMutex;
        static QMap<QString, QString>                 s_assetRootToEngineRootCache;
        static QMutex                                 s_assetRootToEngineRootCacheMutex;
    }

    AZ::Outcome<AZStd::string, AZStd::string> EngineConfiguration::ReadValue(const AZStd::string& rootFolderPath, const AZStd::string& key)
    {
        QString assetFolderCacheKey(rootFolderPath.c_str());
        {
            // Attempt to read from the cache based on the asset root folder first
            QMutexLocker cacheLock(&Cache::s_engineConfigurationCacheMutex);
            auto cacheIter = Cache::s_engineConfigurationCache.find(assetFolderCacheKey);
            if (cacheIter != Cache::s_engineConfigurationCache.end())
            {
                auto valueIter = cacheIter->find(QString(key.c_str()));
                if (valueIter != cacheIter->cend())
                {
                    return AZ::Success(AZStd::string(valueIter->toUtf8().data()));
                }
                else
                {
                    return AZ::Failure(AZStd::string::format("Unable to find key '%s' in engine.json file '%s/engine.json'", key.c_str(), rootFolderPath.c_str()));
                }
            }
        }

        // Locate, open, parse, and cache the contents of the engine.json file.  
        // The engine.json file is expected to be only 1 level deep, that is key + simple value, so the
        // results from any key lookup will be evaluated as a string if possible
        QDir    currentRoot(QString(rootFolderPath.c_str()));
        if (!currentRoot.exists())
        {
            return AZ::Failure(AZStd::string::format("Invalid root folder: %s", rootFolderPath.c_str()));
        }

        QString engineJsonFilePath(currentRoot.filePath("engine.json"));
        if (!QFile::exists(engineJsonFilePath))
        {
            return AZ::Failure(AZStd::string::format("Missing engine.json file at folder: %s", rootFolderPath.c_str()));
        }

        QFile engineJsonFile(engineJsonFilePath);
        if (!engineJsonFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return AZ::Failure(AZStd::string::format("Unable to open engine.json file at folder: %s", rootFolderPath.c_str()));
        }

        QByteArray engineJsonData = engineJsonFile.readAll();
        engineJsonFile.close();
        QJsonDocument engineJsonDoc(QJsonDocument::fromJson(engineJsonData));
        if (engineJsonDoc.isNull())
        {
            return AZ::Failure(AZStd::string::format("Engine.json file at folder '%s' is not a valid json file.", rootFolderPath.c_str()));
        }

        QString inputLookupKey(key.c_str());
        QString foundValueString = "";
        bool foundValue = false;
        {
            QMutexLocker cacheLock(&Cache::s_engineConfigurationCacheMutex);
            Cache::s_engineConfigurationCache[assetFolderCacheKey] = QMap<QString, QString>();
            QMap<QString, QString>& keyValueMap = Cache::s_engineConfigurationCache[assetFolderCacheKey];

            QJsonObject engineJsonRoot = engineJsonDoc.object();
            for (const QString configKey : engineJsonRoot.keys())
            {
                QJsonValue configValue = engineJsonRoot[configKey];
                keyValueMap[configKey] = configValue.toString();
                if (configKey.compare(inputLookupKey) == 0)
                {
                    foundValue = true;
                    foundValueString = configValue.toString();
                }
            }
        }
        if (foundValue)
        {
            return AZ::Success(AZStd::string(foundValueString.toUtf8().data()));
        }
        else
        {
            return AZ::Failure(AZStd::string::format("Unable to find key '%s' in engine.json file '%s/engine.json'", key.c_str(), rootFolderPath.c_str()));
        }
    }

} // namespace AzToolsFramework
