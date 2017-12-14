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

//Forward declarations
namespace AZ 
{
    class Vector3;
}

enum class EngineSpec : AZ::u32;

namespace Water
{
    class EditorWaterVolumeComponentRequests
        : public AZ::ComponentBus
    {
    public:
        // EBus Traits overrides (Configuring this EBus)
        // Using Defaults

        /**
         * Sets the min spec for the water volume
         *
         * The water volume won't render if the min spec is above 
         * the engine's current spec.
         */
        virtual void SetMinSpec(EngineSpec minSpec) = 0;
        ///Gets the min spec that the water volume will render at
        virtual EngineSpec GetEngineSpec() = 0;

        /**
         * Sets whether or not the water volume should render as a filled volume in the editor
         *
         * @param displayFilled Set to true to draw the water volume's area as a filled volume.
         */
        virtual void SetDisplayFilled(bool displayFilled) = 0;
        ///Gets whether or not the water volume is being displayed as a filled volume in the editor
        virtual bool GetDisplayFilled() = 0;

        /**
         * Sets how far from the current camera the water volume will be rendered
         *
         * This is a multiplier to the base view distance value defined by e_MaxViewDistance.
         */
        virtual void SetViewDistanceMultiplier(float viewDistanceMultiplier) = 0;
        /**
         * Gets how far from the current camera the water volume will be rendered
         */
        virtual float GetViewDistanceMultiplier() = 0;
    };

    using EditorWaterVolumeComponentRequestBus = AZ::EBus<EditorWaterVolumeComponentRequests>;

    class WaterVolumeComponentRequests
        : public AZ::ComponentBus
    {
    public:
        // EBus Traits overrides (Configuring this EBus)
        // Using Defaults

        /**
         * Sets how much the surface texture is tiled horizontally.
         *
         * The surface texture is often defined as a bump/normal map.
         * "Horizontally" refers to the U axis of the UV map.
         */
        virtual void SetSurfaceUScale(float surfaceUScale) = 0;
        /**
         * Gets how much the surface texture is tiled horizontally.
         */
        virtual float GetSurfaceUScale() = 0;
        /**
         * Sets how much the surface texture is tiled vertically.
         *
         * The surface texture is often defined as a bump/normal map.
         * "Vertically" refers to the V axis of the UV map.
         */
        virtual void SetSurfaceVScale(float surfaceVScale) = 0;
        /**
         * Gets how much the surface texture is tiled vertically.
         */
        virtual float GetSurfaceVScale() = 0;

        /**
         * Sets the density of the underwater fog in the volume.
         *
         * A smaller value results in clearer water. A higher
         * value produces a more murky effect.
         */
        virtual void SetFogDensity(float fogDensity) = 0;
        /**
         * Gets the density of the fog in the water volume.
         */
        virtual float GetFogDensity() = 0;
        /**
         * Sets the color of the underwater fog in the volume.
         *
         * This color will be modified by the FogColorMultiplier parameter before
         * being rendered.
         */
        virtual void SetFogColor(AZ::Vector3 fogColor) = 0;
        /**
         * Gets the color of the underwater fog in the volume.
         *
         * The returned value is unaffected by the FogColorMultiplier parameter.
         */
        virtual AZ::Vector3 GetFogColor() = 0;
        /**
         * Sets the value that will be multiplied to the FogColor parameter before rendering.
         *
         * A higher value will result in brighter underwater fog.
         */
        virtual void SetFogColorMultiplier(float fogColorMultiplier) = 0;
        /**
         * Gets the value that gets multiplied to the FogColor parameter before rendering.
         */
        virtual float GetFogColorMultiplier() = 0;
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
        virtual void SetFogShadowing(float fogShadowing) = 0;
        /**
         * Gets the strength of shadows that fall on the water volume.
         *
         * While valid values are only in the range of 0.0 to 1.0 this will
         * return whatever the FogShadowing parameter is regardless of whether or 
         * not it is in the valid range.
         */
        virtual float GetFogShadowing() = 0;
        /**
         * Sets whether or not the underwater fog effect is capped at the volume's depth.
         *
         * If set to false underwater fog will continue to render no matter how far beneath 
         * the water volume the camera is.
         */
        virtual void SetCapFogAtVolumeDepth(bool capFogAtVolumeDepth) = 0;
        /**
         * Gets whether or not the underwater fog effect is capped at the volume's depth.
         */
        virtual bool GetCapFogAtVolumeDepth() = 0;

        /**
         * Sets whether or not this water volume should produce caustics.
         *
         * Note that r_WaterVolumeCaustics must be set to 1 for any
         * water caustic settings to have any effect.
         *
         * Water volumes must be at a world height of at least 1 or else 
         * caustics will not display. 
         */
        virtual void SetCausticsEnabled(bool causticsEnabled) = 0;
        /**
         * Gets whether or not this water volume should produce caustics.
         */
        virtual bool GetCausticsEnabled() = 0;
        /**
         * Sets the intensity of the water volume's caustics.
         *
         * This scales the normal map when rendering to the caustic map
         * which is what produces a stronger caustic effect.
         */
        virtual void SetCausticIntensity(float causticIntensity) = 0;
        /**
         * Gets the intensity of the water volume's caustics.
         */
        virtual float GetCausticIntensity() = 0;
        /**
         * Sets a multiplier for the tiling of the water volume's caustics.
         *
         * This is multiplied to the tiling applied to the surface normals
         * which allows you to scale the caustic generation tiling separately 
         * from the material.
         */
        virtual void SetCausticTiling(float causticTiling) = 0;
        /**
         * Gets the value multiplied to the surface's normal tiling during caustic generation.
         */
        virtual float GetCausticTiling() = 0;
        /**
         * Sets how high the caustics can be cast above the water's surface.
         * 
         * A value of 0.0 means that caustics will only be rendered beneath the water's surface.
         * Any value greater than 0 means that caustics can be cast on nearby surfaces that aren't 
         * in the water volume.
         */
        virtual void SetCausticHeight(float causticHeight) = 0;
        /**
         * Gets how high the caustics can be cast above the water's surface.
         */
        virtual float GetCausticHeight() = 0;

        /**
         * Sets how much volume is able to "spill" into a container.
         *
         * If this value is greater than 0, the water volume will try to raycast
         * down to nearby geometry and try to spill into it. That geometry does
         * not need any sort of special collider or rigidbody, just a concave mesh
         * on a mesh component.
         * Volumes cannot spill onto terrain. 
         *
         * This value controls how much water will spill. A higher value means
         * more of the container volume will be filled with water. If any valid 
         * container is found, this spilled water is all that will exist.
         *
         * The depth of the water volume does not affect any of this.
         *
         * See CTriMesh::FloodFill in trimesh.cpp
         */
        virtual void SetSpillableVolume(float spillableVolume) = 0;
        /**
         * Gets how much volume is able to "spill" into a container.
         */
        virtual float GetSpillableVolume() = 0;

        /**
         * Sets how accurate the water level of the spilled water volume is.
         *
         * Water level of a spilled volume will be iteratively calculated until it is within this
         * distance to the water level expected by the water volume. The higher this value,
         * the more iterations are used to calculate the water level. If this
         * is set to 0.0 the water level iterations will hit a hard-coded limit. 
         *
         * See CTriMesh::FloodFill in trimesh.cpp
         */
        virtual void SetVolumeAccuracy(float volumeAccuracy) = 0;
        /**
         * Gets how accurate the water level of the spilled water volume is.
         */
        virtual float GetVolumeAccuracy() = 0;

        /**
         * Sets how much to increase the border of the water volume when spilled.
         *
         * This is useful when wave simulation is happening as if this value is 0
         * then the wave height can reveal open edges of the volume. 
         *
         * See CTriMesh::FloodFill in trimesh.cpp
         */
        virtual void SetExtrudeBorder(float extrudeBorder) = 0;
        /**
         * Gets how much the water volume's border is extruded by when spilled.
         */
        virtual float GetExtrudeBorder() = 0;

        /**
         * Sets whether or not this water volume should look for a convex border when spilling.
         *
         * This is useful when the container that the volume can spill into 
         * has multiple contours. Areas like water volumes don't support multiple
         * contours so checking this tells the container filling logic to 
         * look to break up this one volume into multiple volumes that fit the 
         * contoured container. 
         *
         * See CTriMesh::FloodFill in trimesh.cpp
         */
        virtual void SetConvexBorder(bool convexBorder) = 0;
        /**
         * Sets whether or not this water volume should look for a convex border when spilling.
         */
        virtual bool GetConvexBorder() = 0;

        /**
         * Sets the minimum volume that an object must have to take place in volume displacement.
         *
         * Any object with a volume smaller than this value will not deform the surface.
         */
        virtual void SetObjectSizeLimit(float objectSizeLimit) = 0;
        /**
         * Gets the minimum volume that an object must have to take place in volume displacement.
         */
        virtual float GetObjectSizeLimit() = 0;

        /**
         * Sets how large each wave simulation cell is.
         *
         * The number of wave sim cells is determined by the size of the volume and
         * how large each of them is. This in turn affects the water surface mesh.
         *
         * A large water volume with a small cell size results in a large number of cells
         * and a very large, complicated surface mesh. 
         *
         * A small water volume with a large cell size results in a very low number of cells 
         * and a very small, simple surface mesh. 
         *
         * A large surface mesh will be very hard to evaluate wave simulation on.
         * A dense surface mesh will potentially have an unstable wave simulation unless you 
         * tune other parameters. 
         *
         * Use the cvar p_draw_helpers a_g to see the tessellated surface mesh. You may need to bring the 
         * editor viewport's camera closer to the volume for the mesh to render.
         */
        virtual void SetWaveSurfaceCellSize(float waveSurfaceCellSize) = 0;
        /**
         * Gets how large each wave simulation cell is.
         */
        virtual float GetWaveSurfaceCellSize() = 0;

        /**
         * Sets how fast waves move.
         */
        virtual void SetWaveSpeed(float waveSpeed) = 0;
        /**
         * Gets how fast waves move.
         */
        virtual float GetWaveSpeed() = 0;

        /**
         * Sets how much dampening to apply during wave simulation.
         *
         * A higher value results in waves losing velocity more quickly.
         */
        virtual void SetWaveDampening(float waveDampening) = 0;
        /**
         * Gets how much dampening that is applied during wave simulation.
         */
        virtual float GetWaveDampening() = 0;

        /**
         * Sets how often the wave simulation ticks.
         *
         * A lower value means a more frequent simulation tick. 
         * A more frequent tick will result in the simulation resolving more quickly.
         */
        virtual void SetWaveTimestep(float waveTimestep) = 0;
        /**
         * Gets how often the wave simulation ticks.
         */
        virtual float GetWaveTimestep() = 0;

        /**
         * Sets the minimum velocity needed for a cell to sleep.
         *
         * When a cell's velocity reaches this threshold it will
         * be considered at rest and no longer apply force to its neighbors.
         */
        virtual void SetWaveSleepThreshold(float waveSleepThreshold) = 0;
        /**
         * Gets the minimum velocity needed for a cell to sleep.
         */
        virtual float GetWaveSleepThreshold() = 0;

        /**
         * Sets the size of wave simulation depth cells.
         *
         * This is very similar to SetWaveSurfaceCellSize and has many the same caveats.
         * However this does not directly affect surface mesh generation. It still
         * does carry implications when discussing performance of the wave simulation.
         * A smaller cell size and thus more depth cells results in a more intensive 
         * wave simulation.
         */
        virtual void SetWaveDepthCellSize(float waveDepthCellSize) = 0;
        /**
         * Gets the size of wave simulation depth cells.
         */
        virtual float GetWaveDepthCellSize() = 0;

        /**
         * Sets the height limit on generated waves.
         *
         * This is both a height and depth limit. With a height limit of 3 
         * a cell can be deformed both up 3 units and down 3 units. 
         */
        virtual void SetWaveHeightLimit(float waveHeightLimit) = 0;
        /**
         * Gets the height limit on generated waves.
         */
        virtual float GetWaveHeightLimit() = 0;

        /**
         * Sets how much force wave simulation cells will apply to each other.
         *
         * A low wave force indicates a high amount of resistance between cells.
         * The higher this value is, the less resistance between cells and the more
         * energy is transmitted between them. More transmitted energy results in larger, 
         * more pronounced waves. 
         */
        virtual void SetWaveForce(float waveForce) = 0;
        /**
         * Gets how much force wave simulation cells will apply to each other.
         */
        virtual float GetWaveForce() = 0;

        /**
         * Sets an additional size to add to the simulation area.
         *
         * If the water volume simulation moves or grows outside the initial area 
         * it needs to know where it can move. Increase this variable to add 
         * additional size to this simulation area.
         *
         * The cvar p_draw_helpers a_gj will draw the wave simulation area as a checkerboard-like height-field.
         */
        virtual void SetWaveSimulationAreaGrowth(float waveSimulationAreaGrowth) = 0;
        /**
         * Gets an additional size that's added to the simulation area.
         */
        virtual float GetWaveSimulationAreaGrowth() = 0;
    };

    using WaterVolumeComponentRequestBus = AZ::EBus<WaterVolumeComponentRequests>;

} // namespace Water