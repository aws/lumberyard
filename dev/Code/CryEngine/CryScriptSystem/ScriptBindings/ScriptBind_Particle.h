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

#ifndef CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_PARTICLE_H
#define CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_PARTICLE_H
#pragma once


#include <IScriptSystem.h>

struct ISystem;
struct I3DEngine;
struct ParticleParams;
struct IParticleEffect;
struct CryEngineDecalInfo;

/*
    <description>This class implements script-functions for particles and decals.</description>
    <remarks>After initialization of the script-object it will be globally accessable through scripts using the namespace "Particle".</remarks>
    <example>Particle.CreateDecal(pos, normal, scale, lifetime, decal.texture, decal.object, rotation)</example>
*/
class CScriptBind_Particle
    : public CScriptableBase
{
public:
    CScriptBind_Particle(IScriptSystem* pScriptSystem, ISystem* pSystem);
    virtual ~CScriptBind_Particle();
    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    //! <code>Particle.CreateEffect( name, params )</code>
    //!     <param name="name">Particle effect name.</param>
    //!     <param name="params">Effect parameters.</param>
    //! <description>Creates a new particle effect.</description>
    int CreateEffect(IFunctionHandler* pH, const char* name, SmartScriptTable params);

    //! <code>Particle.DeleteEffect( name )</code>
    //!     <param name="name">Particle effect name.</param>
    //! <description>Deletes the specified particle effect.</description>
    int DeleteEffect(IFunctionHandler* pH, const char* name);

    //! <code>Particle.IsEffectAvailable( name )</code>
    //!     <param name="name">Particle effect name.</param>
    //! <description>Checks if the specified particle effect is available.</description>
    int IsEffectAvailable(IFunctionHandler* pH, const char* name);

    //! <code>Particle.SpawnEffect( effectName, pos, dir )</code>
    //!     <param name="effectName">Effect name.</param>
    //!     <param name="pos">Position vector.</param>
    //!     <param name="dir">Direction vector.</param>
    //! <description>Spawns an effect.</description>
    int SpawnEffect(IFunctionHandler* pH, const char* effectName, Vec3 pos, Vec3 dir);

    //! <code>Particle.SpawnEffectLine( effectName, startPos, endPos, dir, scale, slices )</code>
    //!     <param name="effectName">Effect name.</param>
    //!     <param name="startPos">Start position.</param>
    //!     <param name="endPos">End position.</param>
    //!     <param name="dir">Direction of the effect.</param>
    //!     <param name="scale">Scale value for the effect.</param>
    //!     <param name="slices">Number of slices.</param>
    //! <description>Spawns an effect line.</description>
    int SpawnEffectLine(IFunctionHandler* pH, const char* effectName, Vec3 startPos, Vec3 endPos, Vec3 dir, float scale, int slices);

    //! <code>Particle.SpawnParticles( params, pos, dir )</code>
    //!     <param name="params">Effect parameters.</param>
    //!     <param name="pos">Effect position.</param>
    //!     <param name="dir">Effect direction.</param>
    //! <description>Spawns a particle effect.</description>
    int SpawnParticles(IFunctionHandler* pH, SmartScriptTable params, Vec3 pos, Vec3 dir);

    //! <code>Particle.CreateDecal( pos, normal, size, lifeTime, textureName )</code>
    //!     <param name="pos">Decal position.</param>
    //!     <param name="normal">Decal normal vector.</param>
    //!     <param name="size">Decal size.</param>
    //!     <param name="lifeTime">Decal life time.</param>
    //!     <param name="textureName - Name of the texture.</param>
    //! <description>Creates a decal with the specified parameters.</description>
    int CreateDecal(IFunctionHandler* pH, Vec3 pos, Vec3 normal, float size, float lifeTime, const char* textureName);

    //! <code>Particle.CreateMatDecal( pos, normal, size, lifeTime, materialName )</code>
    //!     <param name="pos">Decal position.</param>
    //!     <param name="normal">Decal normal vector.</param>
    //!     <param name="size">Decal size.</param>
    //!     <param name="lifeTime">Decal life time.</param>
    //!     <param name="materialName">Name of the Material.</param>
    //! <description>Creates a material decal.</description>
    int CreateMatDecal(IFunctionHandler* pH, Vec3 pos, Vec3 normal, float size, float lifeTime, const char* materialName);

    //! <code>Particle.Attach()</code>
    //! <description>Attaches an effect.</description>
    int Attach(IFunctionHandler* pH);

    //! <code>Particle.Detach()</code>
    //! <description>Detaches an effect.</description>
    int Detach(IFunctionHandler* pH);

private:
    void ReadParams(SmartScriptTable& table, ParticleParams* params, IParticleEffect* pEffect);
    void CreateDecalInternal(IFunctionHandler* pH, const Vec3& pos, const Vec3& normal, float size, float lifeTime, const char* name, bool nameIsMaterial);

    ISystem* m_pSystem;
    I3DEngine* m_p3DEngine;
};

#endif // CRYINCLUDE_CRYSCRIPTSYSTEM_SCRIPTBINDINGS_SCRIPTBIND_PARTICLE_H
