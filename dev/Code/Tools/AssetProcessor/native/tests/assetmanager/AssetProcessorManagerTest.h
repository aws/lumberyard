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
#include <AzCore/std/parallel/atomic.h>
#include <qcoreapplication.h>
#include "native/tests/AssetProcessorTest.h"
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include "native/assetprocessor.h"
#include "native/unittests/UnitTestRunner.h"
#include "native/AssetManager/assetProcessorManager.h"
#include "native/utilities/PlatformConfiguration.h"
#include "native/unittests/MockApplicationManager.h"

#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QTemporaryDir>
#include <QMetaObject>

class AssetProcessorManager_Test;

class AssetProcessorManagerTest
    : public AssetProcessor::AssetProcessorTest
{
public:
    

    AssetProcessorManagerTest();
    virtual ~AssetProcessorManagerTest()
    {
    }

    // utility function.  Blocks and runs the QT event pump for up to millisecondsMax and will break out as soon as the APM is idle.
    bool BlockUntilIdle(int millisecondsMax);

protected:
    void SetUp() override;
    void TearDown() override;

    QTemporaryDir m_tempDir;

    AZStd::unique_ptr<AssetProcessorManager_Test> m_assetProcessorManager;
    AZStd::unique_ptr<AssetProcessor::MockApplicationManager> m_mockApplicationManager;
    AZStd::unique_ptr<AssetProcessor::PlatformConfiguration> m_config;
    UnitTestUtils::AssertAbsorber m_assertAbsorber; // absorb asserts/warnings/errors so that the unit test output is not cluttered
    QString m_gameName;
    QDir m_normalizedCacheRootDir;
    AZStd::atomic_bool m_isIdling;
private:
    int         m_argc;
    char**      m_argv;
    AZStd::unique_ptr<UnitTestUtils::ScopedDir> m_scopeDir;

    AZStd::unique_ptr<QCoreApplication> m_qApp;
    QMetaObject::Connection m_idleConnection;
    
};

