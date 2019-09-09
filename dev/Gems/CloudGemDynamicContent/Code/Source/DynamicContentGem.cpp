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

#include "DynamicContentGem.h"
#include <AzCore/RTTI/RTTI.h>

#include <platform_impl.h>

#include <DynamicContentTransferManager.h>

namespace DynamicContent
{
    DynamicContentGem::DynamicContentGem()
    {
        m_descriptors.insert(m_descriptors.end(), {
            CloudCanvas::DynamicContent::DynamicContentTransferManager::CreateDescriptor()
        });
    }

    AZ::ComponentTypeList DynamicContent::DynamicContentGem::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<CloudCanvas::DynamicContent::DynamicContentTransferManager>(),
        };
    }
}
AZ_DECLARE_MODULE_CLASS(CloudGemDynamicContent_3a3eeef064a04c37b4513c6378f4f56a, DynamicContent::DynamicContentGem)
