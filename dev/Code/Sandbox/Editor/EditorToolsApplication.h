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
#include <AzCore/Component/Component.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/EBus/EBus.h>

#include <AzToolsFramework/Application/ToolsApplication.h>

#include "Core/EditorMetricsPlainTextNameRegistration.h"

namespace EditorInternal
{
    // Override the ToolsApplication so that we can special case when the config file is not
    // found and give the user of the Editor a specific message about it.
    class EditorToolsApplication : public AzToolsFramework::ToolsApplication
    {
    public:
        bool OnFailedToFindConfiguration(const char* configFilePath) override;

        bool IsStartupAborted() const;

        void RegisterCoreComponents() override;

        AZ::ComponentTypeList GetRequiredSystemComponents() const;

        void StartCommon(AZ::Entity* systemEntity) override;

        AZ::Outcome<AZStd::string, AZStd::string> ResolveToolApplicationPath(const char* toolName);

        using AzToolsFramework::ToolsApplication::Start;
        bool Start(int argc, char* argv[]);

    private:
        bool m_StartupAborted = false;
        Editor::EditorMetricsPlainTextNameRegistrationBusListener m_metricsPlainTextRegistrar;

        bool GetOptionalAppRootArg(int argc, char* argv[], char destinationRootArgBuffer[], size_t destinationRootArgBufferSize) const;
    };

}


