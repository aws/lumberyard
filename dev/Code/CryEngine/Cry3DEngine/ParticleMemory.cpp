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

// Description : All the particle system's specific memory needs


#include "StdAfx.h"
#include "ParticleMemory.h"
#include "Particle.h"
#include "Cry3DEngineBase.h"

bool g_bParticleObjectPoolInitialized = false;
_MS_ALIGN(128) char sStorageParticleObjectPool[sizeof(ParticleObjectPool)] _ALIGN(128);
CryCriticalSection g_ParticlePoolInitLock;

///////////////////////////////////////////////////////////////////////////////
ParticleObjectPool& ParticleObjectAllocator()
{
    IF (g_bParticleObjectPoolInitialized == false, 0)
    {
        AUTO_LOCK(g_ParticlePoolInitLock);
        IF (g_bParticleObjectPoolInitialized == false, 0)
        {
            new(sStorageParticleObjectPool) ParticleObjectPool();
            alias_cast<ParticleObjectPool*>(sStorageParticleObjectPool)->Init(Cry3DEngineBase::GetCVars()->e_ParticlesPoolSize << 10);
            g_bParticleObjectPoolInitialized = true;
        }
    }

    return *alias_cast<ParticleObjectPool*>(sStorageParticleObjectPool);
}
