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

#ifndef CRYINCLUDE_CRY3DENGINE_PARTICLEEFFECT_H
#define CRYINCLUDE_CRY3DENGINE_PARTICLEEFFECT_H
#pragma once

#include "IParticles.h"
#include "Cry3DEngineBase.h"
#include "ParticleParams.h"
#include "ParticleUtils.h"
#include "CryPtrArray.h"
#include "Options.h"
#include "ICryMiniGUI.h"

#if !defined(CONSOLE)
#define PARTICLE_EDITOR_FUNCTIONS
#endif

//////////////////////////////////////////////////////////////////////////
// Set of flags that can have 3 states: forced on, forced off, or neutral.
template<class T>
struct TrinaryFlags
{
    T       On, Off;

    TrinaryFlags()
        : On(0)
        , Off(0) {}

    void Clear()
    {
        On = Off = 0;
    }

    // Set from an int where -1 is off, 0 is neutral, 1 is forced
    void SetState(int state, T flags)
    {
        On &= ~flags;
        Off &= ~flags;
        if (state < 0)
        {
            Off |= flags;
        }
        else if (state > 0)
        {
            On |= flags;
        }
    }

    TrinaryFlags& operator |= (T flags)
    {
        On |= flags; return *this;
    }
    TrinaryFlags& operator &= (T flags)
    {
        Off |= ~flags; return *this;
    }

    T operator & (T flags) const
    {
        return (flags | On) & ~Off;
    }

    T operator & (TrinaryFlags<T> const& other) const
    {
        return (On | other.On) & ~(Off | other.Off);
    }
};

//////////////////////////////////////////////////////////////////////////
// Requirements and attributes of particle effects
enum EEffectFlags
{
    EFF_LOADED = BIT(0),

    // Environmental requirements.
    ENV_GRAVITY = BIT(1),
    ENV_WIND = BIT(2),
    ENV_WATER = BIT(3),

    ENV_PHYS_AREA = ENV_GRAVITY | ENV_WIND | ENV_WATER,

    // Collision targets.
    ENV_TERRAIN = BIT(4),
    ENV_STATIC_ENT = BIT(5),
    ENV_DYNAMIC_ENT = BIT(6),
    ENV_COLLIDE_INFO = BIT(7),

    ENV_COLLIDE_ANY = ENV_TERRAIN | ENV_STATIC_ENT | ENV_DYNAMIC_ENT,
    ENV_COLLIDE_PHYSICS = ENV_STATIC_ENT | ENV_DYNAMIC_ENT,
    ENV_COLLIDE_CACHE = ENV_TERRAIN | ENV_STATIC_ENT,

    // Additional effects.
    EFF_AUDIO = BIT(8),       // Ensures execution of audio relevant code.
    EFF_FORCE = BIT(9),       // Produces physical force.

    EFF_ANY = EFF_AUDIO | EFF_FORCE,

    // Exclusive rendering methods.
    REN_SPRITE = BIT(10),
    REN_GEOMETRY = BIT(11),
    REN_DECAL = BIT(12),
    REN_LIGHTS = BIT(13),      // Adds light to scene.

    REN_ANY = REN_SPRITE | REN_GEOMETRY | REN_DECAL | REN_LIGHTS,

    // Visual effects of particles
    REN_CAST_SHADOWS = BIT(14),
    REN_BIND_CAMERA = BIT(15),
    REN_SORT = BIT(16),

    // General functionality
    EFF_DYNAMIC_BOUNDS = BIT(17),  // Requires updating particles every frame.
};

struct STileInfo
{
    Vec2        vSize;                                      // Size of texture tile UVs.
    float       fTileCount, fTileStart;     // Range of tiles in texture for emitter.
};

struct SPhysForces
{
    Vec3    vAccel;
    Vec3    vWind;
    Plane   plWater;

    SPhysForces()
    {}

    SPhysForces(type_zero)
        : vAccel(ZERO)
        , vWind(ZERO)
        , plWater(Vec3(0, 0, 1), -WATER_LEVEL_UNKNOWN)
    {}

    void Add(SPhysForces const& other, uint32 nEnvFlags)
    {
        if (nEnvFlags & ENV_GRAVITY)
        {
            vAccel = other.vAccel;
        }
        if (nEnvFlags & ENV_WIND)
        {
            vWind += other.vWind;
        }
        if (nEnvFlags & ENV_WATER)
        {
            if (other.plWater.d < plWater.d)
            {
                plWater = other.plWater;
            }
        }
    }
};

inline bool HasWater(const Plane& pl)
{
    return pl.d < -WATER_LEVEL_UNKNOWN;
}

// Helper structures for emitter functions

struct SForceParams
    : SPhysForces
{
    float       fDrag;
    float       fStretch;
};

// Options for computing static bounds
struct FStaticBounds
{
    Vec3            vSpawnSize;
    float           fAngMax;
    float           fSpeedScale;
    bool            bWithSize;
    float           fMaxLife;

    FStaticBounds()
        : vSpawnSize(0)
        , fAngMax(0)
        , fSpeedScale(1)
        , bWithSize(true)
        , fMaxLife(fHUGE) {}
};

struct FEmitterRandom
{
    static type_min RMin() { return VMIN; }
    static type_max RMax() { return VMAX; }
    static type_min EMin() { return VMIN; }
    static type_max EMax() { return VMAX; }
};

struct FEmitterFixed
{
    CChaosKey       ChaosKey;
    float               fStrength;

    CChaosKey RMin() const { return ChaosKey; }
    CChaosKey RMax() const { return ChaosKey; }
    float EMin() const { return fStrength; }
    float EMax() const { return fStrength; }
};

struct SEmitParams;


//
// Additional runtime parameters.
//
struct ResourceParticleParams
    : ParticleParams
    , Cry3DEngineBase
    , ZeroInit<ResourceParticleParams>
{
    // Texture, material, geometry params.
    float                                   fTexAspect;                             // H/V aspect ratio.
    int16                                   nTexId;                                     // Texture id for sprite.
    int16                                   nNormalTexId;                               // Texture id for normal.
    int16                                   nGlowTexId;                                 // Texture id for glow.
    uint16                              mConfigSpecMask;                    // Allowed config specs.
    _smart_ptr<IMaterial>   pMaterial;                              // Used to override the material
    _smart_ptr<IStatObj>    pStatObj;                                   // If it isn't set to 0, this object will be used instead of a sprite
    uint32                              nEnvFlags;                              // Summary of environment info needed for effect.
    TrinaryFlags<uint32>    nRenObjFlags;                           // Flags for renderer, combining FOB_ and OS_.

                                                                    //Confetti - Chris Hekman
                                                                    //Parameters for trail fading

    ResourceParticleParams()
    {
        ComputeEnvironmentFlags();
    }

    ~ResourceParticleParams()
    {
    }

    explicit ResourceParticleParams(const ParticleParams& params)
        : ParticleParams(params)
    {
        ComputeEnvironmentFlags();
    }

    bool ResourcesLoaded() const
    {
        return nEnvFlags & EFF_LOADED;
    }

    int LoadResources();
    void UnloadResources();
    void UpdateTextureAspect();

    void ComputeEnvironmentFlags();
    bool IsActive() const;

    void GetStaticBounds(AABB& bbResult, const QuatTS& loc, const SPhysForces& forces, const FStaticBounds& opts) const;

    float GetTravelBounds(AABB& bbResult, const QuatTS& loc, const SForceParams& forces, const FStaticBounds& opts, const FEmitterFixed& var) const;

    float GetMaxParticleSize() const
    {
        float fMaxSize = max(fSizeX.GetMaxValue(), fSizeY.GetMaxValue());   // MKS ? wasn't considering aspect before, only fSizeY (height value)
        if (pStatObj)
        {
            fMaxSize *= pStatObj->GetRadius();
        }
        return fMaxSize;
    }
    float GetMaxObjectSize() const
    {
        const float maxX = fSizeX.GetMaxValue();
        const float maxY = fSizeY.GetMaxValue();

        // Prevent an invalid divide.
        if (abs(maxY) <= 0.001f)
        {
            return 0.0f;
        }

        float aspect = maxX / maxY;
        return sqrt_fast_tpl(sqr(1.f + abs(fPivotX.GetMaxValue())) * sqr(aspect) + sqr(1.f + abs(fPivotY.GetMaxValue())));
    }
    float GetMaxObjectSize(IStatObj* pObj) const
    {
        if (pObj)
        {
            AABB bb = pObj->GetAABB();
            if (bNoOffset || ePhysicsType == ePhysicsType.RigidBody)
            {
                // Rendered at origin.
                return sqrt_fast_tpl(max(bb.min.GetLengthSquared(), bb.max.GetLengthSquared()));
            }
            else
            {
                // Rendered at center.
                return bb.GetRadius();
            }
        }
        else
        {
            return GetMaxObjectSize();
        }
    }
    float GetMaxVisibleSize() const;

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        TypeInfo().GetMemoryUsage(pSizer, this);
    }

protected:

    void GetMaxTravelBounds(AABB& bbResult, const QuatTS& loc, const SPhysForces& forces, const FStaticBounds& opts) const;

    template<class Var>
    void GetEmitParams(SEmitParams& emit, const QuatTS& loc, const SPhysForces& forces, const FStaticBounds& opts, const Var& var) const;
};

struct FMaxEffectLife
{
    OPT_STRUCT(FMaxEffectLife)
        OPT_VAR(bool, bAllChildren)
        OPT_VAR(bool, bIndirectChildren)
        OPT_VAR(bool, bParticleLife)
        OPT_VAR(bool, bPreviewMode)
        OPT_VAR(float, fEmitterMaxLife)
};

class CParticleEffect;

class CLodInfo
    : public SLodInfo
{
public:
    CLodInfo();
    CLodInfo(float distance, bool active, IParticleEffect* effect);

    virtual void SetDistance(float distance);
    virtual void SetActive(bool active);

    virtual float GetDistance();
    virtual bool GetActive();
    virtual IParticleEffect* GetTopParticle();

    virtual void Refresh();
    void SetTopParticle(IParticleEffect* particleEffect);
    void GetMemoryUsage(ICrySizer* pSizer) const;

private:
    float fDistance;            // distance
    bool  bActive;              // if LOD of current distacne is active or not
    _smart_ptr<IParticleEffect> pTopParticle;   // The top particle in this lod.
};

class CLodParticle
    : public _i_reference_target_t
{
public:
    CLodParticle();
    CLodParticle(CLodInfo* lod, const CParticleEffect* originalEffect);
    ~CLodParticle();

    CParticleEffect*    GetParticle();
    SLodInfo*           GetLod();
    void                SetLod(SLodInfo* lod);
    bool                IsValid();

    void                Rename(const AZStd::string& name);

    void Serialize(XmlNodeRef node, bool bLoading, bool bAll, CParticleEffect* originalEffect = nullptr);
    void GetMemoryUsage(ICrySizer* pSizer) const;
private:
    AZStd::string GenerateName(const AZStd::string& name);

    _smart_ptr<CLodInfo> pLod;
    _smart_ptr<CParticleEffect> pLodParticle;
};

/*! CParticleEffect implements IParticleEffect interface and contain all components necessary to
to create the effect
*/
class CParticleEffect
    : public IParticleEffect
    , public Cry3DEngineBase
    , public stl::intrusive_linked_list_node<CParticleEffect>
{
public:
    CParticleEffect();
    CParticleEffect(const CParticleEffect& in, bool bResetInheritance = false);
    CParticleEffect(const char* sName);
    CParticleEffect(const char* sName, const ParticleParams& params);
    ~CParticleEffect();

    void Release()
    {
        --m_nRefCounter;
        if (m_nRefCounter <= 0)
        {
            delete this;
        }
    }

    // IParticle interface.
    IParticleEmitter* Spawn(const QuatTS& loc, uint32 uEmitterFlags = 0, const SpawnParams* pSpawnParams = NULL) override;

    void SetName(const char* sFullName) override;
    const char* GetName() const override
    {
        return m_strName.c_str();
    }
    string GetFullName() const override;

    void SetEnabled(bool bEnabled) override;
    bool IsEnabled() const override
    {
        return m_pParticleParams && m_pParticleParams->bEnabled;
    };

    
    /////////////////////////////////////////////////////////////////////////////
    // Particle Level of Detail - Vera, Confetti
    /////////////////////////////////////////////////////////////////////////////
    void AddLevelOfDetail() override;
    bool HasLevelOfDetail() const override;
    int GetLevelOfDetailCount() const override;
    SLodInfo* GetLevelOfDetail(int index) const override;
    int GetLevelOfDetailIndex(const SLodInfo* lod) const override;
    SLodInfo* GetLevelOfDetailByDistance(float distance) const override;
    void RemoveLevelOfDetail(SLodInfo* lod) override;
    void RemoveAllLevelOfDetails() override;
    IParticleEffect* GetLodParticle(SLodInfo* lod) const override;

    void SortLevelOfDetailBasedOnDistance();

    //////////////////////////////////////////////////////////////////////////
    //! Load resources, required by this particle effect (Textures and geometry).
    virtual bool LoadResources()
    {
        return LoadResources(true);
    }
    virtual void UnloadResources()
    {
        UnloadResources(true);
    }

    //////////////////////////////////////////////////////////////////////////
    // Child particle systems.
    //////////////////////////////////////////////////////////////////////////
    int GetChildCount() const
    {
        return m_children.size();
    }
    IParticleEffect* GetChild(int index) const
    {
        return &m_children[index];
    }

    virtual void ClearChilds();
    virtual void InsertChild(int slot, IParticleEffect* pEffect);
    virtual int FindChild(IParticleEffect* pEffect) const;

    void ReorderChildren(const std::vector<IParticleEffect*> & children) override;

    virtual void SetParent(IParticleEffect* pParent);
    virtual IParticleEffect* GetParent() const
    {
        return m_parent;
    }

    //////////////////////////////////////////////////////////////////////////
    virtual void SetParticleParams(const ParticleParams& params);
    virtual const ParticleParams& GetParticleParams() const;
    virtual const ParticleParams& GetDefaultParams() const;

    virtual void Serialize(XmlNodeRef node, bool bLoading, bool bAll, const ParticleParams* defaultParams = nullptr);
    virtual bool Reload(bool bAll);

    void GetMemoryUsage(ICrySizer* pSizer) const;

    // Further interface.

    const ResourceParticleParams& GetParams() const
    {
        assert(m_pParticleParams);
        return *m_pParticleParams;
    }

    bool HasParams() const
    {
        return (m_pParticleParams) ? true : false;
    }

    void InstantiateParams();
    const ParticleParams& GetDefaultParams(ParticleParams::EInheritance eInheritance, int nVersion) const;

    //apply parameter changes to children with inheritance
    void PropagateParticleParams(const ParticleParams &prevParmes, const ParticleParams& newParams);

    const char* GetBaseName() const;

    bool IsNull() const
    {
        // Empty placeholder effect.
        return !m_pParticleParams && m_children.empty();
    }
    bool ResourcesLoaded(bool bAll) const;
    bool LoadResources(bool bAll, cstr sSource = 0) const;
    void UnloadResources(bool bAll) const;
    bool IsActive(bool bAll = false) const;
    uint32 GetEnvironFlags(bool bAll) const;

    CParticleEffect* FindChild(const char* szChildName) const;
    const CParticleEffect* FindActiveEffect(int nVersion) const;

    CParticleEffect* GetIndirectParent() const;

    float CalcEffectLife(FMaxEffectLife const& opts) const;

    float CalcMaxParticleFullLife(bool previewMode) const
    {
        return CalcEffectLife(FMaxEffectLife().bParticleLife(1).bIndirectChildren(1).bPreviewMode(previewMode));
    }

    float GetEquilibriumAge(bool bAll, bool bPreviewMode) const;

    void GetEffectCounts(SEffectCounts& counts) const;

private:
    void RemoveLevelOfDetailRecursive(SLodInfo* lod);
    void ReplaceLOD(SLodInfo* lod);
    void AddLevelOfDetail(SLodInfo* lod, bool addToChildren);

    //! Clear all child effects recursively, starting with leaf
    static void ClearChildsBottomUp(CParticleEffect* effect);

    //////////////////////////////////////////////////////////////////////////
    // Name of effect or sub-effect, minimally qualified.
    // For top-level effects, this includes library.group.
    // For all child effects, is just the base name.
    string                                                  m_strName;
    ResourceParticleParams*                 m_pParticleParams;

    //! Parenting.
    CParticleEffect*                                m_parent;
    SmartPtrArray<CParticleEffect>  m_children;

    // Level of detail array - Vera, Confetti
    SmartPtrArray<CLodParticle> m_levelofdetail;
};


//
// Travel utilities
//
namespace Travel
{
#define fDRAG_APPROX_THRESHOLD  0.01f                       // Max inaccuracy we allow in fast drag force approximation

    inline float TravelDistance(float fV, float fDrag, float fT)
    {
        float fDecay = fDrag * fT;
        if (fDecay < fDRAG_APPROX_THRESHOLD)
        {
            return fV * fT * (1.f - fDecay);
        }
        else
        {
            // Compute accurate distance with drag.
            return fV / fDrag * (1.f - expf(-fDecay));
        }
    }

    inline float TravelSpeed(float fV, float fDrag, float fT)
    {
        float fDecay = fDrag * fT;
        if (fDecay < fDRAG_APPROX_THRESHOLD)
        {
            // Fast approx drag computation.
            return fV * (1.f - fDecay);
        }
        else
        {
            return fV * expf(-fDecay);
        }
    }

    inline void Travel(Vec3& vPos, Vec3& vVel, float fTime, const SForceParams& forces)
    {
        // Analytically compute new velocity and position, accurate for any time step.
        if (forces.fDrag * fTime >= fDRAG_APPROX_THRESHOLD)
        {
            //
            // Air resistance proportional to velocity is typical for slower laminar movement.
            // For drag d (units in 1/time), wind W, and gravity G:
            //
            //      V' = d (W-V) + G
            //
            //  The analytic solution is:
            //
            //      VT = G/d + W,                                   terminal velocity
            //      V = (V-VT) e^(-d t) + VT
            //      X = (V-VT) (1 - e^(-d t))/d + VT t
            //
            //  A faster approximation, accurate to 2nd-order t is:
            //
            //      e^(-d t) => 1 - d t + d^2 t^2/2
            //      X += V t + (G + (W-V) d) t^2/2
            //      V += (G + (W-V) d) t
            //

            float fInvDrag = 1.f / forces.fDrag;
            Vec3 vTerm = forces.vWind + forces.vAccel * fInvDrag;
            float fDecay = 1.f - expf(-forces.fDrag * fTime);
            float fT = fDecay * fInvDrag;
            vPos += vVel * fT + vTerm * (fTime - fT);
            vVel = Lerp(vVel, vTerm, fDecay);
        }
        else
        {
            // Fast approx drag computation.
            Vec3 vAccel = forces.vAccel + (forces.vWind - vVel) * forces.fDrag;
            vPos += vVel * fTime + vAccel * (fTime * fTime * 0.5f);
            vVel += vAccel * fTime;
        }
    }

    float TravelDistanceApprox(const Vec3& vVel, float fTime, const SForceParams& forces);

    float TravelVolume(const AABB& bbSource, const AABB& bbTravel, float fDist, float fSize);
};

//
// Other utilities
//

IStatObj::SSubObject* GetSubGeometry(IStatObj* pParent, int i);
int GetSubGeometryCount(IStatObj* pParent);

#endif // CRYINCLUDE_CRY3DENGINE_PARTICLEEFFECT_H
