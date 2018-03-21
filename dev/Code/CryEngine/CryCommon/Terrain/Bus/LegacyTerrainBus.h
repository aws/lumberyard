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

namespace LegacyTerrain
{
    /**
     * Requests for legacy terrain.
     */
    class LegacyTerrainRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~LegacyTerrainRequests() = default;

        /**
         * Gets the terrain height at specified height map indices (clamped if outside terrain).
         * @param x index in the x direction.
         * @param y index in the y direction.
         * @param[out] height height value at the specified indices.
         * return true if the height query succeeded, otherwise false.
         */
        virtual bool GetTerrainHeight(const AZ::u32 x, const AZ::u32 y, float& height) const = 0;
    };

    using LegacyTerrainRequestBus = AZ::EBus<LegacyTerrainRequests>;

    /**
     * Broadcast legacy terrain notifications.
     */
    class LegacyTerrainNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~LegacyTerrainNotifications() = default;

        /**
         * Indicates that the number of terrain tiles or number of samples per tile has changed.
         * @param numTiles The number of tiles along each edge of the terrain (assumed to be equal in both directions).
         * @param tileSize The number of samples along each edge of a tile (assumed to be equal in both directions).
         */
        virtual void SetNumTiles(const AZ::u32 numTiles, const AZ::u32 tileSize) = 0;

        /**
         * Indicates that the height values for a tile have been updated.
         * The height data are assumed to have been linearly scaled with a multiplier and offset determined by
         * heightMin and heightScale.
         * Note that because of the way height values are split up between tiles in Cry, the tile boundaries furthest
         * away from the origin need to share vertices from adjacent tiles to prevent gaps in the terrain.  Therefore,
         * these boundary values will only be correct if tiles are updated after their neighbours in the +x, +y
         * directions.
         * @param tileX The x co-ordinate of the updated tile.
         * @param tileY The y co-ordinate of the updated tile.
         * @param heightMap Pointer to scaled height data.
         * @param heightMin The height corresponding to the value 0 in the scaled height data.
         * @param heightScale The change in height corresponding to 1 unit of the scaled height data.
         * @param tileSize The number of samples along each edge of a tile (assumed to be equal in both directions).
         * @param heightMapUnitSize The distance in metres between adjacent height samples.
         */
        virtual void UpdateTile(const AZ::u32 tileX, const AZ::u32 tileY, const AZ::u16* heightMap,
            const float heightMin, const float heightScale, const AZ::u32 tileSize, const AZ::u32 heightMapUnitSize) = 0;

    };

    using LegacyTerrainNotificationBus = AZ::EBus<LegacyTerrainNotifications>;
} // namespace LegacyTerrain
