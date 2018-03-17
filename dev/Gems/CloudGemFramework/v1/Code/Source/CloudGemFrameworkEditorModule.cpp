/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#include "CloudGemFramework_precompiled.h"

#include <CloudGemFrameworkEditorModule.h>
#include <CloudGemFrameworkEditorSystemComponent.h>

#include <IGem.h>

namespace CloudGemFramework
{

    CloudGemFrameworkEditorModule::CloudGemFrameworkEditorModule()
        : CloudGemFrameworkModule()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            CloudGemFrameworkEditorSystemComponent::CreateDescriptor(),
        });
    }

    /**
        * Add required SystemComponents to the SystemEntity.
        */
    AZ::ComponentTypeList CloudGemFrameworkEditorModule::GetRequiredSystemComponents() const
    {
        auto returnList = CloudGemFrameworkModule::GetRequiredSystemComponents();

        returnList.push_back(azrtti_typeid<CloudGemFrameworkEditorSystemComponent>());

        return returnList;
    }
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
AZ_DECLARE_MODULE_CLASS(CloudGemFrameworkEditorModule, CloudGemFramework::CloudGemFrameworkEditorModule)
