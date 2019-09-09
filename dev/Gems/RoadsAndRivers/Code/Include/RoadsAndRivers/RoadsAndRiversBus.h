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

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Plane.h>

namespace RoadsAndRivers
{
    class RoadRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Triggers full rebuild of the road object, including geometry and render node generation.
         */
        virtual void Rebuild() = 0;

        /**
         * Sets whether to allow the road texture to be rendered over terrain holes.
         */
        virtual void SetIgnoreTerrainHoles(bool ignoreTerrainHoles) {}

        /**
         * Gets if the road rendering is ignoring terrain holes.
         */
        virtual bool GetIgnoreTerrainHoles() { return false; }

    };
    using RoadRequestBus = AZ::EBus<RoadRequests>;

    class RoadNotifications
        : public AZ::ComponentBus
    {
    public:
        /**
         * Notify listeners that the setting for Ignore Terrain Holes has changed
         */
        virtual void OnIgnoreTerrainHolesChanged(bool ignoreTerrainHoles) {}
    };
    using RoadNotificationBus = AZ::EBus<RoadNotifications>;

    class RiverRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Sets the depth of the river
         */
        virtual void SetWaterVolumeDepth(float waterVolumeDepth) = 0;
        /**
         * Gets the depth of the river
         */
        virtual float GetWaterVolumeDepth() = 0;

        /**
         * Sets how much the surface texture is tiled horizontally.
         *
         * The surface texture is often defined as a bump/normal map.
         * "Horizontally" refers to the U axis of the UV map.
         */
        virtual void SetTileWidth(float tileWidth) = 0;
        /**
         * Gets how much the surface texture is tiled horizontally.
         */
        virtual float GetTileWidth() = 0;

        /**
         * Sets whether or not the underwater fog effect is capped at the volume's depth.
         *
         * If set to false underwater fog will continue to render no matter how far beneath
         * the water volume the camera is.
         */
        virtual void SetWaterCapFogAtVolumeDepth(bool waterCapFogAtVolumeDepth) = 0;
        /**
         * Gets whether or not the underwater fog effect is capped at the volume's depth.
         */
        virtual bool GetWaterCapFogAtVolumeDepth() = 0;

        /**
         * Sets the underwater fog density.
         * 
         * Specifies how dense the fog appears. Larger number will result in "thicker" fog.
         */
        virtual void SetWaterFogDensity(float waterFogDensity) = 0;
        /**
         * Gets the underwater fog density.
         */
        virtual float GetWaterFogDensity() = 0;

        /**
         * Sets the color of the underwater fog in the volume.
         */
        virtual void SetFogColor(AZ::Color fogColor) = 0;
        /**
         * Gets the color of the underwater fog in the volume.
         */
        virtual AZ::Color GetFogColor() = 0;

        /**
         * Sets whether or not the underwater fog in the volume will be affected by the sun.
         *
         * If set to true, fog color will be affected by the SunColor value set
         * in the Time Of Day editor.
         *
         * It's recommended to have this on for water volumes that are outside and
         * off for volumes that are inside.
         */
        virtual void SetFogColorAffectedBySun(bool fogColorAffectedBySun) = 0;
        /**
         * Gets whether or not sun should affect the color of underwater fog in the volume.
         */
        virtual bool GetFogColorAffectedBySun() = 0;

        /**
         * Sets the strength of shadows that fall on the water volume.
         *
         * A value of 0.0 will result in shadows having no effect on the water volume.
         * A value of 1.0 will result in a very dark shadow falling on the volume.
         *
         * Valid values range from 0.0 to 1.0. Values outside that range will be clamped.
         *
         * r_FogShadowsWater must be set to 1 for this parameter to have any effect.
        */
        virtual void SetWaterFogShadowing(float waterFogShadowing) = 0;
        /**
         * Gets the strength of shadows that fall on the water volume.
         */
        virtual float GetWaterFogShadowing() = 0;

        /**
         * Sets whether or not this water volume should produce caustics.
         *
         * Note that r_WaterVolumeCaustics must be set to 1 for any
         * water caustic settings to have any effect.
         *
         * Water volumes must be at a world height of at least 1 or else
         * caustics will not display.
         */
        virtual void SetWaterCaustics(bool waterCaustics) = 0;
        /**
         * Gets whether or not this water volume should produce caustics.
         */
        virtual bool GetWaterCaustics() = 0;

        /**
         * Sets the intensity of the water volume's caustics.
         *
         * This scales the normal map when rendering to the caustic map
         * which is what produces a stronger caustic effect.
         */
        virtual void SetWaterCausticIntensity(float waterCausticIntensity) = 0;
        /**
         * Gets the intensity of the water volume's caustics.
         */
        virtual float GetWaterCausticIntensity() = 0;

        /**
         * Sets how high the caustics can be cast above the water's surface.
         *
         * A value of 0.0 means that caustics will only be rendered beneath the water's surface.
         * Any value greater than 0 means that caustics can be cast on nearby surfaces that aren't
         * in the water volume.
         */
        virtual void SetWaterCausticHeight(float waterCausticHeight) = 0;
        /**
         * Gets how high the caustics can be cast above the water's surface.
         */
        virtual float GetWaterCausticHeight() = 0;

        /**
         * Sets a multiplier for the tiling of the water volume's caustics.
         *
         * This is multiplied to the tiling applied to the surface normals
         * which allows you to scale the caustic generation tiling separately
         * from the material.
         */
        virtual void SetWaterCausticTiling(float waterCausticTiling) = 0;
        /**
         * Gets the value multiplied to the surface's normal tiling during caustic generation.
         */
        virtual float GetWaterCausticTiling() = 0;

        /**
         * Sets whether to bind river with CryPhysics.
         */
        virtual void SetPhysicsEnabled(bool physicalize) = 0;
        /**
         * Gets whether the river is bound with CryPhysics.
         */
        virtual bool GetPhysicsEnabled() = 0;

        /**
         * Sets the water stream speed for physicalized rivers.
         *
         * Defines how fast physicalized objects will be moved along the river. Use negative values to move in the opposite direction.
         */
        virtual void SetWaterStreamSpeed(float waterStreamSpeed) = 0;
        /**
         * Gets the water stream speed for physicalized rivers.
         */
        virtual float GetWaterStreamSpeed() = 0;


        /**
         * Gets the plane representing the rendered water surface.
         */
        virtual AZ::Plane GetWaterSurfacePlane() = 0;
        /* There is no equivalent SetWaterSurfacePlane() because it's driven by the spline geometry. */

        /**
         * Triggers full rebuild of the river object, including geometry and render node generation.
         */
        virtual void Rebuild() = 0;
    };
    using RiverRequestBus = AZ::EBus<RiverRequests>;

    class RiverNotifications
        : public AZ::ComponentBus
    {
    public:
        /**
         * Notify listeners that the river depth has changed
         */
        virtual void OnWaterVolumeDepthChanged(float depth) {}

    };
    using RiverNotificationBus = AZ::EBus<RiverNotifications>;

    class RoadsAndRiversGeometryRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Sets the variable width along the river using the index of spline.
         */
        virtual void SetVariableWidth(AZ::u32 index, float width) = 0;

        /**
         * Gets the variable width along the river using the index of spline.
         */
        virtual float GetVariableWidth(AZ::u32 index) = 0;

        /**
         * Sets the unifrom global width along the road or river. It will be added to variable width specified by the spline width attribute.
         * If no width attribute specified the width of the spline geometry will be uniform and equal to this parameter.
         */
        virtual void SetGlobalWidth(float width) = 0;

        /**
         * Gets the unifrom global width along the road or river.
         */
        virtual float GetGlobalWidth() = 0;

        /**
         * Sets the tile length .
         * 
         * Tile length is texture mapping scale along the geometry spline.
         */
        virtual void SetTileLength(float tileLength) = 0;

        /**
         * Gets the tile length.
         */
        virtual float GetTileLength() = 0;

        /**
         * Sets the segment length.
         *
         * Segment length is the size of the polygon along the road or river in meters; 
         * The lower the number the more polygons are generated.
         */
        virtual void SetSegmentLength(float segmentLength) = 0;

        /**
         * Gets segment length
         */
        virtual float GetSegmentLength() = 0;

        virtual AZStd::vector<AZ::Vector3> GetQuadVertices() const = 0;
    };

    using RoadsAndRiversGeometryRequestsBus = AZ::EBus<RoadsAndRiversGeometryRequests>;

    class RoadsAndRiversGeometryNotifications
        : public AZ::ComponentBus
    {
    public:
        /**
         * Notify listeners that a width value has changed
         */
        virtual void OnWidthChanged() {}

        /**
         * Notify listeners that the tile length has changed
         */
        virtual void OnTileLengthChanged(float tileLength) {}

        /**
         * Notify listeners that the segment length has changed
         */
        virtual void OnSegmentLengthChanged(float segmentLength) {}
    };

    using RoadsAndRiversGeometryNotificationBus = AZ::EBus<RoadsAndRiversGeometryNotifications>;

} // namespace RoadsAndRivers
