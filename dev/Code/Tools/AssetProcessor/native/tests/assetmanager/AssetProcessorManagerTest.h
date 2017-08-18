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

#include <AzTest/AzTest.h>
#include <qcoreapplication.h>
#include "native/tests/AssetProcessorTest.h"
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include "native/assetprocessor.h"
#include "native/unittests/UnitTestRunner.h"
#include "native/AssetManager/assetProcessorManager.h"
#include "native/utilities/PlatformConfiguration.h"

#include <QTemporaryDir>


class AssetProcessorManager_Test
    : public AssetProcessor::AssetProcessorManager
{
public:
    explicit AssetProcessorManager_Test(AssetProcessor::PlatformConfiguration* config, QObject* parent = nullptr);
    ~AssetProcessorManager_Test() override;
public:
    bool CheckJobKeyToJobRunKeyMap(AZStd::string jobKey);
    bool CheckSourceUUIDToSourceNameMap(AZ::Uuid sourceUUID);
};

class AssetProcessorManagerTest
    : public AssetProcessor::AssetProcessorTest
{
public:
    AssetProcessorManagerTest();
    virtual ~AssetProcessorManagerTest()
    {
    }
protected:
    void SetUp() override;
    void TearDown() override;

    QTemporaryDir m_tempDir;
    AssetProcessorManager_Test* m_assetProcessorManager;
    AssetProcessor::PlatformConfiguration m_config;
    QString m_gameName;
    QDir m_normalizedCacheRootDir;
private:
    int         m_argc;
    char**      m_argv;
    UnitTestUtils::ScopedDir m_scopeDir;

    QCoreApplication* m_qApp;
};

