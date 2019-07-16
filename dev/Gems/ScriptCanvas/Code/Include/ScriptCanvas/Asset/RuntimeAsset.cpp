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

#include "RuntimeAsset.h"

#include <AzCore/Component/Entity.h>

namespace ScriptCanvas
{
    void RuntimeData::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<RuntimeData>()
                ->Version(0)
                ->Field("m_graphData", &RuntimeData::m_graphData)
                ->Field("m_variableData", &RuntimeData::m_variableData)
                ;
        }
    }

    RuntimeData::RuntimeData(RuntimeData&& other)
        : m_graphData(AZStd::move(other.m_graphData))
        , m_variableData(AZStd::move(other.m_variableData))
    {
    }

    RuntimeData& RuntimeData::operator=(RuntimeData&& other)
    {
        if (this != &other)
        {
            m_graphData = AZStd::move(other.m_graphData);
            m_variableData = AZStd::move(other.m_variableData);
        }

        return *this;
    }

    RuntimeAsset::RuntimeAsset(const AZ::Data::AssetId& assetId, AZ::Data::AssetData::AssetStatus status)
        : AZ::Data::AssetData(assetId, status)
    {
    }

    RuntimeAsset::~RuntimeAsset()
    {
        m_runtimeData.m_graphData.Clear(true);
    }

    void RuntimeAsset::SetData(const RuntimeData& runtimeData)
    {
        m_runtimeData = runtimeData;
    }
}
