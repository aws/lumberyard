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

#include "CloudCanvasCommon_precompiled.h"

#include <platform_impl.h> // must be included once per DLL so things from CryCommon will function

#include "CloudCanvasCommonModule.h"
#include "CloudCanvasCommonSystemComponent.h"

#include <IGem.h>

namespace CloudCanvasCommon
{

    CloudCanvasCommonModule::CloudCanvasCommonModule()
            : CryHooksModule()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {
            CloudCanvasCommonSystemComponent::CreateDescriptor(),
        });
    }

    /**
    * Add required SystemComponents to the SystemEntity.
    */
    AZ::ComponentTypeList CloudCanvasCommonModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<CloudCanvasCommonSystemComponent>(),
        };
    }

}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudCanvasCommon_102e23cf4c4c4b748585edbce2bbdc65, CloudCanvasCommon::CloudCanvasCommonModule)
