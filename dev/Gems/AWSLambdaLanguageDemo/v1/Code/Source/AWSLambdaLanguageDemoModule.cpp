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

#include "AWSLambdaLanguageDemo_precompiled.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "AWSLambdaLanguageDemoSystemComponent.h"

#include <IGem.h>

namespace AWSLambdaLanguageDemo
{
    class AWSLambdaLanguageDemoModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AWSLambdaLanguageDemoModule, "{7235403F-D1C0-46A2-89B6-F00C10E52D35}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AWSLambdaLanguageDemoModule, AZ::SystemAllocator, 0);

        AWSLambdaLanguageDemoModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                AWSLambdaLanguageDemoSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AWSLambdaLanguageDemoSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(AWSLambdaLanguageDemo_35d2c0caf522417398ce5807cbedb60c, AWSLambdaLanguageDemo::AWSLambdaLanguageDemoModule)
