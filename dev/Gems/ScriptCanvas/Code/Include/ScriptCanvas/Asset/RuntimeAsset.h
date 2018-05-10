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


#include <AzCore/Asset/AssetCommon.h>
#include <ScriptCanvas/Core/GraphData.h>
#include <ScriptCanvas/Variable/VariableData.h>

namespace ScriptCanvas
{
    struct RuntimeData
    {
        AZ_TYPE_INFO(RuntimeData, "{A935EBBC-D167-4C59-927C-5D98C6337B9C}");
        AZ_CLASS_ALLOCATOR(RuntimeData, AZ::SystemAllocator, 0);
        RuntimeData() = default;
        ~RuntimeData() = default;
        RuntimeData(const RuntimeData&) = default;
        RuntimeData& operator=(const RuntimeData&) = default;
        RuntimeData(RuntimeData&&);
        RuntimeData& operator=(RuntimeData&&);

        static void Reflect(AZ::ReflectContext* reflectContext);

        GraphData m_graphData;
        VariableData m_variableData;
    };
    class RuntimeAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(RuntimeAsset, "{3E2AC8CD-713F-453E-967F-29517F331784}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(RuntimeAsset, AZ::SystemAllocator, 0);

        RuntimeAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(), AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded);
        ~RuntimeAsset() override;

        static const char* GetFileExtension() { return "scriptcanvas_compiled"; }
        static const char* GetFileFilter() { return "*.scriptcanvas_compiled"; }

        const RuntimeData& GetData() const { return m_runtimeData; }
        RuntimeData& GetData() { return m_runtimeData; }
        void SetData(const RuntimeData& runtimeData);

    protected:
        friend class RuntimeAssetHandler;
        RuntimeAsset(const RuntimeAsset&) = delete;

        RuntimeData m_runtimeData;
    };
}
