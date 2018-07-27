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

// Description : Interfaces to particle types


#ifndef CRYINCLUDE_CRYCOMMON_IPARTICLES_H
#define CRYINCLUDE_CRYCOMMON_IPARTICLES_H
#pragma once

#include <I3DEngine.h>
#include <IMemory.h>
#include <IEntityRenderState.h>
#include <TimeValue.h>

enum EParticleEmitterFlags
{
    ePEF_Independent = BIT(0),          // Not controlled by entity.
    ePEF_Nowhere = BIT(1),                  // Not in scene (e.g. rendered in editor window)
    ePEF_TemporaryEffect = BIT(2),  // Has temporary programmatically created IParticleEffect.
    ePEF_IgnoreRotation = BIT(3),
    ePEF_NotAttached = BIT(4),      // a.k.a. EmitterNoFollows
    ePEF_DisableLOD = BIT(5),      //set to disable LOD if there are

    ePEF_Custom = BIT(16),                  // Any bits above and including this one can be used for game specific purposes
};

// Summary:
//     Reference to one geometry type
// Description:
//     Reference to one of several types of geometry, for particle attachment.
struct GeomRef
{
    IStatObj*   m_pStatObj;
    ICharacterInstance* m_pChar;
    IPhysicalEntity* m_pPhysEnt;

    GeomRef()
        : m_pStatObj(0)
        , m_pChar(0)
        , m_pPhysEnt(0) {}
    GeomRef(IStatObj* pObj)
        : m_pStatObj(pObj)
        , m_pChar(0)
        , m_pPhysEnt(0) {}
    GeomRef(ICharacterInstance* pChar)
        : m_pChar(pChar)
        , m_pStatObj(0)
        , m_pPhysEnt(0) {}
    GeomRef(IPhysicalEntity* pPhys)
        : m_pChar(0)
        , m_pStatObj(0)
        , m_pPhysEnt(pPhys) {}

    operator bool() const
    {
        return m_pStatObj || m_pChar || m_pPhysEnt;
    }
};
//Summary:
//      Real-time params to control particle emitters.
// Description:
//      Real-time params to control particle emitters. Some parameters override emitter params.
struct SpawnParams
{
    EGeomType           eAttachType;            // What type of object particles emitted from.
    EGeomForm           eAttachForm;            // What aspect of shape emitted from.
    bool                    bCountPerUnit;      // Multiply particle count also by geometry extent (length/area/volume).
    bool                    bEnableAudio;     // Used by particle effect instances to indicate whether audio should be updated or not.
    bool                    bRegisterByBBox;    // Use the Bounding Box instead of Position to Register in VisArea
    float                   fCountScale;            // Multiple for particle count (on top of bCountPerUnit if set).
    float                   fSizeScale;             // Multiple for all effect sizes.
    float                   fSpeedScale;            // Multiple for particle emission speed.
    float                   fTimeScale;             // Multiple for emitter time evolution.
    float                   fPulsePeriod;           // How often to restart emitter.
    float                   fStrength;              // Controls parameter strength curves.
    float                   fStepValue;             // Fast forward the particle by a step value Eric@conffx
    stack_string            sAudioRTPC;             // Indicates what audio RTPC this particle effect instance drives.
    
    //new spawn parameters for particle component
    Vec3                    colorTint;                  // particle color tint
    Vec3                    particleSizeScale;          // Multiplier for particle size. For geom particle, it only uses y component for uniform scale
    float                   particleSizeScaleRandom;    // Random for particle size's scale.

    inline SpawnParams(EGeomType eType = GeomType_None, EGeomForm eForm = GeomForm_Surface)
        : colorTint(1.0f, 1.0f, 1.0f)
        , particleSizeScale(1.0f, 1.0f, 1.0f)
        , particleSizeScaleRandom(0.0f)
    {
        eAttachType = eType;
        eAttachForm = eForm;
        bCountPerUnit = false;
        bEnableAudio = true;
        bRegisterByBBox = false;
        fCountScale = 1;
        fSizeScale = 1;
        fSpeedScale = 1;
        fTimeScale = 1;
        fPulsePeriod = 0;
        fStrength = -1;
        fStepValue = 0;
    }
};

struct ParticleTarget
{
    Vec3    vTarget;                // Target location for particles.
    Vec3    vVelocity;              // Velocity of moving target.
    float   fRadius;                // Size of target, for orbiting.

    // Options.
    bool    bTarget;                // Target is enabled.
    bool    bPriority;              // This target takes priority over others (attractor entities).

    ParticleTarget()
    {
        memset(this, 0, sizeof(*this));
    }
};

struct EmitParticleData
{
    IStatObj*                   pStatObj;               // The displayable geometry object for the entity. If NULL, uses emitter settings for sprite or geometry.
    IPhysicalEntity*    pPhysEnt;               // A physical entity which controls the particle. If NULL, uses emitter settings to physicalise or move particle.
    QuatTS                      Location;               // Specified location for particle.
    Velocity3                   Velocity;               // Specified linear and rotational velocity for particle.
    bool                            bHasLocation;       // Location is specified.
    bool                            bHasVel;                // Velocities are specified.
    float               fBeamAge;

    EmitParticleData()
        : pStatObj(0)
        , pPhysEnt(0)
        , Location(IDENTITY)
        , Velocity(ZERO)
        , bHasLocation(false)
        , bHasVel(false)
        , fBeamAge(0.f)
    {
    }
};

struct ParticleParams;

// Level of Detail related data - Vera, Chris, Confetti
struct SLodInfo
    : public _i_reference_target_t
{
public:
    virtual void SetDistance(float distance) = 0;
    virtual void SetActive(bool active) = 0;

    virtual float GetDistance() = 0;
    virtual bool GetActive() = 0;
    virtual IParticleEffect* GetTopParticle() = 0;
    virtual void Refresh() = 0;
};

// Summary:
//      Interface to control a particle effect.
// Description:
//      This interface is used by I3DEngine::CreateParticleEffect to control a particle effect.
//  It is created by CreateParticleEffect method of 3d engine.
struct IParticleEffect
    : public _i_reference_target_t
{
    static QuatTS ParticleLoc(const Vec3& pos, const Vec3& dir = Vec3(0, 0, 1), float scale = 1.f)
    {
        QuatTS qts(IDENTITY, pos, scale);
        if (!dir.IsZero())
        {
            // Rotate in 2 stages to avoid roll.
            Vec3 dirxy = Vec3(dir.x, dir.y, 0.f);
            if (!dirxy.IsZero(1e-10f))
            {
                dirxy.Normalize();
                qts.q = Quat::CreateRotationV0V1(dirxy, dir.GetNormalized())
                    * Quat::CreateRotationV0V1(Vec3(0, 1, 0), dirxy);
            }
            else
            {
                qts.q = Quat::CreateRotationV0V1(Vec3(0, 1, 0), dir.GetNormalized());
            }
        }

        return qts;
    }

    // <interfuscator:shuffle>
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    // Summary:
    //      Spawns this effect.
    // Arguments:
    //    qLoc - World location to place emitter.
    //      uEmitterFlags - EParticleEmitterFlags
    // Return Value:
    //     The spawned emitter, or 0 if unable.
    virtual struct IParticleEmitter* Spawn(const QuatTS& loc, uint uEmitterFlags = 0, const SpawnParams* pSpawnParams = NULL) = 0;

    // Summary:
    //     Sets a new name to this particle effect.
    // Arguments:
    //     sFullName - The full name of this effect, including library and group qualifiers.
    virtual void SetName(cstr sFullName) = 0;

    // Summary:
    //     Gets the name of this particle effect.
    // Return Value:
    //     A C string which holds the minimally qualified name of this effect.
    //     For top level effects, includes library.group qualifier.
    //     For child effects, includes only the base name.
    virtual cstr GetName() const = 0;

    // Summary:
    //     Gets the name of this particle effect.
    // Return Value:
    //     A std::string which holds the fully qualified name of this effect, with all parents and library.
    virtual string GetFullName() const = 0;

    // Summary:
    //     Enables or disables the effect.
    // Arguments:
    //     bEnabled - set to true to enable the effect or to false to disable it
    virtual void SetEnabled(bool bEnabled) = 0;

    // Summary:
    //     Determines if the effect is already enabled.
    // Return Value:
    //     A boolean value which indicate the status of the effect; true if
    //     enabled or false if disabled.
    virtual bool IsEnabled() const = 0;

    /////////////////////////////////////////////////////////////////////////////
    // Particle Level of Detail - Vera, Confetti
    /////////////////////////////////////////////////////////////////////////////
    // Summary:
    //      Add level of detail to the effect
    // Arguments:
    //      distance - distance of the level
    virtual void AddLevelOfDetail() = 0;


    // Summary:
    //      Determines if the effect has level of detail.
    // Return Value:
    //      A boolean value which indicate if the effect has level of detail;
    //      true if it has, false for not.
    virtual bool HasLevelOfDetail() const = 0;

    // Summary:
    //      Returns the number of LoDs
    // Return Value:
    //      A integer value which indicate the number of current LoDs on this particleeffect;
    virtual int GetLevelOfDetailCount() const = 0;

    // Summary:
    //      Returns the LoD at index
    // Arguments:
    //      index - the index of the LoD to be returned
    // Return Value:
    //      A pointer to a SLodInfo
    virtual SLodInfo* GetLevelOfDetail(int index) const = 0;

    // Summary:
    //      Returns the LoD with that distance if there is one
    // Arguments:
    //      The distance of the LoD
    // Return Value:
    //      A pointer to a SLodInfo - null if there is no lod with that distance
    virtual SLodInfo* GetLevelOfDetailByDistance(float distance) const = 0;

    // Summary:
    //      Returns the index of loD
    // Arguments:
    //      the pointer to a SLodInfo
    // Return Value:
    //      the index of this loD in the loD array
    virtual int GetLevelOfDetailIndex(const SLodInfo* lod) const = 0;

    // Summary:
    //      Remove Level of detail by reference
    // Arguments:
    //      A pointer to a SLodInfo. IParticleEffect will remove this Lod from the effect.
    virtual void RemoveLevelOfDetail(SLodInfo* lod) = 0;

    // Summary:
    //      Removes all Level of details
    virtual void RemoveAllLevelOfDetails() = 0;

    virtual IParticleEffect* GetLodParticle(SLodInfo* lod) const = 0;


    // End of Level of Detail
    //////////////////////////////////////////////////////////////////////////////



    // Summary:
    //     Sets the particle parameters.
    // Return Value:
    //     An object of the type ParticleParams which contains several parameters.
    virtual void SetParticleParams(const ParticleParams& params) = 0;

    //! Return ParticleParams.

    // Summary:
    //     Gets the particle parameters.
    // Return Value:
    //     An object of the type ParticleParams which contains several parameters.
    virtual const ParticleParams& GetParticleParams() const = 0;

    // Summary:
    //     Get the set of ParticleParams used as the default for this effect, for serialization, display, etc
    virtual const ParticleParams& GetDefaultParams() const = 0;

    //////////////////////////////////////////////////////////////////////////
    // Child particle systems.
    //////////////////////////////////////////////////////////////////////////

    // Summary:
    //     Gets the number of sub particles children.
    // Return Value:
    //     An integer representing the amount of sub particles children
    virtual int GetChildCount() const = 0;

    //! Get sub Particles child by index.

    // Summary:
    //     Gets a specified particles child.
    // Arguments:
    //     index - The index of a particle child
    // Return Value:
    //     A pointer to a IParticleEffect derived object.
    virtual IParticleEffect* GetChild(int index) const = 0;

    // Summary:
    //   Removes all child particles.
    virtual void ClearChilds() = 0;

    // Summary:
    //   Inserts a child particle effect at a precise slot.
    // Arguments:
    //   slot - An integer value which specify the desired slot
    //   pEffect - A pointer to the particle effect to insert
    virtual void InsertChild(int slot, IParticleEffect* pEffect) = 0;

    // Summary:
    //   Finds in which slot a child particle effect is stored.
    // Arguments:
    //   pEffect - A pointer to the child particle effect
    // Return Value:
    //   An integer representing the slot number or -1 if the slot is not found.
    virtual int FindChild(IParticleEffect* pEffect) const = 0;

    // Summary:
    //   A cheap patch function to reorder children effects for this particle effect.
    //   the reordering could happen when drag/drop particle items in Particle Editor.
    // Arguments:
    //   children - An array of children with proper order
    virtual void ReorderChildren(const std::vector<IParticleEffect*> & children) = 0;

    // Summary:
    //   Remove effect from current parent, and set new parent
    // Arguments:
    //   pParent: New parent, may be 0
    virtual void SetParent(IParticleEffect* pParent) = 0;

    // Summary:
    //   Gets the particles effect parent, if any.
    // Return Value:
    // A pointer representing the particles effect parent.
    virtual IParticleEffect* GetParent() const = 0;

    // Summary:
    //   Gets the root particle effect.
    // Return Value:
    // A pointer representing the root particles effect
    IParticleEffect* GetRoot()
    {
        IParticleEffect* root = this;
        while (root->GetParent())
        {
            root = root->GetParent();
        }
        return root;
    }

    // Summary:
    //   Loads all resources needed for a particle effects.
    // Returns:
    //   True if any resources loaded.
    virtual bool LoadResources() = 0;

    // Summary:
    //   Unloads all resources previously loaded.
    virtual void UnloadResources() = 0;

    // Summary:
    //   Serializes particle effect to/from XML.
    // Arguments:
    //   bLoading - true when loading, false for saving.
    //   bChildren - When true also recursively serializes effect children.
    virtual void Serialize(XmlNodeRef node, bool bLoading, bool bChildren, const ParticleParams* defaultParams = nullptr) = 0;

    // Summary:
    //   Reloads the effect from the particle database
    // Arguments:
    //   bChildren - When true also recursively reloads effect children.
    // Returns:
    //   true - if sucessfully reloaded
    virtual bool Reload(bool bChildren) = 0;

    // </interfuscator:shuffle>

    // Compatibility versions.
    IParticleEmitter* Spawn(const Matrix34& mLoc, uint uEmitterFlags = 0)
    {
        return Spawn(QuatTS(mLoc), uEmitterFlags);
    }
    IParticleEmitter* Spawn(bool bIndependent, const QuatTS& qLoc)
    {
        return Spawn(qLoc, ePEF_Independent * uint32(bIndependent));
    }
    IParticleEmitter* Spawn(bool bIndependent, const Matrix34& mLoc)
    {
        return Spawn(QuatTS(mLoc), ePEF_Independent * uint32(bIndependent));
    }
};

// Description:
//     An IParticleEmitter should usually be created by
//     I3DEngine::CreateParticleEmitter. Deleting the emitter should be done
//     using I3DEngine::DeleteParticleEmitter.
// Summary:
//     Interface to a particle effect emitter.
struct IParticleEmitter
    : public IRenderNode
    , public CMultiThreadRefCount
{
    // <interfuscator:shuffle>
    // Summary:
    //      Returns whether emitter still alive in engine.
    virtual bool IsAlive() const = 0;

    // Summary:
    //      Returns whether emitter requires no further attachment.
    virtual bool IsInstant() const = 0;

    // Summary:
    //      Sets emitter state to active or inactive. Emitters are initially active.
    //      if bActive = true:
    //              Emitter updates and emits as normal, and deletes itself if limited lifetime.
    //      if bActive = false:
    //              Stops all emitter updating and particle emission.
    //              Existing particles continue to update and render.
    //              Emitter is not deleted.
    virtual void Activate(bool bActive) = 0;

    // Summary:
    //      Removes emitter and all particles instantly.
    virtual void Kill() = 0;

    // Summary:
    //       Advances the emitter to its equilibrium state.
    virtual void Prime() = 0;
    // Summary:
    //       Restarts the emitter from scratch (if active).
    //       Any existing particles are re-used oldest first.
    virtual void Restart() = 0;

    // Description:
    //     Will define the effect used to spawn the particles from the emitter.
    // Notes:
    //     Never call this function if you already used SetParams.
    // See Also:
    //     SetParams
    // Arguments:
    //     pEffect - A pointer to an IParticleEffect object
    // Summary:
    //     Set the effects used by the particle emitter.
    virtual void SetEffect(const IParticleEffect* pEffect) = 0;

    // Description:
    //     Returns particle effect assigned on this emitter.
    virtual const IParticleEffect* GetEffect() const = 0;

    // Summary:
    //       Specifies how particles are emitted from source.
    virtual void SetSpawnParams(const SpawnParams& spawnParams, GeomRef geom = GeomRef()) = 0;

    // Summary:
    //       Retrieves current SpawnParams.
    virtual void GetSpawnParams(SpawnParams& spawnParams) const = 0;

    // Summary:
    //      Associates emitter with entity, for dynamic updating of positions etc.
    // Notes:
    //      Must be done when entity created or serialized, entity association is not serialized.
    virtual void SetEntity(IEntity* pEntity, int nSlot) = 0;

    // Summary:
    //          Sets location with quat-based orientation.
    // Notes:
    //          IRenderNode.SetMatrix() is equivalent, but performs conversion.
    virtual void SetLocation(const QuatTS& loc) = 0;

    // Attractors.
    virtual void SetTarget(const ParticleTarget& target) = 0;

    // Summary:
    //      Updates emitter's state to current time.
    virtual void Update() = 0;

    // Summary:
    //       Programmatically adds particle to emitter for rendering.
    //       With no arguments, spawns particles according to emitter settings.
    //       Specific objects can be passed for programmatic control.
    // Arguments:
    //       pData - Specific data for particle, or NULL for defaults.
    virtual void EmitParticle(const EmitParticleData* pData = NULL) = 0;

    void EmitParticle(IStatObj* pStatObj, IPhysicalEntity* pPhysEnt = NULL, QuatTS* pLocation = NULL, Velocity3* pVel = NULL)
    {
        EmitParticleData data;
        data.pStatObj = pStatObj;
        data.pPhysEnt = pPhysEnt;
        if (pLocation)
        {
            data.Location = *pLocation;
            data.bHasLocation = true;
        }
        if (pVel)
        {
            data.Velocity = *pVel;
            data.bHasVel = true;
        }
        EmitParticle(&data);
    }

    virtual bool UpdateStreamableComponents(float fImportance, Matrix34A& objMatrix, IRenderNode* pRenderNode, float fEntDistance, bool bFullUpdate, int nLod) = 0;

    // Summary:
    //       Get the Entity ID that this particle emitter is attached to
    virtual unsigned int GetAttachedEntityId() = 0;

    // Summary:
    //       Get the Entity Slot that this particle emitter is attached to
    virtual int GetAttachedEntitySlot() = 0;

    // Summary:
    //       Get the flags associated with this particle emitter
    virtual const uint32& GetEmitterFlags() const = 0;

    // Summary:
    //       Set the flags associated with this particle emitter
    virtual void SetEmitterFlags(uint32 flags) = 0;
    // </interfuscator:shuffle>
    virtual float GetEmitterAge() const = 0;             //Eric@conffx
    virtual float GetRelativeAge(float fStartAge = 0.f) const = 0; //provides the relative age for the emitter based on 0-1.f (age/stopAge)
    virtual float GetAliveParticleCount() const = 0;             //Eric@conffx
    virtual QuatTS GetLocationQuat() const = 0;          //Eric@conffx
        
    // Summary:
    //      Returns true when this emitter is in preview mode
    virtual bool GetPreviewMode() const = 0;

    // Summary:
    //      returns the maximum bounds for the emitter and all subemitters
    virtual void GetEmitterBounds(Vec3& totalBounds) = 0;
};

//////////////////////////////////////////////////////////////////////////
// Description:
//   A callback interface for a class that wants to be aware when particle emitters are being created/deleted.
struct IParticleEffectListener
{
    // <interfuscator:shuffle>
    virtual ~IParticleEffectListener(){}
    // Description:
    //   This callback is called when a new particle emitter is created.
    // Arguments:
    //   pEmitter     - The emitter created
    //   bIndependent -
    //   mLoc             - The location of the emitter
    //   pEffect      - The particle effect
    virtual void OnCreateEmitter(IParticleEmitter* pEmitter, const QuatTS& qLoc, const IParticleEffect* pEffect, uint32 uEmitterFlags) = 0;

    // Description:
    //   This callback is called when a particle emitter is deleted.
    // Arguments:
    //   pEmitter - The emitter being deleted
    virtual void OnDeleteEmitter(IParticleEmitter* pEmitter) = 0;
    // </interfuscator:shuffle>
};

//////////////////////////////////////////////////////////////////////////
struct SContainerCounts
{
    float       EmittersRendered, ParticlesRendered;
    float       PixelsProcessed, PixelsRendered;
    float       ParticlesReiterate, ParticlesReject, ParticlesClip;
    float       ParticlesCollideTest, ParticlesCollideHit;

    SContainerCounts()
    {
        memset(this, 0, sizeof(*this));
    }
};

struct SParticleCounts
    : SContainerCounts
{
    float EmittersAlloc;
    float EmittersActive;
    float ParticlesAlloc;
    float ParticlesActive;
    float SubEmittersActive;
    int     nCollidingEmitters;
    int     nCollidingParticles;

    float StaticBoundsVolume;
    float DynamicBoundsVolume;
    float ErrorBoundsVolume;

    SParticleCounts()
    {
        memset(this, 0, sizeof(*this));
    }
};

struct SSumParticleCounts
    : SParticleCounts
{
    float   SumParticlesAlloc, SumEmittersAlloc;

    SSumParticleCounts()
        : SumParticlesAlloc(0.f)
        , SumEmittersAlloc(0.f)
    {}

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
};

struct SEffectCounts
{
    int     nLoaded, nUsed, nEnabled, nActive;

    SEffectCounts()
    {
        memset(this, 0, sizeof(*this));
    }
};

template<class T>
Array<float> FloatArray(T& obj)
{
    return Array<float>((float*)&obj, (float*)(&obj + 1));
}

inline void AddArray(Array<float> dst, Array<const float> src)
{
    for (int i = min(dst.size(), src.size()) - 1; i >= 0; --i)
    {
        dst[i] += src[i];
    }
}

inline void BlendArray(Array<float> dst, float fDst, Array<const float> src, float fSrc)
{
    for (int i = min(dst.size(), src.size()) - 1; i >= 0; --i)
    {
        dst[i] = dst[i] * fDst + src[i] * fSrc;
    }
}


//////////////////////////////////////////////////////////////////////////
struct IParticleEffectIterator
{
    virtual ~IParticleEffectIterator() {}

    virtual void AddRef() = 0;
    virtual void Release() = 0;

    virtual IParticleEffect* Next() = 0;
    virtual int GetCount() const = 0;
};

TYPEDEF_AUTOPTR(IParticleEffectIterator);
typedef IParticleEffectIterator_AutoPtr IParticleEffectIteratorPtr;

//////////////////////////////////////////////////////////////////////////
struct IParticleManager
{
    // <interfuscator:shuffle>
    virtual ~IParticleManager(){}
    //////////////////////////////////////////////////////////////////////////
    // ParticleEffects
    //////////////////////////////////////////////////////////////////////////

    // Summary:
    //     Set the ParticleEffect used as defaults for new effects, and when reading effects from XML
    // Arguments:
    //     pEffect - The effect containing the default params.
    //               If it has children, they will be used if they match the current sys_spec, or particle version number.
    virtual void SetDefaultEffect(const IParticleEffect* pEffect) = 0;

    // Summary:
    //     Get the ParticleEffect tree used as defaults for new effects.
    virtual const IParticleEffect* GetDefaultEffect() = 0;

    // Summary:
    //     Get the ParticleParams tree used as defaults for new effects, for the current sys_spec.
    // Arguments:
    //     nVersion - The particle library version to select for, or 0 for the latest version.
    virtual const ParticleParams& GetDefaultParams(int nVersion = 0) const = 0;

    // Summary:
    //     Creates a new empty particle effect.
    // Return Value:
    //     A pointer to a object derived from IParticleEffect.
    virtual IParticleEffect* CreateEffect() = 0;

    // Summary:
    //     Deletes a specified particle effect.
    // Arguments:
    //     pEffect - A pointer to the particle effect object to delete
    virtual void DeleteEffect(IParticleEffect* pEffect) = 0;

    // Summary:
    //     Searches by name for a particle effect.
    // Arguments:
    //     sEffectName - The fully qualified name (with library prefix) of the particle effect to search.
    //     bLoad - Whether to load the effect's assets if found.
    //     sSource - Optional client context, for diagnostics.
    // Return Value:
    //     A pointer to a particle effect matching the specified name, or NULL if not found.
    virtual IParticleEffect* FindEffect(cstr sEffectName, cstr sSource = "", bool bLoadResources = true) = 0;

    // Summary:
    //     Creates a particle effect from an XML node. Overwrites any existing effect of the same name.
    // Arguments:
    //     sEffectName:         the name of the particle effect to be created.
    //       effectNode:            Xml structure describing the particle effect properties.
    //       bLoadResources:    indicates if the resources for this effect should be loaded.
    // Return value:
    //     Returns a pointer to the particle effect.
    virtual IParticleEffect* LoadEffect(cstr sEffectName, XmlNodeRef& effectNode, bool bLoadResources, const cstr sSource = NULL) = 0;

    // Summary:
    //     Loads a library of effects from an XML description.
    // Arguments:
    // Return value:
#ifdef LoadLibrary
#undef LoadLibrary
#endif
    virtual bool LoadLibrary(cstr sParticlesLibrary, XmlNodeRef& libNode, bool bLoadResources) = 0;
    virtual bool LoadLibrary(cstr sParticlesLibrary, cstr sParticlesLibraryFile = NULL, bool bLoadResources = false) = 0;

    // Summary:
    //     Returns particle iterator.
    // Arguments:
    // Return value:
    virtual IParticleEffectIteratorPtr GetEffectIterator() = 0;


    //////////////////////////////////////////////////////////////////////////
    // ParticleEmitters
    //////////////////////////////////////////////////////////////////////////

    // Summary:
    //     Creates a new particle emitter, with custom particle params instead of a library effect.
    // Arguments:
    //       bIndependent -
    //       mLoc - World location of emitter.
    //       Params - Programmatic particle params.
    // Return Value:
    //     A pointer to an object derived from IParticleEmitter
    virtual IParticleEmitter* CreateEmitter(const QuatTS& loc, const ParticleParams& Params, uint uEmitterFlags = 0, const SpawnParams* pSpawnParams = NULL) = 0;

    IParticleEmitter* CreateEmitter(const Matrix34& mLoc, const ParticleParams& Params, uint uEmitterFlags = 0, const SpawnParams* pSpawnParams = NULL)
    {
        return CreateEmitter(QuatTS(mLoc), Params, uEmitterFlags, pSpawnParams);
    }

    // Summary:
    //     Deletes a specified particle emitter.
    // Arguments:
    //     pPartEmitter - Specify the emitter to delete
    virtual void DeleteEmitter(IParticleEmitter* pPartEmitter) = 0;

    // Summary:
    //     Deletes all particle emitters which have any of the flags in mask set
    // Arguments:
    //     mask - Flags used to filter which emitters to delete
    virtual void DeleteEmitters(uint32 mask) = 0;

    // Summary:
    //     Reads or writes emitter state -- creates emitter if reading
    // Arguments:
    //     ser - Serialization context
    //     pEmitter - Emitter to save if writing, NULL if reading.
    // Return Value:
    //     pEmitter if writing, newly created emitter if reading
    virtual IParticleEmitter* SerializeEmitter(TSerialize ser, IParticleEmitter* pEmitter = NULL) = 0;

    // Processing.
    virtual void Update() = 0;
    virtual void RenderDebugInfo() = 0;
    virtual void Reset(bool bIndependentOnly) = 0;
    virtual void ClearRenderResources(bool bForceClear) = 0;
    virtual void ClearDeferredReleaseResources() = 0;
    virtual void OnFrameStart() = 0;
    virtual void Serialize(TSerialize ser) = 0;
    virtual void PostSerialize(bool bReading) = 0;

    // Summary:
    //     Set the timer used to update the particle system
    // Arguments:
    //     pTimer - Specify the timer
    virtual void SetTimer(ITimer* pTimer) = 0;

    // Stats
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
    virtual void GetCounts(SParticleCounts& counts) = 0;

    //PerfHUD
    virtual void CreatePerfHUDWidget() = 0;

    // Summary:
    //   Registers new particle events listener.
    virtual void AddEventListener(IParticleEffectListener* pListener) = 0;
    virtual void RemoveEventListener(IParticleEffectListener* pListener) = 0;

    // Prepare all date for Job Particle*::Render Tasks
    virtual void PrepareForRender(const SRenderingPassInfo& passInfo) = 0;

    // Finish up all Particle*::Render Tasks
    virtual void FinishParticleRenderTasks(const SRenderingPassInfo& passInfo) = 0;

    // Add nTicks to the number of Ticks spend this frame in particle functions
    virtual void AddFrameTicks(uint64 nTicks) = 0;

    // Reset Ticks Counter
    virtual void ResetFrameTicks() = 0;

    // Get number of Ticks accumulated over this frame
    virtual uint64 NumFrameTicks() const = 0;

    // Get The number of emitters manged by ParticleManager
    virtual uint32 NumEmitter() const = 0;

    // Add nTicks to the number of Ticks spend this frame in particle functions
    virtual void AddFrameSyncTicks(uint64 nTicks) = 0;

    // Get number of Ticks accumulated over this frame
    virtual uint64 NumFrameSyncTicks() const = 0;

    // add a container to collect which take the most memory in the vertex/index pools
    virtual void AddVertexIndexPoolUsageEntry(uint32 nVertexMemory, uint32 nIndexMemory, const char* pContainerName) = 0;
    virtual void MarkAsOutOfMemory() = 0;

    // </interfuscator:shuffle>
};

#if defined(ENABLE_LW_PROFILERS)
class CParticleLightProfileSection
{
public:
    CParticleLightProfileSection()
        : m_nTicks(CryGetTicks())
    {
    }
    ~CParticleLightProfileSection()
    {
        IParticleManager* pPartMan = gEnv->p3DEngine->GetParticleManager();
        IF (pPartMan != NULL, 1)
        {
            pPartMan->AddFrameTicks(CryGetTicks() - m_nTicks);
        }
    }
private:
    uint64 m_nTicks;
};

class CParticleLightProfileSectionSyncTime
{
public:
    CParticleLightProfileSectionSyncTime()
        : m_nTicks(CryGetTicks())
    {}
    ~CParticleLightProfileSectionSyncTime()
    {
        IParticleManager* pPartMan = gEnv->p3DEngine->GetParticleManager();
        IF (pPartMan != NULL, 1)
        {
            pPartMan->AddFrameSyncTicks(CryGetTicks() - m_nTicks);
        }
    }
private:
    uint64 m_nTicks;
};

#define PARTICLE_LIGHT_PROFILER() CParticleLightProfileSection _particleLightProfileSection;
#define PARTICLE_LIGHT_SYNC_PROFILER() CParticleLightProfileSectionSyncTime _particleLightSyncProfileSection;
#else
#define PARTICLE_LIGHT_PROFILER()
#define PARTICLE_LIGHT_SYNC_PROFILER()
#endif

enum EPerfHUD_ParticleDisplayMode
{
    PARTICLE_DISP_MODE_NONE = 0,
    PARTICLE_DISP_MODE_PARTICLE,
    PARTICLE_DISP_MODE_EMITTER,
    PARTICLE_DISP_MODE_FILL,
    PARTICLE_DISP_MODE_NUM,
};

#endif // CRYINCLUDE_CRYCOMMON_IPARTICLES_H

