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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_ENTITY_H
#define CRYINCLUDE_CRYENTITYSYSTEM_ENTITY_H
#pragma once

#include <IEntity.h>
#include <unordered_map>
#include <CharacterBoundsBus.h>
#include "IComponent.h"

//////////////////////////////////////////////////////////////////////////
// These headers cannot be replaced with forward references.
// They are needed for correct up casting to real component classes.
#include "Components/ComponentRender.h"
#include "Components/ComponentPhysics.h"
#include "Components/ComponentScript.h"
#include "Components/ComponentSubstitution.h"
//////////////////////////////////////////////////////////////////////////

// forward declarations.
struct IEntitySystem;
struct IEntityArchetype;
class CEntitySystem;
struct AIObjectParams;
struct SGridLocation;
struct SProximityElement;
struct SEntityLoadParams;

// (MATT) This should really live in a minimal AI include, which right now we don't have  {2009/04/08}
#ifndef INVALID_AIOBJECTID
typedef uint32  tAIObjectID;
    #define INVALID_AIOBJECTID ((tAIObjectID)(0))
#endif

//////////////////////////////////////////////////////////////////////
class CEntity
    : public IEntity
    , private AZ::CharacterBoundsNotificationBus::Handler
{
    // Entity constructor.
    // Should only be called from Entity System.
    CEntity(SEntitySpawnParams& params);

public:
    using TComponentPair = AZStd::pair<const ComponentType, IComponentPtr>;

    // Entity destructor.
    // Should only be called from Entity System.
    virtual ~CEntity();

    IEntitySystem* GetEntitySystem() const;

    // Called by entity system to complete initialization of the entity.
    bool Init(SEntitySpawnParams& params);
    // Called by EntitySystem every frame for each pre-physics active entity.
    void PrePhysicsUpdate(float fFrameTime);
    // Called by EntitySystem every frame for each active entity.
    void Update(SEntityUpdateContext& ctx);
    // Called by EntitySystem before entity is destroyed.
    void ShutDown(bool bRemoveAI = true, bool bRemoveComponents = true);

    //////////////////////////////////////////////////////////////////////////
    // IEntity interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual EntityId GetId() const { return m_nID; }
    virtual EntityGUID GetGuid() const { return m_guid; }

    //////////////////////////////////////////////////////////////////////////
    virtual IEntityClass* GetClass() const { return m_pClass; }
    virtual IEntityArchetype* GetArchetype() { return m_pArchetype; }

    //////////////////////////////////////////////////////////////////////////
    virtual void SetFlags(uint32 flags);
    virtual uint32 GetFlags() const { return m_flags; }
    virtual void AddFlags(uint32 flagsToAdd) { SetFlags(m_flags | flagsToAdd); }
    virtual void ClearFlags(uint32 flagsToClear) { SetFlags(m_flags & (~flagsToClear)); }
    virtual bool CheckFlags(uint32 flagsToCheck) const { return (m_flags & flagsToCheck) == flagsToCheck; }

    virtual void SetFlagsExtended(uint32 flags) { m_flagsExtended = flags; }
    virtual uint32 GetFlagsExtended() const { return m_flagsExtended; }

    virtual bool IsGarbage() const { return m_bGarbage; }
    virtual bool IsLoadedFromLevelFile() const { return m_bLoadedFromLevelFile; }
    void SetLoadedFromLevelFile(bool bIsLoadedFromLevelFile) override { m_bLoadedFromLevelFile = bIsLoadedFromLevelFile; }

    //////////////////////////////////////////////////////////////////////////
    virtual void SetName(const char* sName);
    virtual const char* GetName() const { return m_szName.c_str(); }
    virtual const char* GetEntityTextDescription() const;

    //////////////////////////////////////////////////////////////////////////
    virtual void AttachChild(IEntity* pChildEntity, const SChildAttachParams& attachParams);
    virtual void DetachAll(int nDetachFlags = 0);
    virtual void DetachThis(int nDetachFlags = 0, int nWhyFlags = 0);
    virtual int  GetChildCount() const;
    virtual IEntity* GetChild(int nIndex) const;
    void EnableInheritXForm(bool bEnable) override;
    virtual IEntity* GetParent() const { return (m_pBinds) ? m_pBinds->pParent : NULL; }
    virtual Matrix34 GetParentAttachPointWorldTM() const;
    virtual bool IsParentAttachmentValid() const;
    const IEntity* GetRoot() const override
    {
        const IEntity* pParent, * pRoot = this;
        while (pParent = pRoot->GetParent())
        {
            pRoot = pParent;
        }
        return pRoot;
    }
    IEntity* GetRoot() override { return const_cast<IEntity*>(static_cast<const CEntity*>(this)->GetRoot()); }

    //////////////////////////////////////////////////////////////////////////
    virtual void SetWorldTM(const Matrix34& tm, int nWhyFlags = 0);
    virtual void SetLocalTM(const Matrix34& tm, int nWhyFlags = 0);

    // Set position rotation and scale at once.
    virtual void SetPosRotScale(const Vec3& vPos, const Quat& qRotation, const Vec3& vScale, int nWhyFlags = 0);

    virtual Matrix34 GetLocalTM() const;
    virtual const Matrix34& GetWorldTM() const { return m_worldTM; }

    virtual void GetWorldBounds(AABB& bbox) const;
    virtual void GetLocalBounds(AABB& bbox) const;
    void InvalidateBounds() override;

    //////////////////////////////////////////////////////////////////////////
    virtual void SetPos(const Vec3& vPos, int nWhyFlags = 0, bool bRecalcPhyBounds = false, bool bForce = false);
    virtual const Vec3& GetPos() const { return m_vPos; }

    virtual void SetRotation(const Quat& qRotation, int nWhyFlags = 0);
    virtual const Quat& GetRotation() const { return m_qRotation; }

    virtual void SetScale(const Vec3& vScale, int nWhyFlags = 0);
    virtual const Vec3& GetScale() const { return m_vScale; }

    virtual Vec3 GetWorldPos() const { return m_worldTM.GetTranslation(); }
    virtual Ang3 GetWorldAngles() const;
    virtual Quat GetWorldRotation() const;
    virtual const Vec3& GetForwardDir() const { ComputeForwardDir(); return m_vForwardDir; }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    virtual void Activate(bool bActive);
    virtual bool IsActive() const { return m_bActive; }

    virtual bool IsFromPool() const { return IsPoolControlled(); }

    virtual void PrePhysicsActivate(bool bActive);
    virtual bool IsPrePhysicsActive();

    //////////////////////////////////////////////////////////////////////////
    virtual void Serialize(TSerialize ser, int nFlags);

    virtual bool SendEvent(SEntityEvent& event);
    //////////////////////////////////////////////////////////////////////////

    virtual void SetTimer(int nTimerId, int nMilliSeconds);
    virtual void KillTimer(int nTimerId);

    virtual void Hide(bool bHide);
    virtual bool IsHidden() const { return m_bHidden; }

    virtual void Invisible(bool bInvisible);
    virtual bool IsInvisible() const { return m_bInvisible; }

    //////////////////////////////////////////////////////////////////////////
    virtual IAIObject* GetAI() { return (m_aiObjectID ? GetAIObject() : NULL); }
    virtual bool HasAI() const { return m_aiObjectID != 0; }
    virtual tAIObjectID GetAIObjectID() const { return m_aiObjectID; }
    virtual void SetAIObjectID(tAIObjectID id) { m_aiObjectID = id; }
    //////////////////////////////////////////////////////////////////////////
    virtual bool RegisterInAISystem(const AIObjectParams& params);
    //////////////////////////////////////////////////////////////////////////
    // reflect changes in position or orientation to the AI object
    void UpdateAIObject();
    //////////////////////////////////////////////////////////////////////////

    virtual void SetUpdatePolicy(EEntityUpdatePolicy eUpdatePolicy);
    virtual EEntityUpdatePolicy GetUpdatePolicy() const { return (EEntityUpdatePolicy)m_eUpdatePolicy; }

    //////////////////////////////////////////////////////////////////////////
    bool RegisterComponent(IComponentPtr pComponent) override;
    bool DeregisterComponent(IComponentPtr pComponent) override;

    //////////////////////////////////////////////////////////////////////////
    // Physics.
    //////////////////////////////////////////////////////////////////////////
    virtual void Physicalize(SEntityPhysicalizeParams& params);
    virtual void EnablePhysics(bool enable);
    virtual IPhysicalEntity* GetPhysics() const;

    virtual int PhysicalizeSlot(int slot, SEntityPhysicalizeParams& params);
    virtual void UnphysicalizeSlot(int slot);
    virtual void UpdateSlotPhysics(int slot);

    virtual void SetPhysicsState(XmlNodeRef& physicsState);

    //////////////////////////////////////////////////////////////////////////
    // Audio & Physics
    //////////////////////////////////////////////////////////////////////////
    virtual float GetObstructionMultiplier() const;
    virtual void SetObstructionMultiplier(float obstructionMultiplier);

    //////////////////////////////////////////////////////////////////////////
    // Custom entity material.
    //////////////////////////////////////////////////////////////////////////
    void SetMaterial(_smart_ptr<IMaterial> pMaterial) override;
    virtual _smart_ptr<IMaterial> GetMaterial();

    //////////////////////////////////////////////////////////////////////////
    // Entity Slots interface.
    //////////////////////////////////////////////////////////////////////////
    virtual bool IsSlotValid(int nSlot) const;
    virtual void FreeSlot(int nSlot);
    virtual int  GetSlotCount() const;
    virtual bool GetSlotInfo(int nSlot, SEntitySlotInfo& slotInfo) const;
    virtual const Matrix34& GetSlotWorldTM(int nIndex) const;
    virtual const Matrix34& GetSlotLocalTM(int nIndex, bool bRelativeToParent) const;
    virtual void SetSlotLocalTM(int nIndex, const Matrix34& localTM, int nWhyFlags = 0);
    virtual void SetSlotCameraSpacePos(int nIndex, const Vec3& cameraSpacePos);
    virtual void GetSlotCameraSpacePos(int nSlot, Vec3& cameraSpacePos) const;
    virtual bool SetParentSlot(int nParentSlot, int nChildSlot);
    void SetSlotMaterial(int nSlot, _smart_ptr<IMaterial> pMaterial) override;
    virtual void SetSlotFlags(int nSlot, uint32 nFlags);
    virtual uint32 GetSlotFlags(int nSlot) const;
    virtual bool ShouldUpdateCharacter(int nSlot) const;
    virtual ICharacterInstance* GetCharacter(int nSlot);
    virtual int SetCharacter(ICharacterInstance* pCharacter, int nSlot);
    virtual IStatObj* GetStatObj(int nSlot);
    virtual int SetStatObj(IStatObj* pStatObj, int nSlot, bool bUpdatePhysics, float mass = -1.0f);
    virtual IParticleEmitter* GetParticleEmitter(int nSlot);
    virtual IGeomCacheRenderNode* GetGeomCacheRenderNode(int nSlot);

    virtual void MoveSlot(IEntity* targetIEnt, int nSlot);

    virtual int LoadGeometry(int nSlot, const char* sFilename, const char* sGeomName = NULL, int nLoadFlags = 0);
    virtual int LoadCharacter(int nSlot, const char* sFilename, int nLoadFlags = 0);
#if defined(USE_GEOM_CACHES)
    virtual int LoadGeomCache(int nSlot, const char* sFilename);
#endif
    virtual int SetParticleEmitter(int nSlot, IParticleEmitter* pEmitter, bool bSerialize = false);
    virtual int LoadParticleEmitter(int nSlot, IParticleEffect* pEffect, SpawnParams const* params = NULL, bool bPrime = false, bool bSerialize = false);
    virtual int LoadLight(int nSlot, CDLight* pLight);

    bool UpdateLightClipVolumes(CDLight& light) override;
    int LoadCloud(int nSlot, const char* sFilename) override;
    int SetCloudMovementProperties(int nSlot, const SCloudMovementProperties& properties) override;
    int LoadFogVolume(int nSlot, const SFogVolumeProperties& properties) override;

    int FadeGlobalDensity(int nSlot, float fadeTime, float newGlobalDensity) override;
    int LoadVolumeObject(int nSlot, const char* sFilename) override;
    int SetVolumeObjectMovementProperties(int nSlot, const SVolumeObjectMovementProperties& properties) override;

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
    int LoadPrismObject(int nSlot) override;
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

    virtual void InvalidateTM(int nWhyFlags = 0, bool bRecalcPhyBounds = false);

    // Load/Save entity parameters in XML node.
    virtual void SerializeXML(XmlNodeRef& node, bool bLoading);

    virtual IEntityLink* GetEntityLinks();
    virtual IEntityLink* AddEntityLink(const char* sLinkName, EntityId entityId, EntityGUID entityGuid = 0);
    virtual void RemoveEntityLink(IEntityLink* pLink);
    virtual void RemoveAllEntityLinks();
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    virtual IEntity* UnmapAttachedChild(int& partId);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const;

    //////////////////////////////////////////////////////////////////////////
    // Local methods.
    //////////////////////////////////////////////////////////////////////////

    // Returns true if entity was already fully initialized by this point.
    virtual bool IsInitialized() const { return m_bInitialized; }

    virtual void DebugDraw(const SGeometryDebugDrawInfo& info);
    //////////////////////////////////////////////////////////////////////////

    // Get fast access to the slot, only used internally by entity components.
    class CEntityObject* GetSlot(int nSlot) const;

    // Specializations for the EntityPool
    bool GetSignature(TSerialize& signature) override;
    void SerializeXML_ExceptScriptComponent(XmlNodeRef& node, bool bLoading) override;
    //////////////////////////////////////////////////////////////////////////

    // For internal use.
    CEntitySystem* GetCEntitySystem() const { return g_pIEntitySystem; }

    //////////////////////////////////////////////////////////////////////////
    bool ReloadEntity(SEntityLoadParams& loadParams);

    void SetPoolControl(bool bSet);
    bool IsPoolControlled() const { return m_bIsFromPool; }

    //////////////////////////////////////////////////////////////////////////
    // Activates entity only for specified number of frames.
    // numUpdates must be a small number from 0-15.
    void ActivateForNumUpdates(uint32 numUpdates) override;
    void SetUpdateStatus();
    // Get status if entity need to be update every frame or not.
    bool GetUpdateStatus() const { return (m_bActive || m_nUpdateCounter) && (!m_bHidden || CheckFlags(ENTITY_FLAG_UPDATE_HIDDEN)); }

    //////////////////////////////////////////////////////////////////////////
    // Description:
    //   Invalidates entity and childs cached world transformation.
    void CalcLocalTM(Matrix34& tm) const;
    void Hide(bool bHide, EEntityEvent eEvent1, EEntityEvent eEvent2);

    // for ProximityTriggerSystem
    SProximityElement* GetProximityElement() override { return m_pProximityEntity; }
    void SetProximityElement(SProximityElement* pElement) override { m_pProximityEntity = pElement; }
    void SetTrigger(bool bTrigger) override { m_bTrigger = bTrigger; }

    virtual void IncKeepAliveCounter()  { m_nKeepAliveCounter++; }
    virtual void DecKeepAliveCounter()  { assert(m_nKeepAliveCounter > 0); m_nKeepAliveCounter--; }
    virtual void ResetKeepAliveCounter() {m_nKeepAliveCounter = 0; }
    virtual bool IsKeptAlive() const { return m_nKeepAliveCounter > 0; }

    static void ClearStaticData() { stl::free_container(m_szDescription); }

    void CheckMaterialFlags();

    bool IsInActiveList() const override { return m_bInActiveList; }

    void SetCloneLayerId(int cloneLayerId) override { m_cloneLayerId = cloneLayerId; }
    int GetCloneLayerId() const override { return m_cloneLayerId; }

    //////////////////////////////////////////////////////////////////////////
    // CharacterBoundsNotificationBus
    void OnCharacterBoundsReset() override;
    //////////////////////////////////////////////////////////////////////////


protected:

    //////////////////////////////////////////////////////////////////////////
    // Attachment.
    //////////////////////////////////////////////////////////////////////////
    void AllocBindings();
    void DeallocBindings();
    void OnRellocate(int nWhyFlags);
    void LogEvent(SEntityEvent& event, CTimeValue dt);
    //////////////////////////////////////////////////////////////////////////

private:
    void ComputeForwardDir() const;
    bool IsScaled(float threshold = 0.0003f) const
    {
        return (fabsf(m_vScale.x - 1.0f) + fabsf(m_vScale.y - 1.0f) + fabsf(m_vScale.z - 1.0f)) >= threshold;
    }
    // Fetch the IA object from the AIObjectID, if any
    IAIObject* GetAIObject();

    // Run function on each element in m_components.
    // Assert if components are added or removed during iteration.
    template<class Function>
    void ForEachComponent(Function fn) const;

    // Run function on each element in m_components.
    // It is ok if components are added/removed while iterating.
    // This is less efficient than using ForEachComponent(fn).
    // Components added during iteration will not have function called upon them.
    template<class Function>
    void ForEachComponent_AllowDestroyCreate(Function fn);

private:
    //////////////////////////////////////////////////////////////////////////
    // VARIABLES.
    //////////////////////////////////////////////////////////////////////////
    friend class CEntitySystem;

    // Childs structure, only allocated when any child entity is attached.
    struct SBindings
    {
        enum EBindingType
        {
            eBT_Pivot,
            eBT_GeomCacheNode,
            eBT_CharacterBone,
        };

        SBindings()
            : pParent(NULL)
            , parentBindingType(eBT_Pivot) {}

        std::vector<CEntity*> childs;
        CEntity* pParent;
        EBindingType parentBindingType;
    };

    //////////////////////////////////////////////////////////////////////////
    // Internal entity flags (must be first member var of Centity) (Reduce cache misses on access to entity data).
    //////////////////////////////////////////////////////////////////////////
    unsigned int m_bActive : 1;                 // Active Entities are updated every frame.
    unsigned int m_bInActiveList : 1;           // Added to entity system active list.
    mutable unsigned int m_bBoundsValid : 1;    // Set when the entity bounding box is valid.
    unsigned int m_bInitialized : 1;                  // Set if this entity already Initialized.
    unsigned int m_bHidden : 1;                 // Set if this entity is hidden.
    unsigned int m_bGarbage : 1;                // Set if this entity is garbage and will be removed soon.
    unsigned int m_bHaveEventListeners : 1;     // Set if entity have an event listeners associated in entity system.
    unsigned int m_bTrigger : 1;                // Set if entity is proximity trigger itself.
    unsigned int m_bWasRellocated : 1;          // Set if entity was rellocated at least once.
    unsigned int m_nUpdateCounter : 4;          // Update counter is used to activate the entity for the limited number of frames.
                                                // Usually used for Physical Triggers.
                                                // When update counter is set, and entity is activated, it will automatically
                                                // deactivate after this counter reaches zero
    unsigned int m_eUpdatePolicy : 4;           // Update policy defines in which cases to call
                                                // entity update function every frame.
    unsigned int m_bInvisible : 1;               // Set if this entity is invisible.
    unsigned int m_bNotInheritXform : 1;        // Inherit or not transformation from parent.
    unsigned int m_bInShutDown : 1;             // Entity is being shut down.

    unsigned int m_bIsFromPool : 1;                         // Set if entity was created through the pool system
    mutable bool m_bDirtyForwardDir : 1;                // Cached world transformed forward vector
    unsigned int m_bLoadedFromLevelFile : 1;    // Entitiy was loaded from level file

    mutable bool m_bAllowDestroyOrCreateComponent; // Entity is allowed to create or destroy components

    // Unique ID of the entity.
    EntityId m_nID;

    // Optional entity guid.
    EntityGUID m_guid;

    // Entity flags. any combination of EEntityFlags values.
    uint32 m_flags;

    // Entity extended flags. any combination of EEntityFlagsExtended values.
    uint32 m_flagsExtended;

    // Description of the entity, generated on the fly.
    static string m_szDescription;

    // Pointer to the class that describe this entity.
    IEntityClass* m_pClass;

    // Pointer to the entity archetype.
    IEntityArchetype* m_pArchetype;

    // Position of the entity in local space.
    Vec3 m_vPos;
    // Rotation of the entity in local space.
    Quat m_qRotation;
    // Scale of the entity in local space.
    Vec3 m_vScale;
    // World transformation matrix of this entity.
    mutable Matrix34 m_worldTM;

    mutable Vec3 m_vForwardDir;

    // Pointer to hierarchical binding structure.
    // It contains array of child entities and pointer to the parent entity.
    // Because entity attachments are not used very often most entities do not need it,
    // and space is preserved by keeping it separate structure.
    SBindings* m_pBinds;

    // The representation of this entity in the AI system.
    tAIObjectID m_aiObjectID;

    // Custom entity material.
    _smart_ptr<IMaterial> m_pMaterial;

    //////////////////////////////////////////////////////////////////////////
    using TComponents = AZStd::unordered_map<ComponentType, IComponentPtr>;
    TComponents m_components;

    // Entity Links.
    IEntityLink* m_pEntityLinks;

    // For tracking entity in the partition grid.
    SGridLocation* m_pGridLocation;
    // For tracking entity inside proximity trigger system.
    SProximityElement* m_pProximityEntity;

    // counter to prevent deletion if entity is processed deferred by for example physics events
    uint32 m_nKeepAliveCounter;

    // Name of the entity.
    string m_szName;

    //! Gets the entity's component for the specified type.
    //! \return Returns nullptr if the component is not registered.
    IComponentPtr GetComponentImpl(const ComponentType& type) override;
    IComponentConstPtr GetComponentImpl(const ComponentType& type) const override;

    // If this entity is part of a layer that was cloned at runtime, this is the cloned layer
    // id (not related to the layer id)
    int m_cloneLayerId;
};

//////////////////////////////////////////////////////////////////////////
void ILINE CEntity::ComputeForwardDir() const
{
    if (m_bDirtyForwardDir)
    {
        if (IsScaled())
        {
            Matrix34 auxTM = m_worldTM;
            auxTM.OrthonormalizeFast();

            // assuming (0, 1, 0) as the local forward direction
            m_vForwardDir = auxTM.GetColumn1();
        }
        else
        {
            // assuming (0, 1, 0) as the local forward direction
            m_vForwardDir = m_worldTM.GetColumn1();
        }

        m_bDirtyForwardDir = false;
    }
}

//////////////////////////////////////////////////////////////////////////
template<class Function>
void ILINE CEntity::ForEachComponent(Function fn) const
{
    // Detect modifications to m_components that occur during iteration.
    m_bAllowDestroyOrCreateComponent = false;
    AZStd::for_each(m_components.begin(), m_components.end(), fn);
    m_bAllowDestroyOrCreateComponent = true;
}

//////////////////////////////////////////////////////////////////////////
template<class Function>
void ILINE CEntity::ForEachComponent_AllowDestroyCreate(Function fn)
{
    // Iterate over a copy of m_components so that insert/remove doesn't break iteration.
    // Copy is a vector (rather than map) because we just iterate and don't do random lookups.
    std::vector<TComponentPair> tempComponents;
    tempComponents.reserve(m_components.size());
    for (const auto& pair : m_components)
    {
        tempComponents.emplace_back(pair);
    }

    for (const TComponentPair& pair : tempComponents)
    {
        // Don't call fn on component if it's been removed from m_components
        if (m_components.find(pair.first) != m_components.end())
        {
            fn(pair);
        }
    }
}
#endif // CRYINCLUDE_CRYENTITYSYSTEM_ENTITY_H
