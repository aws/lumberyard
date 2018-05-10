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


namespace CloudsGem
{
    /// Controls what parts of the cloud is regenerated whe Generate is called.
    enum class GenerationFlag : uint32
    {
        GF_ALL = 0,         ///< Used when regenerating all attributes
        GF_ROW,             ///< Used to signal that the selected row of the atlas texture has changed
        GF_TEXTURE,         ///< Used to signal that texture coordinates should be regenerated
        GF_SPRITE_COUNT,    ///< Used to signal an increase or decrease the number of particles in the cloud
        GF_DENSITY,         ///< Used to signal a change int the density of the cloud during voxelization
        GF_MIN_DISTANCE,    ///< Used to signal the min distance between particles
        GF_SIZE,            ///< Used to signal a change in the number size of a particle
        GF_SIZE_VAR,        ///< Used to signal a change in the number size variance of particles
        GF_SEED,            ///< Used to seed rng
    };

    class CloudComponentBehaviorRequests
        : public AZ::ComponentBus 
    {
    public:
        /**
         * Gets the state of the auto movement property. If the auto move property is enabled then the 
         * cloud will move inside of its loop box automatically. The bounds of the loopbox will constrain movement.
         * If automove is disabled then the cloud can only be moved by manipulation of the entity transforms
         * @return returns true if automatic movement is enabled, false otherwise
         */
        virtual bool GetAutoMove() = 0;
        
        /**
         * Sets the state of the auto movement property. If the auto move property is enabled then the
         * cloud will move inside of its loop box automatically. The bounds of the loopbox will constrain movement.
         * If automove is disabled then the cloud can only be moved by manipulation of the entity transforms
         * @param state state of the automove property. 
         */
        virtual void SetAutoMove(bool state) = 0;

        /**
         * Gets the velocity of cloud movement. The velocity controls the speed and direction of the cloud.
         * This value only has meaning when the cloud's auto move property is enabled.
         * @return returns velocity of cloud
         */
        virtual AZ::Vector3 GetVelocity() = 0;

        /**
         * Sets the velocity of cloud movement. The velocity controls the speed and direction of the cloud.
         * This value only has meaning when the cloud's auto move property is enabled.
         * @param velocity new velocity
         */
        virtual void SetVelocity(AZ::Vector3 velocity) = 0;
        
        /**
         * Gets the distance from origin that the cloud should start to fade out prior to reaching the edge
         * of the loop box. This prevents the cloud of popping in and out as it approaches the edge of the loop box.
         * @return returns distance from center to edge of box where fading should begin
         */
        virtual float GetFadeDistance() = 0;

        /**
         * Sets the distance from origin that the cloud should start to fade out prior to reaching the edge
         * of the loop box. This prevents the cloud of popping in and out as it approaches the edge of the loop box.
         * @param distance distance in meters from edge of loop box
         */
        virtual void SetFadeDistance(float distance) = 0;

        /**
         * Determines if volumetric rendering is enabled for this cloud component
         * @return returns true if volumetric rendering is enabled, false otherwise
         */
        virtual bool GetVolumetricRenderingEnabled() = 0;
        
        /**
         * Sets the state of volumetric rendering. Volumetric rendering increases cloud quality and realism at the cost of performance
         * @param velocity new velocity
         */
        virtual void SetVolumetricRenderingEnabled(bool enabled) = 0;

        /**
         * Gets the density of this volumetric cloud. This value controls how solid or translucent the cloud volume appears
         * @return returns volumetric cloud density. This is only relevent when using the volumetric renderer
         */
        virtual float GetVolumetricDensity() = 0;
        
        /**
         * Sets the density of this volumetric cloud which controls how solid or translucent the cloud volume appears
         * @param density value in the range 0.0 - 1.0f that controls cloud density
         */
        virtual void SetVolumetricDensity(float density) = 0;
    };
    using CloudComponentBehaviorRequestBus = AZ::EBus<CloudComponentBehaviorRequests>;

    class EditorCloudComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual void Generate(GenerationFlag flag) = 0;
        virtual void Refresh() = 0;
        virtual void OnMaterialAssetChanged() = 0;
    };
    using EditorCloudComponentRequestBus = AZ::EBus<EditorCloudComponentRequests>;

}