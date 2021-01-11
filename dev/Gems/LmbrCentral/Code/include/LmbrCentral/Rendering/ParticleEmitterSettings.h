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

#include "ParticleAsset.h"
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/string/string.h>

#include <AzCore/Math/Color.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    /*!
    *  Particle emitter settings, these are used by both the in-game and editor components.
    */
    class ParticleEmitterSettings
    {
    public:

        AZ_TYPE_INFO(ParticleEmitterSettings, "{A1E34557-30DB-4716-B4CE-39D52A113D0C}")

        ParticleEmitterSettings()
            : m_visible(true)
            , m_enable(true)
            , m_prime(false)
            , m_color(1.0f, 1.0f, 1.0f, 1.0f)
            , m_countScale(1.f)
            , m_timeScale(1.f)
            , m_speedScale(1.f)
            , m_sizeScale(1.f)
            , m_pulsePeriod(0.f)
            , m_particleSizeScaleX(1.0f)
            , m_particleSizeScaleY(1.0f)
            , m_particleSizeScaleZ(1.0f)
            , m_particleSizeScaleRandom(0.0f)
            , m_strength(-1.f)
            , m_ignoreRotation(false)
            , m_notAttached(false)
            , m_registerByBBox(false)
            , m_useLOD(true)
            , m_enableAudio(false)
            , m_viewDistMultiplier(1.0f)
            , m_useVisAreas(true)
        {}

        AZ::Data::Asset<ParticleAsset> m_asset;                 //!< Used at asset compile time to track the dependency. Not used at edit or runtime.
        bool                        m_visible;                  //!< Controls the emitter's visibility.
        bool                        m_enable;                   //!< Controls whether emitter is active.
        AZStd::string               m_selectedEmitter;          //!< Name of the particle emitter to use.
        AZ::EntityId                m_targetEntity;             //!< Target Entity to be used for emitters with Target Attraction or similar features enabled.
        bool                        m_prime;                    //!< "Pre-Roll". Set emitter as though it's been running indefinitely.
        AZ::Color                   m_color;                    //!< An additional tint color
        float                       m_countScale;               //!< Multiple for particle count (on top of m_countPerUnit if set).
        float                       m_timeScale;                //!< Multiple for emitter time evolution.
        float                       m_speedScale;               //!< Multiple for particle emission speed.
        float                       m_pulsePeriod;              //!< How often to restart emitter.
        float                       m_particleSizeScaleX;       //!< Scaling on particle size X
        float                       m_particleSizeScaleY;       //!< Scaling on particle size Y
        float                       m_particleSizeScaleZ;       //!< Scaling on particle size Z; only for geometry particle
        float                       m_particleSizeScaleRandom;  //!< Random Scaling on particle size scale
        float                       m_strength;                 //!< Controls the affect of Strength Over Emitter lifetime curve, negative value applies strength over emitter lifetime curve.\
                                                                //zero removes the affect of the curve. positive value applies the affect uniformly over emitter lifetime
        float                       m_sizeScale;                //!< Multiple for all effect sizes.
        bool                        m_ignoreRotation;           //!< The entity rotation is ignored.
        bool                        m_notAttached;              //!< The entity's position is ignore (i.e. emitter does not follow its entity).
        bool                        m_registerByBBox;           //!< Use the Bounding Box instead of Position to Register in VisArea
        bool                        m_useLOD;                   //!< Activates LOD if they exist on emitter
        bool                        m_enableAudio;              //!< Used by particle effect instances to indicate whether audio should be updated or not.
        AZStd::string               m_audioRTPC;                //!< Indicates what audio RTPC this particle effect instance drives.

        //! For IRenderNode
        float                       m_viewDistMultiplier;       
        bool                        m_useVisAreas;              //!< Indicates whether it allows VisAreas to control this component's visibility
                
        static void Reflect(AZ::ReflectContext* context);

        const static float MaxCountScale;
        const static float MaxTimeScale;
        const static float MaxSpeedScale;
        const static float MaxSizeScale;
        const static float MaxLifttimeStrength;
        const static float MinLifttimeStrength;
        const static float MaxRandSizeScale;
        
    private:

        static bool VersionConverter(AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement);
    };
} // namespace LmbrCentral
