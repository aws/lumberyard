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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTPHYSICS_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTPHYSICS_H
#pragma once

#include <IComponent.h>
#include <ComponentType.h>

//! Parameters passed to IEntity::Physicalize function.
struct SEntityPhysicalizeParams
{
    //////////////////////////////////////////////////////////////////////////
    SEntityPhysicalizeParams()
        : type(0)
        , density(-1)
        , mass(-1)
        , nSlot(-1)
        , nFlagsOR(0)
        , nFlagsAND(UINT_MAX)
        , pAttachToEntity(NULL)
        , nAttachToPart(-1)
        , fStiffnessScale(0)
        , bCopyJointVelocities(false)
        , pParticle(NULL)
        , pBuoyancy(NULL)
        , pPlayerDimensions(NULL)
        , pPlayerDynamics(NULL)
        , pCar(NULL)
        , pAreaDef(NULL)
        , nLod(0)
        , szPropsOverride(0) {};
    //////////////////////////////////////////////////////////////////////////
    // Physicalization type must be one of pe_type ennums.
    // See Also: pe_type
    int type; // Always must be filled.

    // Index of object slot. -1 if all slots should be used.
    int nSlot;
    // Only one either density or mass must be set, parameter set to 0 is ignored.
    float density;
    float mass;
    // Optional physical flags.
    int nFlagsAND;
    int nFlagsOR;
    // When physicalizing geometry can specify to use physics from different LOD.
    // Used for characters that have ragdoll physics in Lod1
    int nLod;

    // Physical entity to attach this physics object (Only for Soft physical entity).
    IPhysicalEntity* pAttachToEntity;
    // Part ID in entity to attach to (Only for Soft physical entity).
    int nAttachToPart;
    // Used for character physicalization (Scale of force in character joint's springs).
    float fStiffnessScale;

    // Copy joints velocities when converting a character to ragdoll.
    bool bCopyJointVelocities;

    struct pe_params_particle* pParticle;
    struct pe_params_buoyancy* pBuoyancy;
    struct pe_player_dimensions* pPlayerDimensions;
    struct pe_player_dynamics* pPlayerDynamics;
    struct pe_params_car* pCar;

    //////////////////////////////////////////////////////////////////////////
    // This parameters are only used when type == PE_AREA
    //////////////////////////////////////////////////////////////////////////
    struct AreaDefinition
    {
        enum EAreaType
        {
            AREA_SPHERE,        // Physical area will be sphere.
            AREA_BOX,                       // Physical area will be box.
            AREA_GEOMETRY,          // Physical area will use geometry from the specified slot.
            AREA_SHAPE,             // Physical area will points to specify 2D shape.
            AREA_CYLINDER,      // Physical area will be a cylinder
            AREA_SPLINE,                // Physical area will be a spline-tube
        };

        EAreaType areaType;
        float fRadius;      // Must be set when using AREA_SPHERE or AREA_CYLINDER area type or an AREA_SPLINE.
        Vec3 boxmin, boxmax; // Min,Max of bounding box, must be set when using AREA_BOX area type.
        Vec3* pPoints;      // Must be set when using AREA_SHAPE area type or an AREA_SPLINE.
        int nNumPoints;     // Number of points in pPoints array.
        float zmin, zmax;    // Min/Max of points.
        Vec3 center;
        Vec3 axis;

        // pGravityParams must be a valid pointer to the area gravity params structure.
        struct pe_params_area* pGravityParams;

        AreaDefinition()
            : areaType(AREA_SPHERE)
            , fRadius(0)
            , boxmin(0, 0, 0)
            , boxmax(0, 0, 0)
            , pPoints(NULL)
            , nNumPoints(0)
            , pGravityParams(NULL)
            , zmin(0)
            , zmax(0)
            , center(0, 0, 0)
            , axis(0, 0, 0) {}
    };
    // When physicalizing with type == PE_AREA this must be a valid pointer to the AreaDefinition structure.
    AreaDefinition* pAreaDef;
    // an optional string with text properties overrides for CGF nodes
    const char* szPropsOverride;
};

//! Interface for the entity's physics component.
//! This component allows the entity to interact with the the physics system.
//! The physics component can have multiple sub-object slots.
struct IComponentPhysics
    : public IComponent
{
    // <interfuscator:shuffle>
    DECLARE_COMPONENT_TYPE("ComponentPhysics", 0xACC897A89E3842AD, 0xB356261D1EB8309E)

    IComponentPhysics() {}

    ComponentEventPriority GetEventPriority(const int eventID) const override { return EntityEventPriority::Physics; }

    // Description:
    //    Assign a pre-created physical entity to this component.
    // Arguments:
    //    pPhysEntity - The pre-created physical entity.
    //      nSlot - Slot Index to which the new position will be taken from.
    virtual void AssignPhysicalEntity(IPhysicalEntity* pPhysEntity, int nSlot = -1) = 0;
    // Description:
    //    Get world bounds of physical object.
    // Arguments:
    //    bounds - Returns Bounding box in world space.
    virtual void GetWorldBounds(AABB& bounds) = 0;

    // Description:
    //    Get local space physical bounding box.
    // Arguments:
    //    bounds - Returns Bounding box in local entity space.
    virtual void GetLocalBounds(AABB& bounds) = 0;

    virtual void Physicalize(SEntityPhysicalizeParams& params) = 0;
    virtual void ReattachSoftEntityVtx(IPhysicalEntity* pAttachToEntity, int nAttachToPart) = 0;
    virtual IPhysicalEntity* GetPhysicalEntity() const = 0;

    virtual void SerializeTyped(TSerialize ser, int type, int flags) = 0;

    // Enable or disable physical simulation.
    virtual void EnablePhysics(bool bEnable) = 0;
    // Is physical simulation enabled?
    virtual bool IsPhysicsEnabled() const = 0;
    // Add impulse to physical entity.
    virtual void AddImpulse(int ipart, const Vec3& pos, const Vec3& impulse, bool bPos, float fAuxScale, float fPushScale = 1.0f) = 0;

    // Description:
    //    Creates a trigger bounding box.
    //    When physics will detect collision with this bounding box it will send an events to the entity.
    //    If entity have script OnEnterArea and OnLeaveArea events will be called.
    // Arguments:
    //    bbox - Axis aligned bounding box of the trigger in entity local space (Rotation and scale of the entity is ignored).
    //           Set empty bounding box to disable trgger.
    virtual void SetTriggerBounds(const AABB& bbox) = 0;

    // Description:
    //    Retrieve trigger bounding box in local space.
    // Return:
    //    Axis aligned bounding box of the trigger in the local space.
    virtual void GetTriggerBounds(AABB& bbox) const = 0;

    // Description:
    //      physicalizes the foliage of StatObj in slot iSlot
    virtual bool PhysicalizeFoliage(int iSlot) = 0;

    // Description:
    //      dephysicalizes the foliage in slot iSlot
    virtual void DephysicalizeFoliage(int iSlot) = 0;

    // Description:
    //      returns foliage object in slot iSlot
    virtual struct IFoliage* GetFoliage(int iSlot) = 0;

    // Description:
    //    retrieve current partid offset
    virtual int GetPartId0() = 0;


    // Description:
    //    Enable/disable network serialisation of the physics aspect
    virtual void EnableNetworkSerialization(bool enable) = 0;

    // Description:
    //    Have the physics ignore the XForm event
    virtual void IgnoreXFormEvent(bool ignore) = 0;

    // Note:
    //    Promoted functions to IComponentPhysics for access by CScriptBind_Entity.
    virtual uint32 GetFlags() const = 0;

    // Note:
    //    Promoted functions to IComponentPhysics for access by CPhysicsEventListener.
    virtual void SetFlags(uint32 nFlags) = 0;


    // Description:
    //    Called back by physics PostStep event for the owned physical entity.
    // Note:
    //    Promoted functions to IComponentPhysics for access by CPhysicsEventListener.
    virtual void OnPhysicsPostStep(EventPhysPostStep* pEvent = 0) = 0;

    // Note:
    //    Promoted functions to IComponentPhysics for access by CPhysicsEventListener.
    virtual void OnContactWithEntity(IEntity* pEntity) = 0;
    virtual void OnCollision(IEntity* pTarget, int matId, const Vec3& pt, const Vec3& n, const Vec3& vel, const Vec3& targetVel, int partId, float mass) = 0;

    // Description:
    //    Adds physicalized geometry into the specified slot.
    virtual int AddSlotGeometry(int nSlot, SEntityPhysicalizeParams& params, int bNoSubslots = 1) = 0;

    // Description:
    //    Removes physicalized geometry from the specified slot.
    virtual void RemoveSlotGeometry(int nSlot) = 0;

    // Description:
    //    Updates the geometry state in the specified slot.
    virtual void UpdateSlotGeometry(int nSlot, IStatObj* pStatObjNew = 0, float mass = -1.0f, int bNoSubslots = 1) = 0;

    // Description:
    //    Moves the physical properties from this component into the specified component.
    virtual void MovePhysics(IComponentPhysics* dstPhysics) = 0;

    // Description:
    //    Attaches the provided physical entity into the component
    virtual void AttachToPhysicalEntity(IPhysicalEntity* pPhysEntity) = 0;

    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IComponentPhysics);

#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTPHYSICS_H