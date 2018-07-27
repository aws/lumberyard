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
#include <platform_impl.h>
#include "StaticDataGem.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <StaticDataManager.h>

namespace StaticData
{
    StaticDataGem::StaticDataGem()
    {
        m_descriptors.insert(m_descriptors.end(), {
            CloudCanvas::StaticData::StaticDataManager::CreateDescriptor(),
        });

        Initialize();
    }

    StaticDataGem::~StaticDataGem()
    {

    }

    AZ::ComponentTypeList StaticDataGem::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<CloudCanvas::StaticData::StaticDataManager>(),
        };
    }

}
AZ_DECLARE_MODULE_CLASS(StaticData_4a4bf593603c4f329c76c2a10779311b, StaticData::StaticDataGem)
