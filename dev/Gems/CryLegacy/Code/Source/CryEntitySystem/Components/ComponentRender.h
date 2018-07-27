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
#ifndef CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTRENDER_H
#define CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTRENDER_H
#pragma once

#include <IRenderer.h>
#include <IEntityRenderState.h>
#include "Components/IComponentRender.h"
#include "PoolAllocator.h"

// forward declarations.
class CEntityObject;
struct SRendParams;
struct IShaderPublicParams;
class CEntityTimeoutList;
struct AnimEventInstance;

//! Implementation of the entity's rendering component.
//! This is a renderable object that is registered in 3D engine sector.
//! <p>
//! The render component can contain multiple sub object slots.
//! Each slot can have its own relative transformation.
//! Each slot can represent a specific renderable interface (IStatObj,
//! ICharacterInstance, etc). Slots can also form an hierarchical
//! transformation tree by referencing a parent slot for transformation.
class CComponentRender
    : public IComponentRender
    , public IRenderNode
{
public:

    enum EFlags // Must be limited to 32 flags.
    {
        FLAG_CUSTOM_POST_EFFECT = BIT(0), // Has custom post effect ID stored in custom data
        FLAG_BBOX_VALID_LOCAL  = BIT(1),
        FLAG_BBOX_FORCED       = BIT(2),
        FLAG_BBOX_INVALID      = BIT(3),
        FLAG_HIDDEN            = BIT(4), // If render component is hidden.
        //FLAG_HAS_ENV_LIGHTING  = 0x0020, // If render component have environment lighting.
        FLAG_UPDATE            = BIT(5), // If render component needs to be updated.
        FLAG_NOW_VISIBLE       = BIT(6), // If render component currently visible.
        FLAG_REGISTERED_IN_3DENGINE = BIT(7), // If render component have been registered in 3d engine.
        FLAG_POST_INIT         = BIT(8), // If render component have received Post init event.
        FLAG_HAS_LIGHTS        = BIT(9), // If render component has some lights.
        FLAG_GEOMETRY_MODIFIED = BIT(10), // Geometry for this render slot was modified at runtime.
        FLAG_HAS_CHILDRENDERNODES = BIT(11), // If render component contains child render nodes
        FLAG_HAS_PARTICLES     = BIT(12), // If render component contains particle emitters
        FLAG_SHADOW_DISSOLVE   = BIT(13), // If render component requires dissolving shadows
        FLAG_IGNORE_HUD_INTERFERENCE_FILTER  = BIT(15), // HUD render component ignores hud interference filter post effect settings
        FLAG_ANIMATE_OFFSCREEN_SHADOW  = BIT(23), // Update the animation if object drawn in the shadow pass
        FLAG_EXECUTE_AS_JOB_FLAG = BIT(25), // set if this render component can be executed as a Job from the 3DEngine
        FLAG_RECOMPUTE_EXECUTE_AS_JOB_FLAG = BIT(26), // set if the slots changed, to recheck if this render component can execute as a job
    };

    CComponentRender();
    ~CComponentRender();

    EERType GetRenderNodeType();

    //////////////////////////////////////////////////////////////////////////
    // IComponent implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual bool CanExecuteRenderAsJob();

    virtual void Initialize(const SComponentInitializer& init);
    virtual void ProcessEvent(SEntityEvent& event);

    virtual void Done() {};
    // Called when the subsystem initialize.
    virtual void Reload(IEntity* pEntity, SEntitySpawnParams& params);
    virtual void UpdateComponent(SEntityUpdateContext& ctx);
    virtual void SerializeXML(XmlNodeRef& entityNode, bool bLoading);
    virtual void Serialize(TSerialize ser);
    virtual bool NeedSerialize();
    virtual bool GetSignature(TSerialize signature);
    //////////////////////////////////////////////////////////////////////////

    virtual void SetMinSpec(int nMinSpec);
    virtual bool GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const override;
    float GetFirstLodDistance() const override { return m_fLodDistance; }
    void UpdateLodDistance(const SFrameLodInfo& frameLodInfo);

    virtual IPhysicalEntity* GetBranchPhys(int idx, int nSlot = 0) { IFoliage* pFoliage = GetFoliage(); return pFoliage ? pFoliage->GetBranchPhysics(idx) : 0; }
    virtual IFoliage* GetFoliage(int nSlot = 0);


    //////////////////////////////////////////////////////////////////////////
    // IComponentRender implementation.
    //////////////////////////////////////////////////////////////////////////

    // Description:
    //    Get world bounds of render component.
    // Arguments:
    //    bounds - Returns Bounding box in world space.
    void GetWorldBounds(AABB& bounds) override;

    // Description:
    //    Get local space int the entity bounds of render component.
    // Arguments:
    //    bounds - Returns Bounding box in local entity space.
    void GetLocalBounds(AABB& bounds) override;

    // Description:
    //    Force local bounds.
    // Arguments:
    //    bounds - Bounding box in local space.
    //    bDoNotRecalculate - when set to true entity will never try to recalculate local bounding box set by this call.
    void SetLocalBounds(const AABB& bounds, bool bDoNotRecalculate) override;

    // Invalidates local or world space bounding box.
    void InvalidateLocalBounds() override;

    // Description:
    //     Retrieve an actual material used for rendering specified slot.
    ///    Will return custom applied material or if custom material not set will return an actual material assigned to the slot geometry.
    // Arguments:
    //     nSlot - Slot to query used material from, if -1 material will be taken from the first renderable slot.
    // Return:
    //     Material used for rendering, or NULL if slot is not rendered.
    _smart_ptr<IMaterial> GetRenderMaterial(int nSlot = -1) override;

    // Description:
    //     Assign custom material to the slot.
    // Arguments:
    //     nSlot - Slot to apply material to.
    void SetSlotMaterial(int nSlot, _smart_ptr<IMaterial> pMaterial) override;

    // Description:
    //     Retrieve slot's custom material (This material Must have been applied before with the SetSlotMaterial).
    // Arguments:
    //     nSlot - Slot to query custom material from.
    // Return:
    //     Custom material applied on the slot.
    _smart_ptr<IMaterial> GetSlotMaterial(int nSlot) override;

    // Description:
    //    Retrieve engine render node, used to render this entity.
    IRenderNode* GetRenderNode() override
    {
        return this;
    }

    // Description:
    //    Retrieve and optionally create a shaders public params for this render component.
    // Arguments:
    //    bCreate - If Shader public params are not created for this entity, they will be created.
    IShaderPublicParams* GetShaderPublicParams(bool bCreate = true) override;

    // Description:
    //    Add a render component callback
    void AddShaderParamCallback(IShaderParamCallbackPtr pCallback) override;

    // Description:
    //    Remove a render component callback
    bool RemoveShaderParamCallback(IShaderParamCallbackPtr pCallback) override;

    // Description
    //      Check which shader param callback should be activated or disactived depending on
    //      the current entity state
    void CheckShaderParamCallbacks() override;

    // Description
    //      Clears all the shader param callback from this entity
    void ClearShaderParamCallbacks() override;
    // Description:
    //     Assign sub-object hide mask to slot.
    // Arguments:
    //     nSlot - Slot to apply hide mask to.
    void SetSubObjHideMask(int nSlot, uint64 nSubObjHideMask) override;
    uint64 GetSubObjHideMask(int nSlot) const override;

    // Description:
    //    updates indirect lighting for children
    void UpdateIndirLightForChildren(){};

    // Description:
    //    sets material layers masks
    void SetMaterialLayersMask(uint8 nMtlLayersMask) override;
    uint8 GetMaterialLayersMask() const override
    {
        return m_nMaterialLayers;
    }

    // Description:
    //    overrides material layers blend amount
    void SetMaterialLayersBlend(uint32 nMtlLayersBlend) override
    {
        m_nMaterialLayersBlend = nMtlLayersBlend;
    }

    uint32 GetMaterialLayersBlend() const override
    {
        return m_nMaterialLayersBlend;
    }

    // Description:
    //    sets custom post effect
    void SetCustomPostEffect(const char* pPostEffectName) override;

    // Description:
    //    sets hud render component to ignore hud interference filter
    void SetIgnoreHudInterferenceFilter(const bool bIgnoreFiler) override
    {
        if (bIgnoreFiler)
        {
            AddFlags(FLAG_IGNORE_HUD_INTERFERENCE_FILTER);
        }
        else
        {
            ClearFlags(FLAG_IGNORE_HUD_INTERFERENCE_FILTER);
        }
    }

    // Description:
    //    set/get hud silhouetes params
    // Notes:
    // Allows to adjust hud silhouettes params:
    // binocular view uses color and alpha for blending
    void SetHUDSilhouettesParams(float r, float g, float b, float a) override
    {
        m_nHUDSilhouettesParams =  (uint32) (int_round(r * 255.0f) << 24) | (int_round(g * 255.0f) << 16) | (int_round(b * 255.0f) << 8) | (int_round(a * 255.0f));
    }

    uint32 GetHUDSilhouettesParams() const override { return m_nHUDSilhouettesParams; }

    // Description:
    //    set/get shadow dissolving (fade out for phantom perk etc)
    void SetShadowDissolve(bool enable) override
    {
        if (enable)
        {
            AddFlags(FLAG_SHADOW_DISSOLVE);
        }
        else
        {
            ClearFlags(FLAG_SHADOW_DISSOLVE);
        }
    }

    bool GetShadowDissolve() const override { return CheckFlags(FLAG_SHADOW_DISSOLVE); }

    // Description:
    //  return the last time (as set by the system timer) when the render component was last seen.
    float   GetLastSeenTime() const override { return m_fLastSeenTime; }

    // Description:
    //  return true if entity visarea was visible during last frames
    bool IsRenderComponentVisAreaVisible() const override;

    // Description:
    //  Removes all slots from the render component
    void ClearSlots() override
    {
        FreeAllSlots();
    }

    // Description:
    //    Set the view distance multiplier on the render node. Valid input range
    //    [0.0f - 100.0f], other values will be clamped to this range.
    virtual void SetViewDistanceMultiplier(float fViewDistanceMultiplier);

    // Description:
    //    Sets the LodRatio on the render node.
    void SetLodRatio(int nLodRatio) override;

    // Description:
    //    Returns IStatObj* if entity is set up as compound object, otherwise returns nullptr.
    // Note:
    //    Promoted function declaration to IComponentRender so it could be accessed by CBreakableManager.
    IStatObj* GetCompoundObj() const override;

    // Description:
    //    Sets flags, completely replaces all flags which are already set in the entity.
    // Note:
    //    Promoted function to IEntity for access by CScriptBind_Entity.
    void SetFlags(uint32 flags) override
    {
        m_nFlags = flags;
    }

    // Note:
    //    Promoted function to IEntity for access by CScriptBind_Entity.
    uint32 GetFlags() override
    {
        return m_nFlags;
    }

    // Note:
    //    Promoted function to IComponentRender for access by CComponentPhysics
    bool PhysicalizeFoliage(bool bPhysicalize = true, int iSource = 0, int nSlot = 0) override;

    // Note:
    //    Promoted function to IComponentRender for access by CComponentPhysics
    int SetSlotGeometry(int nSlot, IStatObj* pStatObj) override;

    // Description:
    //    Invalidates local or world space bounding box.
    // Note:
    //    Promoted function to IComponentRender for access by CComponentPhysics
    void InvalidateBounds(bool bLocal, bool bWorld) override;

    // Description:
    //
    void SetLastSeenTime(float fNewTime) override
    {
        m_fLastSeenTime = fNewTime;
    }

    // Description:
    //
    void UpdateEntityFlags() override;

    // Description:
    // Called after constructor.
    void PostInit() override;

    // Description:
    // Retrieves the character in the specified slot.
    ICharacterInstance* GetCharacter(int nSlot) override;

    // Description:
    // Allocate a new object slot.
    CEntityObject* AllocSlot(int nIndex) override;

    // Description:
    // Returns true if there is a valid slot at the specified index.
    bool IsSlotValid(int nIndex) const override
    {
        return nIndex >= 0 && nIndex < (int)m_slots.size() && m_slots[nIndex] != NULL;
    }

    // Description:
    // Releases the entity object in the specified slot and removes the slot.
    void FreeSlot(int nIndex) override;

    // Description:
    // Number of slots allocated on this entity.
    int  GetSlotCount() const override;

    // Description:
    //
    CEntityObject* GetSlot(int nIndex) const override
    {
        return (nIndex >= 0 && nIndex < (int)m_slots.size()) ? m_slots[nIndex] : NULL;
    }

    // Description:
    // Get the CEntityObject in the specified slot.
    bool GetSlotInfo(int nIndex, SEntitySlotInfo& slotInfo) const override;

    // Description:
    // Returns world transformation matrix of the object slot.
    const Matrix34& GetSlotWorldTM(int nIndex) const override;

    // Description:
    // Returns local relative to host entity transformation matrix of the object slot.
    const Matrix34& GetSlotLocalTM(int nIndex, bool bRelativeToParent) const override;

    // Description:
    // Set local transformation matrix of the object slot.
    void SetSlotLocalTM(int nIndex, const Matrix34& localTM, int nWhyFlags = 0) override;

    // Description:
    // Set camera space position of the object slot
    void SetSlotCameraSpacePos(int nIndex, const Vec3& cameraSpacePos) override;

    // Description:
    // Get camera space position of the object slot
    void GetSlotCameraSpacePos(int nSlot, Vec3& cameraSpacePos) const override;

    // Description:
    // Establish slot parenting.
    bool SetParentSlot(int nParentIndex, int nChildIndex) override;

    // Description:
    // Set the flags for the specified slot.
    void SetSlotFlags(int nSlot, uint32 nFlags) override;

    // Description:
    // Retrieve the flags for the specified slot.
    uint32 GetSlotFlags(int nSlot) const override;

    // Description:
    // Set a character instance on the specified slot.
    int SetSlotCharacter(int nSlot, ICharacterInstance* pCharacter) override;

    // Description:
    // Get the static object data in the specified slot.
    IStatObj* GetStatObj(int nSlot) override;

    // Description:
    // Retrieves the particle emitter in the specified slot.
    IParticleEmitter* GetParticleEmitter(int nSlot) override;

#if defined(USE_GEOM_CACHES)
    // Description:
    // Get the geom cache render node in the specified slot if available.
    IGeomCacheRenderNode* GetGeomCacheRenderNode(unsigned int nSlot, Matrix34A* pMatrix = NULL, bool bReturnOnlyVisible = false) override;

    // Description:
    // Get the geom cache render node in the specified slot if available.
    IGeomCacheRenderNode* GetGeomCacheRenderNode(int nSlot) override;

    // Description:
    // Loads the specified geom cache from file into the specified slot.
    int LoadGeomCache(int nSlot, const char* sFilename) override;
#endif

    // Description:
    // Loads static geometry from file into the specified slot.
    int LoadGeometry(int nSlot, const char* sFilename, const char* sGeomName = NULL, int nLoadFlags = 0) override;

    // Description:
    // Loads a character from file into the specified slot.
    int LoadCharacter(int nSlot, const char* sFilename, int nLoadFlags = 0) override;

    // Description:
    // Loads cloud data from file into the specified slot.
    int LoadCloud(int nSlot, const char* sFilename) override;

    // Description:
    // Configure the cloud properties for the specified slot.
    int SetCloudMovementProperties(int nSlot, const SCloudMovementProperties& properties) override;

    // Description:
    // Load volume data from file into the specified slot.
    int LoadVolumeObject(int nSlot, const char* sFilename) override;

    // Description:
    // Configure the volume properties for the specified slot.
    int SetVolumeObjectMovementProperties(int nSlot, const SVolumeObjectMovementProperties& properties) override;

    // Description:
    // Configure volumetric fog properties for volume in the specified slot.
    int LoadFogVolume(int nSlot, const SFogVolumeProperties& properties) override;

    // Description:
    // Apply fade density to the volume in the specified slot.
    int FadeGlobalDensity(int nSlot, float fadeTime, float newGlobalDensity) override;

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
    // Description:
    // Created a prism object in the specified slot.
    int LoadPrismObject(int nSlot) override;
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

    // Description:
    // Sets the provided particle emitter into the specified slot.
    int SetParticleEmitter(int nSlot, IParticleEmitter* pEmitter, bool bSerialize = false) override;

    // Description:
    // Spawns a particle effect from the particle emitter in the specified slot.
    int LoadParticleEmitter(int nSlot, IParticleEffect* pEffect, SpawnParams const* params = NULL, bool bPrime = false, bool bSerialize = false) override;

    // Description:
    // Creates a light source in the specified slot.
    int LoadLight(int nSlot, CDLight* pLight, uint16 layerId) override;

    // Description:
    // Draws debug information on-screen.
    void  DebugDraw(const SGeometryDebugDrawInfo& info) override;
    //////////////////////////////////////////////////////////////////////////

    void QueueSlotGeometryChange(int nSlot, IStatObj* pStatObj);

    //////////////////////////////////////////////////////////////////////////
    // Slots.
    //////////////////////////////////////////////////////////////////////////

    CEntityObject* GetParentSlot(int nIndex) const;

    void FreeAllSlots();
    void ExpandCompoundSlot0(); // if there's a compound cgf in slot 0, expand it into subobjects (slots)

    // Register render component in 3d engine.
    void RegisterForRendering(bool bEnable);
    static void RegisterCharactersForRendering();

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // IRenderNode interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetLayerId(uint16 nLayerId);
    virtual void SetMaterial(_smart_ptr<IMaterial> pMat);
    virtual _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = NULL);
    virtual _smart_ptr<IMaterial> GetMaterialOverride() { return m_pMaterial; }
    virtual const char* GetEntityClassName() const;
    virtual Vec3 GetPos(bool bWorldOnly = true) const;
    virtual const Ang3& GetAngles(int realA = 0) const;
    virtual float GetScale() const;
    virtual const char* GetName() const;
    virtual void GetRenderBBox(Vec3& mins, Vec3& maxs);
    virtual void GetBBox(Vec3& mins, Vec3& maxs);
    virtual void FillBBox(AABB& aabb);
    virtual bool HasChanged() { return false; }
    virtual void Render(const SRendParams& EntDrawParams, const SRenderingPassInfo& passInfo);
    virtual IStatObj* GetEntityStatObj(unsigned int nPartId, unsigned int nSubPartId, Matrix34A* pMatrix = NULL, bool bReturnOnlyVisible = false);
    virtual void SetEntityStatObj(unsigned int nSlot, IStatObj* pStatObj, const Matrix34A* pMatrix = NULL) {};
    virtual _smart_ptr<IMaterial> GetEntitySlotMaterial(unsigned int nPartId, bool bReturnOnlyVisible, bool* pbDrawNear);
    virtual ICharacterInstance* GetEntityCharacter(unsigned int nSlot, Matrix34A* pMatrix = NULL, bool bReturnOnlyVisible = false);
    virtual IPhysicalEntity* GetPhysics() const;
    virtual void SetPhysics(IPhysicalEntity* pPhys) {}
    virtual void PreloadInstanceResources(Vec3 vPrevPortalPos, float fPrevPortalDistance, float fTime) {}

    void Render_JobEntry(const SRendParams& inRenderParams, const SRenderingPassInfo& passInfo);

    virtual float GetMaxViewDist();


    virtual void SetOpacity(float fAmount)
    {
        m_nOpacity = int_round(clamp_tpl<float>(fAmount, 0.0f, 1.0f) * 255);
    }

    virtual float GetOpacity() const {  return (float) m_nOpacity / 255.0f; }

    virtual const AABB GetBBox() const { return m_WSBBox; }
    virtual void SetBBox(const AABB& WSBBox) { m_WSBBox = WSBBox; }
    virtual void OffsetPosition(const Vec3& delta) {}

    //////////////////////////////////////////////////////////////////////////

    // Internal slot access function.
    ILINE CEntityObject* Slot(int nIndex) const { assert(nIndex >= 0 && nIndex < (int)m_slots.size()); return m_slots[nIndex]; }

    //////////////////////////////////////////////////////////////////////////
    // Flags
    //////////////////////////////////////////////////////////////////////////
    ILINE void   AddFlags(uint32 flagsToAdd) { SetFlags(m_nFlags | flagsToAdd); }
    ILINE void   ClearFlags(uint32 flagsToClear) { SetFlags(m_nFlags & ~flagsToClear); }
    ILINE bool   CheckFlags(uint32 flagsToCheck) const { return (m_nFlags & flagsToCheck) == flagsToCheck; }
    //////////////////////////////////////////////////////////////////////////

    // Change custom material.
    void SetCustomMaterial(_smart_ptr<IMaterial> pMaterial);
    // Get assigned custom material.
    _smart_ptr<IMaterial> GetCustomMaterial() { return m_pMaterial; }

    //////////////////////////////////////////////////////////////////////////
    // Custom new/delete.
    //////////////////////////////////////////////////////////////////////////
    void* operator new(size_t nSize);
    void operator delete(void* ptr);

    static void SetTimeoutList(CEntityTimeoutList* pList)   {s_pTimeoutList = pList; }

    static ILINE void SetViewDistMin(float fViewDistMin)                  { s_fViewDistMin = fViewDistMin; }
    static ILINE void SetViewDistRatio(float fViewDistRatio)              { s_fViewDistRatio = fViewDistRatio; }
    static ILINE void SetViewDistRatioCustom(float fViewDistRatioCustom)  { s_fViewDistRatioCustom = fViewDistRatioCustom; }
    static ILINE void SetViewDistRatioDetail(float fViewDistRatioDetail)  { s_fViewDistRatioDetail = fViewDistRatioDetail; }

    static int AnimEventCallback(ICharacterInstance* pCharacter, void* userdata);

private:
    int GetSlotId(CEntityObject* pSlot) const;
    void CalcLocalBounds();
    void OnEntityXForm(int nWhyFlags);
    void OnHide(bool bHide);
    void OnReset();

    // Get existing slot or make a new slot if not exist.
    // Is nSlot is negative will allocate a new available slot and return it Index in nSlot parameter.
    CEntityObject* GetOrMakeSlot(int& nSlot);

    I3DEngine* GetI3DEngine() { return gEnv->p3DEngine; }
    bool CheckCharacterForMorphs(ICharacterInstance* pCharacter);
    void AnimationEvent(ICharacterInstance* pCharacter, const AnimEventInstance& event);
    void UpdateMaterialLayersRendParams(SRendParams& pRenderParams, const SRenderingPassInfo& passInfo);
    bool IsMovableByGame() const;

private:
    friend class CEntityObject;

    static float s_fViewDistMin;
    static float s_fViewDistRatio;
    static float s_fViewDistRatioCustom;
    static float s_fViewDistRatioDetail;
    static std::vector<CComponentRender*> s_arrCharactersToRegisterForRendering;

    //////////////////////////////////////////////////////////////////////////
    // Variables.
    //////////////////////////////////////////////////////////////////////////
    // Host entity.
    IEntity* m_pEntity;

    // Object Slots.
    typedef std::vector<CEntityObject*> Slots;
    Slots m_slots;

    // Local bounding box of render component.
    AABB m_localBBox;

    //! Override material.
    _smart_ptr<IMaterial> m_pMaterial;

    // per instance shader public params
    _smart_ptr<IShaderPublicParams> m_pShaderPublicParams;

    // per instance callback object
    typedef std::vector<IShaderParamCallbackPtr> TCallbackVector;
    TCallbackVector m_Callbacks;

    // Time passed since this entity was seen last time  (wrong: this is an absolute time, todo: fix float absolute time values)
    float       m_fLastSeenTime;

    static CEntityTimeoutList* s_pTimeoutList;
    EntityId m_entityId;

    uint32 m_nEntityFlags;

    // Render component flags.
    uint32 m_nFlags;

    // Hud silhouetes param
    uint32 m_nHUDSilhouettesParams;
    // Material layers blending amount
    uint32 m_nMaterialLayersBlend;

    typedef std::pair<int, IStatObj*> SlotGeometry;
    typedef std::vector<SlotGeometry> SlotGeometries;
    SlotGeometries m_queuedGeometryChanges;

    float m_fCustomData[4];

    float m_fLodDistance;
    uint8 m_nOpacity;
    uint8 m_nCustomData;

    AABB m_WSBBox;
};

extern stl::PoolAllocatorNoMT<sizeof(CComponentRender)>* g_Alloc_ComponentRender;

//////////////////////////////////////////////////////////////////////////
DECLARE_COMPONENT_POINTERS(CComponentRender);

//////////////////////////////////////////////////////////////////////////
inline void* CComponentRender::operator new(size_t nSize)
{
    void* ptr = g_Alloc_ComponentRender->Allocate();
    return ptr;
}

//////////////////////////////////////////////////////////////////////////
inline void CComponentRender::operator delete(void* ptr)
{
    if (ptr)
    {
        g_Alloc_ComponentRender->Deallocate(ptr);
    }
}

#endif // CRYINCLUDE_CRYENTITYSYSTEM_COMPONENTS_COMPONENTRENDER_H
