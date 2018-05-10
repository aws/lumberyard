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

#include "CloudsGemEditor.h"

// Metrics
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

#include "EditorCloudComponent.h"

namespace CloudsGem
{
    CloudsGemEditorModule::CloudsGemEditorModule()
        : CloudsGemModule()
    {
        m_descriptors.insert(m_descriptors.end(), { EditorCloudComponent::CreateDescriptor() });

        AZStd::vector<AZ::Uuid> typeIds;
        typeIds.reserve(m_descriptors.size());
        for (AZ::ComponentDescriptor* descriptor : m_descriptors)
        {
            typeIds.emplace_back(descriptor->GetUuid());
        }
        EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, typeIds);
    }

    AZ::ComponentTypeList CloudsGemEditorModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList requiredComponents = CloudsGemModule::GetRequiredSystemComponents();
        return requiredComponents;
    }
} // namespace LmbrCentral

AZ_DECLARE_MODULE_CLASS(CloudsGemEditorModule, CloudsGem::CloudsGemEditorModule)