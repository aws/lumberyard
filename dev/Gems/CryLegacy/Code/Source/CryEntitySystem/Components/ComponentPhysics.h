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
#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTPHYSICS_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTPHYSICS_H
#pragma once

#include "Components/IComponentPhysics.h"

// forward declarations.
struct SEntityEvent;
struct IPhysicalEntity;
struct IPhysicalWorld;

//! Implementation of the entity's physics component.
//! This component allows the entity to interact with the the physics system.
//! CComponentPhysics is tightly coupled with CComponentRender, using
//! the same slots to represent an entity with multiple sub-objects.
class CComponentPhysics
    : public IComponentPhysics
{
public:
    enum EFlags
    {
        CHARACTER_SLOT_MASK     = 0x000F, // Slot Id, of physicalized character.
        // When set, physics component will ignore incoming xform events from the entity.
        // Needed to prevent cycle change, when physical entity change entity xform and recieve back an event about entity xform.
        FLAG_IGNORE_XFORM_EVENT = 0x0010,
        FLAG_IGNORE_BUOYANCY    = 0x0020,
        FLAG_PHYSICS_DISABLED   = 0x0040,
        FLAG_SYNC_CHARACTER     = 0x0080,
        FLAG_WAS_HIDDEN                 = 0x0100,
        FLAG_PHYS_CHARACTER     = 0x0200,
        FLAG_PHYS_AWAKE_WHEN_VISIBLE   = 0x0400,
        FLAG_ATTACH_CLOTH_WHEN_VISIBLE = 0x0800,
        FLAG_POS_EXTRAPOLATED   = 0x1000,
        FLAG_DISABLE_ENT_SERIALIZATION = 0x2000,
        FLAG_PHYSICS_REMOVED   = 0x4000,
    };

    CComponentPhysics();
    ~CComponentPhysics();
    IEntity* GetEntity() const { return m_pEntity; };

    //////////////////////////////////////////////////////////////////////////
    // IComponent interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void Initialize(const SComponentInitializer& init);
    virtual void ProcessEvent(SEntityEvent& event);

    virtual void Done();
    virtual void UpdateComponent(SEntityUpdateContext& ctx);
    virtual void Reload(IEntity* pEntity, SEntitySpawnParams& params);
    virtual void SerializeXML(XmlNodeRef& entityNode, bool bLoading);
    virtual void Serialize(TSerialize ser);
    virtual bool NeedSerialize();
    virtual void SerializeTyped(TSerialize ser, int type, int flags);
    virtual bool GetSignature(TSerialize signature);
    virtual void EnableNetworkSerialization(bool enable);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // IComponentPhysics interface.
    //////////////////////////////////////////////////////////////////////////
    virtual void GetLocalBounds(AABB& bbox);
    virtual void GetWorldBounds(AABB& bbox);

    virtual void Physicalize(SEntityPhysicalizeParams& params);
    virtual IPhysicalEntity* GetPhysicalEntity() const { return m_pPhysicalEntity; }
    virtual void EnablePhysics(bool bEnable);
    virtual bool IsPhysicsEnabled() const;
    virtual void AddImpulse(int ipart, const Vec3& pos, const Vec3& impulse, bool bPos, float fAuxScale, float fPushScale = 1.0f);

    virtual void SetTriggerBounds(const AABB& bbox);
    virtual void GetTriggerBounds(AABB& bbox) const;

    virtual int GetPartId0() { return m_partId0; }
    virtual void IgnoreXFormEvent(bool ignore) { SetFlags(ignore ? (m_nFlags | FLAG_IGNORE_XFORM_EVENT) : (m_nFlags & (~FLAG_IGNORE_XFORM_EVENT))); }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Called from physics events.
    //////////////////////////////////////////////////////////////////////////
    // Called back by physics PostStep event for the owned physical entity.
    void OnPhysicsPostStep(EventPhysPostStep* pEvent = 0) override;
    void AttachToPhysicalEntity(IPhysicalEntity* pPhysEntity) override;
    void CreateRenderGeometry(int nSlot, IGeometry* pFromGeom, bop_meshupdate* pLastUpdate = 0);
    void OnContactWithEntity(IEntity* pEntity) override;
    void OnCollision(IEntity* pTarget, int matId, const Vec3& pt, const Vec3& n, const Vec3& vel, const Vec3& targetVel, int partId, float mass) override;
    //////////////////////////////////////////////////////////////////////////

    void SetFlags(uint32 nFlags) override { m_nFlags = nFlags; };
    uint32 GetFlags() const override { return m_nFlags; };
    bool CheckFlags(uint32 nFlags) const { return (m_nFlags & nFlags) == nFlags; }
    void AssignPhysicalEntity(IPhysicalEntity* pPhysEntity, int nSlot = -1);

    virtual bool PhysicalizeFoliage(int iSlot);
    virtual void DephysicalizeFoliage(int iSlot);
    virtual IFoliage* GetFoliage(int iSlot);

    int AddSlotGeometry(int nSlot, SEntityPhysicalizeParams& params, int bNoSubslots = 1) override;
    void RemoveSlotGeometry(int nSlot) override;
    void UpdateSlotGeometry(int nSlot, IStatObj* pStatObjNew = 0, float mass = -1.0f, int bNoSubslots = 1) override;
    void MovePhysics(IComponentPhysics* dstPhysics) override;
    void SetPartId0(int partId0) { m_partId0 = partId0; }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    void ReattachSoftEntityVtx(IPhysicalEntity* pAttachToEntity, int nAttachToPart);

    float GetAudioObstructionMultiplier() const { return m_audioObstructionMultiplier; }
    void SetAudioObstructionMultiplier(float obstructionMultiplier) { m_audioObstructionMultiplier = obstructionMultiplier; }

# if !defined(_RELEASE)
    static void EnableValidation();
    static void DisableValidation();
# endif

protected:
    IPhysicalWorld* PhysicalWorld() const { return gEnv->pPhysicalWorld; }
    void OnEntityXForm(SEntityEvent& event);
    void OnChangedPhysics(bool bEnabled);
    void DestroyPhysicalEntity(bool bDestroyCharacters = true, int iMode = 0);

    void PhysicalizeSimple(SEntityPhysicalizeParams& params);
    void PhysicalizeLiving(SEntityPhysicalizeParams& params);
    void PhysicalizeParticle(SEntityPhysicalizeParams& params);
    void PhysicalizeSoft(SEntityPhysicalizeParams& params);
    void AttachSoftVtx(IRenderMesh* pRM, IPhysicalEntity* pAttachToEntity, int nAttachToPart);
    void PhysicalizeArea(SEntityPhysicalizeParams& params);
#if defined(USE_GEOM_CACHES)
    bool PhysicalizeGeomCache(SEntityPhysicalizeParams& params);
#endif
    bool PhysicalizeCharacter(SEntityPhysicalizeParams& params);
    bool ConvertCharacterToRagdoll(SEntityPhysicalizeParams& params, const Vec3& velInitial);

    void CreatePhysicalEntity(SEntityPhysicalizeParams& params);
    phys_geometry* GetSlotGeometry(int nSlot);
    void SyncCharacterWithPhysics();

    void MoveChildPhysicsParts(IPhysicalEntity* pSrcAdam, IEntity* pChild, pe_action_move_parts& amp, uint64 usedRanges);

    //////////////////////////////////////////////////////////////////////////
    // Handle colliders.
    //////////////////////////////////////////////////////////////////////////
    void AddCollider(EntityId id);
    void RemoveCollider(EntityId id);
    void CheckColliders();
    //////////////////////////////////////////////////////////////////////////

    IPhysicalEntity* QueryPhyscalEntity(IEntity* pEntity) const;

    void ReleasePhysicalEntity();
    void ReleaseColliders();

    uint32 m_nFlags;

    IEntity* m_pEntity;

    // Pointer to physical object.
    IPhysicalEntity* m_pPhysicalEntity;

    //////////////////////////////////////////////////////////////////////////
    //! List of colliding entities, used by all triggers.
    //! When entity is first added to this list it is considered as entering
    //! to proximity, when it erased from it it is leaving proximity.
    typedef std::set<EntityId> ColliderSet;
    struct Colliders
    {
        IPhysicalEntity* m_pPhysicalBBox;  // Pointer to physical bounding box (optional).
        ColliderSet colliders;
    };
    Colliders* m_pColliders;
    int m_partId0;  // partid in physics = slot# + m_partsidOffs
    float m_timeLastSync;

    // The Audio Obstruction Multiplier value
    // Used to override the value of obstruction on any particular geometric entity
    float m_audioObstructionMultiplier;
};

DECLARE_COMPONENT_POINTERS(CComponentPhysics);

#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTPHYSICS_H
