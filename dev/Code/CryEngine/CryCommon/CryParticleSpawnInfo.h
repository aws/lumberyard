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

// Description : declaration of struct CryParticleSpawnInfo.

// Notes:
//    3D Engine and Character Animation subsystems (as well as perhaps
//    some others) transfer data about the particles that need to be spawned
//    via this structure. This is to avoid passing many parameters through
//    each function call, and to save on copying these parameters when just
//    simply passing the structure from one function to another.



#ifndef CRYINCLUDE_CRYCOMMON_CRYPARTICLESPAWNINFO_H
#define CRYINCLUDE_CRYCOMMON_CRYPARTICLESPAWNINFO_H
#pragma once


//! Structure containing common parameters describing particle spawning
//! This is in addition to the ParticleParams defined in the 3D Engine
struct CryParticleSpawnInfo
{
public:
    // Particle spawn rate: the number of particles to be spawned
    // PER SECOND. The number of particles to spawn per Frame is:
    //   fSpawnRate * fFrameDurationSeconds
    float fSpawnRate;
    /*
    // particle spawn mode
    enum SpawnModeEnum
    {
        // rain mode: particles (rain drops) should reflect from the normals
        // vDir sets the rain direction
        SPAWN_RAIN = 0,
        // particles are spawned along normal direction
        SPAWN_NORMAL,
        // particles are spawned randomly into the normal hemisphere (randomly, but outside the skin)
        SPAWN_OUTSIDE,
        // particles are spawned into the direction of vDir
        SPAWN_CONST,
        // particles are spawned absolutely randomly
        SPAWN_RANDOM
    };

    SpawnModeEnum nSpawnMode;

    // particle direction
    Vec3 vDir;
    */

    enum FlagEnum
    {
        // in this mode, only up-looking normals will be taken into account
        FLAGS_RAIN_MODE      = 1,
        // with this flag, the spawn will occur only on one frame
        FLAGS_ONE_TIME_SPAWN = 1 << 1,
        // with this flag, nBoneId will determine the bone from which the particle will be spawned,
        // vBonePos will be the position in bone coordinates at which the particles will be spawned
        FLAGS_SPAWN_FROM_BONE = 1 << 2
    };

    // flags - combination of FlagEnum flags
    unsigned nFlags;
    // valid only with FLAGS_SPAWN_FROM_BONE:
    // the bone number, can be determined with the ICryCharModel::GetBoneByName(), should be non-negative
    unsigned nBone;
    // valid only with FLAGS_SPAWN_FROM_BONE:
    // the position of the particle in local bone coordinates
    Vec3 vBonePos;

    CryParticleSpawnInfo()
        : fSpawnRate (0)
        , nFlags (0)
        , nBone (0)
        , vBonePos(0, 0, 0)
    {
    }

    CryParticleSpawnInfo (float _fSpawnRate, unsigned _nFlags = 0)
        : fSpawnRate(_fSpawnRate)
        , nFlags (_nFlags)
    {
    }
};

#endif // CRYINCLUDE_CRYCOMMON_CRYPARTICLESPAWNINFO_H
