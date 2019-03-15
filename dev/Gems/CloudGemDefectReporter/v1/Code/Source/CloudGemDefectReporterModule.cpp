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

#include "CloudGemDefectReporter_precompiled.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "CloudGemDefectReporterSystemComponent.h"
#include <TransformDefectReportingComponent.h>
#include "LogCollectingComponent.h"
#include "NetworkInfoCollectingComponent.h"
#include "DxDiagCollectingComponent.h"
#include "FPSDropReportingComponent.h"
#include "ImageLoaderSystemComponent.h"

#include <IGem.h>

namespace CloudGemDefectReporter
{
    class CloudGemDefectReporterModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CloudGemDefectReporterModule, "{D61D8D5F-1664-400E-8D87-42F5484491A9}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(CloudGemDefectReporterModule, AZ::SystemAllocator, 0);

        CloudGemDefectReporterModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemDefectReporterSystemComponent::CreateDescriptor(),
                TransformDefectReportingComponent::CreateDescriptor(),
                LogCollectingComponent::CreateDescriptor(),
                NetworkInfoCollectingComponent::CreateDescriptor(),
                DxDiagCollectingComponent::CreateDescriptor(),
                FPSDropReportingComponent::CreateDescriptor(),
                ImageLoaderSystemComponent::CreateDescriptor()
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemDefectReporterSystemComponent>(),
                azrtti_typeid<LogCollectingComponent>(),
                azrtti_typeid<NetworkInfoCollectingComponent>(),
                azrtti_typeid<DxDiagCollectingComponent>(),
                azrtti_typeid<FPSDropReportingComponent>(),
                azrtti_typeid<ImageLoaderSystemComponent>()
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemDefectReporter_1ba66eefc07f4f6d8705b9530c5dd574, CloudGemDefectReporter::CloudGemDefectReporterModule)
