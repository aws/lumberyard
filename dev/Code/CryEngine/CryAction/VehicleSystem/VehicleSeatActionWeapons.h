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

// Description : Implements a seat action to control vehicle weapons


#ifndef CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONWEAPONS_H
#define CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONWEAPONS_H
#pragma once

struct IWeapon;

struct SWeaponAction
{
    std::vector<string> animations;

    void GetMemoryUsage(ICrySizer* s) const
    {
        s->AddObject(animations);
        for (size_t i = 0; i < animations.size(); i++)
        {
            s->Add(animations[i]);
        }
    }
};
typedef std::map<string, SWeaponAction> TWeaponActionMap;

typedef std::vector <IVehicleHelper*> TVehicleHelperVector;

struct SVehicleWeapon
{
    TWeaponActionMap actions;
    TVehicleHelperVector helpers;
    IEntityClass* pEntityClass;
    EntityId weaponEntityId;
    IVehiclePart* pPart;
    bool inheritVelocity;
    float m_respawnTime;

    TagID contextID;

    //Network dependency parent and child ID's
    EntityId networkChildId;
    EntityId networkParentId;

    SVehicleWeapon()
    {
        pEntityClass = NULL;
        weaponEntityId = 0;
        pPart = 0;
        inheritVelocity = true;
        m_respawnTime = 0.f;
        networkChildId = 0;
        networkParentId = 0;
        contextID = TAG_ID_INVALID;
    }

    void GetMemoryUsage(ICrySizer* s) const
    {
        s->AddObject(actions);
        s->AddObject(helpers);
    }
};


class CVehicleSeatActionWeapons
    : public IVehicleSeatAction
    , public IWeaponFiringLocator
    , public IWeaponEventListener
{
    IMPLEMENT_VEHICLEOBJECT

    enum EAttackInput
    {
        eAI_Primary =  0,
        eAI_Secondary,
    };

public:

    CVehicleSeatActionWeapons();
    virtual ~CVehicleSeatActionWeapons();

    // IVehicleSeatAction
    virtual bool Init(IVehicle* pVehicle, IVehicleSeat* pSeat, const CVehicleParams& table);
    virtual void OnSpawnComplete();
    virtual void Reset();
    virtual void Release() { delete this; }

    virtual void StartUsing(EntityId passengerId);
    void         StartUsing();

    virtual void ForceUsage();
    virtual void StopUsing();
    virtual void OnAction(const TVehicleActionId actionId, int activationMode, float value);

    virtual void Serialize(TSerialize ser, EEntityAspects aspects);
    virtual void PostSerialize();
    virtual void Update(const float deltaTime);
    virtual void UpdateFromPassenger(const float frameTime, EntityId playerId);

    virtual void GetMemoryUsage(ICrySizer* s) const;
    // ~IVehicleSeatAction

    // IWeaponFiringLocator
    virtual bool GetProbableHit(EntityId weaponId, const IFireMode* pFireMode, Vec3& hit);
    virtual bool GetFiringPos(EntityId weaponId, const IFireMode* pFireMode, Vec3& pos);
    virtual bool GetFiringDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos);
    virtual bool GetActualWeaponDir(EntityId weaponId, const IFireMode* pFireMode, Vec3& dir, const Vec3& probableHit, const Vec3& firingPos);
    virtual bool GetFiringVelocity(EntityId weaponId, const IFireMode* pFireMode, Vec3& vel, const Vec3& firingDir);
    virtual void WeaponReleased() {}
    // ~IWeaponFiringLocator

    // IWeaponEventListener
    virtual void OnShoot(IWeapon* pWeapon, EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType,
        const Vec3& pos, const Vec3& dir, const Vec3& vel);
    virtual void OnStartFire(IWeapon* pWeapon, EntityId shooterId){}
    virtual void OnStopFire(IWeapon* pWeapon, EntityId shooterId){}
    virtual void OnFireModeChanged(IWeapon* pWeapon, int currentFireMode) {}
    virtual void OnStartReload(IWeapon* pWeapon, EntityId shooterId, IEntityClass* pAmmoType){}
    virtual void OnEndReload(IWeapon* pWeapon, EntityId shooterId, IEntityClass* pAmmoType){}
    virtual void OnSetAmmoCount(IWeapon* pWeapon, EntityId shooterId) {};
    virtual void OnOutOfAmmo(IWeapon* pWeapon, IEntityClass* pAmmoType);
    virtual void OnReadyToFire(IWeapon* pWeapon){}
    virtual void OnPickedUp(IWeapon* pWeapon, EntityId actorId, bool destroyed){}
    virtual void OnDropped(IWeapon* pWeapon, EntityId actorId){}
    virtual void OnMelee(IWeapon* pWeapon, EntityId shooterId) {}
    virtual void OnStartTargetting(IWeapon* pWeapon) {}
    virtual void OnStopTargetting(IWeapon* pWeapon) {}
    virtual void OnSelected(IWeapon* pWeapon, bool select) {}
    virtual void OnEndBurst(IWeapon* pWeapon, EntityId shooterId) {}
    // ~IWeaponEventListener

    void BindWeaponToNetwork(EntityId weaponId);
    Vec3 GetAverageFiringPos();

    virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);

    void StartFire();
    void StopFire();
    bool IsFiring() const { return m_isShooting; }
    void SetFireTarget(const Vec3& target) { m_fireTarget = target; }

    bool IsSecondary() const { return m_isSecondary; }

    bool CanFireWeapon() const;

    unsigned int GetWeaponCount() const;
    unsigned int GetCurrentWeapon() const { return m_weaponIndex; }

    EntityId GetCurrentWeaponEntityId() const;
    EntityId GetWeaponEntityId(unsigned int index) const;
    void ClSetupWeapon(unsigned int index, EntityId weaponId);

    IItem* GetIItem(unsigned int index);
    IWeapon* GetIWeapon(unsigned int index);

    IItem* GetIItem(const SVehicleWeapon& vehicleWeapon);
    IWeapon* GetIWeapon(const SVehicleWeapon& vehicleWeapon);

    SVehicleWeapon*   GetVehicleWeapon    (EntityId weaponId);
    bool              ForcedUsage         () const { return m_Forced; }

    ILINE bool IsMounted() const
    {
        return m_isMounted;
    }

    void OnWeaponRespawned(int weaponIndex, EntityId newWeaponEntityId);
    void OnVehicleActionControllerDeleted();

protected:

    IEntity* SpawnWeapon(SVehicleWeapon& weapon, IEntity* pVehicleEntity, const string& partName, int weaponIndex);
    void SetupWeapon(SVehicleWeapon& weapon);

    SVehicleWeapon& GetVehicleWeapon();
    SVehicleWeapon& GetWeaponInfo(int weaponIndex);

    IVehicleHelper* GetHelper(SVehicleWeapon& vehicleWeapon);
    IActor* GetUserActor();

protected:

    int GetSkipEntities(IPhysicalEntity** pSkipEnts, int nMaxSkip);

    void DoUpdate(const float deltaTime);

    virtual void UpdateWeaponTM(SVehicleWeapon& weapon);
    IEntity* GetEntity(const SVehicleWeapon& weapon);

    bool CanTriggerActionFire(const TVehicleActionId actionId) const;
    bool CanTriggerActionFiremode(const TVehicleActionId actionId) const;
    bool CanTriggerActionZoom(const TVehicleActionId actionId) const;

    IVehicle* m_pVehicle;
    string m_partName;
    IVehicleSeat* m_pSeat;

    EntityId m_passengerId;

    typedef std::vector <SVehicleWeapon> TVehicleWeaponVector;
    TVehicleWeaponVector m_weapons;
    TVehicleWeaponVector m_weaponsCopy;
    Vec3 m_fireTarget;

    int m_lastActionActivationMode;
    int m_weaponIndex;

    float m_shotDelay;
    float m_nextShotTimer;
    float m_respawnTimer;

    EAttackInput m_attackInput;

    bool m_isUsingShootingByUpdate;
    bool m_isSecondary;
    bool m_isShootingToCrosshair;
    bool m_isShooting;
    bool m_Forced;
    bool m_isMounted;
};

#endif // CRYINCLUDE_CRYACTION_VEHICLESYSTEM_VEHICLESEATACTIONWEAPONS_H
