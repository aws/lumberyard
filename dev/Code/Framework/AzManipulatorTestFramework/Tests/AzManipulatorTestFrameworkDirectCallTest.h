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

#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkFixture.h>

namespace UnitTest
{
    class CustomManipulatorManager
        : public AzToolsFramework::ManipulatorManager
    {
        using ManagerBase = AzToolsFramework::ManipulatorManager;
    public:
        using ManagerBase::ManagerBase;

        size_t RegisteredManipulatorCount() const
        {
            return m_manipulatorIdToPtrMap.size();
        }
    };

    class AzManipulatorTestFrameworkCustomManagerTest
        : public AzManipulatorTestFrameworkTest
    {
    protected:
        AzManipulatorTestFrameworkCustomManagerTest()
            : AzManipulatorTestFrameworkTest(AzToolsFramework::ManipulatorManagerId(AZ::Crc32("TestManipulatorManagerId"))) {}

        void SetUpEditorFixtureImpl() override
        {       
            m_manipulatorManager =
                AZStd::make_shared<CustomManipulatorManager>(m_manipulatorManagerId);

            AzManipulatorTestFrameworkTest::SetUpEditorFixtureImpl();
        }

        void TearDownEditorFixtureImpl() override
        {
            AzManipulatorTestFrameworkTest::TearDownEditorFixtureImpl();
            m_manipulatorManager.reset();
        }

        AZStd::shared_ptr<CustomManipulatorManager> m_manipulatorManager;
    };
} // namespace UnitTest

