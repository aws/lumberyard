/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "CloudGemInGameSurvey_precompiled.h"
#include <platform_impl.h>

#include "CloudGemInGameSurveySystemComponent.h"

#include <AzCore/Module/Module.h>

namespace CloudGemInGameSurvey
{
    class CloudGemInGameSurveyModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(CloudGemInGameSurveyModule, "{0C0ECA81-00BB-4139-9A98-FEEAE2C64983}", AZ::Module);

        CloudGemInGameSurveyModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemInGameSurveySystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemInGameSurveySystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemInGameSurvey_ec06978b23ce4c5294c43c1732f3f31d, CloudGemInGameSurvey::CloudGemInGameSurveyModule)
