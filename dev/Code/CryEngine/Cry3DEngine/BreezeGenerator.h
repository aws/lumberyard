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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRY3DENGINE_BREEZEGENERATOR_H
#define CRYINCLUDE_CRY3DENGINE_BREEZEGENERATOR_H
#pragma once

#include "Cry3DEngineBase.h"

struct SBreeze;

// Spawns wind volumes around the camera to emulate breezes
class CBreezeGenerator
    : public Cry3DEngineBase
{
    friend class C3DEngine;

    // The array of active breezes
    SBreeze* m_breezes;

    // The radius around the camera where the breezes will be spawned
    float m_spawn_radius;

    // The spread (variation in direction on spawn)
    float m_spread;

    // The max. number of wind areas active at the same time
    uint32 m_count;

    // The max. extents of each breeze
    float m_radius;

    // The max. life of each breeze
    float m_lifetime;

    // The random variance of each breeze in respect to it's other attributes
    float m_variance;

    // The strength of the breeze (as a factor of the original wind vector)
    float m_strength;

    // The speed of the breeze movement (not coupled to the wind speed)
    float m_movement_speed;

    // The global direction of the environment wind
    Vec3 m_wind_speed;

    // Set a fixed height for the breeze, for levels without terrain. -1 uses the terrain height
    float m_fixed_height;

    // breeze generation enabled?
    bool m_enabled;

public:

    CBreezeGenerator();
    ~CBreezeGenerator();

    void Initialize();

    void Reset();

    void Shutdown();

    void Update();
};


#endif // CRYINCLUDE_CRY3DENGINE_BREEZEGENERATOR_H
