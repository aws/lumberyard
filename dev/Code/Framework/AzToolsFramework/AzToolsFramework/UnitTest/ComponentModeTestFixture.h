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

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzCore/UnitTest/TestTypes.h>
#include "ComponentModeTestDoubles.h"

namespace UnitTest
{
    class ComponentModeTestFixture
        : public AllocatorsTestFixture
    {
    protected:
        void SetUp() override;
        void TearDown() override;
        template<typename ComponentT> void EnterComponentMode();

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AzToolsFramework::ToolsApplication m_app;
        TestEditorActions m_editorActions;
    };

    template<typename ComponentT>
    void ComponentModeTestFixture::EnterComponentMode()
    {
        using namespace AzToolsFramework;
        using namespace ComponentModeFramework;
        ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeSystemRequests::AddSelectedComponentModesOfType,
            AZ::AzTypeInfo<ComponentT>::Uuid());
    }

    void SelectEntities(const AzToolsFramework::EntityIdList& entityList);

} // UnitTest
