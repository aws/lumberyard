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

// Description : Actor System interfaces.


#ifndef CRYINCLUDE_CRYACTION_IACTORSYSTEM_H
#define CRYINCLUDE_CRYACTION_IACTORSYSTEM_H
#pragma once

#include <IEntity.h>
#include <IEntitySystem.h>
#include <IScriptSystem.h>
#include "IGameObjectSystem.h"
#include "IGameObject.h"

enum EActorPhysicalization
{
    eAP_NotPhysicalized,
    eAP_Alive,
    eAP_Ragdoll,
    eAP_Sleep,
    eAP_Frozen,
    eAP_Linked,
    eAP_Spectator,
};

struct IActor;
struct IActionListener;
struct IAnimationGraphState;
struct SViewParams;
class CGameObject;
struct IGameObjectExtension;
struct IInventory;
struct IAnimatedCharacter;
struct ICharacterInstance;
struct AnimEventInstance;
struct IAIAction;
struct pe_params_rope;

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

typedef int ActorClass;

struct IActor
    : public IGameObjectExtension
{
    // This interface will have multiple derived subclasses, so it should not
    // have a type declared here. There is no specific instantiable class that is always
    // the right one to instantiate when asking for an Actor.

    IActor() {}

    virtual ~IActor(){}

    //////////////////////////////////////////////////////////////////////////
    // Mandatory user implementation

    virtual bool IsPlayer() const = 0;
    virtual bool IsClient() const = 0;
    /// ActorClassName is used as a lookup in systems like IActorSystem for ActorParams.
    virtual const char* GetActorClassName() const = 0;
    /// EntityClassName is used as a lookup in systems like the pool manager,
    //// and for printing debug messages.
    virtual const char* GetEntityClassName() const = 0;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Default Method implementations. Can be overridden if necessary

    /// In the engine, ActorClass is used by the CentralInterestManager, PersonalInterestManager, and IActorSystem.
    virtual ActorClass GetActorClass() const { return 0; }

    virtual void    SetHealth(float health) { }
    virtual float   GetHealth() const { return 0; }
    virtual int     GetHealthAsRoundedPercentage() const { return 0; }
    virtual void    SetMaxHealth(float maxHealth) { }
    virtual float   GetMaxHealth() const { return 0; }
    virtual int     GetArmor() const { return 0; }
    virtual int     GetMaxArmor() const { return 0; }

    virtual int     GetTeamId() const { return 0; }

    virtual bool    IsFallen() const { return false; }
    virtual bool    IsDead() const { return false; }
    virtual int     IsGod() { return 0; }
    virtual void    Fall(Vec3 hitPos = Vec3(0, 0, 0)) { }
    virtual bool    AllowLandingBob() { return false; }

    virtual void    PlayAction(const char* action, const char* extension, bool looping = false) { }
    virtual IAnimationGraphState* GetAnimationGraphState() { return nullptr; }
    virtual void    ResetAnimationState() { }

    virtual void CreateScriptEvent(const char* event, float value, const char* str = NULL) { }

    virtual bool BecomeAggressiveToAgent(EntityId entityID) { return false; }

    virtual void SetFacialAlertnessLevel(int alertness) { }
    virtual void RequestFacialExpression(const char* pExpressionName = NULL, f32* sequenceLength = NULL) { }
    virtual void PrecacheFacialExpression(const char* pExpressionName) { }

    virtual EntityId GetGrabbedEntityId() const { return INVALID_ENTITYID; }

    virtual void HideAllAttachments(bool isHiding) { }

    virtual void SetIKPos(const char* pLimbName, const Vec3& goalPos, int priority) { }

    virtual void SetViewInVehicle(Quat viewRotation) { }
    virtual void SetViewRotation(const Quat& rotation) { }
    virtual Quat GetViewRotation() const { return Quat(); }

    virtual bool IsFriendlyEntity(EntityId entityId, bool bUsingAIIgnorePlayer = true) const { return true; }

    virtual Vec3 GetLocalEyePos() const { return Vec3(); }

    virtual void CameraShake(float angle, float shift, float duration, float frequency, Vec3 pos, int ID, const char* source = "") { }

    virtual IItem* GetHolsteredItem() const { return nullptr; }
    virtual void HolsterItem(bool holster, bool playSelect = true, float selectSpeedBias = 1.0f, bool hideLeftHandObject = true) { }
    virtual IItem* GetCurrentItem(bool includeVehicle = false) const { return nullptr; }
    virtual bool DropItem(EntityId itemId, float impulseScale = 1.0f, bool selectNext = true, bool byDeath = false) { return false; }
    virtual IInventory* GetInventory() const { return nullptr; }
    virtual void NotifyCurrentItemChanged(IItem* newItem) { }

    virtual IMovementController* GetMovementController() const { return nullptr; }

    // get currently linked vehicle, or NULL
    virtual IEntity* LinkToVehicle(EntityId vehicleId) { return nullptr; }

    virtual IEntity* GetLinkedEntity() const { return nullptr; }

    virtual uint8 GetSpectatorMode() const { return 0; }

    virtual bool IsThirdPerson() const { return false; }
    virtual void ToggleThirdPerson() { }

    virtual bool IsStillWaitingOnServerUseResponse() const { return false; }
    virtual void SetStillWaitingOnServerUseResponse(bool waiting) {}

    virtual void SetFlyMode(uint8 flyMode) {};
    virtual uint8 GetFlyMode() const { return 0; };

    virtual bool IsMigrating() const { return false; }
    virtual void SetMigrating(bool isMigrating) { }

    virtual void InitLocalPlayer() { }

    virtual void  SerializeXML(XmlNodeRef& node, bool bLoading) { }
    virtual void  SerializeLevelToLevel(TSerialize& ser) { }

    virtual IAnimatedCharacter* GetAnimatedCharacter() { return nullptr; }
    virtual const IAnimatedCharacter* GetAnimatedCharacter() const { return nullptr; }
    virtual void PlayExactPositioningAnimation(const char* sAnimationName, bool bSignal, const Vec3& vPosition, const Vec3& vDirection, float startWidth, float startArcAngle, float directionTolerance) { }
    virtual void CancelExactPositioningAnimation() { }
    virtual void PlayAnimation(const char* sAnimationName, bool bSignal) { }

    // Respawn the actor to some intial data (used by CheckpointSystem)
    virtual bool Respawn()
    {
        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Use of IActor::Respawn when not implemented!");
        return false;
    }

    // Reset the actor to its initial location (used by CheckpointSystem)
    virtual void ResetToSpawnLocation()
    {
        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Use of IActor::ResetToSpawnLocation when not implemented!");
    }

    // Can the actor currently break through glass (used by ActionGame)
    virtual bool CanBreakGlass() const
    {
        return false;
    }

    // Forces the actor break through glass if allowed (used by ActionGame)
    virtual bool MustBreakGlass() const
    {
        return false;
    }

    // Enables/Disables time demo mode.
    // Some movement/view direction things work differently when running under time demo.
    virtual void EnableTimeDemo(bool bTimeDemo) { }

    ChannelId GetChannelId() const
    {
        return GetGameObject()->GetChannelId();
    }
    void SetChannelId(ChannelId id)
    {
        GetGameObject()->SetChannelId(id);
    }

    virtual void SwitchDemoModeSpectator(bool activate) { }

    virtual void SetCustomHead(const char* customHead) { }

    // IVehicle
    virtual IVehicle* GetLinkedVehicle() const { return nullptr; }

    virtual void OnAIProxyEnabled(bool enabled) { }
    virtual void OnReturnedToPool() { }
    virtual void OnPreparedFromPool() { }
    virtual void OnShiftWorld() {}

    virtual void MountedGunControllerEnabled(bool val) {}
    virtual bool MountedGunControllerEnabled() const { return false; }

    virtual bool ShouldMuteWeaponSoundStimulus() const { return false; }

    // Populates list of physics entities to skip for IK raycasting. returns num Entities to skip
    virtual int GetPhysicalSkipEntities(IPhysicalEntity** pSkipList, const int maxSkipSize) const { return 0; }

    virtual void OnReused(IEntity* pEntity, SEntitySpawnParams& params) { }
    //////////////////////////////////////////////////////////////////////////
};


struct IActorIterator
{
    virtual ~IActorIterator(){}
    virtual size_t  Count() = 0;
    virtual IActor* Next() = 0;
    virtual void    AddRef() = 0;
    virtual void    Release() = 0;
};
typedef _smart_ptr<IActorIterator> IActorIteratorPtr;

struct IItemParamsNode;

struct IActorSystem
{
    virtual ~IActorSystem(){}
    virtual void  Reset() = 0;
    virtual void Reload() = 0;
    virtual IActor* GetActor(EntityId entityId) = 0;
    virtual IActor* CreateActor(ChannelId channelId, const char* name, const char* actorClass, const Vec3& pos, const Quat& rot, const Vec3& scale, EntityId id = 0, TSerialize* serializer = nullptr) = 0;
    virtual IActor* GetActorByChannelId(ChannelId channelId) = 0;

    virtual int GetActorCount() const = 0;
    virtual IActorIteratorPtr CreateActorIterator() = 0;

    virtual void SwitchDemoSpectator(EntityId id = 0) = 0;
    virtual IActor* GetCurrentDemoSpectator() = 0;
    virtual IActor* GetOriginalDemoSpectator() = 0;

    virtual void AddActor(EntityId entityId, IActor* pActor) = 0;
    virtual void RemoveActor(EntityId entityId) = 0;

    virtual void Scan(const char* folderName) = 0;
    virtual bool ScanXML(const XmlNodeRef& root, const char* xmlFile) = 0;
    virtual const IItemParamsNode* GetActorParams(const char* actorClass) const = 0;

    virtual bool IsActorClass(IEntityClass* pClass) const = 0;
};


#endif // CRYINCLUDE_CRYACTION_IACTORSYSTEM_H
