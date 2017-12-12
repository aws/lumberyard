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
#include <QDir>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace AzToolsFramework
{

    static const char* ENGINE_UTIL_LOG_NAME = "AzToolsFramework::EngineUtilities";

    QMap<QString, QMap<QString, QString> >	EngineUtilities::m_engineConfigurationCache;
    QMutex									EngineUtilities::m_engineConfigurationCacheMutex;

    AZ::Outcome<AZStd::string, AZStd::string> EngineUtilities::ReadEngineConfigurationValue(const AZStd::string& currentAssetRoot, const AZStd::string& key)
    {
        QString	assetFolderCacheKey(currentAssetRoot.c_str());
        {
            // Attempt to read from the cache based on the asset root folder first
            QMutexLocker cacheLock(&m_engineConfigurationCacheMutex);
            auto cacheIter = m_engineConfigurationCache.find(assetFolderCacheKey);
            if (cacheIter != m_engineConfigurationCache.end())
            {
                auto valueIter = cacheIter->find(QString(key.c_str()));
                if (valueIter != cacheIter->cend())
                {
                    return AZ::Success(AZStd::string(valueIter->toUtf8().data()));
                }
                else
                {
                    return AZ::Failure(AZStd::string::format("Unable to find key '%s' in engine.json file '%s/engine.json'", key.c_str(), currentAssetRoot.c_str()));
                }
            }
        }

        // Locate, open, parse, and cache the contents of the engine.json file.  
        // The engine.json file is expected to be only 1 level deep, that is key + simple value, so the
        // results from any key lookup will be evaluated as a string if possible
        QDir    currentRoot(QString(currentAssetRoot.c_str()));
        if (!currentRoot.exists())
        {
            return AZ::Failure(AZStd::string::format("Invalid asset root folder: %s", currentAssetRoot.c_str()));
        }

        QString engineJsonFilePath(currentRoot.filePath("engine.json"));
        if (!QFile::exists(engineJsonFilePath))
        {
            return AZ::Failure(AZStd::string::format("Missing engine.json file at folder: %s", currentAssetRoot.c_str()));
        }

        QFile engineJsonFile(engineJsonFilePath);
        if (!engineJsonFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return AZ::Failure(AZStd::string::format("Unable to open engine.json file at folder: %s", currentAssetRoot.c_str()));
        }

        QByteArray engineJsonData = engineJsonFile.readAll();
        engineJsonFile.close();
        QJsonDocument engineJsonDoc(QJsonDocument::fromJson(engineJsonData));
        if (engineJsonDoc.isNull())
        {
            return AZ::Failure(AZStd::string::format("Engine.json file at folder '%s' is not a valid json file.", currentAssetRoot.c_str()));
        }

        QString inputLookupKey(key.c_str());
        QString foundValueString = "";
        bool foundValue = false;
        {
            QMutexLocker cacheLock(&m_engineConfigurationCacheMutex);
            m_engineConfigurationCache[assetFolderCacheKey] = QMap<QString, QString>();
            QMap<QString, QString>& keyValueMap = m_engineConfigurationCache[assetFolderCacheKey];

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
            return AZ::Failure(AZStd::string::format("Unable to find key '%s' in engine.json file '%s/engine.json'", key.c_str(), currentAssetRoot.c_str()));
        }
    }


} // namespace AzToolsFramework
