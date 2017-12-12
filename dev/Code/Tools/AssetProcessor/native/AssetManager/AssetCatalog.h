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

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QStringList>
#include <QHash>
#include <QDir>
#include "native/AssetDatabase/AssetDatabase.h"
#include "native/assetprocessor.h"
#include "native/utilities/assetUtilEBusHelper.h"
#include <AzFramework/Asset/AssetRegistry.h>
#include <QMutex>

namespace AzFramework
{
    class AssetRegistry;
    namespace AssetSystem
    {
        class AssetNotificationMessage;
    }
}

namespace AssetProcessor
{
    class AssetProcessorManager;
    class AssetDatabaseConnection;

    class AssetCatalog 
        : public QObject
        , private AssetRegistryRequestBus::Handler
    {
        using NetworkRequestID = AssetProcessor::NetworkRequestID;
        using BaseAssetProcessorMessage = AzFramework::AssetSystem::BaseAssetProcessorMessage;
        Q_OBJECT;
    
    public:
        AssetCatalog(QObject* parent, QStringList platforms, AZStd::shared_ptr<AssetDatabaseConnection> db);
        ~AssetCatalog();

    Q_SIGNALS:
        // outgoing message to the network
        void SendAssetMessage(QString platform, AzFramework::AssetSystem::AssetNotificationMessage message);

    public Q_SLOTS:
        // incoming message from the AP
        void OnAssetMessage(QString platform, AzFramework::AssetSystem::AssetNotificationMessage message);
        void RequestReady(NetworkRequestID requestId, BaseAssetProcessorMessage* message, QString platform, bool fencingFailed = false);

        void SaveRegistry_Impl();
        void BuildRegistry();
        
    private:

        //////////////////////////////////////////////////////////////////////////
        // AssetRegistryRequestBus::Handler overrides
        int SaveRegistry() override;
        //////////////////////////////////////////////////////////////////////////

        void RegistrySaveComplete(int assetCatalogVersion, bool allCatalogsSaved);

        QHash<QString, AzFramework::AssetRegistry> m_registries; // per platform.
        
        QStringList m_platforms;
        AZStd::shared_ptr<AssetDatabaseConnection> m_db;
        QDir m_cacheRoot;

        bool m_registryBuiltOnce;
        bool m_catalogIsDirty = true;
        bool m_currentlySavingCatalog = false;
        int m_currentRegistrySaveVersion = 0;
        QMutex m_savingRegistryMutex;
        QMultiMap<int, AssetProcessor::NetworkRequestID> m_queuedSaveCatalogRequest;

        AZStd::vector<char> m_saveBuffer; // so that we dont realloc all the time
    };
}
