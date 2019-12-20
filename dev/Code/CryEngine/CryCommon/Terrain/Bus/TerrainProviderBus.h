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

#include <AzCore/Math/Vector2.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzCore/EBus/EBus.h>

#include "HeightmapDataBus.h"

class CShader;

namespace Terrain
{
    // This interface defines how the renderer can access the terrain system to set up state and gather information before rendering height maps
    class TerrainProviderRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        // world properties
        virtual AZ::Vector3 GetWorldSize() = 0;
        virtual AZ::Vector3 GetRegionSize() = 0;
        virtual AZ::Vector3 GetWorldOrigin() = 0;
        virtual AZ::Vector2 GetHeightRange() = 0;

        // utility
        virtual void GetRegionIndex(const AZ::Vector2& worldMin, const AZ::Vector2& worldMax, int& regionIndexX, int& regionIndexY) = 0;

        // quadtree min/max height map data request
        // heightmapDataReadyCallback is guaranteed to be called from a separate thread
        virtual void RequestMinMaxHeightmapData(const AZ::Vector2& worldMin, const AZ::Vector2& worldMax, float stride, float* dstBuffer, AZStd::function<void()> heightmapDataReadyCallback) = 0;

        virtual float GetHeightAtIndexedPosition(int ix, int iy) { return 64.0f; }
        virtual float GetHeightAtWorldPosition(float fx, float fy) { return 64.0f; }
        virtual unsigned char GetSurfaceTypeAtIndexedPosition(int ix, int iy) { return 0; }
    };
    using TerrainProviderRequestBus = AZ::EBus<TerrainProviderRequests>;

    // This class exists for the terrain system to inject data into the renderer for generating the GPU-side terrain height map
    struct CRETerrainContext
    {
        // constants for the game to use
        enum class ConstantNames
        {
            Space,
            NoiseSampleSize,
            PixelWorldSize,
            MountainParams,
            Transform,
            PerimeterFlatten,
            ValleyIntensity,
            TractMapTransform,
        };

        // Shader param updates from terrain
        virtual void SetPSConstant(ConstantNames name, float x, float y, float z, float w) = 0;

        // Tract map
        virtual void OnTractVersionUpdate() = 0;

        // Height map version tracking
        virtual void OnHeightMapVersionUpdate() = 0;

        CShader* m_currentShader = nullptr;
    };

    class TerrainProviderNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;
        //////////////////////////////////////////////////////////////////////////

        // interface to be implemented by the game, invoked by the terrain render element

        // pull settings from the world cache, so the next accessors are accurate
        virtual void SynchronizeSettings(CRETerrainContext* context) = 0;

        // height map data requests
        virtual void QueueHeightmapDataRequest(const HeightmapDataRequestInfo& heightmapDataRequest) = 0;

        // height map texture update
        typedef AZStd::function<void(AZ::u32* heightAndNormalData, int topLeftX, int topLeftY, int width, int height)> HeightmapDataFillCallback;
        virtual void ProcessHeightmapDataRequests(int numRequestsToHandle, HeightmapDataFillCallback textureUpdateFunc) = 0;
    };
    using TerrainProviderNotificationBus = AZ::EBus<TerrainProviderNotifications>;
}
