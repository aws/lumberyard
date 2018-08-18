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

// Description : Implements a base class for vehicle parts


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTBASE_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTBASE_H
#pragma once

#include "IVehicleSystem.h"
#include "VehicleCVars.h"
#include "Vehicle.h"
#include "ISharedParamsManager.h"
#include "SharedParams/ISharedParams.h"

class CVehiclePartSubPart;
class CVehiclePartSubPartEntity;
class CVehicle;
class CVehicleComponent;
class CVehicleSeatActionRotateTurret;

#define VEHICLE_MASS_COLLTYPE_HEAVY 1500.f
#define MAX_OPTIONAL_PARTS 5
#define GEOMETRY_DESTROYED_SUFFIX "_damaged"
#define VEH_ANIM_POSE_MODIFIER_LAYER 1

class CVehiclePartBase
    : public IVehiclePart
{
    IMPLEMENT_VEHICLEOBJECT;
public:

    BEGIN_SHARED_PARAMS(SSharedParams_Parts)

    SSharedParams_Parts()
        : m_typeId(eVPT_Base)
        , m_detachBaseForce(ZERO)
        , m_detachProbability(0.0f)
        , m_disableCollision(false)
        , m_isPhysicalized(true)
        , m_wheelIndex(0)
        , m_suspLength(0.0f)
        , m_rimRadius(0.0f)
        , m_position(ZERO)
        , m_useOption(-1)
        , m_slipFrictionMod(0.0f)
        , m_slipSlope(1.0f)
        , m_torqueScale(1.0f)
        , m_mass(0.0f)
        , m_density(-1.0f)
    {
    }

    string m_name;
    string m_helperPosName;

    Vec3 m_position;
    int m_typeId;
    int m_useOption;
    float m_mass;
    float m_density;

    // animated
    string m_filename;
    string m_filenameDestroyed;
    string m_destroyedSuffix;

    // animated joint
    Vec3 m_detachBaseForce;
    float m_detachProbability;

    // wheel
    int m_wheelIndex;
    float m_suspLength;
    float m_rimRadius;
    float m_slipFrictionMod;
    float m_slipSlope;
    float m_torqueScale;

    bool m_disableCollision;
    bool m_isPhysicalized;

    END_SHARED_PARAMS

    CVehiclePartBase();
    virtual ~CVehiclePartBase();

    // IVehiclePart
    virtual bool Init(IVehicle* pVehicle, const CVehicleParams& table, IVehiclePart* parent, CVehicle::SPartInitInfo& initInfo, int partType);
    virtual void PostInit();
    virtual void Reset();
    virtual void Release();

    virtual const char* GetName() { return m_pSharedParameters->m_name.c_str(); }
    virtual IVehiclePart* GetParent(bool root = false);
    virtual IEntity* GetEntity();

    virtual void InvalidateTM(bool invalidate){}
    virtual void AddChildPart(IVehiclePart* pPart) { m_children.push_back((CVehiclePartBase*)pPart); }

    virtual void OnEvent(const SVehiclePartEvent& event);

    virtual bool ChangeState(EVehiclePartState state, int flags = 0);
    virtual EVehiclePartState GetState() const { return m_state; }

    virtual void SetMaterial(_smart_ptr<IMaterial> pMaterial);
    virtual void Physicalize() {}
    virtual void SetMoveable(bool allowTranslationMovement = false) {}

    virtual const Matrix34& GetLocalTM(bool relativeToParentPart, bool forced = false);
    virtual const Matrix34& GetWorldTM();
    virtual void SetLocalTM(const Matrix34& localTM);
    virtual const AABB& GetLocalBounds();

    // set & get baseTM. for standard parts, this just forwards to LocalTM
    virtual const Matrix34& GetLocalBaseTM() { return GetLocalTM(true); }
    virtual void SetLocalBaseTM(const Matrix34& tm) { SetLocalTM(tm); }

    virtual void ResetLocalTM(bool recursive);

    virtual const Matrix34& GetLocalInitialTM() { return GetLocalTM(true); }

    virtual void Update(const float deltaTime);
    virtual void Serialize(TSerialize ser, EEntityAspects aspects);
    virtual void PostSerialize() {}

    virtual void RegisterSerializer(IGameObjectExtension* gameObjectExt) {}
    virtual int GetType(){ return m_pSharedParameters->m_typeId; }

    virtual IVehicleWheel* GetIWheel() { return NULL; }

    virtual const Vec3& GetDetachBaseForce() { return m_pSharedParameters->m_detachBaseForce; }
    virtual float GetDetachProbability() { return m_pSharedParameters->m_detachProbability; }
    virtual float GetMass() { return m_pSharedParameters->m_mass; }
    virtual int GetPhysId() { return m_physId; }
    virtual int GetSlot() const { return m_slot; }
    virtual int GetIndex() const { return m_index; }
    // ~IVehiclePart

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params){}

    virtual IStatObj* GetSubGeometry(CVehiclePartBase* pPart, EVehiclePartState state, Matrix34& position, bool removeFromParent);
    virtual IStatObj* GetStatObj() { return NULL; }
    virtual bool SetStatObj(IStatObj* pStatObj) {  return false; }
    virtual void GetGeometryName(EVehiclePartState state, string& name);

    virtual _smart_ptr<IMaterial> GetMaterial();

    // Is this part using external geometry
    virtual IStatObj* GetExternalGeometry(bool destroyed) { return NULL; }

    bool IsPhysicalized() { return m_pSharedParameters->m_isPhysicalized; }

    const Matrix34& LocalToVehicleTM(const Matrix34& localTM);
    EVehiclePartState GetMaxState();

    void CloneMaterial();

    bool IsRotationBlocked() const { return m_isRotationBlocked; }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
        GetMemoryUsageInternal(s);
    }

    void GetMemoryUsageInternal(ICrySizer* s) const;

    typedef std::vector<CVehiclePartBase*> TVehicleChildParts;
    const TVehicleChildParts& GetChildParts() const { return m_children; }

    static const char* ms_geometryDestroyedSuffixes[eVGS_Last], * ms_nullGeometryDestroyedSuffix;

    static inline const char* GetDestroyedGeometrySuffix(EVehiclePartState state)
    {
        if ((state >= 0) && (state < (sizeof(ms_geometryDestroyedSuffixes) / sizeof(ms_geometryDestroyedSuffixes[0]))))
        {
            return ms_geometryDestroyedSuffixes[state];
        }
        else
        {
            return ms_nullGeometryDestroyedSuffix;
        }
    }

public:

    enum EVehiclePartBaseEvent
    {
        eVPE_ParentPartUpdated = eVPE_OtherEvents,
    };

    virtual void Hide(bool hide);

protected:

    void ParsePhysicsParams(SEntityPhysicalizeParams& physicalizeParams, const CVehicleParams& table);

    bool ClampToRotationLimits(Ang3& angles);

    void CheckColltypeHeavy(int partid);
    void SetFlags(int partid, int flags, bool add);
    void SetFlagsCollider(int partid, int flagsCollider);

    EntityId SpawnEntity();
    int GetCGASlot(int jointId);

    float GetDamageSpeedMul();
    virtual EVehiclePartState GetStateForDamageRatio(float ratio);

    bool RegisterSharedParameters(IVehicle* pVehicle, const CVehicleParams& table, int partType);
    SSharedParams_PartsConstPtr m_pSharedParameters;

protected:
    AABB m_bounds;

    CVehicle* m_pVehicle;
    IVehiclePart* m_pParentPart;
    _smart_ptr<IMaterial> m_pClonedMaterial;

    int m_physId;

    // used to fade in/out opacity
    enum EVehiclePartHide
    {
        eVPH_NoFade = 0,
        eVPH_FadeIn,
        eVPH_FadeOut,
    };

    EVehiclePartState m_state;
    EVehiclePartHide m_hideMode;
    int m_hideCount : 8;    // if 0, not hidden. Allows multiple hide/unhide requests simultaneously.
    int m_slot : 8;

    int m_users : 7;
    bool m_isRotationBlocked : 1;

    TVehicleChildParts m_children;

    float m_damageRatio;

    float m_hideTimeMax;
    float m_hideTimeCount;
    int   m_index;

    friend class CVehiclePartSubPart;
    friend class CVehiclePartSubPartEntity;
    friend class CVehiclePartAnimatedJoint;
    friend class CVehicleSeatActionRotateTurret;
};

#if ENABLE_VEHICLE_DEBUG
namespace
{
    ILINE bool IsDebugParts()
    {
        return VehicleCVars().v_debugdraw == eVDB_Parts;
    }
}
#endif

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLEPARTBASE_H
