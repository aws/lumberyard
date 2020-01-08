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

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string_view.h>

class CShader;

namespace Terrain
{
    class TerrainDataProxy
    {
    public:
        virtual ~TerrainDataProxy() = default;

        virtual bool IsDataReady() const = 0;

        virtual float GetHeight(float x, float y) const = 0;
        virtual AZ::Vector3 GetNormal(float x, float y) const = 0;

        virtual float GetHeightmapCellSize() const = 0;
    };

    class TerrainDataRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        virtual float GetHeightmapCellSize() = 0;

        virtual float GetHeightSynchronous(float x, float y) = 0;
        virtual AZ::Vector3 GetNormalSynchronous(float x, float y) = 0;

        virtual CShader* GetTerrainHeightGeneratorShader() const = 0;
        virtual CShader* GetTerrainMaterialCompositingShader() const = 0;

        //////////////////////////////////////////////////////////////////////////
        // Async Heightmap Tile Request Util
        // Primarily written for fulfilling renderer-specific data requests

        // Check if an area has heightmap data ready in the cache
        virtual bool IsHeightmapDataReady(const AZ::Vector2& worldMin, const AZ::Vector2& worldMax) = 0;

        // Request height data for a 2D AABB area in world-space coordinates
        // Calls heightmapDataReadyCallback once terrain data is available within the terrain provider (see TerrainProvider.cpp)
        virtual AZStd::shared_ptr<TerrainDataProxy> RequestHeightmapData(const AZ::Vector2& worldMin, const AZ::Vector2& worldMax, AZStd::function<void()> heightmapDataReadyCallback) = 0;
    };
    using TerrainDataRequestBus = AZ::EBus<TerrainDataRequests>;

    class TerrainShaderRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        virtual void RefreshShader(const AZStd::string_view name, CShader* shader) = 0;
        virtual void ReleaseShader(CShader* shader) const = 0;
    };
    using TerrainShaderRequestBus = AZ::EBus<TerrainShaderRequests>;
}
